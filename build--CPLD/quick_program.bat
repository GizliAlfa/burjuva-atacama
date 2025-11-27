@echo off
REM CPLD Quick Program - Windows Batch
REM ====================================

echo.
echo ============================================================
echo   CPLD HIZLI PROGRAMLAMA
echo ============================================================
echo.

REM Raspberry Pi bilgileri
set PI_USER=burjuva
set PI_HOST=192.168.1.22

echo [1/3] Dosyalar kopyalaniyor...
echo.

REM SVF dosyasini kopyala
scp "%~dp0output_files\cpld.svf" %PI_USER%@%PI_HOST%:/tmp/cpld.svf
if errorlevel 1 (
    echo HATA: cpld.svf kopyalanamadi!
    pause
    exit /b 1
)

REM Config dosyasini kopyala
scp "%~dp0openocd_cpld.cfg" %PI_USER%@%PI_HOST%:/tmp/openocd_cpld.cfg
if errorlevel 1 (
    echo HATA: openocd_cpld.cfg kopyalanamadi!
    pause
    exit /b 1
)

echo OK - Dosyalar kopyalandi
echo.

echo [2/3] CPLD programlaniyor...
echo.
echo --------------------------------------------------------
echo.

REM OpenOCD calistir
ssh %PI_USER%@%PI_HOST% "sudo openocd -f /tmp/openocd_cpld.cfg -c 'svf /tmp/cpld.svf; shutdown'" 2>&1 | findstr /C:"svf file programmed successfully" /C:"Error" /C:"Warn"

if errorlevel 1 (
    echo.
    echo UYARI: Sonuc belirsiz. Tam log icin manuel kontrol edin.
) else (
    echo.
    echo OK - Programlama tamamlandi
)

echo.
echo --------------------------------------------------------
echo.

echo [3/3] SPI test...
echo.

REM SPI test (opsiyonel)
ssh %PI_USER%@%PI_HOST% "python3 /tmp/quick_spi_test.py 2>&1 | head -5"

echo.
echo ============================================================
echo   TAMAMLANDI
echo ============================================================
echo.

pause
