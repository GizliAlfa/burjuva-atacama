@echo off
REM STM32F103 SPL-based Firmware Build Script
REM Date: 16 Kasım 2025

echo =====================================
echo STM32F103 UART Echo Build (SPL)
echo =====================================
echo.

REM Check toolchain
where arm-none-eabi-gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: arm-none-eabi-gcc not found!
    exit /b 1
)

echo Toolchain: OK
arm-none-eabi-gcc --version | findstr "arm-none-eabi-gcc"
echo.

REM Create build directory
if not exist "build" mkdir build

REM Compiler settings
set CPU=-mcpu=cortex-m3
set MCU=%CPU% -mthumb
set DEFS=-DSTM32F10X_HD -DUSE_STDPERIPH_DRIVER
set INCLUDES=-Isrc -Ispl
set CFLAGS=%MCU% %DEFS% %INCLUDES% -O2 -Wall -fdata-sections -ffunction-sections -g
set LDFLAGS=%MCU% -specs=nano.specs -Tspl/stm32.ld -lc -lm -lnosys -Wl,-Map=build/firmware.map,--cref -Wl,--gc-sections

echo Compiling...

echo [1/8] main.c
arm-none-eabi-gcc -c %CFLAGS% src/main.c -o build/main.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [2/11] modul_algilama.c
arm-none-eabi-gcc -c %CFLAGS% src/modul_algilama.c -o build/modul_algilama.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [3/11] 16kanaldijital.c
arm-none-eabi-gcc -c %CFLAGS% src/16kanaldijital.c -o build/16kanaldijital.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [4/11] 20kanalanalogio.c
arm-none-eabi-gcc -c %CFLAGS% src/20kanalanalogio.c -o build/20kanalanalogio.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [5/12] fpga.c
arm-none-eabi-gcc -c %CFLAGS% src/fpga.c -o build/fpga.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [6/13] uart_helper.c
arm-none-eabi-gcc -c %CFLAGS% src/uart_helper.c -o build/uart_helper.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [7/10] spisurucu.c
arm-none-eabi-gcc -c %CFLAGS% src/spisurucu.c -o build/spisurucu.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [8/10] stm32f10x_gpio.c
arm-none-eabi-gcc -c %CFLAGS% spl/stm32f10x_gpio.c -o build/stm32f10x_gpio.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [9/10] stm32f10x_rcc.c
arm-none-eabi-gcc -c %CFLAGS% spl/stm32f10x_rcc.c -o build/stm32f10x_rcc.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [10/10] stm32f10x_usart.c
arm-none-eabi-gcc -c %CFLAGS% spl/stm32f10x_usart.c -o build/stm32f10x_usart.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [11/10] stm32f10x_spi.c
arm-none-eabi-gcc -c %CFLAGS% spl/stm32f10x_spi.c -o build/stm32f10x_spi.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [12/10] stm32f10x_flash.c
arm-none-eabi-gcc -c %CFLAGS% spl/stm32f10x_flash.c -o build/stm32f10x_flash.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [13/10] system_stm32f10x.c
arm-none-eabi-gcc -c %CFLAGS% spl/system_stm32f10x.c -o build/system_stm32f10x.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo [14/10] startup.c
arm-none-eabi-gcc -c %CFLAGS% spl/startup.c -o build/startup.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo.
echo Linking...
arm-none-eabi-gcc %LDFLAGS% ^
    build/main.o ^
    build/modul_algilama.o ^
    build/16kanaldijital.o ^
    build/20kanalanalogio.o ^
    build/fpga.o ^
    build/uart_helper.o ^
    build/spisurucu.o ^
    build/stm32f10x_gpio.o ^
    build/stm32f10x_rcc.o ^
    build/stm32f10x_usart.o ^
    build/stm32f10x_spi.o ^
    build/stm32f10x_flash.o ^
    build/system_stm32f10x.o ^
    build/startup.o ^
    -o build/firmware.elf

if %ERRORLEVEL% NEQ 0 (
    echo Linking failed!
    exit /b 1
)

echo Creating HEX...
arm-none-eabi-objcopy -O ihex build/firmware.elf build/firmware.hex

echo Creating BIN...
arm-none-eabi-objcopy -O binary -S build/firmware.elf build/firmware.bin

echo.
echo =====================================
echo Build SUCCESS!
echo =====================================
echo.
echo Output files:
dir /b build\firmware.*
echo.

echo Firmware size:
arm-none-eabi-size build/firmware.elf

echo.
echo Ready to flash!
echo Copy firmware.bin to RPi and run: python3 burjuva_flash.py --stm32_only
echo.
