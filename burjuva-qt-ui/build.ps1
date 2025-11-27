# Burjuva-Atacama Qt UI Build Script
# For Windows with Qt5 and MinGW

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "Burjuva-Atacama Qt UI Build Script" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

# Check if Qt is in PATH
if (-not (Get-Command qmake -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: Qt5 not found in PATH!" -ForegroundColor Red
    Write-Host "Please add Qt bin directory to PATH, for example:" -ForegroundColor Yellow
    Write-Host "  C:\Qt\5.15.2\mingw81_64\bin" -ForegroundColor Yellow
    exit 1
}

# Create build directory
if (-not (Test-Path "build")) {
    Write-Host "Creating build directory..." -ForegroundColor Green
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Set-Location build

# Run CMake
Write-Host "Running CMake..." -ForegroundColor Green
cmake -G "MinGW Makefiles" ..

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake failed!" -ForegroundColor Red
    exit 1
}

# Build
Write-Host "Building..." -ForegroundColor Green
mingw32-make

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "==========================================" -ForegroundColor Green
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Green
Write-Host "Executable: $((Get-Location).Path)\burjuva-ui.exe" -ForegroundColor Yellow
Write-Host ""
Write-Host "To run:" -ForegroundColor Cyan
Write-Host "  cd build" -ForegroundColor White
Write-Host "  .\burjuva-ui.exe" -ForegroundColor White
Write-Host ""
