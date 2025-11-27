# BUILD.md - Derleme ve Programlama Rehberi

**Burjuva Atacama - Geliştirici Dokümantasyonu**  
**Tarih:** 27 Kasım 2025

---

## 🛠️ Geliştirme Ortamı Kurulumu

### Windows

**1. ARM GCC Toolchain Kurulumu:**

```powershell
# Chocolatey ile (önerilen)
choco install gcc-arm-embedded

# Manuel kurulum:
# https://developer.arm.com/downloads/-/gnu-rm
# İndir ve PATH'e ekle
```

**2. ST-Link Sürücüleri:**

```powershell
# ST-Link V2 sürücülerini indir:
# https://www.st.com/en/development-tools/stsw-link009.html
```

**3. STM32 Flash Tool:**

```powershell
# st-flash aracını kur:
choco install stlink

# Alternatif: STM32CubeProgrammer (GUI)
# https://www.st.com/en/development-tools/stm32cubeprog.html
```

---

### Linux (Ubuntu/Debian)

**1. Toolchain ve Araçlar:**

```bash
# ARM GCC toolchain
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi

# Build tools
sudo apt install make

# STLink utilities
sudo apt install stlink-tools

# Opsiyonel: Debugging tools
sudo apt install gdb-multiarch openocd
```

**2. USB İzinleri (ST-Link için):**

```bash
# udev kuralları ekle
sudo tee /etc/udev/rules.d/49-stlink.rules > /dev/null <<EOF
# ST-Link V2
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", MODE="0666"
# ST-Link V2.1
SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="374b", MODE="0666"
EOF

# Kuralları yeniden yükle
sudo udevadm control --reload-rules
sudo udevadm trigger
```

---

## 📦 Proje Yapısı

```
stm32-firmware-beta/
├── src/                        # Kaynak kodlar
│   ├── main.c                 # Ana program
│   ├── modul_algilama.c       # 1-Wire modül algılama
│   ├── spisurucu.c            # SPI driver
│   ├── 16kanaldijital.c       # IO16 driver
│   ├── 20kanalanalogio.c      # AIO20 driver
│   └── uart_helper.c          # UART utilities
│
├── spl/                        # STM32 Standard Peripheral Library
│   ├── stm32f10x_*.c/.h       # SPL kaynak ve header dosyaları
│   ├── startup.c              # Startup kodu
│   └── stm32.ld               # Linker script
│
├── build/                      # Derleme çıktıları (oluşturulacak)
│   ├── *.o                    # Object dosyaları
│   ├── firmware.elf           # Debug binary
│   ├── firmware.hex           # Intel HEX format
│   ├── firmware.bin           # Raw binary
│   └── firmware.map           # Memory map
│
├── build.bat                   # Windows build script
└── Makefile                    # Linux build script (opsiyonel)
```

---

## 🔨 Derleme

### Windows (build.bat)

**Standart derleme:**

```powershell
cd stm32-firmware-beta
.\build.bat
```

**Çıktı:**
```
=====================================
STM32F103 UART Echo Build (SPL)
=====================================

Toolchain: OK
arm-none-eabi-gcc (GNU Arm Embedded Toolchain 10.3-2021.10)

Compiling...
[1/13] main.c
[2/13] modul_algilama.c
[3/13] 16kanaldijital.c
[4/13] 20kanalanalogio.c
[5/13] uart_helper.c
[6/13] spisurucu.c
[7/13] stm32f10x_gpio.c
[8/13] stm32f10x_rcc.c
[9/13] stm32f10x_usart.c
[10/13] stm32f10x_spi.c
[11/13] stm32f10x_flash.c
[12/13] system_stm32f10x.c
[13/13] startup.c

Linking...
Creating HEX...
Creating BIN...

=====================================
Build SUCCESS!
=====================================

Output files:
firmware.bin
firmware.elf
firmware.hex
firmware.map

Firmware size:
   text    data     bss     dec     hex filename
  24576    1024    2048   27648    6c00 build/firmware.elf

Ready to flash!
```

**Temizleme:**

```powershell
# build/ klasörünü sil
rmdir /s /q build
```

---

### Linux (Makefile - opsiyonel)

**İlk kurulum:**

```bash
cd stm32-firmware-beta

# Makefile yoksa, build.bat'ı Linux için adapte edin
# veya manuel derleme yapın (aşağıda)
```

**Manuel derleme:**

```bash
# Compiler ayarları
CPU="-mcpu=cortex-m3"
MCU="$CPU -mthumb"
DEFS="-DSTM32F10X_HD -DUSE_STDPERIPH_DRIVER"
INCLUDES="-Isrc -Ispl"
CFLAGS="$MCU $DEFS $INCLUDES -O2 -Wall -fdata-sections -ffunction-sections -g"
LDFLAGS="$MCU -specs=nano.specs -Tspl/stm32.ld -lc -lm -lnosys -Wl,-Map=build/firmware.map,--cref -Wl,--gc-sections"

# Build klasörü oluştur
mkdir -p build

# Derleme
arm-none-eabi-gcc -c $CFLAGS src/main.c -o build/main.o
arm-none-eabi-gcc -c $CFLAGS src/modul_algilama.c -o build/modul_algilama.o
arm-none-eabi-gcc -c $CFLAGS src/16kanaldijital.c -o build/16kanaldijital.o
arm-none-eabi-gcc -c $CFLAGS src/20kanalanalogio.c -o build/20kanalanalogio.o
arm-none-eabi-gcc -c $CFLAGS src/uart_helper.c -o build/uart_helper.o
arm-none-eabi-gcc -c $CFLAGS src/spisurucu.c -o build/spisurucu.o
arm-none-eabi-gcc -c $CFLAGS spl/stm32f10x_gpio.c -o build/stm32f10x_gpio.o
arm-none-eabi-gcc -c $CFLAGS spl/stm32f10x_rcc.c -o build/stm32f10x_rcc.o
arm-none-eabi-gcc -c $CFLAGS spl/stm32f10x_usart.c -o build/stm32f10x_usart.o
arm-none-eabi-gcc -c $CFLAGS spl/stm32f10x_spi.c -o build/stm32f10x_spi.o
arm-none-eabi-gcc -c $CFLAGS spl/stm32f10x_flash.c -o build/stm32f10x_flash.o
arm-none-eabi-gcc -c $CFLAGS spl/system_stm32f10x.c -o build/system_stm32f10x.o
arm-none-eabi-gcc -c $CFLAGS spl/startup.c -o build/startup.o

# Linking
arm-none-eabi-gcc $LDFLAGS \
    build/main.o \
    build/modul_algilama.o \
    build/16kanaldijital.o \
    build/20kanalanalogio.o \
    build/uart_helper.o \
    build/spisurucu.o \
    build/stm32f10x_gpio.o \
    build/stm32f10x_rcc.o \
    build/stm32f10x_usart.o \
    build/stm32f10x_spi.o \
    build/stm32f10x_flash.o \
    build/system_stm32f10x.o \
    build/startup.o \
    -o build/firmware.elf

# HEX ve BIN oluştur
arm-none-eabi-objcopy -O ihex build/firmware.elf build/firmware.hex
arm-none-eabi-objcopy -O binary -S build/firmware.elf build/firmware.bin

# Boyut bilgisi
arm-none-eabi-size build/firmware.elf
```

---

## 📥 Programlama

### ST-Link ile (Önerilen)

**Windows:**

```powershell
# ST-Link Utility (GUI)
# 1. STM32 ST-LINK Utility'yi aç
# 2. Target → Connect
# 3. File → Open → firmware.hex
# 4. Target → Program & Verify
# 5. Start

# Komut satırı (st-flash)
st-flash write build\firmware.bin 0x8000000
```

**Linux:**

```bash
# st-flash ile
st-flash write build/firmware.bin 0x8000000

# Çıktı:
st-flash 1.7.0
2025-11-27T10:30:45 INFO common.c: F1xx Medium-density: 128 KiB SRAM, 256 KiB flash in at least 1 KiB pages.
2025-11-27T10:30:45 INFO common.c: Attempting to write 24576 bytes to stm32 address: 134217728
2025-11-27T10:30:45 INFO common.c: Flash page at addr: 0x08000000 erased
...
2025-11-27T10:30:46 INFO common.c: Finished flashing
2025-11-27T10:30:46 INFO common.c: Starting verification
2025-11-27T10:30:46 INFO common.c: Flash written and verified! jolly good!
```

---

### OpenOCD ile (Alternatif)

**1. OpenOCD Config:**

`stm32f103.cfg` dosyası oluştur:

```tcl
source [find interface/stlink.cfg]
source [find target/stm32f1x.cfg]
```

**2. Flash komutu:**

```bash
# OpenOCD başlat (arka planda)
openocd -f stm32f103.cfg &

# Telnet ile bağlan
telnet localhost 4444

# Flash işlemi
> reset halt
> flash write_image erase build/firmware.bin 0x08000000
> reset run
> exit
```

---

### USB Bootloader ile (ST-Link olmadan)

**Gereksinimler:**
- BOOT0 pin'i HIGH (3.3V)
- USB-Serial dönüştürücü (UART1 üzerinden)

**1. Bootloader moduna geç:**

```
1. BOOT0 pin'ini HIGH yap (jumper ile 3.3V'a bağla)
2. RESET butonuna bas veya kartı yeniden güçlendir
3. Kart bootloader modunda başlar (LED yanıp sönmez)
```

**2. stm32flash ile programla:**

**Linux:**

```bash
# stm32flash kur
sudo apt install stm32flash

# Flash yaz
stm32flash -w build/firmware.bin -v -g 0x0 /dev/ttyUSB0

# Çıktı:
stm32flash 0.5
Interface serial_posix: 57600 8E1
Version      : 0x22
Option 1     : 0x00
Option 2     : 0x00
Device ID    : 0x0414 (STM32F10x_HD)
- RAM        : 64KiB
- Flash      : 256KiB (1024 pages x 256 bytes)
- Option RAM : 16 bytes
- System RAM : 2KiB
Write to memory
Erasing memory
Wrote address 0x08006000 (100.00%) Done.
Starting execution at address 0x08000000... done.
```

**Windows:**

```powershell
# stm32flash.exe indir:
# https://sourceforge.net/projects/stm32flash/

.\stm32flash.exe -w build\firmware.bin -v -g 0x0 COM3
```

**3. Normal moda dön:**

```
1. BOOT0 pin'ini LOW yap (GND'ye bağla veya jumper'ı çıkar)
2. RESET butonuna bas
3. Firmware çalışmaya başlar
```

---

## 🐛 Debugging

### Serial Wire Debug (SWD)

**OpenOCD + GDB:**

**1. OpenOCD başlat:**

```bash
openocd -f stm32f103.cfg
```

**2. GDB başlat (başka terminal):**

```bash
arm-none-eabi-gdb build/firmware.elf

# GDB içinde:
(gdb) target remote localhost:3333
(gdb) monitor reset halt
(gdb) load
(gdb) break main
(gdb) continue
```

---

### UART Debug

**printf() desteği:**

`uart_helper.c` dosyasına ekle:

```c
// Retarget printf to UART
int _write(int file, char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        UART_SendChar(ptr[i]);
    }
    return len;
}
```

**Kullanım:**

```c
#include <stdio.h>

printf("Debug: value = %d\r\n", value);
```

---

## 📊 Memory Map

### STM32F103RCT6 Bellek Düzeni

```
0x08000000 - 0x0803FFFF  FLASH (256 KB)
  ├─ 0x08000000          Vector Table
  ├─ 0x08000100          Startup Code
  └─ 0x08001000          Main Program

0x20000000 - 0x2000BFFF  SRAM (48 KB)
  ├─ 0x20000000          .data (initialized variables)
  ├─ 0x20001000          .bss (uninitialized variables)
  ├─ 0x20008000          Heap
  └─ 0x2000BF00          Stack (grows downward)

0x40000000 - 0x40023FFF  Peripherals
  ├─ 0x40013800          USART1
  ├─ 0x40003800          SPI2
  ├─ 0x40010800          GPIOA
  ├─ 0x40010C00          GPIOB
  └─ 0x40011000          GPIOC
```

### Firmware Boyutu

**Tipik boyutlar:**

| Bölüm | Boyut | Açıklama |
|-------|-------|----------|
| `.text` | ~20 KB | Program kodu |
| `.rodata` | ~2 KB | Sabit veriler (strings) |
| `.data` | ~1 KB | Başlatılmış değişkenler |
| `.bss` | ~2 KB | Başlatılmamış değişkenler |
| **Toplam Flash** | ~23 KB | 256 KB'ın %9'u |
| **Toplam RAM** | ~3 KB | 48 KB'ın %6'sı |

---

## 🔧 Build Ayarları

### Optimizasyon Seviyeleri

`build.bat` içinde değiştir:

```bat
REM -O0: Optimizasyon yok (debug)
REM -O1: Temel optimizasyon
REM -O2: Orta seviye (varsayılan)
REM -O3: Agresif optimizasyon
REM -Os: Boyut optimizasyonu

set CFLAGS=%MCU% %DEFS% %INCLUDES% -O2 -Wall -fdata-sections -ffunction-sections -g
```

### Debug Sembolleri

**Debug açık (geliştirme):**

```bat
set CFLAGS=... -g -gdwarf-2
```

**Debug kapalı (production):**

```bat
set CFLAGS=... -O2
```

### Linker Optimizasyonu

**Kullanılmayan kodu kaldır:**

```bat
set LDFLAGS=... -Wl,--gc-sections
```

**Map dosyası oluştur:**

```bat
set LDFLAGS=... -Wl,-Map=build/firmware.map,--cref
```

---

## 📝 Sık Karşılaşılan Sorunlar

### Derleme Hataları

**Hata: `arm-none-eabi-gcc: command not found`**

```bash
# Toolchain kurulu değil
# Windows: choco install gcc-arm-embedded
# Linux: sudo apt install gcc-arm-none-eabi
```

**Hata: `undefined reference to '_start'`**

```bash
# startup.c eksik veya linker script hatalı
# spl/startup.c ve spl/stm32.ld dosyalarını kontrol edin
```

**Hata: `region 'FLASH' overflowed`**

```bash
# Kod FLASH'a sığmıyor (>256KB)
# - Optimizasyon seviyesini artırın (-Os)
# - Kullanılmayan kütüphaneleri kaldırın
# - Büyük dizileri const yapın (FLASH'ta tutar)
```

---

### Programlama Hataları

**Hata: `Can't connect to target!`**

```bash
# ST-Link bağlantı sorunu
# 1. USB kablosunu kontrol edin
# 2. ST-Link sürücülerini yükleyin
# 3. SWDIO/SWCLK pinlerini kontrol edin
# 4. Kartı sıfırlayın
```

**Hata: `Flash write failed`**

```bash
# Flash kilitli veya korumalı
# 1. BOOT0 = GND olduğundan emin olun
# 2. Flash koruma bitlerini temizleyin:
st-flash --reset write build/firmware.bin 0x8000000
```

---

## ✅ Test ve Doğrulama

### Temel Test

**1. LED testi:**

```bash
# PC13 LED'i yanıp sönmeli
# Sistem çalışıyorsa LED toggle olur
```

**2. UART testi:**

```bash
# Serial terminal bağlan
minicom -D /dev/ttyUSB0 -b 115200

# Komut gönder
> help

# Yanıt alırsanız UART çalışıyor ✓
```

**3. Modül testi:**

```bash
# Modül algılama
> modul-algila

# IO16 test (modül varsa)
> io16:0:readall

# AIO20 test (modül varsa)
> aio20:1:status
```

---

## 📚 Ek Kaynaklar

- **STM32F103 Reference Manual:** [RM0008](https://www.st.com/resource/en/reference_manual/cd00171190-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)
- **STM32F103 Datasheet:** [DS5319](https://www.st.com/resource/en/datasheet/stm32f103rc.pdf)
- **ARM GCC Manual:** [GCC ARM Options](https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html)
- **Linker Scripts:** [GNU LD Manual](https://sourceware.org/binutils/docs/ld/)

---

**Son Güncelleme:** 27 Kasım 2025  
**Yazar:** Burjuva Pilot Ekibi
