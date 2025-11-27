#!/usr/bin/env python3
"""
JTAG Test - FINAL CORRECT GPIO mapping
Pin 16 (GPIO 23) -> TMS
Pin 15 (GPIO 22) -> TDI
Pin 22 (GPIO 25) -> TCK
Pin 18 (GPIO 24) -> TDO
"""

import time
import sys

try:
    import RPi.GPIO as GPIO
except ImportError:
    print("✗ RPi.GPIO library not found")
    sys.exit(1)

# FINAL CORRECT Pin mapping
PINS = {
    'TMS': 23,  # Pin 16 -> Altera PIN 33
    'TDI': 22,  # Pin 15 -> Altera PIN 34
    'TCK': 25,  # Pin 22 -> Altera PIN 35
    'TDO': 24,  # Pin 18 -> Altera PIN 36
}

print("=" * 70)
print("  CPLD JTAG TEST - FINAL CORRECT VERSION")
print("=" * 70)
print()
print("⚠️  ÖNEMLİ: Pin 12 (Altera PIN25 VCCIO1) güç aldığından emin olun!")
print()

# GPIO setup
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

# TDO input, others output
GPIO.setup(PINS['TMS'], GPIO.OUT)
GPIO.setup(PINS['TDI'], GPIO.OUT)
GPIO.setup(PINS['TCK'], GPIO.OUT)
GPIO.setup(PINS['TDO'], GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

print("📍 GPIO Pin Mapping (FINAL CORRECT):")
print(f"  TMS: GPIO {PINS['TMS']} (Pin 16) -> Altera PIN 33")
print(f"  TDI: GPIO {PINS['TDI']} (Pin 15) -> Altera PIN 34")
print(f"  TCK: GPIO {PINS['TCK']} (Pin 22) -> Altera PIN 35")
print(f"  TDO: GPIO {PINS['TDO']} (Pin 18) -> Altera PIN 36")
print()

# Test 1: Reset sequence (TAP State Machine)
print("🔄 Test 1: JTAG Reset Sequence (5x clock with TMS=1)...")
GPIO.output(PINS['TMS'], GPIO.HIGH)
GPIO.output(PINS['TDI'], GPIO.LOW)
for i in range(5):
    GPIO.output(PINS['TCK'], GPIO.LOW)
    time.sleep(0.001)
    GPIO.output(PINS['TCK'], GPIO.HIGH)
    time.sleep(0.001)
    print(f"  Reset clock {i+1}/5")
print("  ✓ Reset sequence sent")
print()

# Test 2: Check TDO response
print("🔄 Test 2: TDO Response Check...")
tdo_samples = []
for i in range(20):
    # Clock pulse
    GPIO.output(PINS['TCK'], GPIO.LOW)
    time.sleep(0.001)
    GPIO.output(PINS['TCK'], GPIO.HIGH)
    time.sleep(0.001)
    
    # Sample TDO
    tdo = GPIO.input(PINS['TDO'])
    tdo_samples.append(tdo)
    
    if i < 10:
        print(f"  Sample {i+1:2d}: TDO = {tdo} ({'HIGH' if tdo else 'LOW'})")

# Analysis
tdo_high_count = sum(tdo_samples)
tdo_low_count = len(tdo_samples) - tdo_high_count

print()
print(f"📊 TDO Statistics (20 samples):")
print(f"  HIGH count: {tdo_high_count}")
print(f"  LOW count:  {tdo_low_count}")
print()

if tdo_high_count == 20:
    print("  ❌ TDO hep HIGH - CPLD yanıt vermiyor!")
    print("     Kontrol:")
    print("     1. Pin 12 (VCCIO1) 3.3V alıyor mu?")
    print("     2. CPLD'nin ana VCC'si besleniyor mu?")
    print("     3. JTAG kabloları doğru bağlı mı?")
    result = "FAIL"
elif tdo_low_count == 20:
    print("  ❌ TDO hep LOW - Bağlantı sorunu olabilir")
    print("     Pin 18'in bağlantısını kontrol edin")
    result = "FAIL"
elif 5 <= tdo_high_count <= 15:
    print("  ✅ TDO değişken - CPLD muhtemelen yanıt veriyor!")
    print("     OpenOCD ile tam scan yapılabilir")
    result = "SUCCESS"
else:
    print("  ⚠️  TDO kararsız - Belirsiz durum")
    result = "UNCERTAIN"

# Test 3: IDCODE read attempt (if TDO responsive)
if result == "SUCCESS":
    print()
    print("🔄 Test 3: IDCODE Read Attempt...")
    # Shift-IR: Load IDCODE instruction (all 1s)
    GPIO.output(PINS['TMS'], GPIO.HIGH)
    for _ in range(5): # Go to Shift-IR
        GPIO.output(PINS['TCK'], GPIO.LOW)
        time.sleep(0.0001)
        GPIO.output(PINS['TCK'], GPIO.HIGH)
        time.sleep(0.0001)
    
    # Shift 10 bits of 1s (IDCODE instruction for MAX V)
    GPIO.output(PINS['TMS'], GPIO.LOW)
    GPIO.output(PINS['TDI'], GPIO.HIGH)
    for i in range(10):
        GPIO.output(PINS['TCK'], GPIO.LOW)
        time.sleep(0.0001)
        GPIO.output(PINS['TCK'], GPIO.HIGH)
        time.sleep(0.0001)
    
    # Go to Shift-DR
    GPIO.output(PINS['TMS'], GPIO.HIGH)
    for _ in range(4):
        GPIO.output(PINS['TCK'], GPIO.LOW)
        time.sleep(0.0001)
        GPIO.output(PINS['TCK'], GPIO.HIGH)
        time.sleep(0.0001)
    
    # Read 32 bits of IDCODE
    GPIO.output(PINS['TMS'], GPIO.LOW)
    GPIO.output(PINS['TDI'], GPIO.LOW)
    idcode_bits = []
    for i in range(32):
        GPIO.output(PINS['TCK'], GPIO.LOW)
        time.sleep(0.0001)
        tdo = GPIO.input(PINS['TDO'])
        idcode_bits.append(tdo)
        GPIO.output(PINS['TCK'], GPIO.HIGH)
        time.sleep(0.0001)
    
    # Convert to hex
    idcode = 0
    for i, bit in enumerate(idcode_bits):
        if bit:
            idcode |= (1 << i)
    
    print(f"  IDCODE read: 0x{idcode:08X}")
    if idcode == 0x020a10dd:
        print("  ✅ CORRECT! Altera MAX V 5M80Z detected!")
    elif idcode == 0xFFFFFFFF:
        print("  ❌ All 1s - No device responding")
    elif idcode == 0x00000000:
        print("  ❌ All 0s - No device responding")
    else:
        print(f"  ⚠️  Unexpected IDCODE (expected 0x020a10dd)")

# Cleanup
GPIO.cleanup()

print()
print("=" * 70)
print("  TEST COMPLETE")
print("=" * 70)
print()

sys.exit(0 if result == "SUCCESS" else 1)
