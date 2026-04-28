#define STB_AVIF_IMPLEMENTATION
#include "../stb_avif.h"
#include <stdio.h>
#include <stdlib.h>

/* Dump raw YUV planes before RGB conversion */
int main(int argc, char **argv) {
    stbi_avif__av1_decode_ctx ctx;
    stbi_avif__av1_seq_header seq;
    stbi_avif__av1_frame_header fhdr;
    stbi_avif__av1_planes planes;
    unsigned char *data;
    int w, h, ch;
    int i;
    FILE *fy, *fu, *fv;
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s input.avif\n", argv[0]);
        return 1;
    }
    
    /* Use internal decode to get planes */
    data = stbi_avif_load(argv[1], &w, &h, &ch, 0);
    if (!data) {
        fprintf(stderr, "Failed to load %s\n", argv[1]);
        return 1;
    }
    
    /* Since stbi_avif_load returns RGB, we need to decode again to get YUV
     * Let's just check the RGB values to see if Y is the issue */
    printf("Image: %dx%d channels=%d\n", w, h, ch);
    printf("First pixel RGB: %d %d %d\n", data[0], data[1], data[2]);
    printf("Pixel at (100,100): %d %d %d\n", data[(100*w+100)*ch], data[(100*w+100)*ch+1], data[(100*w+100)*ch+2]);
    
    stbi_avif_image_free(data);
    return 0;
}
