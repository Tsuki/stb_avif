#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s width height\n", argv[0]);
        return 1;
    }
    int w = atoi(argv[1]);
    int h = atoi(argv[2]);
    size_t y_count = (size_t)w * h;
    
    uint16_t *our_y = malloc(y_count * 2);
    uint8_t *ref_y8 = malloc(y_count);
    if (!our_y || !ref_y8) { perror("malloc"); return 1; }
    
    FILE *f = fopen("/tmp/our_y.raw", "rb");
    if (!f) { perror("our_y.raw"); return 1; }
    fread(our_y, 2, y_count, f);
    fclose(f);
    
    f = fopen("/tmp/fox_ref_yuv.raw", "rb");
    if (!f) { perror("fox_ref_yuv.raw"); return 1; }
    fread(ref_y8, 1, y_count, f);
    fclose(f);
    
    double mae = 0;
    int max_diff = 0;
    int at_128 = 0;
    int at_init = 0;
    int diff_count = 0;
    
    for (size_t i = 0; i < y_count; i++) {
        int our = our_y[i];
        int ref = ref_y8[i]; // 8-bit values stored as 0-255 in uint16_t
        int diff = abs(our - ref);
        mae += diff;
        if (diff > max_diff) max_diff = diff;
        if (our == 128) at_128++;
        if (our == 128 * 257) at_init++;
        if (diff > 10) diff_count++;
    }
    mae /= y_count;
    
    printf("Y MAE: %.2f\n", mae);
    printf("Max diff: %d\n", max_diff);
    printf("Our Y[0]: %d\n", our_y[0]);
    printf("Ref Y[0]: %d\n", ref_y8[0]);
    printf("Pixels exactly 128: %d (%.2f%%)\n", at_128, 100.0 * at_128 / y_count);
    printf("Pixels exactly 128*257: %d (%.2f%%)\n", at_init, 100.0 * at_init / y_count);
    printf("Pixels with diff > 10: %d (%.2f%%)\n", diff_count, 100.0 * diff_count / y_count);
    
    free(our_y);
    free(ref_y8);
    return 0;
}
