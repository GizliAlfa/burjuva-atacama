# CPLD Firmware - Burjuva Pilot

## 📦 İçindekiler

Bu klasör, Altera MAX V 5M80ZT100C5 CPLD'nin firmware'ini içerir.

### ✅ Durum: PROGRAMLANDI

**Tarih:** 10 Kasım 2025  
**Device:** Altera MAX V 5M80ZT100C5  
**IDCODE:** 0x020a50dd (doğrulandı)  
**Firmware Fonksiyonu:** RPI ↔ CPLD ↔ STM32 SPI passthrough + 4x module routing

---

## 📖 Dokümantasyon

### 🎓 Başlamadan Önce Okuyun
1. **[LESSONS_LEARNED.md](./LESSONS_LEARNED.md)** ⭐ **ÖNEMLİ!**
   - Yaşanan tüm sorunlar ve çözümleri
   - Neler yanlış gitti, nasıl düzeltildi
   - Dikkat edilmesi gerekenler
   - **İlk bunu okuyun!** Çok zaman kazandırır

2. **[PROGRAMMING_GUIDE.md](./PROGRAMMING_GUIDE.md)**
   - Adım adım programlama rehberi
   - Quartus derleme talimatları
   - OpenOCD kullanımı
   - Pin mapping ve bağlantılar
   - Troubleshooting

3. **[ARCHITECTURE_ANALYSIS.md](./ARCHITECTURE_ANALYSIS.md)**
   - CPLD mimarisi analizi
   - Module routing mantığı
   - SPI protocol detayları

4. **[BUILD.md](./BUILD.md)**
   - Quartus build process
   - Compiler ayarları

---

## 📂 Dosya Yapısı

### ✅ Kaynak Kodlar (Verilog)
- **`top.v`** (322 satır) - Ana CPLD modülü
  - RPI SPI bridge (pin 12/14/27/28/52)
  - 4x module connector routing (CON0-3)
  - STM32 GPIO mapping (PA/PB/PC/PD)
  
- **`rpi.v`** (27 satır) - SPI passthrough modülü
  - RPI SPI → STM32 SPI direkt bağlantı
  
- **`testin.v`** (44 satır) - Input module routing
  - Debouncing logic
  
- **`testout.v`** (54 satır) - Output module routing

### ⚙️ Quartus Proje Dosyaları
- **`cpld.qpf`** - Quartus Project File
- **`cpld.qsf`** - Quartus Settings File (pin assignments)
- **`cpld_assignment_defaults.qdf`** - Default assignments

### 🔧 Programlama Araçları
- **`openocd_cpld.cfg`** ✅ **ÇALIŞAN CONFIG**
  - OpenOCD configuration (bcm2835gpio driver)
  - GPIO pin mapping: 22/23/24/25
  - Doğru IDCODE: 0x020a50dd
  
### 🧪 Test Scripts
- **`quick_spi_test.py`** - CPLD SPI iletişim testi
- **`jtag_final_test.py`** - JTAG sinyalleri doğrulama

### 📦 Output Files (Derleme Çıktıları)
```
output_files/
├── cpld.svf        ✅ Programlanan firmware (271 KB)
├── cpld.pof        - Programmer Object File (7.8 KB)
├── cpld.jam        - JAM STAPL (50 KB, eski toolchain için)
├── cpld.fit.summary - Fit raporu (2/80 LE kullanıldı)
└── *.rpt           - Detaylı raporlar
```

### 🗂️ Archive (Eski Denemeler)
```
archive/
├── cpld_program*.cfg       - Başarısız OpenOCD config'ler
├── openocd_*.cfg           - Deneysel config'ler
├── jtag_test*.py           - Debug scriptleri
├── program_cpld*.sh        - Eski programlama scriptleri
└── STATUS*.md              - Eski durum raporları
```

---

## 🚀 Hızlı Başlangıç

### 1️⃣ İlk Kez Programlama

**Windows'ta Quartus ile derleyin:**
```powershell
cd C:\temp\cpld-build  # Türkçe karaktersiz path!
quartus_sh --flow compile cpld
```

**Raspberry Pi'ye kopyalayın:**
```powershell
scp output_files\cpld.svf burjuva@192.168.1.22:/tmp/
scp openocd_cpld.cfg burjuva@192.168.1.22:/tmp/
```

**Raspberry Pi'de programlayın:**
```bash
sudo openocd -f /tmp/openocd_cpld.cfg \
  -c 'svf /tmp/cpld.svf; shutdown'
```

### 2️⃣ Test

**SPI iletişim testi:**
```bash
python3 quick_spi_test.py
```

**Beklenen sonuç:** CPLD SPI passthrough çalışıyor (STM32'den cevap gelecek)

---

## ⚠️ Önemli Notlar

### 🔴 IDCODE Dikkat!
- **Template IDCODE:** 0x020a10dd (YANLIŞ!)
- **Gerçek IDCODE:** 0x020a50dd (Bu karttaki cihaz)
- `openocd_cpld.cfg` dosyasında doğru IDCODE kullanılmalı

### 🔤 Türkçe Karakter Sorunu
- Quartus, path'te Türkçe karakter tanımıyor
- **Çözüm:** Projeyi `C:\temp\cpld-build\` gibi yola kopyala

### 🎭 JAM Player Uyumsuzluğu
- Mevcut sistem `jamplayer` (2004) çok eski
- Modern Quartus (2025) JAM formatı uyumlu değil
- **Çözüm:** SVF formatı + OpenOCD kullan

### 📍 Pin Mapping
```
RPI Pin (fiziksel) -> BCM GPIO -> CPLD Pin -> Sinyal
Pin 16 (çift/sol)  -> GPIO 23   -> PIN 33  -> TMS
Pin 15 (tek/sağ)   -> GPIO 22   -> PIN 34  -> TDI
Pin 22 (çift/sol)  -> GPIO 25   -> PIN 35  -> TCK
Pin 18 (çift/sol)  -> GPIO 24   -> PIN 36  -> TDO
Pin 6  (çift/sol)  -> GND       -> GND     -> GND
```

---

## 🛠️ Troubleshooting

### "Error: IR capture error"
**Sebep:** Yanlış GPIO pin mapping veya kablo bağlantı hatası  
**Çözüm:** `gpio readall` ile pinleri kontrol et, multimetre ile fiziksel bağlantıları test et

### "JTAG tap: unexpected IDCODE"
**Sebep:** Config'de yanlış IDCODE  
**Çözüm:** `scan_chain` ile gerçek IDCODE'u oku, `openocd_cpld.cfg`'de güncelle

### "Can't create project" (Quartus)
**Sebep:** Path'te Türkçe karakter var  
**Çözüm:** Projeyi `C:\temp\` gibi Türkçe karaktersiz yola kopyala

### SPI test 0x00 dönüyor
**Sebep:** STM32 henüz programlanmadı (normal)  
**Çözüm:** STM32'yi programladıktan sonra tekrar test et

---

## 📊 Kaynak Kullanımı

```
Altera MAX V 5M80ZT100C5
=========================
Logic Elements: 2 / 80 (3%)
Total Pins:     76 / 79 (96%)
UFM Blocks:     0 / 1 (0%)
Max Frequency:  ~100 MHz
```

---

## 🔗 Referanslar

- **Altera MAX V Handbook:** https://www.intel.com/content/www/us/en/docs/programmable/683643/
- **OpenOCD Documentation:** https://openocd.org/doc/html/
- **Raspberry Pi Pinout:** https://pinout.xyz/
- **Quartus Download:** https://www.intel.com/content/www/us/en/software/programmable/quartus-prime/download.html

---

## 📞 Sorun mu Yaşıyorsunuz?

1. **[LESSONS_LEARNED.md](./LESSONS_LEARNED.md)** dokümanını okuyun (çoğu sorun orada)
2. **[PROGRAMMING_GUIDE.md](./PROGRAMMING_GUIDE.md)** troubleshooting bölümüne bakın
3. `scan_chain` ile IDCODE doğrulayın
4. GPIO pinleri multimetre ile test edin
5. OpenOCD log'unu `-d3` (debug) modunda inceleyin

---

**Son Güncelleme:** 10 Kasım 2025  
**Durum:** ✅ ÇALIŞIYOR

🎉 **CPLD başarıyla programlandı ve test edildi!**
