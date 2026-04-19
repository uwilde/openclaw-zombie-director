[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$releaseRoot = $PSScriptRoot
$projectRoot = Split-Path (Split-Path $releaseRoot -Parent) -Parent
$distRoot = Join-Path $releaseRoot "dist"
$zipPath = Join-Path $distRoot "openclaw-zombie-director-public-release.zip"
$stagingRoot = Join-Path $env:TEMP "ocd-public-release-stage"

$windowsBundleZip = Join-Path $projectRoot "deploy\cloud\dist\openclaw-zombie-director-cloud-windows-dedicated-remote-openclaw.zip"
$linuxBundleZip = Join-Path $projectRoot "deploy\cloud\dist\openclaw-zombie-director-cloud-linux-dedicated-remote-openclaw.zip"
$serverQuickStart = Join-Path $projectRoot "docs\server-operator-quick-start.md"
$hostingQuickStart = Join-Path $projectRoot "docs\hosting-panel-quick-start.md"
$publishSafeDefaults = Join-Path $projectRoot "docs\publish-safe-scenario-defaults.md"
$releaseReadme = Join-Path $releaseRoot "README.md"
$startHere = Join-Path $releaseRoot "START HERE.md"
$supported = Join-Path $releaseRoot "SUPPORTED.md"
$publicStack = Join-Path $releaseRoot "PUBLIC STACK.md"
$releaseChecklist = Join-Path $releaseRoot "RELEASE CHECKLIST.md"
$releaseValidation = Join-Path $releaseRoot "RELEASE-VALIDATION.md"
$publishManifest = Join-Path $releaseRoot "PUBLISH MANIFEST.json"
$publishChecklist = Join-Path $releaseRoot "PUBLISH CHECKLIST.md"
$workshopCopy = Join-Path $releaseRoot "workshop-release-description.md"

New-Item -ItemType Directory -Force -Path $distRoot | Out-Null
if (Test-Path -LiteralPath $zipPath) {
	Remove-Item -LiteralPath $zipPath -Force
}

if (Test-Path -LiteralPath $stagingRoot) {
	Remove-Item -LiteralPath $stagingRoot -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $stagingRoot | Out-Null

$items = @(
	@{ Source = $releaseReadme; Destination = "README.md" },
	@{ Source = $startHere; Destination = "START HERE.md" },
	@{ Source = $supported; Destination = "SUPPORTED.md" },
	@{ Source = $publicStack; Destination = "PUBLIC STACK.md" },
	@{ Source = $releaseChecklist; Destination = "RELEASE CHECKLIST.md" },
	@{ Source = $releaseValidation; Destination = "RELEASE-VALIDATION.md" },
	@{ Source = $publishManifest; Destination = "PUBLISH MANIFEST.json" },
	@{ Source = $publishChecklist; Destination = "PUBLISH CHECKLIST.md" },
	@{ Source = $workshopCopy; Destination = "workshop-release-description.md" },
	@{ Source = $serverQuickStart; Destination = "docs\server-operator-quick-start.md" },
	@{ Source = $hostingQuickStart; Destination = "docs\hosting-panel-quick-start.md" },
	@{ Source = $publishSafeDefaults; Destination = "docs\publish-safe-scenario-defaults.md" },
	@{ Source = $windowsBundleZip; Destination = "bundles\openclaw-zombie-director-cloud-windows-dedicated-remote-openclaw.zip" },
	@{ Source = $linuxBundleZip; Destination = "bundles\openclaw-zombie-director-cloud-linux-dedicated-remote-openclaw.zip" }
)

foreach ($item in $items) {
	if (-not (Test-Path -LiteralPath $item.Source)) {
		throw "Missing required release artifact '$($item.Source)'."
	}

	$targetPath = Join-Path $stagingRoot $item.Destination
	New-Item -ItemType Directory -Force -Path (Split-Path $targetPath -Parent) | Out-Null
	Copy-Item -LiteralPath $item.Source -Destination $targetPath -Force
}

Compress-Archive -Path (Join-Path $stagingRoot "*") -DestinationPath $zipPath
Remove-Item -LiteralPath $stagingRoot -Recurse -Force
Write-Output $zipPath
