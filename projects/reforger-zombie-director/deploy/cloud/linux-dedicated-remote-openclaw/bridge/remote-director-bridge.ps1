[CmdletBinding()]
param(
	[ValidateSet("help","start","run","stop","health","get-snapshot","post-hint")]
	[string]$Action = "help",
	[string]$ConfigPath = "",
	[string]$Type,
	[string]$ZoneId,
	[string]$TemplateId,
	[int]$Budget = -1,
	[double]$Weight = 1.0,
	[double]$TtlSeconds = 30,
	[string]$Reason = ""
)

$ErrorActionPreference = "Stop"
$ScriptPath = $PSCommandPath

if ([string]::IsNullOrWhiteSpace($ConfigPath)) {
	$ConfigPath = Join-Path $PSScriptRoot "director-bridge.config.json"
}

function Get-DefaultBridgeConfig {
	[ordered]@{
		listenPrefix = "http://127.0.0.1:18890/"
		clientBaseUrl = "http://127.0.0.1:18890/"
		publicBaseUrl = ""
		stateRoot = "%LOCALAPPDATA%\\OpenClawDirectorBridgeCloud"
		requireToken = $true
		allowHealthWithoutToken = $true
		token = ""
	}
}

function Expand-ConfigValue {
	param([string]$Value)

	if ([string]::IsNullOrWhiteSpace($Value)) {
		return $Value
	}

	return [Environment]::ExpandEnvironmentVariables($Value)
}

function Normalize-BaseUrl {
	param([string]$BaseUrl)

	if ([string]::IsNullOrWhiteSpace($BaseUrl)) {
		return ""
	}

	if ($BaseUrl.EndsWith("/")) {
		return $BaseUrl
	}

	return "$BaseUrl/"
}

function Load-BridgeConfig {
	param([string]$Path)

	$configMap = Get-DefaultBridgeConfig
	if (Test-Path -LiteralPath $Path) {
		$raw = Get-Content -Path $Path -Raw -Encoding UTF8
		if (-not [string]::IsNullOrWhiteSpace($raw)) {
			$fileConfig = $raw | ConvertFrom-Json
			foreach ($property in $fileConfig.PSObject.Properties) {
				$configMap[$property.Name] = $property.Value
			}
		}
	}

	$configMap.listenPrefix = Normalize-BaseUrl (Expand-ConfigValue $configMap.listenPrefix)
	$configMap.clientBaseUrl = Normalize-BaseUrl (Expand-ConfigValue $configMap.clientBaseUrl)
	$configMap.publicBaseUrl = Normalize-BaseUrl (Expand-ConfigValue $configMap.publicBaseUrl)
	$configMap.stateRoot = Expand-ConfigValue $configMap.stateRoot

	if ([string]::IsNullOrWhiteSpace($configMap.clientBaseUrl)) {
		$configMap.clientBaseUrl = $configMap.listenPrefix
	}

	return [pscustomobject]$configMap
}

$BridgeConfig = Load-BridgeConfig -Path $ConfigPath
$StateRoot = $BridgeConfig.stateRoot
$PidPath = Join-Path $StateRoot "bridge.pid"
$QueuePath = Join-Path $StateRoot "hints.json"
$SnapshotPath = Join-Path $StateRoot "snapshot.json"
$StdOutPath = Join-Path $StateRoot "bridge.stdout.log"
$StdErrPath = Join-Path $StateRoot "bridge.stderr.log"

function Initialize-BridgeState {
	New-Item -ItemType Directory -Force -Path $StateRoot | Out-Null

	if (-not (Test-Path -LiteralPath $QueuePath)) {
		"{`"lastSeq`":0,`"hints`":[]}" | Set-Content -Path $QueuePath -Encoding UTF8
	}

	if (-not (Test-Path -LiteralPath $SnapshotPath)) {
		"{`"budgetCurrent`":0,`"budgetCap`":0,`"activeWaveCount`":0,`"players`":[],`"zones`":[]}" | Set-Content -Path $SnapshotPath -Encoding UTF8
	}
}

function Read-JsonFile {
	param([string]$Path)

	if (-not (Test-Path -LiteralPath $Path)) {
		return $null
	}

	$raw = Get-Content -Path $Path -Raw -Encoding UTF8
	if ([string]::IsNullOrWhiteSpace($raw)) {
		return $null
	}

	return $raw | ConvertFrom-Json
}

function Write-JsonFile {
	param(
		[string]$Path,
		$Object
	)

	$json = $Object | ConvertTo-Json -Depth 20 -Compress
	Set-Content -Path $Path -Value $json -Encoding UTF8
}

function Get-BridgeUrl {
	$BridgeConfig.clientBaseUrl
}

function Get-AuthHeaders {
	$headers = @{}
	if (-not [string]::IsNullOrWhiteSpace($BridgeConfig.token)) {
		$headers["X-Director-Token"] = $BridgeConfig.token
	}

	return $headers
}

function Get-RequestToken {
	param([Parameter(Mandatory = $true)]$Request)

	$token = $Request.Headers["X-Director-Token"]
	if ([string]::IsNullOrWhiteSpace($token)) {
		$authHeader = $Request.Headers["Authorization"]
		if ($authHeader -match "^Bearer\s+(.+)$") {
			$token = $Matches[1]
		}
	}

	if ([string]::IsNullOrWhiteSpace($token)) {
		$token = $Request.QueryString["token"]
	}

	return $token
}

function Test-RequestAuthorized {
	param(
		[Parameter(Mandatory = $true)]$Request,
		[switch]$AllowAnonymous
	)

	if ($AllowAnonymous) {
		return $true
	}

	if (-not $BridgeConfig.requireToken) {
		return $true
	}

	if ([string]::IsNullOrWhiteSpace($BridgeConfig.token)) {
		return $true
	}

	$provided = Get-RequestToken -Request $Request
	return $provided -eq $BridgeConfig.token
}

function Start-Bridge {
	Initialize-BridgeState

	if (Test-Path -LiteralPath $PidPath) {
		$existingPid = Get-Content -Path $PidPath -Raw -ErrorAction SilentlyContinue
		if ($existingPid) {
			$existing = Get-Process -Id ([int]$existingPid) -ErrorAction SilentlyContinue
			if ($existing) {
				Write-Output (@{
					ok = $true
					started = $false
					pid = $existing.Id
					baseUrl = (Get-BridgeUrl)
					publicBaseUrl = $BridgeConfig.publicBaseUrl
				} | ConvertTo-Json -Compress)
				return
			}
		}
	}

	$resolvedScript = (Resolve-Path -LiteralPath $ScriptPath).Path
	$resolvedConfig = $ConfigPath
	if (Test-Path -LiteralPath $ConfigPath) {
		$resolvedConfig = (Resolve-Path -LiteralPath $ConfigPath).Path
	}

	$args = "-NoProfile -ExecutionPolicy Bypass -File `"$resolvedScript`" -Action run -ConfigPath `"$resolvedConfig`""
	$proc = Start-Process powershell.exe -WindowStyle Hidden -PassThru -WorkingDirectory (Split-Path $resolvedScript) -ArgumentList $args -RedirectStandardOutput $StdOutPath -RedirectStandardError $StdErrPath

	Set-Content -Path $PidPath -Value $proc.Id -Encoding UTF8
	Start-Sleep -Milliseconds 700

	$running = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
	if (-not $running) {
		Remove-Item -Path $PidPath -Force -ErrorAction SilentlyContinue
		$errTail = ""
		if (Test-Path -LiteralPath $StdErrPath) {
			$errTail = Get-Content -Path $StdErrPath -Raw -ErrorAction SilentlyContinue
		}

		if ([string]::IsNullOrWhiteSpace($errTail)) {
			throw "Bridge process exited during startup."
		}

		throw "Bridge process exited during startup. $errTail"
	}

	Write-Output (@{
		ok = $true
		started = $true
		pid = $running.Id
		baseUrl = (Get-BridgeUrl)
		publicBaseUrl = $BridgeConfig.publicBaseUrl
	} | ConvertTo-Json -Compress)
}

function Stop-Bridge {
	if (-not (Test-Path -LiteralPath $PidPath)) {
		Write-Output '{"ok":true,"stopped":false}'
		return
	}

	$pidValue = Get-Content -Path $PidPath -Raw -ErrorAction SilentlyContinue
	if ($pidValue) {
		$proc = Get-Process -Id ([int]$pidValue) -ErrorAction SilentlyContinue
		if ($proc) {
			Stop-Process -Id $proc.Id -Force
		}
	}

	Remove-Item -Path $PidPath -Force -ErrorAction SilentlyContinue
	Write-Output '{"ok":true,"stopped":true}'
}

function Invoke-Health {
	$uri = "$(Get-BridgeUrl)reforger/zombie-director/health"
	Invoke-RestMethod -Uri $uri -Method Get -Headers (Get-AuthHeaders) | ConvertTo-Json -Compress
}

function Invoke-GetSnapshot {
	$uri = "$(Get-BridgeUrl)reforger/zombie-director/snapshot"
	Invoke-RestMethod -Uri $uri -Method Get -Headers (Get-AuthHeaders) | ConvertTo-Json -Depth 20 -Compress
}

function Invoke-PostHint {
	if ([string]::IsNullOrWhiteSpace($Type)) {
		throw "Type is required for post-hint."
	}

	if ([string]::IsNullOrWhiteSpace($ZoneId)) {
		throw "ZoneId is required for post-hint."
	}

	$body = @{
		hints = @(
			@{
				type = $Type
				targetZoneId = $ZoneId
				templateId = $TemplateId
				requestedBudget = $Budget
				weight = $Weight
				ttlSeconds = $TtlSeconds
				reason = $Reason
				correlationId = ([guid]::NewGuid().ToString())
				anchor = "0 0 0"
			}
		)
	} | ConvertTo-Json -Depth 10

	$uri = "$(Get-BridgeUrl)reforger/zombie-director/hints"
	Invoke-RestMethod -Uri $uri -Method Post -Headers (Get-AuthHeaders) -Body $body -ContentType "application/json" | ConvertTo-Json -Compress
}

function Send-JsonResponse {
	param(
		[Parameter(Mandatory = $true)]$Context,
		[int]$StatusCode = 200,
		$Payload
	)

	$response = $Context.Response
	$response.StatusCode = $StatusCode
	$response.ContentType = "application/json; charset=utf-8"

	$json = $Payload
	if ($Payload -isnot [string]) {
		$json = $Payload | ConvertTo-Json -Depth 20 -Compress
	}

	$bytes = [System.Text.Encoding]::UTF8.GetBytes($json)
	$response.ContentLength64 = $bytes.Length
	$response.OutputStream.Write($bytes, 0, $bytes.Length)
	$response.OutputStream.Close()
}

function Get-RequestBody {
	param([Parameter(Mandatory = $true)]$Request)

	$reader = New-Object System.IO.StreamReader($Request.InputStream, $Request.ContentEncoding)
	try {
		$reader.ReadToEnd()
	}
	finally {
		$reader.Dispose()
	}
}

function Run-Bridge {
	Initialize-BridgeState

	$listener = [System.Net.HttpListener]::new()
	$listener.Prefixes.Add($BridgeConfig.listenPrefix)
	$listener.Start()

	try {
		while ($listener.IsListening) {
			$context = $listener.GetContext()
			$request = $context.Request
			$path = $request.Url.AbsolutePath.Trim("/")

			$allowAnonymous = $false
			if ($BridgeConfig.allowHealthWithoutToken -and $request.HttpMethod -eq "GET" -and $path -eq "reforger/zombie-director/health") {
				$allowAnonymous = $true
			}

			if (-not (Test-RequestAuthorized -Request $request -AllowAnonymous:$allowAnonymous)) {
				Send-JsonResponse -Context $context -StatusCode 401 -Payload @{
					ok = $false
					error = "unauthorized"
				}
				continue
			}

			switch ("$($request.HttpMethod) $path") {
				"GET reforger/zombie-director/health" {
					Send-JsonResponse -Context $context -Payload @{
						ok = $true
						service = "openclaw-zombie-director-bridge"
						publicBaseUrl = $BridgeConfig.publicBaseUrl
					}
					continue
				}
				"GET reforger/zombie-director/hints" {
					$afterSeq = 0
					[void][int]::TryParse($request.QueryString["afterSeq"], [ref]$afterSeq)
					$queue = Read-JsonFile -Path $QueuePath
					$items = @()

					if ($queue -and $queue.hints) {
						$items = @($queue.hints | Where-Object { [int]$_.seq -gt $afterSeq })
					}

					Send-JsonResponse -Context $context -Payload @{
						serverTime = [DateTime]::UtcNow.ToString("o")
						hints = $items
					}
					continue
				}
				"POST reforger/zombie-director/hints" {
					$bodyText = Get-RequestBody -Request $request
					$body = $bodyText | ConvertFrom-Json
					$queue = Read-JsonFile -Path $QueuePath
					if (-not $queue) {
						$queue = [pscustomobject]@{
							lastSeq = 0
							hints = @()
						}
					}

					$hintList = @()
					if ($body.hints) {
						$hintList = @($body.hints)
					}

					foreach ($hint in $hintList) {
						$queue.lastSeq = [int]$queue.lastSeq + 1
						$hint | Add-Member -NotePropertyName seq -NotePropertyValue ([int]$queue.lastSeq) -Force
						$queue.hints += $hint
					}

					Write-JsonFile -Path $QueuePath -Object $queue
					Send-JsonResponse -Context $context -Payload @{
						ok = $true
						lastSeq = [int]$queue.lastSeq
					}
					continue
				}
				"POST reforger/zombie-director/snapshot" {
					$bodyText = Get-RequestBody -Request $request
					if ([string]::IsNullOrWhiteSpace($bodyText)) {
						Send-JsonResponse -Context $context -StatusCode 400 -Payload @{
							ok = $false
							error = "empty body"
						}
						continue
					}

					$snapshot = $bodyText | ConvertFrom-Json
					Write-JsonFile -Path $SnapshotPath -Object $snapshot
					Send-JsonResponse -Context $context -Payload @{ ok = $true }
					continue
				}
				"GET reforger/zombie-director/snapshot" {
					$snapshot = Read-JsonFile -Path $SnapshotPath
					if (-not $snapshot) {
						$snapshot = @{
							budgetCurrent = 0
							budgetCap = 0
							activeWaveCount = 0
							players = @()
							zones = @()
						}
					}

					Send-JsonResponse -Context $context -Payload $snapshot
					continue
				}
				default {
					Send-JsonResponse -Context $context -StatusCode 404 -Payload @{
						ok = $false
						error = "not found"
						method = $request.HttpMethod
						path = $path
					}
					continue
				}
			}
		}
	}
	finally {
		if ($listener.IsListening) {
			$listener.Stop()
		}

		$listener.Close()
	}
}

switch ($Action) {
	"help" {
		Write-Output @'
Actions:
  start         Start the bridge in a hidden PowerShell process
  run           Run the bridge in the current process
  stop          Stop the background bridge
  health        Query bridge health
  get-snapshot  Read the latest Reforger snapshot
  post-hint     Queue one director hint

Example:
  powershell -ExecutionPolicy Bypass -File ".\remote-director-bridge.ps1" -Action post-hint -ConfigPath ".\director-bridge.config.json" -Type reinforce_zone -ZoneId dead_everon_base_morton -TemplateId dead_everon_tier1_sweep -Budget 8 -Weight 1.8 -TtlSeconds 45 -Reason "base nearly captured"
'@
	}
	"start" { Start-Bridge }
	"run" { Run-Bridge }
	"stop" { Stop-Bridge }
	"health" { Invoke-Health }
	"get-snapshot" { Invoke-GetSnapshot }
	"post-hint" { Invoke-PostHint }
}
