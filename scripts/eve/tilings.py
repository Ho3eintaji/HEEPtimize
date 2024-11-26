


def generate_tiling_sequence(memory_limit_bytes, ra, ca, cb, bytes_per_element=4, verbose=False):
    """
    Generates a tiling sequence for matrix multiplication to fit within memory constraints.
    
    Args:
        memory_limit_bytes (int): The available memory in bytes for a given PE.
        ra (int): Number of rows in matrix A.
        ca (int): Number of columns in matrix A (also rows in matrix B).
        cb (int): Number of columns in matrix B.
        bytes_per_element (int, optional): Size of each matrix element in bytes. Defaults to 4.
        verbose (bool, optional): If True, provides detailed commentary on how the tiling is decided. Defaults to False.
    
    Returns:
        list of dict: A sequence of tiles, where each tile is represented by a dictionary
                      containing the sub-operation sizes.

    Example:

    memory_limit_bytes = 4 * 1024  # 32 KB
    ra, ca, cb = 64, 16, 32   # Example matmul dimensions
    tiles = generate_tiling_sequence(memory_limit_bytes, ra, ca, cb, verbose=True)
    print("Generated Tiles Sequence:")
    for idx, tile in enumerate(tiles, 1):
        print(f"Tile {idx}: A({tile['tile_ra']} x {tile['tile_ca']}) B({tile['tile_ca']} x {tile['tile_cb']}) C({tile['tile_ra']} x {tile['tile_cb']})")

    
    """
    
    # Define list to hold the sequence of tiles to be processed
    tiles_sequence = []

    if verbose:
        print("=== Generating Tiling Sequence ===")
        print(f"Input Size: A({ra} x {ca}) B({ca} x {cb})")
        print(f"Memory Limit: {memory_limit_bytes / 1024:.2f} KB\n")

    # Start tiling by splitting `ra` and `cb` into the largest possible sizes
    remaining_ra = ra
    remaining_cb = cb

    while remaining_ra > 0:
        # Determine the largest possible tile size for `tile_ra`
        tile_ra = remaining_ra

        while tile_ra > 1:
            # Determine the size of tile A and output C
            size_a = tile_ra * ca * bytes_per_element
            size_c = tile_ra * cb * bytes_per_element

            # Ensure this is the best fit based on the memory limit
            if size_a + size_c <= memory_limit_bytes:
                break
            tile_ra -= 1

        remaining_cb = cb
        while remaining_cb > 0:
            # Determine the largest possible tile size for `tile_cb`
            tile_cb = remaining_cb

            while tile_cb > 1:
                # Calculate memory requirements for tiles
                size_a = tile_ra * ca * bytes_per_element
                size_b = ca * tile_cb * bytes_per_element
                size_c = tile_ra * tile_cb * bytes_per_element

                if size_a + size_b + size_c <= memory_limit_bytes:
                    break
                tile_cb -= 1

            # Add the current tile to the sequence
            tiles_sequence.append({
                'tile_ra': tile_ra,
                'tile_ca': ca,
                'tile_cb': tile_cb
            })

            if verbose:
                print(f"Tile Added: A({tile_ra} x {ca}) B({ca} x {tile_cb}) C({tile_ra} x {tile_cb})")
                print(f"Memory Required: {((tile_ra * ca + ca * tile_cb + tile_ra * tile_cb) * bytes_per_element) / 1024:.2f} KB")
                print(f"Remaining Rows (ra): {remaining_ra}, Remaining Columns (cb): {remaining_cb}\n")

            # Move to the next set of columns
            remaining_cb -= tile_cb

        # Move to the next set of rows
        remaining_ra -= tile_ra

    if verbose:
        print("=== Tiling Sequence Generation Completed ===\n")
    
    return tiles_sequence

def generate_tiling_sequence_for_pe(memory_limit_bytes, ra, ca, cb, bytes_per_element=4, verbose=False):
    """
    Generates a tiling sequence for a single PE with given memory constraints and assigned cb.

    Example:
    
    memory_limit_bytes = 100 * 1024  # 32 KB
    ra, ca, cb = 32, 1024, 32   # Example matmul dimensions
    tiles = generate_tiling_sequence_for_pe(memory_limit_bytes, ra, ca, cb, verbose=False)
    print("Generated Tiles Sequence:")
    for idx, tile in enumerate(tiles, 1):
        print(f"Tile {idx}: A({tile['tile_ra']} x {tile['tile_ca']}) B({tile['tile_ca']} x {tile['tile_cb']}) C({tile['tile_ra']} x {tile['tile_cb']})")
    """
    tiles_sequence = []

    if verbose:
        print("=== Generating Tiling Sequence ===")
        print(f"Input Size: A({ra} x {ca}) B({ca} x {cb})")
        print(f"Memory Limit: {memory_limit_bytes / 1024:.2f} KB\n")

    remaining_ra = ra

    while remaining_ra > 0:
        # Determine the largest possible tile size for `tile_ra`
        tile_ra = remaining_ra

        while tile_ra > 0:
            size_a = tile_ra * ca * bytes_per_element
            size_c = tile_ra * cb * bytes_per_element
            if size_a + size_c <= memory_limit_bytes:
                break
            tile_ra -= 1

        remaining_ca = ca  # Assuming full ca is used

        # Add the current tile to the sequence
        tiles_sequence.append({
            'tile_ra': tile_ra,
            'tile_ca': ca,
            'tile_cb': cb
        })

        if verbose:
            size_a = tile_ra * ca * bytes_per_element
            size_b = ca * cb * bytes_per_element
            size_c = tile_ra * cb * bytes_per_element
            total_memory = size_a + size_b + size_c
            print(f"Tile Added: A({tile_ra} x {ca}) B({ca} x {cb}) C({tile_ra} x {cb})")
            print(f"Memory Required: {total_memory / 1024:.2f} KB")
            print(f"Remaining Rows (ra): {remaining_ra}\n")

        # Move to the next set of rows
        remaining_ra -= tile_ra

    if verbose:
        print("=== Tiling Sequence Generation Completed ===\n")
        
    return tiles_sequence

def generate_tiling_sequence_multiple_pes(pe_configs, ra, ca, cb, bytes_per_element=4, verbose=False):
    """
    Generates tiling sequences for multiple PEs with different memory constraints,
    by dynamically splitting the workload along either `ra` or `cb`.

    Args:
        pe_configs (list of dict): A list where each dict contains:
            - 'id': Unique identifier for the PE.
            - 'name': Name of the PE.
            - 'memory_limit_bytes': Memory limit of the PE in bytes.
        ra (int): Number of rows in matrix A.
        ca (int): Number of columns in matrix A (also rows in matrix B).
        cb (int): Number of columns in matrix B.
        bytes_per_element (int, optional): Size of each matrix element in bytes. Defaults to 4.
        verbose (bool, optional): If True, provides detailed commentary. Defaults to False.
    
    Returns:
        List of batches, where each batch is a list of tiles assigned to PEs.

    # Example usage with PEs having different memory constraints
    pe_configs = [
        {'id': 0, 'name': 'cgra', 'memory_limit_bytes': 100 * 1024},  # PE 0 with 100 KB
        {'id': 1, 'name': 'carus', 'memory_limit_bytes': 20 * 1024},   # PE 1 with 20 KB
    ]

    ra, ca, cb = 32, 32, 1024  # Matmul dimensions
    batches = generate_tiling_sequence_multiple_pes(pe_configs, ra, ca, cb, verbose=False)

    # Printing the generated batches
    for batch_idx, batch in enumerate(batches, 1):
        print(f"\nBatch {batch_idx}:")
        for tile in batch:
            pe_name = tile['pe_name']
            print(f"  PE {pe_name} executes tile: A({tile['tile_ra']} x {tile['tile_ca']}) "
                f"B({tile['tile_ca']} x {tile['tile_cb']}) "
                f"C({tile['tile_ra']} x {tile['tile_cb']}) "
                f"at position ({tile['start_row']}, {tile['start_col']})")

    """
    # Determine which dimension to split based on size
    split_dimension = 'cb' if cb > ra else 'ra'
    if verbose:
        print(f"Splitting along dimension: {split_dimension}\n")
    
    # Step 1: Split the chosen dimension proportionally among PEs
    total_memory = sum(pe['memory_limit_bytes'] for pe in pe_configs)
    assigned_parts = {}
    
    if split_dimension == 'cb':
        # Split along `cb`
        total_assigned_cb = 0
        for pe in pe_configs:
            fraction = pe['memory_limit_bytes'] / total_memory
            assigned_columns = int(round(fraction * cb))
            total_assigned_cb += assigned_columns
            assigned_parts[pe['id']] = {'assigned_cb': assigned_columns, 'name': pe['name']}
    
        # Adjust if total assigned columns do not sum up to `cb`
        if total_assigned_cb != cb:
            difference = cb - total_assigned_cb
            # Adjust the first PE's assigned columns
            first_pe_id = pe_configs[0]['id']
            assigned_parts[first_pe_id]['assigned_cb'] += difference
    
    elif split_dimension == 'ra':
        # Split along `ra`
        total_assigned_ra = 0
        for pe in pe_configs:
            fraction = pe['memory_limit_bytes'] / total_memory
            assigned_rows = int(round(fraction * ra))
            total_assigned_ra += assigned_rows
            assigned_parts[pe['id']] = {'assigned_ra': assigned_rows, 'name': pe['name']}
    
        # Adjust if total assigned rows do not sum up to `ra`
        if total_assigned_ra != ra:
            difference = ra - total_assigned_ra
            # Adjust the first PE's assigned rows
            first_pe_id = pe_configs[0]['id']
            assigned_parts[first_pe_id]['assigned_ra'] += difference
    
    # Step 2: Generate tiling sequences for each PE
    pe_tiling_sequences = {}
    offset = 0  # Keep track of offset for each PE in the chosen dimension
    
    max_num_tiles = 0  # To keep track of the maximum number of tiles among all PEs
    
    for pe in pe_configs:
        pe_id = pe['id']
        memory_limit_bytes = pe['memory_limit_bytes']
        pe_name = assigned_parts[pe_id]['name']
    
        if split_dimension == 'cb':
            pe_cb = assigned_parts[pe_id]['assigned_cb']
            if pe_cb <= 0:
                continue
    
            tiles_sequence = generate_tiling_sequence_for_pe(memory_limit_bytes, ra, ca, pe_cb, bytes_per_element, verbose=verbose)
                
            # Adjust tile_cb to reflect the offset in cb
            for tile in tiles_sequence:
                tile['start_row'] = 0
                tile['start_col'] = offset
                tile['start_depth'] = 0
                tile['accelerator_id'] = pe_id
                tile['pe_name'] = pe_name
            offset += pe_cb  # Update offset for next PE
    
        elif split_dimension == 'ra':
            pe_ra = assigned_parts[pe_id]['assigned_ra']
            if pe_ra <= 0:
                continue
    
            tiles_sequence = generate_tiling_sequence_for_pe(memory_limit_bytes, pe_ra, ca, cb, bytes_per_element, verbose=verbose)
                
            # Adjust tile_ra to reflect the offset in ra
            for tile in tiles_sequence:
                tile['start_row'] = offset
                tile['start_col'] = 0
                tile['start_depth'] = 0
                tile['accelerator_id'] = pe_id
                tile['pe_name'] = pe_name
            offset += pe_ra  # Update offset for next PE
    
        pe_tiling_sequences[pe_id] = tiles_sequence
        if len(tiles_sequence) > max_num_tiles:
            max_num_tiles = len(tiles_sequence)
    
    # Step 3: Generate batches similar to the old function
    batches = []
    for i in range(max_num_tiles):
        batch = []
        for pe_id, tiles in pe_tiling_sequences.items():
            if i < len(tiles):
                batch.append(tiles[i])
        batches.append(batch)
    
    if verbose:
        print("\nGenerated Batches of Tiles:")
        for batch_idx, batch in enumerate(batches, 1):
            print(f"\nBatch {batch_idx}:")
            for tile in batch:
                pe_name = tile['pe_name']
                print(f"  PE {pe_name} executes tile: A({tile['tile_ra']} x {tile['tile_ca']}) "
                      f"B({tile['tile_ca']} x {tile['tile_cb']}) "
                      f"C({tile['tile_ra']} x {tile['tile_cb']}) "
                      f"at position ({tile['start_row']}, {tile['start_col']})")
    
    return batches


# =================================================================================================
# Unsecussful attempt to generate tiling sequence for multiple PEs with different memory constraints
# =================================================================================================

def generate_tiling_sequence_multiple_pes_dynamic_split(pe_constraints, ra, ca, cb, bytes_per_element=4, verbose=False):
    # This is back-up code for working version
    """
    Generates tiling sequences for multiple PEs with different memory constraints,
    by dynamically splitting the workload along either `ra` or `cb`.
    """
    # Determine which dimension to split based on size
    split_dimension = 'cb' if cb > ra else 'ra'
    if verbose:
        print(f"Splitting along dimension: {split_dimension}\n")

    # Step 1: Split the chosen dimension proportionally among PEs
    total_memory = sum(pe['memory_limit_bytes'] for pe in pe_constraints)
    assigned_parts = {}

    if split_dimension == 'cb':
        # Split along `cb`
        total_assigned_cb = 0
        for pe in pe_constraints:
            fraction = pe['memory_limit_bytes'] / total_memory
            assigned_columns = int(round(fraction * cb))
            total_assigned_cb += assigned_columns
            assigned_parts[pe['id']] = {'assigned_cb': assigned_columns}

        # Adjust if total assigned columns do not sum up to `cb`
        if total_assigned_cb != cb:
            difference = cb - total_assigned_cb
            # Adjust the first PE's assigned columns
            first_pe_id = pe_constraints[0]['id']
            assigned_parts[first_pe_id]['assigned_cb'] += difference

    elif split_dimension == 'ra':
        # Split along `ra`
        total_assigned_ra = 0
        for pe in pe_constraints:
            fraction = pe['memory_limit_bytes'] / total_memory
            assigned_rows = int(round(fraction * ra))
            total_assigned_ra += assigned_rows
            assigned_parts[pe['id']] = {'assigned_ra': assigned_rows}

        # Adjust if total assigned rows do not sum up to `ra`
        if total_assigned_ra != ra:
            difference = ra - total_assigned_ra
            # Adjust the first PE's assigned rows
            first_pe_id = pe_constraints[0]['id']
            assigned_parts[first_pe_id]['assigned_ra'] += difference

    # Step 2: Generate tiling sequences for each PE
    pe_tiling_sequences = {}
    offset = 0  # Keep track of offset for each PE in the chosen dimension
    for pe in pe_constraints:
        pe_id = pe['id']
        memory_limit_bytes = pe['memory_limit_bytes']

        if split_dimension == 'cb':
            pe_cb = assigned_parts[pe_id]['assigned_cb']
            if pe_cb <= 0:
                continue

            if verbose:
                print(f"Generating tiling sequence for PE {pe_id} with memory limit {memory_limit_bytes / 1024} KB and assigned columns {pe_cb}")

            tiles_sequence = generate_tiling_sequence_for_pe(memory_limit_bytes, ra, ca, pe_cb, bytes_per_element, verbose=verbose)
            
            # Adjust tile_cb to reflect the offset in cb
            for tile in tiles_sequence:
                tile['cb_offset'] = offset  # Starting index in cb
                tile['cb_range'] = (offset, offset + pe_cb)  # Range in cb
            offset += pe_cb  # Update offset for next PE

        elif split_dimension == 'ra':
            pe_ra = assigned_parts[pe_id]['assigned_ra']
            if pe_ra <= 0:
                continue

            if verbose:
                print(f"Generating tiling sequence for PE {pe_id} with memory limit {memory_limit_bytes / 1024} KB and assigned rows {pe_ra}")

            tiles_sequence = generate_tiling_sequence_for_pe(memory_limit_bytes, pe_ra, ca, cb, bytes_per_element, verbose=verbose)
            
            # Adjust tile_ra to reflect the offset in ra
            for tile in tiles_sequence:
                tile['ra_offset'] = offset  # Starting index in ra
                tile['ra_range'] = (offset, offset + pe_ra)  # Range in ra
            offset += pe_ra  # Update offset for next PE

        pe_tiling_sequences[pe_id] = tiles_sequence

    return pe_tiling_sequences

def generate_tiling_sequence_multiple_pes_dynamic_split(pe_constraints, ra, ca, cb, bytes_per_element=4, verbose=False):
    """
    Generates tiling sequences for multiple PEs with different memory constraints,
    by dynamically splitting the workload along either `ra` or `cb`.
    """
    # Determine which dimension to split based on size
    split_dimension = 'cb' if cb > ra else 'ra'
    if verbose:
        print(f"Splitting along dimension: {split_dimension}\n")

    # Step 1: Split the chosen dimension proportionally among PEs
    total_memory = sum(pe['memory_limit_bytes'] for pe in pe_constraints)
    assigned_parts = {}

    if split_dimension == 'cb':
        # Split along `cb`
        total_assigned_cb = 0
        for pe in pe_constraints:
            fraction = pe['memory_limit_bytes'] / total_memory
            assigned_columns = int(round(fraction * cb))
            total_assigned_cb += assigned_columns
            assigned_parts[pe['id']] = {'assigned_cb': assigned_columns}

        # Adjust if total assigned columns do not sum up to `cb`
        if total_assigned_cb != cb:
            difference = cb - total_assigned_cb
            # Adjust the first PE's assigned columns
            first_pe_id = pe_constraints[0]['id']
            assigned_parts[first_pe_id]['assigned_cb'] += difference

    elif split_dimension == 'ra':
        # Split along `ra`
        total_assigned_ra = 0
        for pe in pe_constraints:
            fraction = pe['memory_limit_bytes'] / total_memory
            assigned_rows = int(round(fraction * ra))
            total_assigned_ra += assigned_rows
            assigned_parts[pe['id']] = {'assigned_ra': assigned_rows}

        # Adjust if total assigned rows do not sum up to `ra`
        if total_assigned_ra != ra:
            difference = ra - total_assigned_ra
            # Adjust the first PE's assigned rows
            first_pe_id = pe_constraints[0]['id']
            assigned_parts[first_pe_id]['assigned_ra'] += difference

    # Step 2: Generate tiling sequences for each PE
    pe_tiling_sequences = {}
    offset = 0  # Keep track of offset for each PE in the chosen dimension
    for pe in pe_constraints:
        pe_id = pe['id']
        memory_limit_bytes = pe['memory_limit_bytes']

        if split_dimension == 'cb':
            pe_cb = assigned_parts[pe_id]['assigned_cb']
            if pe_cb <= 0:
                continue

            if verbose:
                print(f"Generating tiling sequence for PE {pe_id} with memory limit {memory_limit_bytes / 1024} KB and assigned columns {pe_cb}")

            tiles_sequence = generate_tiling_sequence_for_pe(memory_limit_bytes, ra, ca, pe_cb, bytes_per_element, verbose=verbose)
            
            # Adjust tile_cb to reflect the offset in cb
            for tile in tiles_sequence:
                tile['cb_offset'] = offset  # Starting index in cb
                tile['cb_range'] = (offset, offset + pe_cb)  # Range in cb
            offset += pe_cb  # Update offset for next PE

        elif split_dimension == 'ra':
            pe_ra = assigned_parts[pe_id]['assigned_ra']
            if pe_ra <= 0:
                continue

            if verbose:
                print(f"Generating tiling sequence for PE {pe_id} with memory limit {memory_limit_bytes / 1024} KB and assigned rows {pe_ra}")

            tiles_sequence = generate_tiling_sequence_for_pe(memory_limit_bytes, pe_ra, ca, cb, bytes_per_element, verbose=verbose)
            
            # Adjust tile_ra to reflect the offset in ra
            for tile in tiles_sequence:
                tile['ra_offset'] = offset  # Starting index in ra
                tile['ra_range'] = (offset, offset + pe_ra)  # Range in ra
            offset += pe_ra  # Update offset for next PE

        pe_tiling_sequences[pe_id] = tiles_sequence

    return pe_tiling_sequences

def generate_tiling_sequence_parallel(
    ra, ca, cb,
    accelerator_configs,
    bytes_per_element=4,
    verbose=False
):
    """
    Generates tiling sequences for matrix multiplication for each accelerator based on performance and memory constraints.

    Args:
        ra (int): Number of rows in matrix A.
        ca (int): Number of columns in matrix A (also rows in matrix B).
        cb (int): Number of columns in matrix B.
        accelerator_configs (list of dict): A list where each dict contains:
            - 'memory_limit_bytes': Memory limit of the accelerator in bytes.
            - 'performance': Relative performance metric of the accelerator.
        bytes_per_element (int, optional): Size of each matrix element in bytes. Defaults to 4.
        verbose (bool, optional): If True, provides detailed commentary. Defaults to False.

    Returns:
        dict: A mapping from accelerator IDs to their assigned tiles or a single workload for one accelerator.
    """
    # Pre-check: Can the entire workload fit on the prioritized accelerator?
    prioritized_accelerator = max(accelerator_configs, key=lambda x: x['performance'])
    total_memory_required = (
        ra * ca + ca * cb + ra * cb
    ) * bytes_per_element  # A + B + C

    if total_memory_required <= prioritized_accelerator['memory_limit_bytes']:
        if verbose:
            print("The entire workload fits on the prioritized accelerator. No tiling or batching required.")
        return {accelerator_configs.index(prioritized_accelerator): [{
            'start_row': 0,
            'start_col': 0,
            'start_depth': 0,
            'tile_ra': ra,
            'tile_ca': ca,
            'tile_cb': cb
        }]}

    # Step 1: Calculate total weights for performance and memory
    total_performance = sum(acc['performance'] for acc in accelerator_configs)
    total_memory = sum(acc['memory_limit_bytes'] for acc in accelerator_configs)

    total_weight = 0
    for acc in accelerator_configs:
        acc['performance_weight'] = acc['performance'] / total_performance
        acc['memory_weight'] = acc['memory_limit_bytes'] / total_memory
        acc['total_weight'] = (acc['performance_weight'] + acc['memory_weight']) / 2
        total_weight += acc['total_weight']

    # Step 2: Assign workload fractions
    for acc in accelerator_configs:
        acc['workload_fraction'] = acc['total_weight'] / total_weight
        if verbose:
            print(f"Accelerator {accelerator_configs.index(acc)}:")
            print(f"  Performance Weight: {acc['performance_weight']:.2f}")
            print(f"  Memory Weight: {acc['memory_weight']:.2f}")
            print(f"  Total Weight: {acc['total_weight']:.2f}")
            print(f"  Workload Fraction: {acc['workload_fraction']:.2f}\n")

    # Step 3: Split workload among accelerators
    current_row = 0
    accelerators_tiles = {}
    total_rows_assigned = 0

    sorted_accelerators = sorted(accelerator_configs, key=lambda x: x['performance'], reverse=True)
    prioritized_accelerator_id = accelerator_configs.index(prioritized_accelerator)

    for acc in sorted_accelerators:
        acc_id = accelerator_configs.index(acc)
        accelerators_tiles[acc_id] = []

        assigned_rows = int(ra * acc['workload_fraction'])
        if acc is accelerator_configs[-1]:
            assigned_rows = ra - total_rows_assigned

        total_rows_assigned += assigned_rows

        if verbose:
            print(f"Accelerator {acc_id} assigned rows {current_row} to {current_row + assigned_rows - 1}")

        max_tile_ra, max_tile_ca, max_tile_cb = get_max_tile_sizes(
            assigned_rows, ca, cb, acc['memory_limit_bytes'], bytes_per_element
        )

        tiles = []
        for i in range(current_row, current_row + assigned_rows, max_tile_ra):
            tile_ra = min(max_tile_ra, current_row + assigned_rows - i)
            for j in range(0, cb, max_tile_cb):
                tile_cb = min(max_tile_cb, cb - j)
                for k in range(0, ca, max_tile_ca):
                    tile_ca = min(max_tile_ca, ca - k)
                    tile = {
                        'start_row': i,
                        'start_col': j,
                        'start_depth': k,
                        'tile_ra': tile_ra,
                        'tile_ca': tile_ca,
                        'tile_cb': tile_cb,
                        'accelerator_id': acc_id
                    }
                    tiles.append(tile)
        accelerators_tiles[acc_id] = tiles
        current_row += assigned_rows

    # Step 4: Adjust tiles for other accelerators based on prioritized one
    num_tiles_prioritized = len(accelerators_tiles[prioritized_accelerator_id])
    for acc in sorted_accelerators[1:]:
        acc_id = accelerator_configs.index(acc)
        tiles = accelerators_tiles[acc_id]
        if len(tiles) > num_tiles_prioritized:
            factor = len(tiles) / num_tiles_prioritized
            max_tile_ra, max_tile_ca, max_tile_cb = adjust_tile_sizes(
                acc['memory_limit_bytes'], bytes_per_element, factor,
                tiles[0]['tile_ra'], tiles[0]['tile_ca'], tiles[0]['tile_cb'],
                ra, ca, cb
            )
            new_tiles = []
            assigned_rows = sum(tile['tile_ra'] for tile in tiles)
            current_row = tiles[0]['start_row']
            for i in range(current_row, current_row + assigned_rows, max_tile_ra):
                tile_ra = min(max_tile_ra, current_row + assigned_rows - i)
                for j in range(0, cb, max_tile_cb):
                    tile_cb = min(max_tile_cb, cb - j)
                    for k in range(0, ca, max_tile_ca):
                        tile_ca = min(max_tile_ca, ca - k)
                        tile = {
                            'start_row': i,
                            'start_col': j,
                            'start_depth': k,
                            'tile_ra': tile_ra,
                            'tile_ca': tile_ca,
                            'tile_cb': tile_cb,
                            'accelerator_id': acc_id
                        }
                        new_tiles.append(tile)
            accelerators_tiles[acc_id] = new_tiles

    # Generate batches
    batches = []
    max_batch_size = max(len(tiles) for tiles in accelerators_tiles.values())
    for i in range(max_batch_size):
        batch = []
        for acc_id, tiles in accelerators_tiles.items():
            if i < len(tiles):
                batch.append(tiles[i])
        batches.append(batch)

    return batches

def get_max_tile_sizes(ra, ca, cb, memory_limit_bytes, bytes_per_element):
    max_tile_ra = ra
    max_tile_ca = ca
    max_tile_cb = cb
    for tile_ra in range(ra, 0, -1):
        for tile_ca in range(ca, 0, -1):
            for tile_cb in range(cb, 0, -1):
                size_a = tile_ra * tile_ca * bytes_per_element
                size_b = tile_ca * tile_cb * bytes_per_element
                size_c = tile_ra * tile_cb * bytes_per_element
                total_size = size_a + size_b + size_c
                if total_size <= memory_limit_bytes:
                    return tile_ra, tile_ca, tile_cb
    return 1, 1, 1

def adjust_tile_sizes(memory_limit_bytes, bytes_per_element, factor, tile_ra, tile_ca, tile_cb, ra, ca, cb):
    new_tile_ra = min(int(tile_ra * factor), ra)
    new_tile_ca = min(int(tile_ca * factor), ca)
    new_tile_cb = min(int(tile_cb * factor), cb)
    size_a = new_tile_ra * new_tile_ca * bytes_per_element
    size_b = new_tile_ca * new_tile_cb * bytes_per_element
    size_c = new_tile_ra * new_tile_cb * bytes_per_element
    total_size = size_a + size_b + size_c
    while total_size > memory_limit_bytes and new_tile_ra > 1 and new_tile_ca > 1 and new_tile_cb > 1:
        new_tile_ra = max(1, new_tile_ra // 2)
        new_tile_ca = max(1, new_tile_ca // 2)
        new_tile_cb = max(1, new_tile_cb // 2)
        size_a = new_tile_ra * new_tile_ca * bytes_per_element
        size_b = new_tile_ca * new_tile_cb * bytes_per_element
        size_c = new_tile_ra * new_tile_cb * bytes_per_element
        total_size = size_a + size_b + size_c
    return new_tile_ra, new_tile_ca, new_tile_cb

def generate_tiling_sequence_parallel_when_i_want_seperately(
    ra, ca, cb,
    accelerator_configs,
    bytes_per_element=4,
    verbose=False
):
    """
    Generates a tiling sequence for matrix multiplication suitable for parallel processing across multiple accelerators with varying memory constraints and performance.

    Args:
        ra (int): Number of rows in matrix A.
        ca (int): Number of columns in matrix A (also rows in matrix B).
        cb (int): Number of columns in matrix B.
        accelerator_configs (list of dict): A list where each dict contains 'memory_limit_bytes' and 'performance' keys for an accelerator.
        bytes_per_element (int, optional): Size of each matrix element in bytes. Defaults to 4.
        verbose (bool, optional): If True, provides detailed commentary. Defaults to False.

    Returns:
        dict: A mapping from accelerator IDs to their assigned tiles.
    """
    # Calculate total performance
    total_performance = sum(acc['performance'] for acc in accelerator_configs)

    # Initialize accelerators
    for idx, acc in enumerate(accelerator_configs):
        acc['id'] = idx
        acc['workload_fraction'] = acc['performance'] / total_performance
        # Determine maximum tile sizes for this accelerator
        acc['max_tile_ra'], acc['max_tile_cb'] = get_max_tile_sizes_when_i_want_seperately(
            ra, ca, cb, acc['memory_limit_bytes'], bytes_per_element
        )
        if verbose:
            print(f"Accelerator {idx}:")
            print(f"  Memory Limit: {acc['memory_limit_bytes'] / 1024:.2f} KB")
            print(f"  Performance: {acc['performance']}")
            print(f"  Workload Fraction: {acc['workload_fraction']:.2f}")
            print(f"  Max Tile Sizes: tile_ra={acc['max_tile_ra']}, tile_cb={acc['max_tile_cb']}\n")

    # Partition the matrix among accelerators
    # For simplicity, we partition the rows of matrix A among accelerators
    current_row = 0
    accelerators_tiles = {}
    for acc in accelerator_configs:
        acc_id = acc['id']
        accelerators_tiles[acc_id] = []
        # Calculate the number of rows this accelerator should process
        acc_rows = int(ra * acc['workload_fraction'])
        if acc is accelerator_configs[-1]:
            # Ensure the last accelerator gets any remaining rows
            acc_rows = ra - current_row
        if verbose:
            print(f"Accelerator {acc_id} assigned rows {current_row} to {current_row + acc_rows - 1}")

        # Generate tiles for this accelerator
        max_tile_ra = acc['max_tile_ra']
        max_tile_cb = acc['max_tile_cb']
        acc_tiles = []
        for i in range(current_row, current_row + acc_rows, max_tile_ra):
            tile_ra = min(max_tile_ra, current_row + acc_rows - i)
            for j in range(0, cb, max_tile_cb):
                tile_cb = min(max_tile_cb, cb - j)
                tile = {
                    'start_row': i,
                    'start_col': j,
                    'tile_ra': tile_ra,
                    'tile_ca': ca,
                    'tile_cb': tile_cb,
                    'accelerator_id': acc_id
                }
                acc_tiles.append(tile)
                if verbose:
                    print(f"Accelerator {acc_id} assigned tile:")
                    print(f"  Rows {i}-{i + tile_ra - 1}, Cols {j}-{j + tile_cb - 1}")
                    print(f"  Tile Sizes: A({tile_ra} x {ca}), B({ca} x {tile_cb}), C({tile_ra} x {tile_cb})")
        accelerators_tiles[acc_id] = acc_tiles
        current_row += acc_rows

    return accelerators_tiles

def get_max_tile_sizes_when_i_want_seperately(ra, ca, cb, memory_limit_bytes, bytes_per_element):
    max_tile_ra = ra
    max_tile_cb = cb

    # Find max_tile_ra
    for tile_ra_candidate in range(ra, 0, -1):
        size_a = tile_ra_candidate * ca * bytes_per_element
        size_c = tile_ra_candidate * cb * bytes_per_element
        if size_a + size_c <= memory_limit_bytes:
            max_tile_ra = tile_ra_candidate
            break

    # Find max_tile_cb
    for tile_cb_candidate in range(cb, 0, -1):
        size_b = ca * tile_cb_candidate * bytes_per_element
        size_c = max_tile_ra * tile_cb_candidate * bytes_per_element
        if size_b + size_c <= memory_limit_bytes:
            max_tile_cb = tile_cb_candidate
            break

    return max_tile_ra, max_tile_cb

# =================================================================================================