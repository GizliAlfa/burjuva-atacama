# STM32 HAL Kütüphanesi Kurulum Scripti
# Tarih: 16 Kasım 2025

Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "STM32 HAL Kütüphanesi Kurulumu" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

$ErrorActionPreference = "Stop"

# Proje dizinini kontrol et
if (-not (Test-Path "Makefile")) {
    Write-Host "HATA: Bu scripti stm32-firmware-beta klasöründe çalıştırın!" -ForegroundColor Red
    exit 1
}

# Drivers klasörü zaten var mı kontrol et
if (Test-Path "Drivers") {
    Write-Host "⚠️  Drivers klasörü zaten mevcut." -ForegroundColor Yellow
    $response = Read-Host "Yeniden indirmek ister misiniz? (y/n)"
    if ($response -ne "y") {
        Write-Host "İşlem iptal edildi." -ForegroundColor Yellow
        exit 0
    }
    Write-Host "Eski Drivers klasörü siliniyor..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "Drivers"
}

# Git kurulu mu kontrol et
try {
    $null = git --version
} catch {
    Write-Host "HATA: Git bulunamadı! Git'i yükleyin: https://git-scm.com/download/win" -ForegroundColor Red
    exit 1
}

Write-Host "📦 STM32CubeF1 kütüphanesi indiriliyor..." -ForegroundColor Green
Write-Host "   (Bu işlem birkaç dakika sürebilir)" -ForegroundColor Gray

# STM32CubeF1'i shallow clone yap
try {
    git clone --depth 1 https://github.com/STMicroelectronics/STM32CubeF1.git temp_cube
} catch {
    Write-Host "HATA: GitHub'dan indirme başarısız!" -ForegroundColor Red
    exit 1
}

Write-Host "📂 Gerekli dosyalar kopyalanıyor..." -ForegroundColor Green

# Drivers klasörünü oluştur
New-Item -ItemType Directory -Force -Path "Drivers" | Out-Null

# HAL Driver'ı kopyala
Copy-Item -Recurse "temp_cube/Drivers/STM32F1xx_HAL_Driver" "Drivers/"
Write-Host "   ✓ STM32F1xx_HAL_Driver kopyalandı" -ForegroundColor Green

# CMSIS'i kopyala
Copy-Item -Recurse "temp_cube/Drivers/CMSIS" "Drivers/"
Write-Host "   ✓ CMSIS kopyalandı" -ForegroundColor Green

# Geçici klasörü sil
Write-Host "🧹 Geçici dosyalar temizleniyor..." -ForegroundColor Green
Remove-Item -Recurse -Force "temp_cube"

# Kontrol et
$halExists = Test-Path "Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal.h"
$cmsisExists = Test-Path "Drivers/CMSIS/Include/core_cm3.h"

Write-Host ""
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "Kurulum Tamamlandı!" -ForegroundColor Green
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

if ($halExists -and $cmsisExists) {
    Write-Host "✅ HAL Driver: OK" -ForegroundColor Green
    Write-Host "✅ CMSIS: OK" -ForegroundColor Green
    Write-Host ""
    Write-Host "🚀 Şimdi derleme yapabilirsiniz:" -ForegroundColor Yellow
    Write-Host "   make clean" -ForegroundColor White
    Write-Host "   make" -ForegroundColor White
} else {
    Write-Host "⚠️  Bazı dosyalar eksik!" -ForegroundColor Yellow
    Write-Host "HAL: $halExists" -ForegroundColor Gray
    Write-Host "CMSIS: $cmsisExists" -ForegroundColor Gray
}

Write-Host ""
