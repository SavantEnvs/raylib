// raylib's string and path helpers (rtext.c / rcore.c): TextInsert/Replace/Split/Join/Subtext/case
// conversion, UTF-8 codepoint decoding and the file-path helpers, all on NUL-terminated copies of the input.
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    SetTraceLogLevel(LOG_NONE);
    return 0;
}

static char *dup_range(const uint8_t *p, size_t n)
{
    char *s = (char *)malloc(n + 1);
    memcpy(s, p, n);
    s[n] = '\0';
    return s;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 4) return -1;

    // Three strings: A, B, C (thirds of the input); the first byte also parameterises positions
    size_t third = size/3;
    char *a = dup_range(data, third);
    char *b = dup_range(data + third, third);
    char *c = dup_range(data + 2*third, size - 2*third);
    int pos = (int)data[0] - 64;
    int len = (int)data[size - 1] - 64;

    char *r;
    r = TextInsertAlloc(a, b, pos); MemFree(r);
    r = TextInsert(a, b, pos); (void)r;
    r = TextReplaceAlloc(a, b, c); MemFree(r);
    r = TextReplace(a, b, c); (void)r;
    r = TextReplaceBetweenAlloc(a, b, c, a); MemFree(r);
    r = GetTextBetween(a, b, c); (void)r;
    (void)TextSubtext(a, pos, len);
    (void)TextRemoveSpaces(a);
    (void)TextToUpper(a); (void)TextToLower(a); (void)TextToPascal(a);
    (void)TextToSnake(a); (void)TextToCamel(a);
    (void)TextToInteger(a); (void)TextToFloat(a);
    (void)TextFindIndex(a, b);
    (void)TextIsEqual(a, b);
    (void)TextLength(a);

    int n = 0;
    char **parts = TextSplit(a, b[0] ? b[0] : ',', &n);
    if (n > 0) { r = TextJoin(parts, n, c); (void)r; }

    char **lines = LoadTextLines(a, &n);
    if (lines != NULL) UnloadTextLines(lines, n);

    char app[256] = "";
    int apos = 0;
    TextAppend(app, "x", &apos);

    // UTF-8
    int cpSize = 0;
    (void)GetCodepoint(a, &cpSize);
    (void)GetCodepointNext(a, &cpSize);
    (void)GetCodepointCount(a);
    int *cps = LoadCodepoints(a, &n);
    if (cps != NULL)
    {
        char *u = LoadUTF8(cps, n);
        MemFree(u);
        UnloadCodepoints(cps);
    }
    (void)CodepointToUTF8((int)(((unsigned)data[0] << 16) | ((unsigned)data[1] << 8) | data[2]), &cpSize);

    // Paths
    (void)GetFileExtension(a);
    (void)GetFileName(a);
    (void)GetFileNameWithoutExt(a);
    (void)GetDirectoryPath(a);
    (void)GetPrevDirectoryPath(a);
    (void)IsFileExtension(a, b);

    free(a); free(b); free(c);
    return 0;
}
