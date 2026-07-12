<#
  Copyright 2026 Twilight  --  Apache-2.0 (see LICENSE).

  Build the local emulator-verification environment: a throwaway Python venv at
  tmp/emu-venv (git-ignored) with PyBoy in it, so `tst_emu_parity` can boot the real
  game and check the editor against it.

  This is LOCAL-ONLY and entirely optional. Without it -- and without a ROM -- the
  emulator tests SKIP and the suite stays green. Nothing here is required to build,
  test, or ship the editor.

  THE ROM. tst_emu_parity needs assets/references/backup.gb: a dump of a cartridge you
  own, which is git-ignored and must NEVER be committed, published, or copied into any
  build artifact. It is not ours to distribute. Nothing in this repo does distribute it,
  and .gitignore refuses *.gb / *.gbc / *.gba / *.rom anywhere in the tree so it cannot
  be added by accident.

  Usage:  pwsh -File scripts\emu\setup.ps1  [-Recreate]
#>
[CmdletBinding()]
param([switch]$Recreate)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$venv = Join-Path $repo 'tmp\emu-venv'
$python = Join-Path $venv 'Scripts\python.exe'

if ($Recreate -and (Test-Path $venv)) {
  Write-Host "Removing $venv..."
  Remove-Item -Recurse -Force $venv
}

if (-not (Test-Path $python)) {
  Write-Host "Creating venv at $venv..."
  python -m venv $venv
}

Write-Host "Installing PyBoy (+ numpy, pillow)..."
& $python -m pip install --quiet --upgrade pip
& $python -m pip install --quiet --timeout 180 --retries 10 pyboy pillow

& $python -c "import pyboy; print('PyBoy ready')"

$rom = Join-Path $repo 'assets\references\backup.gb'
if (Test-Path $rom) {
  # The disassembly ships the SHA-1s of the ROMs it builds. If the dump matches, then the
  # ROM, the disassembly we read the map format out of, and the editor are all the same
  # game -- which is what makes the emulator a valid oracle rather than an approximation.
  $sha = (Get-FileHash $rom -Algorithm SHA1).Hash.ToLower()
  $known = Get-Content (Join-Path $repo 'assets\references\pokered\roms.sha1') -ErrorAction SilentlyContinue

  $match = $known | Where-Object { $_ -like "$sha*" }
  if ($match) {
    Write-Host "ROM verified: $($match -replace '\*', '') (matches the disassembly's own roms.sha1)" -ForegroundColor Green
  }
  else {
    Write-Host "ROM present, but its SHA-1 ($sha) is not one the disassembly knows." -ForegroundColor Yellow
    Write-Host "The emulator tests will still run, but the ROM may be a different revision." -ForegroundColor Yellow
  }
}
else {
  Write-Host "No ROM at assets/references/backup.gb -- the emulator tests will SKIP." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Done. Run it with:  ctest -R tst_emu_parity --output-on-failure"
