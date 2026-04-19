[CmdletBinding()]
param(
	[string]$BridgeConfigPath = "",
	[string]$OutputPath = "",
	[switch]$IncludeToken
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($BridgeConfigPath)) {
	$BridgeConfigPath = Join-Path (Split-Path $PSScriptRoot -Parent) "bridge\director-bridge.config.json"
}

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
	$OutputPath = Join-Path $PSScriptRoot "openclaw-director-session.generated.md"
}

if (-not (Test-Path -LiteralPath $BridgeConfigPath)) {
	throw "Bridge config not found at '$BridgeConfigPath'."
}

$config = Get-Content -Path $BridgeConfigPath -Raw -Encoding UTF8 | ConvertFrom-Json
$bridgeBaseUrl = $config.clientBaseUrl
if ([string]::IsNullOrWhiteSpace($bridgeBaseUrl)) {
	$bridgeBaseUrl = "http://127.0.0.1:18890/"
}

$bridgeScriptPath = Join-Path (Split-Path $PSScriptRoot -Parent) "bridge\remote-director-bridge.ps1"
$tokenLine = ""
if ($IncludeToken -and -not [string]::IsNullOrWhiteSpace($config.token)) {
	$tokenLine = "- Bridge token: `"$($config.token)`"`r`n"
}

$content = @'
# OpenClaw Director Session

Use the Reforger Zombie Director bridge instead of clicking the Game Master UI.

Bridge settings:

- Bridge base URL: "__BRIDGE_BASE_URL__"
__TOKEN_LINE__- Config path: "__CONFIG_PATH__"

Rules:

1. Fetch the latest snapshot before making a decision.
2. Post only one hint at a time unless the user explicitly wants chaos.
3. Use only these hint types: `reinforce_zone`, `stage_ambush`, `sweep_corridor`, `force_spike`, `quiet_down`.
4. Use only zone ids and template ids already present in snapshots or mission config.
5. Do not use GUI automation for Reforger.

Useful commands:

```powershell
powershell -ExecutionPolicy Bypass -File "__BRIDGE_SCRIPT_PATH__" -Action health -ConfigPath "__CONFIG_PATH__"
powershell -ExecutionPolicy Bypass -File "__BRIDGE_SCRIPT_PATH__" -Action get-snapshot -ConfigPath "__CONFIG_PATH__"
powershell -ExecutionPolicy Bypass -File "__BRIDGE_SCRIPT_PATH__" -Action post-hint -ConfigPath "__CONFIG_PATH__" -Type reinforce_zone -ZoneId dead_everon_base_morton -TemplateId dead_everon_tier1_sweep -Budget 8 -Weight 1.8 -TtlSeconds 45 -Reason "base nearly captured"
```

Decision model:

- If a base is nearly captured, prefer `reinforce_zone`.
- If players are entering open terrain, prefer `stage_ambush`.
- If players are pushing a lane or settlement, prefer `sweep_corridor`.
- If the session is flat and budget is available, prefer `force_spike`.
- If pressure has stacked too hard, prefer `quiet_down`.
'@

$content = $content.Replace("__BRIDGE_BASE_URL__", $bridgeBaseUrl)
$content = $content.Replace("__TOKEN_LINE__", $tokenLine)
$content = $content.Replace("__CONFIG_PATH__", $BridgeConfigPath)
$content = $content.Replace("__BRIDGE_SCRIPT_PATH__", $bridgeScriptPath)

Set-Content -Path $OutputPath -Value $content -Encoding UTF8
Write-Output $OutputPath
