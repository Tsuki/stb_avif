#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main() {
    int w = 1204, h = 800;
    size_t count = (size_t)w * h;
    uint16_t *buf = malloc(count * 2);
    FILE *f = fopen("/tmp/our_y.raw", "rb");
    fread(buf, 2, count, f);
    fclose(f);
    
    int at_999 = 0;
    for (size_t i = 0; i < count; i++)
        if (buf[i] == 999) at_999++;
    
    printf("Pixels at 999: %d (%.2f%%)\n", at_999, 100.0 * at_999 / count);
    free(buf);
    return 0;
}
