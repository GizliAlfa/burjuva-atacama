# CPLD Hızlı Programlama - PowerShell Komutları
# ================================================

# 1. SSH ile bağlan
Write-Host "Adım 1: SSH ile Raspberry Pi'ye bağlan" -ForegroundColor Cyan
Write-Host "ssh burjuva@192.168.1.22" -ForegroundColor Yellow
Write-Host ""

# 2. Script'i çalıştır
Write-Host "Adım 2: Aşağıdaki komutu çalıştır:" -ForegroundColor Cyan
Write-Host "chmod +x /tmp/quick_program.sh && /tmp/quick_program.sh" -ForegroundColor Yellow
Write-Host ""

# VEYA Direkt OpenOCD komutu
Write-Host "Alternatif: Direkt programlama komutu:" -ForegroundColor Cyan
Write-Host "sudo openocd -f /tmp/openocd_cpld.cfg -c 'svf /tmp/cpld.svf; shutdown'" -ForegroundColor Yellow
Write-Host ""

Write-Host "Beklenen Süre: ~4 saniye" -ForegroundColor Green
Write-Host ""
Write-Host "Beklenen Çıktı:" -ForegroundColor Cyan
Write-Host "  Info : JTAG tap: maxv.tap tap/device found: 0x020a50dd" -ForegroundColor Gray
Write-Host "  Info : svf processing file: /tmp/cpld.svf" -ForegroundColor Gray
Write-Host "  Progress: 0% ... 100%" -ForegroundColor Gray
Write-Host "  Info : svf file programmed successfully" -ForegroundColor Green
Write-Host ""
