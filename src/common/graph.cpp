#include "graph.h"
#include <cmath>

Grafo::Grafo(const cv::Mat& imagem) {
    altura = imagem.rows;
    largura = imagem.cols;
    numVertices = altura * largura;

    for (int i = 0; i < altura; i++) {
        for (int j = 0; j < largura; j++) {

            // aresta para o pixel da direita
            if (j + 1 < largura) {
                float peso;
                if (imagem.channels() == 1) {
                    // cinza: diferença simples
                    peso = abs(imagem.at<uchar>(i, j) - 
                               imagem.at<uchar>(i, j+1));
                } else {
                    // colorido: distância euclidiana no espaço RGB
                    cv::Vec3b p = imagem.at<cv::Vec3b>(i, j);
                    cv::Vec3b q = imagem.at<cv::Vec3b>(i, j+1);
                    peso = sqrt(pow(p[0]-q[0], 2) + 
                                pow(p[1]-q[1], 2) + 
                                pow(p[2]-q[2], 2));
                }
                arestas.push_back({idx(i,j), idx(i,j+1), peso});
            }

            // aresta para o pixel de baixo
            if (i + 1 < altura) {
                float peso;
                if (imagem.channels() == 1) {
                    peso = abs(imagem.at<uchar>(i, j) - 
                               imagem.at<uchar>(i+1, j));
                } else {
                    cv::Vec3b p = imagem.at<cv::Vec3b>(i, j);
                    cv::Vec3b q = imagem.at<cv::Vec3b>(i+1, j);
                    peso = sqrt(pow(p[0]-q[0], 2) + 
                                pow(p[1]-q[1], 2) + 
                                pow(p[2]-q[2], 2));
                }
                arestas.push_back({idx(i,j), idx(i+1,j), peso});
            }
        }
    }
}

int Grafo::idx(int linha, int col) {
    return linha * largura + col;
}