[CmdletBinding()]
param(
	[switch]$Play,
	[switch]$ForceRestart
)

$ErrorActionPreference = "Stop"

$workbenchExe = "C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe"
$gprojPath = "X:\OpenClaw\workspace\projects\reforger-zombie-director-dead-everon-overlay\addon.gproj"
$overlayWindowTitle = "Enfusion Workbench - OpenClaw Zombie Director - Dead Everon Overlay"
$worldWindowTitle = "World Editor - OpenClaw-Zombie-Director-Dead-Everon/Worlds/DeadEveronDirector/DeadEveronDirector.ent"

function Wait-ForWorkbenchWindow {
	param(
		[string]$TitleLike,
		[int]$TimeoutSeconds = 45
	)

	$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
	do {
		$process = Get-Process | Where-Object {
			$_.ProcessName -like "ArmaReforgerWorkbench*" -and
			$_.MainWindowTitle -like "*$TitleLike*"
		} | Select-Object -First 1

		if ($process) {
			return $process
		}

		Start-Sleep -Milliseconds 500
	} while ((Get-Date) -lt $deadline)

	throw "Timed out waiting for Workbench window like '$TitleLike'."
}

function Initialize-UiAutomation {
	Add-Type -AssemblyName UIAutomationClient
	Add-Type -AssemblyName UIAutomationTypes
}

function Get-WindowRoot {
	param([int]$ProcessId)

	$process = Get-Process -Id $ProcessId
	return [System.Windows.Automation.AutomationElement]::FromHandle($process.MainWindowHandle)
}

function Find-TopLevelWindow {
	param(
		[string]$WindowTitle
	)

	$condition = New-Object System.Windows.Automation.PropertyCondition(
		[System.Windows.Automation.AutomationElement]::NameProperty,
		$WindowTitle
	)

	try {
		return [System.Windows.Automation.AutomationElement]::RootElement.FindFirst(
			[System.Windows.Automation.TreeScope]::Children,
			$condition
		)
	} catch {
		return $null
	}
}

function Invoke-DescendantButton {
	param(
		[System.Windows.Automation.AutomationElement]$Root,
		[string]$ButtonName
	)

	$buttonCondition = New-Object System.Windows.Automation.AndCondition(
		(New-Object System.Windows.Automation.PropertyCondition(
			[System.Windows.Automation.AutomationElement]::ControlTypeProperty,
			[System.Windows.Automation.ControlType]::Button
		)),
		(New-Object System.Windows.Automation.PropertyCondition(
			[System.Windows.Automation.AutomationElement]::NameProperty,
			$ButtonName
		))
	)

	try {
		$button = $Root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $buttonCondition)
	} catch {
		return $false
	}
	if (-not $button) {
		return $false
	}

	try {
		$invoke = [System.Windows.Automation.InvokePattern]$button.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
		$invoke.Invoke()
		return $true
	} catch {
		return $false
	}
}

function Approve-ScriptAuthorizationPrompt {
	param(
		[int]$TimeoutSeconds = 20
	)

	$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
	do {
		$prompt = Find-TopLevelWindow -WindowTitle "Script Authorization Required"
		if (-not $prompt) {
			return $true
		}

		if (Invoke-DescendantButton -Root $prompt -ButtonName "Yes to All") {
			Start-Sleep -Milliseconds 500
			continue
		}

		Start-Sleep -Milliseconds 500
	} while ((Get-Date) -lt $deadline)

	Write-Warning "Timed out waiting for 'Script Authorization Required' to clear automatically."
	return $false
}

function Start-PlayMode {
	param(
		[System.Windows.Automation.AutomationElement]$WorldRoot,
		[int]$WorldProcessId
	)

	if (Invoke-DescendantButton -Root $WorldRoot -ButtonName "Play Game Button") {
		return
	}

	# Toolbar indexing drifts between Workbench versions. Falling back to F5 is more stable.
	Add-Type -AssemblyName Microsoft.VisualBasic
	[Microsoft.VisualBasic.Interaction]::AppActivate($WorldProcessId) | Out-Null
	Start-Sleep -Milliseconds 400

	$wsh = New-Object -ComObject WScript.Shell
	$wsh.SendKeys("{F5}")
}

function Invoke-MenuItem {
	param(
		[System.Windows.Automation.AutomationElement]$Root,
		[string]$MenuName,
		[string]$ItemName
	)

	$menuCondition = New-Object System.Windows.Automation.AndCondition(
		(New-Object System.Windows.Automation.PropertyCondition(
			[System.Windows.Automation.AutomationElement]::ControlTypeProperty,
			[System.Windows.Automation.ControlType]::MenuItem
		)),
		(New-Object System.Windows.Automation.PropertyCondition(
			[System.Windows.Automation.AutomationElement]::NameProperty,
			$MenuName
		))
	)

	try {
		$menu = $Root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $menuCondition)
	} catch {
		throw "Menu '$MenuName' could not be queried."
	}
	if (-not $menu) {
		throw "Menu '$MenuName' not found."
	}

	$expand = [System.Windows.Automation.ExpandCollapsePattern]$menu.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern)
	$expand.Expand()
	Start-Sleep -Milliseconds 700

	$itemCondition = New-Object System.Windows.Automation.AndCondition(
		(New-Object System.Windows.Automation.PropertyCondition(
			[System.Windows.Automation.AutomationElement]::ControlTypeProperty,
			[System.Windows.Automation.ControlType]::MenuItem
		)),
		(New-Object System.Windows.Automation.PropertyCondition(
			[System.Windows.Automation.AutomationElement]::NameProperty,
			$ItemName
		))
	)

	try {
		$item = $Root.FindFirst([System.Windows.Automation.TreeScope]::Descendants, $itemCondition)
	} catch {
		throw "Menu item '$ItemName' under '$MenuName' could not be queried."
	}
	if (-not $item) {
		throw "Menu item '$ItemName' not found under '$MenuName'."
	}

	$invoke = [System.Windows.Automation.InvokePattern]$item.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
	$invoke.Invoke()
}

if (-not (Test-Path -LiteralPath $workbenchExe)) {
	throw "Workbench executable not found at '$workbenchExe'."
}

if (-not (Test-Path -LiteralPath $gprojPath)) {
	throw "Overlay project not found at '$gprojPath'."
}

if ($ForceRestart) {
	Get-Process | Where-Object { $_.ProcessName -like "ArmaReforgerWorkbench*" } | Stop-Process -Force
	Start-Sleep -Seconds 2
}

$existing = Get-Process | Where-Object {
	$_.ProcessName -like "ArmaReforgerWorkbench*" -and
	(
		$_.MainWindowTitle -like "*$overlayWindowTitle*" -or
		$_.MainWindowTitle -like "*$worldWindowTitle*"
	)
} | Select-Object -First 1

if (-not $existing) {
	Start-Process -FilePath $workbenchExe -ArgumentList @(
		"-gproj",
		$gprojPath,
		"-logLevel",
		"verbose",
		"-nothrow",
		"-noCrashDialog"
	) | Out-Null
}

$workbench = $existing
if (-not $workbench) {
	$workbench = Wait-ForWorkbenchWindow -TitleLike $overlayWindowTitle
}

Initialize-UiAutomation
$world = $null
if ($workbench.MainWindowTitle -like "*$worldWindowTitle*") {
	$world = $workbench
} else {
	$root = Get-WindowRoot -ProcessId $workbench.Id
	Invoke-MenuItem -Root $root -MenuName "File" -ItemName "DeadEveronDirector.ent"
	$world = Wait-ForWorkbenchWindow -TitleLike $worldWindowTitle
}

if ($Play) {
	Approve-ScriptAuthorizationPrompt -TimeoutSeconds 2 | Out-Null
	$root = Get-WindowRoot -ProcessId $world.Id
	Start-PlayMode -WorldRoot $root -WorldProcessId $world.Id
	Start-Sleep -Seconds 1
	Approve-ScriptAuthorizationPrompt -TimeoutSeconds 20 | Out-Null
}

Write-Output "Workbench PID: $($world.Id)"
Write-Output "Window: $($world.MainWindowTitle)"
