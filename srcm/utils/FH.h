#ifndef FH_H
#define FH_H

#include <vector>
#include <algorithm>
#include <numeric>
#include "PixelConfiguration.h" // Garante acesso à estrutura Edge/ARESTA

class FH {
private:
    std::vector<double> internal_index; // Int(C) - maior peso de aresta no componente
    std::vector<int> parent;            // Vetor de pais do Union-Find
    std::vector<int> component_size;    // |C| - tamanho de cada componente em píxeis
    double k;                           // Parâmetro de escala (controla o tamanho dos segmentos)
    int size;

public:
    /**
     * @brief Construtor da classe de Felzenszwalb.
     * @param k Parâmetro de escala. Valores maiores geram regiões maiores.
     * @param size Número total de vértices (width * height).
     */
    FH(int k, int size) {
        this->size = size;
        this->k = static_cast<double>(k); // Guardado como double para evitar casts repetidos
        
        internal_index.assign(size, 0.0);
        component_size.assign(size, 1);
        parent.resize(size);

        // Inicializa cada nó como seu próprio pai (floresta inicial)
        std::iota(parent.begin(), parent.end(), 0);
    }

    ~FH() { }

    /**
     * @brief Tenta agrupar dois componentes conectados pela aresta atual seguindo o critério do paper.
     * @param edge Aresta candidata a união.
     */
    void FelzenszwalbNHuttenlocher(const Edge& edge) {
        int root_u = find(edge.u);
        int root_v = find(edge.v);
  
        if (root_u != root_v) {
            // CORREÇÃO: t1 e t2 alterados para double para manter a precisão flutuante do limiar tau
            double t1 = k / component_size[root_v];
            double t2 = k / component_size[root_u];

            // M_Int = min(Int(C1) + tau(C1), Int(C2) + tau(C2))
            double M_Int = std::min(internal_index[root_u] + t2, internal_index[root_v] + t1);
            
            // Se o peso for menor ou igual ao limiar interno adaptativo, une os componentes
            if (edge.weight <= M_Int) { 
                int new_parent, child;
                
                // Union by Size (mantém a árvore balanceada)
                if (component_size[root_u] < component_size[root_v]) {
                    new_parent = root_v;
                    child = root_u;
                } else {
                    new_parent = root_u;
                    child = root_v;
                }

                parent[child] = new_parent;
                component_size[new_parent] += component_size[child];
                
                // Otimização: Como as arestas estão ordenadas, o peso atual é sempre o maior
                internal_index[new_parent] = edge.weight;
            }
        }
    }

    /**
     * @brief Realiza a busca do nó representativo com compressão de caminho (Path Compression).
     */
    int find(int i) {
        if (parent[i] == i)
            return i;
        
        return parent[i] = find(parent[i]); // Compressão de caminho em O(alpha(N))
    }

    int getParent(int i) {
        return find(i); // Retorna a raiz real e atualizada do nó
    }

    int getComponentSize(int i) {
        return component_size[find(i)];
    }

    /**
     * @brief Pós-processamento para eliminar fragmentos ou micro-segmentos menores que o threshold.
     * @param edges Lista de todas as arestas ordenadas.
     * @param min_segment_size Tamanho mínimo aceitável para um segmento funcional.
     */
    void mergeSmallSegments(const std::vector<Edge>& edges, int min_segment_size) {
        for (const auto& edge : edges) {
            int root_u = find(edge.u);
            int root_v = find(edge.v);

            if (root_u != root_v) {
                // Se qualquer um dos componentes for menor que o limite, força a união
                if (component_size[root_u] < min_segment_size || component_size[root_v] < min_segment_size) {
                    int new_parent, child;
                    if (component_size[root_u] < component_size[root_v]) {
                        new_parent = root_v;
                        child = root_u;
                    } else {
                        new_parent = root_u;
                        child = root_v;
                    }
                    
                    parent[child] = new_parent;
                    component_size[new_parent] += component_size[child];
                }
            }
        }
    }   
};

#endif // FH_H