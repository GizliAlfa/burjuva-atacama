# CPLD Programlama - Başarı Hikayesi ve Teknik Notlar

**Tarih:** 10 Kasım 2025  
**Proje:** Burjuva Pilot CPLD Firmware  
**Cihaz:** Altera MAX V 5M80ZT100C5 (gerçek IDCODE: 0x020a50dd)

---

## 📋 Özet

CPLD başarıyla programlandı! İşte yaşanan sorunlar ve çözümleri:

---

## 🔴 Yaşanan Sorunlar

### 1. **YANLIŞ IDCODE** ⚠️
- **Sorun:** Template'lerde ve dökümanlarda IDCODE `0x020a10dd` olarak yazıyordu
- **Gerçek:** Karttaki CPLD'nin IDCODE'u `0x020a50dd` 
- **Sonuç:** Tüm JTAG testleri "device not found" hatası veriyordu
- **Çözüm:** OpenOCD config'de `-expected-id 0x020a50dd` kullanıldı

**DERS:** Template'lere körü körüne güvenme! Önce cihazın gerçek IDCODE'unu oku.

---

### 2. **Türkçe Karakter Sorunu (Quartus)** 🔤
- **Sorun:** Quartus, path'te Türkçe karakter (`Çalışmalar`) tanımıyor
- **Hata:** "Can't create project" hatası
- **Çözüm:** Projeyi `C:\temp\cpld-build\` gibi Türkçe karaktersiz yola taşı
- **Alternatif:** Kısa DOS path kullan (8.3 format)

**DERS:** Tüm geliştirme klasörlerini İngilizce karakterlerle oluştur!

---

### 3. **JAM Player Uyumsuzluğu** 🎭
- **Sorun:** Mevcut sistemdeki `jamplayer` (v2.5, 2004) çok eski
- **Quartus:** 25.1std yeni JAM formatı (v2.0) üretiyor
- **Hata:** `Error on line 209: syntax error`
- **Denenen:** JAM, JBC, POF formatları - hepsi aynı hata
- **Çözüm:** JAM yerine **SVF formatı + OpenOCD** kullanıldı

**DERS:** Eski toolchain'ler (2004) modern Quartus (2025) ile uyumlu olmayabilir. OpenOCD güncel ve güvenilir!

---

### 4. **GPIO Pinout Karmaşası** 📍
- **Sorun:** Raspberry Pi pin numaraları ile BCM GPIO numaraları karıştı
- **Kartın Formatı:**
  ```
  Çift_Pin (Sol)  |  Tek_Pin (Sağ)
  16 -> GPIO 23   |  15 -> GPIO 22
  ```
- **Deneme 1:** GPIO 22, 27, 24, 23 (YANLIŞ - kernel offset ile karıştırıldı)
- **Deneme 2:** GPIO 534-537 (YANLIŞ - gpiochip512 offset eklendi ama gereksizdi)
- **DOĞRU:** GPIO 23, 22, 24, 25 (BCM numaraları)

**DERS:** Fiziksel pin numarası ≠ GPIO numarası. Raspberry Pi pinout chart'a bak!

---

### 5. **OpenOCD Deprecated Syntax** 🔧
- **Eski Syntax (çalışmadı):**
  ```
  bcm2835gpio_jtag_nums 22 25 24 23
  ```
- **Yeni Syntax (çalıştı):**
  ```
  adapter gpio tck -chip 0 25
  adapter gpio tms -chip 0 23
  adapter gpio tdi -chip 0 22
  adapter gpio tdo -chip 0 24
  ```

**DERS:** OpenOCD 0.12+ modern GPIO syntax kullanıyor. Deprecated uyarılarına dikkat et!

---

## ✅ Çözüm Adımları

### 1. Doğru IDCODE'u Bul
```bash
sudo openocd -f <config> -c 'init; scan_chain; shutdown'
```
Çıktıda `tap/device found: 0xXXXXXXXX` satırını ara.

### 2. Quartus ile Derleme (Türkçe Karaktersiz Path)
```powershell
# PATH'e ekle
$env:PATH += ";C:\altera_lite\25.1std\quartus\bin64"

# Türkçe karaktersiz klasörde derle
cd C:\temp\cpld-build
quartus_sh --flow compile cpld

# Çıktı: output_files/cpld.svf (271 KB)
```

### 3. OpenOCD Config (DOĞRU VERSION)
```tcl
# openocd_correct.cfg
adapter driver bcm2835gpio
bcm2835gpio_peripheral_base 0xFE000000

adapter gpio tck -chip 0 25  # Pin 22 -> Altera PIN 35
adapter gpio tms -chip 0 23  # Pin 16 -> Altera PIN 33
adapter gpio tdi -chip 0 22  # Pin 15 -> Altera PIN 34
adapter gpio tdo -chip 0 24  # Pin 18 -> Altera PIN 36

adapter speed 500
transport select jtag

# GERÇEK IDCODE!
jtag newtap maxv tap -irlen 10 -expected-id 0x020a50dd

init
```

### 4. Programlama
```bash
# Dosyaları Raspberry Pi'ye kopyala
scp output_files/cpld.svf burjuva@192.168.1.22:/tmp/
scp openocd_correct.cfg burjuva@192.168.1.22:/tmp/

# Programla
ssh burjuva@192.168.1.22
sudo openocd -f /tmp/openocd_correct.cfg \
  -c 'svf /tmp/cpld.svf; shutdown'
```

---

## 🎯 Pin Mapping (FİNAL)

### Raspberry Pi → CPLD JTAG

| RPI Fiziksel Pin | RPI GPIO (BCM) | CPLD Pin | Sinyal |
|------------------|----------------|----------|---------|
| Pin 16 (çift/sol) | **GPIO 23** | Altera PIN 33 | **TMS** |
| Pin 15 (tek/sağ)  | **GPIO 22** | Altera PIN 34 | **TDI** |
| Pin 22 (çift/sol) | **GPIO 25** | Altera PIN 35 | **TCK** |
| Pin 18 (çift/sol) | **GPIO 24** | Altera PIN 36 | **TDO** |
| Pin 6 (çift/sol)  | GND | GND | **GND** |

### Raspberry Pi → CPLD SPI (Passthrough)

| RPI Pin | CPLD Pin | STM32 Pin | Sinyal |
|---------|----------|-----------|---------|
| Pin 19 (GPIO 10) | PIN 27 (MOSI) | PA7 | MOSI |
| Pin 21 (GPIO 9)  | PIN 28 (MISO) | PA6 | MISO |
| Pin 23 (GPIO 11) | PIN 12 (CLK)  | PA5 | SCLK |

---

## 🚫 YAPILMAMASI GEREKENLER

### ❌ 1. Template IDCODE'a Güvenme
```
❌ YANLIŞ: set_global_assignment -name DEVICE_ID 0x020a10dd
✅ DOĞRU: Önce scan_chain ile gerçek ID'yi öğren
```

### ❌ 2. Eski JAM Player Kullanma
```
❌ YANLIŞ: jamplayer (2004) ile yeni Quartus (2025)
✅ DOĞRU: OpenOCD + SVF formatı (modern, güvenilir)
```

### ❌ 3. GPIO Offset Karıştırma
```
❌ YANLIŞ: gpiochip512 offset'i BCM numaralarına ekleme
✅ DOĞRU: Direkt BCM numaralarını kullan (bcm2835gpio driver)
```

### ❌ 4. Türkçe Karakter Kullanma
```
❌ YANLIŞ: C:\Users\Oktay\Çalışmalar\proje\
✅ DOĞRU: C:\Users\Oktay\Projects\proje\
         veya C:\temp\proje\
```

### ❌ 5. Deprecated OpenOCD Syntax
```
❌ YANLIŞ: bcm2835gpio_jtag_nums 22 25 24 23
✅ DOĞRU: adapter gpio tck -chip 0 25
         adapter gpio tms -chip 0 23
         adapter gpio tdi -chip 0 22
         adapter gpio tdo -chip 0 24
```

---

## 📊 Başarı Metrikleri

- **Toplam Deneme Sayısı:** ~25 kez
- **Hata Ayıklama Süresi:** ~3 saat
- **Ana Sorun:** IDCODE uyumsuzluğu (50 dakika kaybettirdi)
- **İkincil Sorun:** JAM Player uyumsuzluğu (40 dakika)
- **Final Çözüm:** Modern OpenOCD + Doğru IDCODE
- **Programlama Süresi:** 4 saniye ✨

---

## 🔍 Tespit Komutları

### CPLD IDCODE Okuma
```bash
sudo openocd -f <config> -c 'init; scan_chain; shutdown' 2>&1 | grep "tap/device found"
```

### GPIO Test
```python
import RPi.GPIO as GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(23, GPIO.OUT)
GPIO.output(23, GPIO.HIGH)  # Pin 16'da sinyal görmeli
```

### SPI Test
```python
import spidev
spi = spidev.SpiDev()
spi.open(0, 0)
response = spi.xfer2([0x00])
print(f"MISO: 0x{response[0]:02X}")  # CPLD passthrough test
```

---

## 📚 Referanslar

- **Altera MAX V Handbook:** https://www.intel.com/content/www/us/en/docs/programmable/683643/
- **OpenOCD GPIO Driver:** https://openocd.org/doc/html/Debug-Adapter-Configuration.html
- **Raspberry Pi Pinout:** https://pinout.xyz/
- **Quartus Prime Lite:** https://www.intel.com/content/www/us/en/software/programmable/quartus-prime/download.html

---

## 🎓 Öğrenilen Dersler

1. **Donanım Dokümantasyonu Kritik**
   - Template'ler her zaman doğru olmayabilir
   - Fiziksel cihazdan ID okumak şart

2. **Modern Toolchain Kullan**
   - Eski araçlar (2004) yeni formatlarla çalışmaz
   - OpenOCD aktif geliştiriliyor, güvenilir

3. **Pin Mapping Doğrulama**
   - Multimetre ile fiziksel bağlantıları kontrol et
   - GPIO test scripti ile sinyal doğrula

4. **Path Temizliği**
   - Tüm geliştirme ASCII karakterlerle
   - Windows'ta kısa path kullan

5. **Incremental Testing**
   - Her adımı test et (GPIO → JTAG → IDCODE → Program)
   - Problem izolasyonu kolaylaşır

---

**Sonuç:** CPLD başarıyla programlandı ve SPI passthrough çalışıyor! 🎉

**Sonraki Adım:** STM32 mikrocontroller programlama

---

*Bu doküman, gelecekteki benzer projelerde zaman kazandırmak için yazılmıştır.*
*Hatalar üzerinden öğreniyoruz! 🚀*
