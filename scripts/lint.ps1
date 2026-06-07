[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-SteppeRoot
Set-Location $root

Invoke-SteppeNodeCheck -Root $root -Paths @(
  "proxy/server.js",
  "public/app.js"
)
