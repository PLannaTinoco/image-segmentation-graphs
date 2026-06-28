// STB - deve ser definido UMA única vez, antes de tudo
#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "libs/stb_image_write.h"

#include "srcm/stb/image_processing.h"        // FH + Cousty
#include "srcm/stb/third_image_processing.h"   // IFT

#include <iostream>
#include <string>

void printUsage(const char* prog) {
    std::cout << "\nUso: " << prog << " <metodo> <imagem> [opcoes]\n\n";
    std::cout << "Metodos:\n";
    std::cout << "  fh      <imagem> <K> <min_size>   Felzenszwalb-Huttenlocher\n";
    std::cout << "  cousty  <imagem> <lambda>          Cousty Quasi-Flat Zones\n";
    std::cout << "  ift     <imagem>                   IFT / Watershed\n";
    std::cout << "\nExemplos:\n";
    std::cout << "  " << prog << " fh foto.jpg 300 50\n";
    std::cout << "  " << prog << " cousty foto.jpg 10.0\n";
    std::cout << "  " << prog << " ift foto.jpg\n";
    std::cout << "\nResultados salvos em output/\n\n";
}

// Gera sementes automaticas: borda da imagem = fundo, regiao central-direita = objeto
// O objeto e centrado em 65% da largura e 45% da altura para cobrir sujeitos deslocados
Seeds generateAutoSeeds(int width, int height, int margin = 15) {
    Seeds seeds;

    // Fundo: margem mais larga para sementes mais seguras e afastadas do sujeito
    for (int x = 0; x < width; ++x) {
        for (int m = 0; m < margin; ++m) {
            seeds.backgroundObj.push_back(m * width + x);                      // topo
            seeds.backgroundObj.push_back((height - 1 - m) * width + x);      // base
        }
    }
    for (int y = margin; y < height - margin; ++y) {
        for (int m = 0; m < margin; ++m) {
            seeds.backgroundObj.push_back(y * width + m);                      // esquerda
            seeds.backgroundObj.push_back(y * width + (width - 1 - m));       // direita
        }
    }

    // Objeto: bloco eliptico centrado em 65% da largura e 45% da altura
    // Raios proporcional a imagem para cobrir sujeitos de tamanhos variados
    int cx = (int)(width * 0.65);
    int cy = (int)(height * 0.45);
    int rx = width  / 7;   // raio horizontal mais conservador
    int ry = height / 4;   // raio vertical mais conservador

    for (int y = cy - ry; y <= cy + ry; ++y) {
        for (int x = cx - rx; x <= cx + rx; ++x) {
            if (y >= margin && y < height - margin && x >= margin && x < width - margin) {
                // Forma eliptica para aproximar melhor o contorno do sujeito
                double dx = (double)(x - cx) / rx;
                double dy = (double)(y - cy) / ry;
                if (dx*dx + dy*dy <= 1.0)
                    seeds.obj.push_back(y * width + x);
            }
        }
    }
    return seeds;
}

int main(int argc, char* argv[]) {
    if (argc < 3) { printUsage(argv[0]); return 1; }

    std::string method   = argv[1];
    const char* imgPath  = argv[2];

    system("mkdir -p output/MST output/cousty output/IFT");

    // --- Felzenszwalb-Huttenlocher ---
    if (method == "fh") {
        if (argc < 5) { std::cerr << "fh requer: <imagem> <K> <min_size>\n"; return 1; }
        int K       = std::atoi(argv[3]);
        int minSize = std::atoi(argv[4]);
        std::cout << "=== Felzenszwalb-Huttenlocher (K=" << K << ", min_size=" << minSize << ") ===\n";
        return processImage(imgPath, "output/MST/resultado.png", K, minSize);

    // --- Cousty Quasi-Flat Zones ---
    } else if (method == "cousty") {
        if (argc < 4) { std::cerr << "cousty requer: <imagem> <lambda>\n"; return 1; }
        double lambda = std::atof(argv[3]);
        std::cout << "=== Cousty Quasi-Flat Zones (lambda=" << lambda << ") ===\n";
        return processImageCousty(imgPath, lambda);

    // --- IFT / Watershed ---
    } else if (method == "ift") {
        std::cout << "=== IFT / Watershed ===\n";
        DirectedGraph graph;
        unsigned char* imgData = create_graph(imgPath, graph);
        if (!imgData) return 1;

        int w = graph.getWidth(), h = graph.getHeight();
        Seeds seeds = generateAutoSeeds(w, h);
        std::cout << "Sementes: " << seeds.obj.size() << " objeto, "
                  << seeds.backgroundObj.size() << " fundo\n";

        std::vector<int> labels;
        runIFT(graph, seeds, labels);

        saveSegmentation(labels, imgData, w, h, "output/IFT/resultado_ift.png");
        saveIFTBoundaries(labels, w, h, "output/IFT/bordas_ift.png");
        delete[] imgData;
        std::cout << "Salvo em output/IFT/\n";
        return 0;

    } else {
        std::cerr << "Metodo desconhecido: " << method << "\n";
        printUsage(argv[0]);
        return 1;
    }
}