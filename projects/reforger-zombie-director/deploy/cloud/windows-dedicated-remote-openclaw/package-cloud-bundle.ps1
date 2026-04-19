[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

$bundleRoot = $PSScriptRoot
$distRoot = Join-Path (Split-Path $bundleRoot -Parent) "dist"
$zipPath = Join-Path $distRoot "openclaw-zombie-director-cloud-windows-dedicated-remote-openclaw.zip"
$stagingRoot = Join-Path $env:TEMP "ocd-cloud-bundle-stage-windows"

New-Item -ItemType Directory -Force -Path $distRoot | Out-Null
if (Test-Path -LiteralPath $zipPath) {
	Remove-Item -Path $zipPath -Force
}

if (Test-Path -LiteralPath $stagingRoot) {
	Remove-Item -Path $stagingRoot -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $stagingRoot | Out-Null

$excludePatterns = @(
	"*.generated",
	"*.generated.*",
	"director-bridge.config.json"
)

Get-ChildItem -Path $bundleRoot -Recurse -File | Where-Object {
	$relative = $_.FullName.Substring($bundleRoot.Length).TrimStart('\')
	foreach ($pattern in $excludePatterns) {
		if ($relative -like $pattern -or $_.Name -like $pattern) {
			return $false
		}
	}

	return $true
} | ForEach-Object {
	$relative = $_.FullName.Substring($bundleRoot.Length).TrimStart('\')
	$targetPath = Join-Path $stagingRoot $relative
	New-Item -ItemType Directory -Force -Path (Split-Path $targetPath -Parent) | Out-Null
	Copy-Item -LiteralPath $_.FullName -Destination $targetPath -Force
}

Compress-Archive -Path (Join-Path $stagingRoot "*") -DestinationPath $zipPath
Remove-Item -Path $stagingRoot -Recurse -Force
Write-Output $zipPath
