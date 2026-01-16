param(
  [Parameter(Mandatory = $true)]
  [int]$Port
)

$conns = Get-NetTCPConnection -LocalPort $Port -ErrorAction SilentlyContinue
if (-not $conns) {
  Write-Host "No process found on port $Port"
  exit 0
}

$pids = $conns | Select-Object -ExpandProperty OwningProcess -Unique
foreach ($p in $pids) {
  if ($p -le 4) {
    # 0 (Idle) / 4 (System) cannot be terminated; ignore.
    continue
  }
  try {
    Write-Host "Stopping PID $p (port $Port)..."
    Stop-Process -Id $p -Force -ErrorAction Stop
  } catch {
    Write-Host "Failed to stop PID $p : $($_.Exception.Message)"
  }
}

exit 0

