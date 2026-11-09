// raylib's data utilities (rcore.c): DEFLATE (sdefl/sinfl), Base64, CRC32/MD5/SHA1/SHA256.
// The first input byte selects the operation and the rest is its payload, so every byte string reaches
// the untrusted decoders (DEFLATE, Base64) and no input is filtered out.
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "raylib.h"

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    SetTraceLogLevel(LOG_NONE);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *input, size_t inputSize)
{
    // The API takes int sizes (raylib.h); one byte is the operation selector
    if (inputSize < 1 || inputSize - 1 > INT_MAX) return -1;
    const uint8_t *data = input + 1;
    size_t size = inputSize - 1;
    int outSize = 0;

    switch (input[0] % 4)
    {
        case 0: // Untrusted DEFLATE stream
        {
            unsigned char *dec = DecompressData(data, (int)size, &outSize);
            if (dec != NULL) MemFree(dec);
        } break;
        case 1: // Untrusted Base64 text
        {
            char *text = (char *)malloc(size + 1);
            memcpy(text, data, size);
            text[size] = '\0';
            unsigned char *b64 = DecodeDataBase64(text, &outSize);
            if (b64 != NULL) MemFree(b64);
            free(text);
        } break;
        case 2: // Checksums
        {
            (void)ComputeCRC32(data, (int)size);
            (void)ComputeMD5(data, (int)size);
            (void)ComputeSHA1(data, (int)size);
            (void)ComputeSHA256(data, (int)size);
        } break;
        default: // Round trips: compress/decompress and encode/decode must return the input
        {
            if (size == 0) break;
            int compSize = 0;
            unsigned char *comp = CompressData(data, (int)size, &compSize);
            if (comp != NULL)
            {
                unsigned char *back = DecompressData(comp, compSize, &outSize);
                if (back == NULL || (size_t)outSize != size || memcmp(back, data, size) != 0) abort();
                MemFree(back);
                MemFree(comp);
            }

            char *enc = EncodeDataBase64(data, (int)size, &outSize);
            if (enc != NULL)
            {
                unsigned char *back = DecodeDataBase64(enc, &outSize);
                if (back == NULL || (size_t)outSize != size || memcmp(back, data, size) != 0) abort();
                MemFree(back);
                MemFree(enc);
            }
        } break;
    }
    return 0;
}
