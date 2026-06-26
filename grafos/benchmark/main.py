import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from scipy.optimize import curve_fit

# 1. Carregar dados (Certifique-se de que o CSV tem as colunas abaixo)
try:
    data = pd.read_csv('benchmark_data.csv')
except FileNotFoundError:
    # Dados de exemplo caso queira testar o script antes de gerar o CSV real
    data = pd.DataFrame({
        'pixels': [10000, 50000, 100000, 500000, 1000000],
        'time_felzenszwalb': [0.005, 0.028, 0.061, 0.35, 0.75],
        'time_cousty': [0.007, 0.035, 0.078, 0.45, 0.98],
        'time_ift': [0.002, 0.011, 0.022, 0.11, 0.23]
    })

x = data['pixels'].to_numpy()

# 2. Definição das Funções Teóricas de Complexidade
def linear(x, a, b):
    return a * x + b

def n_log_n(x, a, b):
    return a * x * np.log(x) + b

# 3. Configuração do Gráfico
plt.figure(figsize=(12, 7))

# --- ALGORITMO 1: Felzenszwalb (Esperado: O(N log N) ou Quase-Linear) ---
y_felzen = data['time_felzenszwalb'].to_numpy()
plt.scatter(x, y_felzen, color='blue', label='Real: Felzenszwalb', zorder=5)
try:
    popt, _ = curve_fit(n_log_n, x, y_felzen)
    plt.plot(x, n_log_n(x, *popt), color='blue', linestyle='--', label='Ajuste Teórico $O(N \log N)$')
except:
    plt.plot(x, y_felzen, color='blue', linestyle=':')

# --- ALGORITMO 2: Cousty et al. (Esperado: O(N log N) devido à ordenação da MST) ---
y_cousty = data['time_cousty'].to_numpy()
plt.scatter(x, y_cousty, color='green', label='Real: Cousty (ZQF)', zorder=5)
try:
    popt, _ = curve_fit(n_log_n, x, y_cousty)
    plt.plot(x, n_log_n(x, *popt), color='green', linestyle='--', label='Ajuste Teórico $O(N \log N)$')
except:
    plt.plot(x, y_cousty, color='green', linestyle=':')

# --- ALGORITMO 3: IFT Falcão (Esperado: O(N) se usar fila de baldes / Quase-linear) ---
y_ift = data['time_ift'].to_numpy()
plt.scatter(x, y_ift, color='red', label='Real: IFT Falcão', zorder=5)
try:
    popt, _ = curve_fit(linear, x, y_ift)
    plt.plot(x, linear(x, *popt), color='red', linestyle='-.', label='Ajuste Teórico Linear $O(N)$')
except:
    plt.plot(x, y_ift, color='red', linestyle=':')

# 4. Estética Avançada para o Relatório JBCS
plt.title('Comparativo de Complexidade Prática: Segmentação de Imagens em Grafos', fontsize=14, fontweight='bold')
plt.xlabel('Tamanho da Entrada (Número de Pixels - $N$)', fontsize=12)
plt.ylabel('Tempo de Execução (Segundos)', fontsize=12)
plt.grid(True, which="both", linestyle="--", alpha=0.6)
plt.legend(loc='upper left', fontsize=10)

# Formatar eixos para não exibir notação científica confusa
plt.ticklabel_format(style='plain', axis='x')

plt.tight_layout()
plt.savefig('comparativo_complexidade.png', dpi=300) # Salva em alta definição para o PDF em LaTeX
plt.show()