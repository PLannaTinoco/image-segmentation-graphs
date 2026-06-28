# Segmentação de Imagens em Grafos

## Como compilar

Na raiz do projeto, rode:

```bash
bash rodar.sh
```

## Como testar

Substitua `sua_imagem.jpg` pelo caminho da sua imagem.

```bash
# Felzenszwalb-Huttenlocher
./segmentar fh sua_imagem.jpg 300 50

# Cousty Quasi-Flat Zones
./segmentar cousty sua_imagem.jpg 10.0



# IFT / Watershed
./segmentar ift sua_imagem.jpg
```

## Onde ficam os resultados

| Algoritmo  | Pasta de saída      |
|------------|---------------------|
| FH         | `output/MST/`       |
| Cousty     | `output/cousty/`    |
| IFT        | `output/IFT/`       |

## Parâmetros

| Parâmetro   | Algoritmo | Descrição                                              |
|-------------|-----------|--------------------------------------------------------|
| `K`         | FH        | Escala dos segmentos. Maior = regiões maiores          |
| `min_size`  | FH        | Tamanho mínimo de segmento em pixels                   |
| `lambda`    | Cousty    | Limiar de corte da MST. Maior = menos regiões          |
