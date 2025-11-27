#!/usr/bin/env python3
"""
JTAG Bağlantı Test Scripti
CPLD'nin JTAG üzerinden görünüp görünmediğini test eder
"""

import subprocess
import sys

def create_openocd_config():
    """Basit OpenOCD config oluştur"""
    config = """
adapter driver sysfsgpio
adapter speed 100
sysfsgpio tck_num 25
sysfsgpio tms_num 22
sysfsgpio tdi_num 23
sysfsgpio tdo_num 24
transport select jtag
jtag newtap chip tap -irlen 10 -expected-id 0x020a10dd
init
scan_chain
shutdown
"""
    with open('/tmp/jtag_test.cfg', 'w') as f:
        f.write(config)
    print("✓ OpenOCD config oluşturuldu")

def run_jtag_scan():
    """JTAG taraması yap"""
    print("\n============================================================")
    print("  JTAG CHAIN SCAN")
    print("============================================================\n")
    
    try:
        result = subprocess.run(
            ['sudo', 'openocd', '-f', '/tmp/jtag_test.cfg'],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        print(result.stdout)
        if result.stderr:
            print(result.stderr)
        
        if "TapName             Enabled  IdCode" in result.stdout:
            print("\n✓ JTAG chain tespit edildi!")
            return True
        else:
            print("\n✗ JTAG chain tespit edilemedi")
            return False
            
    except subprocess.TimeoutExpired:
        print("✗ Timeout - JTAG yanıt vermiyor")
        return False
    except Exception as e:
        print(f"✗ Hata: {e}")
        return False

if __name__ == "__main__":
    create_openocd_config()
    success = run_jtag_scan()
    sys.exit(0 if success else 1)
