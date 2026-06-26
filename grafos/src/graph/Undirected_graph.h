#ifndef UNDIRECTED_GRAPH_H
#define UNDIRECTED_GRAPH_H

#include "Graph.h"
#include "utils/PixelConfiguration.h"
#include "utils/FH.h"
#include "utils/UnionFind.h"
#include <algorithm>
#include <vector>
#include <iostream>

class Undirected_graph : public Graph {
    private:
        int width;
        int height;

    public:
        Undirected_graph() { 
            this->width = 0;
            this->height = 0;
        }

        void insert(const int u, const int v, const double weight) {
            if(u >= this->size || v >= this->size) return;
            this->adj[u].push_back({v, weight});
            this->adj[v].push_back({u, weight});
        }

        void inicializar(int size) {
            this->setSize(size);
        }

        void printNeighbors(int u) const {
            if (u >= this->size) {
                std::cout << "Nó " << u << " está fora dos limites do grafo." << std::endl;
                return;
            }
            std::cout << "Vizinhos do nó " << u << ":" << std::endl;
            if (this->adj[u].empty()) {
                std::cout << "  (Nenhum vizinho encontrado)" << std::endl;
                return;
            }
            for (const auto& edge : this->adj[u]) {
                std::cout << "  -> Nó " << edge.first << " | Peso: " << edge.second << std::endl;
            }
        }

        std::vector<ARESTA> sort_edges() {
            std::vector<ARESTA> all_edges;
            for (int u = 0; u < this->size; ++u) {
                for (const auto& edge_pair : this->adj[u]) {
                    int v = edge_pair.first;
                    double weight = edge_pair.second;
                    if (u < v) {
                        all_edges.push_back({u, v, weight});
                    }
                }
            }
            std::sort(all_edges.begin(), all_edges.end(), [](const ARESTA& a, const ARESTA& b) {
                return a.weight < b.weight;
            });
            return all_edges;
        }

        std::vector<ARESTA> Kruskal() {
            std::vector<ARESTA> sortedEdges = sort_edges();
            std::vector<ARESTA> mst;
            UnionFind unionFind(this->size);
            
            for(const ARESTA& aresta : sortedEdges) {
                if(mst.size() == this->size - 1) {
                    return mst;
                }
                if(unionFind.find(aresta.u) != unionFind.find(aresta.v)) {
                    unionFind.union_sets(aresta.u, aresta.v);
                    mst.push_back(aresta);
                }
            }
            return mst;
        }

        // --- MÉTODO 1: Felzenszwalb & Huttenlocher ---
        FH MST_Forest(int k, const std::vector<ARESTA>& sortedEdges) {
            FH segmentador(k, this->size);
            for(const auto& aresta: sortedEdges) {
                segmentador.FelzenszwalbNHuttenlocher(aresta);
            }
            return segmentador;
        }

        // --- MÉTODO 2: Cousty et al. (2018) - Zonas Quase-Planas ---
        /**
         * @brief Segmentação baseada no artigo de Cousty et al. 
         * Corta a MST baseando-se em um limiar fixo lambda.
         * @param lambdaThreshold Peso máximo permitido para manter píxeis na mesma região
         * @return Vetor de inteiros representando os rótulos (labels) finais de cada pixel
         */
        std::vector<int> CoustyQuasiFlatZones(double lambdaThreshold) {
            // 1. Obtém a Árvore Geradora Mínima (MST) da imagem via Kruskal
            std::vector<ARESTA> mst = this->Kruskal();
            
            // 2. Instancia uma nova estrutura Union-Find para mapear as regiões finais
            UnionFind uf_qfz(this->size);

            // 3. Aplica o corte hierárquico: Une componentes adjacentes na MST 
            // apenas se a dissimilaridade (peso) for menor ou igual a lambda
            for (const ARESTA& aresta : mst) {
                if (aresta.weight <= lambdaThreshold) {
                    uf_qfz.union_sets(aresta.u, aresta.v);
                }
            }

            // 4. Mapeia a raiz de cada componente no vetor final de rótulos
            std::vector<int> labels(this->size);
            for (int i = 0; i < this->size; ++i) {
                labels[i] = uf_qfz.find(i);
            }

            return labels;
        }

        UnionFind findComponents(std::vector<ARESTA> &mst) {
            UnionFind uf(this->size);
            for(const ARESTA& aresta: mst) {
                uf.union_sets(aresta.u, aresta.v);
            }
            return uf;
        }

        void setWidth(int w) { this->width = w; }
        void setHeight(int h) { this->height = h; } 
        int getSize() { return this->size; }
        int getWidth() { return this->width; }
        int getHeight() { return this->height; }
};

#endifv