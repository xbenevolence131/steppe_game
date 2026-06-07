[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-SteppeRoot
Set-Location $root

Stop-SteppeDaemons
Invoke-SteppeBuild -Root $root
