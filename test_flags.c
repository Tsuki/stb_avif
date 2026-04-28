#define STB_AVIF_IMPLEMENTATION
#include "stb_avif.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <avif_file>\n", argv[0]);
        return 1;
    }
    
    int w, h, ch;
    stbi_avif_info(argv[1], &w, &h, &ch);
    
    /* The info function parses the sequence header and first frame header.
     * We need to hook into the decode to print frame header flags.
     * Since we can't easily do that, let's just decode and capture stderr.
     */
    unsigned char *data = stbi_avif_load(argv[1], &w, &h, &ch, 0);
    if (data) stbi_image_free(data);
    
    return 0;
}
