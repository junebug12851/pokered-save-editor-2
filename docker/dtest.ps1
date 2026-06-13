<#
.SYNOPSIS
    Build + run the Pokered Save Editor 2 test suite in the Linux container.

.DESCRIPTION
    Builds the pse-linux-test image once (cached), then runs a test variant.
    The repo is bind-mounted read-only at /host and rsynced into a persistent
    named volume (pse-build) so builds happen on the container's fast ext4
    filesystem, not over the slow Windows/WSL bind mount. ccache + the build
    tree live in the volume, so repeat runs are incremental and fast.

.PARAMETER Variant
    standard (default) | asan | xvfb | coverage | all

.PARAMETER Rebuild
    Force a rebuild of the Docker image (e.g. after editing the Dockerfile).

.PARAMETER Clean
    Remove the persistent build volume first (full clean rebuild of the source).

.EXAMPLE
    .\docker\dtest.ps1                 # standard offscreen ctest
    .\docker\dtest.ps1 asan            # AddressSanitizer + UBSan run
    .\docker\dtest.ps1 coverage        # llvm-cov report
    .\docker\dtest.ps1 all -Rebuild    # rebuild image, run every variant
#>
param(
    [ValidateSet('standard','asan','xvfb','coverage','all')]
    [string]$Variant = 'standard',
    [switch]$Rebuild,
    [switch]$Clean
)
$ErrorActionPreference = 'Stop'

$here = $PSScriptRoot                       # ...\docker
$repo = Split-Path -Parent $here           # repo root
$img  = 'pse-linux-test'
$vol  = 'pse-build'

if ($Clean) {
    Write-Host "==> Removing build volume '$vol'" -ForegroundColor Yellow
    docker volume rm $vol 2>$null | Out-Null
}

$haveImg = (docker images -q $img)
if ($Rebuild -or -not $haveImg) {
    Write-Host "==> Building image '$img' (one-time; cached afterwards)" -ForegroundColor Cyan
    docker build -t $img $here
    if ($LASTEXITCODE -ne 0) { throw "docker build failed" }
}

docker volume create $vol | Out-Null

Write-Host "==> Running variant '$Variant'" -ForegroundColor Cyan
docker run --rm `
    -v "${repo}:/host:ro" `
    -v "${vol}:/build" `
    $img $Variant
exit $LASTEXITCODE
