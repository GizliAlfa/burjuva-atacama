#!/usr/bin/env python3
"""
JTAG Test - Alternatif GPIO pinleri ile
GPIO 25 yerine GPIO 26 deneyelim
"""

import subprocess
import sys

# Farklı pin kombinasyonları dene
pin_configs = [
    # Config 1: Orijinal (GPIO 25 problem oluyor)
    {"name": "Orijinal (22,23,24,25)", "tms": 22, "tdi": 23, "tdo": 24, "tck": 25},
    # Config 2: GPIO 26 kullan
    {"name": "Alternatif-1 (22,23,24,26)", "tms": 22, "tdi": 23, "tdo": 24, "tck": 26},
    # Config 3: Tamamen farklı pinler
    {"name": "Alternatif-2 (17,27,24,23)", "tms": 17, "tdi": 27, "tdo": 24, "tck": 23},
]

def test_gpio_availability(pins):
    """GPIO pinlerinin kullanılabilir olup olmadığını test et"""
    for pin in pins:
        try:
            # GPIO'yu export etmeyi dene
            with open('/sys/class/gpio/export', 'w') as f:
                f.write(str(pin))
            # Başarılı, geri al
            with open('/sys/class/gpio/unexport', 'w') as f:
                f.write(str(pin))
            print(f"  GPIO {pin}: ✓ Kullanılabilir")
        except Exception as e:
            print(f"  GPIO {pin}: ✗ KULLANILAMAZ - {e}")
            return False
    return True

def create_config(config):
    """OpenOCD config oluştur"""
    cfg = f"""
adapter driver sysfsgpio
adapter speed 100
sysfsgpio tck_num {config['tck']}
sysfsgpio tms_num {config['tms']}
sysfsgpio tdi_num {config['tdi']}
sysfsgpio tdo_num {config['tdo']}
transport select jtag
jtag newtap chip tap -irlen 10 -expected-id 0x020a10dd
init
scan_chain
shutdown
"""
    with open('/tmp/jtag_config.cfg', 'w') as f:
        f.write(cfg)

def run_jtag_test(config):
    """JTAG test çalıştır"""
    print(f"\n{'='*60}")
    print(f"  Test: {config['name']}")
    print(f"  TMS={config['tms']}, TDI={config['tdi']}, TDO={config['tdo']}, TCK={config['tck']}")
    print(f"{'='*60}")
    
    # GPIO kullanılabilirliğini test et
    pins = [config['tms'], config['tdi'], config['tdo'], config['tck']]
    if not test_gpio_availability(pins):
        print("  ⚠ Bu pinler kullanılamıyor, atlıyorum...\n")
        return False
    
    create_config(config)
    
    try:
        result = subprocess.run(
            ['sudo', 'openocd', '-f', '/tmp/jtag_config.cfg'],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        output = result.stdout + result.stderr
        
        if "Error" in output and "Couldn't export" in output:
            print("  ✗ GPIO export hatası\n")
            return False
        elif "TapName" in output or "scan_chain" in output:
            print("  ✓ JTAG chain başarılı!")
            print("\n" + output)
            return True
        else:
            print("  ✗ JTAG yanıt yok")
            print(output[:500])
            return False
            
    except subprocess.TimeoutExpired:
        print("  ✗ Timeout\n")
        return False
    except Exception as e:
        print(f"  ✗ Hata: {e}\n")
        return False

if __name__ == "__main__":
    print("\n" + "="*60)
    print("  CPLD JTAG PIN TEST")
    print("="*60)
    
    for config in pin_configs:
        if run_jtag_test(config):
            print("\n✓✓✓ BAŞARILI KONFİGÜRASYON BULUNDU! ✓✓✓")
            print(f"Kullanılacak pinler: TMS={config['tms']}, TDI={config['tdi']}, TDO={config['tdo']}, TCK={config['tck']}")
            sys.exit(0)
    
    print("\n✗✗✗ HİÇBİR KONFİGÜRASYON ÇALIŞMADI ✗✗✗")
    sys.exit(1)
