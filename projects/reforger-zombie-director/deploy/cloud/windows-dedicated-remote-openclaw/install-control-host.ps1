[CmdletBinding()]
param(
	[string]$PublicBaseUrl = "",
	[string]$Token = "",
	[string]$OpenClawCommand = "",
	[string]$ListenPrefix = "",
	[string]$ClientBaseUrl = "",
	[string]$StateRoot = "",
	[switch]$GenerateToken,
	[switch]$SkipScheduledTask,
	[switch]$TaskAtLogon,
	[switch]$NoOpenClaw,
	[switch]$Force
)

$ErrorActionPreference = "Stop"

$bundleRoot = $PSScriptRoot
$configureScript = Join-Path $bundleRoot "configure-cloud-bundle.ps1"
$bridgeScript = Join-Path $bundleRoot "bridge\remote-director-bridge.ps1"
$taskScript = Join-Path $bundleRoot "bridge\install-bridge-task.ps1"
$startOpenClawScript = Join-Path $bundleRoot "openclaw\start-openclaw-director.ps1"
$bridgeConfigPath = Join-Path $bundleRoot "bridge\director-bridge.config.json"
$serverOverridePath = Join-Path $bundleRoot "server\rest-bridge.profile-override.generated.json"

if (-not (Test-Path -LiteralPath $configureScript)) {
	throw "Configurator script not found at '$configureScript'."
}

$configureArgs = @()
if (-not [string]::IsNullOrWhiteSpace($PublicBaseUrl)) {
	$configureArgs += @("-PublicBaseUrl", $PublicBaseUrl)
}
if (-not [string]::IsNullOrWhiteSpace($Token)) {
	$configureArgs += @("-Token", $Token)
}
if (-not [string]::IsNullOrWhiteSpace($ListenPrefix)) {
	$configureArgs += @("-ListenPrefix", $ListenPrefix)
}
if (-not [string]::IsNullOrWhiteSpace($ClientBaseUrl)) {
	$configureArgs += @("-ClientBaseUrl", $ClientBaseUrl)
}
if (-not [string]::IsNullOrWhiteSpace($StateRoot)) {
	$configureArgs += @("-StateRoot", $StateRoot)
}
if ($GenerateToken -or [string]::IsNullOrWhiteSpace($Token)) {
	$configureArgs += "-GenerateToken"
}
if ($Force) {
	$configureArgs += "-Force"
}

& powershell -ExecutionPolicy Bypass -File $configureScript @configureArgs | Out-Host

$bridgeStarted = $false
try {
	$startOutput = & powershell -ExecutionPolicy Bypass -File $bridgeScript -Action start -ConfigPath $bridgeConfigPath 2>&1
	$startExitCode = $LASTEXITCODE
	if ($startOutput) {
		$startOutput | Out-Host
	}

	if ($startExitCode -ne 0) {
		throw "bridge_start_failed"
	}

	$bridgeStarted = $true
} catch {
	try {
		$healthOutput = & powershell -ExecutionPolicy Bypass -File $bridgeScript -Action health -ConfigPath $bridgeConfigPath 2>&1
		$healthExitCode = $LASTEXITCODE
		if ($healthOutput) {
			$healthOutput | Out-Host
		}
		if ($healthExitCode -ne 0) {
			throw "bridge_health_failed"
		}

		Write-Warning "Bridge startup failed, but the configured bridge endpoint is already reachable. Continuing."
	} catch {
		throw
	}
}

if (-not $SkipScheduledTask) {
	$taskArgs = @("-ConfigPath", $bridgeConfigPath)
	if ($TaskAtLogon) {
		$taskArgs += "-AtLogon"
	} else {
		$taskArgs += "-AtStartup"
	}

	& powershell -ExecutionPolicy Bypass -File $taskScript @taskArgs | Out-Host
}

if (-not $NoOpenClaw) {
	$openClawArgs = @("-BridgeConfigPath", $bridgeConfigPath)
	if (-not [string]::IsNullOrWhiteSpace($OpenClawCommand)) {
		$openClawArgs += @("-OpenClawCommand", $OpenClawCommand)
	}

	& powershell -ExecutionPolicy Bypass -File $startOpenClawScript @openClawArgs | Out-Host
}

Write-Output ""
Write-Output "Control-host install complete."
if ($bridgeStarted) {
	Write-Output "The bridge was started immediately on this host."
} else {
	Write-Output "The configured bridge endpoint was already reachable on this host."
}
Write-Output "Upload this file to the Reforger server profile:"
Write-Output "  $serverOverridePath"
Write-Output "Destination on the server:"
Write-Output "  <server profile>/openclaw-zombie-director/rest-bridge.override.json"
