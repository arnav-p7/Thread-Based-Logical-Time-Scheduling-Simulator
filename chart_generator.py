
# Chart Generator for Scheduling Algorithms

import os
import sys
import matplotlib.pyplot as plt
import numpy as np

FN = "results.txt"
OUT = "results_chart.png"

if not os.path.exists(FN):
    print(f"{FN} not found — nothing to plot.")
    sys.exit(0)

algos = []
awt = []
atat = []

with open(FN, "r") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        parts = [p.strip() for p in line.split(",")]
        if len(parts) < 3:
            continue
        try:
            alg = parts[0]
            avg_w = float(parts[1])
            avg_t = float(parts[2])
        except:
            continue

        algos.append(alg)
        awt.append(avg_w)
        atat.append(avg_t)

if not algos:
    print("No valid data found in results.txt.")
    sys.exit(0)

# keep last entry per algorithm
latest = {}
for a, w, t in zip(algos, awt, atat):
    latest[a] = (w, t)

# sort algorithms alphabetically for a clean graph
algos = sorted(latest.keys())
awt = [latest[a][0] for a in algos]
atat = [latest[a][1] for a in algos]

# Theme settings
plt.style.use("dark_background")
plt.rcParams.update({
    "font.size": 12,
    "axes.titlesize": 16,
    "axes.labelsize": 13,
    "figure.titlesize": 18,
})

x = np.arange(len(algos))
bar_width = 0.5

fig = plt.figure(figsize=(12, 6))
fig.patch.set_facecolor("#111111")

# ----------------------
# WAITING TIME BAR CHART
# ----------------------
ax1 = fig.add_subplot(1, 2, 1)
bars1 = ax1.bar(x, awt, bar_width, edgecolor="white", linewidth=1.2)

ax1.set_title("Average Waiting Time", pad=15)
ax1.set_ylabel("Time Units")
ax1.set_xticks(x)
ax1.set_xticklabels(algos, rotation=25, ha="right")

# add value labels
for bar in bars1:
    height = bar.get_height()
    ax1.text(bar.get_x() + bar.get_width()/2, height + 0.05,
             f"{height:.2f}", ha="center", va="bottom", fontsize=10)

ax1.grid(color="gray", linestyle="--", linewidth=0.4, alpha=0.5)

# -------------------------
# TURNAROUND TIME BAR CHART
# -------------------------
ax2 = fig.add_subplot(1, 2, 2)
bars2 = ax2.bar(x, atat, bar_width, edgecolor="white", linewidth=1.2)

ax2.set_title("Average Turnaround Time", pad=15)
ax2.set_ylabel("Time Units")
ax2.set_xticks(x)
ax2.set_xticklabels(algos, rotation=25, ha="right")

# add value labels
for bar in bars2:
    height = bar.get_height()
    ax2.text(bar.get_x() + bar.get_width()/2, height + 0.05,
             f"{height:.2f}", ha="center", va="bottom", fontsize=10)

ax2.grid(color="gray", linestyle="--", linewidth=0.4, alpha=0.5)

plt.suptitle("Scheduling Algorithm Performance Comparison", y=1.05)
plt.tight_layout()

plt.savefig(OUT, dpi=200, bbox_inches="tight")
print(f"chart saved to {OUT}")
