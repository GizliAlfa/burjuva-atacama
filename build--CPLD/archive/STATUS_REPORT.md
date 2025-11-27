# CPLD Programlama Durum Raporu
# Tarih: 10 Kasım 2025
# ============================================================

## ✅ BAŞARILANLAR

1. **CPLD Firmware Derleme**
   - Quartus 25.1std ile başarıyla derlendi
   - Dosya: C:\temp\cpld-build\output_files\cpld.svf (271 KB)
   - Device: Altera MAX V 5M80ZT100C5
   - Logic kullanımı: 2/80 (%3)
   - Pin kullanımı: 76/79 (%96)

2. **GPIO Pin Testi**
   - Raspberry Pi GPIO 22, 27, 23, 24 pinleri çalışıyor
   - Clock ve data sinyalleri başarıyla gönderiliyor
   - Python RPi.GPIO ile test edildi

3. **JTAG Pin Eşlemesi (Düzeltildi)**
   ```
   Raspberry Pi          Fiziksel Pin    Altera CPLD
   GPIO 22 (TMS)    ->   Pin 15      ->  Altera PIN 33
   GPIO 27 (TDI)    ->   Pin 22      ->  Altera PIN 34
   GPIO 23 (TCK)    ->   Pin 16      ->  Altera PIN 35
   GPIO 24 (TDO)    <-   Pin 23      <-  Altera PIN 36
   GND              ->   Pin 3/7     ->  GND
   ```

## ❌ SORUN

**TDO Sinyali Hep HIGH (0xFFFF)**
- CPLD'den JTAG yanıtı alınamıyor
- OpenOCD hatası: "JTAG scan chain interrogation failed: all ones"
- IR capture hatası: 0x3ff okunuyor (0x1 bekleniyor)

## 🔍 YAPILACAK FİZİKSEL KONTROLLER

### 1. CPLD Güç Kontrolü
- [ ] CPLD'nin VCC pinlerinde 3.3V var mı? (Multimetre)
- [ ] GND bağlantısı sağlam mı?
- [ ] Pin 6: "Altera PIN25 VCCIO1" - bu CPLD'nin gücü mü?

### 2. JTAG Bağlantı Kontrolü (Multimetre - Süreklilik Testi)
- [ ] RPI Pin 15 <-> Altera PIN 33 (TMS)
- [ ] RPI Pin 22 <-> Altera PIN 34 (TDI)
- [ ] RPI Pin 16 <-> Altera PIN 35 (TCK)
- [ ] RPI Pin 23 <-> Altera PIN 36 (TDO)
- [ ] GND ortak mı?

### 3. CPLD Durumu
- [ ] CPLD'de bir LED var mı? Yanıyor mu?
- [ ] CPLD'nin sıcaklığı normal mi? (Aşırı ısınma = hasar)
- [ ] Gözle görünür hasar var mı?

### 4. Alternatif: SPI Test
JTAG çalışmasa bile, CPLD daha önce programlanmışsa SPI üzerinden yanıt verebilir:
- [ ] python3 /tmp/cpld_spi_test.py çalıştır
- [ ] MISO pininden 0x00 dışında bir değer geliyor mu?

## 📝 SONRAKİ ADIMLAR

**Eğer CPLD yanıt vermiyorsa:**
1. Fiziksel bağlantıları multimetre ile kontrol et
2. CPLD'nin güç aldığından emin ol
3. Alternatif bir CPLD ile test et (varsa)

**Eğer CPLD yanıt veriyorsa:**
1. `sudo /tmp/program_cpld_fixed.sh` çalıştır
2. Programlama tamamlandıktan sonra SPI testi yap
3. Modül taramasını çalıştır

## 🛠️ HAZIR DOSYALAR

Raspberry Pi'de (`/tmp/`):
- cpld.svf - CPLD firmware (271 KB)
- cpld_program_correct.cfg - Düzeltilmiş OpenOCD config
- program_cpld_fixed.sh - Programlama scripti
- gpio_jtag_test.py - GPIO test scripti
- cpld_spi_test.py - SPI iletişim testi (önceki session)

Windows'ta (`C:\temp\cpld-build\`):
- Tüm Verilog kaynak kodları
- Quartus proje dosyaları
- Derlenmiş output_files/
