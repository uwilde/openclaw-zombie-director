param(
	[string]$WorkbenchAddonsRoot = "$env:USERPROFILE\Documents\My games\ArmaReforgerWorkbench\addons",
	[string]$CoreProject = "X:\OpenClaw\workspace\projects\reforger-zombie-director\addon.gproj",
	[string]$OverlayProject = "X:\OpenClaw\workspace\projects\reforger-zombie-director-dead-everon-overlay\addon.gproj",
	[string]$LogPath,
	[switch]$AsJson
)

$baseGameGuid = "58D0FB3206B6F859"
$deadEveronGuid = "5D58A217A9611AFB"
$baconGuid = "622120A5448725E3"

function Get-GprojInfo {
	param(
		[Parameter(Mandatory = $true)]
		[string]$Path
	)

	if (!(Test-Path -LiteralPath $Path)) {
		throw "Missing gproj: $Path"
	}

	$info = [ordered]@{
		Path = $Path
		Id = $null
		Guid = $null
		Title = $null
		Dependencies = @()
	}

	$inDependencies = $false
	foreach ($line in Get-Content -LiteralPath $Path) {
		$trim = $line.Trim()
		if ($trim -match '^ID\s+"([^"]+)"') {
			$info.Id = $matches[1]
			continue
		}

		if ($trim -match '^GUID\s+"([^"]+)"') {
			$info.Guid = $matches[1]
			continue
		}

		if ($trim -match '^TITLE\s+"([^"]+)"') {
			$info.Title = $matches[1]
			continue
		}

		if ($trim -match '^Dependencies\s*\{') {
			$inDependencies = $true
			continue
		}

		if ($inDependencies) {
			if ($trim -eq '}') {
				$inDependencies = $false
				continue
			}

			foreach ($match in [regex]::Matches($trim, '"([0-9A-F]+)"')) {
				$info.Dependencies += $match.Groups[1].Value
			}
		}
	}

	[pscustomobject]$info
}

function Get-AddonInfoByGuid {
	param(
		[Parameter(Mandatory = $true)]
		[string]$Guid
	)

	if ($Guid -eq $baseGameGuid) {
		return [pscustomobject]@{
			Path = "Arma Reforger Tools\Workbench\addons\data"
			Id = "data"
			Guid = $baseGameGuid
			Title = "Base game data addon"
			Dependencies = @()
		}
	}

	$directory = Get-ChildItem -LiteralPath $WorkbenchAddonsRoot -Directory |
		Where-Object { $_.Name -match "_$Guid$" } |
		Select-Object -First 1

	if (!$directory) {
		return [pscustomobject]@{
			Path = $null
			Id = $null
			Guid = $Guid
			Title = "Missing addon $Guid"
			Dependencies = @()
		}
	}

	$gprojPath = Join-Path $directory.FullName "addon.gproj"
	if (Test-Path -LiteralPath $gprojPath) {
		return Get-GprojInfo -Path $gprojPath
	}

	return [pscustomobject]@{
		Path = $directory.FullName
		Id = $null
		Guid = $Guid
		Title = $directory.Name
		Dependencies = @()
	}
}

function Get-LatestWorkbenchLog {
	$logsRoot = Join-Path $env:USERPROFILE "Documents\My games\ArmaReforgerWorkbench\logs"
	$latestDirectory = Get-ChildItem -LiteralPath $logsRoot -Directory |
		Sort-Object LastWriteTime -Descending |
		Select-Object -First 1

	if (!$latestDirectory) {
		throw "Could not find a Workbench log directory under $logsRoot"
	}

	$latestLog = Join-Path $latestDirectory.FullName "console.log"
	if (!(Test-Path -LiteralPath $latestLog)) {
		throw "Could not find console.log under $($latestDirectory.FullName)"
	}

	return $latestLog
}

function Get-Examples {
	param(
		[Parameter(Mandatory = $true)]
		[AllowEmptyString()]
		[string[]]$Lines,
		[Parameter(Mandatory = $true)]
		[string]$Pattern,
		[int]$MaxCount = 3
	)

	return $Lines |
		Where-Object { $_ -match $Pattern } |
		Select-Object -Unique -First $MaxCount
}

function Get-DependencyClosure {
	param(
		[Parameter(Mandatory = $true)]
		[string[]]$RootGuids
	)

	$queue = [System.Collections.Generic.Queue[string]]::new()
	$visited = [System.Collections.Generic.HashSet[string]]::new()
	$results = [System.Collections.Generic.List[object]]::new()

	foreach ($guid in $RootGuids) {
		$queue.Enqueue($guid)
	}

	while ($queue.Count -gt 0) {
		$guid = $queue.Dequeue()
		if (!$visited.Add($guid)) {
			continue
		}

		$addon = Get-AddonInfoByGuid -Guid $guid
		$results.Add($addon)
		foreach ($dependencyGuid in $addon.Dependencies) {
			if ($dependencyGuid -ne $baseGameGuid) {
				$queue.Enqueue($dependencyGuid)
			}
		}
	}

	return $results
}

if (!$LogPath) {
	$LogPath = Get-LatestWorkbenchLog
}

$core = Get-GprojInfo -Path $CoreProject
$overlay = Get-GprojInfo -Path $OverlayProject
$deadEveron = Get-AddonInfoByGuid -Guid $deadEveronGuid
$bacon = Get-AddonInfoByGuid -Guid $baconGuid
$deadEveronTransitives = @(Get-DependencyClosure -RootGuids $deadEveron.Dependencies)

$allowedGuids = @(
	$baseGameGuid
	$core.Guid
	$overlay.Guid
	$deadEveron.Guid
	$bacon.Guid
) + @($deadEveronTransitives | ForEach-Object { $_.Guid })

$logLines = Get-Content -LiteralPath $LogPath

$knownByName = @{}
foreach ($item in @(
	(Get-AddonInfoByGuid -Guid $baseGameGuid),
	$core,
	$overlay,
	$deadEveron,
	$bacon
) + $deadEveronTransitives) {
	if ($item.Title) {
		$knownByName[$item.Title] = $item
	}
	if ($item.Id) {
		$knownByName[$item.Id] = $item
	}
}

$mountedAddons = @{}
foreach ($line in $logLines) {
	if ($line -match "FileSystem: Adding package '([^']+)' \(pak count: \d+\) to filesystem under name (.+)$") {
		$path = $matches[1]
		$name = $matches[2].Trim()
		$known = $knownByName[$name]
		$key = if ($known) { $known.Guid } else { $name }
		if (!$mountedAddons.ContainsKey($key)) {
			$mountedAddons[$key] = [pscustomobject]@{
				Name = $name
				Guid = if ($known) { $known.Guid } else { $null }
				Path = $path
			}
		}
		continue
	}

	if ($line -match "FileSystem: Adding relative directory '([^']+)' to filesystem under name (.+)$") {
		$path = $matches[1]
		$name = $matches[2].Trim()
		$known = $knownByName[$name]
		$key = if ($known) { $known.Guid } else { $name }
		if (!$mountedAddons.ContainsKey($key)) {
			$mountedAddons[$key] = [pscustomobject]@{
				Name = $name
				Guid = if ($known) { $known.Guid } else { $null }
				Path = $path
			}
		}
	}
}

$unexpectedLoadedAddons = @(
	$mountedAddons.Values |
		Where-Object {
			if ([string]::IsNullOrWhiteSpace($_.Name)) {
				return $false
			}

			if ($_.Guid) {
				return $_.Guid -notin $allowedGuids
			}

			return $_.Name -notin @("profile", "logs", "ArmaReforger", "core")
		} |
		Sort-Object Name, Guid
)

$findings = @(
	[pscustomobject]@{
		Id = "resource_db_scan"
		Status = "cosmetic"
		Count = @($logLines | Where-Object { $_ -match 'metafile without corresponding resource|resource not registered|duplicate GUID found' }).Count
		Impact = "Safe to ignore for the first playable pass unless you need to edit the affected upstream assets."
		Examples = @(Get-Examples -Lines $logLines -Pattern 'metafile without corresponding resource|resource not registered|duplicate GUID found')
	},
	[pscustomobject]@{
		Id = "asset_binding"
		Status = "watch"
		Count = @($logLines | Where-Object { $_ -match 'Wrong GUID/name for resource|Wrong GUID for resource|Failed to open|Can''t remap' }).Count
		Impact = "World load succeeds, but some props or materials may render incorrectly or lose specific sub-assets."
		Examples = @(Get-Examples -Lines $logLines -Pattern 'Wrong GUID/name for resource|Wrong GUID for resource|Failed to open|Can''t remap')
	},
	[pscustomobject]@{
		Id = "navmesh_links"
		Status = "pathing_risk"
		Count = @($logLines | Where-Object { $_ -match 'NavmeshCustomLinkComponent couldn''t find dependent class' }).Count
		Impact = "Can affect AI traversal around the affected door prefabs. This matters for zombie routing near those structures."
		Examples = @(Get-Examples -Lines $logLines -Pattern 'NavmeshCustomLinkComponent couldn''t find dependent class')
	},
	[pscustomobject]@{
		Id = "component_conflicts"
		Status = "interaction_risk"
		Count = @($logLines | Where-Object { $_ -match 'component MeshObject cannot be combined|component Hierarchy cannot be combined|component RplComponent cannot be combined|Destructible .*doesn''t have a initial and final phase|SCR_DestructionMultiPhaseComponent can''t be attached' }).Count
		Impact = "Likely local to specific props, destructibles, or replicated entities. Treat affected areas as suspect during smoke tests."
		Examples = @(Get-Examples -Lines $logLines -Pattern 'component MeshObject cannot be combined|component Hierarchy cannot be combined|component RplComponent cannot be combined|Destructible .*doesn''t have a initial and final phase|SCR_DestructionMultiPhaseComponent can''t be attached')
	},
	[pscustomobject]@{
		Id = "interactable_actions"
		Status = "interaction_risk"
		Count = @($logLines | Where-Object { $_ -match 'no ActionsManagerComponent present' }).Count
		Impact = "Some doors or gates may not behave correctly when interacted with. This is relevant if encounter design depends on those openings."
		Examples = @(Get-Examples -Lines $logLines -Pattern 'no ActionsManagerComponent present')
	}
)

$report = [pscustomobject]@{
	GeneratedAt = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss zzz")
	LogPath = $LogPath
	DirectStack = @(
		(Get-AddonInfoByGuid -Guid $baseGameGuid),
		$core,
		$overlay,
		$deadEveron,
		$bacon
	)
	DeadEveronTransitiveStack = $deadEveronTransitives
	UnexpectedLoadedAddons = $unexpectedLoadedAddons
	Findings = $findings
	Verdict = if ($unexpectedLoadedAddons.Count -eq 0) {
		"No unexpected workshop addons were mounted beyond the Dead Everon transitive dependency chain."
	} else {
		"Unexpected addon mounts were detected. Review UnexpectedLoadedAddons before the next smoke pass."
	}
}

if ($AsJson) {
	$report | ConvertTo-Json -Depth 6
	return
}

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("# Minimal Stack Audit")
$lines.Add("")
$lines.Add("Generated: $($report.GeneratedAt)")
$lines.Add("")
$lines.Add("Log: ``$($report.LogPath)``")
$lines.Add("")
$lines.Add("## Direct stack")
$lines.Add("")
foreach ($item in $report.DirectStack) {
	$label = if ($item.Title) { $item.Title } elseif ($item.Id) { $item.Id } else { $item.Guid }
	$lines.Add("- ``$label`` ($($item.Guid))")
}

$lines.Add("")
$lines.Add("## Dead Everon transitive stack")
$lines.Add("")
foreach ($item in $report.DeadEveronTransitiveStack) {
	$label = if ($item.Title) { $item.Title } elseif ($item.Id) { $item.Id } else { $item.Guid }
	$lines.Add("- ``$label`` ($($item.Guid))")
}

$lines.Add("")
$lines.Add("## Unexpected loaded addons")
$lines.Add("")
if ($report.UnexpectedLoadedAddons.Count -eq 0) {
	$lines.Add("- none")
} else {
	foreach ($item in $report.UnexpectedLoadedAddons) {
		$lines.Add("- ``$($item.Guid)`` from ``$($item.Path)``")
	}
}

$lines.Add("")
$lines.Add("## Findings")
$lines.Add("")
foreach ($finding in $report.Findings) {
	$lines.Add("- ``$($finding.Id)``: status ``$($finding.Status)``, count ``$($finding.Count)``")
	$lines.Add("  Impact: $($finding.Impact)")
	foreach ($example in $finding.Examples) {
		$lines.Add("  Example: ``$example``")
	}
}

$lines.Add("")
$lines.Add("## Verdict")
$lines.Add("")
$lines.Add($report.Verdict)

$lines -join [Environment]::NewLine
