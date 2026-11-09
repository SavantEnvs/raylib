// Model loaders behind LoadModel / LoadModelAnimations / LoadMaterials (tinyobj, cgltf, IQM, VOX, M3D).
// raylib loads models by file name, so each input is written to a private scratch directory under
// $TMPDIR and removed again before the next one. Runs on the software renderer (PLATFORM_MEMORY).
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "raylib.h"


// raylib dispatches on the file extension; pick it from the content's magic as a file name would.
// Text that matches none is offered as OBJ (tinyobj), and "{" as glTF JSON.
static const char *pick_ext(const uint8_t *d, size_t n)
{
    if (n >= 16 && memcmp(d, "INTERQUAKEMODEL\0", 16) == 0) return ".iqm";
    if (n >= 4 && memcmp(d, "glTF", 4) == 0) return ".glb";
    if (n >= 4 && memcmp(d, "VOX ", 4) == 0) return ".vox";
    if (n >= 4 && memcmp(d, "3DMO", 4) == 0) return ".m3d";
    if (d[0] == '{') return ".gltf";
    return ".obj";
}
static char gDir[512];

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    const char *tmp = getenv("TMPDIR");
    if (tmp == NULL || tmp[0] == '\0') tmp = "/tmp";
    snprintf(gDir, sizeof(gDir), "%s/fuzz_model.XXXXXX", tmp);
    if (mkdtemp(gDir) == NULL) abort();

    SetTraceLogLevel(LOG_NONE);
    InitWindow(64, 64, "fuzz_model");
    return 0;
}

static void load_all(const char *path, const char *ext)
{
    Model model = LoadModel(path);
    if (IsModelValid(model)) (void)GetModelBoundingBox(model);
    UnloadModel(model);

    int count = 0;
    ModelAnimation *anims = LoadModelAnimations(path, &count);
    if (anims != NULL) UnloadModelAnimations(anims, count);

    if (strcmp(ext, ".obj") == 0 || strcmp(ext, ".gltf") == 0 || strcmp(ext, ".glb") == 0)
    {
        int matCount = 0;
        Material *mats = LoadMaterials(path, &matCount);
        if (mats != NULL)
        {
            for (int m = 0; m < matCount; m++) UnloadMaterial(mats[m]);
            MemFree(mats);
        }
    }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0 || size > INT_MAX) return -1;

    const char *ext = pick_ext(data, size);
    char path[640];
    snprintf(path, sizeof(path), "%s/model%s", gDir, ext);
    FILE *f = fopen(path, "wb");
    if (f == NULL) abort();
    if (fwrite(data, 1, size, f) != size) abort();
    fclose(f);

    load_all(path, ext);
    unlink(path);
    return 0;
}
