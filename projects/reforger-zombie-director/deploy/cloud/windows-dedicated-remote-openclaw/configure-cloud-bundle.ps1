[CmdletBinding()]
param(
	[string]$PublicBaseUrl = "",
	[string]$Token = "",
	[string]$ListenPrefix = "http://127.0.0.1:18890/",
	[string]$ClientBaseUrl = "http://127.0.0.1:18890/",
	[string]$StateRoot = "%LOCALAPPDATA%\\OpenClawDirectorBridgeCloud",
	[string]$ConcreteProfileId = "dead_everon_bacon_concrete",
	[switch]$GenerateToken,
	[switch]$Force
)

$ErrorActionPreference = "Stop"

$bundleRoot = $PSScriptRoot
$bridgeDir = Join-Path $bundleRoot "bridge"
$serverDir = Join-Path $bundleRoot "server"
$openclawDir = Join-Path $bundleRoot "openclaw"

$bridgeConfigPath = Join-Path $bridgeDir "director-bridge.config.json"
$serverConfigPath = Join-Path $serverDir "rest-bridge.profile-override.generated.json"
$caddyfilePath = Join-Path $bridgeDir "Caddyfile.generated"
$sessionPromptPath = Join-Path $openclawDir "openclaw-director-session.generated.md"
$renderScriptPath = Join-Path $openclawDir "render-openclaw-director-session.ps1"

function Normalize-BaseUrl {
	param([string]$Value)

	if ([string]::IsNullOrWhiteSpace($Value)) {
		return ""
	}

	$trimmed = $Value.Trim()
	if (-not ($trimmed -match '^[a-zA-Z]+://')) {
		$trimmed = "https://$trimmed"
	}

	if (-not $trimmed.EndsWith("/")) {
		$trimmed = "$trimmed/"
	}

	return $trimmed
}

function New-SharedToken {
	return ([guid]::NewGuid().ToString("N") + [guid]::NewGuid().ToString("N"))
}

function Require-WritePath {
	param([string]$Path)

	if ((Test-Path -LiteralPath $Path) -and -not $Force) {
		throw "Refusing to overwrite '$Path' without -Force."
	}
}

function Write-Utf8NoBomFile {
	param(
		[string]$Path,
		[string]$Content
	)

	$encoding = New-Object System.Text.UTF8Encoding($false)
	[System.IO.File]::WriteAllText($Path, $Content, $encoding)
}

if ([string]::IsNullOrWhiteSpace($PublicBaseUrl)) {
	$PublicBaseUrl = Read-Host "Public HTTPS hostname or URL for the bridge (example: director.example.com)"
}

$PublicBaseUrl = Normalize-BaseUrl $PublicBaseUrl
if ([string]::IsNullOrWhiteSpace($PublicBaseUrl)) {
	throw "A public bridge hostname or URL is required."
}

if ($GenerateToken -or [string]::IsNullOrWhiteSpace($Token)) {
	$Token = New-SharedToken
}

$hostName = $PublicBaseUrl.Replace("https://", "").Replace("http://", "").TrimEnd("/")

Require-WritePath -Path $bridgeConfigPath
Require-WritePath -Path $serverConfigPath
Require-WritePath -Path $caddyfilePath

$bridgeConfig = [ordered]@{
	listenPrefix = $ListenPrefix
	clientBaseUrl = $ClientBaseUrl
	publicBaseUrl = $PublicBaseUrl
	stateRoot = $StateRoot
	requireToken = $true
	allowHealthWithoutToken = $true
	token = $Token
}
Write-Utf8NoBomFile -Path $bridgeConfigPath -Content ($bridgeConfig | ConvertTo-Json)

$serverConfig = [ordered]@{
	kind = "openclaw-zombie-director.rest-bridge-override"
	version = 1
	enabled = 1
	baseUrl = $PublicBaseUrl
	hintRoute = "reforger/zombie-director/hints"
	snapshotRoute = "reforger/zombie-director/snapshot"
	token = $Token
	pollIntervalSeconds = 0.75
	postSnapshots = 1
}
Write-Utf8NoBomFile -Path $serverConfigPath -Content ($serverConfig | ConvertTo-Json -Depth 10)

$caddyfile = @"
$hostName {
	encode zstd gzip

	log {
		output file C:\Logs\director-bridge-access.log
	}

	@director_api path /reforger/*
	reverse_proxy @director_api 127.0.0.1:18890
}
"@
Set-Content -Path $caddyfilePath -Value $caddyfile -Encoding UTF8

if (Test-Path -LiteralPath $renderScriptPath) {
	powershell -ExecutionPolicy Bypass -File $renderScriptPath -BridgeConfigPath $bridgeConfigPath -OutputPath $sessionPromptPath | Out-Null
}

Write-Output ""
Write-Output "Generated:"
Write-Output "  Bridge config: $bridgeConfigPath"
Write-Output "  Server profile override: $serverConfigPath"
Write-Output "  Caddy config:  $caddyfilePath"
if (Test-Path -LiteralPath $sessionPromptPath) {
	Write-Output "  Session prompt: $sessionPromptPath"
}

Write-Output ""
Write-Output "Upload the generated server override file to:"
Write-Output "  <server profile>/openclaw-zombie-director/rest-bridge.override.json"

Write-Output ""
Write-Output "The published scenario should still use:"
Write-Output "  m_sConcreteProfileId = $ConcreteProfileId"
Write-Output "  m_RestBridge.m_sProfileOverridePath = openclaw-zombie-director/rest-bridge.override.json"
Write-Output "  m_RestBridge.m_bEnabled = 0    (recommended default when no bridge file is present)"

Write-Output ""
Write-Output "Only the server operator needs this hostname/token."
Write-Output "Regular players joining the server do not need to know or enter either value."
