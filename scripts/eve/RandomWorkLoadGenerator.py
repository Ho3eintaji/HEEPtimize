import numpy as np
import random

class WorkloadGenerator:
    """
    Class to generate workloads of matmul operations.

    Example:
        generator = WorkloadGenerator(ra_size=[16, 32, 64, 128], ca_size=[16, 32, 64, 128], cb_size=[16, 32, 64, 128])
        workload = generator.generate_workload(num_operations=100)
    """
    def __init__(self, ra_size, ca_size, cb_size):
        """
        Initializes the WorkloadGenerator.

        Args:
            ra_size (list): List of sizes to choose from. Defaults to [16, 32, 64, 128].
            ca_size (list): List of sizes to choose from. Defaults to [16, 32, 64, 128].
            cb_size (list): List of sizes to choose from. Defaults to [16, 32, 64, 128].
        """
        self.ra_size = ra_size
        self.ca_size = ca_size
        self.cb_size = cb_size

    def generate_workload(self, num_operations, seed=None):
        """
        Generates a workload of random matmul operations.

        Args:
            num_operations (int): Number of operations to generate.

        Returns:
            list: A list of operation dictionaries.

        Example:
            workload = generator.generate_workload(num_operations=100)
        """
        if seed is None:
            seed = random.randint(0, 2**32 - 1)  # Generate a random seed if none is provided
        random.seed(seed)  # Set the seed for reproducibility

        workload = []
        for _ in range(num_operations):
            row_a = random.choice(self.ra_size)
            col_a = random.choice(self.ca_size)
            col_b = random.choice(self.cb_size) 
            workload.append({
                'row_a': row_a,
                'col_a': col_a,
                'col_b': col_b
            })
        return seed, workload   