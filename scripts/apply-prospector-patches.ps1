$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$prospectorDir = Join-Path (Split-Path $repoRoot -Parent) "prospector-zmk-module"

if (-not (Test-Path -LiteralPath $prospectorDir -PathType Container)) {
    $prospectorDir = Join-Path $repoRoot "prospector-zmk-module"
}

if (-not (Test-Path -LiteralPath $prospectorDir -PathType Container)) {
    throw "prospector-zmk-module was not found. Run west update first."
}

$classicLayoutDir = Join-Path $prospectorDir "boards/shields/prospector_adapter/src/layouts/classic"
if (-not (Test-Path -LiteralPath $classicLayoutDir -PathType Container)) {
    throw "Prospector classic layout directory was not found: $classicLayoutDir"
}

Copy-Item -LiteralPath (Join-Path $repoRoot "patches/battery_bar.c") `
    -Destination (Join-Path $classicLayoutDir "battery_bar.c") -Force

Write-Host "Applied Prospector battery bar patch."
