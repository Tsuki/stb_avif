#define STB_AVIF_IMPLEMENTATION
#include "../stb_avif.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int w, h, ch;
    unsigned char *data;
    int x, y;
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s input.avif\n", argv[0]);
        return 1;
    }
    
    data = stbi_avif_load(argv[1], &w, &h, &ch, 0);
    if (!data) {
        fprintf(stderr, "Failed to load %s\n", argv[1]);
        return 1;
    }
    
    printf("Image: %dx%d channels=%d\n", w, h, ch);
    printf("Sample pixels (RGB):\n");
    for (y = 0; y < h; y += h/4) {
        for (x = 0; x < w; x += w/4) {
            int idx = (y * w + x) * ch;
            printf("  (%4d,%4d): %3d %3d %3d\n", x, y, data[idx], data[idx+1], data[idx+2]);
        }
    }
    
    stbi_avif_image_free(data);
    return 0;
}
