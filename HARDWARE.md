# HARDWARE.md - Donanım Dokümantasyonu

**Burjuva Atacama - Elektriksel ve Mekanik Özellikler**  
**Tarih:** 27 Kasım 2025

---

## 📐 Pin Mapping Tablosu

### STM32F103RCT6 Pin Kullanımı

| Pin | Fonksiyon | Açıklama | Yön |
|-----|-----------|----------|-----|
| **PA9** | USART1_TX | UART verici | OUT |
| **PA10** | USART1_RX | UART alıcı | IN |
| **PB13** | SPI2_SCK | SPI clock | OUT |
| **PB14** | SPI2_MISO | SPI master in | IN |
| **PB15** | SPI2_MOSI | SPI master out | OUT |
| **PC13** | CS_SLOT0 | IO16 #1 chip select | OUT |
| **PA0** | CS_SLOT1 | AIO20 chip select | OUT |
| **PA1** | CS_SLOT2 | RESERVED | OUT |
| **PA2** | CS_SLOT3 | IO16 #2 chip select | OUT |
| **PC0** | 1WIRE_SLOT1 | Modül algılama | I/O |
| **PC1** | 1WIRE_SLOT3 | Modül algılama | I/O |
| **PC2** | 1WIRE_SLOT0 | Modül algılama | I/O |
| **PC3** | 1WIRE_SLOT2 | Modül algılama | I/O |
| **PC13** | LED | Onboard LED | OUT |

---

## 🔌 Modül Slot Konfigürasyonu

### Slot 0 - IO16 #1

**Chip:** iC-JX678  
**SPI Pins:**
- CS: PC13 (aktif LOW)
- SCK: PB13 (paylaşımlı)
- MISO: PB14 (CPLD multiplexed)
- MOSI: PB15 (paylaşımlı)

**1-Wire:**
- Detection: PC2

**Özellikler:**
- 16× GPIO (configurable I/O)
- Grup bazlı direction kontrolü
- SPI hızı: 4.5 MHz (Prescaler /8)

---

### Slot 1 - AIO20

**Chip:** MAX11300 PIXI  
**SPI Pins:**
- CS: PA0 (aktif LOW)
- SCK: PB13 (paylaşımlı)
- MISO: PB14 (CPLD multiplexed)
- MOSI: PB15 (paylaşımlı)
- CNVT: PC5 (conversion trigger)

**1-Wire:**
- Detection: PC0

**Interrupt:**
- INT: PC4

**Özellikler:**
- 20× Analog channels (12 IN + 8 OUT)
- 12-bit ADC/DAC
- SPI hızı: 9.0 MHz (Prescaler /4)

---

### Slot 2 - RESERVED

**Status:** Kullanılmıyor (eski FPGA slot)  
**Pins:**
- CS: PA1
- 1-Wire: PC3

---

### Slot 3 - IO16 #2

**Chip:** iC-JX678  
**SPI Pins:**
- CS: PA2 (aktif LOW)
- SCK: PB13 (paylaşımlı)
- MISO: PB14 (CPLD multiplexed)
- MOSI: PB15 (paylaşımlı)

**1-Wire:**
- Detection: PC1

**Interrupt:**
- INT: PB11

**Özellikler:**
- 16× GPIO (configurable I/O)
- Grup bazlı direction kontrolü
- SPI hızı: 4.5 MHz (Prescaler /8)

---

## 🔀 CPLD Multiplexer Mantığı

CPLD (Complex Programmable Logic Device), 4 modül arasında SPI bus paylaşımını yönetir.

### MISO Multiplexer

```verilog
// CPLD içinde otomatik MISO switching
PB_14 = !PC13 ? IO16_MISO_0  :  // Slot 0 seçili
        !PA0  ? AIO20_MISO_1 :  // Slot 1 seçili
        !PA1  ? 0            :  // Slot 2 (RESERVED)
        !PA2  ? IO16_MISO_3  :  // Slot 3 seçili
        0;                      // Hiçbiri seçili değil
```

### CS Routing

Her slot'un CS pini **aktif LOW** olarak çalışır:
- CS = HIGH (1) → Modül deaktif
- CS = LOW (0) → Modül aktif

**Önemli:** Aynı anda sadece **1 slot** seçili olabilir!

---

## ⚡ Elektriksel Özellikler

### Power Supply

| Rail | Voltaj | Akım (Typ) | Akım (Max) | Kaynak |
|------|--------|------------|------------|--------|
| VDD | 3.3V | 50 mA | 100 mA | Regülatör |
| VDDA | 3.3V | 5 mA | 10 mA | Analog |
| IO_5V | 5.0V | 200 mA | 500 mA | Modüller |
| IO_24V | 24V | 1 A | 2 A | Endüstriyel I/O |

### GPIO Seviyeler

**STM32 (3.3V Logic):**
- VOH (min): 2.4V @ 2mA
- VOL (max): 0.4V @ 2mA
- VIH (min): 2.0V
- VIL (max): 0.8V

**Modüller (5V Tolerant):**
- IO16: 5V tolerant inputs
- AIO20: 3.3V logic (seviye çevirici gerekmez)

---

## 🕐 Timing Özellikleri

### SPI Timing

**Mode 0 (CPOL=0, CPHA=0):**

| Parametre | Min | Typ | Max | Birim |
|-----------|-----|-----|-----|-------|
| f_SCK (Slot 0/3) | - | 4.5 | 5.0 | MHz |
| f_SCK (Slot 1) | - | 9.0 | 10.0 | MHz |
| t_setup (CS) | 50 | 100 | - | µs |
| t_hold (CS) | 50 | 100 | - | µs |
| t_inter_byte | 10 | 20 | - | µs |

**CS Zamanlama:**
```
         ____________________      __________
CS      /                    \____/          \___
                   
         ↑                    ↑    ↑          ↑
         t_setup              t_hold  t_setup  
```

### 1-Wire Timing (Overdrive Speed)

| Parametre | Değer | Birim |
|-----------|-------|-------|
| t_RSTL (Reset LOW) | 70 | µs |
| t_RSTH (Reset HIGH) | 8.5 | µs |
| t_W1L (Write 1 LOW) | 1.0 | µs |
| t_W0L (Write 0 LOW) | 7.5 | µs |
| t_SLOT (Bit slot) | 10 | µs |
| t_REC (Recovery) | 7.0 | µs |

---

## 🔧 Mekanik Özellikler

### PCB Boyutları

- **Ana Kart:** 100mm × 80mm (4-layer PCB)
- **Modül Kartı:** 50mm × 40mm (2-layer PCB)
- **Bağlantı:** 2×10 pin header (2.54mm pitch)

### Connector Pin-out (CON0/1/2/3)

**20-pin IDC Header (2×10):**

```
Pin Layout (Top View):
 1  GND          VCC  2
 3  SCK         MOSI  4
 5  MISO          CS  6
 7  INT        1WIRE  8
 9  GPIO0      GPIO1 10
11  GPIO2      GPIO3 12
13  GPIO4      GPIO5 14
15  GPIO6      GPIO7 16
17  CNVT       RESET 18
19  +5V         +24V 20
```

### Montaj Notları

1. **CS Pinleri:** 10kΩ pull-up ile HIGH'da tut
2. **1-Wire:** 4.7kΩ pull-up gerekli
3. **SPI Bus:** 33Ω seri terminasyon (opsiyonel)
4. **Decoupling:** Her VDD pinine 100nF + 10µF

---

## 🛡️ ESD Koruması

Tüm harici pinlerde ESD diyotları bulunur:
- **ESD Rating:** ±2kV (Human Body Model)
- **Latch-up:** >100mA @ 125°C

**Koruma Elemanları:**
- TVS diyotlar: 24V hatlarında
- Schottky diyotlar: 5V hatlarında
- RC filtreleme: Analog girişlerde

---

## 📊 Performans Metrikleri

### Sistem Gecikmeleri

| İşlem | Gecikme | Açıklama |
|-------|---------|----------|
| SPI transfer (1 byte) | ~2 µs | @ 4.5 MHz |
| Modül algılama | ~100 ms | 1-Wire scan |
| UART komut işleme | <1 ms | Parse + execute |
| GPIO okuma (IO16) | ~50 µs | SPI round-trip |
| ADC okuma (AIO20) | ~100 µs | Conversion + SPI |

### Throughput

- **Max SPI:** ~1.125 MB/s (9 MHz × 8 bit)
- **UART:** 14.4 kB/s (115200 baud)
- **GPIO polling rate:** ~20 kHz (tüm 16 pin)
- **ADC sampling rate:** ~10 kSPS (tek kanal)

---

## 🔍 Debug ve Test Noktaları

### Test Points

| TP | Net | Fonksiyon |
|----|-----|-----------|
| TP1 | VDD | 3.3V power |
| TP2 | GND | Ground |
| TP3 | SCK | SPI clock |
| TP4 | MISO | SPI data in |
| TP5 | MOSI | SPI data out |
| TP6 | PA9 | UART TX |
| TP7 | PA10 | UART RX |

### LED Göstergeleri

| LED | Renk | Fonksiyon |
|-----|------|-----------|
| PWR | Yeşil | 3.3V power OK |
| ACT | Mavi | CPU activity (blink) |
| TX | Sarı | UART transmit |
| RX | Sarı | UART receive |

---

## 📝 Sertifikasyonlar

- **CE:** Uyumlu (EN 61000-6-2, EN 61000-6-4)
- **RoHS:** Uyumlu
- **Çalışma Sıcaklığı:** -20°C ~ +70°C
- **Saklama Sıcaklığı:** -40°C ~ +85°C
- **Nem:** 10% ~ 90% RH (yoğuşmasız)

---

**Son Güncelleme:** 27 Kasım 2025  
**Revizyon:** A
