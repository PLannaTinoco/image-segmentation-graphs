#include "src/common/image.h"
#include "src/common/graph.h"
#include "src/common/union_find.h"
#include <iostream>

int main() {
    // carrega imagem em cinza
    cv::Mat imagem = carregarImagem("images/gray/teste.png", true);

    std::cout << "Imagem carregada!" << std::endl;
    std::cout << "Largura: " << imagem.cols << std::endl;
    std::cout << "Altura:  " << imagem.rows << std::endl;
    std::cout << "Canais:  " << imagem.channels() << std::endl;

    // monta o grafo
    Grafo g(imagem);
    std::cout << "Vertices: " << g.numVertices << std::endl;
    std::cout << "Arestas:  " << g.arestas.size() << std::endl;

    // testa union find
    UnionFind uf(g.numVertices);
    std::cout << "Componentes iniciais: " << uf.numComponentes() << std::endl;

    uf.unite(0, 1);
    uf.unite(1, 2);
    std::cout << "Após 2 uniões: " << uf.numComponentes() << std::endl;
    std::cout << "find(2) == find(0)? " << (uf.find(2) == uf.find(0) ? "sim" : "não") << std::endl;

    // testa imagem colorida
cv::Mat img_color = carregarImagem("images/color/teste_color.png", false);

std::cout << "\nImagem colorida carregada!" << std::endl;
std::cout << "Largura: " << img_color.cols << std::endl;
std::cout << "Altura:  " << img_color.rows << std::endl;
std::cout << "Canais:  " << img_color.channels() << std::endl;

Grafo g_color(img_color);
std::cout << "Vertices: " << g_color.numVertices << std::endl;
std::cout << "Arestas:  " << g_color.arestas.size() << std::endl;

// verifica se os pesos estão sendo calculados com Pitágoras 3D
std::cout << "Peso da primeira aresta: " << g_color.arestas[0].peso << std::endl;

int meio = g_color.arestas.size() / 2;
std::cout << "Peso aresta do meio: " << g_color.arestas[meio].peso << std::endl;

    return 0;
}