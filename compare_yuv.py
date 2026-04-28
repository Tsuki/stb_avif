#!/usr/bin/env python3
import struct

def read_16bit_raw(path, width, height):
    with open(path, 'rb') as f:
        data = f.read()
    arr = []
    idx = 0
    for row in range(height):
        row_data = []
        for col in range(width):
            val = struct.unpack_from('<H', data, idx)[0]
            row_data.append(val)
            idx += 2
        arr.append(row_data)
    return arr

def read_yuv420(path, y_w, y_h, uv_w, uv_h):
    with open(path, 'rb') as f:
        data = f.read()
    y_size = y_w * y_h
    uv_size = uv_w * uv_h
    
    y = []
    idx = 0
    for row in range(y_h):
        y.append(list(data[idx:idx+y_w]))
        idx += y_w
    
    u = []
    for row in range(uv_h):
        u.append(list(data[idx:idx+uv_w]))
        idx += uv_w
    
    v = []
    for row in range(uv_h):
        v.append(list(data[idx:idx+uv_w]))
        idx += uv_w
    
    return y, u, v

def show_neighborhood(arr, row, col, label, width=4):
    h = len(arr)
    w = len(arr[0])
    print(f"\n{label} neighborhood around ({row}, {col}):")
    for r in range(max(0, row-1), min(h, row+width-1)):
        vals = []
        for c in range(max(0, col-1), min(w, col+width-1)):
            vals.append(f"{arr[r][c]:4d}")
        print("  " + " ".join(vals))

def compare_plane(ours_16bit, ff_8bit, name, threshold=1):
    h = len(ours_16bit)
    w = len(ours_16bit[0])
    
    diff = [[0]*w for _ in range(h)]
    total_diff = 0
    max_diff = 0
    match_count = 0
    total_pixels = h * w
    
    first_mismatch = None
    
    for row in range(h):
        for col in range(w):
            ours_val = ours_16bit[row][col] & 0xFF
            ff_val = ff_8bit[row][col]
            d = abs(ours_val - ff_val)
            diff[row][col] = d
            total_diff += d
            if d > max_diff:
                max_diff = d
            if d <= threshold:
                match_count += 1
            
            if first_mismatch is None and d > threshold:
                first_mismatch = (row, col, ours_val, ff_val, d)
    
    mean_diff = total_diff / total_pixels
    match_pct = (match_count / total_pixels) * 100
    
    print(f"\n{'='*60}")
    print(f"{name} Plane Comparison")
    print(f"{'='*60}")
    print(f"Dimensions: {h}x{w}")
    print(f"Mean difference: {mean_diff:.3f}")
    print(f"Max difference: {max_diff}")
    print(f"Pixels within threshold (≤{threshold}): {match_pct:.2f}%")
    
    if first_mismatch:
        row, col, ours_val, ff_val, d = first_mismatch
        print(f"First mismatch at: ({row}, {col})")
        print(f"  Our value: {ours_val}")
        print(f"  FFmpeg value: {ff_val}")
        print(f"  Difference: {d}")
        
        show_neighborhood([[v & 0xFF for v in r] for r in ours_16bit], row, col, "Our decoder")
        show_neighborhood(ff_8bit, row, col, "FFmpeg")
        show_neighborhood(diff, row, col, "Difference")
        
        return row, col, diff
    else:
        print("All pixels match within threshold!")
        return None, None, diff

def analyze_patterns(diff, name):
    print(f"\n{name} difference pattern analysis:")
    h = len(diff)
    w = len(diff[0])
    
    even_count = 0
    even_sum = 0
    odd_count = 0
    odd_sum = 0
    
    for row in range(h):
        for col in range(w):
            if row % 2 == 0 and col % 2 == 0:
                even_sum += diff[row][col]
                even_count += 1
            elif row % 2 == 1 and col % 2 == 1:
                odd_sum += diff[row][col]
                odd_count += 1
    
    even_mean = even_sum / even_count if even_count > 0 else 0
    odd_mean = odd_sum / odd_count if odd_count > 0 else 0
    print(f"  Even-position (0,0) pixels mean diff: {even_mean:.3f}")
    print(f"  Odd-position (1,1) pixels mean diff: {odd_mean:.3f}")
    
    if h > 4 and w > 4:
        edge_sum = 0
        edge_count = 0
        interior_sum = 0
        interior_count = 0
        
        for row in range(h):
            for col in range(w):
                is_edge = (row < 2 or row >= h-2 or col < 2 or col >= w-2)
                if is_edge:
                    edge_sum += diff[row][col]
                    edge_count += 1
                else:
                    interior_sum += diff[row][col]
                    interior_count += 1
        
        edge_mean = edge_sum / edge_count if edge_count > 0 else 0
        interior_mean = interior_sum / interior_count if interior_count > 0 else 0
        print(f"  Edge pixels mean diff: {edge_mean:.3f}")
        print(f"  Interior pixels mean diff: {interior_mean:.3f}")
    
    if h >= 8 and w >= 8:
        block_diffs = []
        for by in range(0, h, 8):
            for bx in range(0, w, 8):
                block_sum = 0
                block_count = 0
                for r in range(by, min(by+8, h)):
                    for c in range(bx, min(bx+8, w)):
                        block_sum += diff[r][c]
                        block_count += 1
                block_diffs.append(block_sum / block_count if block_count > 0 else 0)
        
        print(f"  Mean 8x8 block difference: {sum(block_diffs)/len(block_diffs):.3f}")
        print(f"  Max 8x8 block difference: {max(block_diffs):.3f}")

def analyze_y_divergence(our_y, ff_y):
    """Special analysis for Y plane to find where divergence starts"""
    h = len(our_y)
    w = len(our_y[0])
    
    print("\nY plane divergence analysis:")
    print("-" * 40)
    
    # Check first few rows for exact matches
    match_streak = 0
    for row in range(min(10, h)):
        row_matches = 0
        row_diffs = []
        for col in range(w):
            ours = our_y[row][col] & 0xFF
            ff = ff_y[row][col]
            if ours == ff:
                row_matches += 1
            else:
                row_diffs.append((col, ours, ff, abs(ours-ff)))
        
        if row_diffs:
            print(f"Row {row}: {row_matches}/{w} exact matches")
            print(f"  First few differences: {row_diffs[:5]}")
        else:
            match_streak += 1
            print(f"Row {row}: All {w} pixels match exactly!")
    
    if match_streak >= 10:
        print("  First 10 rows all match. Divergence starts later.")

print("Loading files...")

our_y = read_16bit_raw('/tmp/our_y_prefilter.raw', 1204, 800)
our_u = read_16bit_raw('/tmp/our_u.raw', 602, 400)
our_v = read_16bit_raw('/tmp/our_v.raw', 602, 400)

ff_y, ff_u, ff_v = read_yuv420('/tmp/ff_yuv420.yuv', 1204, 800, 602, 400)

print(f"Our Y shape: 800x1204 (16-bit)")
print(f"Our U shape: 400x602 (16-bit)")
print(f"Our V shape: 400x602 (16-bit)")
print(f"FFmpeg Y shape: 800x1204 (8-bit)")
print(f"FFmpeg U shape: 400x602 (8-bit)")
print(f"FFmpeg V shape: 400x602 (8-bit)")

analyze_y_divergence(our_y, ff_y)

y_row, y_col, y_diff = compare_plane(our_y, ff_y, "Y", threshold=1)
u_row, u_col, u_diff = compare_plane(our_u, ff_u, "U", threshold=1)
v_row, v_col, v_diff = compare_plane(our_v, ff_v, "V", threshold=1)

if y_row is not None:
    analyze_patterns(y_diff, "Y")
if u_row is not None:
    analyze_patterns(u_diff, "U")
if v_row is not None:
    analyze_patterns(v_diff, "V")

print("\n" + "="*60)
print("SUMMARY")
print("="*60)
print(f"Y plane first mismatch: {'None' if y_row is None else f'({y_row}, {y_col})'}")
print(f"U plane first mismatch: {'None' if u_row is None else f'({u_row}, {u_col})'}")
print(f"V plane first mismatch: {'None' if v_row is None else f'({v_row}, {v_col})'}")

# Also show prefilter comparison briefly
print("\n" + "="*60)
print("PREFILTER Y COMPARISON (for reference)")
print("="*60)
our_y_prefilter = read_16bit_raw('/tmp/our_y_prefilter.raw', 1204, 800)
# Just check first few pixels
print("First 10 pixels comparison:")
print(f"{'Pixel':<8} {'Prefilter':<12} {'Postfilter':<12} {'FFmpeg':<10}")
for i in range(10):
    row, col = 0, i
    pre = our_y_prefilter[row][col] & 0xFF
    post = our_y[row][col] & 0xFF
    ff = ff_y[row][col]
    print(f"({row},{col})  {pre:<12} {post:<12} {ff:<10}")
