// Audio decoders reachable through LoadWaveFromMemory
// (dr_wav, stb_vorbis, dr_mp3, qoa, dr_flac, jar_xm, jar_mod). No audio device is opened.
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include "raylib.h"

static const char *const kExts[] = { ".wav", ".ogg", ".mp3", ".qoa", ".flac", ".xm", ".mod" };

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    SetTraceLogLevel(LOG_NONE);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // LoadWaveFromMemory takes an int dataSize (raylib.h)
    if (size == 0 || size > INT_MAX) return -1;

    for (size_t i = 0; i < sizeof(kExts)/sizeof(kExts[0]); i++)
    {
        Wave wave = LoadWaveFromMemory(kExts[i], data, (int)size);
        if (IsWaveValid(wave))
        {
            if (wave.frameCount <= (1u << 18))
            {
                float *samples = LoadWaveSamples(wave);
                UnloadWaveSamples(samples);

                Wave copy = WaveCopy(wave);
                WaveCrop(&copy, 0, (int)(copy.frameCount/2));
                WaveFormat(&copy, 22050, 16, 1);
                UnloadWave(copy);
            }
        }
        UnloadWave(wave);
    }
    return 0;
}
