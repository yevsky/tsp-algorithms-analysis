"""
plot_results.py
Generuje wykresy do sprawozdania z wyników TSP (results.csv).

Wykresy:
  1. brute_force_time.png     – czas Brute-Force (TSP) vs n, linia trendu n!
  2. nn_rnn_rand_time.png     – czas NN / RNN / RAND vs n (log-log, linie trendu)
  3. rel_error.png            – błąd względny NN / RNN / RAND vs n (log-log)

Uruchomienie:
  python plot_results.py [ścieżka_do_csv]
Domyślna ścieżka: results/results.csv
"""

import sys
import os
import pandas as pd
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
from scipy.stats import linregress

# ---------------------------------------------------------------------------
# Konfiguracja
# ---------------------------------------------------------------------------
CSV_PATH = sys.argv[1] if len(sys.argv) > 1 else "results/results.csv"
OUT_DIR  = "results"
os.makedirs(OUT_DIR, exist_ok=True)

matplotlib.rcParams.update({
    "font.family":  "serif",
    "font.size":    9,
    "axes.titlesize": 9,
    "axes.labelsize": 9,
    "xtick.labelsize": 8,
    "ytick.labelsize": 8,
    "legend.fontsize": 8,
    "figure.dpi":   150,
})

# ---------------------------------------------------------------------------
# Wczytanie danych
# ---------------------------------------------------------------------------
df = pd.read_csv(CSV_PATH)
df.columns = df.columns.str.strip()

# ---------------------------------------------------------------------------
# Pomocnicze: dopasowanie linii trendu w skali log-log
# ---------------------------------------------------------------------------
def power_trend(x, y, label_base, color, ax, linestyle="--"):
    """Dopasowuje y = a * x^b w skali log-log i rysuje linię trendu."""
    mask = (x > 0) & (y > 0)
    lx, ly = np.log(x[mask]), np.log(y[mask])
    if len(lx) < 2:
        return
    b, log_a, *_ = linregress(lx, ly)
    a = np.exp(log_a)
    xs = np.linspace(x[mask].min(), x[mask].max(), 300)
    ys = a * xs ** b
    ax.plot(xs, ys, linestyle=linestyle, color=color, linewidth=1.2,
            label=f"trend {label_base}: $O(n^{{{b:.2f}}})$")

# ---------------------------------------------------------------------------
# 1. Brute-Force: średni czas TSP i ATSP vs n
# ---------------------------------------------------------------------------
brute_tsp  = df[(df["algorithm"] == "BRUTE") & (df["instance"].str.startswith("TSP_"))]
brute_atsp = df[(df["algorithm"] == "BRUTE") & (df["instance"].str.startswith("ATSP_"))]

brute_tsp_avg  = (brute_tsp.groupby("n")["time_ms"]
                            .mean().reset_index()
                            .rename(columns={"time_ms": "avg_ms"}))
brute_atsp_avg = (brute_atsp.groupby("n")["time_ms"]
                             .mean().reset_index()
                             .rename(columns={"time_ms": "avg_ms"}))

# Zamień 0 na 0.1 ms żeby log-scale nie pomijał punktów
brute_tsp_avg["avg_ms"]  = brute_tsp_avg["avg_ms"].replace(0, 0.1)
brute_atsp_avg["avg_ms"] = brute_atsp_avg["avg_ms"].replace(0, 0.1)

fig, ax = plt.subplots(figsize=(7, 4))

# --- Punkty TSP ---
ax.scatter(brute_tsp_avg["n"], brute_tsp_avg["avg_ms"],
           color="steelblue", s=40, zorder=3, label="Brute-Force TSP (średnia)")

# Linia trendu TSP
mask_tsp = brute_tsp_avg["avg_ms"] > 0
x_tsp = brute_tsp_avg.loc[mask_tsp, "n"].values.astype(float)
y_tsp = brute_tsp_avg.loc[mask_tsp, "avg_ms"].values.astype(float)
if len(x_tsp) >= 2:
    coeffs_tsp = np.polyfit(x_tsp, np.log(y_tsp + 1e-9), 1)
    xs = np.linspace(x_tsp.min(), x_tsp.max(), 300)
    ax.plot(xs, np.exp(np.polyval(coeffs_tsp, xs)),
            "--", color="steelblue", linewidth=1.4,
            label=f"trend TSP: $e^{{{coeffs_tsp[0]:.2f} \\cdot n}}$")

# --- Punkty ATSP ---
ax.scatter(brute_atsp_avg["n"], brute_atsp_avg["avg_ms"],
           color="darkorange", s=40, zorder=3, marker="s",
           label="Brute-Force ATSP (średnia)")

# Linia trendu ATSP
mask_atsp = brute_atsp_avg["avg_ms"] > 0
x_atsp = brute_atsp_avg.loc[mask_atsp, "n"].values.astype(float)
y_atsp = brute_atsp_avg.loc[mask_atsp, "avg_ms"].values.astype(float)
if len(x_atsp) >= 2:
    coeffs_atsp = np.polyfit(x_atsp, np.log(y_atsp + 1e-9), 1)
    xs = np.linspace(x_atsp.min(), x_atsp.max(), 300)
    ax.plot(xs, np.exp(np.polyval(coeffs_atsp, xs)),
            "--", color="darkorange", linewidth=1.4,
            label=f"trend ATSP: $e^{{{coeffs_atsp[0]:.2f} \\cdot n}}$")

# --- Adnotacja przy n=15 (TSP) ---
n15 = brute_tsp_avg[brute_tsp_avg["n"] == 15]
if not n15.empty:
    ax.annotate("limit 30 min\n(n=15)",
                xy=(15, n15["avg_ms"].values[0]),
                xytext=(13.2, n15["avg_ms"].values[0] * 0.6),
                fontsize=7, color="tomato",
                arrowprops=dict(arrowstyle="->", color="tomato", lw=0.8))

ax.set_xlabel("Rozmiar instancji $n$")
ax.set_ylabel("Czas [ms]")
ax.set_yscale("log")

# Oś X – wszystkie wartości n które wystąpiły
all_n = sorted(set(brute_tsp_avg["n"].tolist() + brute_atsp_avg["n"].tolist()))
ax.set_xticks(all_n)

ax.legend()
ax.grid(True, which="both", linestyle=":", linewidth=0.5, alpha=0.7)
plt.tight_layout()
plt.savefig(os.path.join(OUT_DIR, "brute_force_time.png"), bbox_inches="tight")
plt.close()
print("Zapisano: brute_force_time.png")

# ---------------------------------------------------------------------------
# 2. NN / RNN / RAND: czas vs n (instancje TSPLIB)
# ---------------------------------------------------------------------------
tsplib = df[~df["instance"].str.startswith("TSP_") &
            ~df["instance"].str.startswith("ATSP_")]

colors  = {"NN": "steelblue", "RNN": "darkorange", "RAND": "seagreen"}
markers = {"NN": "o",         "RNN": "s",           "RAND": "^"}

# Wartości do filtrowania (faktyczne z CSV)
TIMEOUT_RNN_MS  = 300000
TIMEOUT_RAND_MS = 600000
TIMEOUTS = {"NN": None, "RNN": TIMEOUT_RNN_MS, "RAND": TIMEOUT_RAND_MS}

# Wartości do wyświetlania na wykresie (z config.txt)
DISPLAY_TIMEOUT_MS = {"RNN": 15 * 60 * 1000, "RAND": 30 * 60 * 1000}

fig, ax = plt.subplots(figsize=(7, 4))

for algo in ["NN", "RNN", "RAND"]:
    sub = tsplib[tsplib["algorithm"] == algo].copy()
    if sub.empty:
        continue
    sub["n"] = sub["n"].astype(int)
    sub = sub.sort_values("n")

    # Zamień 0 na 0.1 żeby log-scale nie pomijał punktów
    sub["time_ms"] = sub["time_ms"].replace(0, 0.1)

    timeout_ms = TIMEOUTS[algo]

    if timeout_ms is not None:
        normal  = sub[sub["time_ms"] < timeout_ms]
        timeout = sub[sub["time_ms"] >= timeout_ms]
    else:
        normal  = sub
        timeout = sub.iloc[0:0]

    # Punkty normalne
    ax.scatter(normal["n"], normal["time_ms"],
               color=colors[algo], marker=markers[algo], s=35, zorder=3,
               label=algo)

    # Punkty timeout – rysowane na wysokości prawdziwego limitu z configu
    if not timeout.empty:
        display_y = DISPLAY_TIMEOUT_MS[algo]
        ax.scatter(timeout["n"], [display_y] * len(timeout),
                   color=colors[algo], marker="x", s=60, zorder=3,
                   label=f"{algo} (timeout {display_y // 60000} min)")

    # Trend TYLKO z normalnych punktów
    x_tr = normal["n"].values.astype(float)
    y_tr = normal["time_ms"].values.astype(float)
    mask = y_tr > 0
    if mask.sum() >= 2:
        power_trend(x_tr[mask], y_tr[mask], algo, colors[algo], ax)

# Oś X – konkretne wartości n, posortowane numerycznie
actual_n = sorted(tsplib["n"].astype(int).unique().tolist())

ax.set_xscale("log")
ax.set_yscale("log")

ax.set_xticks(actual_n)
ax.xaxis.set_major_formatter(matplotlib.ticker.FixedFormatter([str(n) for n in actual_n]))
ax.xaxis.set_minor_locator(matplotlib.ticker.NullLocator())
ax.xaxis.set_minor_formatter(matplotlib.ticker.NullFormatter())
ax.set_xticklabels([str(n) for n in actual_n], rotation=45, ha="right")

ax.set_xlabel("Rozmiar instancji $n$")
ax.set_ylabel("Czas [ms]")
ax.legend(ncol=2)
ax.grid(True, which="both", linestyle=":", linewidth=0.5, alpha=0.7)
plt.tight_layout()
plt.savefig(os.path.join(OUT_DIR, "nn_rnn_rand_time.png"), bbox_inches="tight")
plt.close()
print("Zapisano: nn_rnn_rand_time.png")
# ---------------------------------------------------------------------------
# 3. Błąd względny NN / RNN / RAND vs n
# ---------------------------------------------------------------------------
fig, ax = plt.subplots(figsize=(7, 4))

for algo in ["NN", "RNN", "RAND"]:
    sub = tsplib[(tsplib["algorithm"] == algo) & (tsplib["rel_error"] >= 0)].copy()
    if sub.empty:
        continue
    sub["n"] = sub["n"].astype(int)
    sub = sub.sort_values("n")

    ax.scatter(sub["n"], sub["rel_error"],
               color=colors[algo], marker=markers[algo], s=35, zorder=3,
               label=algo)

    # Linia trendu
    x_tr = sub["n"].values.astype(float)
    y_tr = sub["rel_error"].values.astype(float)
    mask = (x_tr > 0) & (y_tr > 0)
    if mask.sum() >= 2:
        lx, ly = np.log(x_tr[mask]), np.log(y_tr[mask])
        b, log_a, *_ = linregress(lx, ly)
        a = np.exp(log_a)
        xs = np.linspace(x_tr[mask].min(), x_tr[mask].max(), 300)
        ys = a * xs ** b
        ax.plot(xs, ys, "--", color=colors[algo], linewidth=1.1,
                label=f"trend {algo}: $O(n^{{{b:.2f}}})$")

# Oś X – konkretne wartości n, posortowane numerycznie
actual_n = sorted(tsplib["n"].astype(int).unique().tolist())

ax.set_xscale("log")
ax.set_yscale("log")

ax.set_xticks(actual_n)
ax.xaxis.set_major_formatter(matplotlib.ticker.FixedFormatter([str(n) for n in actual_n]))
ax.xaxis.set_minor_locator(matplotlib.ticker.NullLocator())
ax.xaxis.set_minor_formatter(matplotlib.ticker.NullFormatter())
ax.set_xticklabels([str(n) for n in actual_n], rotation=45, ha="right")

ax.set_xlabel("Rozmiar instancji $n$")
ax.set_ylabel("Błąd względny [%]")
ax.legend(ncol=2)
ax.grid(True, which="both", linestyle=":", linewidth=0.5, alpha=0.7)
plt.tight_layout()
plt.savefig(os.path.join(OUT_DIR, "rel_error.png"), bbox_inches="tight")
plt.close()
print("Zapisano: rel_error.png")
print("\nWszystkie wykresy zapisane w katalogu:", OUT_DIR)