#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: %s width height threshold\n", argv[0]);
        fprintf(stderr, "  Compares /tmp/our_y.raw (16-bit LE) with /tmp/ref_y8.raw (8-bit)\n");
        fprintf(stderr, "  Reports first pixel where diff > threshold\n");
        return 1;
    }
    int w = atoi(argv[1]);
    int h = atoi(argv[2]);
    int threshold = atoi(argv[3]);
    size_t count = (size_t)w * h;
    
    uint16_t *our_y = malloc(count * 2);
    uint8_t *ref_y8 = malloc(count);
    if (!our_y || !ref_y8) { perror("malloc"); return 1; }
    
    FILE *f = fopen("/tmp/our_y.raw", "rb");
    if (!f) { perror("our_y.raw"); return 1; }
    fread(our_y, 2, count, f);
    fclose(f);
    
    f = fopen("/tmp/fox_ref_yuv.raw", "rb");
    if (!f) { perror("fox_ref_yuv.raw"); return 1; }
    fread(ref_y8, 1, count, f);
    fclose(f);
    
    int diff_count = 0;
    int first_diff_x = -1, first_diff_y = -1;
    int max_diff = 0;
    int max_x = -1, max_y = -1;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            size_t i = (size_t)y * w + x;
            int our = our_y[i];
            int ref = ref_y8[i];
            int diff = abs(our - ref);
            
            if (diff > max_diff) {
                max_diff = diff;
                max_x = x;
                max_y = y;
            }
            
            if (diff > threshold) {
                diff_count++;
                if (first_diff_x < 0) {
                    first_diff_x = x;
                    first_diff_y = y;
                }
            }
        }
    }
    
    printf("Total pixels: %zu\n", count);
    printf("Pixels with diff > %d: %d (%.2f%%)\n", threshold, diff_count, 100.0 * diff_count / count);
    printf("First pixel with diff > %d: (%d, %d)  our=%d ref=%d\n",
           threshold, first_diff_x, first_diff_y,
           first_diff_x >= 0 ? our_y[(size_t)first_diff_y * w + first_diff_x] : 0,
           first_diff_x >= 0 ? ref_y8[(size_t)first_diff_y * w + first_diff_x] : 0);
    printf("Max diff: %d at (%d, %d)  our=%d ref=%d\n",
           max_diff, max_x, max_y,
           our_y[(size_t)max_y * w + max_x],
           ref_y8[(size_t)max_y * w + max_x]);
    
    // Show histogram of diff magnitudes
    int hist[6] = {0};
    for (size_t i = 0; i < count; i++) {
        int diff = abs((int)our_y[i] - (int)ref_y8[i]);
        if (diff <= 5) hist[0]++;
        else if (diff <= 20) hist[1]++;
        else if (diff <= 50) hist[2]++;
        else if (diff <= 100) hist[3]++;
        else if (diff <= 200) hist[4]++;
        else hist[5]++;
    }
    printf("\nDiff histogram (our_y vs ref_y8):\n");
    printf("  0-5:   %d (%.1f%%)\n", hist[0], 100.0 * hist[0] / count);
    printf("  6-20:  %d (%.1f%%)\n", hist[1], 100.0 * hist[1] / count);
    printf("  21-50: %d (%.1f%%)\n", hist[2], 100.0 * hist[2] / count);
    printf("  51-100:%d (%.1f%%)\n", hist[3], 100.0 * hist[3] / count);
    printf("  101-200:%d (%.1f%%)\n", hist[4], 100.0 * hist[4] / count);
    printf("  >200:   %d (%.1f%%)\n", hist[5], 100.0 * hist[5] / count);
    
    free(our_y);
    free(ref_y8);
    return 0;
}
