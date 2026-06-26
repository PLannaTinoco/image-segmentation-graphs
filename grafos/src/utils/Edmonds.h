#ifndef EDMONDS_H
#define EDMONDS_H

#include <vector>
#include <algorithm>
#include <iostream>
#include "PixelConfiguration.h" // Garante acesso à estrutura Edge

class Edmonds {
public:
    /**
     * @brief Resolve o algoritmo de Chu-Liu-Edmonds para grafos direcionados.
     * @param totalVertices O número total absoluto de vértices/píxeis da imagem original (width * height).
     * @param numNodes O número de nós ativos na iteração atual.
     * @param edges Vetor contendo as arestas do grafo.
     * @param rootId ID do nó raiz (semente).
     * @return Vetor com as arestas que compõem a arborescência geradora mínima.
     */
    static std::vector<Edge> solve(int totalVertices, int numNodes, const std::vector<Edge>& edges, int rootId) {
        if (edges.empty()) return {};
        
        std::cout << "--- Iniciando Edmonds Otimizado (Nos: " << numNodes 
                  << " | Max ID Seguro: " << totalVertices << ") ---" << std::endl;
        
        // Criamos o vetor de lookup com o tamanho total absoluto do grafo original.
        // Isto impede overflows silenciosos na expansão e elimina a necessidade de ifs lentos.
        std::vector<int> lookup(totalVertices, -1);

        return solveRecursive(numNodes, edges, rootId, edges, 0, lookup, totalVertices);
    }

private:
    static std::vector<Edge> solveRecursive(int numNodes, 
                                            const std::vector<Edge>& activeEdges, 
                                            int rootId, 
                                            const std::vector<Edge>& globalOriginalEdges,
                                            int depth,
                                            std::vector<int>& lookup,
                                            int totalVertices) { 
        
        // Rastreamento simples de profundidade para depuração de pilhas grandes
        if (depth % 100 == 0) { 
            std::cout << "[Depth " << depth << "] Processando " << numNodes << " nos..." << std::endl;
        }

        // 1. FASE GULOSA (Min-In)
        std::vector<int> minIn(numNodes, -1);
        for (int i = 0; i < (int)activeEdges.size(); ++i) {
            const auto& e = activeEdges[i];
            
            // Filtros de integridade topológica
            if (e.u == e.v || e.v == rootId || e.u >= numNodes || e.v >= numNodes) continue;
            
            if (minIn[e.v] == -1 || e.weight < activeEdges[minIn[e.v]].weight) {
                minIn[e.v] = i;
            }
        }

        // 2. DETECÇÃO DE CICLOS (Tarjan simplificado / Busca por Caminho)
        std::vector<int> group(numNodes, -1);
        std::vector<int> visited(numNodes, -1);
        std::vector<bool> nodeInCycle(numNodes, false);
        int newNumNodes = 0;
        int cycleCount = 0;

        for (int i = 0; i < numNodes; ++i) {
            if (i == rootId || minIn[i] == -1 || group[i] != -1) continue;
            
            int curr = i;
            while (curr != rootId && minIn[curr] != -1 && visited[curr] != i && group[curr] == -1) {
                visited[curr] = i;
                curr = activeEdges[minIn[curr]].u;
            }
            
            if (curr != rootId && minIn[curr] != -1 && visited[curr] == i) {
                int cycleStart = curr;
                int temp = cycleStart;
                do {
                    group[temp] = newNumNodes;
                    nodeInCycle[temp] = true;
                    temp = activeEdges[minIn[temp]].u;
                } while (temp != cycleStart);
                newNumNodes++;
                cycleCount++;
            }
        }

        // 3. CASO BASE (Se a topologia atual não contiver ciclos, reconstrói o resultado local)
        if (cycleCount == 0) {
            std::vector<Edge> result;
            result.reserve(numNodes);
            for (int i = 0; i < numNodes; ++i) {
                if (i != rootId && minIn[i] != -1) {
                    result.push_back(globalOriginalEdges[activeEdges[minIn[i]].edgeId]);
                }
            }
            return result;
        }

        // 4. CONTRAÇÃO (Agrupamento de super-nós)
        for (int i = 0; i < numNodes; ++i) {
            if (group[i] == -1) group[i] = newNumNodes++;
        }

        std::vector<Edge> nextLevelEdges;
        nextLevelEdges.reserve(activeEdges.size());
        
        for (const auto& e : activeEdges) {
            if (e.u >= numNodes || e.v >= numNodes) continue;
            int newU = group[e.u];
            int newV = group[e.v];
            
            if (newU != newV) {
                double newWeight = e.weight;
                if (nodeInCycle[e.v]) {
                    newWeight -= activeEdges[minIn[e.v]].weight; // Redução de custo de Edmonds
                }
                nextLevelEdges.push_back({newU, newV, newWeight, e.edgeId});
            }
        }

        // 5. RECURSÃO (Processa o grafo contraído no próximo nível hierárquico)
        auto recursiveResult = solveRecursive(newNumNodes, nextLevelEdges, group[rootId], globalOriginalEdges, depth + 1, lookup, totalVertices);

        // 6. EXPANSÃO OTIMIZADA EM O(N)
        std::vector<Edge> finalResult = recursiveResult; 
        std::vector<bool> isCovered(numNodes, false);
        
        std::vector<int> cleanupList; 
        cleanupList.reserve(numNodes); 

        // Passo A: Preenche a tabela de Lookup mapeando o pixel real de destino ao nó do ciclo correspondente
        for (int i = 0; i < numNodes; ++i) {
            if (nodeInCycle[i]) {
                int internalEdgeOriginalId = activeEdges[minIn[i]].edgeId;
                int realDestPixel = globalOriginalEdges[internalEdgeOriginalId].v;

                // Acesso direto perfeitamente seguro e rápido
                lookup[realDestPixel] = i; 
                cleanupList.push_back(realDestPixel);
            }
        }

        // Passo B: Avalia as decisões da sub-recursão em tempo constante O(1)
        for (const auto& resEdge : recursiveResult) {
            int resDestReal = resEdge.v; 

            if (lookup[resDestReal] != -1) {
                int cycleNodeIndex = lookup[resDestReal];
                isCovered[cycleNodeIndex] = true; // Identifica qual aresta interna do ciclo deve cair
            }
        }

        // Passo C: Limpeza em O(K) do vetor global de lookup para reaproveitamento nas próximas sub-subidas
        for (int idx : cleanupList) {
            lookup[idx] = -1;
        }

        // Reconstrói o ciclo expandido adicionando as arestas internas que mantêm a conectividade
        for (int i = 0; i < numNodes; ++i) {
            if (nodeInCycle[i] && !isCovered[i]) {
                finalResult.push_back(globalOriginalEdges[activeEdges[minIn[i]].edgeId]);
            }
        }

        return finalResult;
    }
};

#endif // EDMONDS_H