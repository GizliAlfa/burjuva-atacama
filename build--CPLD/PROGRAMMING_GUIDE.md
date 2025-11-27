# CPLD Programlama Rehberi

## 🎯 Hızlı Başlangıç

Bu rehber, Altera MAX V CPLD'yi Raspberry Pi üzerinden JTAG ile programlamak için adım adım talimatlar içerir.

---

## 📋 Gereksinimler

### Donanım
- Raspberry Pi 4 (veya 3B+)
- Altera MAX V 5M80ZT100C5 CPLD
- JTAG bağlantıları (4 pin: TDI, TMS, TCK, TDO)
- Ortak GND bağlantısı

### Yazılım
- **Windows (PC):**
  - Quartus Prime Lite 25.1std veya üzeri
  - Dosya kopyalama aracı (scp/WinSCP)

- **Raspberry Pi:**
  - Raspbian/Raspberry Pi OS (64-bit)
  - OpenOCD 0.12.0 veya üzeri
  - Root erişimi

---

## 🔌 Pin Bağlantıları

### Raspberry Pi GPIO → CPLD JTAG

```
Raspberry Pi (40-pin header)     CPLD (100-pin TQFP)
================================ ===================
Pin 16 (GPIO 23) -------------> PIN 33 (TMS)
Pin 15 (GPIO 22) -------------> PIN 34 (TDI)
Pin 22 (GPIO 25) -------------> PIN 35 (TCK)
Pin 18 (GPIO 24) -------------> PIN 36 (TDO)
Pin 6  (GND)     -------------> GND
```

**Önemli:** BCM GPIO numaralarını kullanın, fiziksel pin numaralarını değil!

---

## 🛠️ Adım 1: Quartus ile Derleme (Windows)

### 1.1. Quartus Kurulumu
```powershell
# Quartus Prime Lite'ı indirin ve kurun
# https://www.intel.com/content/www/us/en/software/programmable/quartus-prime/download.html

# PATH'e ekleyin (PowerShell)
$env:PATH += ";C:\altera_lite\25.1std\quartus\bin64"
```

### 1.2. Proje Hazırlama

**⚠️ ÖNEMLİ:** Türkçe karakter içermeyen bir klasör kullanın!

```powershell
# YANLIŞ (Türkçe karakter var):
# cd "C:\Users\Oktay\Çalışmalar\proje"

# DOĞRU:
cd "C:\temp\cpld-build"

# Veya kısa DOS path kullanın:
cd (Get-Item "C:\Users\Oktay\DOCUME~1\CAL_A~1\BURJUV~1").FullName
```

### 1.3. Derleme Ayarları

**cpld.qsf** dosyasında kritik ayarlar:

```tcl
set_global_assignment -name FAMILY "MAX V"
set_global_assignment -name DEVICE 5M80ZT100C5
set_global_assignment -name TOP_LEVEL_ENTITY top
set_global_assignment -name PROJECT_OUTPUT_DIRECTORY output_files

# SVF formatı için (OpenOCD)
set_global_assignment -name GENERATE_SVF_FILE ON
set_global_assignment -name USE_CONFIGURATION_DEVICE ON

# JAM formatı (opsiyonel, eski toolchain için)
set_global_assignment -name GENERATE_JAM_FILE ON
```

### 1.4. Derleme Komutu

```powershell
# Tam derleme (synthesis + fitting + assembly)
quartus_sh --flow compile cpld

# Başarılı derleme çıktısı:
# output_files/cpld.svf  (SVF formatı - OpenOCD için)
# output_files/cpld.pof  (Programmer Object File)
# output_files/cpld.jam  (JAM STAPL - eski toolchain)
```

### 1.5. Derleme Sonuç Kontrolü

```powershell
# Derleme raporunu inceleyin
cat output_files\cpld.fit.summary

# Örnek çıktı:
# Fitter Status : Successful - <tarih>
# Logic utilization : 3 % ( 2 / 80 )
# Total pins      : 76 / 79 ( 96 % )
```

---

## 📤 Adım 2: Dosyaları Raspberry Pi'ye Kopyalama

### 2.1. OpenOCD Config Dosyası

**openocd_cpld.cfg** dosyası oluşturun:

```tcl
# bcm2835gpio driver (Raspberry Pi GPIO doğrudan erişim)
adapter driver bcm2835gpio

# Raspberry Pi 4 için base address
bcm2835gpio_peripheral_base 0xFE000000

# Raspberry Pi 3 için:
# bcm2835gpio_peripheral_base 0x3F000000

# GPIO pin atamaları (BCM numaraları!)
adapter gpio tck -chip 0 25  # GPIO 25 -> TCK
adapter gpio tms -chip 0 23  # GPIO 23 -> TMS
adapter gpio tdi -chip 0 22  # GPIO 22 -> TDI
adapter gpio tdo -chip 0 24  # GPIO 24 -> TDO

# JTAG hızı (500 kHz güvenli)
adapter speed 500

# JTAG transport seç
transport select jtag

# MAX V TAP tanımı
# -irlen 10: Instruction register 10 bit
# -expected-id: CPLD'nin gerçek IDCODE'u
jtag newtap maxv tap -irlen 10 -expected-id 0x020a50dd

init
```

**⚠️ IDCODE Uyarısı:**
- Template'lerde `0x020a10dd` olabilir
- Gerçek cihazınızın IDCODE'unu scan_chain ile doğrulayın!
- Yanlış IDCODE → "device not found" hatası

### 2.2. Dosya Transfer (Windows → Raspberry Pi)

```powershell
# PowerShell ile scp
scp output_files\cpld.svf burjuva@192.168.1.22:/tmp/
scp openocd_cpld.cfg burjuva@192.168.1.22:/tmp/

# Veya WinSCP GUI kullanın
```

---

## 🔍 Adım 3: JTAG Bağlantı Testi (Raspberry Pi)

SSH ile Raspberry Pi'ye bağlanın:

```bash
ssh burjuva@192.168.1.22
```

### 3.1. OpenOCD Kurulumu

```bash
# OpenOCD yükleyin
sudo apt update
sudo apt install openocd -y

# Versiyon kontrolü (0.12.0+ olmalı)
openocd --version
```

### 3.2. GPIO İzinleri

```bash
# Root olarak çalıştırın veya gpio grubuna ekleyin
sudo usermod -a -G gpio $USER

# Oturumu yeniden başlatın
logout
ssh burjuva@192.168.1.22
```

### 3.3. IDCODE Okuma (Önemli!)

```bash
# CPLD'nin gerçek IDCODE'unu okuyun
sudo openocd -f /tmp/openocd_cpld.cfg \
  -c 'init; scan_chain; shutdown' 2>&1 | grep "tap/device found"

# Örnek çıktı:
# Info : tap/device found: 0x020a50dd (mfg: 0x06e (Altera), part: 0x20a5, ver: 0x0)

# IDCODE'u not edin ve config'de güncelleyin!
```

**Yaygın Hatalar:**

| Hata Mesajı | Sebep | Çözüm |
|-------------|-------|-------|
| `all ones` veya `all zeroes` | JTAG bağlantısı yok | Kabloları kontrol et, pinout doğrula |
| `Warn : JTAG tap: maxv.tap unexpected: 0xXXXXXXXX` | IDCODE uyumsuzluğu | Config'de expected-id'yi güncelle |
| `Error: IR capture error` | Yanlış pin mapping | GPIO numaralarını kontrol et |

### 3.4. Bağlantı Başarı Testi

```bash
# TAP başlatma ve kapatma (hızlı test)
sudo openocd -f /tmp/openocd_cpld.cfg \
  -c 'init; shutdown'

# Başarılı çıktı:
# Info : Listening on port 6666 for tcl connections
# Info : Listening on port 4444 for telnet connections
# shutdown command invoked
```

---

## 🚀 Adım 4: CPLD Programlama

### 4.1. SVF Dosyası ile Programlama

```bash
# Tek komutla programlama
sudo openocd -f /tmp/openocd_cpld.cfg \
  -c 'svf /tmp/cpld.svf; shutdown'
```

### 4.2. Başarılı Programlama Çıktısı

```
Open On-Chip Debugger 0.12.0
Info : auto-selecting first available session transport "jtag"
Info : BCM2835 GPIO JTAG/SWD bitbang driver
Info : clock speed 500 kHz
Info : JTAG tap: maxv.tap tap/device found: 0x020a50dd
Info : svf processing file: "/tmp/cpld.svf"
Progress: 0%
Progress: 5%
Progress: 10%
...
Progress: 95%
Progress: 100%
Time used: 0m4s
Info : svf file programmed successfully for maxv.tap with no error
shutdown command invoked
```

**Süre:** ~4 saniye (271 KB SVF dosyası için)

### 4.3. Hata Durumları

#### Hata 1: IDCODE Uyumsuzluğu
```
Warn : JTAG tap: maxv.tap unexpected: 0x020a50dd (mfg: 0x06e, part: 0x20a5)
Error : IR capture error at bit 2
```
**Çözüm:** `openocd_cpld.cfg` dosyasında `-expected-id 0x020a50dd` olarak güncelle.

#### Hata 2: SVF Dosyası Bulunamadı
```
Error: unable to open SVF file /tmp/cpld.svf
```
**Çözüm:** Dosya yolunu ve izinlerini kontrol et.

#### Hata 3: GPIO İzin Hatası
```
Error: unable to open /dev/gpiomem or /dev/mem
```
**Çözüm:** `sudo` ile çalıştır veya kullanıcıyı `gpio` grubuna ekle.

---

## ✅ Adım 5: Doğrulama

### 5.1. SPI Passthrough Testi

CPLD'nin SPI passthrough çalıştığını test edin:

```python
# quick_spi_test.py
import spidev
import time

spi = spidev.SpiDev()
spi.open(0, 0)  # SPI bus 0, device 0
spi.max_speed_hz = 1000000  # 1 MHz

# Test verisi gönder
response = spi.xfer2([0x00, 0xFF, 0xAA, 0x55])
print(f"Response: {[hex(b) for b in response]}")

spi.close()
```

**Beklenen Sonuç:**
- CPLD programlanmış: Response alınır (STM32'ye bağlı olarak değişir)
- CPLD programlanmamış: Response alınamaz veya 0x00 döner

### 5.2. GPIO Sinyal Testi

```python
# gpio_verify.py
import RPi.GPIO as GPIO
import time

GPIO.setmode(GPIO.BCM)

# JTAG pinlerini test et
test_pins = [22, 23, 24, 25]
for pin in test_pins:
    GPIO.setup(pin, GPIO.OUT)
    GPIO.output(pin, GPIO.HIGH)
    time.sleep(0.1)
    print(f"GPIO {pin} -> HIGH (multimetre ile ölç)")
    GPIO.output(pin, GPIO.LOW)

GPIO.cleanup()
```

**Multimetre ile doğrula:**
- HIGH: ~3.3V
- LOW: ~0V

---

## 🔧 Troubleshooting

### Problem: "Error: IR capture error"

**Nedenler:**
1. Yanlış GPIO pin mapping
2. Kablo bağlantı hatası
3. CPLD güç sorunu

**Çözüm:**
```bash
# GPIO pinlerini kontrol et
gpio readall | grep -E "GPIO.2[2-5]"

# Fiziksel bağlantıları multimetre ile test et
```

---

### Problem: "JTAG tap: unexpected IDCODE"

**Sebep:** CPLD variant farklı

**Çözüm:**
```bash
# Gerçek IDCODE'u oku
sudo openocd -f /tmp/openocd_cpld.cfg \
  -c 'init; scan_chain; shutdown' 2>&1 | grep "0x[0-9a-f]*"

# Config dosyasında güncelle:
# jtag newtap maxv tap -irlen 10 -expected-id 0xXXXXXXXX
```

---

### Problem: Quartus "Can't create project" Hatası

**Sebep:** Path'te Türkçe karakter var

**Çözüm:**
```powershell
# Projeyi Türkçe karaktersiz yola taşı
Copy-Item -Recurse "C:\Users\Oktay\Çalışmalar\proje" "C:\temp\proje"
cd C:\temp\proje
```

---

### Problem: "svf file programmed successfully" ama çalışmıyor

**Nedenler:**
1. CPLD power-on-reset gerektiriyor
2. SPI pinleri doğru yapılandırılmamış

**Çözüm:**
```bash
# CPLD'yi resetle (güç kes-aç)
# veya
# Reset pinini toggle et (varsa)

# SPI test et
python3 quick_spi_test.py
```

---

## 📚 Ek Bilgiler

### Quartus Proje Yapısı

```
cpld-build/
├── cpld.qpf              # Quartus Project File
├── cpld.qsf              # Quartus Settings File
├── top.v                 # Top-level Verilog
├── rpi.v                 # RPI SPI module
├── testin.v              # Input module
├── testout.v             # Output module
├── db/                   # Database (ara dosyalar)
├── incremental_db/       # Incremental compilation
└── output_files/
    ├── cpld.svf          # Serial Vector Format (OpenOCD)
    ├── cpld.pof          # Programmer Object File
    ├── cpld.jam          # JAM STAPL (eski toolchain)
    └── cpld.fit.summary  # Fit raporu
```

### OpenOCD Komut Referansı

```bash
# TAP listesi
init; scan_chain

# IDCODE okuma
init; drscan maxv.tap 32 0

# SVF programlama
svf /path/to/file.svf

# Verbose mod
openocd -d3 -f config.cfg -c 'init; scan_chain; shutdown'
```

### Altera CPLD IDCODE Formatı

```
32-bit IDCODE:
[31:28] Version      (4 bit)
[27:12] Part Number  (16 bit)
[11:1]  Manufacturer (11 bit) - Altera: 0x06E
[0]     Required 1   (1 bit)

Örnek: 0x020a50dd
  0x0   = Version 0
  0x20a5 = MAX V 5M80Z
  0x06E = Altera
  0x1   = LSB (her zaman 1)
```

---

## 🎓 Best Practices

1. **Her Zaman IDCODE Doğrulama Yapın**
   ```bash
   sudo openocd -f config.cfg -c 'init; scan_chain; shutdown'
   ```

2. **Türkçe Karakter Kullanmayın**
   - Tüm proje dosyaları ASCII-only path'te olmalı

3. **SVF Formatını Tercih Edin**
   - JAM formatı eski (2004)
   - SVF modern ve güvenilir

4. **GPIO Test Edin**
   - Programlama öncesi fiziksel bağlantıları doğrulayın

5. **Incremental Backup**
   - Her başarılı derleme sonrası output_files/'ı yedekleyin

---

## 📞 Sorun Giderme İletişim

Sorun yaşarsanız:

1. **IDCODE kontrolü**: `scan_chain` çıktısını paylaşın
2. **GPIO pinout**: `gpio readall` çıktısını paylaşın
3. **OpenOCD log**: `-d3` (debug level 3) ile çalıştırın
4. **Quartus log**: `output_files/cpld.fit.rpt` dosyasını inceleyin

---

**Başarılar! 🚀**

*Son güncelleme: 10 Kasım 2025*
