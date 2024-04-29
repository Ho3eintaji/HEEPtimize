# Copyright 2023 EPFL and Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: test_scheduler.py
# Author: Michele Caon
# Date: 02/11/2023
# Description: Test scheduler for throughput and power analysis

import argparse
import subprocess
import sys
import os
import re
import csv
import pandas as pd

# Kernel test class
class app_test:
    """Test class."""
    data_types = ["int32", "int16", "int8"]

    # Initialize test properties
    def __init__(self, name: str, data_type: str, num_outs: int, kernel_params: str = ""):
        self.data: dict = {}
        self.data["app_name"] = name
        if not data_type in self.data_types:
            raise ValueError("Invalid data type")
        self.data["data_type"] = data_type
        self.data["num_outs"] = num_outs
        s = name.split("-", 1)
        self.data["memory_type"] = s[0]
        self.data["kernel_name"] = s[1]
        self.data["kernel_params"] = kernel_params

    # Append additional data
    def append_data(self, data: dict):
        """Append additional data."""
        self.data.update(data)

class test_scheduler:
    """Test scheduler class."""

    # Initialize the configuration file
    def __init__(self, config_file: str):
        self.config_file = config_file
        self.tests: app_test = []
        self.report_file: str = None

    # Configuration file parser
    def config_parser(self, pwr: bool = False) -> int:
        """Parse the configuration file."""
        
        # Load test lines
        with open(self.config_file, "r") as cf:
            tests = cf.readlines()

        # Discard empty or commented lines
        tests = [t.strip() for t in tests if t.strip() and not t.startswith("#")]

        # Parse tests
        for t in tests:
            # Get the app anme, data type, and number of output samples from the first three parameters
            if pwr:
                app_name, data_type = t.split(" ")[:2]
                num_outs = 0
                # All the remaining parameters are kernel parameters
                kernel_params = " ".join(t.split(" ")[2:])
            else:
                app_name, data_type, num_outs = t.split(" ")[:3]
                num_outs = int(num_outs)
                kernel_params = " ".join(t.split(" ")[3:])

            # Create test
            self.tests.append(app_test(app_name, data_type, num_outs, kernel_params))

        # Return the number of tests
        return len(self.tests)

    # Print tests
    def print_tests(self):
        """Print the tests."""
        for t in self.tests:
            print(f"Test name: {t.data['app_name']}")
            print(f"- data type: {t.data['data_type']}")
            print(f"- kernel parameters: {t.data['kernel_params']}")
            print()


    # Build test application
    def build_test(self, test: app_test, cdefs: str = ""):
        """Build the test application."""
        try:
            subprocess.run(["make", "app", f"PROJECT={test.data['app_name']}", f"KERNEL_PARAMS={test.data['data_type']} {test.data['kernel_params']}", f"CDEFS={cdefs}"], check=True, capture_output=True)
        except subprocess.CalledProcessError as e:
            print(f"\n### ERROR: failed to build '{test.data['app_name']}'", file=sys.stderr)
            print(e.stderr.decode("utf-8"), file=sys.stderr)
            sys.exit(1)


    # Run RTL simulation with verilator and capture stdout
    def run_verilator(self, test: app_test) -> str:
        try:
            sim_out = subprocess.run(["make", "verilator-opt", "MAX_CYCLES=2000000"], check=True, capture_output=True)
        except subprocess.CalledProcessError as e:
            print(f"\n### ERROR: failed to simulate '{test.data['app_name']}'", file=sys.stderr)
            print(e.stderr.decode("utf-8"), file=sys.stderr)
            sys.exit(1)
        return sim_out.stdout.decode("utf-8")

    # Run postlayout simulation using Questasim
    def run_postlayout(self, test: app_test, sim_args: str = ""):
        arg_list = [arg for arg in sim_args.split(" ") if arg]
        try:
            subprocess.run(["make", "questasim-postlayout-run"] + arg_list, check=True)
        except subprocess.CalledProcessError as e:
            print(f"\n### ERROR: failed to simulate '{test.data['app_name']}'", file=sys.stderr)
            print(e.stderr.decode("utf-8"), file=sys.stderr)
            sys.exit(1)

    # Perform power analysis using PrimePower
    def run_primepower(self, vcd_file: str):
        try:
            subprocess.run(["make", "power-analysis", f"PWR_VCD={os.path.abspath(vcd_file)}"], check=True)
        except subprocess.CalledProcessError as e:
            print("\n### ERROR: failed to run power analysis", file=sys.stderr)
            print(e.stderr.decode("utf-8"), file=sys.stderr)
            sys.exit(1)

    # Run RTL simulations on Verilator and extract throughput metrics
    def run_throughput(self, report_dir: str, report_name: str):
        """Run throughput simulations with Verilator."""
        # Parse the configuration file
        print(f"### Parsing configuration file '{self.config_file}'...")
        num_tests = self.config_parser()
        print(f"- {num_tests} tests found")

        # Initialize report file
        report_file = os.path.join(report_dir, report_name)
        self.init_throughput_report(report_file)

        # Run tests
        curr_test = 0
        for t in self.tests:
            curr_test += 1
            print(f"### [{curr_test}/{num_tests}] - {t.data['app_name']}")
            print(f"    - data type: {t.data['data_type']}")
            print(f"    - kernel parameters: {t.data['kernel_params']}")
            
            # Build test application
            print(f"  # Building test {t.data['app_name']}...")
            self.build_test(t)

            # Simulate the application with Verilator
            print(f"  # Simulating test {t.data['app_name']} with Verilator...")
            sim_out = self.run_verilator(t)

            # Parse the simulation output
            print("  # Parsing simulation output...")
            thr_data = self.parse_sim_output(sim_out)

            # Generate throughput report entry
            rpt: dict = {
                "kernel_name": t.data['kernel_name'],
                "data_type": t.data['data_type'],
                "num_outs": t.data['num_outs'],
                "kernel_params": t.data['kernel_params']
            }
            print(f" ## {t.data['app_name']} - {t.data['data_type']} simulation results:")
            if 'carus-' in t.data['app_name']:
                # Add CPU cycles to the report
                rpt['memory_type'] = "cpu"
                rpt['cycles'] = thr_data['cpu_cycles']
                print(f"    - CPU cycles: {thr_data['cpu_cycles']}")
                self.add_throughput_report(rpt)

                # Add NM-Carus cycles to the report
                rpt['memory_type'] = "carus"
                rpt['cycles'] = thr_data['carus_cycles']
                print(f"    - NM-Carus cycles: {thr_data['carus_cycles']}")
                self.add_throughput_report(rpt)

            else:
                print(f"ERROR: unsupported test name '{t.data['app_name']}'", file=sys.stderr)
                sys.exit(1)


    # Run power simulation (Questasim + PrimePower) and extract power metrics
    def run_power(self, log_dir: str, report_dir: str, report_name: str):
        """Run power simulations with Questasim and PrimePower."""

        report_file = os.path.join(report_dir, report_name)
        self.init_power_report(report_file)

        # Parse the configuration file
        print(f"### Parsing configuration file '{self.config_file}'...")
        num_tests = self.config_parser(pwr=True)
        print(f"- {num_tests} tests found")

        # Run tests
        curr_test = 0
        for t in self.tests:
            curr_test += 1
            cdefs = "POWER_SIM"
            print(f"### [{curr_test}/{num_tests}] - {t.data['app_name']}")
            print(f"    - data type: {t.data['data_type']}")
            print(f"    - kernel parameters: {t.data['kernel_params']}")
            print(f"    - CDEFS: {cdefs}")

            # Build test application
            print(f"  # Building test {t.data['app_name']}...")
            self.build_test(t, cdefs)

            # Run post-layout simulation
            print(f"  # Simulating test {t.data['app_name']} with Questasim...")
            self.run_postlayout(t, "VCD_MODE=2")
            
            # Prepare test output directory
            out_dir = os.path.join(report_dir, t.data['app_name'])
            os.makedirs(out_dir, exist_ok=True)

            # Initialize power
            pwr_data: dict
            
            # Initialize report
            rpt: dict = {
                "kernel_name": t.data['kernel_name'],
                "data_type": t.data['data_type'],
                "num_outs": t.data['num_outs'],
                "kernel_params": t.data['kernel_params']
            }

            if "carus-" in t.data['app_name']:
                # Run power analysis on NM-Carus VCD file
                vcd_file = os.path.join(log_dir, "waves-0.vcd")
                print(f"  # Running NM-Carus power analysis on {vcd_file}...")
                self.run_primepower(vcd_file)
                pwr_csv = os.path.join("implementation", "power_analysis", "reports", "power.csv")
                pwr_csv_new = os.path.join(out_dir, f"power-carus-{t.data['data_type']}.csv")
                os.rename(pwr_csv, pwr_csv_new)
                pwr_data = self.get_power("carus", pwr_csv_new)

                # Add test results to the report
                rpt['memory_type'] = "carus"
                rpt.update(pwr_data)
                self.add_power_report(rpt)
                
                # Report power consumption
                print(f"    - NM-Carus power: {(pwr_data['sys_pwr'] + pwr_data['nmc_pwr'])*1000:.4} mW")

                # Run power analysis on CPU VCD file
                vcd_file = os.path.join(log_dir, "waves-1.vcd")
                print(f"  # Running CPU power analysis on {vcd_file}...")
                self.run_primepower(vcd_file)
                pwr_csv = os.path.join("implementation", "power_analysis", "reports", "power.csv")
                pwr_csv_new = os.path.join(out_dir, f"power-cpu-{t.data['data_type']}.csv")
                os.rename(pwr_csv, pwr_csv_new)
                pwr_data = self.get_power("cpu", pwr_csv_new)

                # Add test results to the report
                rpt['memory_type'] = "cpu"
                rpt.update(pwr_data)
                self.add_power_report(rpt)

                # Report power consumption
                print(f"    - CPU power: {(pwr_data['sys_pwr'])*1000:.4} mW")

            else:
                print(f"ERROR: unsupported test name '{t.data['app_name']}'", file=sys.stderr)
                sys.exit(1)


    def get_power(self, mode: str, csv_file: str) -> dict:
        """Analyse power report."""
        # Cehck for valid mode
        if not mode in ["cpu", "carus"]:
            print(f"ERROR: invalid mode '{mode}'", file=sys.stderr)
            sys.exit(1)

        # Load CSV file
        power_data = pd.read_csv(csv_file, index_col=0, header=0)
        nmc_pwr_rows = []
        nmc_ctl_rows = []
        nmc_comp_rows = []
        nmc_mem_rows = []
        sys_pwr_rows = []
        sys_cpu_rows = []
        sys_mem_rows = []
        sys_peri_rows = []
        if mode == "cpu":
            # Add CPU contributions from the CSV file
            sys_pwr_rows = [
                "u_core_v_mini_mcu/cpu_subsystem_i",                            # system CPU
                "u_core_v_mini_mcu/memory_subsystem_i",                         # all SRAM banks
                "u_core_v_mini_mcu/system_bus_i",                               # system bus
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/boot_rom_i",       # Boot ROM
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/soc_ctrl_i",       # SoC control registers
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/power_manager_i",  # Power manager
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/fast_intr_ctrl_i", # fast interrupt controller
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/dma_i",            # DMA
                "u_core_v_mini_mcu/peripheral_subsystem_i/rv_plic_i",           # PLIC
            ]
            sys_cpu_rows = [
                "u_core_v_mini_mcu/cpu_subsystem_i"
            ]
            sys_mem_rows = [
                "u_core_v_mini_mcu/memory_subsystem_i"
            ]
            sys_peri_rows = [
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/fast_intr_ctrl_i",
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/dma_i",
                "u_core_v_mini_mcu/peripheral_subsystem_i/rv_plic_i",
            ]
        elif mode == "carus":
            # Add NM-Carus contributions from the CSV file
            sys_pwr_rows = [
                "u_core_v_mini_mcu/cpu_subsystem_i",                            # system CPU (idle)
                "u_core_v_mini_mcu/system_bus_i",                               # system bus
                "heeperator_bus",                                               # external bus
                "u_core_v_mini_mcu/memory_subsystem_i/gen_sram_0__ram_i",       # SRAM bank 0 (.text)
                "u_core_v_mini_mcu/memory_subsystem_i/gen_sram_1__ram_i",       # SRAM bank 1 (.data)
                "u_core_v_mini_mcu/memory_subsystem_i/gen_sram_2__ram_i",       # SRAM interleaved bank 0
                "u_core_v_mini_mcu/memory_subsystem_i/gen_sram_3__ram_i",       # SRAM interleaved bank 1
                "u_core_v_mini_mcu/memory_subsystem_i/gen_sram_4__ram_i",       # SRAM interleaved bank 2
                # "u_core_v_mini_mcu/memory_subsystem_i/gen_sram_5__ram_i",     # Replaced by NM-Carus
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/boot_rom_i",       # Boot ROM
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/soc_ctrl_i",       # SoC controller
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/power_manager_i",  # Power manager
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/fast_intr_ctrl_i", # fast interrupt controller (DMA)
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/dma_i",            # DMA
                "u_core_v_mini_mcu/peripheral_subsystem_i/rv_plic_i",           # PLIC
            ]
            sys_cpu_rows = [
                "u_core_v_mini_mcu/cpu_subsystem_i"
            ]
            sys_mem_rows = [
                "u_core_v_mini_mcu/memory_subsystem_i/gen_sram_0__ram_i",       # SRAM bank 0 (.text)
                "u_core_v_mini_mcu/memory_subsystem_i/gen_sram_1__ram_i",       # SRAM bank 1 (.data)
                "u_core_v_mini_mcu/memory_subsystem_i/gen_sram_2__ram_i",       # SRAM interleaved bank 0
                "u_core_v_mini_mcu/memory_subsystem_i/gen_sram_3__ram_i",       # SRAM interleaved bank 1
                "u_core_v_mini_mcu/memory_subsystem_i/gen_sram_4__ram_i",       # SRAM interleaved bank 2
                # "u_core_v_mini_mcu/memory_subsystem_i/gen_sram_5__ram_i",     # Replaced by NM-Carus
            ]
            sys_peri_rows = [
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/fast_intr_ctrl_i",
                "u_core_v_mini_mcu/ao_peripheral_subsystem_i/dma_i",
                "u_core_v_mini_mcu/peripheral_subsystem_i/rv_plic_i",
            ]
            nmc_pwr_rows = [
                "u_heeperator_peripherals/gen_carus_0__u_nm_carus_wrapper",     # NM-Carus
            ]
            nmc_ctl_rows = ["carus_ctl"]
            nmc_comp_rows = ["carus_vector"]
            nmc_mem_rows = ["carus_vrf"]

        # Compute power consumption
        sys_pwr: float = 0.0
        sys_cpu_pwr: float = 0.0
        sys_mem_pwr: float = 0.0
        sys_peri_pwr: float = 0.0
        nmc_pwr: float = 0.0
        nmc_ctl_pwr: float = 0.0
        nmc_comp_pwr: float = 0.0
        nmc_mem_pwr: float = 0.0

        # Compute system power
        for row in sys_pwr_rows:
            sys_pwr += power_data.loc[row, "TOTAL_POWER"]
        for row in sys_cpu_rows:
            sys_cpu_pwr += power_data.loc[row, "TOTAL_POWER"]
        for row in sys_mem_rows:
            sys_mem_pwr += power_data.loc[row, "TOTAL_POWER"]
        for row in sys_peri_rows:
            sys_peri_pwr += power_data.loc[row, "TOTAL_POWER"]

        # Compute NM-Carus power
        for row in nmc_pwr_rows:
            nmc_pwr += power_data.loc[row, "TOTAL_POWER"]
        for row in nmc_ctl_rows:
            nmc_ctl_pwr += power_data.loc[row, "TOTAL_POWER"]
        for row in nmc_comp_rows:
            nmc_comp_pwr += power_data.loc[row, "TOTAL_POWER"]
        for row in nmc_mem_rows:
            nmc_mem_pwr += power_data.loc[row, "TOTAL_POWER"]
        
        # Define dictionary with power data
        pwr_data: dict = {
            "sys_pwr": sys_pwr,
            "sys_cpu_pwr": sys_cpu_pwr,
            "sys_mem_pwr": sys_mem_pwr,
            "sys_peri_pwr": sys_peri_pwr,
            "nmc_pwr": nmc_pwr,
            "nmc_ctl_pwr": nmc_ctl_pwr,
            "nmc_comp_pwr": nmc_comp_pwr,
            "nmc_mem_pwr": nmc_mem_pwr
        }
        return pwr_data

    # Parse Verilator simulation output   
    def parse_sim_output(self, sim_out: str) -> (int, int, int):
        """Parse the simulation output."""
        # Look for NM-Carus number of cycles
        carus_regex = re.compile(r"- NM-Carus kernel execution time: (\d+) cycles\n")
        cpu_regex = re.compile(r"CPU: (\d+)\n")
        carus_cycles = carus_regex.search(sim_out)
        cpu_cycles = cpu_regex.search(sim_out)
        carus_cycles = int(carus_cycles.group(1))
        if cpu_cycles is None:
            cpu_cycles = 0
        else:
            cpu_cycles = int(cpu_cycles.group(1))
        thr_data: dict = {
            "carus_cycles": carus_cycles,
            "cpu_cycles": cpu_cycles
        }
        return thr_data

    # Initialize throughput CSV report
    def init_throughput_report(self, report_file: str):
        """Initialize the timing report."""
        with open(report_file, "w", encoding='utf8') as rf:
            self.report_file = report_file
            writer = csv.writer(rf)
            writer.writerow([
                "memory_type",
                "kernel_name",
                "data_type",
                "num_outs",
                "cycles",
                "kernel_params"
            ])

    # Add entry to throughput CSV report
    def add_throughput_report(self, data: dict):
        """Add a test report."""
        with open(self.report_file, "a", encoding='utf8') as rf:
            writer = csv.writer(rf)
            writer.writerow([
                data['memory_type'],
                data['kernel_name'],
                data['data_type'],
                data['num_outs'],
                data['cycles'],
                data['kernel_params']
            ])

    # Initialize power CSV report
    def init_power_report(self, report_file: str):
        """Initialize the power report."""
        with open(report_file, "w", encoding='utf8') as rf:
            self.report_file = report_file
            writer = csv.writer(rf)
            writer.writerow([
                "memory_type",
                "kernel_name",
                "data_type",
                "num_outs",
                "sys_pwr",
                "sys_cpu_pwr",
                "sys_mem_pwr",
                "sys_peri_pwr",
                "nmc_pwr",
                "nmc_ctl_pwr",
                "nmc_comp_pwr",
                "nmc_mem_pwr",
                "kernel_params"
            ])

    # Add entry to power CSV report
    def add_power_report(self, data: dict):
        """Add a test report."""
        with open(self.report_file, "a", encoding='utf8') as rf:
            writer = csv.writer(rf)
            writer.writerow([
                data['memory_type'],
                data['kernel_name'],
                data['data_type'],
                data['num_outs'],
                data['sys_pwr'],
                data['sys_cpu_pwr'],
                data['sys_mem_pwr'],
                data['sys_peri_pwr'],
                data['nmc_pwr'],
                data['nmc_ctl_pwr'],
                data['nmc_comp_pwr'],
                data['nmc_mem_pwr'],
                data['kernel_params']
            ])

if __name__ == "__main__":
    # Command line arguments
    cmd_parser = argparse.ArgumentParser(description="Test scheduler")
    cmd_parser.add_argument("cfg", 
                            help="Configuration file")
    cmd_parser.add_argument("out_dir", 
                            help="Output directory",
                            nargs="?",
                            default=os.getcwd())
    
    # Parse command line arguments
    args = cmd_parser.parse_args()

    # Define output file path
    thr_report_file = os.path.join(args.out_dir, "throughput.csv")

    ts = test_scheduler(args.cfg)
    ts.run_timing(thr_report_file)

    sys.exit(0)
