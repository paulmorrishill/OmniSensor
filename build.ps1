#!/usr/bin/env pwsh
# PowerShell script to build WiFi Temperature Sensor project using PlatformIO
# This script will build the project for all configured environments and generate .bin files

param(
    [string]$Environment = "all",
    [switch]$Clean = $false,
    [switch]$Upload = $false,
    [string]$Port = ""
)

# Colors for output
$Red = "`e[31m"
$Green = "`e[32m"
$Yellow = "`e[33m"
$Blue = "`e[34m"
$Reset = "`e[0m"

function Write-ColorOutput {
    param([string]$Message, [string]$Color = $Reset)
    Write-Host "$Color$Message$Reset"
}

function Test-PlatformIO {
    try {
        $pioVersion = pio --version 2>$null
        if ($LASTEXITCODE -eq 0) {
            Write-ColorOutput "PlatformIO found: $pioVersion" $Green
            return $true
        }
    }
    catch {
        Write-ColorOutput "PlatformIO not found in PATH" $Red
        Write-ColorOutput "Please install PlatformIO CLI or ensure it's in your PATH" $Yellow
        Write-ColorOutput "Installation: https://docs.platformio.org/en/latest/core/installation.html" $Blue
        return $false
    }
    return $false
}

function Get-AvailableEnvironments {
    $environments = @()
    try {
        $pioOutput = pio project config --json-output 2>$null | ConvertFrom-Json
        # The output is an array where each element is [env_name, config_array]
        foreach ($envConfig in $pioOutput) {
            if ($envConfig[0] -match "^env:(.+)") {
                $environments += $matches[1]
            }
        }
    }
    catch {
        # Fallback to parsing platformio.ini manually
        if (Test-Path "platformio.ini") {
            $content = Get-Content "platformio.ini"
            foreach ($line in $content) {
                if ($line -match "^\[env:(.+)\]") {
                    $environments += $matches[1]
                }
            }
        }
    }
    return $environments
}

function Generate-HtmlConstants {
    Write-ColorOutput "`nGenerating HTML constants..." $Blue
    Write-ColorOutput "=" * 50 $Blue
    
    # Check if HTML files exist
    $htmlFiles = Get-ChildItem "html\*.html" -ErrorAction SilentlyContinue
    if ($htmlFiles.Count -eq 0) {
        Write-ColorOutput "No HTML files found in html\ directory" $Yellow
        return $true
    }
    
    # Build command arguments
    $args = @("tools\html_to_header.py", "src\html_constants.h")
    $args += $htmlFiles | ForEach-Object { $_.FullName }
    
    try {
        Write-ColorOutput "Running HTML generator..." $Yellow
        $result = & python @args
        
        if ($LASTEXITCODE -eq 0) {
            Write-ColorOutput "HTML constants generated successfully!" $Green
            return $true
        }
        else {
            Write-ColorOutput "Failed to generate HTML constants" $Red
            return $false
        }
    }
    catch {
        Write-ColorOutput "Error running HTML generator: $_" $Red
        return $false
    }
}

function Build-Environment {
    param([string]$EnvName)
    
    Write-ColorOutput "`nBuilding environment: $EnvName" $Blue
    Write-ColorOutput "=" * 50 $Blue
    
    if ($Clean) {
        Write-ColorOutput "Cleaning environment: $EnvName" $Yellow
        pio run -e $EnvName --target clean
        if ($LASTEXITCODE -ne 0) {
            Write-ColorOutput "Clean failed for $EnvName" $Red
            return $false
        }
    }
    
    Write-ColorOutput "Compiling $EnvName..." $Yellow
    pio run -e $EnvName
    
    if ($LASTEXITCODE -eq 0) {
        Write-ColorOutput "Build successful for $EnvName" $Green
        
        # Check for generated .bin file
        $binPath = ".pio\build\$EnvName\firmware.bin"
        if (Test-Path $binPath) {
            $binSize = (Get-Item $binPath).Length
            Write-ColorOutput "Generated: $binPath ($([math]::Round($binSize/1KB, 2)) KB)" $Green
            
            # Copy to root directory with descriptive name
            $outputBin = "WifiTempSensor_$EnvName.bin"
            Copy-Item $binPath $outputBin -Force
            Write-ColorOutput "Copied to: $outputBin" $Green
        }
        return $true
    }
    else {
        Write-ColorOutput "Build failed for $EnvName" $Red
        return $false
    }
}

function Upload-Firmware {
    param([string]$EnvName, [string]$UploadPort)
    
    Write-ColorOutput "`nUploading to device..." $Blue
    
    $uploadCmd = "pio run -e $EnvName --target upload"
    if ($UploadPort) {
        $uploadCmd += " --upload-port $UploadPort"
    }
    
    Invoke-Expression $uploadCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-ColorOutput "Upload successful!" $Green
    }
    else {
        Write-ColorOutput "Upload failed!" $Red
    }
}

# Main script execution
Write-ColorOutput "WiFi Temperature Sensor Build Script" $Blue
Write-ColorOutput "=" * 50 $Blue

# Check if we're in the right directory
if (-not (Test-Path "platformio.ini")) {
    Write-ColorOutput "platformio.ini not found. Please run this script from the project root directory." $Red
    exit 1
}

# Check PlatformIO installation
if (-not (Test-PlatformIO)) {
    exit 1
}

# Get available environments
$availableEnvs = Get-AvailableEnvironments
if ($availableEnvs.Count -eq 0) {
    Write-ColorOutput "No environments found in platformio.ini" $Red
    exit 1
}

Write-ColorOutput "Available environments: $($availableEnvs -join ', ')" $Yellow

# Generate HTML constants first
if (-not (Generate-HtmlConstants)) {
    Write-ColorOutput "Failed to generate HTML constants. Build aborted." $Red
    exit 1
}

# Determine which environments to build
$envsToBuild = @()
if ($Environment -eq "all") {
    $envsToBuild = $availableEnvs
}
elseif ($Environment -in $availableEnvs) {
    $envsToBuild = @($Environment)
}
else {
    Write-ColorOutput "Environment '$Environment' not found. Available: $($availableEnvs -join ', ')" $Red
    exit 1
}

# Build environments
$successCount = 0
$totalCount = $envsToBuild.Count

foreach ($env in $envsToBuild) {
    if (Build-Environment $env) {
        $successCount++
    }
}

# Summary
Write-ColorOutput "`nBuild Summary" $Blue
Write-ColorOutput "=" * 50 $Blue
Write-ColorOutput "Successful builds: $successCount/$totalCount" $Green

if ($successCount -gt 0) {
    Write-ColorOutput "`nGenerated files:" $Blue
    Get-ChildItem "WifiTempSensor_*.bin" -ErrorAction SilentlyContinue | ForEach-Object {
        $size = [math]::Round($_.Length/1KB, 2)
        Write-ColorOutput "  $($_.Name) ($size KB)" $Green
    }
}

# Upload if requested
if ($Upload -and $successCount -gt 0) {
    if ($envsToBuild.Count -eq 1) {
        Upload-Firmware $envsToBuild[0] $Port
    }
    else {
        Write-ColorOutput "`nMultiple environments built. Please specify which one to upload:" $Yellow
        for ($i = 0; $i -lt $envsToBuild.Count; $i++) {
            Write-ColorOutput "  $($i + 1). $($envsToBuild[$i])" $Yellow
        }
        $choice = Read-Host "Enter choice (1-$($envsToBuild.Count))"
        if ($choice -match '^\d+$' -and [int]$choice -ge 1 -and [int]$choice -le $envsToBuild.Count) {
            Upload-Firmware $envsToBuild[[int]$choice - 1] $Port
        }
    }
}

if ($successCount -eq $totalCount) {
    Write-ColorOutput "`nAll builds completed successfully!" $Green
    exit 0
}
else {
    Write-ColorOutput "`nSome builds failed. Check the output above for details." $Red
    exit 1
}