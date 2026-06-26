#ifndef IMAGE_SEGMENTER_H
#define IMAGE_SEGMENTER_H

#include "utils/Edmonds.h"        
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "utils/PixelConfiguration.h" 
#include "utils/Filters.h"            

#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <cmath>
#include <algorithm>
#include <queue>

// -----------------------------------------------------------------------------
// Configuração para ajuste fino (Sem recompilar lógica)
// -----------------------------------------------------------------------------
struct SegmentationConfig {
    int blockSize = 4;           // Tamanho do grid de superpixels
    double sobelThreshold = 60.0;// Limite para considerar barreira de gradiente
    double sobelPenalty = 2.0;   // Multiplicador de peso para barreiras
    double colorWeight = 0.5;    // Peso da diferença de cor (L*a*b*)
    double spatialWeight = 3.5;  // Peso da distância física
    double seedPriority = -99999.0; // Custo fortemente negativo para fixar as sementes
};

// -----------------------------------------------------------------------------
// Classe Principal de Segmentação por Edmonds
// -----------------------------------------------------------------------------
class ImageSegmenter {
private:
    int width, height, channels;
    std::unique_ptr<unsigned char[], void(*)(void*)> imgData; // Smart Pointer para gerenciamento do STB
    
    std::vector<unsigned char> sobelData;
    std::vector<SuperPixel> superPixels;
    int gridW, gridH;

    SegmentationConfig config;

    // Calcula o maior gradiente na fronteira entre dois superpixels vizinhos
    double getBoundaryMaxGradient(int x1, int y1, int x2, int y2) const {
        double maxGrad = 0.0;
        int bs = config.blockSize;

        if (x2 > x1) { // Vizinho da Direita
            int boundaryX = std::min(x2 * bs, width - 1);
            int startY = y1 * bs;
            int endY = std::min((y1 + 1) * bs, height);
            
            for (int y = startY; y < endY; ++y) {
                int idx = y * width + boundaryX;
                if (sobelData[idx] > maxGrad) maxGrad = sobelData[idx];
            }
        } else if (y2 > y1) { // Vizinho de Baixo
            int boundaryY = std::min(y2 * bs, height - 1);
            int startX = x1 * bs;
            int endX = std::min((x1 + 1) * bs, width);

            for (int x = startX; x < endX; ++x) {
                int idx = boundaryY * width + x;
                if (sobelData[idx] > maxGrad) maxGrad = sobelData[idx];
            }
        } else { // Diagonal
            int cx = std::min((x1 + 1) * bs, width - 1);
            int cy = std::min((y1 + 1) * bs, height - 1);
            maxGrad = sobelData[cy * width + cx];
        }
        return maxGrad;
    }

public:
    ImageSegmenter() : width(0), height(0), channels(0), imgData(nullptr, stbi_image_free) {}

    // 1. Carregar Imagem
    bool load(const std::string& path) {
        int w, h, c;
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 3); // Força 3 canais internos
        if (!data) {
            std::cerr << "Erro: Nao foi possivel carregar " << path << std::endl;
            return false;
        }
        
        imgData.reset(data); 
        width = w; height = h; channels = 3;
        return true;
    }

    // 2. Configurar Parâmetros
    void setConfig(const SegmentationConfig& cfg) {
        this->config = cfg;
    }

    // Exporta um mapa visual das sementes para depuração
    void saveSeedsDebug(const char* filename, int w, int h, const Seeds& seeds) {
        std::vector<unsigned char> debugImg(w * h * 3, 128); // Cinza neutro de fundo

        for (int idx : seeds.obj) {
            debugImg[idx * 3 + 0] = 0;   // R
            debugImg[idx * 3 + 1] = 255; // G (Verde para Objeto)
            debugImg[idx * 3 + 2] = 0;   // B
        }

        for (int idx : seeds.backgroundObj) {
            debugImg[idx * 3 + 0] = 255; // R (Vermelho para Fundo)
            debugImg[idx * 3 + 1] = 0;   // G
            debugImg[idx * 3 + 2] = 0;   // B
        }

        stbi_write_png(filename, w, h, 3, debugImg.data(), w * 3);
    }

    // Executa o pipeline completo baseado no algoritmo de Edmonds
    void runWithSeeds(const Seeds& seeds, const std::string& outputPath) {
        if (!imgData) {
            std::cerr << "Erro: Nenhuma imagem carregada na classe antes de rodar o algoritmo." << std::endl;
            return;
        }
        
        // Pré-processamento de filtros
        unsigned char* rawBlur = toGaussian_blur(imgData.get(), width, height, 3);
        rawBlur = toGaussian_blur(rawBlur, width, height, 3);

        unsigned char* rawSobel = sobelFilter(rawBlur, width, height, 3);
        apply_gamma(imgData.get(), width, height, 3, 1.5f);
        
        sobelData.assign(rawSobel, rawSobel + (width * height));
        delete[] rawBlur; 
        delete[] rawSobel;

        // Construção do grafo reduzido por Superpixels
        generateSuperPixels();
        std::vector<Edge> edges = buildGraph();

        int ghostRoot = superPixels.size();
        int nextId = edges.size();

        // Conecta o nó fantasma (Ghost Root) às sementes do usuário
        for(int s : seeds.backgroundObj) {
            int spID = ((s / width) / config.blockSize) * gridW + ((s % width) / config.blockSize);
            if (spID < (int)superPixels.size()) {
                edges.push_back({ghostRoot, spID, config.seedPriority, nextId++});
            }
        }

        for(int s : seeds.obj) {
            int spID = ((s / width) / config.blockSize) * gridW + ((s % width) / config.blockSize);
            if (spID < (int)superPixels.size()) {
                edges.push_back({ghostRoot, spID, config.seedPriority, nextId++});
            }
        }

        // Resolve a Arborescência de Custo Mínimo via Edmonds
        std::vector<Edge> tree = Edmonds::solve(ghostRoot + 1, edges, ghostRoot);
        
        // Propaga os rótulos e renderiza a imagem segmentada final
        renderFromTreeAndSeeds(tree, seeds, outputPath);
        saveDebugBorders(tree, seeds, "output/edmonds/bordas_edmonds.png");
    }

    // Desenha as bordas dos segmentos gerados por Edmonds sobrepostos na imagem
    void saveDebugBorders(const std::vector<Edge>& tree, const Seeds& seeds, const std::string& filename) {
        int numSPs = superPixels.size();
        
        std::vector<std::vector<int>> adj(numSPs);
        for (const auto& e : tree) {
            if (e.u < numSPs && e.v < numSPs) adj[e.u].push_back(e.v);
        }

        std::vector<int> labels(numSPs, 2); // 2 = Desconhecido
        std::queue<int> q;

        auto mapPixelToSP = [&](int pixelIdx) {
            int py = pixelIdx / width;
            int px = pixelIdx % width;
            return (py / config.blockSize) * gridW + (px / config.blockSize);
        };

        for (int s : seeds.obj) {
            int sp = mapPixelToSP(s);
            if (sp < numSPs && labels[sp] == 2) { labels[sp] = 1; q.push(sp); }
        }
        for (int s : seeds.backgroundObj) {
            int sp = mapPixelToSP(s);
            if (sp < numSPs && labels[sp] == 2) { labels[sp] = 0; q.push(sp); }
        }

        while(!q.empty()) {
            int u = q.front(); q.pop();
            for (int v : adj[u]) {
                if (labels[v] == 2) {
                    labels[v] = labels[u];
                    q.push(v);
                }
            }
        }

        std::vector<unsigned char> output(width * height * 3);
        int bs = config.blockSize;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 3;
                
                int spCurrent = (y / bs) * gridW + (x / bs);
                if (spCurrent >= numSPs) spCurrent = numSPs - 1;
                int lblCurrent = labels[spCurrent];

                bool isBorder = false;
                int checkX[] = {1, 0};
                int checkY[] = {0, 1};

                for(int k = 0; k < 2; ++k) {
                    int nx = x + checkX[k];
                    int ny = y + checkY[k];

                    if(nx < width && ny < height) {
                        int spNeighbor = (ny / bs) * gridW + (nx / bs);
                        if (spNeighbor < numSPs) {
                            if (labels[spNeighbor] != lblCurrent) {
                                isBorder = true;
                                break;
                            }
                        }
                    }
                }

                if (isBorder) {
                    // Magenta Neon para destacar as bordas no relatório
                    output[idx]   = 255;
                    output[idx+1] = 0;
                    output[idx+2] = 255;
                } else {
                    unsigned char* rawImg = imgData.get();
                    output[idx]   = (unsigned char)(rawImg[idx] * 0.7);
                    output[idx+1] = (unsigned char)(rawImg[idx+1] * 0.7);
                    output[idx+2] = (unsigned char)(rawImg[idx+2] * 0.7);
                }
            }
        }

        stbi_write_png(filename.c_str(), width, height, 3, output.data(), width * 3);
        std::cout << "Debug de bordas salvo em: " << filename << std::endl;
    }

private:
    // Agrupa os pixels locais gerando as métricas estatísticas de cor do superpixel
    void generateSuperPixels() {
        int bs = config.blockSize;
        gridW = (width + bs - 1) / bs;
        gridH = (height + bs - 1) / bs;
        int numSP = gridW * gridH;

        superPixels.resize(numSP);

        for (int gy = 0; gy < gridH; ++gy) {
            for (int gx = 0; gx < gridW; ++gx) {
                int id = gy * gridW + gx;
                
                double sumL=0, sumA=0, sumB=0;
                double sumR=0, sumG=0, sumB_vis=0;
                int count = 0;

                int startY = gy * bs;
                int endY = std::min((gy + 1) * bs, height);
                int startX = gx * bs;
                int endX = std::min((gx + 1) * bs, width);

                unsigned char* rawData = imgData.get();
                for (int y = startY; y < endY; ++y) {
                    for (int x = startX; x < endX; ++x) {
                        int idx = (y * width + x) * 3;
                        unsigned char r = rawData[idx];
                        unsigned char g = rawData[idx+1];
                        unsigned char b = rawData[idx+2];

                        sumR += r; sumG += g; sumB_vis += b;
                        
                        CIELAB lab = RGBtoLab(r, g, b);
                        sumL += lab.L; sumA += lab.a; sumB += lab.b;
                        count++;
                    }
                }
                
                if (count == 0) count = 1;

                superPixels[id].id = id;
                superPixels[id].L = sumL / count;
                superPixels[id].a = sumA / count;
                superPixels[id].b = sumB / count;
                superPixels[id].avgR = (unsigned char)(sumR / count);
                superPixels[id].avgG = (unsigned char)(sumG / count);
                superPixels[id].avgB = (unsigned char)(sumB_vis / count);
            }
        }
    }

    // Monta o grafo direcionado ponderado entre os superpixels adjacentes (Conectividade-4 adaptada)
    std::vector<Edge> buildGraph() {
        std::vector<Edge> edges;
        edges.reserve(superPixels.size() * 4);

        int dx[] = {1,  0,  1, -1};
        int dy[] = {0,  1,  1,  1};
        double distFactor[] = {1.0, 1.0, 1.414, 1.414}; 
        
        int edgeIdCounter = 0;

        for (int gy = 0; gy < gridH; ++gy) {
            for (int gx = 0; gx < gridW; ++gx) {
                int u = gy * gridW + gx;

                for (int i = 0; i < 4; ++i) {
                    int nx = gx + dx[i];
                    int ny = gy + dy[i];

                    if (nx >= 0 && nx < gridW && ny >= 0 && ny < gridH) {
                        int v = ny * gridW + nx;

                        double dL = superPixels[u].L - superPixels[v].L;
                        double dA = superPixels[u].a - superPixels[v].a;
                        double dB = superPixels[u].b - superPixels[v].b;
                        double colorDist = std::sqrt(dL*dL + dA*dA + dB*dB);

                        double barrier = getBoundaryMaxGradient(gx, gy, nx, ny);
                        
                        double edgePenalty = 0.0;
                        if (barrier > config.sobelThreshold) {
                            edgePenalty = barrier * config.sobelPenalty;
                        }

                        double weight = (colorDist * config.colorWeight) + 
                                        edgePenalty + 
                                        (distFactor[i] * config.spatialWeight);

                        // Como o algoritmo exige grafo direcionado, adicionamos os dois arcos direcionados
                        edges.push_back({u, v, weight, edgeIdCounter++});
                        edges.push_back({v, u, weight, edgeIdCounter++});
                    }
                }
            }
        }
        return edges;
    }

    // Reconstrói as subárvores e colore a imagem segmentada em Objeto vs Fundo
    void renderFromTreeAndSeeds(const std::vector<Edge>& tree, const Seeds& seeds, const std::string& filename) {
        int numSPs = superPixels.size();
        
        std::vector<std::vector<int>> adj(numSPs);
        for (const auto& e : tree) {
            if (e.u < numSPs && e.v < numSPs) {
                adj[e.u].push_back(e.v);
            }
        }

        std::vector<int> labels(numSPs, 2);
        std::queue<int> q;

        auto mapPixelToSP = [&](int pixelIdx) {
            int py = pixelIdx / width;
            int px = pixelIdx % width;
            return (py / config.blockSize) * gridW + (px / config.blockSize);
        };

        for (int s : seeds.obj) {
            int sp = mapPixelToSP(s);
            if (sp < numSPs && labels[sp] == 2) { labels[sp] = 1; q.push(sp); }
        }
        for (int s : seeds.backgroundObj) {
            int sp = mapPixelToSP(s);
            if (sp < numSPs && labels[sp] == 2) { labels[sp] = 0; q.push(sp); }
        }

        while(!q.empty()) {
            int u = q.front(); q.pop();
            for (int v : adj[u]) {
                if (labels[v] == 2) {
                    labels[v] = labels[u];
                    q.push(v);
                }
            }
        }

        std::vector<unsigned char> output(width * height * 3);
        int bs = config.blockSize;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int sp = (y / bs) * gridW + (x / bs);
                if (sp >= numSPs) sp = numSPs - 1;

                int idx = (y * width + x) * 3;
                int lbl = labels[sp];

                if (lbl == 1) { // Objeto -> Pinta com a cor real média da região
                    output[idx]   = superPixels[sp].avgR;
                    output[idx+1] = superPixels[sp].avgG;
                    output[idx+2] = superPixels[sp].avgB;
                } else { // Fundo -> Mascara para preto absoluto
                    output[idx]   = 0;
                    output[idx+1] = 0;
                    output[idx+2] = 0;
                }
            }
        }

        stbi_write_png(filename.c_str(), width, height, 3, output.data(), width * 3);
    }
};

#endif // IMAGE_SEGMENTER_H