// Known-answer probe for mayhem/test.sh: runs fixed inputs through raylib's real decoders and helpers
// and prints what came out. test.sh compares each printed line with an exact expected string.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "raylib.h"

static const unsigned char kPng[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xb6, 0x0d,
    0x24, 0x00, 0x00, 0x00, 0x13, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0xf8, 0xcf, 0xc0, 0xf0,
    0x1f, 0x0c, 0x81, 0x34, 0x08, 0x34, 0x00, 0x00, 0x49, 0x49, 0x09, 0x78, 0x28, 0xa0, 0xdb, 0x77,
    0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};

// RIFF/WAVE, PCM 16-bit mono 8000 Hz, samples 0, 16384, -16384, 32767
static const unsigned char kWav[] = {
    'R', 'I', 'F', 'F', 44, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ',
    16, 0, 0, 0, 1, 0, 1, 0, 0x40, 0x1f, 0, 0, 0x80, 0x3e, 0, 0,
    2, 0, 16, 0, 'd', 'a', 't', 'a', 8, 0, 0, 0,
    0x00, 0x00, 0x00, 0x40, 0x00, 0xc0, 0xff, 0x7f,
};

static void kat_image(void)
{
    Image img = LoadImageFromMemory(".png", kPng, (int)sizeof(kPng));
    printf("png.size=%dx%d format=%d\n", img.width, img.height, img.format);
    for (int y = 0; y < img.height; y++)
        for (int x = 0; x < img.width; x++)
        {
            Color c = GetImageColor(img, x, y);
            printf("png.pixel[%d,%d]=%u,%u,%u,%u\n", x, y, c.r, c.g, c.b, c.a);
        }
    ImageFlipVertical(&img);
    Color c = GetImageColor(img, 0, 0);
    printf("png.flipped[0,0]=%u,%u,%u,%u\n", c.r, c.g, c.b, c.a);

    int outSize = 0;
    unsigned char *out = ExportImageToMemory(img, ".png", &outSize);
    Image again = LoadImageFromMemory(".png", out, outSize);
    Color c2 = GetImageColor(again, 1, 0);
    printf("png.roundtrip[1,0]=%u,%u,%u,%u\n", c2.r, c2.g, c2.b, c2.a);
    MemFree(out);
    UnloadImage(again);
    UnloadImage(img);
}

static void kat_wave(void)
{
    Wave w = LoadWaveFromMemory(".wav", kWav, (int)sizeof(kWav));
    printf("wav.frames=%u rate=%u bits=%u channels=%u\n", w.frameCount, w.sampleRate, w.sampleSize, w.channels);
    float *s = LoadWaveSamples(w);
    printf("wav.samples=%.4f,%.4f,%.4f,%.4f\n", s[0], s[1], s[2], s[3]);
    UnloadWaveSamples(s);
    UnloadWave(w);
}

static void kat_model(void)
{
    char dir[] = "/tmp/raylib-kat.XXXXXX";
    if (mkdtemp(dir) == NULL) return;
    char path[128];
    snprintf(path, sizeof(path), "%s/tri.obj", dir);
    FILE *f = fopen(path, "wb");
    fputs("v 0 0 0\nv 1 0 0\nv 0 2 0\nf 1 2 3\n", f);
    fclose(f);

    Model m = LoadModel(path);
    printf("obj.meshes=%d vertices=%d triangles=%d\n", m.meshCount, m.meshes[0].vertexCount, m.meshes[0].triangleCount);
    float *v = m.meshes[0].vertices;
    printf("obj.v1=%.1f,%.1f,%.1f v2=%.1f,%.1f,%.1f\n", v[3], v[4], v[5], v[6], v[7], v[8]);
    BoundingBox bb = GetModelBoundingBox(m);
    printf("obj.bbox.max=%.1f,%.1f,%.1f\n", bb.max.x, bb.max.y, bb.max.z);
    UnloadModel(m);
    unlink(path);
    rmdir(dir);
}

static void kat_text(void)
{
    char *s = TextInsertAlloc("hello", "XY", 2);
    printf("text.insert=%s\n", s);
    MemFree(s);
    s = TextReplaceAlloc("a-b-c", "-", "++");
    printf("text.replace=%s\n", s);
    MemFree(s);
    printf("text.subtext=%s\n", TextSubtext("raylib", 1, 3));
    printf("text.upper=%s\n", TextToUpper("Ray lib"));
    printf("text.snake=%s\n", TextToSnake("helloWorldFoo"));
    printf("text.int=%d float=%.2f\n", TextToInteger("1234"), TextToFloat("-3.25"));
    int n = 0;
    char **parts = TextSplit("one;two;three", ';', &n);
    printf("text.split=%d:%s|%s|%s\n", n, parts[0], parts[1], parts[2]);
    printf("text.find=%d\n", TextFindIndex("raylib", "lib"));
    printf("path.dir=%s\n", GetDirectoryPath("/usr/local/lib/libray.a"));
    printf("path.prev=%s\n", GetPrevDirectoryPath("/usr/local/lib"));
    printf("path.name=%s ext=%s\n", GetFileName("/usr/local/lib/libray.a"), GetFileExtension("/usr/local/lib/libray.a"));
    int cpSize = 0;
    int cp = GetCodepoint("\xe2\x82\xac", &cpSize);
    printf("utf8.codepoint=U+%04X size=%d\n", cp, cpSize);
    const char *enc = CodepointToUTF8(0x1F600, &cpSize);
    printf("utf8.encode=%02x%02x%02x%02x size=%d\n", (unsigned char)enc[0], (unsigned char)enc[1], (unsigned char)enc[2], (unsigned char)enc[3], cpSize);
}

static void kat_data(void)
{
    printf("crc32=%08x\n", ComputeCRC32((const unsigned char *)"hello", 5));
    unsigned int *sha = ComputeSHA256((const unsigned char *)"abc", 3);
    printf("sha256=%08x%08x%08x%08x%08x%08x%08x%08x\n", sha[0], sha[1], sha[2], sha[3], sha[4], sha[5], sha[6], sha[7]);
    unsigned int *md5 = ComputeMD5((const unsigned char *)"abc", 3);
    printf("md5=%08x%08x%08x%08x\n", md5[0], md5[1], md5[2], md5[3]);

    int n = 0;
    unsigned char *dec = DecodeDataBase64("aGVsbG8gd29ybGQ=", &n);
    printf("base64.decode=%.*s len=%d\n", n, dec, n);
    MemFree(dec);
    char *enc = EncodeDataBase64((const unsigned char *)"raylib!", 7, &n);
    printf("base64.encode=%s\n", enc);
    MemFree(enc);

    unsigned char plain[256];
    for (int i = 0; i < 256; i++) plain[i] = (unsigned char)(i % 7);
    int cn = 0;
    unsigned char *comp = CompressData(plain, sizeof(plain), &cn);
    int dn = 0;
    unsigned char *back = DecompressData(comp, cn, &dn);
    printf("deflate.roundtrip=%d len=%d smaller=%d\n", memcmp(plain, back, sizeof(plain)) == 0, dn, cn < (int)sizeof(plain));
    MemFree(comp);
    MemFree(back);
}

int main(void)
{
    SetTraceLogLevel(LOG_NONE);
    InitWindow(64, 64, "raylib-kat");
    kat_image();
    kat_wave();
    kat_model();
    kat_text();
    kat_data();
    fflush(stdout);
    return 0;
}
