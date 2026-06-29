#!/bin/bash

IMAGEM="sua_imagem.jpg"

echo "=== Compilando ==="
g++ -std=c++17 -O2 -I. -Isrcm -Ilibs main_teste.cpp -o segmentar -lm
if [ $? -ne 0 ]; then
    echo "Erro na compilacao!"
    exit 1
fi
echo "Compilado com sucesso!"
echo ""

echo "=== Felzenszwalb-Huttenlocher ==="
./segmentar fh $IMAGEM 300 50
echo ""

echo "=== Cousty Quasi-Flat Zones ==="
./segmentar cousty $IMAGEM 5000.0
echo ""

echo "=== IFT / Watershed ==="
./segmentar ift $IMAGEM
echo ""

echo "=== Gerando Benchmark ==="
./segmentar benchmark $IMAGEM
echo ""



echo "=== Arquivos gerados ==="
find output -type f | sort