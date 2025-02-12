import json
import argparse
import matplotlib.pyplot as plt
import numpy as np
from apps import TSD, EpilepticSeizureConv2DArchitecture, LCT_conv  # Import the applications

# Default file paths
DEFAULT_POWER_JSON = 'transformer-power.json'
DEFAULT_TIMING_JSON = 'tsd-time.json'

# Frequency in Hz (100 MHz)
frequency = 100e6  # 100 MHz
cycle_time_ns = 10  # Each cycle is 10 ns


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

def calculate_energy(kernel, size, pe, voltage, timing_data, power_data):
    # Get the number of cycles
    n_cycles = timing_data[kernel][size][pe]
    
    # Calculate time in seconds
    time_ns = n_cycles * cycle_time_ns
    time_s = time_ns * 1e-9
    
    # Get the power in mW
    power_mW = power_data[kernel][pe][voltage]["total"]
    
    # Calculate energy in mJ (power in mW * time in seconds)
    energy_mJ = power_mW * time_s
    
    return energy_mJ, time_ns

def generate_workload_from_app(app, pe_priority, voltage, timing_data):
    workload = []

    for kernel_info in app:
        kernel = kernel_info["kernel"]
        size = kernel_info["shape"]
        repeat = kernel_info["repeat"]
        data_type = kernel_info["dataType"]

        # Determine the processing element (PE) based on priority
        pe = None
        for candidate_pe in pe_priority:
            if candidate_pe in timing_data.get(kernel, {}).get(size, {}):
                pe = candidate_pe
                break

        if pe is None:
            raise ValueError(f"No valid PE found for kernel {kernel} with size {size}")

        # Add the workload entry
        workload.append((kernel, size, pe, voltage, repeat))

    return workload

def calculate_total_energy_and_time(workload, power_data, timing_data):
    total_energy = 0  # Total energy in mJ
    total_time_ns = 0  # Total time in nanoseconds

    for kernel, size, pe, voltage, num_repeats in workload:
        # Get the number of cycles for the kernel
        if kernel in timing_data and size in timing_data[kernel] and pe in timing_data[kernel][size]:
            n_cycles = timing_data[kernel][size][pe]
        else:
            raise ValueError(f"Invalid kernel, size, or PE: {kernel}, {size}, {pe}")

        # Calculate the time in nanoseconds
        time_ns = n_cycles * cycle_time_ns
        
        # Get the power in mW
        if kernel in power_data and pe in power_data[kernel] and voltage in power_data[kernel][pe]:
            power_mW = power_data[kernel][pe][voltage]["total"]
        else:
            raise ValueError(f"Invalid kernel, PE, or voltage: {kernel}, {pe}, {voltage}")

        # Calculate the energy in mJ (power in mW * time in seconds)
        energy_mJ = power_mW * (time_ns * 1e-9)
        
        # Multiply by the number of repetitions
        total_energy += energy_mJ * num_repeats
        total_time_ns += time_ns * num_repeats
    
    # Convert total time to milliseconds
    total_time_ms = total_time_ns * 1e-6

    return total_energy, total_time_ms

def parse_arguments():
    parser = argparse.ArgumentParser(description="Calculate total energy and timing for a workload.")
    parser.add_argument('--power', type=str, default=DEFAULT_POWER_JSON,
                        help=f"Path to power JSON file (default: {DEFAULT_POWER_JSON})")
    parser.add_argument('--timing', type=str, default=DEFAULT_TIMING_JSON,
                        help=f"Path to timing JSON file (default: {DEFAULT_TIMING_JSON})")
    parser.add_argument('--app', type=str, choices=['TSD', 'EpilepticSeizureConv2DArchitecture', 'LCT_conv'],
                        default='TSD', help="Application to run (default: TSD)")
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
    elif args.app == 'EpilepticSeizureConv2DArchitecture':
        app = EpilepticSeizureConv2DArchitecture
    elif args.app == 'LCT_conv':
        app = LCT_conv
    else:
        raise ValueError(f"Invalid application: {args.app}")

    # Generate workloads with different policies
    workload_cpu = generate_workload_from_app(app, pe_priority=["cpu"], voltage="0.80V", timing_data=timing_data)
    workload_cpu_carus = generate_workload_from_app(app, pe_priority=["carus", "cpu"], voltage="0.80V", timing_data=timing_data)
    workload_cpu_cgra = generate_workload_from_app(app, pe_priority=["cgra", "cpu"], voltage="0.80V", timing_data=timing_data)
    
    # Calculate total energy and time for each workload
    results = {
        "CPU Only": calculate_total_energy_and_time(workload_cpu, power_data, timing_data),
        "Carus + CPU": calculate_total_energy_and_time(workload_cpu_carus, power_data, timing_data),
        "CGRA + CPU": calculate_total_energy_and_time(workload_cpu_cgra, power_data, timing_data),
    }

    # Print results
    for workload_name, (energy, time) in results.items():
        print(f"{workload_name}: Energy = {energy:.2f} mJ, Time = {time:.2f} ms")

    # Visualize the results
    visualize_results(results)

if __name__ == "__main__":
    main()