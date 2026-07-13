#!/usr/bin/env bash
# mayhem/test.sh — RUN nanort's functional tests (built by mayhem/build.sh; nothing compiles here).
#
# Upstream test inventory: nanort ships exactly ONE test program —
# test/regression/possible-accuracy-problem-30/main.cc (a BVH traversal regression reproducer).
# It prints its verdict instead of asserting, so we assert on its OUTPUT here (golden-output
# check). Its "activate precision bug" mode reproduces a KNOWN-OPEN upstream accuracy bug
# (issue #30) and is recorded as skipped. There is no other upstream suite (no ctest/make
# check/gtest), so the authored known-answer oracle /mayhem/nanort_selftest (mayhem/selftest.cc,
# 6 behavioral checks over nanort + the nanosg parsers) supplements it.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
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

PASS=0; FAIL=0; SKIP=0

# --- upstream regression test (golden-output assertion) ---------------------------------
if [ ! -x /mayhem/nanort_regression_test ]; then
  echo "FATAL: /mayhem/nanort_regression_test missing — build.sh did not produce the test runner" >&2
  emit_ctrf "nanort-tests" 0 1 0
  exit 1
fi
out="$(/mayhem/nanort_regression_test 2>&1)" || true
if printf '%s' "$out" | grep -q "We have the expected result" && \
   printf '%s' "$out" | grep -q "Intersection isect.u ="; then
  echo "PASS regression/possible-accuracy-problem-30 (normal case)"
  PASS=$((PASS+1))
else
  echo "FAIL regression/possible-accuracy-problem-30 (normal case)"
  echo "$out"
  FAIL=$((FAIL+1))
fi
# Known-open upstream accuracy bug (lighttransport/nanort#30): the perturbed-direction mode
# still reproduces the miss upstream — recorded as skipped, not failed.
SKIP=$((SKIP+1))

# --- authored known-answer oracle --------------------------------------------------------
if [ ! -x /mayhem/nanort_selftest ]; then
  echo "FATAL: /mayhem/nanort_selftest missing — build.sh did not produce the oracle" >&2
  emit_ctrf "nanort-tests" "$PASS" $((FAIL+1)) "$SKIP"
  exit 1
fi
sout="$(/mayhem/nanort_selftest 2>&1)"; src=$?
echo "$sout"
sp="$(printf '%s\n' "$sout" | grep -c '^PASS ')" || true
sf="$(printf '%s\n' "$sout" | grep -c '^FAIL ')" || true
# 6 known-answer checks expected; a neutered/no-op binary reports 0 PASS lines -> counted failed.
if [ "$src" -ne 0 ] || [ "$sp" -ne 6 ] || [ "$sf" -ne 0 ]; then
  FAIL=$((FAIL + (sf > 0 ? sf : 6 - sp) ))
  PASS=$((PASS + sp))
else
  PASS=$((PASS + 6))
fi

emit_ctrf "nanort-tests" "$PASS" "$FAIL" "$SKIP"
