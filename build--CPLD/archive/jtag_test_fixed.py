#!/usr/bin/env python3
"""
JTAG Test - Yeni GPIO numaralandırma ile (gpiochip512)
"""

import subprocess
import sys

# GPIO base offset (gpiochip512 için)
GPIO_BASE = 512

# Orijinal pin numaraları
ORIGINAL_PINS = {
    "tms": 22,
    "tdi": 23,
    "tdo": 24,
    "tck": 25
}

# Gerçek GPIO numaraları (512 offset ile)
REAL_PINS = {
    "tms": GPIO_BASE + ORIGINAL_PINS["tms"],  # 534
    "tdi": GPIO_BASE + ORIGINAL_PINS["tdi"],  # 535
    "tdo": GPIO_BASE + ORIGINAL_PINS["tdo"],  # 536
    "tck": GPIO_BASE + ORIGINAL_PINS["tck"],  # 537
}

def test_gpio(pin):
    """GPIO'nun kullanılabilir olup olmadığını test et"""
    try:
        with open('/sys/class/gpio/export', 'w') as f:
            f.write(str(pin))
        with open('/sys/class/gpio/unexport', 'w') as f:
            f.write(str(pin))
        return True
    except Exception as e:
        return False

def create_config():
    """OpenOCD config oluştur"""
    cfg = f"""
adapter driver sysfsgpio
adapter speed 100
sysfsgpio tck_num {REAL_PINS['tck']}
sysfsgpio tms_num {REAL_PINS['tms']}
sysfsgpio tdi_num {REAL_PINS['tdi']}
sysfsgpio tdo_num {REAL_PINS['tdo']}
transport select jtag
jtag newtap chip tap -irlen 10 -expected-id 0x020a10dd
init
scan_chain
shutdown
"""
    with open('/tmp/jtag_corrected.cfg', 'w') as f:
        f.write(cfg)
    print("✓ OpenOCD config oluşturuldu")

def main():
    print("\n" + "="*60)
    print("  CPLD JTAG TEST - GPIO Offset Düzeltmeli")
    print("="*60)
    print(f"\nGPIO Base Offset: {GPIO_BASE}")
    print(f"Pin Mapping:")
    print(f"  GPIO {ORIGINAL_PINS['tms']} (TMS) -> {REAL_PINS['tms']}")
    print(f"  GPIO {ORIGINAL_PINS['tdi']} (TDI) -> {REAL_PINS['tdi']}")
    print(f"  GPIO {ORIGINAL_PINS['tdo']} (TDO) -> {REAL_PINS['tdo']}")
    print(f"  GPIO {ORIGINAL_PINS['tck']} (TCK) -> {REAL_PINS['tck']}")
    
    print("\nGPIO Kullanılabilirlik Testi:")
    all_ok = True
    for name, pin in REAL_PINS.items():
        if test_gpio(pin):
            print(f"  GPIO {pin} ({name.upper()}): ✓ Kullanılabilir")
        else:
            print(f"  GPIO {pin} ({name.upper()}): ✗ KULLANILAMAZ")
            all_ok = False
    
    if not all_ok:
        print("\n✗ Bazı GPIO pinleri kullanılamıyor!")
        return False
    
    print("\n" + "="*60)
    print("  JTAG CHAIN SCAN")
    print("="*60 + "\n")
    
    create_config()
    
    try:
        result = subprocess.run(
            ['sudo', 'openocd', '-f', '/tmp/jtag_corrected.cfg'],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        print(result.stdout)
        if result.stderr:
            print(result.stderr)
        
        if "TapName" in result.stdout or ("chip.tap" in result.stdout and "enabled" in result.stdout.lower()):
            print("\n✓✓✓ JTAG CHAIN BAŞARILI! ✓✓✓")
            return True
        else:
            print("\n✗ JTAG yanıt yok")
            return False
            
    except subprocess.TimeoutExpired:
        print("✗ Timeout")
        return False
    except Exception as e:
        print(f"✗ Hata: {e}")
        return False

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
