import os
import re
import csv
import pickle

class MatmulSimulationData:
    def __init__(self):
        # Data storage: a list of dictionaries
        self.data_list = []

    def extract_data(self, root_dir='private/matmul_postsynth_sims'):
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
        with open(filename, 'wb') as f:
            pickle.dump(self.data_list, f)
        print(f"Data saved to {filename}")

    def load_data(self, filename='simulation_data.pkl'):
        with open(filename, 'rb') as f:
            self.data_list = pickle.load(f)
        print(f"Data loaded from {filename}")

    def query_power(self, PE, row_a, col_a, col_b, voltage, frequency_MHz=None, power_type='total', domains=None):
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

# Example usage:

# Create an instance of the data class
simulation_data = MatmulSimulationData()

# Extract data from files (this needs to be done once)
simulation_data.extract_data(root_dir='../private/matmul_postsynth_sims')

# Save data to a file for future use
simulation_data.save_data('simulation_data.pkl')

# Load data from a file (in future runs)
# simulation_data.load_data('simulation_data.pkl')

# # Query total power consumption (default behavior)
# simulation_data.print_power_report(
#     PE='carus',
#     row_a=8,
#     col_a=8,
#     col_b=256,
#     voltage=0.5,
#     frequency_MHz=200  # New frequency, for scaling
# )

# # Query dynamic power only
# simulation_data.print_power_report(
#     PE='carus',
#     row_a=8,
#     col_a=8,
#     col_b=256,
#     voltage=0.5,
#     frequency_MHz=200,
#     power_type='dynamic'
# )

# # Query static power only
# simulation_data.print_power_report(
#     PE='carus',
#     row_a=8,
#     col_a=8,
#     col_b=256,
#     voltage=0.5,
#     frequency_MHz=200,
#     power_type='static'
# )

# # Query power for a specific domain (e.g., 'pow_mem')
# simulation_data.print_power_report(
#     PE='carus',
#     row_a=8,
#     col_a=8,
#     col_b=256,
#     voltage=0.5,
#     frequency_MHz=200,
#     domains=['pow_mem']
# )

# # Query dynamic power for a specific domain
# simulation_data.print_power_report(
#     PE='carus',
#     row_a=8,
#     col_a=8,
#     col_b=256,
#     voltage=0.5,
#     frequency_MHz=200,
#     power_type='dynamic',
#     domains=['pow_mem']
# )

# simulation_data.print_power_report(
#     PE='carus',
#     row_a=8,
#     col_a=8,
#     col_b=256,
#     voltage=0.5,
#     frequency_MHz=200
# )

simulation_data.print_power_report(
    PE='carus',
    row_a=8,
    col_a=8,
    col_b=256,
    voltage=0.5,
    frequency_MHz=100,
    power_type='static',
    domains=['pow_carus']
)
