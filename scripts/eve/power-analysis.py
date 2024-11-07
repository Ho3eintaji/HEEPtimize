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
import matplotlib.pyplot as plt
import numpy as np
import argparse
import glob

from sims_data_models import MatmulSimulationData, MatmulDataAnalysis, MatmulPowerModel, MatmulPowerModelMultiPE
from RandomWorkload import WorkloadGenerator as random_workload_generator
from policies import GreedyEnergyPolicy, FixedPEPolicy, OptimalMCKPEnergyPolicy, MaxPerformancePolicy, OptimalFixedVoltageEnergyPolicy, PerOperationFixedVoltageEnergyPolicy, MultiPESplittingPolicy
from eve_emulator import EVE
import TransformerWorkload



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Matmul Simulation Data Analysis')
    parser.add_argument('--data_dir', type=str, default='private/matmul_postsynth_sims_onlykernelnmc', help='Directory of simulation data')
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
    # simulation_data.save_data(filename=f'{output_dir}/matmul_simulation_data_cmplt.pkl')
    simulation_data.load_data(filename=f'{output_dir}/matmul_simulation_data_cmplt.pkl')

    # data analysis class
    data_analysis = MatmulDataAnalysis(simulation_data=simulation_data, output_dir=output_dir)

    models = MatmulPowerModelMultiPE(
        sim_data=simulation_data,
        output_dir=output_dir,
        use_total_ops=True, #TODO: check and change
        degree_time=2,
        degree_dyn_power=2,
        degree_static_power=0,
        reference_frequency_MHz=100,
        model_type_time='randomforest',
        model_type_dyn_power='randomforest',
        model_type_static_power='linear',
        apply_log_transform_time=False,
        apply_log_transform_dyn_power=False,
        apply_log_transform_static_power=False,
        positive=False,
        alpha_time=100,
        alpha_dyn_power=1.0,
        alpha_static_power=1.0,
        l1_ratio_time=0.5,
        l1_ratio_dyn_power=0.5,
        l1_ratio_static_power=0.5
    )


    ######### Application Scenario #########
    # RandomWorkloadGenerator = random_workload_generator(ra_size=[2, 4, 8, 16], ca_size=[2, 4, 8, 16], cb_size=[4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048])
    # seed, workload = RandomWorkloadGenerator.generate_workload(num_operations=100, seed=None)
    # print(f"Workload generated with seed: {seed}")

    workload_details = TransformerWorkload.calculate_tsd_operations(verbose=False)
    workload = TransformerWorkload.generate_workload_from_operations(workload_details['operations'])
    
    # Optionally, save operations to a file or process further
    # For example, to save to a JSON file:
    # import json
    # with open('tsd_operations.json', 'w') as f:
    #     json.dump(results['operations'], f, indent=4)

    



    # Create the EVE emulator
    time_budget_s =  1   # 1500 * 1e-6  # us

    ######### POLICIES #########
    policies = []

    # GreedyEnergyPolicy
    optimized_energy_policy = GreedyEnergyPolicy(models=models, available_PEs=['carus', 'caesar', 'cgra', 'cpu'], voltages=[0.5, 0.65, 0.8, 0.9])
    policies.append(optimized_energy_policy)

    # OptimalMCKPEnergyPolicy
    optimal_energy_policy = OptimalMCKPEnergyPolicy(models=models, available_PEs=['carus', 'caesar', 'cgra', 'cpu'], voltages=[0.5, 0.65, 0.8, 0.9])
    policies.append(optimal_energy_policy)

    # MaxPerformancePolicy
    max_performance_policy = MaxPerformancePolicy(models=models, available_PEs=['carus', 'caesar', 'cgra', 'cpu'], voltages=[0.9, 0.8, 0.65, 0.5])
    policies.append(max_performance_policy)

    # OptimalFixedVoltageEnergyPolicy for each voltage
    for voltage in [0.5, 0.65, 0.8, 0.9]:
        policy = OptimalFixedVoltageEnergyPolicy(models=models, available_PEs=['carus', 'caesar', 'cgra', 'cpu'], voltage=voltage)
        policy.name = f"OptimalFixedVoltageEnergyPolicy_{voltage}V"
        policies.append(policy)

    # PerOperationFixedVoltageEnergyPolicy for each voltage
    for voltage in [0.5, 0.65, 0.8, 0.9]:
        policy = PerOperationFixedVoltageEnergyPolicy(models=models, available_PEs=['carus', 'caesar', 'cgra', 'cpu'], voltage=voltage)
        policy.name = f"PerOpFixedVoltageEnergyPolicy_{voltage}V"
        policies.append(policy)

    # FixedPEPolicy for each PE and voltage
    for voltage in [0.5, 0.65, 0.8, 0.9]:
        for PE in ['carus', 'caesar', 'cgra', 'cpu']:
            policy = FixedPEPolicy(models=models, PE=PE, voltage=voltage)
            policy.name = f"FixedPEPolicy_{PE}_{voltage}V"
            policies.append(policy)

    # MultiPESplittingPolicy
    # multi_pe_splitting_policy = MultiPESplittingPolicy(models=models, available_PEs=['carus', 'cgra', 'caesar'], voltages=[0.5, 0.65, 0.8, 0.9])
    # policies.append(multi_pe_splitting_policy)

    # # MultiPEWeightedSplittingPolicy
    # multi_pe_weighted_splitting_policy = MultiPEWeightedSplittingPolicy(models=models, available_PEs=['carus', 'cgra', 'caesar'], voltages=[0.5, 0.65, 0.8, 0.9])
    # policies.append(multi_pe_weighted_splitting_policy)

    # # OptimalSplittingPolicy
    # optimal_splitting_policy = OptimalSplittingPolicy(models=models, available_PEs=['carus', 'caesar', 'cgra'], voltages=[0.8, 0.9], time_budget_s=time_budget_s)
    # policies.append(optimal_splitting_policy)

    # # LimitedConfigSplittingPolicy
    # limited_config_splitting_policy = LimitedConfigSplittingPolicy(models=models, available_PEs=['carus', 'cgra'], voltages=[0.65, 0.8, 0.9], splits=[1.0, 0.5, 0.8], max_splits_per_operation=3)
    # policies.append(limited_config_splitting_policy)

    ######### RUN POLICIES #########
    eve = EVE(models=models, workload=workload, time_budget_s=time_budget_s)
    eve.run_multiple(policies=policies)
    policy_results = eve.results

    result_basic_df = eve.get_results_basic()
    print(result_basic_df)

    results_full = eve.get_results()
    print(results_full['OptimalMCKPEnergyPolicy']['total_energy_mJ'])

    


