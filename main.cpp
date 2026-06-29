
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

// Estrutura para representar as arestas do grafo da imagem
struct Edge {
    int u, v;
    double weight;
    bool operator<(const Edge& other) const {
        return weight < other.weight;
    }
};

// Estrutura Union-Find (Disjoint Set) para computar a MST e as componentes ligadas
struct DisjointSet {
    std::vector<int> parent;
    std::vector<int> rank;

    DisjointSet(int n) {
        parent.resize(n);
        rank.resize(n, 0);
        for (int i = 0; i < n; ++i) parent[i] = i;
    }

    int find(int i) {
        if (parent[i] == i)
            return i;
        return parent[i] = find(parent[i]);
    }

    bool unite(int i, int j) {
        int root_i = find(i);
        int root_j = find(j);
        if (root_i != root_j) {
            if (rank[root_i] < rank[root_j]) {
                parent[root_i] = root_j;
            } else if (rank[root_i] > rank[root_j]) {
                parent[root_j] = root_i;
            } else {
                parent[root_j] = root_i;
                rank[root_i]++;
            }
            return true;
        }
        return false;
    }
};

class HierarchicalMSTSegmenter {
private:
    unsigned char* imgData;
    int width;
    int height;
    int channels;

    // Calcula o peso da aresta (dissimilaridade) com base na Distância Euclidiana de cor
    double calculateWeight(int idx1, int idx2) {
        double diff = 0.0;
        for (int c = 0; c < channels; ++c) {
            double val1 = imgData[idx1 * channels + c];
            double val2 = imgData[idx2 * channels + c];
            diff += (val1 - val2) * (val1 - val2);
        }
        return std::sqrt(diff);
    }

public:
    HierarchicalMSTSegmenter() : imgData(nullptr), width(0), height(0), channels(0) {}

    void loadImage(unsigned char* img, int w, int h, int c) {
        this->imgData = img;
        this->width = w;
        this->height = h;
        this->channels = c;
    }

    // Implementação direta do conceito de Quasi-flat Zones baseadas em MST (Cousty et al.)
    void runQuasiFlatZones(double lambdaThreshold, std::vector<int>& labels) {
        int numPixels = width * height;
        labels.resize(numPixels);

        if (!imgData) {
            std::cerr << "Erro: Dados da imagem não carregados!" << std::endl;
            return;
        }

        std::vector<Edge> allEdges;
        // 1. Modelagem do Grafo não-direcionado com Conectividade-4 (Grelha de Pixels)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int currentIdx = y * width + x;

                // Aresta com o vizinho da direita
                if (x + 1 < width) {
                    int rightIdx = y * width + (x + 1);
                    allEdges.push_back({currentIdx, rightIdx, calculateWeight(currentIdx, rightIdx)});
                }
                // Aresta com o vizinho de baixo
                if (y + 1 < height) {
                    int bottomIdx = (y + 1) * width + x;
                    allEdges.push_back({currentIdx, bottomIdx, calculateWeight(currentIdx, bottomIdx)});
                }
            }
        }

        // 2. Ordenação das arestas para o Algoritmo de Kruskal
        std::sort(allEdges.begin(), allEdges.end());

        // 3. Construção da Árvore Geradora Mínima (MST) completa do grafo
        DisjointSet mstSet(numPixels);
        std::vector<Edge> mstEdges;
        
        for (const auto& edge : allEdges) {
            if (mstSet.unite(edge.u, edge.v)) {
                mstEdges.push_back(edge);
                if (mstEdges.size() == numPixels - 1) break;
            }
        }

        // 4. O Corte Hierárquico de Cousty: Filtra as arestas da MST pelo limiar lambda
        // Pixels conectados na MST por arestas com peso <= lambda formam a mesma Zona Quase-Plana
        DisjointSet qfzSet(numPixels);
        for (const auto& edge : mstEdges) {
            if (edge.weight <= lambdaThreshold) {
                qfzSet.unite(edge.u, edge.v);
            }
        }

        // 5. Atribuição dos identificadores finais de região (labels) para cada pixel
        for (int i = 0; i < numPixels; ++i) {
            labels[i] = qfzSet.find(i);
        }
    }
};

