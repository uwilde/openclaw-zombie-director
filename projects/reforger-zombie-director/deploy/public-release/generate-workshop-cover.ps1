[CmdletBinding()]
param(
  [string]$OutputPath = "X:\OpenClaw\workspace\projects\reforger-zombie-director\deploy\public-release\assets\openclaw-zombie-director-workshop-cover.png",
  [int]$Width = 1920,
  [int]$Height = 1080
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Add-Type -AssemblyName PresentationCore
Add-Type -AssemblyName PresentationFramework
Add-Type -AssemblyName WindowsBase

$outputDir = Split-Path -Parent $OutputPath
if (-not (Test-Path $outputDir)) {
  New-Item -ItemType Directory -Path $outputDir | Out-Null
}

function New-Text {
  param(
    [string]$Text,
    [double]$FontSize,
    [string]$FontFamily = "Segoe UI",
    [System.Windows.FontWeight]$FontWeight = [System.Windows.FontWeights]::Normal,
    [System.Windows.Media.Brush]$Brush,
    [double]$PixelsPerDip = 1.0
  )

  return [System.Windows.Media.FormattedText]::new(
    $Text,
    [System.Globalization.CultureInfo]::InvariantCulture,
    [System.Windows.FlowDirection]::LeftToRight,
    [System.Windows.Media.Typeface]::new(
      [System.Windows.Media.FontFamily]::new($FontFamily),
      [System.Windows.FontStyles]::Normal,
      $FontWeight,
      [System.Windows.FontStretches]::Normal
    ),
    $FontSize,
    $Brush,
    $PixelsPerDip
  )
}

$visual = [System.Windows.Media.DrawingVisual]::new()
$dc = $visual.RenderOpen()

$background = [System.Windows.Media.LinearGradientBrush]::new()
$background.StartPoint = [System.Windows.Point]::new(0, 0)
$background.EndPoint = [System.Windows.Point]::new(1, 1)
$background.GradientStops.Add([System.Windows.Media.GradientStop]::new([System.Windows.Media.Color]::FromRgb(10, 17, 24), 0.0))
$background.GradientStops.Add([System.Windows.Media.GradientStop]::new([System.Windows.Media.Color]::FromRgb(18, 31, 42), 0.45))
$background.GradientStops.Add([System.Windows.Media.GradientStop]::new([System.Windows.Media.Color]::FromRgb(9, 13, 18), 1.0))
$dc.DrawRectangle($background, $null, [System.Windows.Rect]::new(0, 0, $Width, $Height))

$redGlow = [System.Windows.Media.RadialGradientBrush]::new()
$redGlow.Center = [System.Windows.Point]::new(0.2, 0.18)
$redGlow.GradientOrigin = $redGlow.Center
$redGlow.RadiusX = 0.42
$redGlow.RadiusY = 0.42
$redGlow.GradientStops.Add([System.Windows.Media.GradientStop]::new([System.Windows.Media.Color]::FromArgb(170, 193, 43, 43), 0.0))
$redGlow.GradientStops.Add([System.Windows.Media.GradientStop]::new([System.Windows.Media.Color]::FromArgb(0, 193, 43, 43), 1.0))
$dc.DrawRectangle($redGlow, $null, [System.Windows.Rect]::new(0, 0, $Width, $Height))

$tealGlow = [System.Windows.Media.RadialGradientBrush]::new()
$tealGlow.Center = [System.Windows.Point]::new(0.83, 0.78)
$tealGlow.GradientOrigin = $tealGlow.Center
$tealGlow.RadiusX = 0.36
$tealGlow.RadiusY = 0.36
$tealGlow.GradientStops.Add([System.Windows.Media.GradientStop]::new([System.Windows.Media.Color]::FromArgb(100, 34, 115, 123), 0.0))
$tealGlow.GradientStops.Add([System.Windows.Media.GradientStop]::new([System.Windows.Media.Color]::FromArgb(0, 34, 115, 123), 1.0))
$dc.DrawRectangle($tealGlow, $null, [System.Windows.Rect]::new(0, 0, $Width, $Height))

$gridPen = [System.Windows.Media.Pen]::new(
  [System.Windows.Media.SolidColorBrush]::new([System.Windows.Media.Color]::FromArgb(28, 255, 255, 255)),
  1
)
for ($x = 0; $x -lt $Width; $x += 96) {
  $dc.DrawLine($gridPen, [System.Windows.Point]::new($x, 0), [System.Windows.Point]::new($x, $Height))
}
for ($y = 0; $y -lt $Height; $y += 96) {
  $dc.DrawLine($gridPen, [System.Windows.Point]::new(0, $y), [System.Windows.Point]::new($Width, $y))
}

$slashPen = [System.Windows.Media.Pen]::new(
  [System.Windows.Media.LinearGradientBrush]::new(
    [System.Windows.Media.Color]::FromArgb(220, 255, 77, 77),
    [System.Windows.Media.Color]::FromArgb(80, 255, 145, 0),
    30
  ),
  18
)
$slashPen.StartLineCap = [System.Windows.Media.PenLineCap]::Round
$slashPen.EndLineCap = [System.Windows.Media.PenLineCap]::Round

$slashBaseX = $Width - 420
$slashBaseY = 160
foreach ($offset in 0, 44, 88, 132) {
  $dc.DrawLine(
    $slashPen,
    [System.Windows.Point]::new($slashBaseX + $offset, $slashBaseY),
    [System.Windows.Point]::new($slashBaseX - 36 + $offset, $slashBaseY + 250)
  )
}

$badgeRect = [System.Windows.Rect]::new(112, 108, 276, 54)
$badgeBrush = [System.Windows.Media.SolidColorBrush]::new([System.Windows.Media.Color]::FromArgb(220, 184, 42, 42))
$badgePen = [System.Windows.Media.Pen]::new(
  [System.Windows.Media.SolidColorBrush]::new([System.Windows.Media.Color]::FromArgb(255, 255, 174, 174)),
  1.5
)
$dc.DrawRoundedRectangle($badgeBrush, $badgePen, $badgeRect, 10, 10)

$badgeText = New-Text -Text "EXPERIMENTAL" -FontSize 24 -FontFamily "Segoe UI Semibold" -FontWeight ([System.Windows.FontWeights]::Bold) -Brush ([System.Windows.Media.Brushes]::White)
$dc.DrawText($badgeText, [System.Windows.Point]::new(136, 119))

$title1 = New-Text -Text "OpenClaw" -FontSize 92 -FontFamily "Segoe UI Semibold" -FontWeight ([System.Windows.FontWeights]::Bold) -Brush ([System.Windows.Media.Brushes]::White)
$dc.DrawText($title1, [System.Windows.Point]::new(106, 248))

$title2 = New-Text -Text "Zombie Director" -FontSize 118 -FontFamily "Segoe UI Semibold" -FontWeight ([System.Windows.FontWeights]::Bold) -Brush ([System.Windows.Media.SolidColorBrush]::new([System.Windows.Media.Color]::FromRgb(245, 86, 74)))
$dc.DrawText($title2, [System.Windows.Point]::new(100, 336))

$subtitle = New-Text -Text "Zombie encounter director for Arma Reforger" -FontSize 36 -FontFamily "Segoe UI" -Brush ([System.Windows.Media.SolidColorBrush]::new([System.Windows.Media.Color]::FromRgb(213, 223, 229)))
$dc.DrawText($subtitle, [System.Windows.Point]::new(112, 500))

$lineBrush = [System.Windows.Media.LinearGradientBrush]::new(
  [System.Windows.Media.Color]::FromArgb(255, 255, 100, 88),
  [System.Windows.Media.Color]::FromArgb(255, 90, 212, 217),
  0
)
$linePen = [System.Windows.Media.Pen]::new($lineBrush, 6)
$dc.DrawLine($linePen, [System.Windows.Point]::new(112, 564), [System.Windows.Point]::new(950, 564))

$featureBrush = [System.Windows.Media.SolidColorBrush]::new([System.Windows.Media.Color]::FromRgb(198, 206, 210))
$feature1 = New-Text -Text "Works with or without OpenClaw" -FontSize 28 -FontFamily "Segoe UI" -Brush $featureBrush
$feature2 = New-Text -Text "Dead Everon + Bacon Zombies public stack" -FontSize 28 -FontFamily "Segoe UI" -Brush $featureBrush
$feature3 = New-Text -Text "Local servers, dedicated servers, cloud servers" -FontSize 28 -FontFamily "Segoe UI" -Brush $featureBrush
$dc.DrawText($feature1, [System.Windows.Point]::new(118, 620))
$dc.DrawText($feature2, [System.Windows.Point]::new(118, 668))
$dc.DrawText($feature3, [System.Windows.Point]::new(118, 716))

$footerBrush = [System.Windows.Media.SolidColorBrush]::new([System.Windows.Media.Color]::FromArgb(220, 255, 255, 255))
$footerText = New-Text -Text "Experimental beta release • REST bridge optional • Headless-friendly path supported" -FontSize 22 -FontFamily "Segoe UI" -Brush $footerBrush
$dc.DrawText($footerText, [System.Windows.Point]::new(112, $Height - 92))

$vignetteBrush = [System.Windows.Media.RadialGradientBrush]::new()
$vignetteBrush.Center = [System.Windows.Point]::new(0.5, 0.5)
$vignetteBrush.GradientOrigin = $vignetteBrush.Center
$vignetteBrush.RadiusX = 0.9
$vignetteBrush.RadiusY = 0.9
$vignetteBrush.GradientStops.Add([System.Windows.Media.GradientStop]::new([System.Windows.Media.Color]::FromArgb(0, 0, 0, 0), 0.58))
$vignetteBrush.GradientStops.Add([System.Windows.Media.GradientStop]::new([System.Windows.Media.Color]::FromArgb(145, 0, 0, 0), 1.0))
$dc.DrawRectangle($vignetteBrush, $null, [System.Windows.Rect]::new(0, 0, $Width, $Height))

$dc.Close()

$bitmap = [System.Windows.Media.Imaging.RenderTargetBitmap]::new($Width, $Height, 96, 96, [System.Windows.Media.PixelFormats]::Pbgra32)
$bitmap.Render($visual)

$encoder = [System.Windows.Media.Imaging.PngBitmapEncoder]::new()
$encoder.Frames.Add([System.Windows.Media.Imaging.BitmapFrame]::Create($bitmap))

$stream = [System.IO.File]::Create($OutputPath)
try {
  $encoder.Save($stream)
} finally {
  $stream.Dispose()
}

Write-Host "Generated workshop cover: $OutputPath"
