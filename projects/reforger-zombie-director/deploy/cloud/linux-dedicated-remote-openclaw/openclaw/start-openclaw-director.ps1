[CmdletBinding()]
param(
	[string]$BridgeConfigPath = "",
	[string]$PromptOutputPath = "",
	[string]$OpenClawCommand = "",
	[switch]$NoOpen
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($BridgeConfigPath)) {
	$BridgeConfigPath = Join-Path (Split-Path $PSScriptRoot -Parent) "bridge\director-bridge.config.json"
}

if ([string]::IsNullOrWhiteSpace($PromptOutputPath)) {
	$PromptOutputPath = Join-Path $PSScriptRoot "openclaw-director-session.generated.md"
}

$bridgeScriptPath = Join-Path (Split-Path $PSScriptRoot -Parent) "bridge\remote-director-bridge.ps1"
$renderScriptPath = Join-Path $PSScriptRoot "render-openclaw-director-session.ps1"

if (-not (Test-Path -LiteralPath $bridgeScriptPath)) {
	throw "Bridge script not found at '$bridgeScriptPath'."
}

if (-not (Test-Path -LiteralPath $renderScriptPath)) {
	throw "Renderer script not found at '$renderScriptPath'."
}

$dashboardCommandPath = $OpenClawCommand
if ([string]::IsNullOrWhiteSpace($dashboardCommandPath)) {
	$dashboardCommand = Get-Command openclaw.cmd, openclaw -ErrorAction SilentlyContinue | Select-Object -First 1
	if ($dashboardCommand) {
		$dashboardCommandPath = $dashboardCommand.Source
	}
}

$healthJson = powershell -ExecutionPolicy Bypass -File $bridgeScriptPath -Action health -ConfigPath $BridgeConfigPath
$promptPath = powershell -ExecutionPolicy Bypass -File $renderScriptPath -BridgeConfigPath $BridgeConfigPath -OutputPath $PromptOutputPath

if ([string]::IsNullOrWhiteSpace($dashboardCommandPath)) {
	Write-Warning "OpenClaw CLI was not found in PATH. Rendered the session prompt anyway."
} else {
	$dashboardArgs = @("dashboard")
	if ($NoOpen) {
		$dashboardArgs += "--no-open"
		Write-Output "OpenClaw dashboard command:"
		Write-Output "$dashboardCommandPath $($dashboardArgs -join ' ')"
	} else {
		Start-Process -FilePath $dashboardCommandPath -ArgumentList $dashboardArgs | Out-Null
		Write-Output "Launched OpenClaw dashboard in a separate process."
	}
}

Write-Output ""
Write-Output "Bridge health:"
Write-Output $healthJson
Write-Output ""
Write-Output "Director prompt:"
Write-Output $promptPath
