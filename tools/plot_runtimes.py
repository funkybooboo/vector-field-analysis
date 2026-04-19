import matplotlib.pyplot as plt
import pandas as pd

df = pd.read_csv("runtime-data/runtimes.csv", index_col=0)

sizes = [128, 256, 512, 1024, 2048, 4096, 8192]
cols = df.columns

sequential = df.loc["sequential"]

fig, ax = plt.subplots(figsize=(10, 6))

for impl in df.index:
    if impl == "sequential":
        continue
    ratios = []
    for i, col in enumerate(cols):
        seq_val = sequential.iloc[i]
        impl_val = df.loc[impl, col]
        if pd.notna(impl_val) and pd.notna(seq_val) and seq_val > 0:
            ratio = seq_val / impl_val
            ratios.append(ratio)
        else:
            ratios.append(None)
    ax.plot(sizes[: len(ratios)], ratios, marker="o", label=impl)

ax.set_xlabel("Grid Size")
ax.set_ylabel("Ratio vs Sequential")
ax.set_title("Comparative Performance Increase Across Implementation")
ax.legend()
ax.set_xscale("log")
ax.set_xticks(sizes)
ax.set_xticklabels(sizes)
ax.grid(True)

plt.tight_layout()
plt.savefig("runtime-data/performance_plot.png")
plt.show()
