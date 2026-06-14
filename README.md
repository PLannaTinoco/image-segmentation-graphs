# Segmentação de Imagens com Grafos
PUC Minas — Teoria de Grafos e Computabilidade — Prof. Silvio Jamil F. Guimarães

## Estrutura do repositório

```
segmentacao/
│
├── src/
│   ├── common/          # código compartilhado pelos três métodos
│   │   ├── image.h/cpp        # leitura e escrita de imagens (PGM/PPM)
│   │   ├── graph.h/cpp        # estrutura do grafo (vértices, arestas, pesos)
│   │   └── union_find.h/cpp   # Union-Find para Kruskal e Felzenszwalb
│   │
│   ├── felzenszwalb/    # método (a) — AGM com critério adaptativo
│   ├── cousty/          # método (b) — AGM hierárquica / quasi-flat zones
│   └── ift/             # método (c) — floresta de caminhos mínimos
│
├── images/
│   ├── gray/            # imagens de teste em tons de cinza (.pgm)
│   └── color/           # imagens de teste coloridas (.ppm)
│
├── results/             # segmentações geradas por cada método
├── report/              # relatório LaTeX no modelo JBCS
├── docs/                # artigos de referência
├── Makefile
└── README.md
```

## Compilação

```bash
make all          # compila todos os métodos
make felzenszwalb # compila individualmente
make cousty
make ift
make clean        # remove binários
```

## Membros do grupo
- (adicionar nomes e responsabilidades)
