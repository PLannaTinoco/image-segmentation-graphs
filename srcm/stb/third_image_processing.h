#ifndef IFT_SEGMENTATION_H
#define IFT_SEGMENTATION_H

#include <vector>
#include <queue>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <limits>

#include "../graph/DirectedGraph.h"
#include "../utils/Filters.h"
#include "utils/PixelConfiguration.h" // Garante acesso à estrutura Seeds e CIELAB

// Estrutura auxiliar para a Fila de Prioridade do IFT
struct IFTNode {
    int vertexId;
    double cost;

    // Operador de comparação inverso para transformar a std::priority_queue em um Min-Heap
    bool operator>(const IFTNode& other) const {
        return cost > other.cost;
    }
};

/**
 * @brief Cria um grafo direcionado ponderado a partir de uma imagem digital.
 * @param imagePath Caminho para o arquivo de imagem.
 * @param graph Grafo direcionado que será populado.
 * @return Ponteiro para os dados brutos da imagem (necessário liberar com stbi_image_free mais tarde).
 */
unsigned char* create_graph(const char * imagePath, DirectedGraph &graph) {
    int height, width, original_channels;
    
    // Força o carregamento com 3 canais (RGB)
    unsigned char * imageData = stbi_load(imagePath, &width, &height, &original_channels, 3); 

    if(imageData == nullptr) {
        std::cerr << "Erro: Imagem vazia ou invalida " << imagePath << std::endl;
        return nullptr;
    }

    // Suavização Gaussiana tripla para redução de ruídos espúrios e melhor definição de borda
    imageData = toGaussian_blur(imageData, width, height, 3); 
    imageData = toGaussian_blur(imageData, width, height, 3);
    imageData = toGaussian_blur(imageData, width, height, 3);
    
    // Extração de magnitudes de borda via Filtro de Sobel
    unsigned char* sobelData = sobelFilter(imageData, width, height, 3);

    const long totalSize = height * width;
    graph.inicializar(totalSize, width, height);

    const double W = 1500.0; // Peso modulador da penalidade do Sobel
    
    // Conectividade-4 (Vizinhos imediatos: Direita, Esquerda, Baixo, Cima)
    const int dx[] = {1, -1, 0, 0};
    const int dy[] = {0, 0, 1, -1};
 
    const int channels_in_memory = 3;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned long index = (y * width + x) * channels_in_memory;
            CIELAB current = RGBtoLab(imageData[index], imageData[index + 1], imageData[index + 2]);
            
            for (int i = 0; i < 4; i++) {
                int nextX = x + dx[i];
                int nextY = y + dy[i];

                if (nextX >= 0 && nextX < width && nextY >= 0 && nextY < height) {
                    unsigned long next_index = (nextY * width + nextX) * channels_in_memory;
                    CIELAB next = RGBtoLab(imageData[next_index], imageData[next_index + 1], imageData[next_index + 2]);

                    const int current_vertex_id = y * width + x;
                    const int next_vertex_id = nextY * width + nextX;

                    unsigned char sobel_current = sobelData[current_vertex_id];
                    unsigned char sobel_next = sobelData[next_vertex_id];
                    
                    // A penalidade avalia a transição sobre a crista do gradiente
                    double sobel_penalty = std::max(sobel_current, sobel_next);

                    // Cálculo da distância Euclidiana Euclidiana no espaço perceptivo CIELAB
                    double color_dist_sq = std::pow(current.L - next.L, 2) + 
                                           std::pow(current.a - next.a, 2) + 
                                           std::pow(current.b - next.b, 2); 

                    double weight = color_dist_sq + (W * sobel_penalty);
                    
                    // Insere o arco direcionado no grafo
                    graph.insert(current_vertex_id, next_vertex_id, weight);
                }
            }
        }
    }

    // CRUCIAL: Libera a memória alocada dinamicamente pelo Sobel para evitar vazamento de memória RAM
    delete[] sobelData; 

    return imageData;
}

/**
 * @brief Executa o algoritmo central da Image Foresting Transform (Geral de Dijkstra)
 * @param graph Grafo populado com os pesos das transições de pixels.
 * @param seeds Estrutura contendo os índices dos pixels sementes (objeto e fundo).
 * @param labels Vetor de saída que armazenará os rótulos finais (1 para objeto, 0 para fundo).
 */
void runIFT(const DirectedGraph& graph, const Seeds& seeds, std::vector<int>& labels) {
    int numVertices = graph.getSize(); // Alinhar com o método correspondente da sua classe
    
    // Inicializa vetores de controle do Dijkstra
    std::vector<double> cost(numVertices, std::numeric_limits<double>::infinity());
    labels.assign(numVertices, -1); // -1 indica nó não explorado

    // Min-Priority Queue para processamento guloso dos custos de caminho
    std::priority_queue<IFTNode, std::vector<IFTNode>, std::greater<IFTNode>> pq;

    // 1. Inicializa as Sementes de Objeto (Rótulo = 1, Custo Inicial = 0)
    for (int s : seeds.obj) {
        if (s >= 0 && s < numVertices) {
            cost[s] = 0.0;
            labels[s] = 1;
            pq.push({s, 0.0});
        }
    }

    // 2. Inicializa as Sementes de Fundo (Rótulo = 0, Custo Inicial = 0)
    for (int s : seeds.backgroundObj) {
        if (s >= 0 && s < numVertices) {
            cost[s] = 0.0;
            labels[s] = 0;
            pq.push({s, 0.0});
        }
    }

    // 3. Loop de Propagação do Mapa de Custos (Coração do IFT)
    while (!pq.empty()) {
        IFTNode current = pq.top();
        pq.pop();

        int u = current.vertexId;

        // Se encontramos um caminho com custo maior do que o já otimizado, descarta
        if (current.cost > cost[u]) continue;

        // Explora as adjacências do vértice atual
        // Nota: Ajuste a iteração de arestas ('graph.getNeighbors(u)') conforme a assinatura real do seu DirectedGraph
        for (const auto& edge : graph.getNeighbors(u)) {
            int v = edge.first; // Vértice destino
            double weight = edge.second; // Peso do arco

            // Função de custo aditiva padrão do IFT
            double offeringCost = cost[u] + weight;

            // Condição de relaxamento: Se o novo caminho for estritamente mais barato
            if (offeringCost < cost[v]) {
                cost[v] = offeringCost;
                labels[v] = labels[u]; // O vizinho herda o rótulo do nó gerador (Conquista de território)
                pq.push({v, offeringCost});
            }
        }
    }
}

/**
 * @brief Exporta o resultado da segmentação isolando o objeto contra um fundo preto absoluto.
 */
void saveSegmentation(const std::vector<int>& labels, const unsigned char* originalImg, int w, int h, const char* filename) {
    std::vector<unsigned char> out(w * h * 3);

    for (int i = 0; i < w * h; ++i) {
        if (labels[i] == 1) { // Objeto -> Copia a cor real original
            out[i*3]     = originalImg[i*3];
            out[i*3 + 1] = originalImg[i*3 + 1];
            out[i*3 + 2] = originalImg[i*3 + 2];
        } else { // Fundo -> Mascara para Preto
            out[i*3]     = 0;
            out[i*3 + 1] = 0;
            out[i*3 + 2] = 0;
        }
    }
    stbi_write_png(filename, w, h, 3, out.data(), w * 3);
}

/**
 * @brief Gera uma imagem contendo apenas as bordas da segmentação IFT.
 */
void saveIFTBoundaries(const std::vector<int>& labels, int w, int h, const char* filename) {
    std::vector<unsigned char> out(w * h * 3, 0); // Inicializada inteiramente em preto

    int dx[] = {1, -1, 0, 0};
    int dy[] = {0, 0, 1, -1};

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            int currentLabel = labels[idx];
            bool isBorder = false;

            for (int i = 0; i < 4; ++i) {
                int nx = x + dx[i];
                int ny = y + dy[i];

                if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                    int nIdx = ny * w + nx;
                    
                    // Se houver transição/divergência de rótulos entre vizinhos, marca como fronteira
                    if (labels[nIdx] != currentLabel) {
                        isBorder = true;
                        break; 
                    }
                }
            }

            if (isBorder) {
                // Pinta a linha de contorno em Branco Puro
                out[idx * 3]     = 255; 
                out[idx * 3 + 1] = 255;
                out[idx * 3 + 2] = 255;
            }
        }
    }

    stbi_write_png(filename, w, h, 3, out.data(), w * 3);
}

#endif // IFT_SEGMENTATION_H