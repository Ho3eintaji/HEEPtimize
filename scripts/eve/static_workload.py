# import json

# # Load the power and timing data from JSON files
# with open('transformer-power.json', 'r') as f:
#     power_data = json.load(f)

# with open('tsd-time.json', 'r') as f:
#     timing_data = json.load(f)

# # Frequency in Hz (100 MHz)
# frequency = 100e6  # 100 MHz
# cycle_time_ns = 10  # Each cycle is 10 ns

# # Function to calculate total energy for a workload
# def calculate_total_energy(workload, power_data, timing_data):
#     total_energy = 0  # Total energy in mJ

#     for kernel, size, pe, voltage, num_repeats in workload:
#         # Get the number of cycles for the kernel
#         n_cycles = timing_data[kernel][size][pe]
        
#         # Calculate the time in seconds
#         time_ns = n_cycles * cycle_time_ns
#         time_s = time_ns * 1e-9
        
#         # Get the power in mW
#         power_mW = power_data[kernel][pe][voltage]["total"]
        
#         # Calculate the energy in mJ (power in mW * time in seconds)
#         energy_mJ = power_mW * time_s
        
#         # Multiply by the number of repetitions
#         total_energy += energy_mJ * num_repeats
    
#     return total_energy
import json
import argparse
import matplotlib.pyplot as plt
import numpy as np


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
    # plt.show()
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

# Function to generate the most energy-efficient workload
def generate_most_efficient_workload(timing_data, power_data, voltage):
    workload = []

    for kernel, sizes in timing_data.items():
        for size, details in sizes.items():
            # Skip entries that are purely comments (e.g., "_comment")
            if size.startswith("_comment"):
                continue

            # Get the number of repetitions
            num_repeats = details.get("_num_rpt", 1)

            # Find the most energy-efficient PE for this kernel and size
            min_energy = float('inf')
            best_pe = None

            for pe in details:
                if pe.startswith("_"):  # Skip comments and metadata
                    continue
                try:
                    energy, _ = calculate_energy(kernel, size, pe, voltage, timing_data, power_data)
                    if energy < min_energy:
                        min_energy = energy
                        best_pe = pe
                except KeyError:
                    continue  # Skip if power data is missing for this PE

            if best_pe is None:
                raise ValueError(f"No valid PE found for kernel {kernel} with size {size}")

            # Remove leading underscore from size (if any) for the workload entry
            clean_size = size.lstrip("_")

            # Add the workload entry
            workload.append((kernel, clean_size, best_pe, voltage, num_repeats))

    return workload

# Function to calculate total energy and timing for a workload
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

# Function to generate a workload
# def generate_workload(timing_data, pe_priority, voltage):
#     workload = []

#     for kernel, sizes in timing_data.items():
#         for size, details in sizes.items():
#             # Skip entries that start with "_" (comments or repeated sizes)
#             if size.startswith("_"):
#                 continue

#             # Get the number of repetitions
#             num_repeats = details.get("_num_rpt", 1)

#             # Determine the processing element (PE) based on priority
#             pe = None
#             for candidate_pe in pe_priority:
#                 if candidate_pe in details:
#                     pe = candidate_pe
#                     break

#             if pe is None:
#                 raise ValueError(f"No valid PE found for kernel {kernel} with size {size}")

#             # Add the workload entry
#             workload.append((kernel, size, pe, voltage, num_repeats))

#     return workload
# Function to generate a workload
def generate_workload(timing_data, pe_priority, voltage):
    workload = []

    for kernel, sizes in timing_data.items():
        for size, details in sizes.items():
            # Skip entries that are purely comments (e.g., "_comment")
            if size.startswith("_comment"):
                continue

            # Get the number of repetitions
            num_repeats = details.get("_num_rpt", 1)

            # Determine the processing element (PE) based on priority
            pe = None
            for candidate_pe in pe_priority:
                if candidate_pe in details:
                    pe = candidate_pe
                    break

            if pe is None:
                raise ValueError(f"No valid PE found for kernel {kernel} with size {size}")

            # Remove leading underscore from size (if any) for the workload entry
            clean_size = size.lstrip("_")

            # Add the workload entry
            workload.append((kernel, clean_size, pe, voltage, num_repeats))

    return workload

# Print the workloads
def print_workload(workload, title):
    print(f"\n{title}:")
    for entry in workload:
        print(entry)


# Parse command-line arguments
def parse_arguments():
    parser = argparse.ArgumentParser(description="Calculate total energy and timing for a workload.")
    parser.add_argument('--power', type=str, default=DEFAULT_POWER_JSON,
                        help=f"Path to power JSON file (default: {DEFAULT_POWER_JSON})")
    parser.add_argument('--timing', type=str, default=DEFAULT_TIMING_JSON,
                        help=f"Path to timing JSON file (default: {DEFAULT_TIMING_JSON})")
    return parser.parse_args()

# Main function
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

    # Example workload: list of (kernel, size, pe, voltage, num_repeats)
    workload = [
        ("add", "120x16", "cpu", "0.50V", 1),
        # ("add", "121x4", "carus", "0.65V", 48),
        # ("norm", "120x400", "cpu", "0.80V", 1),
        # ("clsConcatenate", "120x16", "cpu", "0.90V", 1),
    ]

    try:    
        # Calculate the total energy for the workload
        total_energy, time_ms = calculate_total_energy_and_time(workload, power_data, timing_data)
        print(f"Total time: {time_ms:.4f} ms")
        print(f"Total energy consumption: {total_energy:.4f} mJ")
        print(f"Total energy consumption: {(1e3 * total_energy):.4f} uJ")
    except ValueError as e:
        print(f"Error: {e}")

    workload = [
        ("add", "120x16", "carus", "0.50V", 2),
        ("add", "120x16", "cpu", "0.50V", 1),
        # ("add", "121x4", "carus", "0.65V", 48),
        # ("norm", "120x400", "cpu", "0.80V", 1),
        ("clsConcatenate", "120x16", "cpu", "0.90V", 1),
    ]

    try:    
        # Calculate the total energy for the workload
        total_energy, time_ms = calculate_total_energy_and_time(workload, power_data, timing_data)
        print(f"Total time: {time_ms:.4f} ms")
        print(f"Total energy consumption: {total_energy:.4f} mJ")
        print(f"Total energy consumption: {(1e3 * total_energy):.4f} uJ")
    except ValueError as e:
        print(f"Error: {e}")

    # workloads
    # Workload 1: Run everything on CPU at 0.8V
    workload_cpu = generate_workload(timing_data, pe_priority=["cpu"], voltage="0.80V")
    # Workload 2: Prefer Carus, fallback to CPU at 0.8V
    workload_cpu_carus = generate_workload(timing_data, pe_priority=["carus", "cpu"], voltage="0.80V")
    # Workload 3: Prefer CGRA, fallback to CPU at 0.8V
    workload_cpu_cgra = generate_workload(timing_data, pe_priority=["cgra", "cpu"], voltage="0.80V")

    # # Generate all four workloads
    # workload_cpu = generate_workload(timing_data, pe_priority=["cpu"], voltage="0.80V")
    # workload_cpu_carus = generate_workload(timing_data, pe_priority=["carus", "cpu"], voltage="0.80V")
    # workload_cpu_cgra = generate_workload(timing_data, pe_priority=["cgra", "cpu"], voltage="0.80V")
    workload_most_efficient = generate_most_efficient_workload(timing_data, power_data, voltage="0.80V")

    # print_workload(workload_cpu, "Workload 1: CPU Only at 0.8V")
    # print_workload(workload_cpu_carus, "Workload 2: Carus (Preferred) and CPU at 0.8V")
    # print_workload(workload_cpu_cgra, "Workload 3: CGRA (Preferred) and CPU at 0.8V")

    # Calculate total energy and time for each workload
    results = {
        "CPU Only": calculate_total_energy_and_time(workload_cpu, power_data, timing_data),
        "Carus + CPU": calculate_total_energy_and_time(workload_cpu_carus, power_data, timing_data),
        "CGRA + CPU": calculate_total_energy_and_time(workload_cpu_cgra, power_data, timing_data),
        "Most Efficient": calculate_total_energy_and_time(workload_most_efficient, power_data, timing_data),
    }

    # Print results
    for workload_name, (energy, time) in results.items():
        print(f"{workload_name}: Energy = {energy:.2f} mJ, Time = {time:.2f} ms")

    # Visualize the results
    visualize_results(results)

    # # Visualize the results
    # workload_names = list(results.keys())
    # energies = [results[name][0] for name in workload_names]
    # times = [results[name][1] for name in workload_names]

    # # Plot energy consumption
    # plt.figure(figsize=(10, 5))
    # plt.bar(workload_names, energies, color='blue')
    # plt.title("Energy Consumption for Different Workloads")
    # plt.ylabel("Energy (mJ)")
    # plt.xlabel("Workload")
    # # plt.show()
    # plt.savefig("energy_consumption.png")

    # # Plot timing
    # plt.figure(figsize=(10, 5))
    # plt.bar(workload_names, times, color='green')
    # plt.title("Execution Time for Different Workloads")
    # plt.ylabel("Time (ms)")
    # plt.xlabel("Workload")
    # # plt.show()
    # plt.savefig("execution_time.png")

if __name__ == "__main__":
    main()