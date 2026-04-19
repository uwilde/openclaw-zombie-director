param(
    [string]$SourceAddonRoot = "C:\Users\UWilde\Documents\My Games\ArmaReforgerWorkbench\addons\ArmaReforger_58D0FB3206B6F859",
    [string]$SourceNmnPath = "",
    [string]$SourceNavDataPath = "",
    [string]$SourceBaseName = "",
    [string]$DestinationBaseName = "dead_everon_smoke_soldiers_patch",
    [string]$ProjectRoot = "X:\OpenClaw\workspace\projects\reforger-zombie-director-dead-everon-overlay",
    [switch]$CleanupSource
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-ExistingPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
    return $resolved.Path
}

function Get-LatestGeneratedNmn {
    param([Parameter(Mandatory = $true)][string]$Root)

    $candidate = Get-ChildItem -LiteralPath $Root -File -Filter "*.nmn" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if (-not $candidate) {
        throw "No generated .nmn files were found under '$Root'. Save a navmesh from Workbench first or pass -SourceNmnPath."
    }

    return $candidate
}

$SourceAddonRoot = Resolve-ExistingPath -Path $SourceAddonRoot

if (-not $SourceNmnPath) {
    if ($SourceBaseName) {
        $SourceNmnPath = Join-Path $SourceAddonRoot ($SourceBaseName + ".nmn")
    } else {
        $SourceNmnPath = (Get-LatestGeneratedNmn -Root $SourceAddonRoot).FullName
    }
}

$SourceNmnPath = Resolve-ExistingPath -Path $SourceNmnPath

if (-not $SourceNavDataPath) {
    $SourceBaseName = [System.IO.Path]::GetFileNameWithoutExtension($SourceNmnPath)
    $SourceNavDataPath = Join-Path $SourceAddonRoot (Join-Path ".NavData" $SourceBaseName)
}

$SourceNavDataPath = Resolve-ExistingPath -Path $SourceNavDataPath
$ProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot -ErrorAction Stop).Path

$destinationDir = Join-Path $ProjectRoot "Saves\Navmesh"
$destinationNavDataRoot = Join-Path $destinationDir ".NavData"
$destinationNmnPath = Join-Path $destinationDir ($DestinationBaseName + ".nmn")
$destinationNavDataPath = Join-Path $destinationNavDataRoot $DestinationBaseName

New-Item -ItemType Directory -Path $destinationDir -Force | Out-Null
New-Item -ItemType Directory -Path $destinationNavDataRoot -Force | Out-Null

if (Test-Path -LiteralPath $destinationNavDataPath) {
    Remove-Item -LiteralPath $destinationNavDataPath -Recurse -Force
}

Copy-Item -LiteralPath $SourceNmnPath -Destination $destinationNmnPath -Force
Copy-Item -LiteralPath $SourceNavDataPath -Destination $destinationNavDataPath -Recurse -Force

if ($CleanupSource) {
    Remove-Item -LiteralPath $SourceNmnPath -Force
    Remove-Item -LiteralPath $SourceNavDataPath -Recurse -Force
}

$tileCount = (Get-ChildItem -LiteralPath $destinationNavDataPath -File | Measure-Object).Count
$navSize = (Get-Item -LiteralPath $destinationNmnPath).Length

Write-Output "Staged navmesh:"
Write-Output "  Source NNM:      $SourceNmnPath"
Write-Output "  Source NavData:  $SourceNavDataPath"
Write-Output "  Destination NNM: $destinationNmnPath"
Write-Output "  Destination NavData: $destinationNavDataPath"
Write-Output "  Tile files:      $tileCount"
Write-Output "  Main file bytes: $navSize"
Write-Output ""
Write-Output "Use this resource path in the smoke world:"
Write-Output "  Saves/Navmesh/$DestinationBaseName.nmn"
