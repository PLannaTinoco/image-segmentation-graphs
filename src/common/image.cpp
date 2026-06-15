#include "image.h"
#include <iostream>
#include <map>
#include <cstdlib>
#include "image.h"
#include <iostream>

cv::Mat carregarImagem(const std::string& caminho, bool gray) {
    cv::Mat imagem;
    if (gray) {
        imagem = cv::imread(caminho, cv::IMREAD_GRAYSCALE);
    } else {
        imagem = cv::imread(caminho, cv::IMREAD_COLOR);
    }

    if (imagem.empty()) {
        std::cerr << "Erro: não foi possível carregar " << caminho << std::endl;
        exit(1);
    }

    return imagem;
}

void salvarImagem(const std::string& caminho, const cv::Mat& imagem) {
    cv::imwrite(caminho, imagem);
}

cv::Mat gerarSegmentacao(const cv::Mat& original, const std::vector<int>& rotulos) {
    int altura = original.rows;
    int largura = original.cols;

    // cria mapa de cores — cada região recebe uma cor aleatória
    std::map<int, cv::Vec3b> cores;
    cv::Mat resultado(altura, largura, CV_8UC3);

    for (int i = 0; i < altura; i++) {
        for (int j = 0; j < largura; j++) {
            int regiao = rotulos[i * largura + j];

            if (cores.find(regiao) == cores.end()) {
                // cor aleatória para essa região
                cores[regiao] = cv::Vec3b(rand() % 256, 
                                          rand() % 256, 
                                          rand() % 256);
            }

            resultado.at<cv::Vec3b>(i, j) = cores[regiao];
        }
    }

    return resultado;
}