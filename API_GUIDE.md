# API_GUIDE.md - Komut Referansı ve Kullanım Örnekleri

**Burjuva Atacama - Detaylı API Dokümantasyonu**  
**Tarih:** 27 Kasım 2025

---

## 📡 UART Protokol Özellikleri

### Bağlantı Ayarları

```
Baud Rate:    115200
Data Bits:    8
Parity:       None
Stop Bits:    1
Flow Control: None
```

### Komut Formatı

**Genel Yapı:**
```
<komut>[:<parametre1>[:<parametre2>...]]<CR/LF>
```

**Özellikler:**
- Case-insensitive (büyük/küçük harf duyarsız)
- `\r` (CR) veya `\n` (LF) ile sonlandırma
- Boşluk karakterleri desteklenmez
- Maksimum komut uzunluğu: 64 karakter

**Yanıt Formatı:**
```
[ACK] Komut alindi: <komut>
<sonuc_verileri>
Komut tamamlandi: <komut>
```

---

## 🔧 Genel Sistem Komutları

### help / yardim

Kullanılabilir komutların listesini gösterir.

**Syntax:**
```bash
help
# veya
yardim
```

**Çıktı:**
```
Mevcut Komutlar:
  modul-algila              -> Bagli modulleri tara
  io16:SLOT:KOMUT           -> IO16 modul kontrolu
  aio20:SLOT:KOMUT          -> AIO20 modul kontrolu
  help                      -> Bu yardim mesaji

Ornek:
  io16:0:set:5:high         -> Slot 0, Pin 5 = HIGH
  aio20:1:readin:3          -> Slot 1, AI3 oku
```

---

### modul-algila

1-Wire protokolü ile bağlı modülleri algılar ve kaydeder.

**Syntax:**
```bash
modul-algila
```

**Çıktı Örneği:**
```
========================================
  BURJUVA MODULE DETECTION
========================================
Protocol: 1-Wire OVERDRIVE SPEED
Slots: PC2(0), PC0(1), PC3(2), PC1(3)
Clock: 72MHz (HSE + PLL)
========================================

Slot 0 (PC2): -> FOUND!
  UID: 2B 00 00 01 23 45 67 89 (Family: 2B=DS2431)
  FID: 69 6F 31 36 20 20 20 21
  TYPE: IO16 - 16 Channel Digital I/O
  NAME: io16   !
  [REGISTERED] IO16 module at slot 0
  [INIT] Initializing IO678 chip...
  [SUCCESS] IO16 chip initialized after 3 tries!

Slot 1 (PC0): -> FOUND!
  UID: 2B 00 00 02 34 56 78 9A
  FID: 61 69 6F 32 30 20 20 65
  TYPE: AIO20 - 20 Channel Analog I/O
  NAME: aio20  e
  [REGISTERED] AIO20 module at slot 1

Slot 2 (PC3): -> EMPTY
Slot 3 (PC1): -> EMPTY

========================================
Scan Complete!
========================================
```

**Not:** Modül takılı/çıkarıldığında bu komutu tekrar çalıştırın.

---

## 🔌 IO16 Komutları

IO16 modülleri iC-JX678 chip'i kullanır ve 16 GPIO hattı sunar.

### Slot Numaraları
- **Slot 0:** İlk IO16 modülü (CS: PC13)
- **Slot 3:** İkinci IO16 modülü (CS: PA2)

---

### io16:read

Tek bir GPIO pininin durumunu okur.

**Syntax:**
```bash
io16:<SLOT>:read:<PIN>
```

**Parametreler:**
- `SLOT`: 0 veya 3
- `PIN`: 0-15 arası pin numarası

**Örnek:**
```bash
> io16:0:read:5
[ACK] Komut alindi: io16:0:read:5
Pin 5: HIGH (1)
Komut tamamlandi: io16
```

---

### io16:write

Tek bir GPIO pinini yazar (HIGH veya LOW).

**Syntax:**
```bash
io16:<SLOT>:write:<PIN>:<VALUE>
```

**Parametreler:**
- `SLOT`: 0 veya 3
- `PIN`: 0-15 arası pin numarası
- `VALUE`: `high`, `low`, `1`, `0`

**Örnekler:**
```bash
# Pin 7'yi HIGH yap
> io16:0:write:7:high
[ACK] Komut alindi: io16:0:write:7:high
Pin 7 yazildi: HIGH
Komut tamamlandi: io16

# Pin 3'ü LOW yap
> io16:3:write:3:low
Pin 3 yazildi: LOW
```

---

### io16:mode

Pin yönünü ayarlar (input veya output).

**Syntax:**
```bash
io16:<SLOT>:mode:<PIN>:<MODE>
```

**Parametreler:**
- `MODE`: `input`, `output`, `in`, `out`

**Örnek:**
```bash
> io16:0:mode:8:output
Pin 8 modu: OUTPUT
```

---

### io16:direction

Grup bazlı direction kontrolü (iC-JX678 özelliği).

**Syntax:**
```bash
io16:<SLOT>:direction:<GROUP>:<DIR>
```

**Parametreler:**
- `GROUP`: 0-3 (her grup 4 pin kontrolü: 0→0-3, 1→4-7, 2→8-11, 3→12-15)
- `DIR`: `input`, `output`

**Örnekler:**
```bash
# Grup 0 (pin 0-3) output yap
> io16:0:direction:0:output
Direction Grup 0: OUTPUT

# Grup 2 (pin 8-11) input yap
> io16:0:direction:2:input
Direction Grup 2: INPUT
```

---

### io16:readall

Tüm 16 pinin durumunu okur.

**Syntax:**
```bash
io16:<SLOT>:readall
```

**Örnek:**
```bash
> io16:0:readall
Port State (16-bit): 0xA5F0
Pin  0: HIGH    Pin  8: LOW
Pin  1: LOW     Pin  9: HIGH
Pin  2: LOW     Pin 10: LOW
Pin  3: HIGH    Pin 11: HIGH
...
```

---

### io16:writeport

16-bit port değerini tek seferde yazar.

**Syntax:**
```bash
io16:<SLOT>:writeport:<VALUE>
```

**Parametreler:**
- `VALUE`: 0x0000-0xFFFF arası hex değer

**Örnek:**
```bash
> io16:0:writeport:0xFF00
Port yazildi: 0xFF00
# Pin 8-15: HIGH, Pin 0-7: LOW
```

---

### io16:toggle

Bir pinin durumunu tersine çevirir.

**Syntax:**
```bash
io16:<SLOT>:toggle:<PIN>
```

**Örnek:**
```bash
> io16:0:toggle:5
Pin 5: HIGH → LOW
```

---

## 📊 AIO20 Komutları

AIO20 modülü MAX11300 PIXI chip'i kullanır ve 20 analog kanal sunar.

### Kanal Yapısı
- **0-11:** Analog Input (AI0-AI11)
- **12-19:** Analog Output (AO0-AO7)

---

### aio20:readin

Analog input kanalını okur (voltaj).

**Syntax:**
```bash
aio20:<SLOT>:readin:<CHANNEL>
```

**Parametreler:**
- `SLOT`: 1 (AIO20 her zaman slot 1'de)
- `CHANNEL`: 0-11 (analog input kanalları)

**Örnek:**
```bash
> aio20:1:readin:3
[ACK] Komut alindi: aio20:1:readin:3
AI3: 5.234 V
ADC Raw: 2145 (12-bit)
Komut tamamlandi: aio20
```

---

### aio20:writeout

Analog output kanalına voltaj yazar.

**Syntax:**
```bash
aio20:<SLOT>:writeout:<CHANNEL>:<VOLTAGE>
```

**Parametreler:**
- `CHANNEL`: 12-19 (analog output kanalları)
- `VOLTAGE`: 0.0-10.0 arası (0-10V modu) veya -10.0-10.0 (±10V modu)

**Örnekler:**
```bash
# 7.5V yaz
> aio20:1:writeout:12:7.5
AO0 (CH12): 7.500 V yazildi
DAC: 3072 (12-bit)

# Negatif voltaj (±10V modunda)
> aio20:1:writeout:13:-3.2
AO1 (CH13): -3.200 V yazildi
```

---

### aio20:config

Kanal modunu yapılandırır.

**Syntax:**
```bash
aio20:<SLOT>:config:<CHANNEL>:<MODE>
```

**Modlar:**

| Mode | Değer | Açıklama |
|------|-------|----------|
| `ain_0_10v` | 0 | Analog input, 0-10V |
| `ain_bipolar` | 1 | Analog input, ±10V |
| `ain_4_20ma` | 2 | Analog input, 4-20mA |
| `aout_0_10v` | 3 | Analog output, 0-10V |
| `aout_bipolar` | 4 | Analog output, ±10V |
| `aout_4_20ma` | 5 | Analog output, 4-20mA |
| `dac` | 6 | DAC mode (raw) |
| `adc` | 7 | ADC mode (raw) |
| `gpio` | 8 | GPIO mode |

**Örnek:**
```bash
> aio20:1:config:0:ain_0_10v
CH0 konfigüre edildi: AIN 0-10V
```

---

### aio20:readadc

ADC raw değerini okur (12-bit, 0-4095).

**Syntax:**
```bash
aio20:<SLOT>:readadc:<CHANNEL>
```

**Örnek:**
```bash
> aio20:1:readadc:5
CH5 ADC: 2048 (12-bit)
Voltaj: ~5.000 V
```

---

### aio20:writedac

DAC raw değeri yazar (12-bit, 0-4095).

**Syntax:**
```bash
aio20:<SLOT>:writedac:<CHANNEL>:<VALUE>
```

**Parametreler:**
- `VALUE`: 0-4095 (12-bit)

**Örnek:**
```bash
> aio20:1:writedac:12:3072
CH12 DAC: 3072 yazildi
Voltaj: ~7.500 V (0-10V modunda)
```

---

### aio20:status

MAX11300 chip durumunu gösterir.

**Syntax:**
```bash
aio20:<SLOT>:status
```

**Örnek:**
```bash
> aio20:1:status
========================================
MAX11300 PIXI Status (Slot 1)
========================================
Device ID: 0x0424
Firmware: v1.2
BRST: 0x0000
LPEN: 0x0000
Active Channels: 8
  CH0: AIN 0-10V   → 5.234 V
  CH1: AIN 0-10V   → 3.102 V
  CH12: AOUT 0-10V → 7.500 V
  CH13: AOUT 0-10V → 2.100 V
========================================
```

---

## 🎯 Kullanım Senaryoları

### Senaryo 1: LED Kontrolü (IO16)

```bash
# LED'i output yap
> io16:0:mode:7:output

# LED'i yak
> io16:0:write:7:high

# 1 saniye bekle...

# LED'i söndür
> io16:0:write:7:low

# Toggle ile yanıp söndür
> io16:0:toggle:7  # Yak
> io16:0:toggle:7  # Söndür
```

---

### Senaryo 2: Buton Okuma (IO16)

```bash
# Butonu input yap (pull-up aktif)
> io16:0:mode:3:input

# Buton durumunu oku
> io16:0:read:3
Pin 3: HIGH (buton basılı değil)

# Tekrar oku
> io16:0:read:3
Pin 3: LOW (buton basılı!)
```

---

### Senaryo 3: Analog Sensör Okuma (AIO20)

```bash
# Kanalı 0-10V input olarak yapılandır
> aio20:1:config:2:ain_0_10v

# Sensör voltajını oku
> aio20:1:readin:2
AI2: 6.543 V

# Sürekli okuma (Python script ile):
while True:
    read_analog(slot=1, channel=2)
    time.sleep(0.1)
```

---

### Senaryo 4: Analog Çıkış Kontrolü (AIO20)

```bash
# Kanalı 0-10V output olarak yapılandır
> aio20:1:config:12:aout_0_10v

# 0V'dan 10V'a ramp
for v in range(0, 101, 5):
    voltage = v / 10.0
    aio20:1:writeout:12:{voltage}
    wait(100ms)
```

---

### Senaryo 5: 4-20mA Akım Döngüsü (AIO20)

```bash
# Input olarak 4-20mA yapılandır
> aio20:1:config:4:ain_4_20ma

# Akım oku (mA cinsinden)
> aio20:1:readin:4
AI4: 12.34 mA
# Not: 4mA = 0%, 20mA = 100%
# 12.34mA = ~52% sensör değeri
```

---

## 🚨 Hata Kodları ve Anlamları

### IO16 Hataları

| Kod | Anlam | Çözüm |
|-----|-------|-------|
| `ERR_SLOT` | Geçersiz slot numarası | Slot 0 veya 3 kullanın |
| `ERR_PIN` | Geçersiz pin numarası | 0-15 arası pin seçin |
| `ERR_SPI` | SPI iletişim hatası | CS bağlantısını kontrol edin |
| `ERR_CHIP` | Chip yanıt vermiyor | Modülün takılı olduğunu doğrulayın |
| `ERR_INIT` | Chip init başarısız | `modul-algila` komutunu çalıştırın |

### AIO20 Hataları

| Kod | Anlam | Çözüm |
|-----|-------|-------|
| `ERR_CH` | Geçersiz kanal | 0-19 arası kanal seçin |
| `ERR_RANGE` | Voltaj aralık dışı | 0-10V veya ±10V aralığına dikkat |
| `ERR_MODE` | Yanlış mod | Kanal yapılandırmasını kontrol edin |
| `ERR_ADC` | ADC okuma hatası | CNVT sinyalini kontrol edin |
| `ERR_DAC` | DAC yazma hatası | Referans voltajını doğrulayın |

---

## 📐 Komut Karşılaştırma Tablosu

| Fonksiyon | IO16 Komutu | AIO20 Komutu |
|-----------|-------------|--------------|
| Tek okuma | `io16:0:read:5` | `aio20:1:readin:3` |
| Tek yazma | `io16:0:write:7:high` | `aio20:1:writeout:12:7.5` |
| Toplu okuma | `io16:0:readall` | `aio20:1:status` |
| Yapılandırma | `io16:0:mode:3:output` | `aio20:1:config:0:ain_0_10v` |
| Raw değer | - | `aio20:1:readadc:5` |

---

## 🔗 Ek Kaynaklar

- **iC-JX678 Datasheet:** GPIO direction control detayları
- **MAX11300 Datasheet:** PIXI channel modes ve timing
- **HARDWARE.md:** Elektriksel özellikler ve pin mapping
- **BUILD.md:** Firmware derleme ve yükleme

---

**Son Güncelleme:** 27 Kasım 2025  
**Yazar:** Burjuva Pilot Ekibi
