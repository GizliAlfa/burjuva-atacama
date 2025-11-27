#!/usr/bin/env python3
"""
Hızlı CPLD SPI Test - CPLD'nin SPI üzerinden yanıt verip vermediğini test eder
"""

import spidev
import sys

print("=" * 60)
print("  CPLD SPI İLETİŞİM TESTİ")
print("=" * 60)
print()

try:
    spi = spidev.SpiDev()
    spi.open(0, 0)  # SPI0, CS0
    spi.max_speed_hz = 500000
    spi.mode = 0
    print("✓ SPI0.0 açıldı (500 kHz)")
except Exception as e:
    print(f"✗ SPI açılamadı: {e}")
    sys.exit(1)

print()
print("🔄 Test 1: Null Command (0x00)")
response = spi.xfer2([0x00])
print(f"   Gönderilen: 0x00")
print(f"   Yanıt: 0x{response[0]:02X}")

if response[0] == 0x00:
    print("   ⚠️  Yanıt 0x00 - CPLD yanıt vermiyor veya boşta")
elif response[0] == 0xFF:
    print("   ⚠️  Yanıt 0xFF - CPLD bağlı değil veya MISO floating")
else:
    print("   ✓ CPLD yanıt veriyor!")

print()
print("🔄 Test 2: Status Read (0x01)")
response = spi.xfer2([0x01, 0x00])
print(f"   Gönderilen: [0x01, 0x00]")
print(f"   Yanıt: [0x{response[0]:02X}, 0x{response[1]:02X}]")

if response == [0xFF, 0xFF]:
    print("   ⚠️  All 0xFF - CPLD programlanmamış veya MISO bağlı değil")
elif response == [0x00, 0x00]:
    print("   ⚠️  All 0x00 - CPLD yanıt vermiyor")
else:
    print("   ✓ CPLD'den veri geliyor!")

print()
print("🔄 Test 3: Pattern Test")
test_patterns = [0xAA, 0x55, 0xF0, 0x0F]
for pattern in test_patterns:
    response = spi.xfer2([pattern])
    print(f"   Test 0x{pattern:02X} -> Yanıt 0x{response[0]:02X}", end="")
    if response[0] == pattern:
        print(" (Echo - loop back?)")
    elif response[0] == 0xFF:
        print(" (All 1s)")
    elif response[0] == 0x00:
        print(" (All 0s)")
    else:
        print(" (CPLD yanıtı!)")

spi.close()

print()
print("=" * 60)
print("  TEST TAMAMLANDI")
print("=" * 60)
print()
print("📝 SONUÇ:")
print("   Eğer tüm yanıtlar 0x00 veya 0xFF ise:")
print("   -> CPLD programlanmamış veya SPI bridge çalışmıyor")
print()
print("   Eğer farklı değerler geliyorsa:")
print("   -> CPLD SPI üzerinden çalışıyor!")
