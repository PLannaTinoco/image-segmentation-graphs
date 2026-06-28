#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H 

#include "../graph/Undirected_graph.h"
#include "../utils/UnionFind.h"
#include "../utils/PixelConfiguration.h"
#include "../utils/FH.h"
#include "../utils/Filters.h"

#include <iostream>
#include <vector>
#include <utility>
#include <cmath>
#include <list>
#include <iomanip>  
#include <algorithm>    
#include <map>
#include <cstring>
#include <string>

// ========================================================================
// 1. FUNÇÕES DE VISUALIZAÇÃO PARA FELZENSZWALB (Mantendo compatibilidade)
// ========================================================================

void write_segmented_image(const char* output_filename, int width, int height, int channels,
                            FH& segmentador, unsigned char* original_imageData) {
    size_t buffer_size = width * height * channels;
    unsigned char *output_data = new unsigned char[buffer_size];

    if(original_imageData == nullptr || output_filename == nullptr) {
        std::cerr << "Erro: Dados nulos passados para gravação de imagem." << std::endl;
        return;
    }
    
    std::memcpy(output_data, original_imageData, buffer_size);
    unsigned char border_color[3] = {255, 255, 0}; 

    for (int y = 0; y < height - 1; ++y) { 
        for (int x = 0; x < width - 1; ++x) { 
            int current_pixel_idx = y * width + x;
            int raiz_atual = segmentador.find(current_pixel_idx);

            int right_pixel_idx = y * width + (x + 1);
            int raiz_direita = segmentador.find(right_pixel_idx);

            int down_pixel_idx = (y + 1) * width + x;
            int raiz_baixo = segmentador.find(down_pixel_idx);
            
            if (raiz_atual != raiz_direita || raiz_atual != raiz_baixo) {
                unsigned char* pixel_out = output_data + current_pixel_idx * channels;
                for (int i = 0; i < channels; ++i) {
                    if (i < 3) pixel_out[i] = border_color[i];
                }
            }
        }
    }
    stbi_write_png(output_filename, width, height, channels, output_data, width * channels);
    delete[] output_data;
}

void color_segments_by_average(const char* output_filename, int width, int height, int channels,
                               FH& segmentador, unsigned char* original_imageData) {
    std::map<int, ColorSum> segment_stats;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixel_idx_1d = y * width + x;
            int segment_id = segmentador.find(pixel_idx_1d);
            int pixel_idx_buffer = pixel_idx_1d * channels;
            
            segment_stats[segment_id].r += original_imageData[pixel_idx_buffer];
            segment_stats[segment_id].g += original_imageData[pixel_idx_buffer + 1];
            segment_stats[segment_id].b += original_imageData[pixel_idx_buffer + 2];
            segment_stats[segment_id].count++;
        }
    }

    std::map<int, PixelColor> average_colors;
    for (auto const& [segment_id, stats] : segment_stats) {
        if (stats.count > 0) {
            average_colors[segment_id] = {
                static_cast<unsigned char>(stats.r / stats.count),
                static_cast<unsigned char>(stats.g / stats.count),
                static_cast<unsigned char>(stats.b / stats.count)
            };
        }
    }

    size_t buffer_size = width * height * channels;
    unsigned char* output_data = new unsigned char[buffer_size];

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixel_idx_1d = y * width + x;
            int segment_id = segmentador.find(pixel_idx_1d);
            PixelColor avg_color = average_colors[segment_id];
            int pixel_idx_buffer = pixel_idx_1d * channels;

            output_data[pixel_idx_buffer]     = avg_color.r;
            output_data[pixel_idx_buffer + 1] = avg_color.g;
            output_data[pixel_idx_buffer + 2] = avg_color.b;
            
            if (channels == 4) {
                 output_data[pixel_idx_buffer + 3] = original_imageData[pixel_idx_buffer + 3];
            }
        }
    }

    stbi_write_png(output_filename, width, height, channels, output_data, width * channels);
    delete[] output_data;
}


// ========================================================================
// 2. SOBRECARGAS ADAPTADAS PARA COUSTY ET AL. / IFT (Recebe std::vector<int>)
// ========================================================================

/**
 * @brief Desenha as bordas dos segmentos gerados por um mapa genérico de rótulos.
 */
void write_segmented_image(const char* output_filename, int width, int height, int channels,
                           const std::vector<int>& labels, unsigned char* original_imageData) {
    size_t buffer_size = width * height * channels;
    unsigned char *output_data = new unsigned char[buffer_size];

    if(original_imageData == nullptr || output_filename == nullptr) {
        std::cerr << "Erro fatal nos parâmetros de gravação por vetor!" << std::endl;
        return;
    }
    
    std::memcpy(output_data, original_imageData, buffer_size);
    unsigned char border_color[3] = {255, 0, 0}; // Bordas vermelhas para diferenciar o método de Cousty

    for (int y = 0; y < height - 1; ++y) { 
        for (int x = 0; x < width - 1; ++x) { 
            int current_pixel_idx = y * width + x;
            int label_atual = labels[current_pixel_idx];

            int right_pixel_idx = y * width + (x + 1);
            int label_direita = labels[right_pixel_idx];

            int down_pixel_idx = (y + 1) * width + x;
            int label_baixo = labels[down_pixel_idx];
            
            if (label_atual != label_direita || label_atual != label_baixo) {
                unsigned char* pixel_out = output_data + current_pixel_idx * channels;
                for (int i = 0; i < channels; ++i) {
                    if (i < 3) pixel_out[i] = border_color[i];
                }
            }
        }
    }
    stbi_write_png(output_filename, width, height, channels, output_data, width * channels);
    delete[] output_data;
}

/**
 * @brief Colore os segmentos calculando a cor média a partir de um vetor de rótulos.
 */
void color_segments_by_average(const char* output_filename, int width, int height, int channels,
                               const std::vector<int>& labels, unsigned char* original_imageData) {
    std::map<int, ColorSum> segment_stats;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixel_idx_1d = y * width + x;
            int segment_id = labels[pixel_idx_1d];
            int pixel_idx_buffer = pixel_idx_1d * channels;
            
            segment_stats[segment_id].r += original_imageData[pixel_idx_buffer];
            segment_stats[segment_id].g += original_imageData[pixel_idx_buffer + 1];
            segment_stats[segment_id].b += original_imageData[pixel_idx_buffer + 2];
            segment_stats[segment_id].count++;
        }
    }

    std::map<int, PixelColor> average_colors;
    for (auto const& [segment_id, stats] : segment_stats) {
        if (stats.count > 0) {
            average_colors[segment_id] = {
                static_cast<unsigned char>(stats.r / stats.count),
                static_cast<unsigned char>(stats.g / stats.count),
                static_cast<unsigned char>(stats.b / stats.count)
            };
        }
    }

    size_t buffer_size = width * height * channels;
    unsigned char* output_data = new unsigned char[buffer_size];

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixel_idx_1d = y * width + x;
            int segment_id = labels[pixel_idx_1d];
            PixelColor avg_color = average_colors[segment_id];
            int pixel_idx_buffer = pixel_idx_1d * channels;

            output_data[pixel_idx_buffer]     = avg_color.r;
            output_data[pixel_idx_buffer + 1] = avg_color.g;
            output_data[pixel_idx_buffer + 2] = avg_color.b;
            
            if (channels == 4) {
                 output_data[pixel_idx_buffer + 3] = original_imageData[pixel_idx_buffer + 3];
            }
        }
    }

    stbi_write_png(output_filename, width, height, channels, output_data, width * channels);
    delete[] output_data;
}


// ========================================================================
// 3. AUXILIARES DE SISTEMA DE ARQUIVOS E MODELAGEM DE GRAFO
// ========================================================================

void escreverImagem(const std::string& nomeArquivo, int width, int height, int channels, const unsigned char* data) {
    std::string ext = nomeArquivo.substr(nomeArquivo.find_last_of(".") + 1);
    int sucesso = 0;
    if (ext == "png") {
        sucesso = stbi_write_png(nomeArquivo.c_str(), width, height, channels, data, width * channels);
    } else if (ext == "jpg" || ext == "jpeg") {
        sucesso = stbi_write_jpg(nomeArquivo.c_str(), width, height, channels, data, 95);
    } else if (ext == "bmp") {
        sucesso = stbi_write_bmp(nomeArquivo.c_str(), width, height, channels, data);
    } else {
        std::cerr << "Erro: Extensao nao suportada: " << ext << std::endl;
        return;
    }
    if (!sucesso) std::cerr << "Erro: Nao foi possivel salvar " << nomeArquivo << std::endl;
}

unsigned char* create_graph(const char * imagePath, Undirected_graph &graph) {
    int height, width, original_channels;
    unsigned char * imageData = stbi_load(imagePath, &width, &height, &original_channels, 3); 

    if(imageData == nullptr) {
        std::cerr << "Imagem vazia ou invalida " << imagePath << std::endl;
        return nullptr;
    }

    imageData = toGaussian_blur(imageData, width, height, 3);
    imageData = toGaussian_blur(imageData, width, height, 3);
    escreverImagem("output/ImagemGaussiana.png", width, height, 3, imageData);

    unsigned char* sobelData = sobelFilter(imageData, width, height, 3);
    escreverImagem("output/ImagemSobel.png", width, height, 1, sobelData);

    graph.setWidth(width);
    graph.setHeight(height);
    const long totalSize = height * width;
    graph.inicializar(totalSize);

    const double W = 500.0;
    const int dx[] = {-1, -1, -1,  0, 0,  1, 1, 1};
    const int dy[] = {-1,  0,  1, -1, 1, -1, 0, 1};
    const int channels_in_memory = 3;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned long index = (y * width + x) * channels_in_memory;
            CIELAB current = RGBtoLab(imageData[index], imageData[index + 1], imageData[index + 2]);
           
            for (int i = 0; i < 8; i++) {
                int nextX = x + dx[i];
                int nextY = y + dy[i];

                if (nextX >= 0 && nextX < width && nextY >= 0 && nextY < height) {
                    unsigned long next_index = (nextY * width + nextX) * channels_in_memory;
                    CIELAB next = RGBtoLab(imageData[next_index], imageData[next_index + 1], imageData[next_index + 2]);

                    const int current_vertex_id = y * width + x;
                    const int next_vertex_id = nextY * width + nextX;

                    unsigned char sobel_current = sobelData[current_vertex_id];
                    unsigned char sobel_next = sobelData[next_vertex_id];
                    double sobel_penalty = std::max(sobel_current, sobel_next);

                    double color_dist_sq = std::pow(current.L - next.L, 2) + std::pow(current.a - next.a, 2) + std::pow(current.b - next.b, 2); 
                    auto weight = color_dist_sq + (W * sobel_penalty);
                    
                    graph.insert(current_vertex_id, next_vertex_id, weight);
                }
            }
        }
    }
    return imageData;
}


// ========================================================================
// 4. PIPELINES PRINCIPAIS DE EXECUÇÃO (MÉTODOS 1 E 2 DO TRABALHO)
// ========================================================================

// --- Método 1: Felzenszwalb & Huttenlocher ---
int processImage(const char* path, const char* output_path, const int K, const int MIN_SEGMENT_SIZE) {
    Undirected_graph g;
    std::cout << "Carregando imagem e criando grafo (Felzenszwalb)..." << std::endl;
    unsigned char* original_imageData = create_graph(path, g);

    if (original_imageData == nullptr) return 1;

    std::vector<Edge> sortedEdges = g.sort_edges();
    std::cout << "Executando o algoritmo de Kruskal (Felzenszwalb)..." << std::endl;
    FH segmentador = g.MST_Forest(K, sortedEdges);
    segmentador.mergeSmallSegments(sortedEdges, MIN_SEGMENT_SIZE);

    int width = g.getWidth();
    int height = g.getHeight();
    int channels = 3;

    std::cout << "Salvando resultados de Felzenszwalb..." << std::endl;
    write_segmented_image("output/MST/ImagemComBordas.png", width, height, channels, segmentador, original_imageData);
    color_segments_by_average("output/MST/ImagemComPixelFundidoColorido.png", width, height, channels, segmentador, original_imageData);

    return 0;
}

// --- Método 2: Cousty et al. (Zonas Quase-Planas) ---
int processImageCousty(const char* path, const double lambdaThreshold) {
    Undirected_graph g;
    std::cout << "Carregando imagem e criando grafo (Cousty)..." << std::endl;
    unsigned char* original_imageData = create_graph(path, g);

    if (original_imageData == nullptr) return 1;

    std::cout << "Executando corte hierárquico na MST pelo limiar Lambda..." << std::endl;
    // Chama a função que adicionamos na classe Undirected_graph
    std::vector<int> coustyLabels = g.CoustyQuasiFlatZones(lambdaThreshold);

    int width = g.getWidth();
    int height = g.getHeight();
    int channels = 3;

    std::cout << "Salvando resultados de Cousty..." << std::endl;
    // O compilador escolhe automaticamente as sobrecarga baseadas em std::vector<int>
    write_segmented_image("output/cousty/ImagemComBordas_Cousty.png", width, height, channels, coustyLabels, original_imageData);
    color_segments_by_average("output/cousty/ImagemColorida_Cousty.png", width, height, channels, coustyLabels, original_imageData);

    return 0;
}

#endif