[CmdletBinding()]
param(
  [switch]$Stat,
  [string[]]$Path = @()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-SteppeRoot
Set-Location $root

if ($Stat) {
  if ($Path.Count -gt 0) {
    git diff --stat -- $Path
  } else {
    git diff --stat
  }
} else {
  if ($Path.Count -gt 0) {
    git diff -- $Path
  } else {
    git diff
  }
}
