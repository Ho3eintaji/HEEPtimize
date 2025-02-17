import json
import argparse
import matplotlib.pyplot as plt
import numpy as np
from apps import TSD, SeizConv2D, LCT_conv  
from timing_helper import get_cycles   # Import get_cycles (and related helpers if needed)

DEFAULT_APP = 'SeizConv2D'
VOLTAGE = "0.90V"
FREQ_MHZ = 100

DEFAULT_POWER_JSON = 'transformer-power.json'
DEFAULT_TIMING_JSON = 'kernels_pe-time.json'


# Frequency in Hz (100 MHz)
frequency = FREQ_MHZ * 1e6
cycle_time_ns = 1 / frequency * 1e9  # Each cycle is 10 ns


# Add kernel mapping for power lookups
powers_kernel_mapping = {
    "conv2d": "matmul",
    "relu": "matmul"

}

def visualize_results(results):
    workload_names = list(results.keys())
    energies = [results[name][0] for name in workload_names]
    times = [results[name][1] for name in workload_names]

    # Set up the figure and subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    fig.suptitle("Workload Comparison: Energy and Timing", fontsize=16, fontweight='bold')

    # Plot energy consumption
    bars1 = ax1.bar(workload_names, energies, color='skyblue', edgecolor='black', alpha=0.8)
    ax1.set_title("Energy Consumption", fontsize=14, fontweight='bold')
    ax1.set_ylabel("Energy (mJ)", fontsize=12)
    ax1.set_xlabel("Workload", fontsize=12)
    ax1.grid(axis='y', linestyle='--', alpha=0.7)
    ax1.set_xticks(range(len(workload_names)))
    ax1.set_xticklabels(workload_names, rotation=45, ha='right', fontsize=10)

    # Add annotations (values on top of bars)
    for bar in bars1:
        height = bar.get_height()
        ax1.annotate(f'{height:.2f}',
                     xy=(bar.get_x() + bar.get_width() / 2, height),
                     xytext=(0, 3),  # 3 points vertical offset
                     textcoords="offset points",
                     ha='center', va='bottom', fontsize=10)

    # Plot execution time
    bars2 = ax2.bar(workload_names, times, color='lightgreen', edgecolor='black', alpha=0.8)
    ax2.set_title("Execution Time", fontsize=14, fontweight='bold')
    ax2.set_ylabel("Time (ms)", fontsize=12)
    ax2.set_xlabel("Workload", fontsize=12)
    ax2.grid(axis='y', linestyle='--', alpha=0.7)
    ax2.set_xticks(range(len(workload_names)))
    ax2.set_xticklabels(workload_names, rotation=45, ha='right', fontsize=10)

    # Add annotations (values on top of bars)
    for bar in bars2:
        height = bar.get_height()
        ax2.annotate(f'{height:.2f}',
                     xy=(bar.get_x() + bar.get_width() / 2, height),
                     xytext=(0, 3),  # 3 points vertical offset
                     textcoords="offset points",
                     ha='center', va='bottom', fontsize=10)

    # Adjust layout to prevent overlap
    plt.tight_layout()
    plt.savefig("workload_comparison.png")

def calculate_energy(kernel, size, pe, voltage, datatype, timing_data, power_data):
    cycles, estimated = get_cycles(kernel, size, pe, datatype, timing_data)
    time_ns = cycles * cycle_time_ns
    time_s = time_ns * 1e-9
    # Map kernel name for power data lookup if necessary
    power_kernel = powers_kernel_mapping.get(kernel, kernel)
    power_mW = power_data[power_kernel][pe][voltage]["total"]
    energy_mJ = power_mW * time_s
    if estimated:
        print(f"Note: Estimated cycles of {cycles} for {kernel} ({size}) on {pe} with {datatype}")
    return energy_mJ, time_ns

# Fix generate_workload_from_app: check group existence rather than key membership for candidate_pe
def generate_workload_from_app(app, pe_priority, voltage, timing_data):
    workload = []
    for kernel_info in app:
        kernel = kernel_info["kernel"]
        size = kernel_info["shape"]
        repeat = kernel_info["repeat"]
        data_type = kernel_info["dataType"]
        pe = None
        for candidate_pe in pe_priority:
            # Check if the group exists and contains any size keys
            if timing_data.get(kernel, {}).get(candidate_pe, {}).get(data_type, {}):
                pe = candidate_pe
                break
        if pe is None:
            raise ValueError(f"No valid PE found for kernel {kernel} with size {size} and datatype {data_type}")
        workload.append((kernel, size, pe, voltage, repeat, data_type))
    return workload

# New function to generate workload based on minimal energy consumption per kernel
def generate_energy_efficient_workload(app, voltage, timing_data, power_data):
    workload = []
    for kernel_info in app:
        kernel = kernel_info["kernel"]
        size = kernel_info["shape"]
        repeat = kernel_info["repeat"]
        data_type = kernel_info["dataType"]
        power_kernel = powers_kernel_mapping.get(kernel, kernel)
        best_energy = None
        best_candidate = None
        for candidate_pe, candidate_data in power_data.get(power_kernel, {}).items():
            if voltage not in candidate_data:
                continue
            try:
                cycles, estimated = get_cycles(kernel, size, candidate_pe, data_type, timing_data)
            except Exception:
                continue
            time_ns = cycles * cycle_time_ns
            time_s = time_ns * 1e-9
            power_mW = candidate_data[voltage]["total"]
            energy = power_mW * time_s
            if best_energy is None or energy < best_energy:
                best_energy = energy
                best_candidate = candidate_pe
        if best_candidate is None:
            raise ValueError(f"No valid energy efficient PE found for kernel {kernel}")
        workload.append((kernel, size, best_candidate, voltage, repeat, data_type))
    return workload

def calculate_total_energy_and_time(workload, power_data, timing_data):
    total_energy = 0
    total_time_ns = 0
    for kernel, size, pe, voltage, num_repeats, datatype in workload:
        try:
            cycles, estimated = get_cycles(kernel, size, pe, datatype, timing_data)
        except ValueError:
            raise ValueError(f"Invalid kernel/pe/datatype combination: {kernel}, {pe}, {datatype}")
        time_ns = cycles * cycle_time_ns
        power_kernel = powers_kernel_mapping.get(kernel, kernel)
        if power_kernel in power_data and pe in power_data[power_kernel] and voltage in power_data[power_kernel][pe]:
            power_mW = power_data[power_kernel][pe][voltage]["total"]
        else:
            raise ValueError(f"Invalid kernel, PE, or voltage: {kernel}, {pe}, {voltage}")
        energy_mJ = power_mW * (time_ns * 1e-9)
        total_energy += energy_mJ * num_repeats
        total_time_ns += time_ns * num_repeats
        if estimated:
            print(f"Note: Estimated cycles of {cycles} for {kernel} ({size}) on {pe} with {datatype}")
    total_time_ms = total_time_ns * 1e-6
    return total_energy, total_time_ms

def parse_arguments():
    parser = argparse.ArgumentParser(description="Calculate total energy and timing for a workload.")
    parser.add_argument('--power', type=str, default=DEFAULT_POWER_JSON,
                        help=f"Path to power JSON file (default: {DEFAULT_POWER_JSON})")
    parser.add_argument('--timing', type=str, default=DEFAULT_TIMING_JSON,
                        help=f"Path to timing JSON file (default: {DEFAULT_TIMING_JSON})")
    parser.add_argument('--app', type=str, choices=['TSD', 'EpilepticSeizureConv2DArchitecture', 'LCT_conv'],
                        default=DEFAULT_APP, help="Application to run (default: TSD)")
    return parser.parse_args()

def main():
    # Parse command-line arguments
    args = parse_arguments()

    # Load the power and timing data from JSON files
    try:
        with open(args.power, 'r') as f:
            power_data = json.load(f)

        with open(args.timing, 'r') as f:
            timing_data = json.load(f)
    except FileNotFoundError as e:
        print(f"Error: {e}")
        return

    # Select the application based on the argument
    if args.app == 'TSD':
        app = TSD
    elif args.app == 'SeizConv2D':
        app = SeizConv2D
    elif args.app == 'LCT_conv':
        app = LCT_conv
    else:
        raise ValueError(f"Invalid application: {args.app}")

    # Generate workloads with different policies
    workload_cpu = generate_workload_from_app(app, pe_priority=["cpu"], voltage=VOLTAGE, timing_data=timing_data)
    workload_cpu_carus = generate_workload_from_app(app, pe_priority=["carus", "cpu"], voltage=VOLTAGE, timing_data=timing_data)
    workload_cpu_cgra = generate_workload_from_app(app, pe_priority=["cgra", "cpu"], voltage=VOLTAGE, timing_data=timing_data)
    workload_energy_efficient = generate_energy_efficient_workload(app, VOLTAGE, timing_data, power_data)
    
    # Calculate total energy and time for each workload
    results = {
        "CPU Only": calculate_total_energy_and_time(workload_cpu, power_data, timing_data),
        "Carus + CPU": calculate_total_energy_and_time(workload_cpu_carus, power_data, timing_data),
        "CGRA + CPU": calculate_total_energy_and_time(workload_cpu_cgra, power_data, timing_data),
        "Energy Efficient": calculate_total_energy_and_time(workload_energy_efficient, power_data, timing_data)
    }

    print(workload_cpu_carus)

    # Print results
    for workload_name, (energy, time) in results.items():
        print(f"{workload_name}: Energy = {energy:.3f} mJ, Time = {time:.2f} ms")

    # Visualize the results
    visualize_results(results)

if __name__ == "__main__":
    main()