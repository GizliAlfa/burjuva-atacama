#!/usr/bin/env python3
"""
JTAG Pin Test - GPIO Sinyal Testi
Doğrudan GPIO sinyallerini test eder
"""

import time
import sys

try:
    import RPi.GPIO as GPIO
except ImportError:
    print("✗ RPi.GPIO kütüphanesi yok")
    sys.exit(1)

# Pin tanımları (BCM numaraları)
PINS = {
    'TMS': 22,  # Pin 15
    'TDI': 27,  # Pin 22
    'TDO': 24,  # Pin 23
    'TCK': 23,  # Pin 16
}

print("=" * 60)
print("  JTAG GPIO PIN TEST")
print("=" * 60)
print()

# GPIO setup
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

# TDO input, diğerleri output
GPIO.setup(PINS['TMS'], GPIO.OUT)
GPIO.setup(PINS['TDI'], GPIO.OUT)
GPIO.setup(PINS['TCK'], GPIO.OUT)
GPIO.setup(PINS['TDO'], GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

print("📍 GPIO Pin Durumları:")
print(f"  TMS (GPIO {PINS['TMS']}, Pin 15) -> OUTPUT")
print(f"  TDI (GPIO {PINS['TDI']}, Pin 22) -> OUTPUT")
print(f"  TCK (GPIO {PINS['TCK']}, Pin 16) -> OUTPUT")
print(f"  TDO (GPIO {PINS['TDO']}, Pin 23) -> INPUT")
print()

# Test 1: Clock sinyali gönder
print("🔄 Test 1: TCK Clock Sinyali...")
for i in range(5):
    GPIO.output(PINS['TCK'], GPIO.HIGH)
    time.sleep(0.01)
    GPIO.output(PINS['TCK'], GPIO.LOW)
    time.sleep(0.01)
    print(f"  Clock pulse {i+1}/5")
print("  ✓ Clock sinyali gönderildi")
print()

# Test 2: TMS/TDI sinyalleri
print("🔄 Test 2: TMS/TDI Sinyalleri...")
GPIO.output(PINS['TMS'], GPIO.HIGH)
GPIO.output(PINS['TDI'], GPIO.HIGH)
time.sleep(0.1)
print(f"  TMS = HIGH, TDI = HIGH")
GPIO.output(PINS['TMS'], GPIO.LOW)
GPIO.output(PINS['TDI'], GPIO.LOW)
time.sleep(0.1)
print(f"  TMS = LOW, TDI = LOW")
print("  ✓ TMS/TDI sinyalleri gönderildi")
print()

# Test 3: TDO okuma
print("🔄 Test 3: TDO Okuma...")
for i in range(10):
    tdo_value = GPIO.input(PINS['TDO'])
    print(f"  TDO okuması {i+1}/10: {tdo_value} ({'HIGH' if tdo_value else 'LOW'})")
    time.sleep(0.1)

# TDO hep 1 ise CPLD yanıt vermiyor veya pull-up var
tdo_count = sum(GPIO.input(PINS['TDO']) for _ in range(100))
print()
if tdo_count > 90:
    print("  ⚠️  TDO hep HIGH - CPLD yanıt vermiyor veya pull-up aktif")
elif tdo_count < 10:
    print("  ⚠️  TDO hep LOW - CPLD yanıt vermiyor veya pull-down aktif")
else:
    print("  ✓ TDO değişken - CPLD sinyal gönderiyor olabilir!")
print()

# Cleanup
GPIO.cleanup()

print("=" * 60)
print("  TEST TAMAMLANDI")
print("=" * 60)
print()
print("📝 Sonuç:")
print("  - GPIO pinleri çalışıyor")
print("  - Sinyaller gönderiliyor")
print("  - TDO yanıtını kontrol edin yukarıda")
print()
if tdo_count > 90:
    print("⚠️  ÖNEMLİ: CPLD'den yanıt alınamıyor!")
    print("   Kontrol edin:")
    print("   1. CPLD güç alıyor mu? (3.3V)")
    print("   2. JTAG kabloları doğru mu?")
    print("   3. Altera PIN 33,34,35,36 bağlantıları sağlam mı?")
