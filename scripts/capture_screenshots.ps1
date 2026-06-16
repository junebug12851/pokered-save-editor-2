<#
  Copyright 2026 Twilight  --  Apache-2.0 (see LICENSE).

  Build + run the headless still-screenshot capture tool. Output ->
  <repo>\tmp\screenshots\ (git-ignored). Still PNGs only -- animated GIFs are added
  manually, one at a time, so there is NO automated GIF/frame generation here.

  This is the Windows / dev-kit driver, meant to run by default after a fast-forward
  of `main` (see CLAUDE.md "Default Workflow"). It:
    1. ensures the Ninja test build dir is configured,
    2. builds ONLY the `screenshooter` target,
    3. runs it on the offscreen platform (software backend, crash-fast error mode).

  Nothing here writes a save byte -- the tool only renders the UI.

  Usage:  pwsh -File scripts\capture_screenshots.ps1 [-OutDir <path>] [-SkipBuild]
#>
[CmdletBinding()]
param(
  [string]$OutDir,
  [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
if (-not $OutDir) { $OutDir = Join-Path $repo 'tmp\screenshots' }

# --- Toolchain (matches the Qt Creator kit; see CLAUDE.md "Build System") ---
$Qt     = 'C:\Qt\6.11.0\llvm-mingw_64'
$LLVM   = 'C:\Qt\Tools\llvm-mingw1706_64\bin'
$CMake  = 'C:\Qt\Tools\CMake_64\bin\cmake.exe'
$build  = Join-Path $repo 'build'

$env:PATH = "$LLVM;$Qt\bin;$env:PATH"
$env:CC   = 'clang'
$env:CXX  = 'clang++'

if (-not $SkipBuild) {
  if (-not (Test-Path (Join-Path $build 'CMakeCache.txt'))) {
    Write-Host "Configuring test build dir ($build)..."
    & $CMake -S (Join-Path $repo 'projects') -B $build -G Ninja `
        -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
  }
  Write-Host "Building screenshooter..."
  & $CMake --build $build --target screenshooter
  if ($LASTEXITCODE -ne 0) { throw "screenshooter build failed ($LASTEXITCODE)" }
}

$exe = Join-Path $build 'screenshooter.exe'
if (-not (Test-Path $exe)) { throw "screenshooter.exe not found at $exe (build first)" }

# Make sure the linked DLLs (common/db/savefile/appcore) are on PATH.
$dllDirs = Get-ChildItem -Path $build -Recurse -Filter '*.dll' -ErrorAction SilentlyContinue |
           Select-Object -ExpandProperty DirectoryName -Unique
foreach ($d in $dllDirs) { $env:PATH = "$d;$env:PATH" }

# Crash-fast: never hang on the Windows JIT-debugger dialog if the tool faults.
Add-Type -Namespace Win32 -Name Err -MemberDefinition @'
  [System.Runtime.InteropServices.DllImport("kernel32.dll")] public static extern uint SetErrorMode(uint mode);
'@ -ErrorAction SilentlyContinue
[Win32.Err]::SetErrorMode(0x0003) | Out-Null   # SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
# Render through a REAL GPU-backed window (NOT offscreen): the offscreen+software
# path silently drops MultiEffect/layered content (Credits cards, Home disabled
# tiles, shadows). The tool shows the window off-screen so nothing flashes on the
# desktop. Pin DPR=1 so the grab is exactly 1130x740 regardless of display scaling.
Remove-Item Env:\QT_QPA_PLATFORM -ErrorAction SilentlyContinue
$env:QT_SCALE_FACTOR = '1'
Write-Host "Capturing screenshots -> $OutDir"
& $exe $OutDir
if ($LASTEXITCODE -ne 0) { throw "screenshooter run failed ($LASTEXITCODE)" }

Write-Host "Screenshot capture complete: $OutDir"
