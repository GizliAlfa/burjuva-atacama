#!/usr/bin/env python3
"""
CPLD Güç/Enable Pin Testi
Pin 12 (GPIO 18) - Altera PIN25 VCCIO1
Pin 11 (GPIO 17) - STM32 NRST (transistör üzerinden)
"""

import RPi.GPIO as GPIO
import time

# Pin 11 -> GPIO 17 (STM32 NRST)
# Pin 12 -> GPIO 18 (VCCIO1)

PINS_TO_TEST = [17, 18]

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

print("=" * 70)
print("  CPLD GÜÇ/ENABLE PIN TESTİ")
print("=" * 70)
print()
print("Test edilecek pinler:")
print("  GPIO 17 (Pin 11) - STM32 NRST (transistör)")
print("  GPIO 18 (Pin 12) - Altera PIN25 VCCIO1")
print()

for pin in PINS_TO_TEST:
    print(f"🔄 GPIO {pin} test ediliyor...")
    
    # Pin'i output olarak ayarla
    GPIO.setup(pin, GPIO.OUT)
    
    # LOW yap
    GPIO.output(pin, GPIO.LOW)
    print(f"   GPIO {pin} = LOW")
    time.sleep(1)
    
    # HIGH yap
    GPIO.output(pin, GPIO.HIGH)
    print(f"   GPIO {pin} = HIGH")
    time.sleep(1)
    
    print()

print("✓ Pinler test edildi")
print()
print("Şimdi JTAG testini tekrar dene:")
print("  sudo python3 /tmp/jtag_final_test.py")

GPIO.cleanup()
