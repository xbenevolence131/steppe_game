param(
    [Parameter(Mandatory = $true)]
    [string]$Reference,

    [Parameter(Mandatory = $true)]
    [string]$Name,

    [string]$OutputDir = "public/unit-sprites",

    [int[]]$Sizes = @(48, 96, 128),

    [int]$Threshold = 150,

    [double]$FillFraction = 0.88,

    [byte]$Alpha = 255
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($Threshold -lt 0 -or $Threshold -gt 765) {
    throw "Threshold must be between 0 and 765."
}
if ($FillFraction -le 0.0 -or $FillFraction -gt 1.0) {
    throw "FillFraction must be greater than 0 and no more than 1."
}
if ($Sizes.Count -eq 0) {
    throw "At least one output size is required."
}

Add-Type -AssemblyName System.Drawing

$referencePath = Resolve-Path -LiteralPath $Reference
$outputPath = Join-Path (Get-Location) $OutputDir
if (-not (Test-Path -LiteralPath $outputPath)) {
    New-Item -ItemType Directory -Path $outputPath | Out-Null
}

$source = [System.Drawing.Bitmap]::FromFile($referencePath)
try {
    $minX = $source.Width
    $minY = $source.Height
    $maxX = -1
    $maxY = -1

    for ($y = 0; $y -lt $source.Height; $y += 1) {
        for ($x = 0; $x -lt $source.Width; $x += 1) {
            $pixel = $source.GetPixel($x, $y)
            if ($pixel.A -gt 0 -and ($pixel.R + $pixel.G + $pixel.B) -le $Threshold) {
                if ($x -lt $minX) { $minX = $x }
                if ($y -lt $minY) { $minY = $y }
                if ($x -gt $maxX) { $maxX = $x }
                if ($y -gt $maxY) { $maxY = $y }
            }
        }
    }

    if ($maxX -lt $minX -or $maxY -lt $minY) {
        throw "No dark silhouette pixels found. Try increasing -Threshold."
    }

    $boxWidth = $maxX - $minX + 1
    $boxHeight = $maxY - $minY + 1
    $boxSide = [Math]::Max($boxWidth, $boxHeight)
    $sampleSide = [Math]::Ceiling($boxSide / $FillFraction)
    $centerX = ($minX + $maxX) / 2.0
    $centerY = ($minY + $maxY) / 2.0
    $sampleLeft = $centerX - ($sampleSide / 2.0)
    $sampleTop = $centerY - ($sampleSide / 2.0)

    foreach ($size in $Sizes) {
        if ($size -le 0) {
            throw "Output sizes must be positive integers."
        }

        $target = New-Object System.Drawing.Bitmap $size, $size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            for ($dy = 0; $dy -lt $size; $dy += 1) {
                for ($dx = 0; $dx -lt $size; $dx += 1) {
                    $sx = [Math]::Floor($sampleLeft + (($dx + 0.5) * $sampleSide / $size))
                    $sy = [Math]::Floor($sampleTop + (($dy + 0.5) * $sampleSide / $size))
                    if ($sx -lt 0 -or $sx -ge $source.Width -or $sy -lt 0 -or $sy -ge $source.Height) {
                        $target.SetPixel($dx, $dy, [System.Drawing.Color]::Transparent)
                        continue
                    }

                    $pixel = $source.GetPixel($sx, $sy)
                    if ($pixel.A -gt 0 -and ($pixel.R + $pixel.G + $pixel.B) -le $Threshold) {
                        $target.SetPixel($dx, $dy, [System.Drawing.Color]::FromArgb($Alpha, 0, 0, 0))
                    } else {
                        $target.SetPixel($dx, $dy, [System.Drawing.Color]::Transparent)
                    }
                }
            }

            $fileName = "{0}_{1}.png" -f $Name, $size
            $destination = Join-Path $outputPath $fileName
            $target.Save($destination, [System.Drawing.Imaging.ImageFormat]::Png)
            Write-Output $destination
        } finally {
            $target.Dispose()
        }
    }
} finally {
    $source.Dispose()
}
