// Font loaders reachable through LoadFontFromMemory (stb_truetype for .ttf/.otf, BDF).
// Runs on raylib's software renderer (PLATFORM_MEMORY) so no display or GPU is needed.
// The first input byte selects the loader (high bit set: TrueType, clear: BDF) and the rest is the font
// file, so every byte string reaches both loaders and no input is filtered out. stb_truetype documents "NO SECURITY GUARANTEE -- DO NOT USE
// THIS ON UNTRUSTED FONT FILES": its crashes are the library's known, unfixed behavior.
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    SetTraceLogLevel(LOG_NONE);
    InitWindow(64, 64, "fuzz_font");
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *input, size_t inputSize)
{
    // LoadFontFromMemory takes an int dataSize (raylib.h); one byte is the loader selector
    if (inputSize < 1 || inputSize - 1 > INT_MAX) return -1;
    const char *ext = (input[0] & 0x80) ? ".ttf" : ".bdf";
    const uint8_t *data = input + 1;
    size_t size = inputSize - 1;

    // The BDF parser scans for newlines, so it gets a newline-terminated copy
    uint8_t *buf = (uint8_t *)malloc(size + 4);
    memcpy(buf, data, size);
    memset(buf + size, '\n', 3);
    buf[size + 3] = 0;

    Font font = LoadFontFromMemory(ext, buf, (int)size, 16, NULL, 0);
    if (IsFontValid(font))
    {
        (void)MeasureTextEx(font, "Hello, world", 16.0f, 1.0f);
        (void)GetGlyphIndex(font, 'A');
        Image img = ImageTextEx(font, "Ab", 16.0f, 1.0f, WHITE);
        UnloadImage(img);
    }
    UnloadFont(font);
    free(buf);
    return 0;
}
