# AVIF Decoder Debug TODO

## Completed
- [x] Fixed critical EOB off-by-one bug (removed `eob += 1` in `stbi_avif__av1_read_coeffs_after_skip`)
- [x] Verified no uninitialized pixels remain (0% at sentinel value)
- [x] Confirmed decode completes without crashes on all 8 test images

## Current Status
- **Fox 8-bit YUV420**: Y MAE = 71.45, 95% pixels differ by >10
- **Red 10-bit YUV420**: Y MAE = 354, 51% pixels at 512 (DC_128)
- RGB MAE vs ffmpeg: ~0.34 (34% of full scale)

## Next Steps (Priority Order)

### Option 1: Block-Level Differential Debugging [IN PROGRESS]
**Goal**: Identify exactly which blocks diverge and trace their decode path.

**Approach**:
1. Create instrumentation to dump per-block state from our decoder:
   - Block position (mi_row, mi_col)
   - Block size, partition type
   - Y mode, UV mode, skip flag
   - TX size, TX type
   - EOB value, coefficient count
   - Pre-residual prediction value (DC or intra-pred)
   - Post-reconstruction value
   - Dequantized coefficient values (first few)

2. Extract matching per-block data from ffmpeg/libdav1d:
   - Use ffmpeg with debug flags OR
   - Build minimal dav1d decoder with block dump hooks
   - Alternatively: use aomdec with --verbose

3. Compare block-by-block:
   - First identify if prediction matches (without residuals)
   - Then check if coefficients match
   - Then check if dequantization matches
   - Then check if inverse transform output matches
   - Finally check reconstruction (pred + residual)

4. Trace decode path for first divergent block:
   - Add detailed logging around the specific block
   - Compare bitstream symbols read
   - Compare context values (skip_ctx, txb_skip_ctx, coeff_ctx)
   - Step through each decode stage

**Expected Outcome**: Pinpoint which decode stage introduces the first error.

---

### Option 2: Inverse Transform Audit
**Goal**: Verify IDCT/IADST/Identity transform accuracy against reference.

**Approach**:
1. Create test harness that feeds known coefficient inputs to transforms
2. Compare output against dav1d reference implementation
3. Check for rounding/scaling errors at each stage:
   - Row transform output (after row_shift)
   - Column transform output (after >>4)
   - rect2 scaling factor (x181+128>>8)
   - Final clip to pixel range

4. Verify transform selection matches for each tx_type
5. Check 32x32 and 64x64 transform handling (only top-left 32x32 coeffs)

**Expected Outcome**: Fix any transform inaccuracies that contribute to error.

---

### Option 3: Prediction Audit
**Goal**: Fix known prediction issues from plan.md.

**Approach**:
1. **V_PRED / H_PRED**: Remove spurious gradient terms (plan.md §3.1)
   - V_PRED should be pure `top[x]` copy
   - H_PRED should be pure `left[y]` copy
   
2. **Angular modes (D45-D67)**: Implement proper Z1/Z2/Z3 predictors
   - Add angle lookup: `mode_to_angle[8] = {90, 180, 45, 135, 113, 157, 203, 67}`
   - Apply angle_delta: `actual_angle = nominal + delta * 3`
   - Implement bilinear interpolation with `dr_intra_derivative` table
   
3. **DC prediction**: Fix rounding for non-square blocks
   - Replace `sum / count` with `(sum + (count >> 1)) >> log2(count)` for square blocks
   - Add multiplier tables for non-square ratios
   
4. **Extended reference pixels**: Load up to `2 * max(bw, bh)` pixels
   - Compute `have_above_right` and `have_below_left`
   - Repeat last available pixel when extended refs unavailable

**Expected Outcome**: Significant MAE reduction (plan.md estimates 27.9 → ~6).

---

## Debug Options (Current Crisis)

### Option 1: Symbol-Level Compare [RECOMMENDED - START HERE]
**Goal**: Trace every arithmetic-decoded symbol against aomdec/dav1d to find first divergence.

**Approach**:
1. Add instrumentation to log every `stbi_avif__av1_read_symbol_adapt` call with:
   - Symbol name (partition, skip, y_mode, txb_skip, coeff_base, etc.)
   - Context values (part_ctx, skip_ctx, txb_skip_ctx, coeff_ctx)
   - CDF state (pre/post update)
   - Decoded symbol value
   - Bit position in stream

2. Run aomdec with `--verbose` or patch dav1d to dump matching symbols
   - Focus on 64x64 synthetic test first (simplest case)
   - Align symbol streams by SB/block order

3. Compare symbol-by-symbol:
   - First divergence = root cause
   - Common failure points: partition context, skip context, txb_skip CDF selection

**Expected Outcome**: Identify exactly which symbol decode diverges and why.

---

### Option 2: Prediction Audit
**Goal**: Verify DC_PRED/V_PRED/H_PRED for 10-bit and edge cases.

**Approach**:
1. Check DC_PRED for 4x4, 8x8, 16x16, 32x32, 64x64, 128x128 blocks
2. Verify DC_128 constant matches bit depth (128 for 8-bit, 512 for 10-bit)
3. Check V_PRED/H_PRED don't have spurious gradient terms
4. Verify left/above reference pixel loading for edge blocks

**Expected Outcome**: Fix prediction if it's the root cause.

---

### Option 3: Clean Build & Baseline
**Goal**: Remove debug instrumentation, verify fixes aren't masked.

**Approach**:
1. Remove all fprintf debug statements
2. Recompile clean
3. Re-run all tests
4. Compare against pre-fix baseline

**Expected Outcome**: Confirm fixes work without debug side effects.

---

### Option 4: Focus on 8-bit Fox
**Goal**: Since 8-bit non-lossless might have different root cause than 10-bit synthetic.

**Approach**:
1. Compare fox decode path (8-bit, non-lossless) vs 10-bit synthetic
2. Check if partition fix improved fox at all
3. Trace fox symbol stream to find divergence

**Expected Outcome**: Determine if fixes are working for 8-bit but not 10-bit.

---

## Notes
- The EOB fix was critical but not sufficient — errors are systematic across entire image
- Prediction bugs (plan.md §3.1-3.4) are likely the largest contributor
- Coefficient decode may have secondary bugs (context computation, sign reading)
- Transform accuracy is probably fine but needs verification
- **CRITICAL**: Both partition context AND lossless transform size fixes applied, but 64x64/128x128 tests still show Y~128 (ours) vs Y~64 (ref). Root cause unknown.

## Detailed Findings

### Partition Context Fix
- **Issue**: `bsl = bsize_log2w - 1` was off by one
- **Fix**: Changed to `bsl = bsize_log2w - 2` (matching AV1 spec)
- **Result**: 128x128 blocks now correctly split recursively
- **Status**: Completed

### 64x64 Inverse Transform Bug
- **Issue**: `stbi_avif__av1_idct64` produces all zeros for any input
- **Evidence**: Instrumentation shows `big[0]=-30` before transform → `big[0]=0` after
- **Impact**: All 64x64 transform blocks reconstruct as pure prediction (zero residual)
- **Status**: In progress

### Coefficient Decode Issues
- **Issue**: Coefficients may be incorrectly dequantized or positioned
- **Evidence**: 64x64 test case shows only 5 non-zero coefficients but output is zero
- **Status**: To investigate after transform fix

### Prediction Issues
- **Issue**: V_PRED and H_PRED may have spurious gradient terms
- **Issue**: DC prediction rounding may be incorrect for non-square blocks
- **Status**: To investigate after coefficient decode is verified
