"""
Copyright 2024 EPFL

File: power-analysis.python
Author:Hossein Taji
Date: 03/10/2024
Description: Matmul Simulation Data Extraction and Analysis Script

This script extracts simulation data from specified directories, processes power consumption data,
and provides methods for querying and analyzing the data. It also includes a data analysis class
to generate graphs based on various parameters.

Key Features:
- Extracts power consumption data for matrix multiplication operations across different PEs (Processing Elements).
- Supports querying power consumption based on PE type, matrix sizes, voltage, frequency, and power domains.
- Integrates maximum frequency constraints based on voltage levels.
- Provides data analysis capabilities to generate graphs for various scenarios.

Usage:
- Instantiate the MatmulSimulationData class and extract data.
- Use the query_power method to obtain power consumption data.
- Use the MatmulDataAnalysis class to generate graphs.

Dependencies:
- Python 3.x
- matplotlib (for plotting graphs)
- pickle (for data serialization)

"""

import os
import re
import csv
import pickle
import matplotlib.pyplot as plt
from itertools import product
import matplotlib.pyplot as plt
import numpy as np


class MatmulSimulationData:
    """
    A class to extract, store, and query matrix multiplication simulation data.
    """

    def __init__(self):
        """
        Initializes the MatmulSimulationData object with an empty data list and max frequency constraints.
        """
        # Data storage: a list of dictionaries
        self.data_list = []

        # Max frequency constraints based on voltage (MHz)
        self.max_frequency = {
            0.5: 122,  # Corresponds to 8.2ns
            0.65: 347,  # Corresponds to 2.88ns
            0.8: 578,  # Corresponds to 1.73ns
            0.9: 690,  # Corresponds to 1.45ns
        }

    def extract_data(self, root_dir='private/matmul_postsynth_sims'):
        """
        Extracts data from simulation files in the specified root directory.

        Args:
            root_dir (str): The root directory containing simulation data.
        """
        # Allow specifying root directory
        self.root_dir = root_dir

        # Define the mapping from PWR_MODE to voltage
        voltage_map = {
            'tt_0p50_25': 0.5,
            'tt_0p65_25': 0.65,
            'tt_0p80_25': 0.8,
            'tt_0p90_25': 0.9,
        }

        # Define the categories and their corresponding cells
        pow_sys_cells = [
            'u_core_v_mini_mcu/system_bus_i',
            'u_core_v_mini_mcu/ao_peripheral_subsystem_i/power_manager_i',
            'u_core_v_mini_mcu/ao_peripheral_subsystem_i/boot_rom_i',
            'u_core_v_mini_mcu/ao_peripheral_subsystem_i/dma_i',
            'u_core_v_mini_mcu/ao_peripheral_subsystem_i/soc_ctrl_i',
            'u_core_v_mini_mcu/ao_peripheral_subsystem_i/fast_intr_ctrl_i',
            'u_core_v_mini_mcu/peripheral_subsystem_i/rv_plic_i',
            'u_heepatia_peripherals/u_heepatia_ctrl_reg',
            'u_heepatia_peripherals/u_fll_subsystem',
            'u_heepatia_bus',
        ]

        pow_cpu_cells = [
            'u_core_v_mini_mcu/cpu_subsystem_i',
        ]

        pow_carus_cells = [
            'u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper',
        ]

        pow_cgra_cells = [
            'u_heepatia_peripherals/u_cgra_top_wrapper',
        ]

        pow_caesar_cells = [
            'u_heepatia_peripherals/gen_caesar_0__u_nm_caesar_wrapper',
        ]

        # For pow_mem, match all 'ramX_i' cells
        pow_mem_pattern = re.compile(r'u_core_v_mini_mcu/memory_subsystem_i/ram\d*_i')

        # Start processing directories
        for dir_name in os.listdir(self.root_dir):
            dir_path = os.path.join(self.root_dir, dir_name)
            if os.path.isdir(dir_path):
                # Parse the directory name
                pattern = r'ra(\d+)_ca(\d+)_cb(\d+)_(\w+)_(tt_\d+p\d+_25)'
                match = re.match(pattern, dir_name)
                if match:
                    row_a = int(match.group(1))
                    col_a = int(match.group(2))
                    col_b = int(match.group(3))
                    PE = match.group(4)
                    PWR_MODE = match.group(5)
                    voltage = voltage_map.get(PWR_MODE, None)
                    if voltage is None:
                        print(f"Unknown PWR_MODE {PWR_MODE} in {dir_name}")
                        continue

                    # Initialize data dictionary
                    data = {
                        'operation': 'matmul',
                        'row_a': row_a,
                        'col_a': col_a,
                        'col_b': col_b,
                        'PE': PE,
                        'voltage': voltage,
                    }

                    # Read clk_ns.txt
                    clk_ns_file = os.path.join(dir_path, 'clk_ns.txt')
                    if os.path.isfile(clk_ns_file):
                        with open(clk_ns_file, 'r') as f:
                            clk_ns_str = f.read().strip()
                            try:
                                clk_ns = float(clk_ns_str)
                                data['clock_period_ns'] = clk_ns
                                data['clock_frequency_MHz'] = 1e3 / clk_ns  # MHz
                            except ValueError:
                                print(f"Invalid clock period in {clk_ns_file}")
                                continue
                    else:
                        print(f"clk_ns.txt not found in {dir_path}")
                        continue

                    # Read time-?.txt
                    time_file = None
                    for filename in os.listdir(dir_path):
                        if filename.startswith('time-') and filename.endswith('.txt'):
                            time_file = os.path.join(dir_path, filename)
                            break
                    if time_file:
                        with open(time_file, 'r') as f:
                            time_ns_str = f.read().strip()
                            try:
                                execution_time_ns = float(time_ns_str)
                                data['execution_time_ns'] = execution_time_ns
                            except ValueError:
                                print(f"Invalid execution time in {time_file}")
                                continue
                    else:
                        print(f"time-?.txt not found in {dir_path}")
                        continue

                    # Read power.csv
                    power_csv_file = os.path.join(dir_path, 'power.csv')
                    if os.path.isfile(power_csv_file):
                        # Initialize power accumulators (in mW)
                        data['pow_sys_dyn'] = 0.0
                        data['pow_sys_static'] = 0.0
                        data['pow_cpu_dyn'] = 0.0
                        data['pow_cpu_static'] = 0.0
                        data['pow_mem_dyn'] = 0.0
                        data['pow_mem_static'] = 0.0
                        data['pow_carus_dyn'] = 0.0
                        data['pow_carus_static'] = 0.0
                        data['pow_cgra_dyn'] = 0.0
                        data['pow_cgra_static'] = 0.0
                        data['pow_caesar_dyn'] = 0.0
                        data['pow_caesar_static'] = 0.0

                        # Open and read the CSV file
                        with open(power_csv_file, 'r') as csvfile:
                            reader = csv.DictReader(csvfile)
                            for row in reader:
                                CELL = row['CELL'].strip()
                                try:
                                    INTERNAL_POWER = float(row['INTERNAL_POWER'])
                                    SWITCHING_POWER = float(row['SWITCHING_POWER'])
                                    LEAKAGE_POWER = float(row['LEAKAGE_POWER'])
                                except ValueError:
                                    # Skip rows with invalid data
                                    continue
                                # Convert power to mW
                                dynamic_power = (INTERNAL_POWER + SWITCHING_POWER) * 1e3
                                static_power = LEAKAGE_POWER * 1e3

                                # Check if CELL belongs to pow_sys
                                if CELL in pow_sys_cells:
                                    data['pow_sys_dyn'] += dynamic_power
                                    data['pow_sys_static'] += static_power

                                # Check if CELL matches pow_mem pattern
                                if pow_mem_pattern.match(CELL):
                                    data['pow_mem_dyn'] += dynamic_power
                                    data['pow_mem_static'] += static_power

                                # Check if CELL belongs to pow_cpu
                                if CELL in pow_cpu_cells:
                                    data['pow_cpu_dyn'] += dynamic_power
                                    data['pow_cpu_static'] += static_power

                                # Check if CELL belongs to pow_carus
                                if CELL in pow_carus_cells:
                                    data['pow_carus_dyn'] += dynamic_power
                                    data['pow_carus_static'] += static_power

                                # Check if CELL belongs to pow_cgra
                                if CELL in pow_cgra_cells:
                                    data['pow_cgra_dyn'] += dynamic_power
                                    data['pow_cgra_static'] += static_power

                                # Check if CELL belongs to pow_caesar
                                if CELL in pow_caesar_cells:
                                    data['pow_caesar_dyn'] += dynamic_power
                                    data['pow_caesar_static'] += static_power

                        # Append the data to the list
                        self.data_list.append(data)
                    else:
                        print(f"power.csv not found in {dir_path}")
                        continue
                else:
                    print(f"Directory name {dir_name} does not match expected pattern")
                    continue

    def save_data(self, filename='simulation_data.pkl'):
        """
        Saves the extracted data to a file for future use.

        Args:
            filename (str): The file name to save the data.
        """
        with open(filename, 'wb') as f:
            pickle.dump(self.data_list, f)
        print(f"Data saved to {filename}")

    def load_data(self, filename='simulation_data.pkl'):
        """
        Loads data from a file.

        Args:
            filename (str): The file name from which to load the data.
        """
        with open(filename, 'rb') as f:
            self.data_list = pickle.load(f)
        print(f"Data loaded from {filename}")

    def query_power(self, PE, row_a, col_a, col_b, voltage, frequency_MHz=None, power_type='total', domains=None):
        """
        Queries power consumption data based on specified parameters.

        Args:
            PE (str): The processing element ('carus', 'caesar', 'cgra', 'cpu').
            row_a (int): Number of rows in matrix A.
            col_a (int): Number of columns in matrix A.
            col_b (int): Number of columns in matrix B.
            voltage (float): Voltage level (e.g., 0.5, 0.65, 0.8, 0.9).
            frequency_MHz (float): Frequency in MHz (optional).
            power_type (str): 'total', 'dynamic', 'static', 'dyn', or 'stat' (default is 'total').
            domains (list): List of power domains to include (optional).

        Returns:
            dict: A dictionary containing the queried data.
        """
        # Find matching data entries
        matches = [d for d in self.data_list if
                   d['PE'] == PE and
                   d['row_a'] == row_a and
                   d['col_a'] == col_a and
                   d['col_b'] == col_b and
                   d['voltage'] == voltage]

        if not matches:
            print("No matching data found.")
            return None

        # For simplicity, take the first matching entry
        data = matches[0]

        # Validate power_type
        valid_power_types = ['total', 'dynamic', 'static', 'dyn', 'stat']
        if power_type not in valid_power_types:
            print(f"Invalid power_type '{power_type}'. Defaulting to 'total'.")
            power_type = 'total'

        # Determine scaling factor
        if frequency_MHz is not None:
            # Check max frequency constraint
            max_freq = self.max_frequency.get(voltage, None)
            if max_freq is None:
                print(f"No max frequency data for voltage {voltage}V.")
                return None
            if frequency_MHz > max_freq:
                print(f"Frequency {frequency_MHz} MHz exceeds max frequency {max_freq} MHz at {voltage}V.")
                return None
            original_frequency = data['clock_frequency_MHz']
            scaling_factor = frequency_MHz / original_frequency
            scaled_execution_time_ns = data['execution_time_ns'] / scaling_factor
            # Scale dynamic power linearly with frequency
            # Static power remains the same
        else:
            scaling_factor = 1.0
            frequency_MHz = data['clock_frequency_MHz']
            scaled_execution_time_ns = data['execution_time_ns']

        # Determine domains to include
        all_domains = ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_carus', 'pow_cgra', 'pow_caesar']

        if domains is not None and isinstance(domains, list) and domains:
            # Use specified domains
            selected_domains = domains
        else:
            # Use default domains based on PE
            if PE == 'carus':
                selected_domains = ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_carus']
            elif PE == 'caesar':
                selected_domains = ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_caesar']
            elif PE == 'cgra':
                selected_domains = ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_cgra']
            elif PE == 'cpu':
                selected_domains = ['pow_sys', 'pow_cpu', 'pow_mem']
            else:
                print(f"Unknown PE type: {PE}")
                return None

        # Sum the relevant power categories
        total_dyn_power = 0.0
        total_static_power = 0.0

        # Dynamic power scaling
        def scale_dyn_power(p):
            return p * scaling_factor

        for domain in selected_domains:
            dyn_key = domain + '_dyn'
            static_key = domain + '_static'
            if dyn_key in data and static_key in data:
                total_dyn_power += scale_dyn_power(data[dyn_key])
                total_static_power += data[static_key]
            else:
                print(f"Unknown power domain: {domain}")

        # Determine which power to report based on power_type
        if power_type in ['total', None]:
            total_power = total_dyn_power + total_static_power
            reported_power = total_power
        elif power_type in ['dynamic', 'dyn']:
            reported_power = total_dyn_power
        elif power_type in ['static', 'stat']:
            reported_power = total_static_power
        else:
            # Should not reach here, but just in case
            total_power = total_dyn_power + total_static_power
            reported_power = total_power

        # Prepare the result
        result = {
            'PE': PE,
            'row_a': row_a,
            'col_a': col_a,
            'col_b': col_b,
            'voltage': voltage,
            'frequency_MHz': frequency_MHz,
            'execution_time_ns': scaled_execution_time_ns,
            'power_mW': reported_power,
            'power_type': power_type,
            'domains': selected_domains,
        }

        # If domains are specified, and it's a single domain, override the power_mW with the domain's power
        if domains is not None and len(domains) == 1:
            domain = domains[0]
            dyn_key = domain + '_dyn'
            static_key = domain + '_static'
            domain_dyn_power = scale_dyn_power(data.get(dyn_key, 0.0))
            domain_static_power = data.get(static_key, 0.0)
            if power_type in ['total', None]:
                result['power_mW'] = domain_dyn_power + domain_static_power
            elif power_type in ['dynamic', 'dyn']:
                result['power_mW'] = domain_dyn_power
            elif power_type in ['static', 'stat']:
                result['power_mW'] = domain_static_power

        return result

    def print_power_report(self, PE, row_a, col_a, col_b, voltage, frequency_MHz=None, power_type='total', domains=None):
        """
        Prints a power consumption report based on specified parameters.

        Args:
            PE (str): The processing element ('carus', 'caesar', 'cgra', 'cpu').
            row_a (int): Number of rows in matrix A.
            col_a (int): Number of columns in matrix A.
            col_b (int): Number of columns in matrix B.
            voltage (float): Voltage level.
            frequency_MHz (float): Frequency in MHz (optional).
            power_type (str): 'total', 'dynamic', 'static', 'dyn', or 'stat' (default is 'total').
            domains (list): List of power domains to include (optional).
        """
        result = self.query_power(PE, row_a, col_a, col_b, voltage, frequency_MHz, power_type, domains)
        if result:
            print("Power Consumption Report:")
            print(f"PE: {result['PE']}")
            print(f"Matrix Size: {result['row_a']}x{result['col_a']} * {result['col_a']}x{result['col_b']}")
            print(f"Voltage: {result['voltage']}V")
            print(f"Frequency: {result['frequency_MHz']} MHz")
            print(f"Execution Time: {result['execution_time_ns']} ns")
            if domains is None or len(domains) > 1:
                print(f"Included Domains: {', '.join(result['domains'])}")
            elif len(domains) == 1:
                print(f"Domain: {result['domains'][0]}")
            power_label = result['power_type'].capitalize() + " Power"
            print(f"{power_label}: {result['power_mW']} mW")
        else:
            print("No data to report.")
    



class MatmulDataAnalysis:
    """
    A class for analyzing and plotting matrix multiplication simulation data.
    """

    def __init__(self, simulation_data):
        """
        Initializes the MatmulDataAnalysis object.

        Args:
            simulation_data (MatmulSimulationData): An instance of MatmulSimulationData.
        """
        self.sim_data = simulation_data

    def plot_power_vs_voltage(self, PE, row_a, col_a, col_b):
        """
        Plots execution time and average power versus voltage at max frequency for the given PE and matrix size.

        Args:
            PE (str): The processing element.
            row_a (int): Number of rows in matrix A.
            col_a (int): Number of columns in matrix A.
            col_b (int): Number of columns in matrix B.
        """
        voltages = sorted(self.sim_data.max_frequency.keys())
        times = []
        powers = []
        energies = []

        for voltage in voltages:
            max_freq = self.sim_data.max_frequency[voltage]
            result = self.sim_data.query_power(
                PE=PE,
                row_a=row_a,
                col_a=col_a,
                col_b=col_b,
                voltage=voltage,
                frequency_MHz=max_freq
            )
            if result:
                execution_time_s = result['execution_time_ns'] * 1e-9
                power_W = result['power_mW'] * 1e-3
                energy_J = execution_time_s * power_W
                times.append(execution_time_s)
                powers.append(power_W)
                energies.append(energy_J)
            else:
                times.append(None)
                powers.append(None)
                energies.append(None)

        fig, ax1 = plt.subplots(figsize=(12, 7))
        # First plot - Execution Time vs Voltage
        ax1.plot(voltages, times, marker='o', linestyle='-', color='b', markersize=8, linewidth=2, label='Execution Time (s)')
        ax1.set_xlabel('Voltage (V)', fontsize=14)
        ax1.set_ylabel('Execution Time (s)', color='b', fontsize=14)
        ax1.tick_params(axis='y', labelcolor='b', labelsize=12)
        # Second y-axis - Average Power vs Voltage
        ax2 = ax1.twinx()
        ax2.plot(voltages, powers, marker='o', linestyle='-', color='g', markersize=8, linewidth=2, label='Power (W)')
        ax2.set_ylabel('Power (W)', color='g', fontsize=14)
        ax2.tick_params(axis='y', labelcolor='g', labelsize=12)
        # Third y-axis - Energy vs Voltage
        ax3 = ax1.twinx()
        ax3.spines['right'].set_position(('outward', 60))  # Offset the third axis
        ax3.plot(voltages, energies, marker='o', linestyle='-', color='r', markersize=8, linewidth=2, label='Energy (J)')
        ax3.set_ylabel('Energy (J)', color='r', fontsize=14)
        ax3.tick_params(axis='y', labelcolor='r', labelsize=12)
        plt.title(f'Execution Time, Power, and Energy vs Voltage for {PE} PE, {row_a}x{col_a} * {col_a}x{col_b}', fontsize=16, fontweight='bold')
        ax1.grid(True, which='both', linestyle='--', linewidth=0.5)
        plt.tight_layout()
        plt.savefig('power_vs_voltage.pdf', bbox_inches='tight')

    def plot_energy_vs_frequency(self, PE, row_a, col_a, col_b, voltage):
        """
        Plots total energy versus frequency for the given PE, matrix size, and voltage.

        Args:
            PE (str): The processing element.
            row_a (int): Number of rows in matrix A.
            col_a (int): Number of columns in matrix A.
            col_b (int): Number of columns in matrix B.
            voltage (float): Voltage level.
        """
        max_freq = self.sim_data.max_frequency.get(voltage, None)
        if max_freq is None:
            print(f"No max frequency data for voltage {voltage}V.")
            return

        frequencies = []
        energies = []

        # Generate frequencies from a low value to max frequency
        freq_values = [max_freq * x / 10 for x in range(1, 11)]

        for freq in freq_values:
            result = self.sim_data.query_power(
                PE=PE,
                row_a=row_a,
                col_a=col_a,
                col_b=col_b,
                voltage=voltage,
                frequency_MHz=freq
            )
            if result:
                execution_time_s = result['execution_time_ns'] * 1e-9
                power_W = result['power_mW'] * 1e-3
                energy_J = execution_time_s * power_W
                frequencies.append(freq)
                energies.append(energy_J)
            else:
                frequencies.append(None)
                energies.append(None)

        # Plotting
        plt.figure(figsize=(10, 7))

        # Plot the data with enhanced markers and line style
        plt.plot(frequencies, energies, marker='o', linestyle='-', color='b', markersize=8, linewidth=2)

        # Add title and labels with enhanced font sizes
        # shows also selected PE, matrix size, and voltage
        # plt.title(f'Energy vs Frequency at {voltage}V for {row_a}x{col_a} * {col_a}x{col_b}', fontsize=16, fontweight='bold')
        plt.title(f'Energy vs Frequency at {voltage}V for {PE} PE, {row_a}x{col_a} * {col_a}x{col_b}', fontsize=16, fontweight='bold')
        plt.xlabel('Frequency (MHz)', fontsize=14)
        plt.ylabel('Energy (J)', fontsize=14)

        # Add grid and improve readability with larger ticks
        plt.grid(True, which='both', linestyle='--', linewidth=0.5)
        plt.xticks(fontsize=12)
        plt.yticks(fontsize=12)

        # Optional minor grid lines
        plt.minorticks_on()
        plt.grid(which='minor', linestyle=':', linewidth=0.5)

        # Save the plot
        plt.savefig('energy_vs_frequency.pdf', bbox_inches='tight')

    def plot_power_vs_size(self, ra, ca, voltage, PEs=None):
        """
        Plots energy versus matrix size (varying cb) for specified PEs at the given voltage and max frequency.

        Args:
            ra (int): Number of rows in matrix A.
            ca (int): Number of columns in matrix A.
            voltage (float): Voltage level.
            PEs (list, optional): List of PEs to include in the plot. Defaults to ['carus', 'caesar', 'cgra', 'cpu'].
        """
        max_freq = self.sim_data.max_frequency.get(voltage, None)
        if max_freq is None:
            print(f"No max frequency data for voltage {voltage}V.")
            return

        if PEs is None:
            PEs = ['carus', 'caesar', 'cgra', 'cpu']

        # Collect cb values for which data is available for the given ra, ca, and voltage
        cb_values = set()
        for data in self.sim_data.data_list:
            if data['row_a'] == ra and data['col_a'] == ca and data['voltage'] == voltage and data['PE'] in PEs:
                cb_values.add(data['col_b'])
        cb_values = sorted(cb_values)

        if not cb_values:
            print(f"No data available for ra={ra}, ca={ca}, voltage={voltage}V.")
            return

        power_data = {PE: [] for PE in PEs}

        for cb in cb_values:
            for PE in PEs:
                result = self.sim_data.query_power(
                    PE=PE,
                    row_a=ra,
                    col_a=ca,
                    col_b=cb,
                    voltage=voltage,
                    frequency_MHz=max_freq
                )
                if result:
                    power_W = result['power_mW'] * 1e-3
                    power_data[PE].append(power_W)
                else:
                    power_data[PE].append(None)


        # Plotting
        plt.figure(figsize=(10, 7))

        for PE in PEs:
            # Filter out None values
            power_values = power_data[PE]
            if any(power_values):
                plt.plot(cb_values, power_values, marker='o', linestyle='-', markersize=8, linewidth=2, label=PE)

        pe_labels = ', '.join(PEs)
        plt.title(
            f'Power vs Matrix Size (cb) at {voltage}V and max frequency\n'
            f'for {ra}x{ca} * {ca}x(cb)',
            fontsize=16, fontweight='bold'
        )
        plt.xlabel('cb', fontsize=14)
        plt.ylabel('Power (W)', fontsize=14)
        plt.grid(True, which='both', linestyle='--', linewidth=0.5)
        plt.legend(fontsize=12)
        plt.xticks(cb_values, fontsize=12)  # Show only the cb_values we have data for
        plt.yticks(fontsize=12)
        plt.minorticks_on()
        plt.grid(which='minor', linestyle=':', linewidth=0.5)
        plt.savefig(f'power_vs_size_ra{ra}xca{ca}_{voltage}V.pdf', bbox_inches='tight')
        plt.show()

    
    # add a plot for showing max frequency in each voltage
    def plot_max_frequency(self):
        """
        Plots maximum frequency versus voltage.
        """
        voltages = sorted(self.sim_data.max_frequency.keys())
        frequencies = [self.sim_data.max_frequency[v] for v in voltages]

        # Create the figure and set the size
        plt.figure(figsize=(10, 7))

        # Plot the data with enhanced markers and line style
        plt.plot(voltages, frequencies, marker='o', linestyle='-', color='b', markersize=8, linewidth=2)
        # in the points I have value, show max frequency values explicitly
        for i, freq in enumerate(frequencies):
            plt.text(voltages[i], freq, f"{freq} MHz", fontsize=12, va='bottom', ha='center')
        

        # Add title and labels with increased font sizes
        plt.title('Maximum Frequency vs Voltage', fontsize=16, fontweight='bold')
        plt.xlabel('Voltage (V)', fontsize=14)
        plt.ylabel('Frequency (MHz)', fontsize=14)

        # Add grid for better visibility
        plt.grid(True, which='both', linestyle='--', linewidth=0.5)

        # Make ticks larger for better readability
        plt.xticks(fontsize=12)
        plt.yticks(fontsize=12)

        # Optional: Adding minor grid lines
        plt.minorticks_on()
        plt.grid(which='minor', linestyle=':', linewidth=0.5)

        # Save the plot as a PDF
        plt.savefig('max_frequency_vs_voltage.pdf', bbox_inches='tight')

    def plot_energy_vs_size(self, ra, ca, voltage, PEs=None):
        """
        Plots power versus matrix size (varying cb) for specified PEs at the given voltage and max frequency.

        Args:
            ra (int): Number of rows in matrix A.
            ca (int): Number of columns in matrix A.
            voltage (float): Voltage level.
            PEs (list, optional): List of PEs to include in the plot. Defaults to ['carus', 'caesar', 'cgra', 'cpu'].
        """
        max_freq = self.sim_data.max_frequency.get(voltage, None)
        if max_freq is None:
            print(f"No max frequency data for voltage {voltage}V.")
            return

        if PEs is None:
            PEs = ['carus', 'caesar', 'cgra', 'cpu']

        # Collect cb values for which data is available for the given ra, ca, and voltage
        cb_values = set()
        for data in self.sim_data.data_list:
            if data['row_a'] == ra and data['col_a'] == ca and data['voltage'] == voltage and data['PE'] in PEs:
                cb_values.add(data['col_b'])
        cb_values = sorted(cb_values)

        if not cb_values:
            print(f"No data available for ra={ra}, ca={ca}, voltage={voltage}V.")
            return

        energy_data = {PE: [] for PE in PEs}

        for cb in cb_values:
            for PE in PEs:
                result = self.sim_data.query_power(
                    PE=PE,
                    row_a=ra,
                    col_a=ca,
                    col_b=cb,
                    voltage=voltage,
                    frequency_MHz=max_freq
                )
                if result:
                    execution_time_s = result['execution_time_ns'] * 1e-9
                    power_W = result['power_mW'] * 1e-3
                    energy_J = execution_time_s * power_W
                    energy_data[PE].append(energy_J)
                else:
                    energy_data[PE].append(None)

        # Plotting
        plt.figure(figsize=(10, 7))

        for PE in PEs:
            # Filter out None values
            energy_values = energy_data[PE]
            if any(energy_values):
                plt.plot(cb_values, energy_values, marker='o', linestyle='-', markersize=8, linewidth=2, label=PE)

        pe_labels = ', '.join(PEs)
        plt.title(
            f'Energy vs Matrix Size (cb) at {voltage}V and max frequency\n'
            f'for {ra}x{ca} * {ca}x(cb)',
            fontsize=16, fontweight='bold'
        )
        plt.xlabel('cb', fontsize=14)
        plt.ylabel('Energy (J)', fontsize=14)
        plt.grid(True, which='both', linestyle='--', linewidth=0.5)
        plt.legend(fontsize=12)
        plt.xticks(cb_values, fontsize=12)  # Show only the cb_values we have data for
        plt.yticks(fontsize=12)
        plt.minorticks_on()
        plt.grid(which='minor', linestyle=':', linewidth=0.5)
        plt.savefig(f'energy_vs_size_ra{ra}xca{ca}_{voltage}V.pdf', bbox_inches='tight')
        # plt.show()
    
    def plot_metric_vs_size(self, sweep_params, fixed_params, voltage, metrics=['energy'], PEs=None, domains=None):
        """
        Plots specified metrics versus matrix sizes, sweeping over given parameters for specified PEs at given voltage.
        
        Args:
            sweep_params (list): List of parameters to sweep over, e.g., ['col_b'], ['row_a', 'col_a'].
            fixed_params (dict): Dictionary of fixed parameters, e.g., {'row_a': 8, 'col_a': 8}.
            voltage (float): Voltage level.
            metrics (list): List of metrics to plot. Options are 'power', 'time', 'energy'.
            PEs (list, optional): List of PEs to include in the plot. Defaults to ['carus', 'caesar', 'cgra', 'cpu'].
            domains (dict, optional): Dictionary mapping PEs to lists of domains to include in power calculation.
        """
        max_freq = self.sim_data.max_frequency.get(voltage, None)
        if max_freq is None:
            print(f"No max frequency data for voltage {voltage}V.")
            return

        if PEs is None:
            PEs = ['carus', 'caesar', 'cgra', 'cpu']

        # Collect all unique values for sweep parameters from data_list
        param_values = {param: set() for param in sweep_params}
        for data in self.sim_data.data_list:
            if data['voltage'] == voltage and data['PE'] in PEs:
                match = True
                for fixed_param, fixed_value in fixed_params.items():
                    if data.get(fixed_param) != fixed_value:
                        match = False
                        break
                if match:
                    for param in sweep_params:
                        param_values[param].add(data[param])

        # Check if we have data
        if not all(param_values.values()):
            print("No data available for the given parameters.")
            return

        # Sort the values
        for param in sweep_params:
            param_values[param] = sorted(param_values[param])

        # Create all combinations of sweep parameters
        sweep_param_combinations = list(product(*[param_values[param] for param in sweep_params]))

        # Prepare data structure for metrics
        metric_data = {metric: {PE: [] for PE in PEs} for metric in metrics}

        # Prepare x-axis labels
        x_labels = []
        x_values = []  # For numeric x-axis if needed

        for idx, combination in enumerate(sweep_param_combinations):
            params = dict(zip(sweep_params, combination))
            params.update(fixed_params)

            # Map parameter names to their short forms
            param_short_names = {
                'row_a': 'ra',
                'col_a': 'ca',
                'col_b': 'cb',
            }

            # Generate x-labels with short parameter names
            x_label = ','.join([f"{param_short_names.get(k, k)}={v}" for k, v in params.items() if k in sweep_params])
            x_labels.append(x_label)
            x_values.append(idx)

            for PE in PEs:
                # Get domains for this PE
                pe_domains = domains.get(PE) if domains else None
                result = self.sim_data.query_power(
                    PE=PE,
                    row_a=params.get('row_a'),
                    col_a=params.get('col_a'),
                    col_b=params.get('col_b'),
                    voltage=voltage,
                    frequency_MHz=max_freq,
                    power_type='total',
                    domains=pe_domains
                )
                if result:
                    execution_time_s = result['execution_time_ns'] * 1e-9
                    power_W = result['power_mW'] * 1e-3
                    energy_J = execution_time_s * power_W

                    for metric in metrics:
                        if metric == 'power':
                            metric_data[metric][PE].append(power_W)
                        elif metric == 'time':
                            metric_data[metric][PE].append(execution_time_s)
                        elif metric == 'energy':
                            metric_data[metric][PE].append(energy_J)
                else:
                    for metric in metrics:
                        metric_data[metric][PE].append(None)

        # Remove data points where all PEs have None (no data)
        valid_indices = []
        for idx in range(len(x_labels)):
            is_valid = False
            for metric in metrics:
                for PE in PEs:
                    if metric_data[metric][PE][idx] is not None:
                        is_valid = True
                        break
                if is_valid:
                    break
            if is_valid:
                valid_indices.append(idx)

        # Filter data to include only valid indices
        x_labels = [x_labels[i] for i in valid_indices]
        # x_labels = []
        # for idx, combination in enumerate(sweep_param_combinations):
        #     params = dict(zip(sweep_params, combination))
        #     params.update(fixed_params)
        #     # Use abbreviations for parameter names and combine with their values
        #     abbrev_map = {'row_a': 'ra', 'col_a': 'ca', 'col_b': 'cb'}
        #     x_label = ','.join([f"{abbrev_map.get(k, k)}={v}" for k, v in params.items() if k in sweep_params])
        #     x_labels.append(x_label)
        x_values = [x_values[i] for i in valid_indices]
        for metric in metrics:
            for PE in PEs:
                metric_data[metric][PE] = [metric_data[metric][PE][i] for i in valid_indices]

        # Plotting
        num_metrics = len(metrics)
        plt.figure(figsize=(8 * num_metrics, 6))

        for idx, metric in enumerate(metrics):
            plt.subplot(1, num_metrics, idx + 1)
            for PE in PEs:
                metric_values = metric_data[metric][PE]
                if any(metric_values):
                    plt.plot(x_values, metric_values, marker='o', linestyle='-', markersize=8, linewidth=2, label=PE)
            plt.title(f'{metric.capitalize()} vs Sizes at {voltage}V', fontsize=16, fontweight='bold')
            plt.xlabel('Sizes', fontsize=14)
            plt.ylabel(f'{metric.capitalize()}', fontsize=14)
            plt.xticks(x_values, x_labels, rotation=45, ha='right', fontsize=12)
            plt.yticks(fontsize=12)
            plt.grid(True, which='both', linestyle='--', linewidth=0.5)
            plt.minorticks_on()
            plt.grid(which='minor', linestyle=':', linewidth=0.5)
            plt.legend(fontsize=12)
            plt.tight_layout()

        # Save the plot as PDF
        # Generate a relevant filename
        sweep_params_str = '_'.join(sweep_params)
        metrics_str = '_'.join(metrics)
        PEs_str = '_'.join(PEs)
        filename = f'plot_{metrics_str}_vs_{sweep_params_str}_at_{voltage}V_PEs_{PEs_str}.pdf'
        plt.savefig(filename, bbox_inches='tight')
        print(f"Plot saved as {filename}")
        plt.close()
    
    # write another function to shows breakdown of energy consumed in each voltage on different domains
    def plot_power_breakdown(self, PE, voltage, row_a, col_a, col_b, metrics=['energy'], domains=None):
        """
        Plots the breakdown of power or energy consumption across different domains for the specified PE(s) at given voltage(s) and sizes.
        
        Args:
            PE (str or list): The processing element(s) to analyze.
            voltage (float or list): Voltage level(s) to consider.
            row_a (int): Number of rows in matrix A.
            col_a (int): Number of columns in matrix A.
            col_b (int): Number of columns in matrix B.
            metrics (list): List of metrics to plot. Options are 'power', 'energy'.
            domains (dict, optional): Dictionary mapping PEs to lists of domains to include in the breakdown.
                                    If not provided, uses the default domains for each PE.

            # Example: Plot energy breakdown for 'carus' and 'cgra' at 0.8V for sizes ra=8, ca=8, cb=256
            data_analysis.plot_power_breakdown(
                PE=['carus', 'cgra'],
                voltage=0.8,
                row_a=8,
                col_a=8,
                col_b=256,
                metrics=['energy']
            )
        """

        # Ensure PE and voltage are lists
        PEs = [PE] if isinstance(PE, str) else PE
        voltages = [voltage] if isinstance(voltage, (float, int)) else voltage

        # Prepare data structure for metrics
        breakdown_data = {pe: {v: {} for v in voltages} for pe in PEs}

        for pe in PEs:
            for v in voltages:
                max_freq = self.sim_data.max_frequency.get(v, None)
                if max_freq is None:
                    print(f"No max frequency data for voltage {v}V.")
                    continue

                # Use provided domains or default ones based on PE
                if domains and pe in domains:
                    pe_domains = domains[pe]
                else:
                    # Default domains based on PE
                    if pe == 'carus':
                        pe_domains = ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_carus']
                    elif pe == 'caesar':
                        pe_domains = ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_caesar']
                    elif pe == 'cgra':
                        pe_domains = ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_cgra']
                    elif pe == 'cpu':
                        pe_domains = ['pow_sys', 'pow_cpu', 'pow_mem']
                    else:
                        print(f"Unknown PE type: {pe}")
                        continue

                # Find data entries matching the PE, voltage, and sizes
                data_entries = [d for d in self.sim_data.data_list if d['PE'] == pe and d['voltage'] == v and d['row_a'] == row_a and d['col_a'] == col_a and d['col_b'] == col_b]
                if not data_entries:
                    print(f"No data available for PE={pe} at voltage={v}V with sizes ra={row_a}, ca={col_a}, cb={col_b}.")
                    continue

                # Use the first data entry
                data = data_entries[0]

                # Get the power or energy for each domain
                domain_values = {}
                scaling_factor = max_freq / data['clock_frequency_MHz']
                scaled_execution_time_ns = data['execution_time_ns'] / scaling_factor
                execution_time_s = scaled_execution_time_ns * 1e-9  # Convert ns to s

                total_power_mW = 0.0  # Total power for calculating percentages
                total_energy_J = 0.0  # Total energy for calculating percentages

                domain_powers = {}  # For storing domain powers
                domain_energies = {}  # For storing domain energies

                for domain in pe_domains:
                    dyn_key = domain + '_dyn'
                    static_key = domain + '_static'

                    total_dyn_power = data.get(dyn_key, 0.0)
                    total_static_power = data.get(static_key, 0.0)
                    # Scale dynamic power with frequency
                    scaled_dyn_power = total_dyn_power * scaling_factor
                    # Total power for the domain
                    domain_power_mW = scaled_dyn_power + total_static_power
                    domain_powers[domain] = domain_power_mW
                    total_power_mW += domain_power_mW

                    # Calculate energy in Joules
                    power_W = domain_power_mW * 1e-3  # Convert mW to W
                    energy_J = execution_time_s * power_W
                    domain_energies[domain] = energy_J
                    total_energy_J += energy_J

                # Store the breakdown data
                if 'power' in metrics:
                    breakdown_data[pe][v]['domain_values'] = domain_powers
                    breakdown_data[pe][v]['total_value'] = total_power_mW
                if 'energy' in metrics:
                    breakdown_data[pe][v]['domain_values'] = domain_energies
                    breakdown_data[pe][v]['total_value'] = total_energy_J

        # Plotting
        for metric in metrics:
            num_PEs = len(PEs)
            num_voltages = len(voltages)
            total_bars = num_PEs * num_voltages
            indices = np.arange(total_bars)
            bar_width = 0.5

            fig, ax = plt.subplots(figsize=(10, 7))

            # Initialize bottom positions for stacking
            bottoms = np.zeros(total_bars)
            domain_names = pe_domains  # Assuming same domains for all PEs

            # Stack the bars for each domain
            for domain in domain_names:
                values = []
                percentages = []  # For storing percentages
                for pe in PEs:
                    for v in voltages:
                        domain_value = breakdown_data[pe][v]['domain_values'].get(domain, 0.0)
                        total_value = breakdown_data[pe][v]['total_value']
                        values.append(domain_value)
                        if total_value > 0:
                            percent = (domain_value / total_value) * 100
                        else:
                            percent = 0.0
                        percentages.append(percent)

                # Plot the bar segment
                ax.bar(indices, values, bar_width, bottom=bottoms, label=domain)

                # Annotate percentages
                for idx, (value, bottom, percent) in enumerate(zip(values, bottoms, percentages)):
                    if value > 0:
                        ax.text(
                            idx, bottom + value / 2,
                            f"{percent:.1f}%",
                            ha='center', va='center', fontsize=10, color='white', fontweight='bold'
                        )

                # Update bottoms for next stack
                bottoms += np.array(values)

            # Labels and aesthetics
            ax.set_title(f'{metric.capitalize()} Breakdown by Domain\nSizes: ra={row_a}, ca={col_a}, cb={col_b}', fontsize=16, fontweight='bold')
            ax.set_xlabel('PE and Voltage', fontsize=14)
            ylabel_unit = 'J' if metric == 'energy' else 'mW'
            ax.set_ylabel(f'{metric.capitalize()} ({ylabel_unit})', fontsize=14)
            # Create x-tick labels
            x_labels = []
            for pe in PEs:
                for v in voltages:
                    x_labels.append(f'{pe}@{v}V')
            ax.set_xticks(indices)
            ax.set_xticklabels(x_labels, rotation=45, ha='right', fontsize=12)
            ax.yaxis.set_tick_params(labelsize=12)

            ax.grid(True, which='both', linestyle='--', linewidth=0.5)
            ax.legend(fontsize=12)
            plt.tight_layout()

            # Save the plot
            metric_str = '_'.join(metrics)
            PEs_str = '_'.join(PEs)
            volts_str = '_'.join([str(v) for v in voltages])
            filename = f'{metric}_breakdown_{PEs_str}_volts_{volts_str}_ra{row_a}_ca{col_a}_cb{col_b}.pdf'
            plt.savefig(filename, bbox_inches='tight')
            plt.close()



# Example usage:

# Create an instance of the data class
simulation_data = MatmulSimulationData()

# Extract data from files (this needs to be done once)
simulation_data.extract_data(root_dir='../private/matmul_postsynth_sims')

# Save data to a file for future use
simulation_data.save_data('simulation_data.pkl')

# Load data from a file (in future runs)
# simulation_data.load_data('simulation_data.pkl')

# Instantiate the data analysis class
data_analysis = MatmulDataAnalysis(simulation_data)

# data_analysis.plot_power_vs_voltage(PE='carus', row_a=8, col_a=8, col_b=256)
# data_analysis.plot_energy_vs_frequency(PE='carus', row_a=8, col_a=8, col_b=256, voltage=0.8)
# data_analysis.plot_max_frequency()

# data_analysis.plot_energy_vs_size(ra=4, ca=4, voltage=0.9, PEs=['carus', 'caesar','cgra'])
# data_analysis.plot_power_vs_size(ra=4, ca=4, voltage=0.9, PEs=['carus', 'caesar','cgra'])

# # Example 1: Sweep over 'col_b', fixed 'row_a' and 'col_a', plot 'energy' for specified PEs and domains
# data_analysis.plot_metric_vs_size(
#     sweep_params=['col_b'],
#     fixed_params={'row_a': 8, 'col_a': 8},
#     voltage=0.8,
#     metrics=['energy'],
#     PEs=['carus', 'cgra'],
#     domains={
#         'carus': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_carus'],
#         'cgra': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_cgra']
#     }
# )

# =================================================================================================

# Example: Sweep over 'col_b', fixed 'row_a' and 'col_a', plot 'energy' for specified PEs and domains
# data_analysis.plot_metric_vs_size(
#     sweep_params=['row_a', 'col_a', 'col_b'],
#     fixed_params={},
#     voltage=0.5,
#     metrics=['energy', 'time', 'power'],
#     PEs=['carus', 'caesar', 'cgra'],
#     domains={
#         'carus': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_carus'],
#         'caesar': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_caesar'],
#         'cgra': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_cgra']
#     }
# )

# data_analysis.plot_metric_vs_size(
#     sweep_params=['row_a', 'col_a', 'col_b'],
#     fixed_params={},
#     voltage=0.9,
#     metrics=['energy', 'time', 'power'],
#     PEs=['carus', 'caesar', 'cgra'],
#     domains={
#         'carus': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_carus'],
#         'caesar': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_caesar'],
#         'cgra': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_cgra']
#     }
# )
