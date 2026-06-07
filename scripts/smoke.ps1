[CmdletBinding()]
param(
  [int]$Port = 3000
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$body = @{ width = 80; height = 40; rivers = 3 } | ConvertTo-Json
Invoke-RestMethod -Uri "http://localhost:$Port/api/generate" -Method Post -ContentType "application/json" -Body $body | Out-Null
Write-Host "Smoke test passed: POST /api/generate"
