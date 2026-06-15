# Segmentação de Imagens com Grafos

PUC Minas — Teoria de Grafos e Computabilidade  
Prof. Silvio Jamil F. Guimarães — 2026/1

---

## Visão Geral

Este projeto implementa três métodos de segmentação de imagens baseados em grafos:

- **(a) Felzenszwalb & Huttenlocher** — AGM com critério adaptativo
- **(b) Cousty & Guimarães** — AGM hierárquica com quasi-flat zones
- **(c) IFT — Falcão** — floresta de caminhos mínimos

A ideia central é sempre a mesma: cada pixel da imagem é um vértice do grafo, cada par de pixels vizinhos é uma aresta, e o peso da aresta é a diferença de intensidade entre eles. Segmentar a imagem vira um problema de particionamento de vértices.

---

## Estrutura do Repositório

```
image-segmentation-graphs/
│
├── src/
│   ├── common/              # base compartilhada pelos três métodos
│   │   ├── image.h/cpp      # leitura e escrita de imagens via OpenCV
│   │   ├── graph.h/cpp      # construção do grafo a partir da imagem
│   │   └── union_find.h/cpp # estrutura Union-Find para o Kruskal
│   │
│   ├── felzenszwalb/        # método (a)
│   ├── cousty/              # método (b)
│   └── ift/                 # método (c)
│
├── images/
│   ├── gray/                # imagens de teste em tons de cinza
│   └── color/               # imagens de teste coloridas
│
├── results/                 # segmentações geradas por cada método
├── report/                  # relatório LaTeX no modelo JBCS
├── Makefile
└── README.md
```
---

## Testando o Common

### Linux / macOS

```bash
g++ -std=c++17 $(pkg-config --cflags opencv4) \
    teste.cpp \
    src/common/image.cpp \
    src/common/graph.cpp \
    src/common/union_find.cpp \
    $(pkg-config --libs opencv4) \
    -o teste && ./teste
```

### Windows (com OpenCV instalado via vcpkg)

```bash
g++ -std=c++17 \
    teste.cpp \
    src/common/image.cpp \
    src/common/graph.cpp \
    src/common/union_find.cpp \
    -I"C:/vcpkg/installed/x64-windows/include" \
    -L"C:/vcpkg/installed/x64-windows/lib" \
    -lopencv_core4100 -lopencv_imgcodecs4100 -lopencv_imgproc4100 \
    -o teste.exe && ./teste.exe
```

> **Observação Windows:** o caminho `-I` e `-L` pode variar dependendo de onde o OpenCV foi instalado. Se estiver usando Visual Studio, importe o projeto e configure as dependências do OpenCV pelo gerenciador de propriedades.

A saída esperada para a imagem `images/gray/teste.png` (100x100 pixels):

---
## Dependências

- `g++` com suporte a C++17
- `OpenCV 4.x`

Instalação no Ubuntu:
```bash
sudo apt install libopencv-dev
```

## Configuração do Ambiente

### Linux (Ubuntu/Debian)

```bash
# instala o g++ e ferramentas de compilação
sudo apt install build-essential

# instala o OpenCV
sudo apt install libopencv-dev

# verifica se está tudo instalado
g++ --version
pkg-config --modversion opencv4
```

### macOS

```bash
# instala o Homebrew se não tiver
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# instala o OpenCV
brew install opencv

# verifica
g++ --version
pkg-config --modversion opencv4
```

### Windows

A forma mais simples é usar o **WSL (Windows Subsystem for Linux)** e seguir os passos do Linux:

```bash
# no PowerShell como administrador
wsl --install

# após reiniciar, abra o WSL e execute os passos do Linux
```

Alternativa nativa: instalar o OpenCV via [vcpkg](https://vcpkg.io/) e usar o Visual Studio ou MinGW.

> **Recomendação:** se estiver no Windows, use o WSL. Evita problemas de configuração e garante que todos do grupo compilem da mesma forma.


---

## Compilação

```bash
make all          # compila os três métodos
make felzenszwalb # compila apenas o Felzenszwalb
make cousty       # compila apenas o Cousty
make ift          # compila apenas o IFT
make clean        # remove os binários gerados
```

---

## Como Usar

```bash
# Felzenszwalb — parâmetro k controla o tamanho das regiões
./felzenszwalb images/gray/foto.png results/felzenszwalb/saida.png 300

# Cousty — parâmetro lambda controla o nível da hierarquia
./cousty images/gray/foto.png results/cousty/saida.png 10

# IFT — arquivo de sementes define os objetos de interesse
./ift images/gray/foto.png results/ift/saida.png sementes.txt
```

---

## Módulo Common

O `common` é a fundação do projeto. Todo método usa essas três estruturas.

---

### `image.h` — Carregamento e Salvamento de Imagens

Usa OpenCV internamente. Suporta PNG, JPEG, BMP e outros formatos.

**Funções disponíveis:**

```cpp
// carrega imagem do disco
// gray=true  → tons de cinza (1 canal)
// gray=false → colorida BGR (3 canais)
cv::Mat carregarImagem(const std::string& caminho, bool gray = false);

// salva imagem no disco
void salvarImagem(const std::string& caminho, const cv::Mat& imagem);

// gera imagem de saída colorida onde cada região tem uma cor aleatória
// rotulos[i] = índice da região do pixel i
cv::Mat gerarSegmentacao(const cv::Mat& original, const std::vector<int>& rotulos);
```

**Exemplo de uso:**
```cpp
#include "image.h"

// carrega em cinza
cv::Mat img_cinza = carregarImagem("foto.png", true);

// carrega colorida
cv::Mat img_color = carregarImagem("foto.png", false);

// salva resultado
salvarImagem("resultado.png", img_cinza);

// gera visualização da segmentação
std::vector<int> rotulos(img_cinza.rows * img_cinza.cols);
// ... preenche rotulos com o índice de região de cada pixel ...
cv::Mat resultado = gerarSegmentacao(img_cinza, rotulos);
salvarImagem("segmentado.png", resultado);
```

---

### `graph.h` — Construção do Grafo

Transforma a imagem em um grafo ponderado onde:
- cada pixel é um vértice com índice `i * largura + j`
- arestas conectam pixels vizinhos (vizinhança 4-conectada)
- o peso da aresta é a diferença de intensidade entre os pixels

**Para imagens em cinza:**
```
w(p, q) = |I(p) - I(q)|
```

**Para imagens coloridas (distância euclidiana no espaço BGR):**
```
w(p, q) = sqrt((Bp-Bq)² + (Gp-Gq)² + (Rp-Rq)²)
```

**Estruturas:**
```cpp
struct Aresta {
    int u;      // índice do pixel origem
    int v;      // índice do pixel destino
    float peso; // diferença de intensidade
};

class Grafo {
public:
    int largura;
    int altura;
    int numVertices;
    std::vector<Aresta> arestas;  // todas as arestas do grafo

    Grafo(const cv::Mat& imagem); // constrói o grafo a partir da imagem
    int idx(int linha, int col);  // converte (linha, col) para índice
};
```

**Exemplo de uso:**
```cpp
#include "graph.h"

cv::Mat imagem = carregarImagem("foto.png", true);
Grafo g(imagem);

std::cout << "Vértices: " << g.numVertices << std::endl;
std::cout << "Arestas:  " << g.arestas.size() << std::endl;

// acessar uma aresta
Aresta a = g.arestas[0];
std::cout << "Aresta: " << a.u << " -> " << a.v 
          << " peso=" << a.peso << std::endl;

// converter posição para índice
int indice = g.idx(2, 3); // pixel na linha 2, coluna 3
```

**Observação:** apenas arestas para direita e para baixo são criadas, evitando duplicatas. O total de arestas é aproximadamente `2 * largura * altura`.

---

### `union_find.h` — Union-Find

Estrutura de dados que responde eficientemente: **"esses dois pixels pertencem à mesma região?"**

Usada pelo Kruskal (dentro do Felzenszwalb e do Cousty) para detectar ciclos em tempo quase constante.

Implementa duas otimizações clássicas:
- **Compressão de caminho** no `find` — todos os nós apontam direto para a raiz
- **Union by rank** no `unite` — árvore menor entra embaixo da maior

**Funções disponíveis:**
```cpp
class UnionFind {
public:
    UnionFind(int n);        // inicializa n pixels, cada um na sua região
    int find(int p);         // retorna o representante da região de p
    bool unite(int p, int q);// junta regiões de p e q — false se já juntos
    int size(int p);         // tamanho da região de p
    int numComponentes();    // quantas regiões existem agora
};
```

**Exemplo de uso no Kruskal:**
```cpp
#include "union_find.h"
#include "graph.h"
#include <algorithm>

cv::Mat imagem = carregarImagem("foto.png", true);
Grafo g(imagem);

// ordena arestas por peso crescente
std::sort(g.arestas.begin(), g.arestas.end(),
    [](const Aresta& a, const Aresta& b) {
        return a.peso < b.peso;
    });

UnionFind uf(g.numVertices);

for (const Aresta& aresta : g.arestas) {
    // se os pixels não estão na mesma região, une
    if (uf.find(aresta.u) != uf.find(aresta.v)) {
        uf.unite(aresta.u, aresta.v);
    }
}
```

**Por que o Union-Find evita ciclos?**  
Antes de unir dois pixels, o Kruskal verifica `find(u) == find(v)`. Se sim, já estão na mesma região — adicionar essa aresta criaria um ciclo — então ignora. O próprio `find` é a detecção de ciclo.

---

## Conceito: Como Funciona a Segmentação

Todos os métodos seguem a mesma ideia base:

1. Cada pixel vira um vértice do grafo
2. Pixels vizinhos são conectados por arestas com peso = diferença de intensidade
3. Pixels similares têm arestas de peso baixo → provavelmente mesma região
4. Pixels diferentes têm arestas de peso alto → provavelmente fronteira

A diferença entre os métodos está em **como decidem onde cortar**:

| Método | Base | Threshold | Saída |
|--------|------|-----------|-------|
| Felzenszwalb | Kruskal modificado | k adaptativo por região | 1 segmentação |
| Cousty | Kruskal puro | λ global, varia depois | hierarquia completa |
| IFT | Dijkstra modificado | sementes do usuário | 1 segmentação |

---

## Membros do Grupo

| Nome | Responsabilidade |
|------|-----------------|
|      | Common + Felzenszwalb |
|      | Cousty & Guimarães |
|      | IFT — Falcão |
|      | Relatório LaTeX |

---

## Referências

- Felzenszwalb, P. F., Huttenlocher, D. P. "Efficient Graph-Based Image Segmentation." IJCV, 2004.
- Cousty, J., Najman, L., Kenmochi, Y., Guimarães, S. "Hierarchical Segmentations with Graphs: Quasi-flat Zones, Minimum Spanning Trees, and Saliency Maps." JMIV, 2018.
- Falcão, A. X., Stolfi, J., Lotufo, R. "The Image Foresting Transform: Theory, Algorithms, and Applications." IEEE TPAMI, 2004.