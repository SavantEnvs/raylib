#!/usr/bin/env bash
#
# mayhem/build.sh — build raylib's fuzz harnesses (sanitized, instrumented) and the KAT oracle probe.
# Runs inside the commit image as `mayhem` in /mayhem; re-runnable and needs no network.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

# raylib's software-rendered, windowless backend: no X11/GLFW/GPU, so everything links from libc alone.
PLATFORM_DEFS="-D_GNU_SOURCE -DPLATFORM_MEMORY -DGRAPHICS_API_OPENGL_SOFTWARE"
# The fuzz build also turns on every optional decoder that raylib ships but disables by default.
FORMAT_DEFS=""
for fmt in JPG TGA PSD HDR PIC PNM KTX ASTC PKM PVR PEP BDF FLAC; do
    FORMAT_DEFS="$FORMAT_DEFS -DSUPPORT_FILEFORMAT_$fmt=1"
done
MODULES="rcore rshapes rtextures rtext rmodels raudio"
LIBS="-lm -lpthread -ldl"

FUZZ_DIR="$SRC/mayhem/build-fuzz"
ORACLE_DIR="$SRC/mayhem/build-oracle"
mkdir -p "$FUZZ_DIR" "$ORACLE_DIR"

# 1) The sanitized, SanCov-instrumented library (fuzz build). -fsanitize=fuzzer-no-link goes on the
#    library unconditionally so the fuzzed code carries coverage even with SANITIZER_FLAGS empty.
#    -fno-sanitize=function: raylib passes callbacks through differently typed function pointers.
FUZZ_CFLAGS="-O1 $SANITIZER_FLAGS -fno-sanitize=function -fsanitize=fuzzer-no-link $DEBUG_FLAGS -fno-strict-aliasing"
#    Relaxed UBSan checks, per module, each firing on every valid input of a bundled third-party header:
#    float-cast-overflow (rtextures: HDR floats > 1.0 cast to bytes), pointer-overflow (raudio, rmodels:
#    NULL + 0 offsets in miniaudio's ma_offset_ptr and the glTF/OBJ loaders), alignment (rmodels: vox_loader.h and
#    m3d.h read unaligned u32 from the file).
for m in $MODULES; do
    case "$m" in
        rtextures) RELAX="-fno-sanitize=float-cast-overflow" ;;
        raudio)    RELAX="-fno-sanitize=pointer-overflow" ;;
        rmodels)   RELAX="-fno-sanitize=alignment,pointer-overflow" ;;
        *)         RELAX="" ;;
    esac
    $CC $FUZZ_CFLAGS $RELAX $PLATFORM_DEFS $FORMAT_DEFS -I"$SRC" -Isrc -c "src/$m.c" -o "$FUZZ_DIR/$m.o" &
done
wait
ar rcs "$FUZZ_DIR/libraylib.a" $(for m in $MODULES; do echo "$FUZZ_DIR/$m.o"; done)

# Leak detection off at build time (ASan/UBSan stay on).
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$FUZZ_DIR/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$FUZZ_DIR/standalone_main.o"

# 2) Each harness twice: libFuzzer binary + run-once standalone reproducer.
for h in fuzz_image fuzz_wave fuzz_font fuzz_model fuzz_text fuzz_data; do
    $CC $FUZZ_CFLAGS -Isrc -c "mayhem/$h.c" -o "$FUZZ_DIR/$h.o"
    $CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$FUZZ_DIR/$h.o" "$FUZZ_DIR/lsan_off.o" \
        "$FUZZ_DIR/libraylib.a" $LIBS -o "$SRC/$h"
    $CC $SANITIZER_FLAGS $DEBUG_FLAGS "$FUZZ_DIR/standalone_main.o" "$FUZZ_DIR/$h.o" "$FUZZ_DIR/lsan_off.o" \
        "$FUZZ_DIR/libraylib.a" $LIBS -o "$SRC/$h-standalone"
done

# 3) The oracle: raylib's default configuration with the project's normal flags (no sanitizer, no
#    -g), plus a known-answer probe that mayhem/test.sh runs and checks from bash.
ORACLE_CFLAGS="-O2 $COVERAGE_FLAGS -fno-strict-aliasing"
for m in $MODULES; do
    $CC $ORACLE_CFLAGS $PLATFORM_DEFS -I"$SRC" -Isrc -c "src/$m.c" -o "$ORACLE_DIR/$m.o" &
done
wait
ar rcs "$ORACLE_DIR/libraylib.a" $(for m in $MODULES; do echo "$ORACLE_DIR/$m.o"; done)
$CC $ORACLE_CFLAGS -Isrc mayhem/kat.c "$ORACLE_DIR/libraylib.a" $LIBS $COVERAGE_FLAGS -o "$SRC/raylib-kat"
file "$SRC/raylib-kat" | grep -q 'dynamically linked'
