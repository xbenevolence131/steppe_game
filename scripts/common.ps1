Set-StrictMode -Version Latest

function Get-SteppeRoot {
  return (Split-Path -Parent $PSScriptRoot)
}

function Stop-SteppeProcessById {
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

function Stop-SteppeListenersOnPort {
  param([int]$LocalPort)

  $connections = Get-NetTCPConnection -LocalPort $LocalPort -State Listen -ErrorAction SilentlyContinue
  foreach ($connection in $connections) {
    Stop-SteppeProcessById -ProcessId $connection.OwningProcess -Reason "listener on port $LocalPort"
  }
}

function Stop-SteppeDaemons {
  $daemons = Get-Process steppe_daemon -ErrorAction SilentlyContinue
  foreach ($daemon in $daemons) {
    Stop-SteppeProcessById -ProcessId $daemon.Id -Reason "steppe_daemon"
  }
}

function Find-SteppeVsDevCmd {
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

function Invoke-SteppeCmdChecked {
  param([string]$Command)

  & cmd.exe /d /s /c $Command
  if ($LASTEXITCODE -ne 0) {
    throw "Command failed with exit code $LASTEXITCODE`: $Command"
  }
}

function Invoke-SteppeBuild {
  param(
    [string]$Root = (Get-SteppeRoot),
    [string]$BuildDir = (Join-Path $Root "build")
  )

  $vsDevCmd = Find-SteppeVsDevCmd
  $commands = @()
  if (!(Test-Path -LiteralPath (Join-Path $BuildDir "build.ninja"))) {
    $commands += 'cmake -S "' + $Root + '" -B "' + $BuildDir + '" -G Ninja'
  }
  $commands += 'cmake --build "' + $BuildDir + '"'

  if ($vsDevCmd) {
    $buildCommand = 'call "' + $vsDevCmd + '" -arch=x64 && ' + ($commands -join " && ")
    Invoke-SteppeCmdChecked $buildCommand
    return
  }

  Write-Host "Visual Studio developer command prompt not found; trying current shell environment."
  foreach ($command in $commands) {
    Invoke-SteppeCmdChecked $command
  }
}

function Wait-SteppePort {
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

function Invoke-SteppeNodeCheck {
  param(
    [string]$Root = (Get-SteppeRoot),
    [string[]]$Paths
  )

  foreach ($relativePath in $Paths) {
    Write-Host "node --check $relativePath"
    & node --check (Join-Path $Root $relativePath)
    if ($LASTEXITCODE -ne 0) {
      throw "node --check failed for $relativePath"
    }
  }
}
