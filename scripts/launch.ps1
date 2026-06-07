[CmdletBinding()]
param(
  [switch]$SkipBuild,
  [switch]$NoStart,
  [int]$Port = 3000,
  [int]$DaemonPort = 4001
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-SteppeRoot
$serverLog = Join-Path $root "server.log"
$serverErr = Join-Path $root "server.err"

Set-Location $root

Write-Host "Stopping existing Steppe daemon instances and port listeners."
Stop-SteppeDaemons
Stop-SteppeListenersOnPort -LocalPort $Port
Stop-SteppeListenersOnPort -LocalPort $DaemonPort
Start-Sleep -Milliseconds 500

if (!$SkipBuild) {
  Write-Host "Building C++ engine."
  Invoke-SteppeBuild -Root $root
}

if ($NoStart) {
  Write-Host "Launch skipped because -NoStart was provided."
  exit 0
}

if (Test-Path -LiteralPath $serverLog) {
  Remove-Item -LiteralPath $serverLog
}
if (Test-Path -LiteralPath $serverErr) {
  Remove-Item -LiteralPath $serverErr
}

$env:PORT = [string]$Port
$env:STEPPE_DAEMON_PORT = [string]$DaemonPort

Write-Host "Starting Steppe proxy on http://localhost:$Port."
$process = Start-Process `
  -FilePath "node" `
  -ArgumentList "proxy/server.js" `
  -WorkingDirectory $root `
  -RedirectStandardOutput $serverLog `
  -RedirectStandardError $serverErr `
  -WindowStyle Hidden `
  -PassThru

$listener = Wait-SteppePort -LocalPort $Port -TimeoutSeconds 10
if (!$listener) {
  Write-Host "Proxy failed to listen on port $Port. stderr:"
  if (Test-Path -LiteralPath $serverErr) {
    Get-Content -Path $serverErr
  }
  throw "Steppe proxy did not start on port $Port"
}

& (Join-Path $PSScriptRoot "smoke.ps1") -Port $Port

Write-Host "Steppe proxy PID: $($process.Id)"
Write-Host "URL: http://localhost:$Port"
Write-Host "Logs: $serverLog"
