# Pilot CP## 📚 Dokümantasyon

- **ARCHITECTURE_ANALYSIS.md** - CPLD mimarisi, avantaj/dezavantajlar, alternatifler
- **BUILD.md** - Derleme ve programlama talimatları
- **STATUS.md** - Proje durumu

## 🚀 Hızlı Başlangıç

**Quartus Lite kurulduktan sonra**:

```powershell
# VS Code'da aç
code .

# Derle (Ctrl+Shift+B)
# veya
quartus_sh --flow compile cpld

# Raspberry Pi'ye gönder
scp output_files/cpld.svf burjuva@192.168.1.22:/tmp/

# Programla
ssh burjuva@192.168.1.22 "sudo /tmp/program_cpld.sh"
```

**Detaylı talimatlar**: `BUILD.md` dosyasına bakın CPLD Bilgileri

- **Device**: Altera MAX V 5M80ZT100C5
- **Package**: 100-pin TQFP
- **Macrocells**: 80
- **I/O Pins**: ~80 user I/O
- **Programming**: JTAG via OpenOCD

## � Hızlı Başlangıç

**Quartus Lite kurulduktan sonra**:

```powershell
# VS Code'da aç
code .

# Derle (Ctrl+Shift+B)
# veya
quartus_sh --flow compile cpld

# Raspberry Pi'ye gönder
scp output_files/cpld.svf burjuva@192.168.1.22:/tmp/

# Programla
ssh burjuva@192.168.1.22 "sudo /tmp/program_cpld.sh"
```

**Detaylı talimatlar**: `BUILD.md` dosyasına bakın

## �📁 Dosyalar

```
cpld-build/
├── top.v              # Ana modül (RPI + STM32 + 4x connector routing)
├── rpi.v              # Raspberry Pi SPI bridge
├── testin.v           # Input module routing
├── testout.v          # Output module routing
├── cpld.qsf           # Quartus settings
├── cpld.qpf           # Quartus project
├── cpld_program.cfg   # OpenOCD config
├── program_cpld.sh    # Programlama scripti
├── BUILD.md           # Derleme talimatları
└── README.md          # Bu dosya
```

## 🔧 Derleme Seçenekleri

### Seçenek 1: Quartus II (Windows/Linux)

**Gereksinimler**:
- Intel/Altera Quartus II 15.1 veya üstü
- MAX V device desteği

**Derleme**:
```bash
quartus_map cpld
quartus_fit cpld
quartus_asm cpld
```

**Çıktı**:
- `output_files/cpld.pof` - Programming Object File
- `output_files/cpld.jam` - JTAG programming file
- `output_files/cpld.svf` - Serial Vector Format

### Seçenek 2: OpenOCD + SVF (Raspberry Pi'de)

Quartus'ta `.svf` dosyası oluşturduktan sonra Raspberry Pi'ye kopyalayın:

```bash
# SVF dosyasını kopyala
scp output_files/cpld.svf pi@192.168.1.22:/tmp/

# Raspberry Pi'de:
ssh pi@192.168.1.22

# OpenOCD ile programla
sudo openocd -f cpld_program.cfg
```

## 🔌 JTAG Bağlantısı

Raspberry Pi GPIO -> CPLD JTAG pinleri:

```
RPi GPIO     CPLD JTAG
--------     ---------
GPIO 25  ->  TCK  (pin 1)
GPIO 24  ->  TDO  (pin 2)
GPIO 23  ->  TDI  (pin 3)
GPIO 22  ->  TMS  (pin 4)
GND      ->  GND
```

## 📝 OpenOCD Yapılandırması

`cpld_program.cfg` dosyası oluşturun:

```tcl
# OpenOCD configuration for MAX V CPLD via Raspberry Pi GPIO

interface bcm2835gpio
bcm2835gpio_peripheral_base 0x3F000000

# RPi GPIO pins for JTAG
bcm2835gpio_jtag_nums 22 25 24 23

# JTAG speed
adapter_khz 1000

# MAX V CPLD
jtag newtap maxv tap -irlen 10 -expected-id 0x020a10dd

init
scan_chain
svf /tmp/cpld.svf
shutdown
```

## 🚀 Hızlı Programlama

### Raspberry Pi'de (OpenOCD ile):

```bash
# 1. CPLD firmware hazırla
cd cpld-build
quartus_sh --flow compile cpld

# 2. SVF dosyasını Raspberry Pi'ye gönder
scp output_files/cpld.svf burjuva@192.168.1.22:/tmp/

# 3. OpenOCD config gönder
scp cpld_program.cfg burjuva@192.168.1.22:/tmp/

# 4. SSH ile bağlan ve programla
ssh burjuva@192.168.1.22
cd /tmp
sudo openocd -f cpld_program.cfg
```

## 🎯 Fonksiyonlar

### 1. RPI SPI Bridge
- Raspberry Pi SPI -> STM32 SPI
- **CLK**: RPI GPIO 11 -> PA5 (STM32)
- **MOSI**: RPI GPIO 10 -> PA7 (STM32)
- **MISO**: PA6 (STM32) -> RPI GPIO 9

### 2. Module Routing
- **CON0** -> STM32 Port B (Input, 9 pins)
- **CON1** -> STM32 Port C/D (Input, 9 pins)
- **CON2** <- STM32 Port A (Output, 9 pins)
- **CON3** <- STM32 Port B/C (Output, 9 pins)

### 3. Signal Processing
- Input debouncing (CON0, CON1)
- Output buffering (CON2, CON3)
- Simple passthrough logic

## ⚠️ Önemli Notlar

1. **Power**: CPLD programlamadan önce board'un power aldığından emin olun
2. **JTAG**: JTAG pinlerinin doğru bağlı olduğunu kontrol edin
3. **Backup**: Mevcut CPLD firmware'ini yedekleyin (gerekirse)
4. **Test**: Programlama sonrası SPI iletişimini test edin

## 🔍 Doğrulama

CPLD programlandıktan sonra:

```bash
# SPI test
python3 /tmp/cpld_spi_test.py

# Beklenen: MISO'dan cevap gelecek (0x00 değil)
```

## 📚 Referanslar

- [Altera MAX V Device Handbook](https://www.intel.com/content/www/us/en/programmable/documentation/sam1394323698316.html)
- [Quartus Prime Documentation](https://www.intel.com/content/www/us/en/software/programmable/quartus-prime/overview.html)
- [OpenOCD User Guide](http://openocd.org/doc/html/index.html)

---

**Hazırlayan**: Burjuva Pilot Team  
**Tarih**: 9 Kasım 2025
