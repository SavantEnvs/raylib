#!/usr/bin/env bash
# mayhem/test.sh — checks raylib against known answers: decoded pixels/samples/vertices, string and path
# helpers, hashes, Base64 and DEFLATE round trips, via the mayhem/kat.c probe built by build.sh.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}


# Run the known-answer probe (built by mayhem/build.sh from raylib's default configuration, normal
# flags) and require every expected line. A missing probe or empty output fails every check.
PROBE="$SRC/raylib-kat"
EXPECTED="$SRC/mayhem/kat.expected"
[ -x "$PROBE" ] || { echo "missing $PROBE: mayhem/build.sh did not build it" >&2; emit_ctrf "raylib-kat" 0 1 0; exit 1; }

actual="$("$PROBE" 2>/dev/null || true)"
passed=0
failed=0
while IFS= read -r want; do
  if grep -qxF -- "$want" <<<"$actual"; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1))
    echo "FAIL: expected line not produced: $want" >&2
  fi
done < "$EXPECTED"

emit_ctrf "raylib-kat" "$passed" "$failed" 0
