


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