#ifndef UNION_FIND_H
#define UNION_FIND_H

#include <vector>

class UnionFind {
public:
    // inicializa n elementos, cada um na sua própria região
    UnionFind(int n);

    // retorna o representante (raiz) da região do elemento p
    int find(int p);

    // junta a região de p com a região de q
    // retorna false se já estavam na mesma região (evita ciclo)
    bool unite(int p, int q);

    // retorna o tamanho da região do elemento p
    int size(int p);

    // retorna quantas regiões existem no momento
    int numComponentes();

private:
    std::vector<int> pai;   // pai[i] = pai do elemento i
    std::vector<int> rank;  // rank[i] = altura da árvore em i
    std::vector<int> tam;   // tam[i] = tamanho da região de i
    int componentes;        // número de regiões distintas
};

#endif