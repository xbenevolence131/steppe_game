[CmdletBinding()]
param(
  [string]$Grep = "",
  [switch]$Headed,
  [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-SteppeRoot
Set-Location $root

if (!$SkipBuild) {
  Invoke-SteppeBuild -Root $root
}

$playwright = Join-Path $root "node_modules\.bin\playwright.cmd"
if (!(Test-Path -LiteralPath $playwright)) {
  throw "Playwright binary not found. Run npm install first."
}

$args = @("test")
if ($Headed) {
  $args += "--headed"
}
if ($Grep.Trim()) {
  $args += "--grep"
  $args += $Grep
}

& $playwright @args
if ($LASTEXITCODE -ne 0) {
  throw "Playwright failed with exit code $LASTEXITCODE"
}
