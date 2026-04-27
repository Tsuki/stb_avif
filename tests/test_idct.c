#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_AVIF_IMPLEMENTATION
#include "../stb_avif.h"

int test_1d_idct_dc_scaling() {
    int input[64], output[64];
    int sizes[] = {4, 8, 16, 32, 64};
    const char *names[] = {"IDCT4", "IDCT8", "IDCT16", "IDCT32", "IDCT64"};
    int i, failures = 0;
    
    printf("=== 1D IDCT DC scaling test ===\n");
    for (i = 0; i < 5; i++) {
        int sz = sizes[i];
        memset(input, 0, sizeof(input));
        input[0] = 1024;
        
        switch(sz) {
            case 4: stbi_avif__av1_idct4(input, output); break;
            case 8: stbi_avif__av1_idct8(input, output); break;
            case 16: stbi_avif__av1_idct16(input, output); break;
            case 32: stbi_avif__av1_idct32(input, output); break;
            case 64: stbi_avif__av1_idct64(input, output); break;
        }
        
        double ratio = (double)output[0] / 1024.0;
        printf("%s DC=1024: output[0]=%d (ratio=%.4f)\n", names[i], output[0], ratio);
        
        // All 1D transforms should have ~0.707 DC scaling
        if (ratio < 0.70 || ratio > 0.72) {
            printf("  [FAIL] Expected ratio ~0.707\n");
            failures++;
        } else {
            printf("  [PASS]\n");
        }
    }
    return failures;
}

int test_2d_idct_dc_scaling() {
    int coeffs[64 * 64];
    int tx_sizes[] = {4, 8, 16, 32};
    const char *names[] = {"4x4", "8x8", "16x16", "32x32"};
    int i, j, failures = 0;
    
    printf("\n=== 2D IDCT DC scaling test ===\n");
    for (i = 0; i < 4; i++) {
        int sz = tx_sizes[i];
        /* AV1 transform convention: output is 2x orthonormal DCT.
         * Total scaling = txw * txh / 2, so DC output = input * 2 / (txw * txh) */
        int expected = (1024 * 2) / (sz * sz);
        int all_flat = 1;
        
        memset(coeffs, 0, sizeof(coeffs));
        coeffs[0] = 1024;
        stbi_avif__av1_inverse_transform_2d_rect(coeffs, sz, sz, 0);
        
        for (j = 1; j < sz * sz; j++) {
            if (coeffs[j] != coeffs[0]) all_flat = 0;
        }
        
        printf("%s DC=1024: output[0]=%d, expected=%d, flat=%s", 
               names[i], coeffs[0], expected, all_flat ? "YES" : "NO");
        
        if (coeffs[0] != expected) {
            printf("  [FAIL]\n");
            failures++;
        } else {
            printf("  [PASS]\n");
        }
    }
    return failures;
}

int main() {
    int failures = 0;
    
    failures += test_1d_idct_dc_scaling();
    failures += test_2d_idct_dc_scaling();
    
    printf("\n=== Summary: %d failures ===\n", failures);
    return failures > 0 ? 1 : 0;
}
