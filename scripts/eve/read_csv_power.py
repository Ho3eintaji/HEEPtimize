#!/usr/bin/env python3
import os
import csv
import json
import argparse

def parse_power_csv(csv_file_path):
    """
    Parse the CSV file at csv_file_path and return a dictionary of summed TOTAL_POWER
    for the required categories:

      - mem_instr: sum of ram0 + ram1
      - mem: sum of ram2 + ram3
      - mem_intrlv: sum of ram4 + ram5 + ram6 + ram7
      - cpu_wo_mem: u_core_v_mini_mcu/cpu_subsystem_i
      - carus: u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper
      - caesar: u_heepatia_peripherals/gen_caesar_0__u_nm_caesar_wrapper
      - cgra_wo_mem: u_heepatia_peripherals/u_cgra_top_wrapper
      - system: all other listed relevant blocks
    """

    # Categories we will store
    # Initialize sums to zero
    power_data = {
        "mem_instr": 0.0,
        "mem_doublebuffer": 0.0,
        "mem_intrlv": 0.0,
        "cpu_wo_mem": 0.0,
        "carus": 0.0,
        "caesar": 0.0,
        "cgra_wo_mem": 0.0,
        "system": 0.0,
        "total": 0.0
    }

    # Define sets of cells belonging to each category
    mem_instr_cells = {
        "u_core_v_mini_mcu/memory_subsystem_i/ram0_i",
        "u_core_v_mini_mcu/memory_subsystem_i/ram1_i"
    }
    mem_doublebuffer_cells = {
        "u_core_v_mini_mcu/memory_subsystem_i/ram2_i",
        "u_core_v_mini_mcu/memory_subsystem_i/ram3_i"
    }
    mem_intrlv_cells = {
        "u_core_v_mini_mcu/memory_subsystem_i/ram4_i",
        "u_core_v_mini_mcu/memory_subsystem_i/ram5_i",
        "u_core_v_mini_mcu/memory_subsystem_i/ram6_i",
        "u_core_v_mini_mcu/memory_subsystem_i/ram7_i"
    }
    cpu_wo_mem_cells = "u_core_v_mini_mcu/cpu_subsystem_i"
    carus_cells = "u_heepatia_peripherals/gen_carus_0__u_nm_carus_wrapper"
    caesar_cells= "u_heepatia_peripherals/gen_caesar_0__u_nm_caesar_wrapper"
    cgra_wo_mem_cells = "u_heepatia_peripherals/u_cgra_top_wrapper"

    # Cells that we consider as 'system' if they aren't in one of the categories above
    # (other relevant modules that we want grouped into 'system')
    system_cells = {
        "u_pad_control",
        "i_clk_int_div",
        "u_rstgen",
        "u_pad_ring",
        "u_heepatia_bus",
        "u_core_v_mini_mcu/peripheral_subsystem_i",
        "u_core_v_mini_mcu/system_bus_i",
        "u_core_v_mini_mcu/debug_subsystem_i",
        "u_core_v_mini_mcu/ao_peripheral_subsystem_i",
        "u_heepatia_peripherals/u_heepatia_ctrl_reg",
        "u_heepatia_peripherals/u_fll_subsystem"
    }

    # Read the CSV
    if not os.path.isfile(csv_file_path):
        return power_data  # If no file, return zeros

    with open(csv_file_path, 'r', newline='') as f:
        reader = csv.DictReader(f)
        for row in reader:
            cell = row['CELL'].strip()

            # Attempt to read total power from the row
            try:
                # You could either sum INTERNAL_POWER + SWITCHING_POWER + LEAKAGE_POWER
                # or read the provided TOTAL_POWER field directly if it's in the CSV.
                # For clarity, let's read TOTAL_POWER.
                total_power = float(row['TOTAL_POWER']) * 1e3
                is_in_list = 0
            except ValueError:
                # If the row isn't valid or missing data, skip it
                continue

            # Classify the cell into a category
            if cell in mem_instr_cells:
                power_data["mem_instr"] += total_power
                is_in_list = 1
            elif cell in mem_doublebuffer_cells:
                power_data["mem_doublebuffer"] += total_power
                is_in_list = 1
            elif cell in mem_intrlv_cells:
                power_data["mem_intrlv"] += total_power
                is_in_list = 1
            elif cell == cpu_wo_mem_cells:
                power_data["cpu_wo_mem"] += total_power
                is_in_list = 1
            elif cell == carus_cells:
                power_data["carus"] += total_power
                is_in_list = 1
            elif cell == caesar_cells:
                power_data["caesar"] += total_power
                is_in_list = 1
            elif cell == cgra_wo_mem_cells:
                power_data["cgra_wo_mem"] += total_power
                is_in_list = 1
            elif cell in system_cells:
                power_data["system"] += total_power
                is_in_list = 1
            
            # Always sum the total power
            if is_in_list == 1:
                power_data["total"] += total_power
            

    return power_data


def main():
    parser = argparse.ArgumentParser(description="Parse power CSVs and update JSON.")
    parser.add_argument("--directory", required=True, 
                        help="Path to directory containing the 4 voltage subdirectories.")
    parser.add_argument("--kernel", required=True, help="Kernel name.")
    parser.add_argument("--pe", required=True, help="PE name.")
    parser.add_argument("--json_file", default="transformer-power.json", 
                        help="Path to the JSON file to read/update.")

    args = parser.parse_args()

    # Example mapping from subdirectory name to voltage 
    # (adapt to match the actual subfolders you have)
    voltage_map = {
        "tt_0p50_25": "0.50V",
        "tt_0p65_25": "0.65V",
        "tt_0p80_25": "0.80V",
        "tt_0p90_25": "0.90V",
    }

    # Load existing JSON if present, otherwise create an empty dict
    if os.path.isfile(args.json_file):
        with open(args.json_file, 'r') as jf:
            big_data = json.load(jf)
    else:
        big_data = {}

    # Ensure the kernel entry exists
    if args.kernel not in big_data:
        big_data[args.kernel] = {}

    # Ensure the PE entry within the kernel exists
    if args.pe not in big_data[args.kernel]:
        big_data[args.kernel][args.pe] = {}

    # Loop through the subdirectories that represent different voltages
    # e.g. if your directory structure is something like:
    #
    #   <args.directory>/
    #       tt_0p50_25/power.csv
    #       tt_0p65_25/power.csv
    #       tt_0p80_25/power.csv
    #       tt_0p90_25/power.csv
    #
    # Then we iterate over the keys in voltage_map to find those subdirectories.
    #
    for subdir, voltage_str in voltage_map.items():
        path_to_csv = os.path.join(args.directory, subdir, "power.csv")

        # Parse the CSV to get the category sums
        power_sums = parse_power_csv(path_to_csv)

        # Store the results under the "voltage_str" key 
        # inside big_data[kernel][pe]
        big_data[args.kernel][args.pe][voltage_str] = power_sums

    # Write the updated big_data back to the JSON file
    with open(args.json_file, 'w') as jf:
        json.dump(big_data, jf, indent=2)

    print(f"Updated power data stored in {args.json_file}")


if __name__ == "__main__":
    main()
