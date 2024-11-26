import numpy as np
from pulp import LpProblem, LpMinimize, LpVariable, lpSum, LpBinary, PULP_CBC_CMD, LpStatus
import itertools
from tilings import generate_tiling_sequence, generate_tiling_sequence_multiple_pes
import copy

class Policy:
    def __init__(self, name):
        self.name = name

    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        """
        Run the emulator based on the policy's logic.

        Args:
            workload (list): The list of operations to be run.
            time_budget_s (float): The time budget in seconds.
            pe_memory_capacity_byte (dict): The memory limit for each PE.

        Returns:
            dict: A dictionary containing results (success, energy, timing, etc.)
        """
        raise NotImplementedError("This method should be overridden by subclasses.")
    
    def _run_selected_cfg(self, selections, workload):
        """
        A typical implementation of the run method for a policy that selects configurations and then runs them.

        Args:
            selections (list): The selected configurations for each operation.
            workload (list): The list of operations to be run.

        Returns:
            dict: A dictionary containing results (success, energy, timing, etc.)
        """

        if selections is None:
            results = {
                'success': False,
                'message': 'Time budget could not be met with available configurations.'
            }
            return results
        
        total_energy_mJ = 0
        total_time_ms = 0
        detailed_results = []
        for idx, (operation, selection) in enumerate(zip(workload, selections)):
            if selection is None:
                print(f"Operation {idx + 1} could not be configured.")
                continue
            # prediction = selection['prediction']
            energy_mJ = selection['total_energy_nJ'] * 1e-6  # Convert nJ to mJ
            execution_time_ms = selection['execution_time_ns'] * 1e-6  # Convert ns to ms
            total_energy_mJ += energy_mJ
            total_time_ms += execution_time_ms
            detailed_results.append({
                'operation': operation,
                'PEs': selection['PEs'],
                'voltages': selection['voltage'],
                'frequency_MHz': selection['frequency_MHz'],
                'energy_mJ': energy_mJ,
                'execution_time_ms': execution_time_ms
                # Add other details as needed

            })

        average_energy_mJ = total_energy_mJ / len(workload)
        average_time_ms = total_time_ms / len(workload)
        average_power_mW = (total_energy_mJ * 1e3) / total_time_ms if total_time_ms > 0 else 0  # mW

        results = {
            'success': True,
            'total_energy_mJ': total_energy_mJ,
            'total_time_ms': total_time_ms,
            'average_energy_mJ': average_energy_mJ,
            'average_time_ms': average_time_ms,
            'average_power_mW': average_power_mW,
            'detailed_results': detailed_results
        }
        return results

class GreedyEnergyPolicy(Policy):
    """
    Policy that selects the most energy-efficient configurations for each operation
    while ensuring that the total execution time does not exceed the time budget.
    This policy uses the maximum frequency supported at each voltage.

    Example:
        # Instantiate the policy
        optimized_energy_policy = GreedyEnergyPolicy(
            models=models,
            available_PEs=['carus', 'caesar', 'cgra', 'cpu'],
            voltages=[0.8, 0.9],  # Supported voltages
        )
        # Run the emulator with the policy
        eve.run(policy=optimized_energy_policy)
    """

    def __init__(self, available_PEs, voltages):
        """
        Initializes the GreedyEnergyPolicy.

        Args:
            models (MatmulPowerModel): The power and performance models.
            available_PEs (list): List of available Processing Elements (PEs).
            voltages (list): List of voltages to consider.

        Example:
            optimized_energy_policy = GreedyEnergyPolicy(
                models=models,
                available_PEs=['carus', 'caesar', 'cgra', 'cpu'],
                voltages=[0.8, 0.9],
            )
        """
        super().__init__(name="GreedyEnergyPolicy")
        # self.models = models
        self.available_PEs = available_PEs
        self.voltages = voltages

    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        """
        Selects configurations for each operation to minimize energy consumption
        while ensuring the total execution time does not exceed the time budget.

        Args:
            workload (list): List of operations.
            time_budget_s (float): Total time budget in seconds.

        Returns:
            list or None: Selected configurations for each operation, or None if time budget cannot be met.

        Example:
            selections = policy.select_configurations(workload, time_budget_s=1.0)
        """
        # self.models = models
        # Generate possible configurations for each operation
        selections = self._generate_selection(models, workload, time_budget_s)
        result = super()._run_selected_cfg(selections, workload)
        return result

    def _generate_configs(self, operation, models):
        """
        Generates possible configurations for an operation.

        Args:
            operation (dict): The operation details.

        Returns:
            list: Configurations with predictions.

        Example:
            configs = policy._generate_configs(operation)
        """
        configs = []
        for PE in self.available_PEs:
            for voltage in self.voltages:
                # Get max frequency for this voltage
                max_frequency = models.sim_data.max_frequency.get(voltage, None)
                if max_frequency is None:
                    continue
                prediction = models.predict(
                    PE=PE,
                    row_a=operation['row_a'],
                    col_a=operation['col_a'],
                    col_b=operation['col_b'],
                    voltage=voltage,
                    frequency_MHz=max_frequency
                )
                if prediction is None:
                    continue
                # energy_mJ = prediction['total_power_mW'] * (prediction['execution_time_ns'] * 1e-9)
                total_energy_nJ = prediction['total_energy_nJ'] 
                average_power_mW = prediction['total_power_mW']
                execution_time_ns = prediction['execution_time_ns']
                configs.append({
                    'PEs': PE,
                    'voltage': voltage,
                    'frequency_MHz': max_frequency,
                    'prediction': prediction,
                    # 'energy_mJ': energy_mJ,
                    'execution_time_ns': execution_time_ns,
                    'total_energy_nJ': total_energy_nJ,
                    'average_power_mW': average_power_mW
                })
        return configs
    
    def _generate_selection(self, models, workload, time_budget_s):
        operation_configs = []
        for operation in workload:
            configs = self._generate_configs(operation, models)
            if not configs:
                operation_configs.append([])
                continue
            # Sort configurations by energy consumption (lowest first)
            configs.sort(key=lambda x: x['total_energy_nJ'])
            operation_configs.append(configs)

        # Initial selection: choose the lowest energy configuration for each operation
        selections = [configs[0] if configs else None for configs in operation_configs]

        # Calculate total execution time
        total_time = sum(sel['prediction']['execution_time_ns'] * 1e-9 for sel in selections if sel)

        if total_time <= time_budget_s:
            return selections  # Time budget met

        # If time budget not met, adjust configurations
        # Since frequencies are fixed at max for each voltage, we can only adjust PEs and voltages

        # Create a list of possible selections with indices
        possible_selections = []
        for idx, configs in enumerate(operation_configs):
            if len(configs) > 1:
                # Exclude the current selection
                for conf in configs[1:]:
                    delta_energy = conf['total_energy_nJ'] - selections[idx]['total_energy_nJ']
                    delta_time = conf['prediction']['execution_time_ns'] * 1e-9 - selections[idx]['prediction']['execution_time_ns'] * 1e-9
                    possible_selections.append({
                        'operation_idx': idx,
                        'config': conf,
                        'delta_energy': delta_energy,
                        'delta_time': delta_time
                    })

        # Sort possible selections by efficiency of time reduction per additional energy
        possible_selections.sort(key=lambda x: x['delta_time'] / x['delta_energy'] if x['delta_energy'] > 0 else float('inf'), reverse=True)

        # Iteratively select faster configurations until time budget is met or no more options
        for sel_option in possible_selections:
            idx = sel_option['operation_idx']
            selections[idx] = sel_option['config']
            total_time = sum(sel['prediction']['execution_time_ns'] * 1e-9 for sel in selections if sel)
            if total_time <= time_budget_s:
                break

        # Final check
        if total_time > time_budget_s:
            print("Unable to meet time budget with available configurations.")
            # Indicate failure by returning None
            return None

        return selections

class FixedPEPolicy(Policy):
    """
    Policy that always selects a specified PE and voltage for all operations.

    Example:
        # Instantiate the policy
        fixed_pe_policy = FixedPEPolicy(
            models=models,
            PE='cpu',
            voltage=0.9
        )
        # Run the emulator with the policy
        eve.run(policy=fixed_pe_policy)
    """

    def __init__(self, PE, voltage):
        """
        Initializes the FixedPEPolicy.

        Args:
            models (MatmulPowerModel): The power and performance models.
            PE (str): The Processing Element to use.
            voltage (float): The voltage to use.

        Example:
            fixed_pe_policy = FixedPEPolicy(
                models=models,
                PE='cpu',
                voltage=0.9
            )
        """
        super().__init__(name=f"FixedPE_{PE}_{voltage}V")
        self.PE = PE
        self.voltage = voltage

    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        """
        Selects the specified PE and voltage for all operations.

        Args:
            workload (list): List of operations.
            time_budget_s (float): Total time budget in seconds.

        Returns:
            list or None: Selected configurations for each operation, or None if time budget cannot be met.

        Example:
            selections = policy.select_configurations(workload, time_budget_s=1.0)
        """
        selections = self._generate_selection(models, workload, time_budget_s)
        return super()._run_selected_cfg(selections, workload)
        
    def _generate_selection(self, models, workload, time_budget_s):
        # Get the maximum frequency for the given voltage
        frequency_MHz = models.sim_data.max_frequency.get(self.voltage, None)
        if frequency_MHz is None:
            raise ValueError(f"No maximum frequency found for voltage {self.voltage}V.")
        
        selections = []
        total_time = 0
        for operation in workload:
            prediction = models.predict(
                PE=self.PE,
                row_a=operation['row_a'],
                col_a=operation['col_a'],
                col_b=operation['col_b'],
                voltage=self.voltage,
                frequency_MHz=frequency_MHz
            )
            if prediction is None:
                print(f"Prediction failed for operation: {operation}")
                return None
            execution_time_ns = prediction['execution_time_ns']
            execution_time_s = execution_time_ns * 1e-9
            total_time += execution_time_s
            if total_time > time_budget_s:
                print("Unable to meet time budget with FixedPEPolicy.")
                return None
            
            total_energy_nJ = prediction['total_energy_nJ']
            average_power_mW = prediction['total_power_mW']
            selection = {
                'PEs': self.PE,
                'voltage': self.voltage,
                'frequency_MHz': frequency_MHz,
                'prediction': prediction,
                'total_energy_nJ': total_energy_nJ,
                'average_power_mW': average_power_mW,
                'execution_time_ns': execution_time_ns
            }
            selections.append(selection)
        return selections
class OptimalMCKPEnergyPolicy(Policy):
    """
    Policy that selects configurations to minimize total energy consumption
    while meeting the time budget using optimization (MCKP).
    
    Example:
        optimal_energy_policy = OptimalMCKPEnergyPolicy(
            models=models,
            available_PEs=['carus', 'caesar', 'cgra'],
            voltages=[0.5, 0.65, 0.8, 0.9]
        )
        eve.run(policy=optimal_energy_policy)
    """

    def __init__(self, available_PEs, voltages):
        super().__init__(name="OptimalMCKPEnergyPolicy")
        self.available_PEs = available_PEs
        self.voltages = voltages

    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        selections = self._generate_selection(workload, time_budget_s, models)
        return super()._run_selected_cfg(selections, workload)


    def _generate_selection(self, workload, time_budget_s, models):
        """
        Selects configurations using an optimization solver.

        Args:
            workload (list): List of operations.
            time_budget_s (float): Total time budget in seconds.

        Returns:
            list or None: Selected configurations for each operation, or None if no feasible solution.
        """

        # Generate possible configurations for each operation
        operation_configs = []
        for operation in workload:
            configs = self._generate_configs(operation, models)
            if not configs:
                operation_configs.append([])
            else:
                operation_configs.append(configs)

        # Check if any operation has no configurations
        for idx, configs in enumerate(operation_configs):
            if not configs:
                print(f"No configurations available for operation {idx}.")
                return None

        num_operations = len(workload)
        prob = LpProblem("OptimalMCKPEnergyPolicy", LpMinimize)

        # Decision variables
        x = []
        for i in range(num_operations):
            x_i = []
            for j in range(len(operation_configs[i])):
                var = LpVariable(f"x_{i}_{j}", cat="Binary")
                x_i.append(var)
            x.append(x_i)

        # Objective function: Minimize total energy
        prob += lpSum([
            x[i][j] * operation_configs[i][j]['energy_mJ']
            for i in range(num_operations)
            for j in range(len(operation_configs[i]))
        ])

        # Constraint: Select exactly one configuration per operation
        for i in range(num_operations):
            prob += lpSum(x[i]) == 1

        # Constraint: Total time must be within the time budget
        total_time = lpSum([
            x[i][j] * (operation_configs[i][j]['prediction']['execution_time_ns'] * 1e-9)
            for i in range(num_operations)
            for j in range(len(operation_configs[i]))
        ])
        prob += total_time <= time_budget_s

        # Solve the problem
        solver = PULP_CBC_CMD(msg=False)
        result_status = prob.solve(solver)

        if LpStatus[result_status] != 'Optimal':
            print("No feasible solution found within the time budget.")
            return None

        # Extract the selected configurations
        selections = []
        for i in range(num_operations):
            selected_j = None
            for j in range(len(operation_configs[i])):
                if x[i][j].varValue == 1:
                    selected_j = j
                    break
            if selected_j is None:
                print(f"No configuration selected for operation {i}.")
                return None
            selections.append(operation_configs[i][selected_j])

        return selections

    def _generate_configs(self, operation, models):
        configs = []
        for PE in self.available_PEs:
            for voltage in self.voltages:
                # Get max frequency for this voltage
                max_frequency = models.sim_data.max_frequency.get(voltage, None)
                if max_frequency is None:
                    continue
                prediction = models.predict(
                    PE=PE,
                    row_a=operation['row_a'],
                    col_a=operation['col_a'],
                    col_b=operation['col_b'],
                    voltage=voltage,
                    frequency_MHz=max_frequency
                )
                if prediction is None:
                    continue
                total_energy_nJ = prediction['total_energy_nJ']
                energy_mJ = total_energy_nJ * 1e-6
                average_power_mW = prediction['total_power_mW']
                execution_time_ns = prediction['execution_time_ns']
                configs.append({
                    'PEs': PE,
                    'voltage': voltage,
                    'frequency_MHz': max_frequency,
                    'prediction': prediction,
                    'energy_mJ': energy_mJ,         # I kept it bc previously written based on mJ
                    'total_energy_nJ': total_energy_nJ,
                    'execution_time_ns': execution_time_ns,
                    'average_power_mW': average_power_mW
                })
        return configs
class MaxPerformancePolicy(Policy):
    """
    Policy that selects configurations to minimize total execution time,
    regardless of energy consumption.

    Example:
        max_performance_policy = MaxPerformancePolicy(
            models=models,
            available_PEs=['carus', 'caesar', 'cgra'],
            voltages=[0.9, 0.8, 0.65, 0.5]
        )
        eve.run(policy=max_performance_policy)
    """

    def __init__(self, available_PEs, voltages):
        super().__init__(name="MaxPerformance")
        self.available_PEs = available_PEs
        self.voltages = voltages
    
    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        selections = self._generate_selection(workload, time_budget_s, models, pe_memory_capacity_byte)
        return super()._run_selected_cfg(selections, workload)

    def _generate_selection(self, workload, time_budget_s, models, pe_memory_capacity_byte):
        """
        Selects configurations to minimize total execution time.

        Args:
            workload (list): List of operations.
            time_budget_s (float): Total time budget in seconds.

        Returns:
            list or None: Selected configurations for each operation, or None if no feasible solution.
        """
        selections = []
        total_time = 0
        for operation in workload:
            fastest_config = self._get_fastest_configuration(operation, models)
            if fastest_config is None:
                print(f"No valid configuration for operation: {operation}")
                return None
            execution_time_s = fastest_config['prediction']['execution_time_ns'] * 1e-9
            total_time += execution_time_s
            selections.append(fastest_config)

        if total_time > time_budget_s:
            print("Unable to meet time budget with MaxPerformancePolicy.")
            return None

        return selections

    def _get_fastest_configuration(self, operation, models):
        best_config = None
        min_time = float('inf')
        for PE in self.available_PEs:
            for voltage in self.voltages:
                # Get max frequency for this voltage
                max_frequency = models.sim_data.max_frequency.get(voltage, None)
                if max_frequency is None:
                    continue
                prediction = models.predict(
                    PE=PE,
                    row_a=operation['row_a'],
                    col_a=operation['col_a'],
                    col_b=operation['col_b'],
                    voltage=voltage,
                    frequency_MHz=max_frequency
                )
                if prediction is None:
                    continue
                execution_time_ns = prediction['execution_time_ns']
                if execution_time_ns < min_time:
                    min_time = execution_time_ns
                    # energy_mJ = prediction['total_power_mW'] * (execution_time_ns * 1e-9)
                    total_energy_nJ = prediction['total_energy_nJ']
                    average_power_mW = prediction['total_power_mW']
                    best_config = {
                        'PEs': PE,
                        'voltage': voltage,
                        'frequency_MHz': max_frequency,
                        'prediction': prediction,
                        # 'energy_mJ': energy_mJ,
                        'total_energy_nJ': total_energy_nJ,
                        'average_power_mW': average_power_mW,
                        'execution_time_ns': execution_time_ns
                    }
        return best_config
class OptimalFixedVoltageEnergyPolicy(Policy):
    """
    Policy that selects configurations to minimize total energy consumption under the time budget,
    with all operations performed at the same voltage, using optimization.

    Example:
        optimal_fixed_voltage_policy = OptimalFixedVoltageEnergyPolicy(
            models=models,
            available_PEs=['carus', 'caesar', 'cgra'],
            voltage=0.8
        )
        eve.run(policy=optimal_fixed_voltage_policy)
    """

    def __init__(self, available_PEs, voltage):
        super().__init__(name=f"OptimalFixedVoltageEnergy_{voltage}V")
        self.available_PEs = available_PEs
        self.voltage = voltage
    
    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        selections = self._generate_selection(workload, time_budget_s, models, pe_memory_capacity_byte)
        return super()._run_selected_cfg(selections, workload)

    def _generate_selection(self, workload, time_budget_s, models, pe_memory_capacity_byte):
        """
        Selects configurations using an optimization solver.

        Args:
            workload (list): List of operations.
            time_budget_s (float): Total time budget in seconds.

        Returns:
            list or None: Selected configurations for each operation, or None if no feasible solution.
        """
        import pulp

        # Generate possible configurations for each operation
        operation_configs = []
        for operation in workload:
            configs = self._generate_configs(operation, models)
            if not configs:
                print(f"No configurations available for operation: {operation}")
                return None
            else:
                operation_configs.append(configs)

        # Check if any operation has no configurations
        for idx, configs in enumerate(operation_configs):
            if not configs:
                print(f"No configurations available for operation {idx}.")
                return None

        num_operations = len(workload)
        prob = pulp.LpProblem("OptimalFixedVoltageEnergyPolicy", pulp.LpMinimize)

        # Decision variables
        x = []
        for i in range(num_operations):
            x_i = []
            for j in range(len(operation_configs[i])):
                var = pulp.LpVariable(f"x_{i}_{j}", cat="Binary")
                x_i.append(var)
            x.append(x_i)

        # Objective function: Minimize total energy
        prob += lpSum([
            x[i][j] * operation_configs[i][j]['energy_mJ']
            for i in range(num_operations)
            for j in range(len(operation_configs[i]))
        ])

        # Constraint: Select exactly one configuration per operation
        for i in range(num_operations):
            prob += lpSum(x[i]) == 1

        # Constraint: Total time must be within the time budget
        total_time = lpSum([
            x[i][j] * (operation_configs[i][j]['prediction']['execution_time_ns'] * 1e-9)
            for i in range(num_operations)
            for j in range(len(operation_configs[i]))
        ])
        prob += total_time <= time_budget_s

        # Solve the problem
        solver = PULP_CBC_CMD(msg=False)
        result_status = prob.solve(solver)

        if pulp.LpStatus[result_status] != 'Optimal':
            print("No feasible solution found within the time budget at fixed voltage.")
            return None

        # Extract the selected configurations
        selections = []
        for i in range(num_operations):
            selected_j = None
            for j in range(len(operation_configs[i])):
                if x[i][j].varValue == 1:
                    selected_j = j
                    break
            if selected_j is None:
                print(f"No configuration selected for operation {i}.")
                return None
            selections.append(operation_configs[i][selected_j])

        return selections

    def _generate_configs(self, operation, models):
        configs = []
        voltage = self.voltage
        max_frequency = models.sim_data.max_frequency.get(voltage, None)
        if max_frequency is None:
            print(f"No max frequency found for voltage {voltage}V.")
            return []
        for PE in self.available_PEs:
            prediction = models.predict(
                PE=PE,
                row_a=operation['row_a'],
                col_a=operation['col_a'],
                col_b=operation['col_b'],
                voltage=voltage,
                frequency_MHz=max_frequency
            )
            if prediction is None:
                continue
            execution_time_ns = prediction['execution_time_ns'] 
            # energy_mJ = prediction['total_power_mW'] * execution_time_s
            total_energy_nJ = prediction['total_energy_nJ']
            energy_mJ = total_energy_nJ * 1e-6
            average_power_mW = prediction['total_power_mW']
            configs.append({
                'PEs': PE,
                'voltage': voltage,
                'frequency_MHz': max_frequency,
                'prediction': prediction,
                'energy_mJ': energy_mJ,
                'total_energy_nJ': total_energy_nJ,
                'execution_time_ns': execution_time_ns,
                'average_power_mW': average_power_mW
            })
        return configs
class PerOperationFixedVoltageEnergyPolicy(Policy):
    """
    Policy that selects the most energy-efficient configuration for each operation
    at a fixed voltage, regardless of time budget.

    Example:
        per_op_fixed_voltage_policy = PerOperationFixedVoltageEnergyPolicy(
            models=models,
            available_PEs=['carus', 'caesar', 'cgra'],
            voltage=0.65
        )
        eve.run(policy=per_op_fixed_voltage_policy)
    """

    def __init__(self, available_PEs, voltage):
        super().__init__(name=f"PerOperationEnergy_{voltage}V")
        self.available_PEs = available_PEs
        self.voltage = voltage

    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        selections = self._generate_selection(workload, time_budget_s, models, pe_memory_capacity_byte)
        return super()._run_selected_cfg(selections, workload)

    def _generate_selection(self, workload, time_budget_s, models, pe_memory_capacity_byte):
        """
        Selects the most energy-efficient configuration for each operation at the fixed voltage.

        Args:
            workload (list): List of operations.
            time_budget_s (float): Total time budget in seconds.

        Returns:
            list or None: Selected configurations for each operation.
        """
        selections = []
        total_time = 0
        for operation in workload:
            best_config = self._get_most_energy_efficient_configuration(operation, models)
            if best_config is None:
                print(f"No valid configuration for operation: {operation}")
                return None
            execution_time_s = best_config['prediction']['execution_time_ns'] * 1e-9
            total_time += execution_time_s
            selections.append(best_config)
        
        # check if time budget is met
        if total_time > time_budget_s:
            print("Unable to meet time budget with PerOperationFixedVoltageEnergyPolicy.")
            return None
        
        # Note: This policy does not check the time budget, but you can add a check if needed
        return selections

    def _get_most_energy_efficient_configuration(self, operation, models):
        best_config = None
        min_energy = float('inf')
        voltage = self.voltage
        max_frequency = models.sim_data.max_frequency.get(voltage, None)
        if max_frequency is None:
            print(f"No max frequency found for voltage {voltage}V.")
            return None
        for PE in self.available_PEs:
            prediction = models.predict(
                PE=PE,
                row_a=operation['row_a'],
                col_a=operation['col_a'],
                col_b=operation['col_b'],
                voltage=voltage,
                frequency_MHz=max_frequency
            )
            if prediction is None:
                continue
            execution_time_ns = prediction['execution_time_ns']
            total_energy_nJ = prediction['total_energy_nJ']
            energy_mJ = total_energy_nJ * 1e-6
            if energy_mJ < min_energy:
                min_energy = energy_mJ
                average_power_mW = prediction['total_power_mW']
                best_config = {
                    'PEs': PE,
                    'voltage': voltage,
                    'frequency_MHz': max_frequency,
                    'prediction': prediction,
                    # 'energy_mJ': energy_mJ,
                    'total_energy_nJ': total_energy_nJ,
                    'execution_time_ns': execution_time_ns,
                    'average_power_mW': average_power_mW
                }
        return best_config
class FixedPEPolicyWMem(Policy):
    """
    A policy that executes a workload on a specific PE and voltage, while considering memory limitations.
    If an operation exceeds the memory capacity of the PE, the operation is tiled and executed piece by piece.

    Example:
        fixed_pe_policy_w_mem = FixedPEPolicyWMem(
            PE='carus',
            voltage=0.8
        )
        eve.run(policy=fixed_pe_policy_w_mem)
    """
    def __init__(self, PE, voltage, verbose=False):
        """
        Initializes the FixedPEPolicyWMem.

        Args:
            models (MatmulPowerModel): The power and performance models.
            PE (str): The specific Processing Element (PE) to use.
            voltage (float): The voltage to use for the PE.
            pe_memory_capacity_byte (dict): The memory capacity in bytes for each PE.
        """
        super().__init__(name=f"FixedPEPolicyWMem_{PE}_{voltage:.2f}V")
        self.PE = PE
        self.voltage = voltage
        self.verbose = verbose

    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        """
        Run the workload using the specified PE and voltage, considering the memory capacity.

        Args:
            workload (list): The list of operations to be run.
            time_budget_s (float): The time budget in seconds.
            pe_memory_capacity_byte (dict): The memory capacity in bytes for each PE.

        Returns:
            dict: A dictionary containing results (success, energy, timing, etc.)
        """
        memory_limit_bytes = pe_memory_capacity_byte.get(self.PE, None)
        if memory_limit_bytes is None:
            return {
                'success': False,
                'message': f'Memory capacity for PE "{self.PE}" is not defined.'
            }

        total_energy_mJ = 0
        total_time_ms = 0
        detailed_results = []

        for idx, operation in enumerate(workload):
            row_a, col_a, col_b = operation['row_a'], operation['col_a'], operation['col_b']
            
            # Generate tiling sequence using the memory capacity
            tiling_sequence = generate_tiling_sequence(
                memory_limit_bytes=memory_limit_bytes,
                ra=row_a,
                ca=col_a,
                cb=col_b,
                bytes_per_element=4
            )

            if not tiling_sequence:
                print(f"Unable to generate tiling sequence for operation {idx + 1} due to memory limitation.")
                continue

            # Iterate over each tile, predict and accumulate results
            for tile_idx, tile in enumerate(tiling_sequence):
                # tile_row_a, tile_col_a, tile_col_b = tile
                tile_row_a = tile['tile_ra']
                tile_col_a = tile['tile_ca']
                tile_col_b = tile['tile_cb']
                
                # Predict the energy and execution time for the given tile
                prediction = models.predict_single_pe(
                    PE=self.PE,
                    row_a=tile_row_a,
                    col_a=tile_col_a,
                    col_b=tile_col_b,
                    voltage=self.voltage
                )

                if prediction is None:
                    print(f"Prediction failed for tile {tile_idx + 1} of operation {idx + 1}.")
                    continue

                # Get energy and execution time for the current tile
                # energy_mJ = prediction['total_power_mW'] * (prediction['execution_time_ns'] * 1e-9) * 1e-3  # mJ
                energy_mJ = prediction['total_energy_nJ'] * 1e-6
                execution_time_ms = prediction['execution_time_ns'] * 1e-6  # ms

                total_energy_mJ += energy_mJ
                total_time_ms += execution_time_ms

                # Append detailed results for this tile
                detailed_results.append({
                    'operation': f"Operation {idx + 1} - Tile {tile_idx + 1}",
                    'PE': self.PE,
                    'voltage': self.voltage,
                    'frequency_MHz': prediction['frequency_MHz'],
                    'energy_mJ': energy_mJ,
                    'execution_time_ms': execution_time_ms,
                    'tile_size': (tile_row_a, tile_col_a, tile_col_b)
                })

                if self.verbose:
                    print(f"Operation {idx + 1} - Tile {tile_idx + 1} - Size: {tile_row_a}x{tile_col_a}x{tile_col_b}")
                    print(f"Energy: {energy_mJ:.2f} mJ, Time: {execution_time_ms:.2f} ms")
                    print()

        
        if self.verbose:
            print(f"Total Energy: {total_energy_mJ:.2f} mJ, Total Time: {total_time_ms:.2f} ms")

        # Calculate averages
        if total_time_ms > 0 and len(workload) > 0:
            average_energy_mJ = total_energy_mJ / len(workload)
            average_time_ms = total_time_ms / len(workload)
            average_power_mW = (total_energy_mJ * 1e3) / total_time_ms  # mW
        else:
            return {
                'success': False,
                'message': 'Time budget could not be met or no valid operations found.'
            }

        # Check if we meet the time budget
        if (total_time_ms * 1e-3) > time_budget_s:
            return {
                'success': False,
                'message': 'Time budget could not be met with tiling and given memory limitations.'
            }

        # Return the results in the expected format
        return {
            'success': True,
            'total_energy_mJ': total_energy_mJ,
            'total_time_ms': total_time_ms,
            'average_energy_mJ': average_energy_mJ,
            'average_time_ms': average_time_ms,
            'average_power_mW': average_power_mW,
            'detailed_results': detailed_results
        }
class MaxPerformancePolicyWMem(Policy):
    """
    Policy that selects the highest-performing configuration for each operation while considering memory limitations.
    """

    def __init__(self, available_PEs, voltages):
        """
        Initializes the MaxPerformancePolicyWMem with the specified PEs and voltages.

        Args:
            available_PEs (list): List of available PEs for execution.
            voltages (list): List of voltages to consider for each PE.
        """
        super().__init__(name="MaxPerformancePolicyWMem")
        self.available_PEs = available_PEs
        self.voltages = voltages

    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        """
        Runs the policy and returns the results of running the workload.

        Args:
            models (MatmulPowerModel): The power and performance models.
            workload (list): List of operations to be executed.
            time_budget_s (float): Time budget for the workload.
            pe_memory_capacity_byte (dict): Dictionary of available memory for each PE.

        Returns:
            dict: Results of running the workload including total energy, time, and detailed results.
        """
        total_energy_mJ = 0
        total_time_ms = 0
        detailed_results = []

        for idx, operation in enumerate(workload):
            row_a, col_a, col_b = operation['row_a'], operation['col_a'], operation['col_b']
            selected_config = None
            best_time_ms = float('inf')

            # Iterate over all available PEs and voltages in descending order to prioritize performance
            for PE in self.available_PEs:
                if PE not in pe_memory_capacity_byte:
                    continue

                memory_limit = pe_memory_capacity_byte[PE]
                # Generate the largest possible tiles that fit into memory
                tiling_sequence = generate_tiling_sequence(memory_limit, row_a, col_a, col_b)

                for voltage in sorted(self.voltages, reverse=True):  # Highest voltage first
                    energy_mJ_accumulated = 0
                    time_ms_accumulated = 0
                
                    for tile_idx, tile in enumerate(tiling_sequence):
                        tile_ra, tile_ca, tile_cb = tile['tile_ra'], tile['tile_ca'], tile['tile_cb']
                        # Predict energy and time for each tile
                        prediction = models.predict_single_pe(PE, tile_ra, tile_ca, tile_cb, voltage)
                        if prediction is None:
                            continue

                        # Calculate energy and time for the tile from the prediction
                        energy_mJ = prediction['total_energy_nJ'] * 1e-6  # Convert nJ to mJ
                        execution_time_ms = prediction['execution_time_ns'] * 1e-6  # Convert ns to ms
                        energy_mJ_accumulated += energy_mJ
                        time_ms_accumulated += execution_time_ms

                    # Select the best (minimum time) configuration for this operation
                    if time_ms_accumulated < best_time_ms:
                        best_time_ms = time_ms_accumulated
                        selected_config = {
                            'PE': PE,
                            'voltage': voltage,
                            'frequency_MHz': prediction['frequency_MHz'],
                            'total_energy_mJ': energy_mJ_accumulated,
                            'total_time_ms': time_ms_accumulated,
                            'tiling_sequence': tiling_sequence
                        }

            if selected_config is None:
                print(f"Operation {idx + 1} could not be configured due to memory limitations.")
                continue

            total_energy_mJ += selected_config['total_energy_mJ']
            total_time_ms += selected_config['total_time_ms']
            detailed_results.append({
                'operation': operation,
                'PE': selected_config['PE'],
                'voltage': selected_config['voltage'],
                'frequency_MHz': selected_config['frequency_MHz'],
                'energy_mJ': selected_config['total_energy_mJ'],
                'execution_time_ms': selected_config['total_time_ms'],
                'tiling_sequence': selected_config['tiling_sequence']
            })

        # Calculate averages and return results
        num_operations = len(detailed_results)
        average_energy_mJ = total_energy_mJ / num_operations if num_operations > 0 else 0
        average_time_ms = total_time_ms / num_operations if num_operations > 0 else 0
        average_power_mW = (total_energy_mJ * 1e3) / total_time_ms if total_time_ms > 0 else 0  # mW

        # Check if the total time exceeds the time budget
        if (total_time_ms * 1e-3) > time_budget_s:
            return {
                'success': False,
                'message': 'Time budget could not be met with available configurations.'
            }

        # Return the final results
        return {
            'success': True,
            'total_energy_mJ': total_energy_mJ,
            'total_time_ms': total_time_ms,
            'average_energy_mJ': average_energy_mJ,
            'average_time_ms': average_time_ms,
            'average_power_mW': average_power_mW,
            'detailed_results': detailed_results
        }
class OptimalMCKPEnergyPolicyWMem(Policy):
    """
    Policy that selects configurations to minimize total energy consumption
    while meeting the time budget using optimization (MCKP).
    """

    def __init__(self, available_PEs, voltages, verbose=False):
        """
        Initializes the OptimalMCKPEnergyPolicyWMem.

        Args:
            available_PEs (list): List of available PEs for execution.
            voltages (list): List of voltages to consider for each PE.
            verbose (bool): Whether to enable verbose output for monitoring progress.
        """
        super().__init__(name="OptimalMCKPEnergyPolicyWMem")
        self.available_PEs = available_PEs
        self.voltages = voltages
        self.verbose = verbose

    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        """
        Runs the policy and returns the results of running the workload.

        Args:
            models (MatmulPowerModel): The power and performance models.
            workload (list): List of operations to be executed.
            time_budget_s (float): Time budget for the workload.
            pe_memory_capacity_byte (dict): Dictionary of available memory for each PE.

        Returns:
            dict: Results of running the workload including total energy, time, and detailed results.
        """
        # Step 1: Generate possible configurations for each operation
        if self.verbose:
            print(f"Generating possible configurations for each of {len(workload)} operations...")
        
        operation_configs = []
        for idx, operation in enumerate(workload):
            if self.verbose:
                print(f"  Generating configurations for operation {idx + 1}/{len(workload)}...")
            configs = self._generate_configs(models, operation, pe_memory_capacity_byte)
            if not configs:
                operation_configs.append([])
            else:
                operation_configs.append(configs)

        # Step 2: Check if any operation has no feasible configurations
        for idx, configs in enumerate(operation_configs):
            if not configs:
                print(f"No configurations available for operation {idx}.")
                return {
                    'success': False,
                    'message': f"No feasible configuration found for operation {idx}."
                }

        # Step 3: Solve the optimization problem using MCKP solver (Pulp)
        if self.verbose:
            print("Solving MCKP to find the optimal set of configurations...")
        selections = self._solve_mckp(operation_configs, time_budget_s)

        # If no feasible solution, return failure message
        if selections is None:
            return {
                'success': False,
                'message': 'Time budget could not be met with available configurations.'
            }

        # Step 4: Aggregate results from selected configurations
        if self.verbose:
            print("Aggregating results from the selected configurations...")
        return self._aggregate_results(selections, workload)

    def _generate_configs(self, models, operation, pe_memory_capacity_byte):
        """
        Generates possible configurations for an operation, considering memory limitations.

        Args:
            models (MatmulPowerModel): The power and performance models.
            operation (dict): The operation details.
            pe_memory_capacity_byte (dict): Dictionary of available memory for each PE.

        Returns:
            list: Configurations with predictions.
        """
        configs = []
        row_a, col_a, col_b = operation['row_a'], operation['col_a'], operation['col_b']

        # Iterate over all available PEs and voltages to generate configurations
        for PE in self.available_PEs:
            if PE not in pe_memory_capacity_byte:
                continue

            memory_limit = pe_memory_capacity_byte[PE]
            tiling_sequence = generate_tiling_sequence(memory_limit, row_a, col_a, col_b)

            for voltage in self.voltages:
                energy_mJ_accumulated = 0
                time_ms_accumulated = 0

                if self.verbose:
                    print(f"    Evaluating tiling sequence for PE {PE} at {voltage}V...")
                
                for tile_idx, tile in enumerate(tiling_sequence):
                    tile_ra, tile_ca, tile_cb = tile['tile_ra'], tile['tile_ca'], tile['tile_cb']

                    # Predict energy and time for the tile
                    prediction = models.predict_single_pe(PE, tile_ra, tile_ca, tile_cb, voltage)
                    if prediction is None:
                        continue

                    # Calculate energy and time for the tile from the prediction
                    energy_mJ = prediction['total_energy_nJ'] * 1e-6  # Convert nJ to mJ
                    execution_time_ms = prediction['execution_time_ns'] * 1e-6  # Convert ns to ms
                    energy_mJ_accumulated += energy_mJ
                    time_ms_accumulated += execution_time_ms

                # Create configuration dictionary
                configs.append({
                    'PE': PE,
                    'voltage': voltage,
                    'frequency_MHz': prediction['frequency_MHz'],
                    'total_energy_mJ': energy_mJ_accumulated,
                    'total_time_ms': time_ms_accumulated,
                    'tiling_sequence': tiling_sequence
                })
        return configs

    def _solve_mckp(self, operation_configs, time_budget_s):
        """
        Solves the MCKP problem to find the optimal set of configurations.

        Args:
            operation_configs (list): List of possible configurations for each operation.
            time_budget_s (float): Time budget for the workload.

        Returns:
            list or None: Selected configurations for each operation, or None if no feasible solution.
        """
        num_operations = len(operation_configs)
        prob = LpProblem("OptimalMCKPEnergyPolicyWMem", LpMinimize)

        # Decision variables
        x = []
        for i in range(num_operations):
            x_i = []
            for j in range(len(operation_configs[i])):
                var = LpVariable(f"x_{i}_{j}", cat="Binary")
                x_i.append(var)
            x.append(x_i)

        # Objective function: Minimize total energy
        prob += lpSum([
            x[i][j] * operation_configs[i][j]['total_energy_mJ']
            for i in range(num_operations)
            for j in range(len(operation_configs[i]))
        ])

        # Constraint: Select exactly one configuration per operation
        for i in range(num_operations):
            prob += lpSum(x[i]) == 1

        # Constraint: Total time must be within the time budget
        total_time = lpSum([
            x[i][j] * operation_configs[i][j]['total_time_ms'] * 1e-3  # Convert ms to s
            for i in range(num_operations)
            for j in range(len(operation_configs[i]))
        ])
        prob += total_time <= time_budget_s

        # Solve the problem
        if self.verbose:
            print("    Solving the MCKP optimization problem...")
        solver = PULP_CBC_CMD(msg=True if self.verbose else False)
        result_status = prob.solve(solver)

        if LpStatus[result_status] != 'Optimal':
            print("No feasible solution found within the time budget.")
            return None

        # Extract the selected configurations
        if self.verbose:
            print("    Extracting the selected configurations...")

        selections = []
        for i in range(num_operations):
            selected_j = None
            for j in range(len(operation_configs[i])):
                if x[i][j].varValue == 1:
                    selected_j = j
                    break
            if selected_j is None:
                print(f"No configuration selected for operation {i}.")
                return None
            selections.append(operation_configs[i][selected_j])

        return selections

    def _aggregate_results(self, selections, workload):
        """
        Aggregates the results of the selected configurations for the workload.

        Args:
            selections (list): List of selected configurations for each operation.
            workload (list): List of operations to be executed.

        Returns:
            dict: Results of running the workload.
        """
        if self.verbose:
            print("  Aggregating results from the selected configurations...")

        total_energy_mJ = 0
        total_time_ms = 0
        detailed_results = []

        for idx, (operation, selection) in enumerate(zip(workload, selections)):
            if selection is None:
                continue

            if self.verbose:
                print(f"    Operation {idx + 1}: Using PE {selection['PE']} at voltage {selection['voltage']}V")

            total_energy_mJ += selection['total_energy_mJ']
            total_time_ms += selection['total_time_ms']
            detailed_results.append({
                'operation': operation,
                'PE': selection['PE'],
                'voltage': selection['voltage'],
                'frequency_MHz': selection['frequency_MHz'],
                'energy_mJ': selection['total_energy_mJ'],
                'execution_time_ms': selection['total_time_ms'],
                'tiling_sequence': selection['tiling_sequence']
            })

        num_operations = len(detailed_results)
        average_energy_mJ = total_energy_mJ / num_operations if num_operations > 0 else 0
        average_time_ms = total_time_ms / num_operations if num_operations > 0 else 0
        average_power_mW = (total_energy_mJ * 1e3) / total_time_ms if total_time_ms > 0 else 0  # mW

        return {
            'success': True,
            'total_energy_mJ': total_energy_mJ,
            'total_time_ms': total_time_ms,
            'average_energy_mJ': average_energy_mJ,
            'average_time_ms': average_time_ms,
            'average_power_mW': average_power_mW,
            'detailed_results': detailed_results
        }

class ParallelTilingPolicy(Policy):
    def __init__(self, available_PEs, voltage, verbose=False):
        """
        Initializes the ParallelTilingPolicy.

        Args:
            available_PEs (list): List of available PEs for execution.
            voltage (float): Operating voltage.
            verbose (bool): Whether to print detailed output during the execution.
        """
        super().__init__(name=f"ParallelTilingPolicy_{voltage:.2f}V")
        # self.pe_configs = pe_configs
        self.available_PEs = available_PEs
        self.voltage = voltage
        self.verbose = verbose

    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        """
        Executes the workload using the ParallelTilingPolicy.

        Args:
            models (object): Power and performance models.
            workload (list): List of operations to execute (each operation is a dict).
            time_budget_s (float): Total time budget for executing the workload.
            pe_memory_capacity_byte (dict): Dictionary of memory capacities for each PE.

        Returns:
            dict: Results of executing the workload.
        """
        total_energy_mJ = 0
        total_time_ms = 0
        detailed_results = []

        # Create PE configs for tiling based on available PEs and memory capacities
        pe_configs = []
        for id_pe, pe_name in enumerate(self.available_PEs):
            memory_limit = pe_memory_capacity_byte.get(pe_name, None)
            if memory_limit is None:
                print(f"Memory limit for {pe_name} not provided.")
                continue
            pe_configs.append({
                'id': id_pe,
                'name': pe_name,
                'memory_limit_bytes': memory_limit
            })

        # Process each operation in the workload
        for idx, operation in enumerate(workload):
            ra, ca, cb = operation['row_a'], operation['col_a'], operation['col_b']

            # Generate tiling batches for the given operation
            tiling_batches = generate_tiling_sequence_multiple_pes(
                pe_configs=pe_configs,
                ra=ra,
                ca=ca,
                cb=cb,
                verbose=self.verbose
            )

            # Process each batch
            for batch_idx, batch in enumerate(tiling_batches):
                PEs = [tile['pe_name'] for tile in batch]
                sub_operations = [
                    {
                        'row_a': tile['tile_ra'],
                        'col_a': tile['tile_ca'],
                        'col_b': tile['tile_cb']
                    }
                    for tile in batch
                ]

                # Predict energy and time for each batch (either using a single PE or multiple PEs)
                if len(PEs) == 1:
                    # Use single PE prediction if only one PE is in the batch
                    prediction = models.predict_single_pe(
                        PE=PEs[0],
                        row_a=sub_operations[0]['row_a'],
                        col_a=sub_operations[0]['col_a'],
                        col_b=sub_operations[0]['col_b'],
                        voltage=self.voltage
                    )
                else:
                    # Use multi PE prediction if multiple PEs are in the batch
                    prediction = models.predict_multi_pe(
                        PEs=PEs,
                        operations=sub_operations,
                        voltage=self.voltage
                    )

                if prediction is None:
                    print(f"Prediction failed for batch {batch_idx + 1} of operation {idx + 1}.")
                    continue

                energy_mJ = prediction['total_energy_nJ'] * 1e-6  # Convert nJ to mJ
                execution_time_ms = prediction['execution_time_ns'] * 1e-6  # Convert ns to ms
                total_energy_mJ += energy_mJ
                total_time_ms += execution_time_ms

                detailed_results.append({
                    'operation': operation,
                    'batch_idx': batch_idx + 1,
                    'PEs': PEs,
                    'voltages': self.voltage,
                    'frequency_MHz': prediction.get('all_frequencies_MHz', {}),
                    'energy_mJ': energy_mJ,
                    'execution_time_ms': execution_time_ms,
                    'batch_size': len(batch)
                })

                if self.verbose:
                    print(f"Operation {idx + 1}, Batch {batch_idx + 1}:")
                    print(f"  PEs used: {PEs}")
                    print(f"  Energy: {energy_mJ:.4f} mJ")
                    print(f"  Execution Time: {execution_time_ms:.4f} ms")

        # Check if time budget is met
        if total_time_ms / 1000 > time_budget_s:
            return {
                'success': False,
                'message': 'Time budget could not be met with available configurations.'
            }

        # Calculate average metrics
        average_energy_mJ = total_energy_mJ / len(workload)
        average_time_ms = total_time_ms / len(workload)
        average_power_mW = (total_energy_mJ * 1e3) / total_time_ms if total_time_ms > 0 else 0  # mW

        # Prepare final result
        results = {
            'success': True,
            'total_energy_mJ': total_energy_mJ,
            'total_time_ms': total_time_ms,
            'average_energy_mJ': average_energy_mJ,
            'average_time_ms': average_time_ms,
            'average_power_mW': average_power_mW,
            'detailed_results': detailed_results
        }
        return results


# =============================================================================
# Unsecusseful Policies 
# =============================================================================

class MultiPESplittingPolicy(Policy):
    """
    Policy that splits each matrix multiplication operation across multiple PEs
    to minimize energy consumption while respecting the time budget.

    Example:
        # Instantiate the policy
        multi_pe_splitting_policy = MultiPESplittingPolicy(
            models=models,
            available_PEs=['carus', 'caesar', 'cgra'],
            voltages=[0.5, 0.65, 0.8, 0.9],
        )
        # Run the emulator with the policy
        eve.run(policy=multi_pe_splitting_policy)
    """

    def __init__(self, available_PEs, voltages):
        """
        Initializes the MultiPESplittingPolicy.

        Args:
            models (MatmulPowerModelMultiPE): The power and performance models.
            available_PEs (list): List of available Processing Elements (PEs).
            voltages (list): List of voltages to consider.

        Example:
            multi_pe_splitting_policy = MultiPESplittingPolicy(
                models=models,
                available_PEs=['carus', 'caesar', 'cgra'],
                voltages=[0.5, 0.65, 0.8, 0.9],
            )
        """
        super().__init__(name="MultiPESplittingPolicy")
        self.available_PEs = available_PEs
        self.voltages = voltages
    
    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        selections = self._generate_selection(workload, time_budget_s, models, pe_memory_capacity_byte)
        return super()._run_selected_cfg(selections, workload)

    def _generate_selection(self, workload, time_budget_s, models, pe_memory_capacity_byte):
        """
        Selects configurations for each operation to minimize energy consumption
        while ensuring the total execution time does not exceed the time budget.

        Args:
            workload (list): List of operations.
            time_budget_s (float): Total time budget in seconds.

        Returns:
            list or None: Selected configurations for each operation, or None if time budget cannot be met.

        Example:
            selections = policy.select_configurations(workload, time_budget_s=1.0)
        """
        selections = []
        total_time_s = 0

        for operation in workload:
            # Generate possible split configurations for the operation
            configs = self._generate_split_configs(operation, models)

            if not configs:
                selections.append(None)
                continue

            # Sort configurations by energy consumption (lowest first)
            configs.sort(key=lambda x: x['total_energy_nJ'])
            selected_config = configs[0]
            selections.append(selected_config)

            # Update total time
            total_time_s += selected_config['execution_time_ns'] * 1e-9

        # # Check if total time exceeds the time budget
        # if total_time_s > time_budget_s:
        #     print("Unable to meet time budget with available configurations.")
        #     return None

        return selections

    def _generate_split_configs(self, operation, models):
        """
        Generates possible split configurations for an operation.

        Args:
            operation (dict): The operation details.

        Returns:
            list: Configurations with predictions.

        Example:
            configs = policy._generate_split_configs(operation)
        """
        configs = []
        # num_rows = operation['row_a']
        num_cols_b = operation['col_b']

        # Consider splitting the rows of matrix A among the available PEs
        for num_splits in range(1, len(self.available_PEs) + 1):
            # Generate all combinations of PEs for the given split
            pe_combinations = itertools.combinations(self.available_PEs, num_splits)

            for pe_combo in pe_combinations:
                # Split the rows among the selected PEs
                # rows_per_split = num_rows // num_splits
                cols_b_per_split = num_cols_b // num_splits
                # extra_rows = num_rows % num_splits
                extra_cols_b = num_cols_b % num_splits
                sub_operations = []
                PEs = []
                # start_row = 0
                start_col_b = 0

                for i, PE in enumerate(pe_combo):
                    # assigned_rows = rows_per_split + (1 if i < extra_rows else 0)
                    assigned_cols_b = cols_b_per_split + (1 if i < extra_cols_b else 0)
                    sub_op = {
                        # 'row_a': assigned_rows,
                        'row_a': operation['row_a'],
                        'col_a': operation['col_a'],
                        # 'col_b': operation['col_b']
                        'col_b': assigned_cols_b

                    }
                    sub_operations.append(sub_op)
                    PEs.append(PE)
                    # start_row += assigned_rows
                    start_col_b += assigned_cols_b

                # For each voltage, evaluate the configuration
                for voltage in self.voltages:
                    prediction = models.predict_multi_pe(
                        PEs=PEs,
                        operations=sub_operations,
                        voltage=voltage
                    )

                    if prediction is None:
                        continue

                    total_energy_nJ = prediction['total_energy_nJ']
                    execution_time_ns = prediction['execution_time_ns'] 

                    configs.append({
                        'PEs': PEs,
                        'operations': sub_operations,
                        'voltage': voltage,
                        'frequency_MHz': prediction['all_frequencies_MHz'],
                        'prediction': prediction,
                        'total_energy_nJ': total_energy_nJ,
                        'execution_time_ns': execution_time_ns
                    })

        return configs

class MultiPEWeightedSplittingPolicy(Policy):
    # TODO: there is a bug inside it
    """
    Policy that splits each matrix multiplication operation across multiple PEs
    to minimize energy consumption while respecting the time budget.
    """

    def __init__(self, available_PEs, voltages):
        """
        Initializes the MultiPESplittingPolicy.

        Args:
            models (MatmulPowerModelMultiPE): The power and performance models.
            available_PEs (list): List of available Processing Elements (PEs).
            voltages (list): List of voltages to consider.

        Example:
            multi_pe_splitting_policy = MultiPESplittingPolicy(
                models=models,
                available_PEs=['carus', 'caesar', 'cgra'],
                voltages=[0.5, 0.65, 0.8, 0.9],
            )
        """
        super().__init__(name="MultiPEWeightedSplittingPolicy")
        self.available_PEs = available_PEs
        self.voltages = voltages
    
    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        selections = self._generate_selection(workload, time_budget_s, models, pe_memory_capacity_byte)
        return super()._run_selected_cfg(selections, workload)

    def _generate_selection(self, workload, time_budget_s, models, pe_memory_capacity_byte):
        """
        Selects configurations for each operation to minimize energy consumption
        while ensuring the total execution time does not exceed the time budget.

        Args:
            workload (list): List of operations.
            time_budget_s (float): Total time budget in seconds.

        Returns:
            list or None: Selected configurations for each operation, or None if time budget cannot be met.

        Example:
            selections = policy.select_configurations(workload, time_budget_s=1.0)
        """
        selections = []
        total_time_s = 0

        for operation in workload:
            # Generate possible split configurations for the operation
            configs = self._generate_split_configs(operation, models)

            if not configs:
                selections.append(None)
                continue

            # Sort configurations by energy consumption (lowest first)
            configs.sort(key=lambda x: x['total_energy_nJ'])
            selected_config = configs[0]
            selections.append(selected_config)

            # Update total time
            total_time_s += selected_config['execution_time_ns'] * 1e-9

        # # Check if total time exceeds the time budget
        # if total_time_s > time_budget_s:
        #     print("Unable to meet time budget with available configurations.")
        #     return None

        return selections

    def _generate_split_configs(self, operation, models):
        configs = []
        num_cols_b = operation['col_b']

        # Collect efficiency scores for PEs
        pe_efficiencies = self._get_pe_efficiencies(operation, models)

        # Normalize efficiency scores to get weights
        total_efficiency = sum(pe_efficiencies.values())
        pe_weights = {PE: eff / total_efficiency for PE, eff in pe_efficiencies.items()}

        # Generate splits based on weights
        assigned_cols_b = {PE: int(pe_weights[PE] * num_cols_b) for PE in self.available_PEs}

        # Adjust for any rounding errors to ensure total columns match
        total_assigned = sum(assigned_cols_b.values())
        remainder = num_cols_b - total_assigned
        for PE in self.available_PEs:
            if remainder <= 0:
                break
            assigned_cols_b[PE] += 1
            remainder -= 1

        # Create sub-operations based on assigned columns
        sub_operations = []
        PEs = []
        for PE in self.available_PEs:
            cols_b = assigned_cols_b[PE]
            if cols_b > 0:
                sub_op = {
                    'row_a': operation['row_a'],
                    'col_a': operation['col_a'],
                    'col_b': cols_b
                }
                sub_operations.append(sub_op)
                PEs.append(PE)

        # Evaluate configuration
        for voltage in self.voltages:
            prediction = models.predict_multi_pe(
                PEs=PEs,
                operations=sub_operations,
                voltage=voltage
            )
            if prediction is None:
                continue
            total_energy_nJ = prediction['total_energy_nJ']
            execution_time_ns = prediction['execution_time_ns']
            configs.append({
                'PEs': PEs,
                'operations': sub_operations,
                'voltage': voltage,
                'frequency_MHz': prediction['all_frequencies_MHz'],
                'prediction': prediction,
                'total_energy_nJ': total_energy_nJ,
                'execution_time_ns': execution_time_ns
            })
        return configs

    def _get_pe_efficiencies(self, operation, models):
        efficiencies = {}
        for PE in self.available_PEs:
            # Use a representative voltage and frequency
            voltage = self.voltages[-1]  # Highest voltage
            frequency_MHz = models.sim_data.max_frequency.get(voltage, None)
            if frequency_MHz is None:
                continue
            # Predict performance for the full operation
            prediction = models.predict_single_pe(
                PE=PE,
                row_a=operation['row_a'],
                col_a=operation['col_a'],
                col_b=operation['col_b'],
                voltage=voltage,
                frequency_MHz=frequency_MHz
            )
            if prediction is None:
                continue
            # Compute efficiency (e.g., operations per nJ)
            total_ops = operation['row_a'] * operation['col_a'] * operation['col_b']
            energy_nJ = prediction['total_energy_nJ']
            if energy_nJ > 0:
                efficiency = total_ops / energy_nJ
                efficiencies[PE] = efficiency
        return efficiencies

class OptimalSplittingPolicy(Policy):
    # TODO: there is a bug inside it
    def __init__(self, available_PEs, voltages, time_budget_s):
        super().__init__(name="OptimalSplittingPolicy")
        self.available_PEs = available_PEs
        self.voltages = voltages
        self.time_budget_s = time_budget_s
    
    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        selections = self._generate_selection(workload, time_budget_s, models, pe_memory_capacity_byte)
        return super()._run_selected_cfg(selections, workload)

    def _generate_selection(self, workload, time_budget_s, models, pe_memory_capacity_byte):
        selections = []
        total_time_s = 0

        for operation in workload:
            # Solve the ILP for each operation
            best_config = self._solve_ilp_for_operation(operation, models)
            if best_config is None:
                selections.append(None)
                continue
            selections.append(best_config)
            total_time_s += best_config['max_execution_time_s']

        if total_time_s > time_budget_s:
            print("Unable to meet time budget with available configurations.")
            return None

        return selections

    def _solve_ilp_for_operation(self, operation, models):
        # Parameters
        col_b_total = operation['col_b']
        max_suboperations = min(col_b_total, 10)  # Limit to 10 sub-operations for tractability
        suboperation_sizes = [i+1 for i in range(col_b_total)]  # Sizes from 1 to col_b_total

        # Generate all possible combinations of sub-operation sizes
        possible_suboperation_combinations = [combination for combination in itertools.combinations_with_replacement(suboperation_sizes, max_suboperations)
                                              if sum(combination) == col_b_total]

        best_total_energy_nJ = float('inf')
        best_solution = None

        for subop_sizes in possible_suboperation_combinations:
            S = len(subop_sizes)
            prob = LpProblem("OptimalSplitting", LpMinimize)

            # Variables
            x = LpVariable.dicts("x", [(p, s) for p in self.available_PEs for s in range(S)], cat=LpBinary)
            T_p = LpVariable.dicts("T_p", self.available_PEs, lowBound=0)
            T_max = LpVariable("T_max", lowBound=0)

            # Prepare data structures
            t_p_s = {}
            e_dyn_p_s = {}
            e_static_p_s = {}
            for p in self.available_PEs:
                for s in range(S):
                    # Create sub-operation
                    sub_op = {
                        'row_a': operation['row_a'],
                        'col_a': operation['col_a'],
                        'col_b': subop_sizes[s]
                    }
                    # Use maximum voltage and frequency for simplicity
                    voltage = max(self.voltages)
                    max_frequency = models.sim_data.max_frequency.get(voltage, None)
                    if max_frequency is None:
                        continue
                    prediction = models.predict_single_pe(
                        PE=p,
                        row_a=sub_op['row_a'],
                        col_a=sub_op['col_a'],
                        col_b=sub_op['col_b'],
                        voltage=voltage,
                        frequency_MHz=max_frequency
                    )
                    if prediction is None:
                        t_p_s[(p, s)] = float('inf')
                        e_dyn_p_s[(p, s)] = float('inf')
                        e_static_p_s[(p, s)] = float('inf')
                    else:
                        t_p_s[(p, s)] = prediction['execution_time_ns'] * 1e-9  # Convert ns to s
                        e_dyn_p_s[(p, s)] = prediction['total_dyn_energy_nJ']
                        e_static_p_s[(p, s)] = prediction['total_static_energy_nJ']

            # Objective function
            E_total = lpSum([x[(p, s)] * (e_dyn_p_s[(p, s)] + e_static_p_s[(p, s)])
                             for p in self.available_PEs for s in range(S)])

            # Shared energy (using T_max)
            # For simplicity, assume P_shared is constant (you can refine this)
            P_shared_mW = self._get_shared_power()
            E_shared = P_shared_mW * T_max * 1e3  # Convert mW to nJ (mW * s * 1e3)

            prob += E_total + E_shared  # Minimize total energy including shared domains

            # Constraints
            # Each sub-operation is assigned to exactly one PE
            for s in range(S):
                prob += lpSum([x[(p, s)] for p in self.available_PEs]) == 1

            # Execution time per PE
            for p in self.available_PEs:
                prob += T_p[p] == lpSum([x[(p, s)] * t_p_s[(p, s)] for s in range(S)])

            # T_max is the maximum of T_p
            for p in self.available_PEs:
                prob += T_max >= T_p[p]

            # Time budget constraint (for this operation)
            prob += T_max <= self.time_budget_s

            # Solve the problem
            prob.solve()

            if prob.status != LpStatusOptimal:
                continue  # No optimal solution found for this combination

            # Extract the solution
            assigned_PEs = []
            sub_operations = []
            for s in range(S):
                for p in self.available_PEs:
                    if x[(p, s)].varValue == 1:
                        assigned_PEs.append(p)
                        sub_op = {
                            'row_a': operation['row_a'],
                            'col_a': operation['col_a'],
                            'col_b': subop_sizes[s]
                        }
                        sub_operations.append(sub_op)
                        break  # Move to next sub-operation

            # Predict energy and time using the multi-PE model
            prediction = models.predict_multi_pe(
                PEs=assigned_PEs,
                operations=sub_operations,
                voltage=voltage  # Assuming same voltage for simplicity
            )
            if prediction is None:
                continue

            total_energy_nJ = prediction['total_energy_nJ']
            max_execution_time_s = prediction['execution_time_ns'] * 1e-9

            if total_energy_nJ < best_total_energy_nJ:
                best_total_energy_nJ = total_energy_nJ
                best_solution = {
                    'PEs': assigned_PEs,
                    'operations': sub_operations,
                    'voltage': voltage,
                    'frequency_MHz': prediction['all_frequencies_MHz'],
                    'prediction': prediction,
                    'total_energy_nJ': total_energy_nJ,
                    'max_execution_time_s': max_execution_time_s
                }

        return best_solution

    def _get_shared_power(self):
        # Return an estimate of shared power consumption (mW)
        # You can refine this based on your models or data
        return 100.0  # Example value

class LimitedConfigSplittingPolicy(Policy):
    # TODO: there is a bug inside it
    """
    Policy that generates a limited set of configurations for each operation and
    uses the Multiple-Choice Knapsack Problem (MCKP) formulation with PuLP to
    select configurations that minimize energy consumption while respecting the time budget.

    Example:
        limited_config_splitting_policy = LimitedConfigSplittingPolicy(
            models=models,
            available_PEs=['carus', 'caesar', 'cgra', 'cpu'],
            voltages=[0.5, 0.65, 0.8, 0.9],
            splits=[1.0, 0.5, 0.8],  # 100%, 50/50, 80/20 splits
            max_splits_per_operation=5
        )
        eve.run(policy=limited_config_splitting_policy)
    """

    def __init__(self, available_PEs, voltages, splits, max_splits_per_operation=5):
        super().__init__(name="LimitedConfigSplittingPolicy")
        self.available_PEs = available_PEs
        self.voltages = voltages
        self.splits = splits  # List of split ratios (e.g., [1.0, 0.5, 0.8])
        self.max_splits_per_operation = max_splits_per_operation
    
    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        selections = self._generate_selection(workload, time_budget_s, pe_memory_capacity_byte)
        return super()._run_selected_cfg(selections, workload)

    def _generate_selection(self, workload, time_budget_s, pe_memory_capacity_byte):
        """
        Selects configurations for each operation to minimize energy consumption
        while ensuring the total execution time does not exceed the time budget.

        Args:
            workload (list): List of operations.
            time_budget_s (float): Total time budget in seconds.

        Returns:
            list or None: Selected configurations for each operation, or None if no feasible solution.
        """
        # Generate possible configurations for each operation
        operation_configs = []
        for operation in workload:
            configs = self._generate_configs(operation)
            if not configs:
                operation_configs.append([])
                continue
            # Sort configurations by energy consumption (lowest first)
            configs.sort(key=lambda x: x['total_energy_nJ'])
            # Limit the number of configurations per operation
            operation_configs.append(configs[:self.max_splits_per_operation])

        # Check if any operation has no configurations
        for idx, configs in enumerate(operation_configs):
            if not configs:
                print(f"No configurations available for operation {idx}.")
                return None

        # Formulate and solve the MCKP using PuLP
        selections = self._solve_mckp(operation_configs, time_budget_s)

        return selections

    def _generate_configs(self, operation):
        """
        Generates a limited set of configurations for an operation.

        Args:
            operation (dict): The operation details.

        Returns:
            list: Configurations with predictions.
        """
        configs = []
        col_b_total = operation['col_b']

        # Generate splits based on provided split ratios
        for split_ratio in self.splits:
            # For each possible number of PEs
            for num_PEs in range(1, len(self.available_PEs) + 1):
                # Generate combinations of PEs
                pe_combinations = itertools.combinations(self.available_PEs, num_PEs)
                for pe_combo in pe_combinations:
                    # Calculate split sizes based on split ratios
                    if len(pe_combo) == 1:
                        # Assign entire operation to one PE
                        sub_operations = [operation.copy()]
                        PEs = [pe_combo[0]]
                    else:
                        # Calculate split sizes
                        split_sizes = []
                        remaining_col_b = col_b_total
                        for i, PE in enumerate(pe_combo):
                            if i < len(pe_combo) - 1:
                                assigned_col_b = int(col_b_total * split_ratio)
                            else:
                                assigned_col_b = remaining_col_b
                            remaining_col_b -= assigned_col_b
                            sub_op = operation.copy()
                            sub_op['col_b'] = assigned_col_b
                            sub_operations.append(sub_op)
                            PEs.append(PE)
                    # For each voltage
                    for voltage in self.voltages:
                        # Use the maximum frequency for the voltage
                        max_frequency = self.models.sim_data.max_frequency.get(voltage, None)
                        if max_frequency is None:
                            continue
                        # Predict using predict_multi_pe
                        prediction = self.models.predict_multi_pe(
                            PEs=PEs,
                            operations=sub_operations,
                            voltage=voltage,
                            frequency_MHz=max_frequency  # Assuming same frequency for all PEs
                        )
                        if prediction is None:
                            continue
                        total_energy_nJ = prediction['total_energy_nJ']
                        execution_time_ns = prediction['execution_time_ns']

                        configs.append({
                            'PEs': PEs,
                            'operations': sub_operations,
                            'voltage': voltage,
                            'frequency_MHz': prediction['all_frequencies_MHz'],
                            'total_energy_nJ': total_energy_nJ,
                            'execution_time_ns': execution_time_ns,
                            'prediction': prediction
                        })
        return configs

    def _solve_mckp(self, operation_configs, time_budget_s):
        """
        Solves the Multiple-Choice Knapsack Problem using PuLP.

        Args:
            operation_configs (list): List of configurations per operation.
            time_budget_s (float): Total time budget in seconds.

        Returns:
            list or None: Selected configurations for each operation, or None if no feasible solution.
        """
        num_operations = len(operation_configs)
        prob = LpProblem("LimitedConfigSplittingPolicy_MCKP", LpMinimize)

        # Decision variables
        x = []
        for i in range(num_operations):
            x_i = []
            for j in range(len(operation_configs[i])):
                var = LpVariable(f"x_{i}_{j}", cat="Binary")
                x_i.append(var)
            x.append(x_i)

        # Objective function: Minimize total energy
        prob += lpSum([
            x[i][j] * operation_configs[i][j]['total_energy_nJ']
            for i in range(num_operations)
            for j in range(len(operation_configs[i]))
        ])

        # Constraint: Select exactly one configuration per operation
        for i in range(num_operations):
            prob += lpSum(x[i]) == 1

        # Constraint: Total time must be within the time budget
        total_time = lpSum([
            x[i][j] * (operation_configs[i][j]['execution_time_ns'] * 1e-9)
            for i in range(num_operations)
            for j in range(len(operation_configs[i]))
        ])
        prob += total_time <= time_budget_s

        # Solve the problem
        solver = PULP_CBC_CMD(msg=False)
        result_status = prob.solve(solver)

        if LpStatus[result_status] != 'Optimal':
            print("No feasible solution found within the time budget.")
            return None

        # Extract the selected configurations
        selections = []
        for i in range(num_operations):
            selected_j = None
            for j in range(len(operation_configs[i])):
                if x[i][j].varValue == 1:
                    selected_j = j
                    break
            if selected_j is None:
                print(f"No configuration selected for operation {i}.")
                return None
            selections.append(operation_configs[i][selected_j])

        return selections

class MultiAcceleratorMaxPerformancePolicyv1(Policy):
    # TODO: unseccessful
    """
    Policy that selects configurations to maximize performance by running multiple accelerators in parallel.

    Example:
        multi_acc_max_perf_policy = MultiAcceleratorMaxPerformancePolicy(
            available_PEs=['carus', 'caesar', 'cgra'],
            voltages=[0.5, 0.65, 0.8, 0.9],
            max_parallel_accelerators=2,
            verbose=True
        )
        eve.run(policy=multi_acc_max_perf_policy)
    """

    def __init__(self, available_PEs, voltage, max_parallel_accelerators=2, verbose=False):
        """
        Initializes the MultiAcceleratorMaxPerformancePolicy.

        Args:
            available_PEs (list): List of available Processing Elements (PEs).
            voltages (list): List of voltages to consider.
            max_parallel_accelerators (int): Maximum number of accelerators to run in parallel.
            verbose (bool): Enable verbose output for monitoring the process.
        """
        super().__init__(name="MultiAcceleratorMaxPerformancePolicy")
        self.available_PEs = available_PEs
        self.voltage = voltage
        self.max_parallel_accelerators = max_parallel_accelerators
        self.verbose = verbose

    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        """
        Runs the policy and returns the results of running the workload.

        Args:
            models (MatmulPowerModelMultiPE): The power and performance models.
            workload (list): List of operations to be executed.
            time_budget_s (float): Time budget for the workload.
            pe_memory_capacity_byte (dict): Dictionary of available memory for each PE.

        Returns:
            dict: Results of running the workload including total energy, time, and detailed results.
        """
        total_energy_mJ = 0
        total_time_ms = 0
        detailed_results = []

        # Iterate through the workload and split tasks among available PEs
        for idx, operation in enumerate(workload):
            row_a, col_a, col_b = operation['row_a'], operation['col_a'], operation['col_b']

            # Generate all possible tiling configurations
            tile_configs = self._generate_tile_configs(pe_memory_capacity_byte, row_a, col_a, col_b)

            best_configuration = None
            best_time = float('inf')

            # Try running the operation on different combinations of accelerators in parallel
            for num_accelerators in range(1, self.max_parallel_accelerators + 1):
                pe_combinations = list(itertools.combinations(self.available_PEs, num_accelerators))

                for pe_combo in pe_combinations:
                    # Generate sub-operations for each tile configuration and PE combination
                    sub_operations = [self._create_sub_operation(operation, tile, PE)
                                    for PE, tile in zip(pe_combo, tile_configs)]
                    
                    if self.verbose:
                        print(f"Operation {idx + 1}: Trying combination {pe_combo} with tile sizes...")

                    # Convert tuple to list for predict_multi_pe
                    prediction = models.predict_multi_pe(list(pe_combo), list(sub_operations), voltage=self.voltage)

                    if prediction and prediction['execution_time_ns'] < best_time:
                        best_time = prediction['execution_time_ns']
                        best_configuration = {
                            'PEs': pe_combo,
                            'voltage': self.voltage,
                            'frequency_MHz': prediction['all_frequencies_MHz'],
                            'total_energy_mJ': prediction['total_energy_nJ'] * 1e-6,  # Convert nJ to mJ
                            'execution_time_ns': prediction['execution_time_ns'],
                            'sub_operations': sub_operations,
                        }

            if not best_configuration:
                return {
                    'success': False,
                    'message': f"Unable to find a feasible configuration for operation {idx + 1}."
                }

            # print best configuration all details
            if self.verbose:
                print(f"Operation {idx + 1}: Best configuration found with PEs {best_configuration['PEs']}, "
                      f"voltage {best_configuration['voltage']}V, and frequency {best_configuration['frequency_MHz']}MHz.")
                

            # Aggregate results for the best configuration
            total_energy_mJ += best_configuration['total_energy_mJ']
            total_time_ms += best_configuration['execution_time_ns'] * 1e-6  # Convert ns to ms
            detailed_results.append({
                'operation': operation,
                'PEs': best_configuration['PEs'],
                'voltage': best_configuration['voltage'],
                'frequency_MHz': best_configuration['frequency_MHz'],
                'energy_mJ': best_configuration['total_energy_mJ'],
                'execution_time_ms': best_configuration['execution_time_ns'] * 1e-6,
                'sub_operations': best_configuration['sub_operations'],
            })

        num_operations = len(detailed_results)
        average_energy_mJ = total_energy_mJ / num_operations if num_operations > 0 else 0
        average_time_ms = total_time_ms / num_operations if num_operations > 0 else 0
        average_power_mW = (total_energy_mJ * 1e3) / total_time_ms if total_time_ms > 0 else 0  # mW

        return {
            'success': True,
            'total_energy_mJ': total_energy_mJ,
            'total_time_ms': total_time_ms,
            'average_energy_mJ': average_energy_mJ,
            'average_time_ms': average_time_ms,
            'average_power_mW': average_power_mW,
            'detailed_results': detailed_results
        }

    def _generate_tile_configs(self, pe_memory_capacity_byte, row_a, col_a, col_b):
        """
        Generate tiling configurations for the given matrix multiplication dimensions.

        Args:
            pe_memory_capacity_byte (dict): Dictionary of available memory for each PE.
            row_a, col_a, col_b (int): Dimensions of the matrix multiplication.

        Returns:
            list: Generated tiling configurations based on available memory.
        """
        # For simplicity, divide into 2 tiles for each PE.
        tile_configs = []
        for PE in self.available_PEs:
            memory_limit = pe_memory_capacity_byte[PE]
            tiling_sequence = generate_tiling_sequence(memory_limit, row_a, col_a, col_b)
            tile_configs.extend(tiling_sequence)
        return tile_configs

    def _create_sub_operation(self, operation, tile, PE):
        """
        Create a sub-operation for the given tile and PE.

        Args:
            operation (dict): Original operation to split.
            tile (dict): Tile configuration specifying the dimensions.
            PE (str): Processing Element assigned to run the tile.

        Returns:
            dict: Sub-operation configuration.
        """
        tile_ra, tile_ca, tile_cb = tile['tile_ra'], tile['tile_ca'], tile['tile_cb']
        sub_operation = {
            'row_a': tile_ra,
            'col_a': tile_ca,
            'col_b': tile_cb,
            'PE': PE,
            'details': f"Tile assigned to {PE} with size ({tile_ra}, {tile_ca}, {tile_cb})"
        }
        return sub_operation

class MultiAcceleratorMaxPerformancePolicy(Policy):
    # TODO: unseccessful
    def __init__(self, available_PEs, voltage, max_parallel_accelerators=2, verbose=False):
        super().__init__(name="MultiAcceleratorMaxPerformancePolicy")
        self.available_PEs = available_PEs
        self.voltage = voltage
        self.max_parallel_accelerators = max_parallel_accelerators
        self.verbose = verbose

    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        total_energy_mJ = 0
        total_time_ms = 0
        detailed_results = []
        success = True

        for idx, operation in enumerate(workload):
            remaining_workload = [operation]
            batch_number = 1

            while remaining_workload:
                # Find the best tiling for the first accelerator
                first_pe = self.available_PEs[0]
                first_tiling_sequence = generate_tiling_sequence(pe_memory_capacity_byte[first_pe], remaining_workload[0]['row_a'], remaining_workload[0]['col_a'], remaining_workload[0]['col_b'])
                first_tile = first_tiling_sequence[0]
                remaining_workload = self.calculate_remaining_operation(remaining_workload[0], first_tile)

                # Now select a second PE and determine the best tiling for it
                second_pe = self.available_PEs[1] if len(self.available_PEs) > 1 else None
                if second_pe and remaining_workload:
                    second_tiling_sequence = generate_tiling_sequence(pe_memory_capacity_byte[second_pe], remaining_workload[0]['row_a'], remaining_workload[0]['col_a'], remaining_workload[0]['col_b'])
                    second_tile = second_tiling_sequence[0]
                    remaining_workload = self.calculate_remaining_operation(remaining_workload[0], second_tile)
                else:
                    second_tile = None

                # Predict energy and timing for the batch
                batch_operations = [first_tile] if second_tile is None else [first_tile, second_tile]
                batch_pes = [first_pe] if second_pe is None else [first_pe, second_pe]
                prediction = models.predict_multi_pe(batch_pes, batch_operations, voltage=self.voltage)

                if prediction is None:
                    success = False
                    break

                # Collect batch details
                energy_mJ = prediction['total_energy_nJ'] * 1e-6
                execution_time_ms = prediction['execution_time_ns'] * 1e-6
                total_energy_mJ += energy_mJ
                total_time_ms += execution_time_ms

                detailed_results.append({
                    'operation': operation,
                    'batch_number': batch_number,
                    'PEs': batch_pes,
                    'energy_mJ': energy_mJ,
                    'execution_time_ms': execution_time_ms,
                })

                batch_number += 1

                # Verbose output
                if self.verbose:
                    print(f"Batch {batch_number}:")
                    print(f"  PEs used: {batch_pes}")
                    print(f"  Tile sizes: {[tile for tile in batch_operations]}")
                    print(f"  Energy: {energy_mJ:.4f} mJ")
                    print(f"  Execution time: {execution_time_ms:.4f} ms")

        if not success:
            return {
                'success': False,
                'message': 'Time budget could not be met with available configurations.'
            }

        # Calculate averages
        average_energy_mJ = total_energy_mJ / len(workload)
        average_time_ms = total_time_ms / len(workload)
        average_power_mW = (total_energy_mJ * 1e3) / total_time_ms if total_time_ms > 0 else 0

        # Prepare result
        result = {
            'success': True,
            'total_energy_mJ': total_energy_mJ,
            'total_time_ms': total_time_ms,
            'average_energy_mJ': average_energy_mJ,
            'average_time_ms': average_time_ms,
            'average_power_mW': average_power_mW,
            'detailed_results': detailed_results
        }

        return result

    def calculate_remaining_operation(self, operation, completed_tile):
        """
        Calculates the remaining operation after processing a given tile.

        Args:
            operation (dict): The original operation, containing 'row_a', 'col_a', 'col_b'.
            completed_tile (dict): The tile that was processed, containing 'tile_ra', 'tile_ca', 'tile_cb'.

        Returns:
            dict: The remaining operation.
        """
        remaining_operation = copy.deepcopy(operation)

        # Update dimensions based on the completed tile
        remaining_operation['row_a'] -= completed_tile['tile_ra']
        remaining_operation['col_a'] -= completed_tile['tile_ca']
        remaining_operation['col_b'] -= completed_tile['tile_cb']

        if remaining_operation['row_a'] <= 0 or remaining_operation['col_a'] <= 0 or remaining_operation['col_b'] <= 0:
            return []  # No remaining workload

        return [remaining_operation]

class BalancedParallelMaxPerformancePolicy(Policy):
    # TODO: unseccessful
    def __init__(self, available_PEs, voltages, verbose=False):
        super().__init__(name="BalancedParallelMaxPerformancePolicy")
        self.available_PEs = available_PEs
        self.voltages = voltages
        self.verbose = verbose

    def run(self, models, workload, time_budget_s, pe_memory_capacity_byte):
        # Extract each PE's performance metrics
        pe_performance_metrics = {}
        for PE in self.available_PEs:
            pe_performance_metrics[PE] = self._estimate_performance(models, PE, pe_memory_capacity_byte[PE])
        
        # Normalize performance metrics based on the largest value
        max_performance = max(pe_performance_metrics.values())
        for PE in self.available_PEs:
            pe_performance_metrics[PE] /= max_performance

        # Prepare PE configurations for tiling
        pe_configs_for_tiling = []
        for PE in self.available_PEs:
            pe_memory_limit = pe_memory_capacity_byte.get(PE, None)
            pe_performance_metric = pe_performance_metrics.get(PE, None)
            if pe_memory_limit is None:
                print(f"Memory limit for {PE} not provided.")
                continue
            pe_configs_for_tiling.append({
                'name': PE,
                'memory_limit_bytes': pe_memory_limit,
                'performance': pe_performance_metric
            })

        total_energy_mJ = 0
        total_time_ms = 0
        detailed_results = []

        # Process each operation in the workload
        for idx, operation in enumerate(workload):
            ra, ca, cb = operation['row_a'], operation['col_a'], operation['col_b']

            # Generate tiling batches for the given operation
            tiling_batches = generate_tiling_sequence_parallel(
                ra, ca, cb, pe_configs_for_tiling, verbose=False
                # ra, ca, cb, pe_configs_for_tiling, verbose=self.verbose
            )

            # Process each batch
            for batch_idx, batch in enumerate(tiling_batches):
                PEs = [tile['accelerator_name'] for tile in batch]
                sub_operations = [
                    {
                        'row_a': tile['tile_ra'],
                        'col_a': tile['tile_ca'],
                        'col_b': tile['tile_cb']
                    }
                    for tile in batch
                ]

                # Predict energy and time for each batch (either using a single PE or multiple PEs)
                if len(PEs) == 1:
                    # Use single PE prediction if only one PE is in the batch
                    prediction = models.predict_single_pe(
                        PE=PEs[0],
                        row_a=sub_operations[0]['row_a'],
                        col_a=sub_operations[0]['col_a'],
                        col_b=sub_operations[0]['col_b'],
                        voltage=self.voltages[0]  # Assuming a constant voltage for simplicity
                    )
                else:
                    # Use multi PE prediction if multiple PEs are in the batch
                    prediction = models.predict_multi_pe(
                        PEs=PEs,
                        operations=sub_operations,
                        voltage=self.voltages[0]  # Assuming a constant voltage for simplicity
                    )

                if prediction is None:
                    print(f"Prediction failed for batch {batch_idx + 1} of operation {idx + 1}.")
                    continue

                energy_mJ = prediction['total_energy_nJ'] * 1e-6  # Convert nJ to mJ
                execution_time_ms = prediction['execution_time_ns'] * 1e-6  # Convert ns to ms

                if self.verbose:
                    print(f"Operation {idx + 1}, Batch {batch_idx + 1}:")
                    print(f"  Sub-operations: {sub_operations}")
                    print(f"  PEs used: {PEs}")
                    print(f"  Energy: {energy_mJ:.4f} mJ")
                    print(f"  Execution Time: {execution_time_ms:.4f} ms")
                
                total_energy_mJ += energy_mJ
                total_time_ms += execution_time_ms

                detailed_results.append({
                    'operation': operation,
                    'batch_idx': batch_idx + 1,
                    'PEs': PEs,
                    'voltages': self.voltages[0],
                    'frequency_MHz': prediction.get('all_frequencies_MHz', {}),
                    'energy_mJ': energy_mJ,
                    'execution_time_ms': execution_time_ms,
                    'batch_size': len(batch)
                })


        # Check if time budget is met
        if total_time_ms / 1000 > time_budget_s:
            return {
                'success': False,
                'message': 'Time budget could not be met with available configurations.'
            }

        # Calculate average metrics
        average_energy_mJ = total_energy_mJ / len(workload)
        average_time_ms = total_time_ms / len(workload)
        average_power_mW = (total_energy_mJ * 1e3) / total_time_ms if total_time_ms > 0 else 0  # mW

        # Prepare final result
        results = {
            'success': True,
            'total_energy_mJ': total_energy_mJ,
            'total_time_ms': total_time_ms,
            'average_energy_mJ': average_energy_mJ,
            'average_time_ms': average_time_ms,
            'average_power_mW': average_power_mW,
            'detailed_results': detailed_results
        }
        return results

    def _estimate_performance(self, models, PE, memory_limit, operation=None):
        """
        Estimates the performance of a PE based on memory limit and an example operation.

        Args:
            models (MatmulPowerModelMultiPE): The power and performance models.
            PE (str): The PE to evaluate.
            memory_limit (int): The memory limit of the PE in bytes.

        Returns:
            float: The performance metric (inverse of time taken for an example operation).
        """
        if operation is not None:
            ra, ca, cb = operation['row_a'], operation['col_a'], operation['col_b']
        else:
            # Use a standard example operation size
            ra, ca, cb = 64, 64, 64

        # Generate tiling sequence for given PE based on memory limit
        tiling_sequence = generate_tiling_sequence(
            memory_limit_bytes=memory_limit,
            ra=ra,
            ca=ca,
            cb=cb,
            bytes_per_element=4
        )

        # Sum up the total execution time for all tiles in the sequence
        total_execution_time_ns = 0
        for tile in tiling_sequence:
            tile_ra, tile_ca, tile_cb = tile['tile_ra'], tile['tile_ca'], tile['tile_cb']
            prediction = models.predict_single_pe(
                PE=PE,
                row_a=tile_ra,
                col_a=tile_ca,
                col_b=tile_cb,
                voltage=self.voltages[0]
            )
            if prediction is None:
                continue

            total_execution_time_ns += prediction['execution_time_ns']

        if total_execution_time_ns == 0:
            return float('inf')  # Assign a very low performance if no valid tiles could be run

        # Return the performance metric (inverse of the total time taken)
        return 1.0 / total_execution_time_ns
    


