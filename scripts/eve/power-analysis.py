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
import numpy as np
import argparse
import pandas as pd
from sklearn.linear_model import LinearRegression
from sklearn.metrics import r2_score
from sklearn.preprocessing import PolynomialFeatures
import glob


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

        Example:
        data.query_power(PE='carus', row_a=8, col_a=8, col_b=256, voltage=0.8, frequency_MHz=578, power_type='total')


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

    Example:

    power_model = MatmulPowerModel(
        sim_data=simulation_data,
        use_total_ops=False,
        degree_time=5,
        degree_dyn_power=5,
        degree_static_power=0,  # Static power modeled as a constant
        reference_frequency_MHz=100,
        output_dir=output_dir
    )
    """

    def __init__(self, simulation_data, output_dir):
        """
        Initializes the MatmulDataAnalysis object.

        Args:
            simulation_data (MatmulSimulationData): An instance of MatmulSimulationData.
        """
        self.sim_data = simulation_data
        self.output_dir = output_dir

        if not os.path.exists(self.output_dir):
            os.makedirs(self.output_dir)
        
    def plot_power_vs_voltage(self, PE, row_a, col_a, col_b):
        """
        Plots execution time and average power versus voltage at max frequency for the given PE and matrix size.

        Args:
            PE (str): The processing element.
            row_a (int): Number of rows in matrix A.
            col_a (int): Number of columns in matrix A.
            col_b (int): Number of columns in matrix B.

        Example:
        data_analysis.plot_power_vs_voltage(PE='carus', row_a=8, col_a=8, col_b=256)
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
        plt.savefig(f'{self.output_dir}/power_vs_voltage.pdf', bbox_inches='tight')

    def plot_energy_vs_frequency(self, PE, row_a, col_a, col_b, voltage):
        """
        Plots total energy versus frequency for the given PE, matrix size, and voltage.

        Args:
            PE (str): The processing element.
            row_a (int): Number of rows in matrix A.
            col_a (int): Number of columns in matrix A.
            col_b (int): Number of columns in matrix B.
            voltage (float): Voltage level.

        Example:
        data_analysis.plot_energy_vs_frequency(PE='carus', row_a=8, col_a=8, col_b=256, voltage=0.8)
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
        plt.savefig(f'{self.output_dir}/energy_vs_frequency.pdf', bbox_inches='tight')

    def plot_power_vs_size_1d(self, ra, ca, voltage, PEs=None, power_type='total'):
        """
        Plots power versus matrix size (varying cb) for specified PEs at the given voltage and max frequency.

        Args:
            ra (int): Number of rows in matrix A.
            ca (int): Number of columns in matrix A.
            voltage (float): Voltage level.
            PEs (list, optional): List of PEs to include in the plot. Defaults to ['carus', 'caesar', 'cgra', 'cpu'].
            power_type (str, optional): Type of power to plot ('total', 'dynamic', 'static'). Defaults to 'total'.

        Example:
        data_analysis.plot_power_vs_size_1d(ra=4, ca=4, voltage=0.9, PEs=['carus', 'caesar','cgra'], power_type='dynamic')
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
                    frequency_MHz=max_freq,
                    power_type=power_type
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
            cb_vals_filtered = [cb for cb, val in zip(cb_values, power_values) if val is not None]
            power_values_filtered = [val for val in power_values if val is not None]
            if power_values_filtered:
                plt.plot(cb_vals_filtered, power_values_filtered, marker='o', linestyle='-', markersize=8, linewidth=2, label=PE)

        power_label = power_type.capitalize() + ' Power (W)'
        plt.title(
            f'{power_type.capitalize()} Power vs Matrix Size (cb) at {voltage}V and Max Frequency\n'
            f'for {ra}x{ca} * {ca}x(cb)',
            fontsize=16, fontweight='bold'
        )
        plt.xlabel('cb', fontsize=14)
        plt.ylabel(power_label, fontsize=14)
        plt.grid(True, which='both', linestyle='--', linewidth=0.5)
        plt.legend(fontsize=12)
        plt.xticks(cb_values, fontsize=12)  # Show only the cb_values we have data for
        plt.yticks(fontsize=12)
        plt.minorticks_on()
        plt.grid(which='minor', linestyle=':', linewidth=0.5)
        filename = f'power_vs_size_ra{ra}xca{ca}_{voltage}V_{power_type}.pdf'
        plt.tight_layout()
        plt.savefig(f'{self.output_dir}/{filename}', bbox_inches='tight')
        plt.close()

    def plot_max_frequency(self):
        """
        Plots maximum frequency versus voltage.

        Example:
        data_analysis.plot_max_frequency()
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
        plt.savefig(f'{self.output_dir}/max_frequency_vs_voltage.pdf', bbox_inches='tight')

    def plot_energy_vs_size_1d(self, ra, ca, voltage, PEs=None):
        """
        Plots power versus matrix size (varying cb) for specified PEs at the given voltage and max frequency.

        Args:
            ra (int): Number of rows in matrix A.
            ca (int): Number of columns in matrix A.
            voltage (float): Voltage level.
            PEs (list, optional): List of PEs to include in the plot. Defaults to ['carus', 'caesar', 'cgra', 'cpu'].

        Example:
        data_analysis.plot_energy_vs_size_1d(ra=4, ca=4, voltage=0.9, PEs=['carus', 'caesar','cgra'])
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
        plt.savefig(f'{self.output_dir}/energy_vs_size_ra{ra}xca{ca}_{voltage}V.pdf', bbox_inches='tight')
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

        Example:

        # Example 1: Sweep over 'col_b', fixed 'row_a' and 'col_a', plot 'energy' for specified PEs and domains
        data_analysis.plot_metric_vs_size(
            sweep_params=['col_b'],
            fixed_params={'row_a': 8, 'col_a': 8},
            voltage=0.8,
            metrics=['energy'],
            PEs=['carus', 'cgra'],
            domains={
                'carus': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_carus'],
                'cgra': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_cgra']
            }
        )

        # Example: Sweep over 'col_b', fixed 'row_a' and 'col_a', plot 'energy' for specified PEs and domains
        data_analysis.plot_metric_vs_size(
            sweep_params=['row_a', 'col_a', 'col_b'],
            fixed_params={},
            voltage=0.5,
            metrics=['energy', 'time', 'power'],
            PEs=['carus', 'caesar', 'cgra'],
            domains={
                'carus': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_carus'],
                'caesar': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_caesar'],
                'cgra': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_cgra']
            }
        )

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
        filename = f'{self.output_dir}/plot_{metrics_str}_vs_{sweep_params_str}_at_{voltage}V_PEs_{PEs_str}.pdf'
        plt.savefig(filename, bbox_inches='tight')
        print(f"Plot saved as {filename}")
        plt.close()
    
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
            filename = f'{self.output_dir}/{metric}_breakdown_{PEs_str}_volts_{volts_str}_ra{row_a}_ca{col_a}_cb{col_b}.pdf'
            plt.savefig(filename, bbox_inches='tight')
            plt.close()
    
    def plot_improvement_over_reference(self, PEs, reference_PE, sweep_params, fixed_params, PE_voltages, PE_frequencies=None, reference_voltage=None, reference_frequency=None, metrics=['energy', 'time']):
        """
        Plots the improvement in energy and/or time of specified PEs over a reference PE across varying parameters.

        Args:
            PEs (list): List of PEs to compare.
            reference_PE (str): The reference PE to compare against.
            sweep_params (list): List of parameters to sweep over (e.g., ['col_b']).
            fixed_params (dict): Dictionary of fixed parameters (e.g., {'row_a': 8, 'col_a': 8}).
            PE_voltages (dict): Dictionary mapping PEs to their voltages (e.g., {'carus': 0.8, 'cgra': 0.8}).
            PE_frequencies (dict, optional): Dictionary mapping PEs to their frequencies. If not provided, uses max frequency at given voltage.
            reference_voltage (float): Voltage for the reference PE.
            reference_frequency (float, optional): Frequency for the reference PE. If not provided, uses max frequency at given voltage.
            metrics (list): List of metrics to plot. Options are 'energy', 'time'.

        Example:

        # Compare 'carus' and 'cgra' to 'cpu' over varying 'col_b' sizes
        data_analysis.plot_improvement_over_reference(
            PEs=['carus', 'cgra'],
            reference_PE='cpu',
            sweep_params=['col_b'],
            fixed_params={'row_a': 8, 'col_a': 8},
            PE_voltages={'carus': 0.8, 'cgra': 0.8},
            reference_voltage=0.8,
            metrics=['energy', 'time']
        )

        """
        max_frequency = self.sim_data.max_frequency

        if PE_frequencies is None:
            PE_frequencies = {}
        if reference_voltage is None:
            print("Reference voltage must be specified.")
            return

        if reference_frequency is None:
            reference_frequency = max_frequency.get(reference_voltage, None)
            if reference_frequency is None:
                print(f"No max frequency data for reference voltage {reference_voltage}V.")
                return

        # Collect all unique values for sweep parameters from data_list
        param_values = {param: set() for param in sweep_params}
        for data in self.sim_data.data_list:
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
        from itertools import product
        sweep_param_combinations = list(product(*[param_values[param] for param in sweep_params]))

        # Prepare data structure for metrics
        metric_ratios = {metric: {PE: [] for PE in PEs} for metric in metrics}

        # Prepare x-axis labels
        x_labels = []

        for idx, combination in enumerate(sweep_param_combinations):
            params = dict(zip(sweep_params, combination))
            params.update(fixed_params)
            # Generate x-labels with short parameter names
            param_short_names = {
                'row_a': 'ra',
                'col_a': 'ca',
                'col_b': 'cb',
            }
            x_label = ','.join([f"{param_short_names.get(k, k)}={v}" for k, v in params.items() if k in sweep_params])
            x_labels.append(x_label)

            # Get data for reference PE
            reference_result = self.sim_data.query_power(
                PE=reference_PE,
                row_a=params.get('row_a'),
                col_a=params.get('col_a'),
                col_b=params.get('col_b'),
                voltage=reference_voltage,
                frequency_MHz=reference_frequency
            )
            if not reference_result:
                # Skip this data point if reference data is missing
                for PE in PEs:
                    for metric in metrics:
                        metric_ratios[metric][PE].append(None)
                continue

            ref_execution_time_s = reference_result['execution_time_ns'] * 1e-9
            ref_power_W = reference_result['power_mW'] * 1e-3
            ref_energy_J = ref_execution_time_s * ref_power_W

            for PE in PEs:
                voltage = PE_voltages.get(PE)
                if voltage is None:
                    print(f"No voltage specified for PE {PE}.")
                    continue

                frequency = PE_frequencies.get(PE)
                if frequency is None:
                    frequency = max_frequency.get(voltage, None)
                    if frequency is None:
                        print(f"No max frequency data for voltage {voltage}V.")
                        continue

                result = self.sim_data.query_power(
                    PE=PE,
                    row_a=params.get('row_a'),
                    col_a=params.get('col_a'),
                    col_b=params.get('col_b'),
                    voltage=voltage,
                    frequency_MHz=frequency
                )
                if result:
                    execution_time_s = result['execution_time_ns'] * 1e-9
                    power_W = result['power_mW'] * 1e-3
                    energy_J = execution_time_s * power_W

                    for metric in metrics:
                        if metric == 'energy':
                            ratio = ref_energy_J / energy_J
                        elif metric == 'time':
                            ratio = ref_execution_time_s / execution_time_s
                        elif metric == 'power':
                            ratio = power_W / ref_power_W
                        else:
                            continue
                        metric_ratios[metric][PE].append(ratio)
                else:
                    for metric in metrics:
                        metric_ratios[metric][PE].append(None)

        # Plotting
        import matplotlib.pyplot as plt
        num_metrics = len(metrics)
        plt.figure(figsize=(8 * num_metrics, 6))

        average_gains = {metric: {} for metric in metrics}

        for idx, metric in enumerate(metrics):
            plt.subplot(1, num_metrics, idx + 1)
            for PE in PEs:
                ratios = metric_ratios[metric][PE]
                # Filter out None values
                valid_indices = [i for i, val in enumerate(ratios) if val is not None]
                valid_ratios = [ratios[i] for i in valid_indices]
                valid_x_labels = [x_labels[i] for i in valid_indices]

                if valid_ratios:
                    plt.plot(valid_x_labels, valid_ratios, marker='o', linestyle='-', markersize=8, linewidth=2, label=PE)
                    average_gain = sum(valid_ratios) / len(valid_ratios)
                    average_gains[metric][PE] = average_gain
                else:
                    average_gains[metric][PE] = None

            plt.title(f'{metric.capitalize()} Improvement over {reference_PE} at various sizes', fontsize=16, fontweight='bold')
            plt.xlabel('Sizes', fontsize=14)
            plt.ylabel(f'Improvement Ratio', fontsize=14)
            plt.xticks(rotation=45, ha='right', fontsize=12)
            plt.yticks(fontsize=12)
            plt.grid(True, which='both', linestyle='--', linewidth=0.5)
            plt.minorticks_on()
            plt.grid(which='minor', linestyle=':', linewidth=0.5)
            plt.legend(fontsize=12)
            plt.tight_layout()

        # Print average gains
        for metric in metrics:
            print(f"\nAverage {metric} improvement over {reference_PE}:")
            for PE in PEs:
                gain = average_gains[metric][PE]
                if gain is not None:
                    print(f"{PE}: {gain:.2f}x")
                else:
                    print(f"{PE}: No data")

        # Save the plot
        sweep_params_str = '_'.join(sweep_params)
        metrics_str = '_'.join(metrics)
        PEs_str = '_'.join(PEs)
        filename = f'{self.output_dir}/improvement_{metrics_str}_over_{reference_PE}_vs_{sweep_params_str}.pdf'
        plt.savefig(filename, bbox_inches='tight')
        plt.close()
    
    def analyze_dependency(self, PE, voltage, metric='execution_time_ns'):
        """
        Analyzes whether total operations is sufficient to explain the metric or whether ra, ca, cb separately have an effect.

        Args:
            PE (str): The processing element.
            voltage (float): Voltage level.
            metric (str): The metric to analyze ('execution_time_ns', 'dyn_power_mW', 'static_power_mW').

        Example:

        data_analysis = MatmulDataAnalysis(simulation_data)
        data_analysis.analyze_dependency(PE='carus', voltage=0.8, metric='execution_time_ns')
        """
        # Retrieve data for the given PE and voltage
        data_df = pd.DataFrame(self.sim_data.data_list)
        pe_data = data_df[(data_df['PE'] == PE) & (data_df['voltage'] == voltage)]

        # Compute total operations
        pe_data['total_ops'] = pe_data['row_a'] * pe_data['col_a'] * pe_data['col_b']

        # Get the metric values
        if metric == 'execution_time_ns':
            y = pe_data['execution_time_ns'].values
            y_label = 'Execution Time (ns)'
        elif metric == 'dyn_power_mW':
            dyn_power_keys = [key for key in pe_data.columns if key.endswith('_dyn')]
            y = pe_data[dyn_power_keys].sum(axis=1).values
            y_label = 'Dynamic Power (mW)'
        elif metric == 'static_power_mW':
            static_power_keys = [key for key in pe_data.columns if key.endswith('_static')]
            y = pe_data[static_power_keys].sum(axis=1).values
            y_label = 'Static Power (mW)'
        else:
            print(f"Unknown metric: {metric}")
            return

        # Analysis using total operations
        X_total_ops = pe_data[['total_ops']]
        model_total_ops = LinearRegression()
        model_total_ops.fit(X_total_ops, y)
        y_pred_total_ops = model_total_ops.predict(X_total_ops)
        r2_total_ops = model_total_ops.score(X_total_ops, y)

        # Analysis using ra, ca, cb separately
        X_sizes = pe_data[['row_a', 'col_a', 'col_b']]
        model_sizes = LinearRegression()
        model_sizes.fit(X_sizes, y)
        y_pred_sizes = model_sizes.predict(X_sizes)
        r2_sizes = model_sizes.score(X_sizes, y)

        print(f"R² using total_ops: {r2_total_ops:.4f}")
        print(f"R² using ra, ca, cb separately: {r2_sizes:.4f}")

        # Plotting
        plt.figure(figsize=(12, 5))

        plt.subplot(1, 2, 1)
        plt.scatter(pe_data['total_ops'], y, label='Measured')
        plt.plot(pe_data['total_ops'], y_pred_total_ops, 'r-', label='Predicted')
        plt.xlabel('Total Operations', fontsize=12)
        plt.ylabel(y_label, fontsize=12)
        plt.title(f'{metric.replace("_", " ").capitalize()} vs Total Operations', fontsize=14)
        plt.legend()
        plt.grid(True)

        plt.subplot(1, 2, 2)
        plt.scatter(y, y_pred_sizes, label='Predicted vs Measured')
        plt.plot([y.min(), y.max()], [y.min(), y.max()], 'r--', label='Ideal Fit')
        plt.xlabel('Measured ' + y_label, fontsize=12)
        plt.ylabel('Predicted ' + y_label, fontsize=12)
        plt.title(f'Prediction using ra, ca, cb separately', fontsize=14)
        plt.legend()
        plt.grid(True)

        plt.tight_layout()
        plt.show()

class MatmulPowerModel:
    """
    A class to build predictive models for execution time and power consumption
    based on simulation data collected at a reference frequency.
    """
    def __init__(self, sim_data, output_dir, use_total_ops=True,
                 degree_time=1, degree_dyn_power=1, degree_static_power=0,
                 reference_frequency_MHz=100):
        """
        Initializes the MatmulPowerModel with simulation data.

        Args:
            sim_data (MatmulSimulationData): The simulation data object.
            use_total_ops (bool): Whether to use total operations as input feature.
                If False, uses row_a, col_a, col_b separately.
            degree_time (int): Degree of the polynomial for execution time model.
            degree_dyn_power (int): Degree of the polynomial for dynamic power model.
            degree_static_power (int): Degree of the polynomial for static power model.
            reference_frequency_MHz (float): The reference frequency in MHz at which data is collected.
        """
        self.sim_data = sim_data
        self.models = {}  # Dictionary to store models for each PE
        self.use_total_ops = use_total_ops
        self.degree_time = degree_time
        self.degree_dyn_power = degree_dyn_power
        self.degree_static_power = degree_static_power
        self.reference_frequency_MHz = reference_frequency_MHz
        self.build_models()

        self.output_dir = output_dir
        if not os.path.exists(self.output_dir):
            os.makedirs(self.output_dir)

    def build_models(self):
        """
        Builds regression models for execution time and power for each PE.
        """
        # Convert data_list to a pandas DataFrame for easier manipulation
        data_df = pd.DataFrame(self.sim_data.data_list)

        # Filter data to only include the reference frequency
        data_df = data_df[data_df['clock_frequency_MHz'] == self.reference_frequency_MHz]
        if data_df.empty:
            print(f"No data found at reference frequency {self.reference_frequency_MHz} MHz.")
            return

        # List of PEs
        PEs = data_df['PE'].unique()

        for PE in PEs:
            pe_data = data_df[data_df['PE'] == PE]

            # Build models for each voltage separately
            voltages = pe_data['voltage'].unique()
            self.models[PE] = {}

            for voltage in voltages:
                voltage_data = pe_data[pe_data['voltage'] == voltage]

                # Features: Matrix sizes
                if self.use_total_ops:
                    # Compute total operations
                    total_ops = voltage_data['row_a'] * voltage_data['col_a'] * voltage_data['col_b']
                    X = pd.DataFrame({
                        'total_ops': total_ops
                    })
                else:
                    X = voltage_data[['row_a', 'col_a', 'col_b']]

                if X.empty:
                    print(f"No data for PE={PE}, voltage={voltage}V at reference frequency {self.reference_frequency_MHz} MHz.")
                    continue

                # Build execution time model
                poly_time = PolynomialFeatures(degree=self.degree_time, include_bias=False)
                X_time_poly = poly_time.fit_transform(X)
                y_time = voltage_data['execution_time_ns'].values

                time_model = LinearRegression()
                time_model.fit(X_time_poly, y_time)

                # Determine selected power domains for this PE
                selected_domains = self.get_default_domains_for_PE(PE)

                # Build dynamic power model
                poly_dyn_power = PolynomialFeatures(degree=self.degree_dyn_power, include_bias=False)
                X_dyn_power_poly = poly_dyn_power.fit_transform(X)

                # Sum dynamic power across selected domains
                dyn_power_keys = [domain + '_dyn' for domain in selected_domains]
                y_dyn_power = voltage_data[dyn_power_keys].sum(axis=1).values

                dyn_power_model = LinearRegression()
                dyn_power_model.fit(X_dyn_power_poly, y_dyn_power)

                # Build static power model
                static_power_keys = [domain + '_static' for domain in selected_domains]
                y_static_power = voltage_data[static_power_keys].sum(axis=1).values

                if self.degree_static_power == 0:
                    # Model static power as a constant (mean value)
                    mean_static_power = y_static_power.mean()
                    static_power_model = {'mean_static_power': mean_static_power}
                else:
                    poly_static_power = PolynomialFeatures(degree=self.degree_static_power, include_bias=False)
                    X_static_power_poly = poly_static_power.fit_transform(X)
                    static_power_model_reg = LinearRegression()
                    static_power_model_reg.fit(X_static_power_poly, y_static_power)
                    static_power_model = {
                        'model': static_power_model_reg,
                        'poly': poly_static_power
                    }

                # Store the models
                self.models[PE][voltage] = {
                    'reference_frequency': self.reference_frequency_MHz,
                    'time_model': time_model,
                    'poly_time': poly_time,
                    'dyn_power_model': dyn_power_model,
                    'poly_dyn_power': poly_dyn_power,
                    'static_power_model': static_power_model,
                    # Store selected domains for reference
                    'selected_domains': selected_domains,
                }
    
    def get_default_domains_for_PE(self, PE):
        """
        Returns the default power domains for a given PE.

        Args:
            PE (str): The processing element.

        Returns:
            list: List of default power domains for the PE.
        """
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
            selected_domains = []
        return selected_domains

    def predict(self, PE, row_a, col_a, col_b, voltage, frequency_MHz=None):
        """
        Predicts execution time and power consumption for the given inputs.

        Args:
            PE (str): The processing element.
            row_a (int): Number of rows in matrix A.
            col_a (int): Number of columns in matrix A.
            col_b (int): Number of columns in matrix B.
            voltage (float): Voltage level.
            frequency_MHz (float, optional): Frequency in MHz. If not provided, uses max frequency.

        Returns:
            dict: A dictionary containing predicted execution time and power.

        Example:

        # Predict power and timing for unknown inputs at a different frequency
        prediction = power_model.predict(
            PE='caesar',
            row_a=16,
            col_a=16,
            col_b=32,
            voltage=0.8,
            frequency_MHz=200  # Requesting a different frequency
        )

        if prediction:
            print("Prediction:")
            print(f"Execution Time: {prediction['execution_time_ns']:.2f} ns")
            print(f"Dynamic Power: {prediction['dyn_power_mW']:.2f} mW")
            print(f"Static Power: {prediction['static_power_mW']:.2f} mW")
            print(f"Total Power: {prediction['total_power_mW']:.2f} mW")
        """
        if PE not in self.models or voltage not in self.models[PE]:
            print(f"No model available for PE={PE} at voltage={voltage}V.")
            return None

        model_data = self.models[PE][voltage]
        reference_frequency = model_data['reference_frequency']
        time_model = model_data['time_model']
        poly_time = model_data['poly_time']
        dyn_power_model = model_data['dyn_power_model']
        poly_dyn_power = model_data['poly_dyn_power']
        static_power_model = model_data['static_power_model']

        # Prepare input features
        if self.use_total_ops:
            total_ops = row_a * col_a * col_b
            X_input = pd.DataFrame({
                'total_ops': [total_ops]
            })
        else:
            X_input = pd.DataFrame({
                'row_a': [row_a],
                'col_a': [col_a],
                'col_b': [col_b]
            })

        # Predict execution time at reference frequency
        X_time_poly = poly_time.transform(X_input)
        predicted_time_ns = time_model.predict(X_time_poly)[0]

        # Predict dynamic power at reference frequency
        X_dyn_power_poly = poly_dyn_power.transform(X_input)
        predicted_dyn_power_mW = dyn_power_model.predict(X_dyn_power_poly)[0]

        # Predict static power
        if self.degree_static_power == 0:
            # Use the mean static power
            predicted_static_power_mW = static_power_model['mean_static_power']
        else:
            X_static_power_poly = static_power_model['poly'].transform(X_input)
            predicted_static_power_mW = static_power_model['model'].predict(X_static_power_poly)[0]

        # If a different frequency is requested, adjust execution time and dynamic power
        if frequency_MHz is None:
            frequency_MHz = self.sim_data.max_frequency.get(voltage, None)
            if frequency_MHz is None:
                print(f"No max frequency data for voltage {voltage}V.")
                return None
        else:
            max_freq = self.sim_data.max_frequency.get(voltage, None)
            if max_freq is None:
                print(f"No max frequency data for voltage {voltage}V.")
                return None
            if frequency_MHz > max_freq:
                print(f"Frequency {frequency_MHz} MHz exceeds max frequency {max_freq} MHz at {voltage}V.")
                return None

        # Scaling factors
        freq_scaling = frequency_MHz / reference_frequency

        # Adjust execution time inversely with frequency
        adjusted_time_ns = predicted_time_ns * reference_frequency / frequency_MHz

        # Adjust dynamic power linearly with frequency
        adjusted_dyn_power_mW = predicted_dyn_power_mW * freq_scaling

        # Total power
        total_power_mW = adjusted_dyn_power_mW + predicted_static_power_mW

        result = {
            'PE': PE,
            'row_a': row_a,
            'col_a': col_a,
            'col_b': col_b,
            'voltage': voltage,
            'frequency_MHz': frequency_MHz,
            'execution_time_ns': adjusted_time_ns,
            'dyn_power_mW': adjusted_dyn_power_mW,
            'static_power_mW': predicted_static_power_mW,
            'total_power_mW': total_power_mW
        }

        return result

    def query_measured(self, PE, row_a, col_a, col_b, voltage, frequency_MHz=None):
        """
        Returns the actual measured execution time and power for the given inputs if available.

        Args:
            PE (str): The processing element.
            row_a (int): Number of rows in matrix A.
            col_a (int): Number of columns in matrix A.
            col_b (int): Number of columns in matrix B.
            voltage (float): Voltage level.
            frequency_MHz (float, optional): Frequency in MHz. If not provided, uses the data frequency.

        Returns:
            dict: A dictionary containing the measured execution time and power, or None if data not available.


        Example:

        # Assuming you have an instance of MatmulPowerModel
        measured_data = power_model.query_measured(
            PE='carus',
            row_a=8,
            col_a=8,
            col_b=512,
            voltage=0.8
        )

        if measured_data:
            print("Measured Data:")
            print(f"Execution Time: {measured_data['execution_time_ns']:.2f} ns")
            print(f"Dynamic Power: {measured_data['dyn_power_mW']:.2f} mW")
            print(f"Static Power: {measured_data['static_power_mW']:.2f} mW")
            print(f"Total Power: {measured_data['total_power_mW']:.2f} mW")
        """
        # Retrieve data for the given configuration
        data_df = pd.DataFrame(self.sim_data.data_list)
        filtered_data = data_df[
            (data_df['PE'] == PE) &
            (data_df['row_a'] == row_a) &
            (data_df['col_a'] == col_a) &
            (data_df['col_b'] == col_b) &
            (data_df['voltage'] == voltage)
        ]

        if filtered_data.empty:
            print("No measured data available for the specified configuration.")
            return None

        # If multiple entries exist, select the one with the closest frequency to the requested frequency
        if frequency_MHz is not None:
            filtered_data['freq_diff'] = abs(filtered_data['clock_frequency_MHz'] - frequency_MHz)
            data_row = filtered_data.sort_values('freq_diff').iloc[0]
        else:
            # Use the data with the reference frequency
            data_row = filtered_data.iloc[0]
            frequency_MHz = data_row['clock_frequency_MHz']

        data_frequency_MHz = data_row['clock_frequency_MHz']

        # Check max frequency constraint
        max_freq = self.sim_data.max_frequency.get(voltage, None)
        if max_freq is None:
            print(f"No max frequency data for voltage {voltage}V.")
            return None
        if frequency_MHz > max_freq:
            print(f"Frequency {frequency_MHz} MHz exceeds max frequency {max_freq} MHz at {voltage}V.")
            return None

        # Sum dynamic and static power over the selected domains
        selected_domains = self.get_default_domains_for_PE(PE)
        dyn_power_keys = [domain + '_dyn' for domain in selected_domains]
        static_power_keys = [domain + '_static' for domain in selected_domains]

        measured_dyn_power_mW = data_row[dyn_power_keys].sum()
        measured_static_power_mW = data_row[static_power_keys].sum()
        measured_time_ns = data_row['execution_time_ns']

        # If requested frequency is different from data frequency, adjust execution time and dynamic power
        if frequency_MHz != data_frequency_MHz:
            # Scaling factors
            freq_scaling = frequency_MHz / data_frequency_MHz

            # Adjust execution time inversely with frequency
            adjusted_time_ns = measured_time_ns * data_frequency_MHz / frequency_MHz

            # Adjust dynamic power linearly with frequency
            adjusted_dyn_power_mW = measured_dyn_power_mW * freq_scaling
        else:
            adjusted_time_ns = measured_time_ns
            adjusted_dyn_power_mW = measured_dyn_power_mW

        total_power_mW = adjusted_dyn_power_mW + measured_static_power_mW

        result = {
            'PE': PE,
            'row_a': row_a,
            'col_a': col_a,
            'col_b': col_b,
            'voltage': voltage,
            'frequency_MHz': frequency_MHz,
            'execution_time_ns': adjusted_time_ns,
            'dyn_power_mW': adjusted_dyn_power_mW,
            'static_power_mW': measured_static_power_mW,
            'total_power_mW': total_power_mW
        }

        return result

    def visualize_models(self, PE, voltage, fixed_params=None, varying_params=None, metrics=None):
        """
        Plots the model predictions against the measured data for validation.

        Args:
            PE (str): The processing element.
            voltage (float): Voltage level.
            fixed_params (dict, optional): Parameters to fix (e.g., {'row_a': 8}).
            varying_params (list, optional): Parameters to vary (e.g., ['col_b']).
            metrics (list, optional): Metrics to visualize ('execution_time_ns', 'dyn_power_mW', 'static_power_mW').
                                    If None, plots all three metrics.

        
        Example:

        power_model.visualize_models(
            PE='carus',
            voltage=0.8,
            fixed_params={'row_a': 8},
            varying_params=['col_b'],
            metrics=['execution_time_ns', 'dyn_power_mW', 'static_power_mW']
        )
        """
        if PE not in self.models or voltage not in self.models[PE]:
            print(f"No model available for PE={PE} at voltage={voltage}V.")
            return

        model_data = self.models[PE][voltage]
        reference_frequency = model_data['reference_frequency']
        time_model = model_data['time_model']
        poly_time = model_data['poly_time']
        dyn_power_model = model_data['dyn_power_model']
        poly_dyn_power = model_data['poly_dyn_power']
        static_power_model = model_data['static_power_model']
        selected_domains = model_data['selected_domains']

        # Retrieve data for the given PE and voltage at reference frequency
        data_df = pd.DataFrame(self.sim_data.data_list)
        pe_data = data_df[
            (data_df['PE'] == PE) &
            (data_df['voltage'] == voltage) &
            (data_df['clock_frequency_MHz'] == reference_frequency)
        ]

        if fixed_params:
            for param, value in fixed_params.items():
                pe_data = pe_data[pe_data[param] == value]
                if pe_data.empty:
                    print(f"No data available for {param}={value}.")
                    return

        if pe_data.empty:
            print(f"No data available for visualization for PE={PE} at voltage={voltage}V.")
            return

        # Prepare input features
        if self.use_total_ops:
            pe_data['total_ops'] = pe_data['row_a'] * pe_data['col_a'] * pe_data['col_b']
            X = pe_data[['total_ops']]
        else:
            X = pe_data[['row_a', 'col_a', 'col_b']]

        # Determine varying parameters
        if varying_params is None:
            # If no varying parameters are specified, use all dimensions not fixed
            all_params = ['row_a', 'col_a', 'col_b']
            if self.use_total_ops:
                varying_params = ['total_ops']
            else:
                varying_params = [param for param in all_params if param not in (fixed_params or {})]

        # Get metrics to plot
        if metrics is None:
            metrics = ['execution_time_ns', 'dyn_power_mW', 'static_power_mW']

        num_metrics = len(metrics)
        fig, axes = plt.subplots(1, num_metrics, figsize=(6 * num_metrics, 6))

        if num_metrics == 1:
            axes = [axes]

        for idx, metric in enumerate(metrics):
            ax = axes[idx]

            # Get measured values and predictions
            if metric == 'execution_time_ns':
                y_measured = pe_data['execution_time_ns'].values
                y_label = 'Execution Time (ns)'
                X_poly = poly_time.transform(X)
                y_predicted = time_model.predict(X_poly)
            elif metric == 'dyn_power_mW':
                dyn_power_keys = [domain + '_dyn' for domain in selected_domains]
                y_measured = pe_data[dyn_power_keys].sum(axis=1).values
                y_label = 'Dynamic Power (mW)'
                X_poly = poly_dyn_power.transform(X)
                y_predicted = dyn_power_model.predict(X_poly)
            elif metric == 'static_power_mW':
                static_power_keys = [domain + '_static' for domain in selected_domains]
                y_measured = pe_data[static_power_keys].sum(axis=1).values
                y_label = 'Static Power (mW)'
                if self.degree_static_power == 0:
                    y_predicted = np.full_like(y_measured, static_power_model['mean_static_power'])
                else:
                    X_poly = static_power_model['poly'].transform(X)
                    y_predicted = static_power_model['model'].predict(X_poly)
            else:
                print(f"Unknown metric: {metric}")
                continue

            num_varying = len(varying_params)
            if num_varying == 1:
                var_param = varying_params[0]
                x_values = pe_data[var_param].values
                ax.scatter(x_values, y_measured, label='Measured', marker='o')
                ax.plot(x_values, y_predicted, label='Predicted', linestyle='--')
                ax.set_title(f'{metric.replace("_", " ").capitalize()} vs {var_param}', fontsize=14)
                ax.set_xlabel(var_param, fontsize=12)
                ax.set_ylabel(y_label, fontsize=12)
                ax.legend(fontsize=10)
                ax.grid(True, which='both', linestyle='--', linewidth=0.5)
            elif num_varying == 2:
                var_param1, var_param2 = varying_params
                x_values = pe_data[var_param1].values
                y_values = pe_data[var_param2].values
                z_measured = y_measured
                z_predicted = y_predicted

                from mpl_toolkits.mplot3d import Axes3D
                ax = fig.add_subplot(1, num_metrics, idx + 1, projection='3d')
                ax.scatter(x_values, y_values, z_measured, label='Measured', marker='o')
                ax.scatter(x_values, y_values, z_predicted, label='Predicted', marker='^')
                ax.set_title(f'{metric.replace("_", " ").capitalize()} vs {var_param1} and {var_param2}', fontsize=14)
                ax.set_xlabel(var_param1, fontsize=12)
                ax.set_ylabel(var_param2, fontsize=12)
                ax.set_zlabel(y_label, fontsize=12)
                ax.legend()
            else:
                # For more than 2 varying parameters, plot predicted vs measured
                ax.scatter(y_measured, y_predicted, label='Predicted vs Measured', marker='o')
                # Plot y=x line
                min_val = min(y_measured.min(), y_predicted.min())
                max_val = max(y_measured.max(), y_predicted.max())
                ax.plot([min_val, max_val], [min_val, max_val], 'r--', label='Ideal Fit')
                ax.set_title(f'{metric.replace("_", " ").capitalize()} Model Validation', fontsize=14)
                ax.set_xlabel('Measured ' + y_label, fontsize=12)
                ax.set_ylabel('Predicted ' + y_label, fontsize=12)
                ax.legend(fontsize=10)
                ax.grid(True, which='both', linestyle='--', linewidth=0.5)

        plt.suptitle(f'Model Validation for {PE} at {voltage}V', fontsize=16)
        plt.tight_layout(rect=[0, 0, 1, 0.95])
        plt.savefig(f'{self.output_dir}/{PE}_{voltage}V_model_validation.pdf', bbox_inches='tight')
        plt.show()

    def evaluate_model(self, PEs=None, voltages=None):
        """
        Evaluates the models by calculating R² scores and Mean Squared Error (MSE)
        for specified PEs and voltages.

        Args:
            PEs (list, optional): List of PEs to evaluate. If None, evaluates all available PEs.
            voltages (list, optional): List of voltages to evaluate. If None, evaluates all available voltages.

        Example:

        # Evaluate models for specific PEs and voltages
        power_model.evaluate_model(
            PEs=['carus', 'cgra'],
            voltages=[0.8, 0.9]
        )

        # Evaluate models for all available PEs and voltages
        power_model.evaluate_model()
        """
        from sklearn.metrics import r2_score, mean_squared_error

        # Prepare data
        data_df = pd.DataFrame(self.sim_data.data_list)
        if data_df.empty:
            print("No data available for evaluation.")
            return

        if PEs is None:
            PEs = data_df['PE'].unique()

        if voltages is None:
            voltages = data_df['voltage'].unique()

        # Initialize dictionaries to store aggregated results
        aggregated_results = {
            'execution_time_ns': {'R2': [], 'MSE': []},
            'dyn_power_mW': {'R2': [], 'MSE': []},
            'static_power_mW': {'R2': [], 'MSE': []}
        }

        for PE in PEs:
            for voltage in voltages:
                if PE not in self.models or voltage not in self.models[PE]:
                    print(f"No model available for PE={PE} at voltage={voltage}V.")
                    continue

                model_data = self.models[PE][voltage]
                reference_frequency = model_data['reference_frequency']
                time_model = model_data['time_model']
                poly_time = model_data['poly_time']
                dyn_power_model = model_data['dyn_power_model']
                poly_dyn_power = model_data['poly_dyn_power']
                static_power_model = model_data['static_power_model']
                selected_domains = model_data['selected_domains']

                # Retrieve data for the given PE and voltage at reference frequency
                pe_data = data_df[
                    (data_df['PE'] == PE) &
                    (data_df['voltage'] == voltage) &
                    (data_df['clock_frequency_MHz'] == reference_frequency)
                ]

                if pe_data.empty:
                    print(f"No data available for evaluation for PE={PE} at voltage={voltage}V.")
                    continue

                # Prepare input features
                if self.use_total_ops:
                    pe_data['total_ops'] = pe_data['row_a'] * pe_data['col_a'] * pe_data['col_b']
                    X = pe_data[['total_ops']]
                else:
                    X = pe_data[['row_a', 'col_a', 'col_b']]

                evaluation_results = {}

                # Execution Time Evaluation
                y_measured_time = pe_data['execution_time_ns'].values
                X_time_poly = poly_time.transform(X)
                y_predicted_time = time_model.predict(X_time_poly)
                r2_time = r2_score(y_measured_time, y_predicted_time)
                mse_time = mean_squared_error(y_measured_time, y_predicted_time)
                evaluation_results['execution_time_ns'] = {'R2': r2_time, 'MSE': mse_time}
                aggregated_results['execution_time_ns']['R2'].append(r2_time)
                aggregated_results['execution_time_ns']['MSE'].append(mse_time)

                # Dynamic Power Evaluation
                dyn_power_keys = [domain + '_dyn' for domain in selected_domains]
                y_measured_dyn_power = pe_data[dyn_power_keys].sum(axis=1).values
                X_dyn_power_poly = poly_dyn_power.transform(X)
                y_predicted_dyn_power = dyn_power_model.predict(X_dyn_power_poly)
                r2_dyn_power = r2_score(y_measured_dyn_power, y_predicted_dyn_power)
                mse_dyn_power = mean_squared_error(y_measured_dyn_power, y_predicted_dyn_power)
                evaluation_results['dyn_power_mW'] = {'R2': r2_dyn_power, 'MSE': mse_dyn_power}
                aggregated_results['dyn_power_mW']['R2'].append(r2_dyn_power)
                aggregated_results['dyn_power_mW']['MSE'].append(mse_dyn_power)

                # Static Power Evaluation
                static_power_keys = [domain + '_static' for domain in selected_domains]
                y_measured_static_power = pe_data[static_power_keys].sum(axis=1).values
                if self.degree_static_power == 0:
                    y_predicted_static_power = np.full_like(y_measured_static_power, static_power_model['mean_static_power'])
                else:
                    X_static_power_poly = static_power_model['poly'].transform(X)
                    y_predicted_static_power = static_power_model['model'].predict(X_static_power_poly)
                r2_static_power = r2_score(y_measured_static_power, y_predicted_static_power)
                mse_static_power = mean_squared_error(y_measured_static_power, y_predicted_static_power)
                evaluation_results['static_power_mW'] = {'R2': r2_static_power, 'MSE': mse_static_power}
                aggregated_results['static_power_mW']['R2'].append(r2_static_power)
                aggregated_results['static_power_mW']['MSE'].append(mse_static_power)

                # Print the evaluation results for this PE and voltage
                print(f"Model Evaluation for PE={PE} at voltage={voltage}V:")
                for metric, results in evaluation_results.items():
                    print(f"  {metric}:")
                    print(f"    R² Score: {results['R2']:.4f}")
                    print(f"    Mean Squared Error: {results['MSE']:.4f}")
                print()

        # Calculate and print aggregated results
        print("Aggregated Model Evaluation Across All PEs and Voltages:")
        for metric, results in aggregated_results.items():
            if results['R2']:
                avg_r2 = np.mean(results['R2'])
                avg_mse = np.mean(results['MSE'])
                print(f"  {metric}:")
                print(f"    Average R² Score: {avg_r2:.4f}")
                print(f"    Average Mean Squared Error: {avg_mse:.4f}")
        print()

class EVE:
    def __init__(self, models, workload, time_budget_s):
        self.models = models  # MatmulPowerModel instance
        self.workload = workload  # List of matmul operations
        self.time_budget_s = time_budget_s  # Total time budget in seconds
        self.results = {}  # Store results for different policies

    def run(self, policy):
        """
        Simulates the execution of the workload under the specified policy.

        Args:
            policy (Policy): Policy instance implementing the selection logic.
        """
        self.results[policy.name] = self._simulate(policy)

    def _simulate(self, policy):
        """
        Simulates the workload under a single policy.

        Args:
            policy (Policy): The policy to apply.

        Returns:
            dict: Aggregated metrics for the policy.
        """
        total_energy = 0.0
        total_time = 0.0
        detailed_results = []

        # Use the policy to select configurations for all operations
        selections = policy.select_configurations(self.workload, self.time_budget_s)

        for operation, selection in zip(self.workload, selections):
            if selection is None:
                print(f"No valid configuration for operation {operation}")
                continue

            prediction = selection['prediction']
            selected_PE = selection['PE']
            energy = prediction['total_power_mW'] * (prediction['execution_time_ns'] * 1e-9)  # Energy in mJ
            total_energy += energy
            total_time += prediction['execution_time_ns'] * 1e-9  # Time in seconds

            detailed_results.append({
                'operation': operation,
                'PE': selected_PE,
                'energy_mJ': energy,
                'execution_time_s': prediction['execution_time_ns'] * 1e-9,
                'power_mW': prediction['total_power_mW'],
                'voltage': prediction['voltage'],
                'frequency_MHz': prediction['frequency_MHz']
            })

        return {
            'total_energy_mJ': total_energy,
            'total_time_s': total_time,
            'average_energy_mJ': total_energy / len(self.workload),
            'average_time_s': total_time / len(self.workload),
            'detailed_results': detailed_results
        }

    def compare_policies(self):
        """
        Compares the results of different policies and prints the comparison.
        """
        policy_names = list(self.results.keys())
        if len(policy_names) < 2:
            print("Need at least two policies to compare.")
            return

        base_policy = policy_names[0]
        for other_policy in policy_names[1:]:
            energy_base = self.results[base_policy]['total_energy_mJ']
            energy_other = self.results[other_policy]['total_energy_mJ'] 
            time_base = self.results[base_policy]['total_time_s']
            time_other = self.results[other_policy]['total_time_s']

            energy_savings = (1 - energy_base / energy_other) * 100
            time_difference = time_base - time_other

            print(f"Comparing {base_policy} to {other_policy}:")
            print(f"  Energy Savings: {energy_savings:.2f}%")
            print(f"  Time Difference: {time_difference:.4f} s")


class Policy:
    def __init__(self, name):
        self.name = name

    def select_configurations(self, workload, time_budget_s):
        raise NotImplementedError("This method should be overridden by subclasses.")

class OptimizedEnergyPolicy(Policy):
    def __init__(self, models, available_PEs, voltages, frequencies):
        super().__init__(name="OptimizedEnergy")
        self.models = models
        self.available_PEs = available_PEs
        self.voltages = voltages
        self.frequencies = frequencies

    def select_configurations(self, workload, time_budget_s):
        """
        Selects configurations for all operations to minimize total energy
        while meeting the time budget.

        Args:
            workload (list): List of operations.
            time_budget_s (float): Total time budget in seconds.

        Returns:
            list: Selected configurations for each operation.
        """
        # For each operation, generate possible configurations
        operation_configs = []
        for operation in workload:
            configs = self._generate_configs(operation)
            if not configs:
                operation_configs.append([])
            else:
                # Sort configurations by energy consumption
                configs.sort(key=lambda x: x['energy_mJ'])
                operation_configs.append(configs)

        # Initial selection: lowest energy configurations
        selections = [configs[0] if configs else None for configs in operation_configs]

        total_time = sum(sel['prediction']['execution_time_ns'] * 1e-9 for sel in selections if sel)
        if total_time <= time_budget_s:
            return selections  # Time budget met

        # If time budget not met, adjust configurations
        # Implement a simple heuristic to meet the time budget

        # Create a list of possible selections with indices
        possible_selections = []
        for idx, configs in enumerate(operation_configs):
            if len(configs) > 1:
                # Exclude the current selection
                for conf in configs[1:]:
                    possible_selections.append({
                        'operation_idx': idx,
                        'config': conf,
                        'delta_energy': conf['energy_mJ'] - selections[idx]['energy_mJ'],
                        'delta_time': selections[idx]['prediction']['execution_time_ns'] * 1e-9 - conf['prediction']['execution_time_ns'] * 1e-9
                    })

        # Sort possible selections by efficiency of time reduction per additional energy
        possible_selections.sort(key=lambda x: x['delta_time'] / x['delta_energy'] if x['delta_energy'] > 0 else float('inf'), reverse=True)

        # Iteratively select faster configurations until time budget is met
        for sel in possible_selections:
            idx = sel['operation_idx']
            selections[idx] = sel['config']
            total_time = sum(sel['prediction']['execution_time_ns'] * 1e-9 for sel in selections if sel)
            if total_time <= time_budget_s:
                break

        # Final check
        if total_time > time_budget_s:
            print("Unable to meet time budget with available configurations.")
        return selections

    def _generate_configs(self, operation):
        """
        Generates possible configurations for an operation.

        Args:
            operation (dict): The operation details.

        Returns:
            list: Configurations with predictions.
        """
        configs = []
        for PE in self.available_PEs:
            for voltage in self.voltages:
                for frequency in self.frequencies:
                    prediction = self.models.predict(
                        PE=PE,
                        row_a=operation['row_a'],
                        col_a=operation['col_a'],
                        col_b=operation['col_b'],
                        voltage=voltage,
                        frequency_MHz=frequency
                    )
                    if prediction is None:
                        continue
                    energy_mJ = prediction['total_power_mW'] * (prediction['execution_time_ns'] * 1e-9)
                    configs.append({
                        'PE': PE,
                        'voltage': voltage,
                        'frequency_MHz': frequency,
                        'prediction': prediction,
                        'energy_mJ': energy_mJ
                    })
        return configs





class BaselinePolicy(Policy):
    def __init__(self, models):
        super().__init__(name="Baseline")
        self.models = models

    def select_configurations(self, workload, time_budget_s):
        """
        Uses a fixed configuration for all operations (e.g., CPU at max frequency).
        """
        selections = []
        for operation in workload:
            prediction = self.models.predict(
                PE='cpu',
                row_a=operation['row_a'],
                col_a=operation['col_a'],
                col_b=operation['col_b'],
                voltage=0.9,
                frequency_MHz=None  # Use max frequency
            )
            if prediction is None:
                selections.append(None)
            else:
                energy_mJ = prediction['total_power_mW'] * (prediction['execution_time_ns'] * 1e-9)
                selections.append({
                    'PE': 'cpu',
                    'voltage': 0.9,
                    'frequency_MHz': prediction['frequency_MHz'],
                    'prediction': prediction,
                    'energy_mJ': energy_mJ
                })
        return selections


def generate_workload(num_operations):
    import random
    workload = []
    for _ in range(num_operations):
        ra = random.choice([4, 8, 16, 32, 64])
        ca = random.choice([4, 8, 16, 32, 64])
        cb = random.choice([4, 8, 16, 32, 64])
        operation = {
            'row_a': ra,
            'col_a': ca,
            'col_b': cb,
            # 'voltage': 0.8,  # Optional
            # 'frequency_MHz': 200  # Optional
        }
        workload.append(operation)
    return workload

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Matmul Simulation Data Analysis')
    parser.add_argument('--data_dir', type=str, default='private/matmul_postsynth_sims', help='Directory of simulation data')
    parser.add_argument('--root_dir', type=str, default='.', help='Root directory of project')
    parser.add_argument('--eve_dir', type=str, default='scripts/eve', help='EVE directory (where this script is located)')
    args = parser.parse_args()

    output_dir = args.eve_dir + '/output'

    # clear pdfs inside output_dir
    pdf_files = glob.glob(f'{output_dir}/*.pdf')
    for file in pdf_files:
        os.remove(file)

    simulation_data = MatmulSimulationData()
    # simulation_data.extract_data(root_dir=args.data_dir)
    # simulation_data.save_data(filename=f'{output_dir}/matmul_simulation_data_DataMv.pkl')
    simulation_data.load_data(filename=f'{output_dir}/matmul_simulation_data_noDataMv.pkl')

    # data_analysis = MatmulDataAnalysis(simulation_data, output_dir=output_dir)

    # Voltage = 0.9
    # data_analysis.plot_improvement_over_reference(
    #     PEs=['carus', 'caesar', 'cgra'],
    #     reference_PE='cpu',
    #     sweep_params=['row_a', 'col_a','col_b'],
    #     fixed_params={},
    #     PE_voltages={'carus': Voltage, 'caesar': Voltage, 'cgra': Voltage},
    #     reference_voltage=Voltage,
    #     metrics=['energy', 'time', 'power']
    # )

    # data_analysis.plot_metric_vs_size(
        #     sweep_params=['row_a', 'col_a', 'col_b'],
        #     fixed_params={},
        #     voltage=Voltage,
        #     metrics=['energy', 'time', 'power'],
        #     PEs=['carus', 'caesar', 'cgra'],
        #     domains={
        #         'carus': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_carus'],
        #         'caesar': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_caesar'],
        #         'cgra': ['pow_sys', 'pow_cpu', 'pow_mem', 'pow_cgra']
        #     }
        # )
    
    # Create the power model using total operations and custom degrees for each model
    models = MatmulPowerModel(
        sim_data=simulation_data,
        use_total_ops=False,
        degree_time=3,
        degree_dyn_power=4,
        degree_static_power=0,  # Static power modeled as a constant
        reference_frequency_MHz=100,
        output_dir=output_dir
    )

    # Assuming you have an instance of MatmulPowerModel named 'models'

    # Generate the workload
    workload = generate_workload(num_operations=100)

    # Define available PEs, voltages, and frequencies
    available_PEs = ['carus', 'caesar', 'cgra', 'cpu']
    voltages = [0.8, 0.9]  # Voltages for which models are available
    frequencies = [100, 200, 300]  # Frequencies within the max frequency constraints

    # Define the time budget in seconds
    time_budget_s = 1.0  # For example, total execution time should not exceed 1 second

    # Instantiate the policy
    optimized_energy_policy = OptimizedEnergyPolicy(
        models=models,
        available_PEs=available_PEs,
        voltages=voltages,
        frequencies=frequencies
    )

    # Create the EVE emulator
    eve = EVE(models=models, workload=workload, time_budget_s=time_budget_s)

    # Run the emulator with the optimized energy policy
    eve.run(policy=optimized_energy_policy)

    # Access and print the results
    result = eve.results['OptimizedEnergy']
    print(f"Total Energy Consumption: {result['total_energy_mJ']:.2f} mJ")
    print(f"Total Execution Time: {result['total_time_s']:.4f} s")
    print(f"Average Energy per Operation: {result['average_energy_mJ']:.2f} mJ")
    print(f"Average Time per Operation: {result['average_time_s']:.6f} s")

    # Detailed results per operation
    for idx, detail in enumerate(result['detailed_results']):
        op = detail['operation']
        print(f"Operation {idx + 1}:")
        print(f"  Size: {op['row_a']}x{op['col_a']} * {op['col_a']}x{op['col_b']}")
        print(f"  Selected PE: {detail['PE']}")
        print(f"  Voltage: {detail['voltage']}V")
        print(f"  Frequency: {detail['frequency_MHz']} MHz")
        print(f"  Energy: {detail['energy_mJ']:.4f} mJ")
        print(f"  Execution Time: {detail['execution_time_s']:.6f} s")
        print()





