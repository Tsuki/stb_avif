#define STB_AVIF_IMPLEMENTATION
#include "../stb_avif.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int w, h, ch, i;
    unsigned char *data;
    FILE *fp;
    
    if (argc < 3) {
        fprintf(stderr, "Usage: %s input.avif output.ppm\n", argv[0]);
        return 1;
    }
    
    data = stbi_avif_load(argv[1], &w, &h, &ch, 3);
    if (!data) {
        fprintf(stderr, "Failed to load %s\n", argv[1]);
        return 1;
    }
    
    fp = fopen(argv[2], "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open %s\n", argv[2]);
        stbi_avif_image_free(data);
        return 1;
    }
    
    fprintf(fp, "P6\n%d %d\n255\n", w, h);
    fwrite(data, 1, w * h * 3, fp);
    fclose(fp);
    
    printf("Converted %s -> %s (%dx%d RGB)\n", argv[1], argv[2], w, h);
    
    stbi_avif_image_free(data);
    return 0;
}
