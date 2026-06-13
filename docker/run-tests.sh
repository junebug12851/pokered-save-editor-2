#!/usr/bin/env bash
# Pokered Save Editor 2 — in-container build/test driver.
#
# Runs INSIDE the pse-linux-test container. Expects:
#   /host    read-only bind mount of the repo root
#   /build   persistent named volume (ext4) for the source copy + build dirs + ccache
#
# Usage (via the container entrypoint):
#   run-tests.sh [standard|asan|xvfb|coverage|all]   (default: standard)
set -euo pipefail

VARIANT="${1:-standard}"
export LANG=C.UTF-8 LC_ALL=C.UTF-8   # Qt wants a UTF-8 locale (silences a benign warning)
HOST_SRC=/host
WORK=/build/src
OUT=/build/out
export CCACHE_DIR=/build/ccache
export CMAKE_C_COMPILER_LAUNCHER=ccache
export CMAKE_CXX_COMPILER_LAUNCHER=ccache
JOBS="$(nproc)"

mkdir -p "$WORK" "$OUT" "$CCACHE_DIR"

# --- 1. Copy the repo onto the container's own fast filesystem -------------
# Incremental (rsync --delete) so repeat runs only re-copy changed files.
# Build dirs and .git are excluded — we configure fresh build trees in /build.
echo "==> Syncing source  $HOST_SRC  ->  $WORK"
rsync -a --delete \
    --exclude '.git/' \
    --exclude 'build/' --exclude 'build-*/' \
    --exclude 'projects/build/' --exclude 'projects/build-*/' \
    "$HOST_SRC/" "$WORK/"
cd "$WORK"

CLANG_ARGS=(-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++)

# --- variant: standard -----------------------------------------------------
# Plain Debug build, full ctest headless via the offscreen platform plugin.
run_standard() {
    local B="$OUT/standard"
    echo "::group::standard — configure"
    cmake -S projects -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Debug "${CLANG_ARGS[@]}"
    echo "::endgroup::"
    cmake --build "$B" -j "$JOBS" --target tests_all
    echo "==> ctest (offscreen)"
    QT_QPA_PLATFORM=offscreen ctest --test-dir "$B" --output-on-failure
}

# --- variant: asan ---------------------------------------------------------
# Separate build with AddressSanitizer + UBSan — the memory/UB net that can't
# run on the Windows llvm-mingw kit. This is the headline reason to use Linux.
run_asan() {
    local B="$OUT/asan"
    local SAN="-fsanitize=address,undefined -fno-omit-frame-pointer -g"
    echo "::group::asan — configure"
    cmake -S projects -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Debug "${CLANG_ARGS[@]}" \
        -DCMAKE_C_FLAGS="$SAN" -DCMAKE_CXX_FLAGS="$SAN" \
        -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
        -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address,undefined"
    echo "::endgroup::"
    cmake --build "$B" -j "$JOBS" --target tests_all
    echo "==> ctest (offscreen + ASan/UBSan)"
    QT_QPA_PLATFORM=offscreen \
    ASAN_OPTIONS="detect_leaks=0:halt_on_error=1:detect_odr_violation=0" \
    UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1" \
        ctest --test-dir "$B" --output-on-failure
}

# --- variant: xvfb ---------------------------------------------------------
# Run the suite under a REAL X server (xvfb) requesting the xcb platform, for
# anything the offscreen plugin can't exercise. NOTE: GUI tests that pin
# QT_QPA_PLATFORM=offscreen per-test in CMake keep using offscreen — that
# per-test ENV overrides this shell env; this path still provides a live
# display for any test that needs one.
run_xvfb() {
    local B="$OUT/standard"
    echo "::group::xvfb — configure (reuses standard build)"
    cmake -S projects -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Debug "${CLANG_ARGS[@]}"
    echo "::endgroup::"
    cmake --build "$B" -j "$JOBS" --target tests_all
    echo "==> ctest under xvfb (xcb)"
    xvfb-run -a -s "-screen 0 1280x1024x24" \
        env QT_QPA_PLATFORM=xcb ctest --test-dir "$B" --output-on-failure
}

# --- variant: coverage -----------------------------------------------------
# Instrumented build (llvm source-based coverage) + a per-module report.
# PSE_SHARED_APPCORE=ON so appcore is one shared lib and its coverage merges
# cleanly (see notes/plans/testing.md on the static-lib measurement artifact).
run_coverage() {
    local B="$OUT/coverage"
    local PROF="$B/prof"
    local COV="-fprofile-instr-generate -fcoverage-mapping"
    echo "::group::coverage — configure"
    cmake -S projects -B "$B" -G Ninja -DCMAKE_BUILD_TYPE=Debug "${CLANG_ARGS[@]}" \
        -DPSE_SHARED_APPCORE=ON \
        -DCMAKE_C_FLAGS="$COV" -DCMAKE_CXX_FLAGS="$COV" \
        -DCMAKE_EXE_LINKER_FLAGS="$COV" -DCMAKE_SHARED_LINKER_FLAGS="$COV"
    echo "::endgroup::"
    cmake --build "$B" -j "$JOBS" --target tests_all
    rm -rf "$PROF"; mkdir -p "$PROF"
    echo "==> ctest (offscreen, instrumented)"
    # %m = per-module hash, %p = pid -> one profraw per test process.
    LLVM_PROFILE_FILE="$PROF/%m-%p.profraw" QT_QPA_PLATFORM=offscreen \
        ctest --test-dir "$B" --output-on-failure || true
    echo "==> merging coverage"
    llvm-profdata merge -sparse "$PROF"/*.profraw -o "$B/merged.profdata"
    # Report against the instrumented shared libs (the real source layers).
    # llvm-cov takes ONE binary as the first positional, the rest via -object;
    # filter out tests/Qt/system/generated files so the table is project source.
    mapfile -t LIBS < <(find "$B" -name 'lib*.so' | sort -u)
    if ((${#LIBS[@]} == 0)); then echo "!! no instrumented libs found"; return 0; fi
    local FIRST="${LIBS[0]}"; local OBJ=()
    local l; for l in "${LIBS[@]:1}"; do OBJ+=(-object "$l"); done
    local IGNORE='(/tests/|/opt/Qt/|/usr/|moc_|qrc_|\.moc|autogen)'
    echo "==> per-module coverage summary"
    llvm-cov report -instr-profile="$B/merged.profdata" "$FIRST" "${OBJ[@]}" \
        -ignore-filename-regex="$IGNORE" || true
    echo "==> writing HTML report to $B/html"
    llvm-cov show -format=html -output-dir="$B/html" \
        -instr-profile="$B/merged.profdata" "$FIRST" "${OBJ[@]}" \
        -ignore-filename-regex="$IGNORE" || true
    echo "    (extract with: docker run --rm -v pse-build:/build -v \${PWD}:/out alpine cp -r /build/out/coverage/html /out)"
}

case "$VARIANT" in
    standard) run_standard ;;
    asan)     run_asan ;;
    xvfb)     run_xvfb ;;
    coverage) run_coverage ;;
    all)      run_standard; run_asan; run_xvfb; run_coverage ;;
    *) echo "unknown variant: $VARIANT (use standard|asan|xvfb|coverage|all)"; exit 2 ;;
esac

echo "==> done: $VARIANT"
