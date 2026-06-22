#!/usr/bin/env bash
# Static analysis for pokered-save-editor-2 — clang-tidy (C++), qmllint (QML),
# cppcheck (C++). The Linux/CI mirror of scripts/lint.ps1 (see that file's header
# and notes/plans/testing.md -> "Static analysis / linting" for the full rationale).
#
# Usage:  scripts/lint.sh [--cpp] [--qml] [--cppcheck] [--build DIR]
#   No tool flag  -> run every available tool.
#   Needs build/compile_commands.json (configure with CMAKE_EXPORT_COMPILE_COMMANDS,
#   which projects/CMakeLists.txt forces ON).
# Exit non-zero if any GATED finding is present.
set -uo pipefail

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo"

BUILD="build"; RUN_CPP=0; RUN_QML=0; RUN_CPPCHECK=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --cpp) RUN_CPP=1 ;;
    --qml) RUN_QML=1 ;;
    --cppcheck) RUN_CPPCHECK=1 ;;
    --build) BUILD="$2"; shift ;;
    *) echo "unknown arg: $1"; exit 64 ;;
  esac
  shift
done
if [[ $RUN_CPP -eq 0 && $RUN_QML -eq 0 && $RUN_CPPCHECK -eq 0 ]]; then
  RUN_CPP=1; RUN_QML=1; RUN_CPPCHECK=1
fi

# QML categories disabled when GATING (kept in sync with scripts/lint.ps1):
#  - architecture-induced false positives (brg context property + custom C++ types
#    that follow from the deliberate no-qt_add_qml_module design)
#  - appearance-affecting categories (changing QML appearance is a design decision)
QML_DISABLED=(unqualified unresolved-type missing-type import missing-property \
              incompatible-type restricted-type layout-positioning property-changes-parsed)

fails=0

run_clang_tidy() {
  echo "== clang-tidy =="
  local cc="$BUILD/compile_commands.json"
  if [[ ! -f "$cc" ]]; then
    echo "  compile_commands.json not found in '$BUILD' — configure the build first."; fails=$((fails+1)); return
  fi
  # source TUs only; skip Qt/system + generated (moc/qrc/ui/autogen)
  mapfile -t files < <(python3 - "$cc" <<'PY'
import json,sys,re
for e in json.load(open(sys.argv[1])):
    f=e["file"]
    # Skip Qt/system + generated (moc/qrc/ui/autogen) and the app exe-shell `boot/`
    # TUs -- those include the app's AUTOUIC ui_*.h which only the exe target (not
    # tests_all) generates, so clang-tidy can't parse them headless. boot is thin
    # GUI glue, not unit-tested logic.
    if re.search(r'projects/(common|db|savefile|app)/src',f) \
       and not re.search(r'(moc_|qrc_|ui_|_autogen|/generated/|/boot/)',f):
        print(f)
PY
)
  files=($(printf '%s\n' "${files[@]}" | sort -u))
  local jobs; jobs=$(nproc 2>/dev/null || echo 4)
  echo "  ${#files[@]} translation units (parallel x$jobs)"
  # Run clang-tidy file-parallel -- a single serial invocation over ~140 Qt TUs
  # takes ~25 min on a 2-core runner (it re-parses Qt headers per TU). Each child
  # writes its own log (concurrent appends to one file would interleave); then we
  # concatenate. Results are identical to the serial run (same .clang-tidy + DB).
  local td; td=$(mktemp -d)
  _tidy_one() { clang-tidy -p "$2" "$1" > "$3/$(echo "$1" | tr '/:\\' '___').log" 2>&1; }
  export -f _tidy_one
  printf '%s\n' "${files[@]}" | xargs -P "$jobs" -I{} bash -c '_tidy_one "$1" "$2" "$3"' _ {} "$BUILD" "$td"
  cat "$td"/*.log > lint-clang-tidy.log 2>/dev/null
  rm -rf "$td"
  local n; n=$(grep -cE 'warning:|error:' lint-clang-tidy.log || true)
  if [[ "$n" -gt 0 ]]; then
    echo "  $n clang-tidy findings (see lint-clang-tidy.log):"
    grep -oE '\[[a-z0-9.-]+\]$' lint-clang-tidy.log | sort | uniq -c | sort -rn
    echo "  --- offending lines (first 60) ---"
    grep -nE 'warning:|error:' lint-clang-tidy.log | head -60
    fails=$((fails+n))
  else
    echo "  clean."
  fi
}

run_qmllint() {
  # INFORMATIONAL, NOT GATED — see scripts/lint.ps1 Invoke-Qmllint for the full
  # rationale (qmllint can't resolve the project's C++-registered QML types, so its
  # findings are unreliable here; QML is also design-owned). Surfaced, never fails.
  echo "== qmllint (informational, not gated) =="
  mapfile -t qml < <(find projects/app/ui -name '*.qml' | sort)
  echo "  ${#qml[@]} QML files"
  local disable=(); for c in "${QML_DISABLED[@]}"; do disable+=(--"$c" disable); done
  qmllint "${disable[@]}" "${qml[@]}" 2>&1 | tee lint-qmllint.log >/dev/null
  local n; n=$(grep -c 'Warning:' lint-qmllint.log || true)
  if [[ "$n" -gt 0 ]]; then
    echo "  $n qmllint notes (review by hand; see lint-qmllint.log):"
    grep -oE '\[[a-zA-Z0-9.-]+\]$' lint-qmllint.log | sort | uniq -c | sort -rn
  else
    echo "  no notes."
  fi
}

run_cppcheck() {
  # INFORMATIONAL for now (not gated): cppcheck is noisy on Qt and hasn't yet had a
  # validation pass on this codebase (it isn't on the Windows kit, so it's only seen
  # on the Linux CI). It's surfaced here; promote it to gating (re-add
  # --error-exitcode + the fails increment) once a clean run is confirmed and the
  # suppressions tuned. clang-tidy is the enforced C++ gate meanwhile.
  if ! command -v cppcheck >/dev/null 2>&1; then
    echo "== cppcheck == (not installed, skipping)"; return
  fi
  echo "== cppcheck (informational, not gated) =="
  # -j + --max-configs=1: a full cppcheck over every -D combination took >10 min on
  # CI; limiting configurations and parallelising keeps it to a few minutes.
  cppcheck --quiet --enable=warning,performance,portability \
    -j "$(nproc 2>/dev/null || echo 4)" --max-configs=1 \
    --inline-suppr --suppressions-list=.cppcheck-suppressions \
    --std=c++20 -i build \
    projects/common/src projects/db/src projects/savefile/src projects/app/src \
    2>&1 | tee lint-cppcheck.log || true
}

[[ $RUN_CPP -eq 1 ]] && run_clang_tidy
[[ $RUN_QML -eq 1 ]] && run_qmllint
[[ $RUN_CPPCHECK -eq 1 ]] && run_cppcheck

echo
if [[ $fails -gt 0 ]]; then echo "LINT FAILED ($fails gated findings)."; exit 1; fi
echo "LINT CLEAN."; exit 0
