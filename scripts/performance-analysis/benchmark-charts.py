import argparse
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import rc

rc('text', usetex=True)
rc('text.latex', preamble=r'\usepackage{siunitx}')

# Parse command-line arguments
parser = argparse.ArgumentParser(description="Plot power benchmark chart")
parser.add_argument("power_csv",
                    help="CSV report with power data")
parser.add_argument("throughput_csv",
                    help="CSV report with throughput data")
parser.add_argument("out_dir",
                    help="Output directory where to save charts",
                    default=".",
                    nargs="?",
                    type=str)
parser.add_argument("--log",
                    help="Use a logarithmic scale on the y-axis",
                    action="store_true")
args = parser.parse_args()

# Initialization
throughput_csv = args.throughput_csv
power_csv = args.power_csv
out_dir = args.out_dir
log_scale = args.log

# Read CSV report with throughput data
# CSV format: memory_type,kernel_name,data_type,num_outs,cycles,kernel_params
thr_df = pd.read_csv(throughput_csv, sep=",", header=0)

# Order by kernel, then data type, then memory type
thr_df = thr_df.sort_values(by=["kernel_name", "data_type", "memory_type"]).reset_index(drop=True)

# Compute the number of cycles per output sample
thr_df["cycles_per_output"] = thr_df["cycles"] / thr_df["num_outs"]

# Separate data by memory type
thr_df_cpu = thr_df.loc[thr_df["memory_type"] == "cpu"].reset_index(drop=True)
thr_df_carus = thr_df.loc[thr_df["memory_type"] == "carus"].reset_index(drop=True)

# Read CSV report with power data
# CSV format: memory_type,kernel_name,data_type,num_outs,sys_pwr,sys_cpu_pwr,sys_mem_pwr,sys_peri_pwr,nmc_pwr,nmc_ctl_pwr,nmc_comp_pwr,nmc_mem_pwr,kernel_params
pwr_df = pd.read_csv(power_csv, sep=",", header=0)

# Order by kernel, then data type, then memory type
pwr_df = pwr_df.sort_values(by=["kernel_name", "data_type", "memory_type"]).reset_index(drop=True)

# Compute power * cycles (energy) / number of output samples for each kernel and data type
pwr_df["energy"] = (pwr_df["sys_pwr"] + pwr_df['nmc_pwr']) * thr_df["cycles_per_output"]

# Separate data by memory type
pwr_df_cpu = pwr_df.loc[pwr_df["memory_type"] == "cpu"].reset_index(drop=True)
pwr_df_carus = pwr_df.loc[pwr_df["memory_type"] == "carus"].reset_index(drop=True)

# Throughput benchmark chart
# --------------------------
# Compute throughput gain in each data frame
thr_df_cpu["gain"] = thr_df_cpu["cycles_per_output"] / thr_df_cpu["cycles_per_output"]
thr_df_carus["gain"] = thr_df_cpu["cycles_per_output"] / thr_df_carus["cycles_per_output"]

# Keep only kernel, data type and throughput gain columns and reset index
thr_df_cpu = thr_df_cpu[["kernel_name", "data_type", "gain"]]
thr_df_carus = thr_df_carus[["kernel_name", "data_type", "gain"]]

# Transform rows with the same data type into columns
thr_df_cpu = thr_df_cpu.pivot(index="kernel_name", columns="data_type", values="gain")
thr_df_carus = thr_df_carus.pivot(index="kernel_name", columns="data_type", values="gain")

# Reorder columns
thr_df_cpu = thr_df_cpu[["int32", "int16", "int8"]]
thr_df_carus = thr_df_carus[["int32", "int16", "int8"]]

# Reorder rows as [xor, add, mul, matmul, gemm, conv2d, relu, leaky-relu, maxpool]
thr_df_cpu = thr_df_cpu.reindex(["xor", "add", "mul", "matmul", "gemm", "conv2d", "relu", "leaky-relu", "maxpool"])
thr_df_carus = thr_df_carus.reindex(["xor", "add", "mul", "matmul", "gemm", "conv2d", "relu", "leaky-relu", "maxpool"])

df = pd.concat([thr_df_cpu, thr_df_carus], axis=1, keys=["CPU", "NM-Carus"])
print(df)

# Plot Carus throughput bar chart
# - the kernel is the x-axis
# - the throughput gain is the y-axis
# - group columns by memory type
# - ignore the CPU
colors = ["#6d1a3680", "#6d1a36c0", "#6d1a36ff", "#00748080", "#007480c0", "#007480ff"]
patterns = ["/", "\\", "|", "-", "+", "x"]
df_bars = df[["NM-Carus"]]
df_bars.plot(kind="bar", rot=0, title="Cycles per output sample: CPU only vs. CPU + NMC (higher is better)", ylabel="Throughput w.r.t. CPU", xlabel="Benchmark application", figsize=(12, 3), grid=False, width=0.8, color=colors, logy=log_scale)

# Set patterns by series (broken)
# for i, bar in enumerate(plt.gca().patches):
#     hatch = patterns[i % len(patterns)]
#     bar.set_hatch(hatch)

# Draw horizontal grid only
plt.gca().yaxis.grid(True)

# Superpose an horizontal line at y=1 with label "CPU"
if not log_scale:
    plt.axhline(y=1, color="#3d3d3dff", linestyle="--")
    plt.text(-0.63, 2, "CPU", color="#3d3d3dff", rotation=0, fontsize="small")

# Set Y max value
if log_scale:
    plt.gca().set_ylim([1, 400])
else:
    plt.gca().set_ylim([0, 100])

# Add 'x' to the y-axis step labels
plt.gca().yaxis.set_major_formatter(lambda x, pos: str(int(x)) + "x")

# Set legend titles
plt.gca().set_title(plt.gca().get_title(), fontsize="large", color="#3d3d3dff")
plt.gca().set_xlabel(plt.gca().get_xlabel(), fontweight="bold")
plt.gca().set_ylabel(plt.gca().get_ylabel(), fontweight="bold")

# Legend as one row
if log_scale:
    plt.gca().legend(title="(Memory type, SW data type)", loc="upper left", ncol=3)
else:
    plt.gca().legend(title="(Memory type, SW data type)")

# Save chart to file
chart_file = f"{out_dir}/throughput-bench.png"
print(f"Saving energy benchmark chart to {chart_file}...")
plt.savefig(chart_file, dpi=600, bbox_inches="tight")


# Energy benchmark chart
# ----------------------
# Compute energy gain vs. CPU in each data frame
pwr_df_cpu["gain"] = pwr_df_cpu["energy"] / pwr_df_cpu["energy"]
pwr_df_carus["gain"] = pwr_df_cpu["energy"] / pwr_df_carus["energy"]

# Keep only kernel, data type and energy gain columns
energy_df_cpu = pwr_df_cpu[["kernel_name", "data_type", "gain"]]
energy_df_carus = pwr_df_carus[["kernel_name", "data_type", "gain"]]

# Transform rows with the same data type into columns
energy_df_cpu = energy_df_cpu.pivot(index="kernel_name", columns="data_type", values="gain")
energy_df_carus = energy_df_carus.pivot(index="kernel_name", columns="data_type", values="gain")

# Reorder columns
energy_df_cpu = energy_df_cpu[["int32", "int16", "int8"]]
energy_df_carus = energy_df_carus[["int32", "int16", "int8"]]

# Reorder rows as [xor, add, mul, matmul, gemm, conv2d, relu, leaky-relu, maxpool]
energy_df_cpu = energy_df_cpu.reindex(["xor", "add", "mul", "matmul", "gemm", "conv2d", "relu", "leaky-relu", "maxpool"])
energy_df_carus = energy_df_carus.reindex(["xor", "add", "mul", "matmul", "gemm", "conv2d", "relu", "leaky-relu", "maxpool"])

# Combine data frames, grouping columns by memory type
energy_df = pd.concat([energy_df_cpu, energy_df_carus], axis=1, keys=["CPU", "NM-Carus"])
print(energy_df)

# Plot Carus throughput bar chart
# - the kernel is the x-axis
# - the throughput gain is the y-axis
# - group columns by memory type
# - ignore the CPU
colors = ["#6d1a3680", "#6d1a36c0", "#6d1a36ff", "#00748080", "#007480c0", "#007480ff"]
df_bars = energy_df[["NM-Carus"]]
df_bars.plot(kind="bar", rot=0, title="Energy per output sample: CPU only vs. CPU + NMC (higher is better)", ylabel="Energy efficiency w.r.t. CPU", xlabel="Benchmark application", figsize=(12, 3), grid=False, width=0.8, color=colors, logy=log_scale)

# Draw horizontal grid only
plt.gca().yaxis.grid(True)

# Set suptitle
# plt.suptitle("heepatia Energy Efficiency Improvement", fontweight="bold", fontsize="x-large", color="#3d3d3dff")

# Superpose an horizontal line at y=1 with label "CPU"
if not log_scale:
    plt.axhline(y=1, color="#3d3d3dff", linestyle="--")
    plt.text(-0.62, 2, "CPU", color="#3d3d3dff", rotation=0, fontsize="small")

# Set Y max value
if log_scale:
    plt.gca().set_ylim([1, 400])
else:
    plt.gca().set_ylim([0, 100])

# Add 'x' to the y-axis step labels
plt.gca().yaxis.set_major_formatter(lambda x, pos: str(int(x)) + "x")

# Set legend titles
plt.gca().set_title(plt.gca().get_title(), fontsize="large", color="#3d3d3dff")
plt.gca().set_xlabel(plt.gca().get_xlabel(), fontweight="bold")
plt.gca().set_ylabel(plt.gca().get_ylabel(), fontweight="bold")

# Legend as one row
if log_scale:
    plt.gca().legend(title="(Memory type, SW data type)", loc="upper left", ncol=3)
else:
    plt.gca().legend(title="(Memory type, SW data type)")

# Save chart to file
chart_file = f"{out_dir}/energy-bench.png"
print(f"Saving energy benchmark chart to {chart_file}...")
plt.savefig(chart_file, dpi=600, bbox_inches="tight")


# Power breakdown chart
# ---------------------
# Select matmul kernel 8-bit power data
pwr_df_matmul = pwr_df.loc[pwr_df["kernel_name"] == "matmul"].reset_index(drop=True)
pwr_df_matmul = pwr_df_matmul.loc[pwr_df_matmul["data_type"] == "int8"].reset_index(drop=True)

# Plot power breakdown bars
plot_data = pwr_df_matmul[["memory_type", "nmc_mem_pwr", "nmc_comp_pwr", "nmc_ctl_pwr", "sys_mem_pwr", "sys_cpu_pwr", "sys_peri_pwr"]]

# Scale power data by 1000
plot_data.loc[:, ("nmc_mem_pwr", "nmc_comp_pwr", "nmc_ctl_pwr", "sys_mem_pwr", "sys_cpu_pwr", "sys_peri_pwr")] *= 1000

fig, ax = plt.subplots()

# Set colors
colors = ["#007480ff", "#007480c0", "#00748080", "#434343ff", "#434343c0", "#43434380"]
plot_data.set_index("memory_type").plot(kind="bar", stacked=True, color=colors, ax=ax, ylabel="Power [mW]", xlabel="Compute device", title="System power breakdown for 8-bit matrix multiplication")

# Print legend in reverse order
handles, labels = ax.get_legend_handles_labels()
ax.legend(reversed(handles), reversed(labels))

# Do not tilt x-axis labels and make them bold
plt.xticks(rotation=0)
plt.gca().set_xticklabels(plt.gca().get_xticklabels(), fontweight="bold")

plt.gca().set_ylim([0, 11])

# Sum of values
total_values = plot_data.sum(axis=1, numeric_only=True)

# Total values labels
for i, total in enumerate(total_values):
  ax.text(i, total + 0.1, round(total, 1), ha="center", va="bottom", color="#3d3d3dff")

# Draw horizontal grid only
plt.gca().yaxis.grid(True)

chart_file = f"{out_dir}/power-breakdown.png"
print(f"Saving power breakdown chart to {chart_file}...")
plt.savefig(chart_file, dpi=600, bbox_inches="tight")
