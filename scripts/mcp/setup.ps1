# Creates tmp/mcp-venv and installs the dev MCP server's dependencies (mcp, psutil).
# Dev-only, git-ignored, never shipped — same footing as tmp/emu-venv (see
# notes/reference/emulator-verification.md -> Licensing).
#
#   pwsh -File scripts/mcp/setup.ps1 [-Recreate]

param([switch]$Recreate)

$ErrorActionPreference = "Stop"
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$venv = Join-Path $repo "tmp\mcp-venv"

if ($Recreate -and (Test-Path $venv)) {
    Write-Host "Removing $venv"
    Remove-Item -Recurse -Force $venv
}

if (-not (Test-Path "$venv\Scripts\python.exe")) {
    Write-Host "Creating venv at $venv"
    python -m venv $venv
}

& "$venv\Scripts\python.exe" -m pip install --upgrade pip
& "$venv\Scripts\python.exe" -m pip install --upgrade "mcp>=1.2" psutil

& "$venv\Scripts\python.exe" -c "import mcp, psutil; print('mcp + psutil OK')"
Write-Host "Done. Launcher: scripts\mcp\run.cmd"
