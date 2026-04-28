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
    FILE *fp;
    size_t sz;
    int w, h, ch;
    unsigned char *rgba;
    
    if (argc < 3) {
        fprintf(stderr, "Usage: %s input.avif output.yuv\n", argv[0]);
        return 1;
    }
    
    /* We need to hook into the decoder internals to get YUV.
     * For now, let's just decode to RGB and convert back to YUV
     * using the inverse of the color matrix. This is approximate. */
    rgba = stbi_avif_load(argv[1], &w, &h, &ch, 0);
    if (!rgba) {
        fprintf(stderr, "Failed to load %s\n", argv[1]);
        return 1;
    }
    
    /* Approximate YUV conversion for BT.2020 limited range */
    fp = fopen(argv[2], "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open %s\n", argv[2]);
        stbi_avif_image_free(rgba);
        return 1;
    }
    
    /* This is just an approximation - not useful for accurate comparison */
    /* Better approach: modify stb_avif.h to dump YUV during decode */
    
    fclose(fp);
    stbi_avif_image_free(rgba);
    return 0;
}
