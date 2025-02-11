def parse_size(size_str):
    # Remove extra non-digit parts (e.g. "_f7", "_size2_stride1")
    base = size_str.split("_")[0]
    dims = base.split("x")
    try:
        total = 1
        for d in dims:
            total *= int(d)
        return total
    except ValueError:
        raise ValueError(f"Cannot parse dimensions from '{size_str}'")

def linear_interpolate(x0, y0, x1, y1, x):
    return y0 + (y1 - y0) * ((x - x0) / (x1 - x0))

def get_cycles(kernel, size, pe, datatype, timing_data):
    # Check existence of group
    if kernel not in timing_data or pe not in timing_data[kernel] or datatype not in timing_data[kernel][pe]:
        raise ValueError(f"Timing data missing for kernel '{kernel}' with PE '{pe}' and datatype '{datatype}'")
    records = timing_data[kernel][pe][datatype]
    if size in records:
        return records[size], False  # exact match found, not estimated

    req_size = parse_size(size)
    avail = []
    for key in records:
        try:
            val = parse_size(key)
            avail.append((val, records[key]))
        except ValueError:
            continue
    if not avail:
        raise ValueError(f"No valid sizes available for kernel '{kernel}', PE '{pe}', datatype '{datatype}'")
    avail.sort(key=lambda t: t[0])
    if req_size <= avail[0][0]:
        return avail[0][1], True
    if req_size >= avail[-1][0]:
        return avail[-1][1], True
    for i in range(len(avail) - 1):
        if avail[i][0] <= req_size <= avail[i+1][0]:
            interp_cycles = linear_interpolate(avail[i][0], avail[i][1], avail[i+1][0], avail[i+1][1], req_size)
            return interp_cycles, True
    return avail[-1][1], True
