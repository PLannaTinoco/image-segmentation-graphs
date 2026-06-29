#ifndef PIXELCONFIGURATION_H
#define PIXELCONFIGURATION_H

#include <vector> // CORREÇÃO CRÍTICA: Necessário para a compilação da struct Seeds

struct PixelColor {
    unsigned char r, g, b;
};

struct CIELAB {
    double L;
    double a;
    double b;
};

struct MeanColor {
    double r, g, b;
};

struct SuperPixel {
    int id;
    double L, a, b;                 // Para o cálculo de distância (Grafo)
    unsigned char avgR, avgG, avgB; // Para a pintura final (Visualização)
    int x_center, y_center;         // Coordenadas espaciais do superpixel
};

struct MapStats {
    double mean;
    double stdDev;
    double maxVal;
};

// Mantido para compatibilidade reversa com partes mais antigas do projeto
struct ARESTA {
    int u, v;
    double weight;
};

// Estrutura principal e unificada para o Edmonds, Felzenszwalb e IFT
struct Edge {
    int u;
    int v;
    double weight;
    int edgeId; // Identificador original (CRUCIAL para a expansão O(1) do Edmonds)
};

struct ColorSum {
    // Usamos long long para evitar overflow ao somar milhares de píxeis de segmentos grandes
    long long r = 0, g = 0, b = 0;
    int count = 0;
};

struct grayPixel {
    int pixelIndex;
    unsigned char scale;
};

struct pixelLocation {
    int id;
    long r, g, b;
};

// Estrutura que guarda as sementes interativas de Objeto (Foreground) e Fundo (Background)
struct Seeds {
    std::vector<int> backgroundObj;
    std::vector<int> obj;
};

#endif // PIXELCONFIGURATION_H