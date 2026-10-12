#pragma once

#include <stdint.h>

extern "C" {
    void draw (uint32_t *BitmapMem, int width, int height);
    typedef void (*draw_fn)(uint32_t *pixels, int width, int height);
}

