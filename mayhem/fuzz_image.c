// Image decoders reachable through LoadImageFromMemory (stb_image, qoi, dds, ktx, pkm, pvr, astc, pep, ...).
// The first input byte selects the extension (the index into kExts) and the rest is the file, so every byte
// string reaches every decoder and no input is filtered out.
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"

static const char *const kExts[] = {
    ".png", ".bmp", ".gif", ".qoi", ".tga", ".jpg", ".psd", ".hdr", ".pic", ".pnm",
    ".dds", ".ktx", ".astc", ".pkm", ".pep", ".pvr",
};

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    SetTraceLogLevel(LOG_NONE);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *input, size_t inputSize)
{
    // LoadImageFromMemory takes an int dataSize (raylib.h); one byte is the format selector
    if (inputSize < 1 || inputSize - 1 > INT_MAX) return -1;
    const char *ext = kExts[input[0] % (sizeof(kExts)/sizeof(kExts[0]))];
    size_t size = inputSize - 1;

    // Some loaders (PKM) byte-swap the buffer in place; a caller's file buffer is writable
    uint8_t *buf = (uint8_t *)malloc(size + 1);
    memcpy(buf, input + 1, size);

    Image img = LoadImageFromMemory(ext, buf, (int)size);
    if (IsImageValid(img))
    {
        (void)GetImageColor(img, img.width/2, img.height/2);

        if ((int64_t)img.width*img.height <= (1 << 16))
        {
            Image copy = ImageCopy(img);
            if (IsImageValid(copy))
            {
                ImageFormat(&copy, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                ImageFlipVertical(&copy);
            }
            UnloadImage(copy);
        }
    }
    UnloadImage(img);

    if (strcmp(ext, ".gif") == 0)
    {
        int frames = 0;
        memcpy(buf, input + 1, size);
        Image anim = LoadImageAnimFromMemory(ext, buf, (int)size, &frames);
        UnloadImage(anim);
    }
    free(buf);
    return 0;
}
