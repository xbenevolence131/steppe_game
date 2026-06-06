[CmdletBinding()]
param(
  [switch]$SkipBuild,
  [switch]$NoStart,
  [int]$Port = 3000,
  [int]$DaemonPort = 4001
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent $scriptDir
$buildDir = Join-Path $root "build"
$serverLog = Join-Path $root "server.log"
$serverErr = Join-Path $root "server.err"

function Stop-ProcessById {
  param(
    [int]$ProcessId,
    [string]$Reason
  )

  try {
    Write-Host "Stopping process $ProcessId ($Reason)"
    Stop-Process -Id $ProcessId -Force -ErrorAction Stop
  } catch {
    Write-Host "Process $ProcessId already stopped"
  }
}

function Stop-ListenersOnPort {
  param([int]$LocalPort)

  $connections = Get-NetTCPConnection -LocalPort $LocalPort -State Listen -ErrorAction SilentlyContinue
  foreach ($connection in $connections) {
    Stop-ProcessById -ProcessId $connection.OwningProcess -Reason "listener on port $LocalPort"
  }
}

function Stop-SteppeDaemons {
  $daemons = Get-Process steppe_daemon -ErrorAction SilentlyContinue
  foreach ($daemon in $daemons) {
    Stop-ProcessById -ProcessId $daemon.Id -Reason "steppe_daemon"
  }
}

function Find-VsDevCmd {
  $candidates = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
  )

  foreach ($candidate in $candidates) {
    if (Test-Path -LiteralPath $candidate) {
      return $candidate
    }
  }
  return $null
}

function Invoke-CmdChecked {
  param([string]$Command)

  & cmd.exe /d /s /c $Command
  if ($LASTEXITCODE -ne 0) {
    throw "Command failed with exit code $LASTEXITCODE`: $Command"
  }
}

function Invoke-CMakeBuild {
  $vsDevCmd = Find-VsDevCmd
  $commands = @()
  if (!(Test-Path -LiteralPath (Join-Path $buildDir "build.ninja"))) {
    $commands += 'cmake -S "' + $root + '" -B "' + $buildDir + '" -G Ninja'
  }
  $commands += 'cmake --build "' + $buildDir + '"'

  if ($vsDevCmd) {
    $buildCommand = 'call "' + $vsDevCmd + '" -arch=x64 && ' + ($commands -join " && ")
    Invoke-CmdChecked $buildCommand
    return
  }

  Write-Host "Visual Studio developer command prompt not found; trying current shell environment."
  foreach ($command in $commands) {
    Invoke-CmdChecked $command
  }
}

function Wait-ForPort {
  param(
    [int]$LocalPort,
    [int]$TimeoutSeconds = 10
  )

  $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
  while ((Get-Date) -lt $deadline) {
    $listener = Get-NetTCPConnection -LocalPort $LocalPort -State Listen -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($listener) {
      return $listener
    }
    Start-Sleep -Milliseconds 250
  }
  return $null
}

Set-Location $root

Write-Host "Stopping existing Steppe daemon instances and port listeners."
Stop-SteppeDaemons
Stop-ListenersOnPort -LocalPort $Port
Stop-ListenersOnPort -LocalPort $DaemonPort
Start-Sleep -Milliseconds 500

if (!$SkipBuild) {
  Write-Host "Building C++ engine."
  Invoke-CMakeBuild
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

$listener = Wait-ForPort -LocalPort $Port -TimeoutSeconds 10
if (!$listener) {
  Write-Host "Proxy failed to listen on port $Port. stderr:"
  if (Test-Path -LiteralPath $serverErr) {
    Get-Content -Path $serverErr
  }
  throw "Steppe proxy did not start on port $Port"
}

$body = @{ width = 80; height = 40; rivers = 3 } | ConvertTo-Json
Invoke-RestMethod -Uri "http://localhost:$Port/api/generate" -Method Post -ContentType "application/json" -Body $body | Out-Null

Write-Host "Steppe proxy PID: $($process.Id)"
Write-Host "URL: http://localhost:$Port"
Write-Host "Logs: $serverLog"
