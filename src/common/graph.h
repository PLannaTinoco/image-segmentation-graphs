#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <opencv2/opencv.hpp>

struct Aresta {
    int u;      // índice do pixel origem
    int v;      // índice do pixel destino
    float peso; // diferença de intensidade entre u e v
};

class Grafo {
public:
    int largura;
    int altura;
    int numVertices;
    std::vector<Aresta> arestas;

    // constrói o grafo a partir de uma imagem em tons de cinza
    Grafo(const cv::Mat& imagem);

    // converte posição (linha, coluna) para índice do vértice
    int idx(int linha, int col);
};

#endif