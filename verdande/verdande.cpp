#include "verdande.h"

extern "C" void draw(uint32_t *BitmapMem, int width, int height) {
    uint32_t num = width * height;
    for (uint32_t i = 0; i < num -1; ++i) {
        if ((i % width) % 128 < (uint32_t)64){
            BitmapMem[i] = 0xFFEEEEEE;
        } else {
            BitmapMem[i] = 0xFFFF0000;
        }
    }
}

