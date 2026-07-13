#!/usr/bin/env bash
# mayhem/build.sh — build nanort's fuzz harnesses + standalone reproducers + the upstream test suite.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

COMMON="$SRC/examples/common"
NANOSG="$SRC/examples/nanosg"

# ---------------------------------------------------------------------------
# 1) Fuzz harnesses (instrumented with $SANITIZER_FLAGS + $DEBUG_FLAGS).
# ---------------------------------------------------------------------------

# vnormalize / BVH harness — header-only nanort, so the "project" is compiled straight into the harness.
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE -std=c++11 \
    "$SRC/mayhem/fuzz_vnormalize.cpp" -I"$SRC" \
    -o /mayhem/fuzz_vnormalize

# nanosg input-parsing harness — drives render-config JSON + obj-loader (.obj) parsing in-process.
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE -std=c++11 -DNANOSG_USE_CXX11 \
    "$SRC/mayhem/fuzz_nanosg.cpp" \
    "$NANOSG/obj-loader.cc" "$NANOSG/render-config.cc" \
    -I"$SRC" -I"$NANOSG" -I"$COMMON" \
    -o /mayhem/fuzz_nanosg

# ---------------------------------------------------------------------------
# 2) Standalone (non-fuzzer) reproducers — LLVM's run-once driver, natural crash.
# ---------------------------------------------------------------------------
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o /tmp/standalone_main.o

$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -std=c++11 \
    "$SRC/mayhem/fuzz_vnormalize.cpp" /tmp/standalone_main.o -I"$SRC" \
    -o /mayhem/fuzz_vnormalize-standalone

$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -std=c++11 -DNANOSG_USE_CXX11 \
    "$SRC/mayhem/fuzz_nanosg.cpp" \
    "$NANOSG/obj-loader.cc" "$NANOSG/render-config.cc" /tmp/standalone_main.o \
    -I"$SRC" -I"$NANOSG" -I"$COMMON" \
    -o /mayhem/fuzz_nanosg-standalone

# ---------------------------------------------------------------------------
# 3) Upstream test suite — the regression reproducer under test/, built with NORMAL flags
#    (a clean, uninstrumented build) so mayhem/test.sh only RUNS it.
# ---------------------------------------------------------------------------
$CXX -O0 -std=c++11 $COVERAGE_FLAGS \
    "$SRC/test/regression/possible-accuracy-problem-30/main.cc" -I"$SRC" \
    -o /mayhem/nanort_regression_test

# Authored known-answer oracle (upstream ships no assertion suite) — normal flags too.
$CXX -O0 -std=c++11 -DNANOSG_USE_CXX11 $COVERAGE_FLAGS \
    "$SRC/mayhem/selftest.cc" \
    "$NANOSG/obj-loader.cc" "$NANOSG/render-config.cc" \
    -I"$SRC" -I"$NANOSG" -I"$COMMON" \
    -o /mayhem/nanort_selftest

echo "build.sh complete"
