# Initial setup script for Godot Epic GDExtension (PowerShell)
# Dumps extension API, builds godot-cpp, then builds the GDExtension

Write-Host "=== GodotEpic GDExtension Setup ===" -ForegroundColor Cyan

# Check if Godot is available
if (-not (Get-Command godot -ErrorAction SilentlyContinue)) {
	Write-Error "Godot executable is not installed or not in PATH. Please install Godot before running this script."
	exit 1
}

# Check if scons is available
if (-not (Get-Command scons -ErrorAction SilentlyContinue)) {
    Write-Error "scons not in PATH."; exit 1
}

# Check if EOS SDK is present
$eosIncludePath = Join-Path $PSScriptRoot 'eos_sdk\Include'
if (-not (Test-Path $eosIncludePath) -or -not (Get-ChildItem -Path $eosIncludePath -Filter "*.h" -ErrorAction SilentlyContinue)) {
    Write-Host "" -ForegroundColor Red
    Write-Host "❌ EOS SDK NOT FOUND" -ForegroundColor Red
    Write-Host "" -ForegroundColor Red
    Write-Host "The Epic Online Services SDK is required but not found." -ForegroundColor Yellow
    Write-Host "Please download and extract the EOS SDK to the eos_sdk directory." -ForegroundColor Yellow
    Write-Host "" 
    Write-Host "Instructions:" -ForegroundColor Cyan
    Write-Host "1. Visit https://dev.epicgames.com/" -ForegroundColor White
    Write-Host "2. Create/login to Epic Games Developer account" -ForegroundColor White
    Write-Host "3. Download the Epic Online Services SDK" -ForegroundColor White
    Write-Host "4. Extract it to: $PSScriptRoot\eos_sdk" -ForegroundColor White
    Write-Host "5. Run this script again" -ForegroundColor White
    Write-Host ""
    Write-Host "For detailed instructions, see:" -ForegroundColor Cyan
    Write-Host "- README.md in the project root" -ForegroundColor White
    Write-Host "- GUIDE.md for complete setup instructions" -ForegroundColor White
    Write-Host ""
    exit 1
}

Write-Host "✅ EOS SDK found" -ForegroundColor Green

# Ensure godot-cpp is initialized
$cppPath = Join-Path $PSScriptRoot 'godot-cpp'
$helper = Join-Path $PSScriptRoot 'setup_godot_cpp_module.ps1'

# Run the helper if godot-cpp is missing or empty
if (-not (Test-Path $cppPath) -or -not (Get-ChildItem -Path $cppPath -File -Recurse -ErrorAction SilentlyContinue)) {
    if (-not (Test-Path $helper)) { Write-Error "Helper script missing."; exit 1 }
    Write-Host "⚙️ Setting up godot-cpp..." -ForegroundColor Yellow
    & $helper
    if (-not $? -or -not (Test-Path $cppPath)) { Write-Error "Helper failed."; exit 1 }
}

Write-Host "✅ godot-cpp ready" -ForegroundColor Green

# Step 1: Dump the extension API
Write-Host "⚙️ Dumping extension API..." -ForegroundColor Yellow
godot --dump-extension-api

if (-not (Test-Path "extension_api.json")) {
    Write-Error "Failed to generate extension_api.json"
    exit 1
}
Write-Host "✅ Extension API generated" -ForegroundColor Green

# Step 2: Build godot-cpp
Write-Host "⚙️ Building godot-cpp..." -ForegroundColor Yellow
Push-Location "godot-cpp"
scons platform=windows custom_api_file=../extension_api.json
if (-not $?) {
    Pop-Location
    Write-Error "Failed to build godot-cpp"
    exit 1
}
Pop-Location
Write-Host "✅ godot-cpp built successfully" -ForegroundColor Green

# Step 3: Build GDExtension
Write-Host "⚙️ Building Godot Epic GDExtension..." -ForegroundColor Yellow
scons platform=windows target=template_debug

if ($?) {
    Write-Host ""
    Write-Host "🎉 Setup complete!" -ForegroundColor Green
    Write-Host "✅ GodotEpic extension built successfully" -ForegroundColor Green
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Cyan
    Write-Host "1. Copy the built extension to your Godot project" -ForegroundColor White
    Write-Host "2. Enable the plugin in Project Settings" -ForegroundColor White
    Write-Host "3. Configure your EOS credentials" -ForegroundColor White
    Write-Host ""
    Write-Host "For usage instructions, see GUIDE.md" -ForegroundColor Cyan
} else {
    Write-Host ""
    Write-Host "❌ Build failed!" -ForegroundColor Red
    Write-Host "Check the error messages above for details." -ForegroundColor Yellow
    exit 1
}
