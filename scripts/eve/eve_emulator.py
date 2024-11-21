import numpy as np
import pandas as pd
import itertools


class EVE:
    """
    Emulator for evaluating energy consumption and execution time of a workload
    using different policies.

    Example:
        eve = EVE(models=models, workload=workload, time_budget_s=1.0)
        eve.run(policy=optimized_energy_policy)
        result = eve.results['OptimizedEnergy']

        # ==== Having access to the results for one policy ====
        result = eve.results['OptimizedEnergy']

        if result['success']:
            print(f"Total Energy Consumption: {1000*result['total_energy_mJ']:.4f} uJ")
            print(f"Total Execution Time: {1000*result['total_time_s']:.4f} ms")
            print(f"Average Energy per Operation: {1000*result['average_energy_mJ']:.4f} uJ")
            print(f"Average Time per Operation: {1000*result['average_time_s']:.6f} ms")
            print(f"Average Power Consumption: {result['average_power_mW']:.2f} mW")
            # Detailed results per operation
            for idx, detail in enumerate(result['detailed_results']):
                op = detail['operation']
                print(f"Operation {idx + 1}:")
                print(f"  Size: {op['row_a']}x{op['col_a']} * {op['col_a']}x{op['col_b']}")
                print(f"  Selected PE: {detail['PE']}")
                print(f"  Voltage: {detail['voltage']}V")
                print(f"  Frequency: {detail['frequency_MHz']} MHz")
                print(f"  Energy: {1e6*detail['energy_mJ']:.4f} nJ")
                print(f"  Execution Time: {1e6*detail['execution_time_s']:.6f} us")
                print(f"  Average Power: {detail['average_power_mW']:.2f} mW")
                print()
        else:
            print("Policy was not successful.")
            print(result['message'])
        
        # ==== Getting results as a DataFrame ====
        # Get results as DataFrame
        results_df = eve.get_results_dataframe()
        print(results_df)

        # ==== Plotting results ====
        # Assuming results_df is obtained from get_results_dataframe()
        # Remove policies that did not succeed
        results_df = results_df.dropna(subset=['Total Energy (mJ)'])

    """
    def __init__(self, models, workload, time_budget_s, 
                 pe_memory_capacity_byte={'cpu': 256 * 1024,  'cgra': 256 * 1024, 'carus': 32 * 1024,  'caesar': 32 * 1024}):
        """
        Initializes the EVE emulator.

        Args:
            models (MatmulPowerModel): The power and performance models.
            workload (list): The workload to be processed.
            time_budget_s (float): Total time budget in seconds.

        Example:
            eve = EVE(models=models, workload=workload, time_budget_s=1.0)
        """
        self.models = models
        self.workload = workload
        self.time_budget_s = time_budget_s
        self.pe_memory_capacity_byte = pe_memory_capacity_byte
        self.results = {}

    def run(self, policy):
        """
        Runs the emulator with the specified policy.

        Args:
            policy (Policy): The policy to use for selecting configurations.

        Example:
            eve.run(policy=optimized_energy_policy)
        """
        result = policy.run(self.models, self.workload, self.time_budget_s, self.pe_memory_capacity_byte)
        self.results[policy.name] = result
        if result.get('success') == False:
            print(f"Policy {policy.name} could not meet the time budget.")
        

    def run_multiple(self, policies): 
        """
        Runs the emulator with multiple policies and collects results for comparison.

        Args:
            policies (list): List of Policy instances.

        Example:
            policies = [optimized_energy_policy, fixed_pe_policy]
            eve.run_multiple(policies)
        """
        for policy in policies:
            self.run(policy)

    def get_results_basic(self):
        """
        Returns the results as a pandas DataFrame for easy comparison.

        Returns:
            pandas.DataFrame: DataFrame containing results of all policies.

        Example:
            df = eve.get_results_basic()
        """
        results_list = []
        for policy_name, result in self.results.items():
            if result.get('success', False):
                results_list.append({
                    'Policy': policy_name,
                    'Total Energy (mJ)': result['total_energy_mJ'],
                    'Total Time (ms)': result['total_time_ms'],
                    'Average Energy per Operation (mJ)': result['average_energy_mJ'],
                    'Average Time per Operation (ms)': result['average_time_ms'],
                    'Average Power Consumption (mW)': result['average_power_mW']
                })
            else:
                results_list.append({
                    'Policy': policy_name,
                    'Total Energy (mJ)': None,
                    'Total Time (ms)': None,
                    'Average Energy per Operation (mJ)': None,
                    'Average Time per Operation (ms)': None,
                    'Average Power Consumption (mW)': None
                })
        df = pd.DataFrame(results_list)
        return df

    # now lets write a function that will return the results in a more detailed way
    def get_results(self):
        return self.results
        
