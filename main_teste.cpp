
#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "libs/stb_image_write.h"

#include "srcm/stb/image_processing.h"        // FH + Cousty
#include "srcm/stb/third_image_processing.h"   // IFT

#include <iostream>
#include <string>
#include <chrono>
#include <fstream>

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

// Grava (ou atualiza) uma linha no benchmark_data.csv
void saveBenchmark(int pixels, double time_fh, double time_cousty, double time_ift) {
    std::string path = "benchmark/benchmark_data.csv";
    std::ofstream f(path);
    f << "pixels,time_felzenszwalb,time_cousty,time_ift\n";
    f << pixels << "," << time_fh << "," << time_cousty << "," << time_ift << "\n";
    f.close();
    std::cout << "Benchmark salvo em " << path << "\n";
}

Seeds generateAutoSeeds(int width, int height, int margin = 15) {
    Seeds seeds;

    for (int x = 0; x < width; ++x) {
        for (int m = 0; m < margin; ++m) {
            seeds.backgroundObj.push_back(m * width + x);
            seeds.backgroundObj.push_back((height - 1 - m) * width + x);
        }
    }
    for (int y = margin; y < height - margin; ++y) {
        for (int m = 0; m < margin; ++m) {
            seeds.backgroundObj.push_back(y * width + m);
            seeds.backgroundObj.push_back(y * width + (width - 1 - m));
        }
    }

    int cx = (int)(width * 0.65);
    int cy = (int)(height * 0.45);
    int rx = width  / 7;
    int ry = height / 4;

    for (int y = cy - ry; y <= cy + ry; ++y) {
        for (int x = cx - rx; x <= cx + rx; ++x) {
            if (y >= margin && y < height - margin && x >= margin && x < width - margin) {
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

    std::string method  = argv[1];
    const char* imgPath = argv[2];

    system("mkdir -p output/MST output/cousty output/IFT");

    using clock = std::chrono::high_resolution_clock;

    // --- Felzenszwalb-Huttenlocher ---
    if (method == "fh") {
        if (argc < 5) { std::cerr << "fh requer: <imagem> <K> <min_size>\n"; return 1; }
        int K       = std::atoi(argv[3]);
        int minSize = std::atoi(argv[4]);
        std::cout << "=== Felzenszwalb-Huttenlocher (K=" << K << ", min_size=" << minSize << ") ===\n";

        auto t0 = clock::now();
        int ret = processImage(imgPath, "output/MST/resultado.png", K, minSize);
        auto t1 = clock::now();

        double elapsed = std::chrono::duration<double>(t1 - t0).count();
        std::cout << "Tempo FH: " << elapsed << " s\n";

        // Descobre numero de pixels para o CSV
        int w, h, c;
        stbi_info(imgPath, &w, &h, &c);
        std::cout << "Pixels: " << (w * h) << "\n";

        // Grava benchmark so com FH (cousty e ift ficam 0 ate serem rodados)
        // Para benchmark completo, use o rodar.sh com --benchmark
        std::ofstream f("benchmark/benchmark_data.csv");
        f << "pixels,time_felzenszwalb,time_cousty,time_ift\n";
        f << (w * h) << "," << elapsed << ",0,0\n";
        f.close();

        return ret;

    // --- Cousty Quasi-Flat Zones ---
    } else if (method == "cousty") {
        if (argc < 4) { std::cerr << "cousty requer: <imagem> <lambda>\n"; return 1; }
        double lambda = std::atof(argv[3]);
        std::cout << "=== Cousty Quasi-Flat Zones (lambda=" << lambda << ") ===\n";

        auto t0 = clock::now();
        int ret = processImageCousty(imgPath, lambda);
        auto t1 = clock::now();

        double elapsed = std::chrono::duration<double>(t1 - t0).count();
        std::cout << "Tempo Cousty: " << elapsed << " s\n";

        int w, h, c;
        stbi_info(imgPath, &w, &h, &c);

        std::ofstream f("benchmark/benchmark_data.csv");
        f << "pixels,time_felzenszwalb,time_cousty,time_ift\n";
        f << (w * h) << ",0," << elapsed << ",0\n";
        f.close();

        return ret;

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

        auto t0 = clock::now();
        std::vector<int> labels;
        runIFT(graph, seeds, labels);
        auto t1 = clock::now();

        double elapsed = std::chrono::duration<double>(t1 - t0).count();
        std::cout << "Tempo IFT: " << elapsed << " s\n";
        std::cout << "Pixels: " << (w * h) << "\n";

        saveSegmentation(labels, imgData, w, h, "output/IFT/resultado_ift.png");
        saveIFTBoundaries(labels, w, h, "output/IFT/bordas_ift.png");
        delete[] imgData;

        std::ofstream f("benchmark/benchmark_data.csv");
        f << "pixels,time_felzenszwalb,time_cousty,time_ift\n";
        f << (w * h) << ",0,0," << elapsed << "\n";
        f.close();

        std::cout << "Salvo em output/IFT/\n";
        return 0;

    // --- Benchmark completo (todos os algoritmos de uma vez) ---
    } else if (method == "benchmark") {
        if (argc < 2) { std::cerr << "benchmark requer: <imagem>\n"; return 1; }
        std::cout << "=== Benchmark Completo ===\n";

        int w, h, c;
        stbi_info(imgPath, &w, &h, &c);
        int pixels = w * h;
        std::cout << "Imagem: " << w << "x" << h << " (" << pixels << " pixels)\n\n";

        // FH
        std::cout << "Rodando FH...\n";
        auto t0 = clock::now();
        processImage(imgPath, "output/MST/resultado.png", 300, 50);
        auto t1 = clock::now();
        double time_fh = std::chrono::duration<double>(t1 - t0).count();
        std::cout << "FH: " << time_fh << " s\n";

        // Cousty
        std::cout << "Rodando Cousty...\n";
        t0 = clock::now();
        processImageCousty(imgPath, 10.0);
        t1 = clock::now();
        double time_cousty = std::chrono::duration<double>(t1 - t0).count();
        std::cout << "Cousty: " << time_cousty << " s\n";

        // IFT
        std::cout << "Rodando IFT...\n";
        DirectedGraph graph;
        unsigned char* imgData = create_graph(imgPath, graph);
        Seeds seeds = generateAutoSeeds(w, h);
        t0 = clock::now();
        std::vector<int> labels;
        runIFT(graph, seeds, labels);
        t1 = clock::now();
        double time_ift = std::chrono::duration<double>(t1 - t0).count();
        saveSegmentation(labels, imgData, w, h, "output/IFT/resultado_ift.png");
        saveIFTBoundaries(labels, w, h, "output/IFT/bordas_ift.png");
        delete[] imgData;
        std::cout << "IFT: " << time_ift << " s\n";

        // Salva CSV
        saveBenchmark(pixels, time_fh, time_cousty, time_ift);
        return 0;

    } else {
        std::cerr << "Metodo desconhecido: " << method << "\n";
        printUsage(argv[0]);
        return 1;
    }
}