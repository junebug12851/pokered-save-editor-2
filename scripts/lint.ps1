<#
.SYNOPSIS
  Static analysis for pokered-save-editor-2 — clang-tidy (C++), qmllint (QML),
  and cppcheck (C++, optional/if installed).

.DESCRIPTION
  The single local entry point for the project's static-analysis layer. Mirrors
  what the `lint` GitHub Actions workflow runs (scripts/lint.sh on Linux).

    * clang-tidy  — curated, high-signal check set from the repo-root .clang-tidy.
                    Reads build/compile_commands.json for exact per-file flags, so
                    the build must have been configured first (CMAKE_EXPORT_COMPILE_
                    COMMANDS is forced ON in projects/CMakeLists.txt).
    * qmllint     — INFORMATIONAL only (never fails the build). qmllint can't resolve
                    the project's C++-registered QML types (no-qt_add_qml_module by
                    design), which makes its type-dependent categories false positives
                    and its unused-import detection unreliable; changing QML is also a
                    design decision. So it's surfaced for human review, not gated.
    * cppcheck    — run only if cppcheck is on PATH (not installed on the kit by
                    default; the Linux CI job apt-installs it).

  See notes/plans/testing.md -> "Static analysis / linting".

.PARAMETER Build   Build dir holding compile_commands.json (default: build).
.PARAMETER Cpp     Run clang-tidy only.
.PARAMETER Qml     Run qmllint only.
.PARAMETER Cppcheck Run cppcheck only.
.PARAMETER SetupPath Prepend the known Qt 6.11 llvm-mingw kit bin dirs to PATH.
.PARAMETER Fix     Pass -fix to clang-tidy (apply fix-its). Use with care.

  With no -Cpp/-Qml/-Cppcheck switch, all available tools run.
  Exit code is non-zero if any GATED finding is present (CI-friendly).
#>
[CmdletBinding()]
param(
  [string]$Build = "build",
  [switch]$Cpp,
  [switch]$Qml,
  [switch]$Cppcheck,
  [switch]$SetupPath,
  [switch]$Fix
)

$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
Set-Location $repo

if ($SetupPath) {
  $env:Path = "C:\Qt\Tools\llvm-mingw1706_64\bin;C:\Qt\6.11.0\llvm-mingw_64\bin;C:\Qt\Tools\CMake_64\bin;" + $env:Path
}

$runAll = -not ($Cpp -or $Qml -or $Cppcheck)
$gatedFailures = 0

# --- QML categories that are real DEFECTS and safe to gate on (no appearance
#     change to fix). Everything else is either architecture-induced noise or an
#     appearance decision. Keep in sync with scripts/lint.sh. ------------------
$QmlDisabled = @(
  "unqualified",        # `brg` is an intentional C++ context property (project-wide)
  "unresolved-type",    # custom C++-registered types (no qt_add_qml_module by design)
  "missing-type", "import", "missing-property", "incompatible-type",
  "restricted-type",
  "layout-positioning",         # anchors+Layout mixes — APPEARANCE, surfaced separately
  "property-changes-parsed"     # State property-changes — APPEARANCE, surfaced separately
)

function Invoke-ClangTidy {
  Write-Host "== clang-tidy ==" -ForegroundColor Cyan
  $cc = Join-Path $Build "compile_commands.json"
  if (-not (Test-Path $cc)) {
    Write-Host "  compile_commands.json not found in '$Build' — configure the build first." -ForegroundColor Yellow
    $script:gatedFailures++; return
  }
  $entries = Get-Content $cc -Raw | ConvertFrom-Json
  # Skip generated TUs and the app exe-shell `boot/` (includes AUTOUIC ui_*.h that
  # only the exe target generates -- can't be parsed from a tests_all build).
  $files = $entries.file | Where-Object {
    ($_ -match 'projects[\\/](common|db|savefile|app)[\\/]src') -and
    ($_ -notmatch '(moc_|qrc_|ui_|_autogen|[\\/]generated[\\/]|[\\/]boot[\\/])')
  } | Sort-Object -Unique
  $log = Join-Path $repo "lint-clang-tidy.log"
  if ($Fix) {
    # -fix must be serial (concurrent fix-its to shared headers would race).
    Write-Host "  $($files.Count) translation units (serial, -fix)"
    & clang-tidy -p $Build -fix @files 2>&1 | Tee-Object -FilePath $log | Out-Null
  } else {
    # File-parallel: a single serial run over ~140 Qt TUs is ~15 min (header
    # re-parse per TU). Each child writes its own log; then concatenate. Same
    # results as serial (same .clang-tidy + compile DB).
    Write-Host "  $($files.Count) translation units (parallel)"
    $td = Join-Path ([IO.Path]::GetTempPath()) ("tidy_" + [Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $td | Out-Null
    $files | ForEach-Object -ThrottleLimit 10 -Parallel {
      $safe = ($_ -replace '[\\/:]', '_')
      & clang-tidy -p $using:Build $_ 2>&1 | Out-File (Join-Path $using:td "$safe.log")
    }
    Get-ChildItem $td -Filter *.log | Get-Content | Out-File $log
    Remove-Item $td -Recurse -Force -ErrorAction SilentlyContinue
  }
  $warns = (Select-String -Path $log -Pattern 'warning:|error:' -ErrorAction SilentlyContinue)
  $n = if ($warns) { $warns.Count } else { 0 }
  if ($n -gt 0) {
    Write-Host "  $n clang-tidy findings (see lint-clang-tidy.log):" -ForegroundColor Red
    $warns | Group-Object { ($_ -split '\[')[-1] } | Sort-Object Count -Descending |
      Select-Object Count, Name | Format-Table -AutoSize
    $script:gatedFailures += $n
  } else {
    Write-Host "  clean." -ForegroundColor Green
  }
}

function Invoke-Qmllint {
  # INFORMATIONAL, NOT GATED. qmllint cannot resolve this project's C++-registered
  # QML types (a consequence of the deliberate no-qt_add_qml_module design), so its
  # type-dependent categories are false positives AND its unused-import detection is
  # unreliable (a used directory import like `import "../controls"` can be misflagged
  # -> removing it would break the app). Changing QML is also a design decision. So
  # we SURFACE qmllint for human review but never fail the build on it. The enforced
  # gate is the C++ side (clang-tidy + cppcheck).
  Write-Host "== qmllint (informational, not gated) ==" -ForegroundColor Cyan
  $qml = Get-ChildItem (Join-Path $repo "projects\app\ui") -Recurse -Filter *.qml |
    Select-Object -ExpandProperty FullName
  Write-Host "  $($qml.Count) QML files"
  $disableArgs = @(); foreach ($c in $QmlDisabled) { $disableArgs += @("--$c", "disable") }
  $log = Join-Path $repo "lint-qmllint.log"
  & qmllint @disableArgs @qml 2>&1 | Tee-Object -FilePath $log | Out-Null
  $warns = Select-String -Path $log -Pattern 'Warning:' -ErrorAction SilentlyContinue
  $n = if ($warns) { $warns.Count } else { 0 }
  if ($n -gt 0) {
    Write-Host "  $n qmllint notes (review by hand; see lint-qmllint.log):" -ForegroundColor DarkYellow
    $warns | Group-Object { ($_ -split '\[')[-1] } | Sort-Object Count -Descending |
      Select-Object Count, Name | Format-Table -AutoSize
  } else {
    Write-Host "  no notes." -ForegroundColor Green
  }
}

function Invoke-Cppcheck {
  # INFORMATIONAL for now (not gated) -- see scripts/lint.sh run_cppcheck for why
  # (noisy on Qt, no validation pass yet; promote once confirmed clean). clang-tidy
  # is the enforced C++ gate meanwhile.
  $cppcheck = Get-Command cppcheck -ErrorAction SilentlyContinue
  if (-not $cppcheck) {
    Write-Host "== cppcheck == (not installed, skipping)" -ForegroundColor DarkGray
    return
  }
  Write-Host "== cppcheck (informational, not gated) ==" -ForegroundColor Cyan
  $log = Join-Path $repo "lint-cppcheck.log"
  & cppcheck --quiet --enable=warning,performance,portability `
    -j $env:NUMBER_OF_PROCESSORS --max-configs=1 `
    --inline-suppr --suppressions-list=(Join-Path $repo ".cppcheck-suppressions") `
    --std=c++20 `
    -i (Join-Path $repo "build") `
    (Join-Path $repo "projects\common\src") (Join-Path $repo "projects\db\src") `
    (Join-Path $repo "projects\savefile\src") (Join-Path $repo "projects\app\src") 2>&1 |
    Tee-Object -FilePath $log
}

if ($runAll -or $Cpp)      { Invoke-ClangTidy }
if ($runAll -or $Qml)      { Invoke-Qmllint }
if ($runAll -or $Cppcheck) { Invoke-Cppcheck }

Write-Host ""
if ($gatedFailures -gt 0) {
  Write-Host "LINT FAILED ($gatedFailures gated findings)." -ForegroundColor Red
  exit 1
}
Write-Host "LINT CLEAN." -ForegroundColor Green
exit 0
