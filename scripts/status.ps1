[CmdletBinding()]
param(
  [int]$Port = 3000,
  [int]$DaemonPort = 4001
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-SteppeRoot
Set-Location $root

Write-Host "Git status:"
git status --short

Write-Host ""
Write-Host "Listeners:"
$listeners = Get-NetTCPConnection -LocalPort $Port, $DaemonPort -State Listen -ErrorAction SilentlyContinue |
  Select-Object LocalAddress, LocalPort, OwningProcess
if ($listeners) {
  Write-Host ($listeners | Format-Table -AutoSize | Out-String)
} else {
  Write-Host "none"
}

Write-Host ""
Write-Host "Steppe daemon processes:"
$daemons = Get-Process steppe_daemon -ErrorAction SilentlyContinue |
  Select-Object Id, ProcessName, Path
if ($daemons) {
  Write-Host ($daemons | Format-Table -AutoSize | Out-String)
} else {
  Write-Host "none"
}
