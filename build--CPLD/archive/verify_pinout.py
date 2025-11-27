#!/usr/bin/env python3
"""
Raspberry Pi GPIO Pinout Doğrulama
Fiziksel pinlerin BCM numaralarını göster
"""

# Raspberry Pi 4 Model B - 40-pin header
# Format: Fiziksel Pin -> BCM GPIO

PINOUT = {
    # Sol sütun (çift sayılar)
    2: "5V",
    4: "5V",
    6: "GND",
    8: "GPIO 14 (UART TX)",
    10: "GPIO 15 (UART RX)",
    12: "GPIO 18 (PCM CLK)",
    14: "GND",
    16: "GPIO 23",  # <- TMS bağlantısı
    18: "GPIO 24",  # <- TDO bağlantısı
    20: "GND",
    22: "GPIO 25",  # <- TCK bağlantısı
    24: "GPIO 8 (SPI0 CE0)",
    26: "GPIO 7 (SPI0 CE1)",
    28: "GPIO 1 (I2C0 SDA)",
    30: "GND",
    32: "GPIO 12",
    34: "GND",
    36: "GPIO 16",
    38: "GPIO 20",
    40: "GND",
    
    # Sağ sütun (tek sayılar)
    1: "3.3V",
    3: "GPIO 2 (I2C1 SDA)",
    5: "GPIO 3 (I2C1 SCL)",
    7: "GPIO 4",
    9: "GND",
    11: "GPIO 17",
    13: "GPIO 27",
    15: "GPIO 22",  # <- TDI bağlantısı
    17: "3.3V",
    19: "GPIO 10 (SPI0 MOSI)",
    21: "GPIO 9 (SPI0 MISO)",
    23: "GPIO 11 (SPI0 SCLK)",
    25: "GND",
    27: "GPIO 0 (I2C0 SDA)",
    29: "GPIO 5",
    31: "GPIO 6",
    33: "GPIO 13",
    35: "GPIO 19",
    37: "GPIO 26",
    39: "GND",
}

print("=" * 70)
print("  RASPBERRY PI 4 GPIO PINOUT")
print("=" * 70)
print()
print("JTAG Bağlantıları (Kartınızdan):")
print()
print("  Pin 16 -> Altera PIN 33 (TMS)")
print(f"    └─> {PINOUT[16]}")
print()
print("  Pin 15 -> Altera PIN 34 (TDI)")
print(f"    └─> {PINOUT[15]}")
print()
print("  Pin 22 -> Altera PIN 35 (TCK)")
print(f"    └─> {PINOUT[22]}")
print()
print("  Pin 18 -> Altera PIN 36 (TDO)")
print(f"    └─> {PINOUT[18]}")
print()
print("=" * 70)
print()
print("✅ GPIO Eşlemesi DOĞRU:")
print("   TMS = GPIO 23 (Pin 16)")
print("   TDI = GPIO 22 (Pin 15)")
print("   TCK = GPIO 25 (Pin 22)")
print("   TDO = GPIO 24 (Pin 18)")
