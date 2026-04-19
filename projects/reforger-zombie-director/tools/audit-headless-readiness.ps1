[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$overlayRoot = Join-Path (Split-Path -Parent $repoRoot) "reforger-zombie-director-dead-everon-overlay"
$coreGproj = Join-Path $repoRoot "addon.gproj"
$overlayGproj = Join-Path $overlayRoot "addon.gproj"
$directorScripts = Join-Path $repoRoot "scripts\Game\Director"
$bannedPatterns = @(
	"SCR_PlacingEditorComponent",
	"SCR_EditableEntityComponent",
	"SCR_EditableGroupComponent",
	"SpawnEditable\(",
	"Delete\(false,\s*true\)"
)

function Assert-PathExists {
	param(
		[string]$Path,
		[string]$Label
	)

	if (-not (Test-Path -LiteralPath $Path)) {
		throw "$Label not found at '$Path'."
	}
}

function Test-GprojHeadless {
	param(
		[string]$Path
	)

	$content = Get-Content -Path $Path -Raw
	return $content -match "GameProjectConfig HEADLESS"
}

Assert-PathExists -Path $coreGproj -Label "Core addon.gproj"
Assert-PathExists -Path $overlayGproj -Label "Overlay addon.gproj"
Assert-PathExists -Path $directorScripts -Label "Director scripts"

$results = @()
$results += [pscustomobject]@{
	Check = "Core addon exposes HEADLESS config"
	Pass = Test-GprojHeadless -Path $coreGproj
	Details = $coreGproj
}
$results += [pscustomobject]@{
	Check = "Overlay addon exposes HEADLESS config"
	Pass = Test-GprojHeadless -Path $overlayGproj
	Details = $overlayGproj
}

foreach ($pattern in $bannedPatterns) {
	$matches = rg -n --glob "*.c" --glob "*.et" --glob "*.conf" $pattern $directorScripts 2>$null
	$results += [pscustomobject]@{
		Check = "No banned editor-only reference: $pattern"
		Pass = [string]::IsNullOrWhiteSpace($matches)
		Details = if ($matches) { ($matches -split "`r?`n")[0] } else { "clear" }
	}
}

$failures = $results | Where-Object { -not $_.Pass }
$results | Format-Table -AutoSize

if ($failures) {
	throw ("Headless readiness audit failed: " + ($failures.Check -join "; "))
}
