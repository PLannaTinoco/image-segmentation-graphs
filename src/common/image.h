#ifndef IMAGE_H
#define IMAGE_H

#include <opencv2/opencv.hpp>
#include <string>

// carrega uma imagem do disco
// se gray=true carrega em tons de cinza, senão colorida
cv::Mat carregarImagem(const std::string& caminho, bool gray = false);

// salva uma imagem no disco
void salvarImagem(const std::string& caminho, const cv::Mat& imagem);

// gera imagem colorida de saída onde cada região tem uma cor aleatória
cv::Mat gerarSegmentacao(const cv::Mat& original, const std::vector<int>& rotulos);

#endif