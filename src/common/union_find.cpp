#include "union_find.h"

UnionFind::UnionFind(int n) {
    componentes = n;
    pai.resize(n);
    rank.resize(n, 0);
    tam.resize(n, 1);

    for (int i = 0; i < n; i++) {
        pai[i] = i;  // cada pixel é pai de si mesmo
    }
}

int UnionFind::find(int p) {
    if (pai[p] != p) {
        pai[p] = find(pai[p]);  // compressão de caminho
    }
    return pai[p];
}

bool UnionFind::unite(int p, int q) {
    int raiz_p = find(p);
    int raiz_q = find(q);

    if (raiz_p == raiz_q) return false;  // já na mesma região

    if (rank[raiz_p] < rank[raiz_q]) {
        pai[raiz_p] = raiz_q;
        tam[raiz_q] += tam[raiz_p];
    } else if (rank[raiz_p] > rank[raiz_q]) {
        pai[raiz_q] = raiz_p;
        tam[raiz_p] += tam[raiz_q];
    } else {
        pai[raiz_q] = raiz_p;
        tam[raiz_p] += tam[raiz_q];
        rank[raiz_p]++;
    }

    componentes--;
    return true;
}

int UnionFind::size(int p) {
    return tam[find(p)];
}

int UnionFind::numComponentes() {
    return componentes;
}