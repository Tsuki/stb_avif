#define STB_AVIF_IMPLEMENTATION
#include "../stb_avif.h"
#include <stdio.h>

int main(int argc, char **argv) {
    int w, h, ch;
    if (argc < 2) return 1;
    stbi_avif_info(argv[1], &w, &h, &ch);
    return 0;
}
