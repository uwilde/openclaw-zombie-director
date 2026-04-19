[CmdletBinding()]
param(
	[string]$TaskName = "OpenClaw Zombie Director Bridge",
	[string]$ConfigPath = "",
	[switch]$AtStartup,
	[switch]$AtLogon
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($ConfigPath)) {
	$ConfigPath = Join-Path $PSScriptRoot "director-bridge.config.json"
}

$scriptPath = Join-Path $PSScriptRoot "remote-director-bridge.ps1"
if (-not (Test-Path -LiteralPath $scriptPath)) {
	throw "Bridge script not found at '$scriptPath'."
}

if (-not (Test-Path -LiteralPath $ConfigPath)) {
	throw "Bridge config not found at '$ConfigPath'."
}

if (-not $AtStartup -and -not $AtLogon) {
	$AtStartup = $true
}

$resolvedScript = (Resolve-Path -LiteralPath $scriptPath).Path
$resolvedConfig = (Resolve-Path -LiteralPath $ConfigPath).Path
$arguments = "-NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File `"$resolvedScript`" -Action run -ConfigPath `"$resolvedConfig`""

$action = New-ScheduledTaskAction -Execute "powershell.exe" -Argument $arguments
$trigger = if ($AtLogon) {
	New-ScheduledTaskTrigger -AtLogOn
} else {
	New-ScheduledTaskTrigger -AtStartup
}

Register-ScheduledTask -TaskName $TaskName -Action $action -Trigger $trigger -Description "OpenClaw Zombie Director bridge" -Force | Out-Null
Write-Output "Registered scheduled task '$TaskName'."
