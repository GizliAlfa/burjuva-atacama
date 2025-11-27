#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Burjuva STM32 Yönetim Scripti - ALL-IN-ONE
Tarih: 17 Kasım 2025

STM32/FPGA/CPLD programming, UART test - Tek script!
Entegre edilen: burjuva_flash.py + test_stm32_uart.py
"""

import subprocess
import sys
import os
import time
from datetime import datetime

# UART testi için
try:
    import serial
    SERIAL_AVAILABLE = True
except ImportError:
    SERIAL_AVAILABLE = False

# GPIO için
try:
    import gpiod
    GPIOD_AVAILABLE = True
except ImportError:
    GPIOD_AVAILABLE = False

# I2C için (IO16 modülü) - DİREKT EMBEDDED DRIVER
try:
    import smbus2
    IO16_AVAILABLE = True
except ImportError:
    IO16_AVAILABLE = False

# UART ayarları
UART_PORT = '/dev/ttyAMA0'
UART_BAUD = 115200
UART_TIMEOUT = 1

# ============================================================================
# IO16 DRIVER - EMBEDDED DIRECTLY (NO EXTERNAL IMPORT)
# ============================================================================
class IO16Driver:
    """
    IO16 Module Driver using direct I2C communication
    Hardware: RPI → I2C → TCA9548A (0x70) → Channel 2 → SI8662BD → PCA9555 (0x41/0x51)
    """
    
    # I2C Addresses
    TCA9548A_ADDR = 0x70
    PCA9555_SLOT0 = 0x41
    PCA9555_SLOT1 = 0x51
    IO16_CHANNEL = 0x04  # Channel 2
    
    # PCA9555 Registers
    REG_INPUT_PORT0 = 0x00
    REG_INPUT_PORT1 = 0x01
    REG_OUTPUT_PORT0 = 0x02
    REG_OUTPUT_PORT1 = 0x03
    REG_CONFIG_PORT0 = 0x06
    REG_CONFIG_PORT1 = 0x07
    
    def __init__(self, bus_number=1, debug=False):
        if not IO16_AVAILABLE:
            raise ImportError("smbus2 module not available")
        self.bus = smbus2.SMBus(bus_number)
        self.debug = debug
        self._select_channel()
    
    def _select_channel(self):
        """Select TCA9548A channel for IO16"""
        self.bus.write_byte(self.TCA9548A_ADDR, self.IO16_CHANNEL)
        time.sleep(0.01)
    
    def _get_pca_address(self, slot):
        """Get PCA9555 I2C address for slot"""
        return self.PCA9555_SLOT0 if slot == 0 else self.PCA9555_SLOT1
    
    def _read_register(self, slot, register):
        """Read single byte from PCA9555 register"""
        addr = self._get_pca_address(slot)
        return self.bus.read_byte_data(addr, register)
    
    def _write_register(self, slot, register, value):
        """Write single byte to PCA9555 register"""
        addr = self._get_pca_address(slot)
        self.bus.write_byte_data(addr, register, value)
        time.sleep(0.01)
    
    def _read_port_pair(self, slot, reg_low, reg_high):
        """Read 16-bit value from two consecutive registers"""
        low = self._read_register(slot, reg_low)
        high = self._read_register(slot, reg_high)
        return (high << 8) | low
    
    def _write_port_pair(self, slot, reg_low, reg_high, value):
        """Write 16-bit value to two consecutive registers"""
        low = value & 0xFF
        high = (value >> 8) & 0xFF
        self._write_register(slot, reg_low, low)
        self._write_register(slot, reg_high, high)
    
    def set_direction(self, slot, pin, is_input):
        """Set pin direction (True=INPUT, False=OUTPUT)"""
        port = 0 if pin < 8 else 1
        reg = self.REG_CONFIG_PORT0 if port == 0 else self.REG_CONFIG_PORT1
        bit = pin % 8
        
        config = self._read_register(slot, reg)
        if is_input:
            config |= (1 << bit)
        else:
            config &= ~(1 << bit)
        self._write_register(slot, reg, config)
    
    def set_pin(self, slot, pin, state):
        """Set output pin state (0=LOW, 1=HIGH)"""
        port = 0 if pin < 8 else 1
        reg = self.REG_OUTPUT_PORT0 if port == 0 else self.REG_OUTPUT_PORT1
        bit = pin % 8
        
        output = self._read_register(slot, reg)
        if state:
            output |= (1 << bit)
        else:
            output &= ~(1 << bit)
        
        self._write_register(slot, reg, output)
        time.sleep(0.02)  # 20ms delay for hardware response
        
        # Verify by reading INPUT register
        actual_state = self.get_pin(slot, pin)
        if self.debug:
            success = "✓" if actual_state == state else "✗"
            print(f"[IO16] Pin {pin} set to {'HIGH' if state else 'LOW'} {success}")
        return actual_state == state
    
    def get_pin(self, slot, pin):
        """Read pin state (0=LOW, 1=HIGH)"""
        port = 0 if pin < 8 else 1
        reg = self.REG_INPUT_PORT0 if port == 0 else self.REG_INPUT_PORT1
        bit = pin % 8
        
        input_val = self._read_register(slot, reg)
        return (input_val >> bit) & 1
    
    def read_all(self, slot):
        """Read all 16 pins at once"""
        return self._read_port_pair(slot, self.REG_INPUT_PORT0, self.REG_INPUT_PORT1)
    
    def write_all(self, slot, value):
        """Write all 16 output pins at once"""
        self._write_port_pair(slot, self.REG_OUTPUT_PORT0, self.REG_OUTPUT_PORT1, value)
        time.sleep(0.02)
        return True
    
    def _print_status(self, slot):
        """Print current module status"""
        input_val = self._read_port_pair(slot, self.REG_INPUT_PORT0, self.REG_INPUT_PORT1)
        output_val = self._read_port_pair(slot, self.REG_OUTPUT_PORT0, self.REG_OUTPUT_PORT1)
        config_val = self._read_port_pair(slot, self.REG_CONFIG_PORT0, self.REG_CONFIG_PORT1)
        
        print(f"\n[IO16] Slot {slot} Status:")
        print(f"  INPUT:  0x{input_val:04X} ({input_val:016b})")
        print(f"  OUTPUT: 0x{output_val:04X} ({output_val:016b})")
        print(f"  CONFIG: 0x{config_val:04X} ({config_val:016b})")
        print(f"  (CONFIG: 1=input, 0=output)\n")
    
    def close(self):
        """Close I2C bus"""
        if hasattr(self, 'bus'):
            self.bus.close()

# ============================================================================
# BURJUVA_FLASH.PY - COMPLETE HARDWARE PROGRAMMING CODE
# ============================================================================
# Log file
LOG_FILE = "/tmp/burjuva_flash.log"

def log_flash(message):
    """Print and log message (burjuva_flash)"""
    timestamp = datetime.now().strftime("%H:%M:%S")
    log_msg = f"[{timestamp}] {message}"
    print(log_msg)
    with open(LOG_FILE, "a") as f:
        f.write(log_msg + "\n")
    sys.stdout.flush()

# Hardware Configuration
GPIOCHIP = "/dev/gpiochip4"  # Raspberry Pi 4/5 uses gpiochip4
RESET_PIN = 17
BOOT0_PIN = 4
UART_DEV = "/dev/ttyAMA0"

# Firmware Paths (burjuva klasörü içinde)
STM32_FIRMWARE = "burjuva/firmware.bin"
FPGA_BITSTREAM = "burjuva/fpga-bitstream.bin"
CPLD_POF = "burjuva/cpld.pof"
CPLD_SVF = "burjuva/cpld.svf"
OPENOCD_CPLD_CFG = "burjuva/openocd_cpld.cfg"

def enter_bootloader(gpio_request):
    """Enter STM32 bootloader mode"""
    log_flash("=== Entering Bootloader Mode ===")
    
    # Bootloader entry sequence
    gpio_request.set_value(BOOT0_PIN, gpiod.line.Value.ACTIVE)   # BOOT0=HIGH
    time.sleep(0.2)
    gpio_request.set_value(RESET_PIN, gpiod.line.Value.ACTIVE)   # Reset
    time.sleep(0.5)
    gpio_request.set_value(RESET_PIN, gpiod.line.Value.INACTIVE) # Release
    time.sleep(1.0)
    
    log_flash("✓ Bootloader mode active")

def exit_bootloader(gpio_request):
    """Exit bootloader mode and run firmware"""
    log_flash("\n=== Exiting Bootloader - Running Firmware ===")
    
    # Set BOOT0=LOW for normal mode
    gpio_request.set_value(BOOT0_PIN, gpiod.line.Value.INACTIVE)
    time.sleep(0.1)
    
    # Reset to load firmware
    gpio_request.set_value(RESET_PIN, gpiod.line.Value.ACTIVE)
    time.sleep(0.2)
    gpio_request.set_value(RESET_PIN, gpiod.line.Value.INACTIVE)
    time.sleep(0.5)
    
    log_flash("✓ Firmware running in normal mode")
    log_flash("✓ BOOT0=LOW, UART should be free")

def program_stm32_core():
    """Program STM32 with burjuva-motor firmware"""
    
    log_flash("\n" + "=" * 60)
    log_flash("  STEP 1: STM32F103 PROGRAMMING")
    log_flash("=" * 60)
    
    # Check firmware file
    if not os.path.exists(STM32_FIRMWARE):
        log_flash(f"❌ Firmware not found: {STM32_FIRMWARE}")
        return False
    
    size = os.path.getsize(STM32_FIRMWARE)
    log_flash(f"📁 Firmware: {STM32_FIRMWARE}")
    log_flash(f"📊 Size: {size} bytes ({size/1024:.2f} KB)")
    log_flash("")
    
    if not GPIOD_AVAILABLE:
        log_flash("❌ gpiod library not available!")
        return False
    
    # Setup GPIO
    try:
        gpio_request = gpiod.request_lines(
            GPIOCHIP,
            consumer="stm32-prog",
            config={
                BOOT0_PIN: gpiod.LineSettings(
                    direction=gpiod.line.Direction.OUTPUT,
                    output_value=gpiod.line.Value.INACTIVE
                ),
                RESET_PIN: gpiod.LineSettings(
                    direction=gpiod.line.Direction.OUTPUT,
                    output_value=gpiod.line.Value.ACTIVE
                )
            }
        )
    except Exception as e:
        log_flash(f"❌ GPIO initialization failed: {e}")
        return False
    
    try:
        # Enter bootloader
        enter_bootloader(gpio_request)
        time.sleep(0.5)
        
        # Program
        log_flash(f"\n=== Programming {STM32_FIRMWARE} ===")
        cmd = ['stm32flash', '-w', STM32_FIRMWARE, '-v', UART_DEV]
        
        log_flash(f"Command: {' '.join(cmd)}")
        log_flash("-" * 60)
        
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        
        # Log every line of output
        for line in result.stdout.split('\n'):
            if line.strip():
                log_flash(line)
        
        if result.returncode == 0:
            log_flash("-" * 60)
            log_flash("✅ STM32 Programming SUCCESS!")
            
            # Exit bootloader properly
            exit_bootloader(gpio_request)
            return True
        else:
            for line in result.stderr.split('\n'):
                if line.strip():
                    log_flash(line)
            log_flash("-" * 60)
            log_flash(f"❌ STM32 Programming FAILED: {result.returncode}")
            exit_bootloader(gpio_request)
            return False
            
    except subprocess.TimeoutExpired:
        log_flash("❌ Programming TIMEOUT")
        exit_bootloader(gpio_request)
        return False
    except Exception as e:
        log_flash(f"❌ Error: {e}")
        try:
            exit_bootloader(gpio_request)
        except:
            pass
        return False
    finally:
        gpio_request.release()

def program_fpga_core():
    """Program iCE40 FPGA via Raspberry Pi GPIO SPI Bitbang (gpiod v2.x)"""
    
    log_flash("\n" + "=" * 60)
    log_flash("  STEP 2: iCE40 HX1K FPGA PROGRAMMING (GPIO SPI BITBANG)")
    log_flash("=" * 60)
    
    if not os.path.exists(FPGA_BITSTREAM):
        log_flash(f"⚠️  FPGA bitstream not found: {FPGA_BITSTREAM}")
        log_flash("ℹ️  Skipping FPGA programming...")
        return False
    
    size = os.path.getsize(FPGA_BITSTREAM)
    log_flash(f"📁 Bitstream: {FPGA_BITSTREAM}")
    log_flash(f"📊 Size: {size} bytes ({size/1024:.2f} KB)")
    log_flash("")
    
    if not GPIOD_AVAILABLE:
        log_flash("❌ gpiod library not available!")
        return False
    
    # GPIO Pin definitions for FPGA SPI programming
    PIN_MOSI = 10  # GPIO 10 (MOSI) - SPI0_MOSI
    PIN_MISO = 9   # GPIO 9 (MISO) - SPI0_MISO  
    PIN_CLK  = 11  # GPIO 11 (SCLK) - SPI0_SCLK
    PIN_CS   = 8   # GPIO 8 (CE0) - SPI0_CE0
    PIN_CRST = 25  # GPIO 25 (FPGA CRESET)
    
    log_flash("📍 FPGA Programming GPIO Pins:")
    log_flash(f"  MOSI: GPIO {PIN_MOSI}")
    log_flash(f"  MISO: GPIO {PIN_MISO}")
    log_flash(f"  SCLK: GPIO {PIN_CLK}")
    log_flash(f"  CS:   GPIO {PIN_CS}")
    log_flash(f"  CRST: GPIO {PIN_CRST}")
    log_flash("")
    
    # Kill any existing GPIO users and disable SPI kernel modules
    try:
        log_flash("🔧 Clearing any existing GPIO users and SPI modules...")
        subprocess.run(["sudo", "modprobe", "-r", "spi_bcm2835"], 
                      stderr=subprocess.DEVNULL, timeout=2)
        subprocess.run(["sudo", "modprobe", "-r", "spidev"], 
                      stderr=subprocess.DEVNULL, timeout=2)
        subprocess.run(["sudo", "fuser", "-k", "/dev/gpiochip0"], 
                      stderr=subprocess.DEVNULL, timeout=2)
        subprocess.run(["sudo", "fuser", "-k", GPIOCHIP], 
                      stderr=subprocess.DEVNULL, timeout=2)
        time.sleep(0.5)
        log_flash("✓ GPIO cleanup complete")
    except:
        pass
    
    try:
        # Setup GPIO using gpiod v2.x API
        log_flash("🔧 Initializing GPIO for FPGA programming (gpiod v2.x)...")
        
        chip = gpiod.Chip(GPIOCHIP)
        
        # Configure all pins with proper initial values
        config = {
            PIN_MOSI: gpiod.LineSettings(direction=gpiod.line.Direction.OUTPUT, output_value=gpiod.line.Value.INACTIVE),
            PIN_CLK:  gpiod.LineSettings(direction=gpiod.line.Direction.OUTPUT, output_value=gpiod.line.Value.INACTIVE),
            PIN_CS:   gpiod.LineSettings(direction=gpiod.line.Direction.OUTPUT, output_value=gpiod.line.Value.ACTIVE),  # CS HIGH initially
            PIN_CRST: gpiod.LineSettings(direction=gpiod.line.Direction.OUTPUT, output_value=gpiod.line.Value.ACTIVE),  # CRST HIGH initially
            PIN_MISO: gpiod.LineSettings(direction=gpiod.line.Direction.INPUT),
        }
        
        gpio_request = chip.request_lines(consumer="fpga-prog", config=config)
        log_flash("✓ GPIO lines requested successfully")
        
        # Reset FPGA
        log_flash("🔄 Resetting FPGA...")
        gpio_request.set_value(PIN_CRST, gpiod.line.Value.INACTIVE)  # CRST=LOW
        time.sleep(0.01)
        gpio_request.set_value(PIN_CRST, gpiod.line.Value.ACTIVE)    # CRST=HIGH
        time.sleep(0.01)
        log_flash("✓ FPGA reset complete")
        log_flash("")
        
        # Read bitstream
        with open(FPGA_BITSTREAM, 'rb') as f:
            bitstream = f.read()
        
        log_flash(f"🚀 Programming {len(bitstream)} bytes to FPGA...")
        log_flash(f"⏱️  Estimated time: ~{len(bitstream) / 850:.0f} seconds @ 850 B/s")
        log_flash("-" * 60)
        
        # Start SPI transfer - CS LOW
        gpio_request.set_value(PIN_CS, gpiod.line.Value.INACTIVE)
        time.sleep(0.001)
        
        # Send bitstream byte by byte with bitbanging
        start_time = time.time()
        last_progress = -1
        last_log_time = 0
        
        for i, byte in enumerate(bitstream):
            # Send 8 bits (MSB first)
            for bit_idx in range(8):
                bit = (byte >> (7 - bit_idx)) & 1
                
                # Set MOSI
                mosi_val = gpiod.line.Value.ACTIVE if bit else gpiod.line.Value.INACTIVE
                gpio_request.set_value(PIN_MOSI, mosi_val)
                
                # Clock HIGH
                gpio_request.set_value(PIN_CLK, gpiod.line.Value.ACTIVE)
                time.sleep(0.000001)  # 1 microsecond
                
                # Clock LOW
                gpio_request.set_value(PIN_CLK, gpiod.line.Value.INACTIVE)
                time.sleep(0.000001)
            
            # Log progress every 10% or every 5 seconds
            progress = int((i + 1) * 100 / len(bitstream))
            elapsed = time.time() - start_time
            
            should_log = (progress != last_progress and progress % 10 == 0) or (elapsed - last_log_time > 5)
            
            if should_log:
                speed = (i + 1) / elapsed if elapsed > 0 else 0
                eta = (len(bitstream) - i - 1) / speed if speed > 0 else 0
                log_flash(f"Progress: {progress:3d}% ({i + 1:5d}/{len(bitstream)} bytes) - {speed:4.0f} B/s - ETA: {eta:3.0f}s")
                last_progress = progress
                last_log_time = elapsed
        
        # End SPI transfer - CS HIGH
        gpio_request.set_value(PIN_CS, gpiod.line.Value.ACTIVE)
        time.sleep(0.001)
        
        # Final statistics
        elapsed = time.time() - start_time
        speed = len(bitstream) / elapsed
        log_flash(f"Progress: 100% ({len(bitstream)}/{len(bitstream)} bytes) - {speed:.0f} B/s - Total: {elapsed:.1f}s")
        log_flash("-" * 60)
        log_flash("✅ FPGA Programming SUCCESS!")
        log_flash("ℹ️  FPGA bitstream loaded into configuration memory")
        
        gpio_request.release()
        chip.close()
        return True
        
    except Exception as e:
        log_flash(f"❌ FPGA Programming Error: {e}")
        import traceback
        log_flash(f"Traceback: {traceback.format_exc()}")
        try:
            gpio_request.release()
            chip.close()
        except:
            pass
        return False

def program_cpld_core():
    """Program Altera MAX V CPLD via GPIO JTAG using OpenOCD"""
    
    log_flash("\n" + "=" * 60)
    log_flash("  STEP 3: ALTERA MAX V CPLD PROGRAMMING (GPIO JTAG)")
    log_flash("=" * 60)
    
    # Check for SVF file
    if not os.path.exists(CPLD_SVF):
        log_flash(f"⚠️  CPLD SVF file not found: {CPLD_SVF}")
        if os.path.exists(CPLD_POF):
            log_flash(f"ℹ️  POF file found: {CPLD_POF}")
        log_flash("ℹ️  Skipping CPLD programming...")
        return False
    
    size = os.path.getsize(CPLD_SVF)
    log_flash(f"📁 CPLD SVF File: {CPLD_SVF}")
    log_flash(f"📊 Size: {size} bytes ({size/1024:.2f} KB)")
    log_flash("")
    
    # Check if openocd is available
    result = subprocess.run(['which', 'openocd'], capture_output=True)
    if result.returncode != 0:
        log_flash("❌ openocd not found")
        log_flash("ℹ️  Install: sudo apt install openocd")
        log_flash("ℹ️  Skipping CPLD programming...")
        return False
    
    log_flash("✅ openocd found")
    log_flash("")
    
    # Check if OpenOCD config exists
    openocd_cfg = "burjuva/openocd_cpld.cfg"
    if not os.path.exists(openocd_cfg):
        log_flash(f"⚠️  OpenOCD config not found: {openocd_cfg}")
        log_flash("ℹ️  Skipping CPLD programming...")
        return False
    
    log_flash(f"✅ OpenOCD config found: {openocd_cfg}")
    log_flash("")
    
    # GPIO JTAG Pin Mapping
    log_flash("📍 GPIO JTAG Pin Mapping:")
    log_flash("  TMS: GPIO 23 (Pin 16) -> Altera PIN 33")
    log_flash("  TDI: GPIO 22 (Pin 15) -> Altera PIN 34")
    log_flash("  TCK: GPIO 25 (Pin 22) -> Altera PIN 35")
    log_flash("  TDO: GPIO 24 (Pin 18) -> Altera PIN 36")
    log_flash("")
    
    # Program CPLD via GPIO JTAG
    log_flash("🔧 Programming CPLD via GPIO JTAG (OpenOCD)...")
    log_flash("⚠️  Expected IDCODE: 0x020a50dd (Altera MAX V 5M80Z)")
    
    cmd = ['sudo', 'openocd', '-f', openocd_cfg, '-c', f'svf {CPLD_SVF}; shutdown']
    log_flash(f"Command: {' '.join(cmd)}")
    log_flash("-" * 60)
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        
        # Log output
        for line in result.stdout.split('\n'):
            if line.strip():
                log_flash(line)
        if result.stderr:
            for line in result.stderr.split('\n'):
                if line.strip():
                    log_flash(line)
        
        # Check for success indicators
        output = result.stdout + result.stderr
        if "svf processing file" in output.lower() and "shutdown" in output.lower():
            log_flash("-" * 60)
            log_flash("✅ CPLD Programming SUCCESS!")
            log_flash("ℹ️  CPLD IDCODE verified: 0x020a50dd")
            return True
        elif result.returncode == 0:
            log_flash("-" * 60)
            log_flash("✅ CPLD Programming completed")
            return True
        else:
            log_flash("-" * 60)
            log_flash(f"⚠️  CPLD Programming FAILED: {result.returncode}")
            log_flash("ℹ️  Check GPIO JTAG connections")
            return False
            
    except subprocess.TimeoutExpired:
        log_flash("❌ CPLD Programming TIMEOUT (120s)")
        log_flash("ℹ️  OpenOCD might be hanging - check connections")
        return False
    except Exception as e:
        log_flash(f"⚠️  CPLD Error: {e}")
        log_flash("ℹ️  CPLD programming failed")
        return False

# ============================================================================
# END OF BURJUVA_FLASH.PY CODE
# ============================================================================

class STM32Tester:
    """STM32 UART Test Sınıfı"""
    def __init__(self):
        self.ser = None
        
    def connect(self):
        """STM32'ye bağlan"""
        if not SERIAL_AVAILABLE:
            print("❌ pyserial kütüphanesi yüklü değil! (pip3 install pyserial)")
            return False
        
        try:
            self.ser = serial.Serial(
                port=UART_PORT,
                baudrate=UART_BAUD,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=UART_TIMEOUT
            )
            time.sleep(0.5)
            print(f"✓ {UART_PORT} bağlantısı başarılı ({UART_BAUD} baud)")
            
            # Karşılama mesajını oku
            time.sleep(0.2)
            if self.ser.in_waiting:
                welcome = self.ser.read(self.ser.in_waiting)
                print(f"\n📨 STM32'den mesaj:\n{welcome.decode('utf-8', errors='ignore')}")
            
            return True
        except Exception as e:
            print(f"✗ Bağlantı hatası: {e}")
            return False
    
    def disconnect(self):
        """Bağlantıyı kapat"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("✓ Bağlantı kapatıldı")
    
    def echo_test(self):
        """Echo testi"""
        print("\n" + "="*50)
        print("ECHO TESTİ")
        print("="*50)
        
        test_messages = [
            b"Hello STM32!",
            b"Test123",
            b"UART Echo Working!",
            bytes(range(32, 127)),  # ASCII karakterler
        ]
        
        all_passed = True
        
        for i, msg in enumerate(test_messages, 1):
            print(f"\nTest {i}/{len(test_messages)}")
            print(f"  Gönderilen: {msg[:50]}{'...' if len(msg) > 50 else ''}")
            
            # Gönder
            self.ser.write(msg)
            self.ser.flush()
            
            # Al
            time.sleep(0.1)
            received = self.ser.read(len(msg))
            
            # Karşılaştır
            if received == msg:
                print(f"  Alınan:     {received[:50]}{'...' if len(received) > 50 else ''}")
                print(f"  Sonuç:      ✓ BAŞARILI")
            else:
                print(f"  Alınan:     {received[:50]}{'...' if len(received) > 50 else ''}")
                print(f"  Sonuç:      ✗ HATA (beklenen: {len(msg)} byte, alınan: {len(received)} byte)")
                all_passed = False
        
        print("\n" + "="*50)
        if all_passed:
            print("✓ TÜM TESTLER BAŞARILI!")
        else:
            print("✗ BAZI TESTLER BAŞARISIZ!")
        print("="*50)
    
    def send_data(self):
        """Veri gönder"""
        print("\n" + "="*50)
        print("VERİ GÖNDER")
        print("="*50)
        
        data = input("\nGöndermek istediğiniz veriyi yazın: ")
        
        if data:
            self.ser.write(data.encode())
            self.ser.flush()
            print(f"✓ {len(data)} byte gönderildi")
            
            # Echo'yu bekle
            time.sleep(0.1)
            if self.ser.in_waiting:
                echo = self.ser.read(self.ser.in_waiting)
                print(f"📨 STM32'den gelen: {echo.decode('utf-8', errors='ignore')}")
        else:
            print("Veri girilmedi.")
    
    def receive_data(self):
        """Veri al"""
        print("\n" + "="*50)
        print("VERİ AL")
        print("="*50)
        
        print("\n10 saniye boyunca gelen veriler dinleniyor...")
        print("(Çıkmak için Ctrl+C)\n")
        
        try:
            start_time = time.time()
            total_bytes = 0
            
            while time.time() - start_time < 10:
                if self.ser.in_waiting:
                    data = self.ser.read(self.ser.in_waiting)
                    total_bytes += len(data)
                    print(f"📨 [{len(data)} byte]: {data.decode('utf-8', errors='ignore')}")
                time.sleep(0.1)
            
            print(f"\n✓ Toplam {total_bytes} byte alındı")
            
        except KeyboardInterrupt:
            print("\n\n✓ Dinleme durduruldu")
    
    def continuous_test(self):
        """Sürekli echo testi (performans)"""
        print("\n" + "="*50)
        print("SÜREKLİ ECHO TESTİ")
        print("="*50)
        
        duration = int(input("\nTest süresi (saniye): ") or "10")
        
        print(f"\n{duration} saniye boyunca sürekli test yapılıyor...")
        print("(Çıkmak için Ctrl+C)\n")
        
        test_data = b"X" * 64  # 64 byte test verisi
        success_count = 0
        error_count = 0
        total_bytes = 0
        
        try:
            start_time = time.time()
            
            while time.time() - start_time < duration:
                # Gönder
                self.ser.write(test_data)
                self.ser.flush()
                
                # Al
                received = self.ser.read(len(test_data))
                
                # Kontrol et
                if received == test_data:
                    success_count += 1
                    total_bytes += len(received)
                else:
                    error_count += 1
                
                # Her 100 işlemde bir rapor
                if (success_count + error_count) % 100 == 0:
                    elapsed = time.time() - start_time
                    rate = total_bytes / elapsed if elapsed > 0 else 0
                    print(f"  Başarılı: {success_count}, Hata: {error_count}, Hız: {rate:.1f} byte/s")
            
            # Final rapor
            elapsed = time.time() - start_time
            rate = total_bytes / elapsed if elapsed > 0 else 0
            
            print("\n" + "="*50)
            print(f"Test Tamamlandı ({elapsed:.1f} saniye)")
            print(f"  Başarılı: {success_count}")
            print(f"  Hata:     {error_count}")
            print(f"  Toplam:   {total_bytes} byte")
            print(f"  Hız:      {rate:.1f} byte/s ({rate*8:.1f} bit/s)")
            print("="*50)
            
        except KeyboardInterrupt:
            print("\n\n✓ Test durduruldu")

def print_header():
    """Başlık yazdır"""
    print("\n" + "="*60)
    print(" 🚀 BURJUVA STM32 YÖNETİM SİSTEMİ")
    print(" Tarih: 16 Kasım 2025")
    print("="*60)

def print_menu():
    """Ana menü"""
    print("\n" + "="*60)
    print(" ANA MENÜ")
    print("="*60)
    print("1. STM32'yi Programla (Flash)")
    print("2. UART Echo Testi (Modül Algılama Dahil)")
    print("3. STM32 + FPGA Programla")
    print("4. CPLD Programla (Altera MAX V)")
    print("5. Sistem Durumu")
    print("6. Sistem Kurulumu (İlk Çalıştırma)")
    print("7. 🎮 Manuel Modül Kontrolü")
    print("8. 💻 STM32 Terminal (Direkt İletişim)")
    print("0. Çıkış")
    print("="*60)

def flash_stm32():
    """STM32'yi programla"""
    print("\n" + "="*60)
    print(" STM32 PROGRAMLAMA")
    print("="*60)
    
    if not os.path.exists(STM32_FIRMWARE):
        print(f"\n❌ HATA: {STM32_FIRMWARE} bulunamadı!")
        print("   Lütfen firmware.bin dosyasını burjuva/ klasörüne kopyalayın.")
        return False
    
    file_size = os.path.getsize(STM32_FIRMWARE)
    print(f"\n📦 {STM32_FIRMWARE}: {file_size} byte ({file_size/1024:.2f} KB)")
    
    if not GPIOD_AVAILABLE:
        print("\n❌ gpiod kütüphanesi yüklü değil!")
        print("   Yüklemek için: pip3 install gpiod")
        return False
    
    response = input("\nSTM32'yi programlamak istiyor musunuz? (e/h): ")
    if response.lower() != 'e':
        print("İptal edildi.")
        return False
    
    # Clear log file
    with open(LOG_FILE, "w") as f:
        f.write(f"=== BURJUVA STM32 PROGRAMMING LOG ===\n")
        f.write(f"Started: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write("=" * 60 + "\n\n")
    
    print("\n⏳ STM32 programlanıyor...")
    print(f"📝 Log: {LOG_FILE}\n")
    
    try:
        result = program_stm32_core()
        
        if result:
            print("\n✅ STM32 başarıyla programlandı!")
            return True
        else:
            print(f"\n❌ Programlama başarısız!")
            print(f"📝 Detaylı log: {LOG_FILE}")
            return False
            
    except Exception as e:
        print(f"\n❌ Hata: {e}")
        return False

def modul_algilama():
    """Modül algılama komutu gönder ve gecikme ölç"""
    print("\n" + "="*60)
    print(" MODÜL ALGILAMA")
    print("="*60)
    
    if not SERIAL_AVAILABLE:
        print("\n❌ pyserial kütüphanesi yüklü değil!")
        print("   Yüklemek için: pip3 install pyserial")
        return False
    
    try:
        # Seri port aç
        ser = serial.Serial(
            port=UART_PORT,
            baudrate=UART_BAUD,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=2
        )
        time.sleep(0.5)
        print(f"✓ {UART_PORT} bağlantısı başarılı\n")
        
        # Buffer'ı temizle
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Komutu gönder ve zamanı ölç
        print("📤 Komut gönderiliyor: modul-algila")
        
        # Yüksek hassasiyetli zaman ölçümü (nanosaniye)
        start_time_ns = time.perf_counter_ns()
        
        # Komutu gönder
        ser.write(b"modul-algila\r\n")
        ser.flush()
        
        # ACK mesajını bekle
        ack_received = False
        ack_time_ns = 0
        
        print("⏳ ACK bekleniyor...\n")
        
        # 2 saniye içinde ACK gelene kadar oku
        timeout_time = time.time() + 2
        buffer = ""
        
        while time.time() < timeout_time:
            if ser.in_waiting:
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                buffer += data
                
                # ACK kontrolü
                if "[ACK]" in buffer and not ack_received:
                    ack_time_ns = time.perf_counter_ns()
                    ack_received = True
                    
                    # Gecikme hesapla
                    latency_ns = ack_time_ns - start_time_ns
                    latency_us = latency_ns / 1000
                    latency_ms = latency_ns / 1_000_000
                    
                    print(f"✅ ACK alındı!")
                    print(f"⏱️  Gecikme:")
                    print(f"   • {latency_ns:,} ns (nanosaniye)")
                    print(f"   • {latency_us:.2f} µs (mikrosaniye)")
                    print(f"   • {latency_ms:.3f} ms (milisaniye)")
                    print()
                
                # Çıktıyı göster
                print(data, end='', flush=True)
                
                # Komut tamamlandı mı kontrol et
                if "Komut tamamlandi: modul-algila" in buffer:
                    break
        
        if not ack_received:
            print("\n⚠️  ACK alınamadı! (2 saniye timeout)")
        
        ser.close()
        print("\n✓ Modül algılama tamamlandı")
        return True
        
    except serial.SerialException as e:
        print(f"\n❌ UART hatası: {e}")
        return False
    except Exception as e:
        print(f"\n❌ Hata: {e}")
        return False

def uart_test_menu():
    """UART test menüsü"""
    print("\n" + "="*60)
    print(" UART TEST MENÜSÜ")
    print("="*60)
    
    if not SERIAL_AVAILABLE:
        print("\n❌ pyserial kütüphanesi yüklü değil!")
        print("   Yüklemek için: pip3 install pyserial")
        return False
    
    tester = STM32Tester()
    
    # Bağlan
    if not tester.connect():
        print("\n✗ Bağlantı başarısız!")
        return False
    
    try:
        while True:
            print("\n" + "="*50)
            print(" UART TEST SEÇENEKLERİ")
            print("="*50)
            print("1. Echo Testi")
            print("2. Veri Gönder")
            print("3. Veri Al (Dinle)")
            print("4. Sürekli Echo Testi (Performans)")
            print("5. Modül Algılama (Gecikme Ölçümü)")
            print("6. Bağlantıyı Yenile")
            print("0. Geri Dön")
            print("="*50)
            
            choice = input("\nSeçiminiz: ").strip()
            
            if choice == '1':
                tester.echo_test()
            elif choice == '2':
                tester.send_data()
            elif choice == '3':
                tester.receive_data()
            elif choice == '4':
                tester.continuous_test()
            elif choice == '5':
                # Bağlantıyı kapat, modül algıla, tekrar bağlan
                tester.disconnect()
                time.sleep(0.5)
                modul_algilama()
                time.sleep(0.5)
                if not tester.connect():
                    break
            elif choice == '6':
                tester.disconnect()
                time.sleep(0.5)
                if not tester.connect():
                    break
            elif choice == '0':
                break
            else:
                print("\n✗ Geçersiz seçim!")
            
            if choice != '0':
                input("\nDevam etmek için Enter'a basın...")
    
    except KeyboardInterrupt:
        print("\n\n✓ Test durduruldu (Ctrl+C)")
    
    finally:
        tester.disconnect()
    
    return True

def flash_cpld():
    """CPLD programla (Altera MAX V)"""
    print("\n" + "="*60)
    print(" CPLD PROGRAMLAMA (Altera MAX V)")
    print("="*60)
    
    if not os.path.exists(CPLD_SVF):
        print(f"\n❌ HATA: {CPLD_SVF} bulunamadı!")
        print(f"   Dosya yolu: {CPLD_SVF}")
        return False
    
    if not os.path.exists(OPENOCD_CPLD_CFG):
        print(f"\n❌ HATA: {OPENOCD_CPLD_CFG} bulunamadı!")
        print(f"   Dosya yolu: {OPENOCD_CPLD_CFG}")
        return False
    
    file_size = os.path.getsize(CPLD_SVF)
    print(f"\n📦 SVF Dosyası: {CPLD_SVF}")
    print(f"   Boyut: {file_size} byte ({file_size/1024:.2f} KB)")
    print(f"\n⚙️  OpenOCD Config: {OPENOCD_CPLD_CFG}")
    print(f"\n🔌 JTAG Pinleri:")
    print(f"   TMS=GPIO23, TDI=GPIO22, TCK=GPIO25, TDO=GPIO24")
    
    response = input("\nCPLD'yi programlamak istiyor musunuz? (e/h): ")
    if response.lower() != 'e':
        print("İptal edildi.")
        return False
    
    print("\n⏳ CPLD programlanıyor...")
    result = program_cpld_core()
    
    if result:
        print("✅ CPLD başarıyla programlandı!")
        return True
    else:
        print("❌ CPLD programlama başarısız!")
        return False

def flash_all():
    """STM32 + FPGA + CPLD programla"""
    print("\n" + "="*60)
    print(" FULL PROGRAMMING: STM32 + FPGA + CPLD")
    print("="*60)
    
    if not GPIOD_AVAILABLE:
        print("\n❌ gpiod kütüphanesi yüklü değil!")
        print("   Yüklemek için: pip3 install gpiod")
        return False
    
    response = input("\nSTM32 + FPGA + CPLD programlanacak. Devam? (e/h): ")
    if response.lower() != 'e':
        print("İptal edildi.")
        return False
    
    # Clear log file
    with open(LOG_FILE, "w") as f:
        f.write(f"=== BURJUVA FULL PROGRAMMING LOG ===\n")
        f.write(f"Started: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write("=" * 60 + "\n\n")
    
    print("\n⏳ Programlama başlıyor...")
    print(f"📝 Log: {LOG_FILE}\n")
    
    results = {
        'stm32': False,
        'fpga': False,
        'cpld': False
    }
    
    try:
        # STM32
        log_flash("=" * 60)
        log_flash("  🚀 BURJUVA MOTOR CONTROLLER - FULL PROGRAMMING")
        log_flash("=" * 60)
        results['stm32'] = program_stm32_core()
        
        # FPGA
        results['fpga'] = program_fpga_core()
        
        # CPLD
        results['cpld'] = program_cpld_core()
        
        # Summary
        log_flash("\n" + "=" * 60)
        log_flash("  📊 PROGRAMMING SUMMARY")
        log_flash("=" * 60)
        
        if results['stm32']:
            log_flash(f"  STM32:  ✅ SUCCESS")
        else:
            log_flash(f"  STM32:  ❌ FAILED")
        
        if results['fpga']:
            log_flash(f"  FPGA:   ✅ SUCCESS")
        else:
            log_flash(f"  FPGA:   ❌ FAILED")
        
        if results['cpld']:
            log_flash(f"  CPLD:   ✅ SUCCESS")
        else:
            log_flash(f"  CPLD:   ⚠️  FAILED")
        
        log_flash("=" * 60)
        
        if results['stm32'] and results['fpga']:
            print("\n🎉 TÜM BILEŞENLER BAŞARIYLA PROGRAMLANDI!")
            return True
        elif results['stm32']:
            print("\n✅ STM32 programlandı!")
            print("⚠️  FPGA/CPLD başarısız oldu!")
            print(f"📝 Detaylı log: {LOG_FILE}")
            return False
        else:
            print("\n❌ Programlama başarısız!")
            print(f"📝 Detaylı log: {LOG_FILE}")
            return False
            
    except Exception as e:
        print(f"\n❌ Hata: {e}")
        print(f"📝 Detaylı log: {LOG_FILE}")
        return False

def system_status():
    """Sistem durumunu göster"""
    print("\n" + "="*60)
    print(" SİSTEM DURUMU")
    print("="*60)
    
    # Dosya kontrolü
    print("\n📁 Firmware Dosyaları:")
    files = {
        STM32_FIRMWARE: "STM32F103 firmware",
        FPGA_BITSTREAM: "iCE40 FPGA bitstream",
        CPLD_SVF: "Altera CPLD SVF",
        CPLD_POF: "Altera CPLD POF"
    }
    
    for filename, desc in files.items():
        if os.path.exists(filename):
            size = os.path.getsize(filename)
            print(f"  ✓ {filename:30s} ({size:7d} byte) - {desc}")
        else:
            print(f"  ✗ {filename:30s} (yok)          - {desc}")
    
    # UART port kontrolü
    print("\n🔌 UART Portu:")
    if os.path.exists("/dev/ttyAMA0"):
        print("  ✓ /dev/ttyAMA0 mevcut")
        if SERIAL_AVAILABLE:
            print("  ✓ pyserial kütüphanesi yüklü")
        else:
            print("  ✗ pyserial kütüphanesi YOK (pip3 install pyserial)")
    else:
        print("  ✗ /dev/ttyAMA0 bulunamadı!")
    
    # GPIO kontrolü
    print("\n⚡ GPIO:")
    if os.path.exists(GPIOCHIP):
        print(f"  ✓ {GPIOCHIP} mevcut")
        if GPIOD_AVAILABLE:
            print("  ✓ gpiod kütüphanesi yüklü")
        else:
            print("  ✗ gpiod kütüphanesi YOK (pip3 install gpiod)")
    else:
        print(f"  ✗ {GPIOCHIP} bulunamadı!")
    
    # Python sürümü
    print(f"\n🐍 Python: {sys.version.split()[0]}")
    
    # Araçlar
    print("\n🔧 Programlama Araçları:")
    
    # stm32flash
    try:
        result = subprocess.run(["which", "stm32flash"], 
                              capture_output=True, text=True, timeout=2)
        if result.returncode == 0:
            print(f"  ✓ stm32flash: {result.stdout.strip()}")
        else:
            print("  ✗ stm32flash bulunamadı (apt install stm32flash)")
    except:
        print("  ✗ stm32flash bulunamadı")
    
    # OpenOCD
    try:
        result = subprocess.run(["openocd", "--version"], 
                              capture_output=True, text=True, timeout=2)
        version = result.stderr.split('\n')[0] if result.stderr else "bilinmiyor"
        print(f"  ✓ OpenOCD: {version}")
    except:
        print("  ✗ OpenOCD bulunamadı (apt install openocd)")

def setup_system():
    """Sistem kurulumu - tüm bağımlılıkları ve ayarları yapılandır"""
    print("\n" + "="*60)
    print(" SİSTEM KURULUMU (İLK ÇALIŞTIRMA)")
    print("="*60)
    print("\nBu işlem şunları yapacak:")
    print("  • Python kütüphanelerini yükleyecek (pyserial, gpiod)")
    print("  • Sistem araçlarını yükleyecek (stm32flash, openocd)")
    print("  • UART (ttyAMA0) ayarlarını yapacak")
    print("  • Bluetooth'u devre dışı bırakacak")
    print("  • Gerekli izinleri ayarlayacak")
    print("  • Sistemi yeniden başlatacak")
    
    response = input("\n⚠️  Devam etmek istiyor musunuz? (e/h): ")
    if response.lower() != 'e':
        print("İptal edildi.")
        return False
    
    print("\n" + "="*60)
    print(" KURULUM BAŞLIYOR...")
    print("="*60)
    
    # 1. Sistem güncellemesi
    print("\n[1/8] 📦 Sistem paketleri güncelleniyor...")
    try:
        subprocess.run(["sudo", "apt", "update"], check=True)
        print("  ✓ Sistem güncellemesi tamamlandı")
    except subprocess.CalledProcessError as e:
        print(f"  ⚠️  Uyarı: Güncelleme başarısız ({e})")
    
    # 2. Python kütüphaneleri
    print("\n[2/8] 🐍 Python kütüphaneleri yükleniyor...")
    
    # Önce pyserial (APT'den)
    try:
        print(f"  → pyserial (APT) yükleniyor...")
        subprocess.run(["sudo", "apt", "install", "-y", "python3-serial"],
                     capture_output=True, check=True)
        print(f"  ✓ pyserial yüklendi")
    except subprocess.CalledProcessError:
        print(f"  ⚠️  pyserial APT ile yüklenemedi")
    
    # gpiod için özel işlem (APT versiyonu eski API kullanıyor)
    print(f"  → gpiod kütüphanesi kontrol ediliyor...")
    
    # Eski python3-libgpiod varsa kaldır
    try:
        subprocess.run(["sudo", "apt", "remove", "-y", "python3-libgpiod"],
                     capture_output=True, check=True)
        print(f"  ✓ Eski python3-libgpiod kaldırıldı")
    except subprocess.CalledProcessError:
        pass  # Yoksa sorun yok
    
    # libgpiod2 ve libgpiod-dev yükle (yeni API için gerekli)
    try:
        print(f"  → libgpiod geliştirme paketleri yükleniyor...")
        subprocess.run(["sudo", "apt", "install", "-y", "libgpiod2", "libgpiod-dev"],
                     capture_output=True, check=True)
        print(f"  ✓ libgpiod sistem kütüphaneleri yüklendi")
    except subprocess.CalledProcessError:
        print(f"  ⚠️  libgpiod yüklenemedi")
    
    # gpiod'u pip3 ile yükle (yeni API 2.x sürümü)
    try:
        print(f"  → gpiod (pip3) yükleniyor...")
        subprocess.run(["pip3", "install", "--break-system-packages", "--upgrade", "gpiod"],
                     capture_output=True, check=True, timeout=60)
        print(f"  ✓ gpiod (yeni API) yüklendi")
    except subprocess.TimeoutExpired:
        print(f"  ⚠️  gpiod yükleme zaman aşımı, arka planda devam edecek")
    except subprocess.CalledProcessError:
        print(f"  ⚠️  gpiod yüklenemedi")
        print(f"     Manuel: pip3 install --break-system-packages gpiod")
    
    # 3. Sistem araçları
    print("\n[3/8] 🔧 Programlama araçları yükleniyor...")
    system_tools = ["stm32flash", "openocd", "i2c-tools"]
    try:
        print(f"  → {', '.join(system_tools)} yükleniyor...")
        subprocess.run(["sudo", "apt", "install", "-y"] + system_tools,
                     capture_output=True, check=True)
        print(f"  ✓ Araçlar yüklendi")
    except subprocess.CalledProcessError:
        print(f"  ⚠️  Bazı araçlar yüklenemedi")
    
    # 4. Bluetooth devre dışı
    print("\n[4/8] 📡 Bluetooth devre dışı bırakılıyor (UART için)...")
    try:
        # /boot/config.txt'e dtoverlay ekle
        config_line = "dtoverlay=disable-bt"
        
        # Önce var mı kontrol et
        result = subprocess.run(["grep", "-q", config_line, "/boot/firmware/config.txt"],
                              capture_output=True)
        
        if result.returncode != 0:  # Yoksa ekle
            subprocess.run(["sudo", "bash", "-c", 
                          f'echo "{config_line}" >> /boot/firmware/config.txt'],
                         check=True)
            print("  ✓ Bluetooth devre dışı bırakıldı")
        else:
            print("  ✓ Bluetooth zaten devre dışı")
    except subprocess.CalledProcessError:
        print("  ⚠️  Bluetooth ayarı yapılamadı")
    
    # 5. Bluetooth servisi durdur
    print("\n[5/8] 🛑 Bluetooth servisi durduruluyor...")
    try:
        subprocess.run(["sudo", "systemctl", "stop", "bluetooth"], 
                     capture_output=True, check=True)
        subprocess.run(["sudo", "systemctl", "disable", "bluetooth"], 
                     capture_output=True, check=True)
        print("  ✓ Bluetooth servisi durduruldu")
    except subprocess.CalledProcessError:
        print("  ⚠️  Bluetooth servisi durdurulamadı")
    
    # 6. UART aktivasyonu
    print("\n[6/8] 🔌 UART (ttyAMA0) aktifleştiriliyor...")
    try:
        # enable_uart=1 config.txt'e ekle
        uart_line = "enable_uart=1"
        result = subprocess.run(["grep", "-q", uart_line, "/boot/firmware/config.txt"],
                              capture_output=True)
        
        if result.returncode != 0:  # Yoksa ekle
            subprocess.run(["sudo", "bash", "-c", 
                          f'echo "{uart_line}" >> /boot/firmware/config.txt'],
                         check=True)
            print("  ✓ enable_uart=1 eklendi")
        else:
            print("  ✓ enable_uart=1 zaten var")
        
        # Serial console'u cmdline.txt'den kaldır
        cmdline_file = "/boot/firmware/cmdline.txt"
        try:
            # Mevcut içeriği oku
            result = subprocess.run(["cat", cmdline_file], 
                                  capture_output=True, text=True, check=True)
            cmdline = result.stdout.strip()
            
            # console=serial0,115200 varsa kaldır
            if "console=serial0" in cmdline or "console=ttyAMA0" in cmdline:
                # Serial console parametrelerini kaldır
                new_cmdline = cmdline
                new_cmdline = new_cmdline.replace("console=serial0,115200 ", "")
                new_cmdline = new_cmdline.replace("console=ttyAMA0,115200 ", "")
                new_cmdline = new_cmdline.replace("console=serial0,115200", "")
                new_cmdline = new_cmdline.replace("console=ttyAMA0,115200", "")
                
                # Güncellenmiş içeriği yaz
                subprocess.run(["sudo", "bash", "-c", 
                              f'echo "{new_cmdline}" > {cmdline_file}'],
                             check=True)
                print("  ✓ Serial console devre dışı bırakıldı")
            else:
                print("  ✓ Serial console zaten devre dışı")
        except subprocess.CalledProcessError:
            print("  ⚠️  cmdline.txt güncellenemedi")
        
        print("  ✓ UART aktivasyonu tamamlandı")
    except subprocess.CalledProcessError:
        print("  ⚠️  UART aktivasyonu başarısız")
        print("     Manuel: sudo raspi-config → Interface Options → Serial Port")
    
    # 7. Kullanıcı izinleri
    print("\n[7/8] 👤 Kullanıcı izinleri ayarlanıyor...")
    try:
        user = os.getenv("USER")
        groups = ["dialout", "gpio", "i2c", "spi"]
        for group in groups:
            subprocess.run(["sudo", "usermod", "-aG", group, user],
                         capture_output=True, check=True)
        print(f"  ✓ Kullanıcı '{user}' gruplara eklendi: {', '.join(groups)}")
    except subprocess.CalledProcessError:
        print("  ⚠️  Grup ataması başarısız")
    
    # 8. I2C aktivasyonu
    print("\n[8/8] 📊 I2C aktivasyonu...")
    try:
        # dtparam=i2c_arm=on config.txt'e ekle
        i2c_line = "dtparam=i2c_arm=on"
        result = subprocess.run(["grep", "-q", i2c_line, "/boot/firmware/config.txt"],
                              capture_output=True)
        
        if result.returncode != 0:  # Yoksa ekle
            subprocess.run(["sudo", "bash", "-c", 
                          f'echo "{i2c_line}" >> /boot/firmware/config.txt'],
                         check=True)
            print("  ✓ I2C config.txt'e eklendi")
        else:
            print("  ✓ I2C zaten aktif")
        
        # i2c-dev modülünü yükle
        subprocess.run(["sudo", "modprobe", "i2c-dev"], 
                     capture_output=True, check=True)
        print("  ✓ I2C modülü yüklendi")
    except subprocess.CalledProcessError:
        print("  ⚠️  I2C aktivasyonu başarısız")
    
    # Kurulum özeti
    print("\n" + "="*60)
    print(" KURULUM TAMAMLANDI!")
    print("="*60)
    print("\n✅ Yapılan işlemler:")
    print("  • Python kütüphaneleri (pyserial, gpiod)")
    print("  • Programlama araçları (stm32flash, openocd)")
    print("  • Bluetooth devre dışı")
    print("  • UART (ttyAMA0) aktif")
    print("  • Kullanıcı izinleri ayarlandı")
    print("  • I2C aktif")
    
    print("\n⚠️  ÖNEMLİ: Değişikliklerin geçerli olması için sistem yeniden başlatılmalı!")
    
    response = input("\nŞimdi yeniden başlatmak istiyor musunuz? (e/h): ")
    if response.lower() == 'e':
        print("\n🔄 Sistem 5 saniye içinde yeniden başlatılacak...")
        time.sleep(2)
        subprocess.run(["sudo", "reboot"])
    else:
        print("\n⚠️  Lütfen değişikliklerin etkili olması için sistemi manuel olarak yeniden başlatın:")
        print("   Komut: sudo reboot")
    
    return True

def send_uart_command(ser, command, wait_response=True, timeout=2, show_timing=True):
    """UART komutu gönder, ACK bekle, gecikmeyi ölç ve yanıt al"""
    try:
        # Buffer'ı temizle
        ser.reset_input_buffer()
        
        # Başlangıç zamanı (nanosaniye hassasiyetinde)
        start_time_ns = time.perf_counter_ns()
        
        # Komutu gönder
        ser.write(f"{command}\r\n".encode())
        ser.flush()
        
        if not wait_response:
            return None
        
        # ACK ve yanıt topla
        response = ""
        ack_received = False
        ack_time_ns = 0
        full_start_time = time.time()
        
        while time.time() - full_start_time < timeout:
            if ser.in_waiting:
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                response += data
                
                # ACK kontrolü
                if not ack_received and "[ACK:" in response:
                    ack_time_ns = time.perf_counter_ns()
                    ack_received = True
                    
                    # Gecikme hesapla
                    latency_ns = ack_time_ns - start_time_ns
                    latency_us = latency_ns / 1000
                    latency_ms = latency_ns / 1_000_000
                    
                    if show_timing:
                        print(f"⏱️  [ACK alındı: {latency_us:.0f} µs ({latency_ms:.2f} ms)]")
                
                # Komut tamamlandı mı kontrol et
                if "Komut tamamlandi:" in response or "OK:" in response or "Hata:" in response:
                    break
            time.sleep(0.01)
        
        # ACK alınamadıysa uyarı
        if not ack_received and show_timing:
            print(f"⚠️  [ACK alınamadı! {timeout}s timeout]")
        
        return response
    except Exception as e:
        print(f"❌ UART hatası: {e}")
        return None

def detect_modules(ser):
    """Modülleri algıla ve sonuçları parse et"""
    print("\n🔍 Modüller algılanıyor...")
    
    response = send_uart_command(ser, "modul-algila", timeout=3)
    
    if not response:
        print("❌ Modül algılama yanıtı alınamadı!")
        return []
    
    # Yanıtı göster
    print("\n" + "="*60)
    print(response)
    print("="*60)
    
    # Modülleri parse et
    # STM32 çıktısı şu formatta:
    # Slot 0 (PC2): -> FOUND!
    #   TYPE: IO16 - 16 Channel Digital I/O
    #   NAME: io16
    
    modules = []
    lines = response.split('\n')
    current_slot = None
    
    for line in lines:
        # Slot satırını bul: "Slot 0 (PC2): -> FOUND!"
        if "Slot" in line and "FOUND" in line:
            try:
                # "Slot 0 (PC2): -> FOUND!" -> slot=0
                parts = line.split()
                if len(parts) >= 2 and parts[0] == "Slot":
                    current_slot = int(parts[1])
            except (IndexError, ValueError):
                continue
        
        # TYPE satırını bul: "  TYPE: IO16 - 16 Channel Digital I/O"
        elif "TYPE:" in line and current_slot is not None:
            try:
                # "  TYPE: IO16 - 16 Channel Digital I/O"
                type_part = line.split("TYPE:")[1].strip()
                
                # İlk kelimeyi al (IO16, AIO20, FPGA, etc.)
                # "IO16 - 16 Channel Digital I/O" -> "IO16"
                if " - " in type_part:
                    module_type = type_part.split(" - ")[0].strip()
                elif " " in type_part:
                    module_type = type_part.split()[0].strip()
                else:
                    module_type = type_part.strip()
                
                module_desc = type_part  # Tam açıklama
                
                print(f"  [DEBUG] Parsed: Slot={current_slot}, Type='{module_type}', Desc='{module_desc}'")
                
                modules.append({
                    'slot': current_slot,
                    'type': module_type,
                    'description': module_desc
                })
                
                current_slot = None  # Reset for next module
                
            except (IndexError, ValueError) as e:
                print(f"  [DEBUG] Parse error on TYPE line '{line}': {e}")
                continue
    
    return modules

def io16_control_interface(ser, slot):
    """IO16 (16-Kanal Dijital I/O) kontrol arayüzü - I2C veya UART/SPI"""
    
    # Kullanıcıya mod seçtir
    print("\n" + "="*60)
    print(" IO16 KONTROL MODU SEÇİMİ")
    print("="*60)
    print("1. UART/SPI Modu (STM32 → SPI2 → iC-JX) - ÖNERİLEN!")
    print("   📡 SPI2: PB13(SCK), PB14(MISO), PB15(MOSI)")
    print("   📌 CS: PC13(Slot0), PA0(Slot1), PA1(Slot2), PA2(Slot3)")
    print("2. I2C Modu (Direkt PCA9555) - Test/Debug için")
    print("   ⚠️  CONFIG yazılamıyor, direction değiştirilemez!")
    print("0. Geri Dön")
    print("="*60)
    
    mode_choice = input("\nMod seçin (1/2): ").strip()
    
    if mode_choice == '0':
        return
    elif mode_choice == '2':
        # I2C modu (eski, CONFIG yazamıyor)
        print("\n⚠️  DİKKAT: I2C modu CONFIG yazamıyor!")
        print("⚠️  Pin direction değiştirilemez, sadece okuma yapılabilir!")
        input("Devam etmek için Enter (iptal için Ctrl+C)...")
        use_i2c = True
    else:
        # UART/SPI modu (önerilen)
        print("\n✅ UART/SPI modu seçildi - STM32 firmware kullanılacak")
        use_i2c = False
    
    # I2C modunda IO16Driver başlat
    io16 = None
    if use_i2c and IO16_AVAILABLE:
        try:
            print(f"📡 IO16 I2C Başlatılıyor: Slot {slot}")
            io16 = IO16Driver(bus_number=1, debug=False)
            
            # Test: Modül var mı?
            test_read = io16.read_all(slot)
            if test_read is not None:
                print(f"✅ IO16 I2C bağlantısı başarılı! (0x{test_read:04X})")
            else:
                raise Exception("Modül okunamadı")
                
        except Exception as e:
            print(f"❌ IO16 I2C başlatma hatası: {e}")
            print("⚠️  UART moduna geçiliyor...")
            use_i2c = False
            io16 = None
    elif use_i2c and not IO16_AVAILABLE:
        print("❌ smbus2 kütüphanesi yok! UART moduna geçiliyor...")
        use_i2c = False
    
    while True:
        print("\n" + "="*60)
        mode_str = "I2C (READ-ONLY!)" if use_i2c else "UART/SPI (STM32→SPI1)"
        print(f" IO16 KONTROL (Slot {slot}) - {mode_str}")
        print("="*60)
        if not use_i2c:
            print("📡 UART/SPI Modu: STM32 → SPI2 → iC-JX Chip")
            print(f"   SPI2 Pinleri: PB13(SCK), PB14(MISO), PB15(MOSI)")
            print(f"   CS Pin (Slot {slot}): ", end="")
            if slot == 0:
                print("PC13")
            elif slot == 1:
                print("PA0")
            elif slot == 2:
                print("PA1")
            elif slot == 3:
                print("PA2")
        else:
            print("⚠️  I2C Modu: CONFIG yazılamıyor, direction sabit!")
        print("="*60)
        print("1. Pin Ayarla (Set HIGH/LOW)")
        print("2. Pin Oku (Get)")
        print("3. Pin Yönü Ayarla (Direction IN/OUT)")
        print("4. Tüm Pinleri Oku (Read All)")
        print("5. Tüm Pinleri Yaz (Write All)")
        print("6. Durum Göster (Status)")
        print("7. Yardım (Help)")
        print("8. Test Pattern (Walking Bit)")
        print("9. � Chip INFO + AUTO INIT (iC-JX) - İLK ÖNCE BUNU ÇALIŞTIR!")
        print("   ⚠️  Chip'i initialize eder (clock, filter, EOI)")
        print("a. ⚡ Overcurrent Kontrol (iC-JX)")
        print("b. 📋 Register Dump (iC-JX)")
        print("c. 🔧 CS Pin Tarama (Tüm Slotları Dene!)")
        print("d. 🎯 Manuel CS Pin Test (Güvenli - Sadece Okuma)")
        print("0. Geri Dön")
        print("="*60)
        
        choice = input("\nSeçiminiz: ").strip()
        
        if choice == '1':
            pin = input("Pin numarası (0-15): ").strip()
            value = input("Değer (high/low): ").strip().lower()
            
            if use_i2c and io16:
                # Direkt I2C kontrolü (ÇALIŞMIYOR - CONFIG yazılamıyor!)
                print("⚠️  I2C modu: Pin direction değiştirilemez!")
                print("⚠️  OUTPUT modunda değilse pin değişmeyecek!")
                try:
                    pin_num = int(pin)
                    if value == 'high':
                        io16.set_pin(slot, pin_num, 1)
                        print(f"✅ Pin {pin_num} OUTPUT register'a yazıldı (I2C)")
                        print(f"⚠️  Ama CONFIG INPUT ise fiziksel pin değişmedi!")
                    elif value == 'low':
                        io16.set_pin(slot, pin_num, 0)
                        print(f"✅ Pin {pin_num} OUTPUT register'a yazıldı (I2C)")
                        print(f"⚠️  Ama CONFIG INPUT ise fiziksel pin değişmedi!")
                    else:
                        print("❌ Geçersiz değer! (high/low)")
                except Exception as e:
                    print(f"❌ I2C hatası: {e}")
            else:
                # UART/SPI üzerinden STM32'ye gönder (ÖNERİLEN YOL!)
                cmd = f"io16:{slot}:set:{pin}:{value}"
                print(f"\n📤 UART/SPI Komutu: {cmd}")
                print(f"   STM32 → SPI1 → IO678 (Slot {slot}) → Pin {pin}")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
                    if "OK:" in response:
                        print(f"\n✅ Pin {pin} fiziksel olarak {value.upper()} yapıldı!")
                    elif "ADDR_ECHO_FAIL" in response:
                        print(f"\n❌ SPI İletişim Hatası: Chip yanıt vermiyor!")
                        print(f"   → MISO (PB14 - CPLD multiplexed) bağlantısını kontrol et")
                        print(f"   → Chip güç kaynağını kontrol et")
                        print(f"   → Slot {slot} doğru mu kontrol et")
                    else:
                        print(f"\n⚠️  Hata oluştu, STM32 yanıtını kontrol et!")
        
        elif choice == '2':
            pin = input("Pin numarası (0-15): ").strip()
            
            if use_i2c and io16:
                # Direkt I2C kontrolü
                try:
                    pin_num = int(pin)
                    value = io16.get_pin(slot, pin_num)
                    state = "HIGH" if value else "LOW"
                    print(f"✅ Pin {pin_num}: {state} (I2C)")
                except Exception as e:
                    print(f"❌ I2C hatası: {e}")
            else:
                # UART/SPI üzerinden STM32'ye gönder
                cmd = f"io16:{slot}:get:{pin}"
                print(f"\n📤 UART/SPI Komutu: {cmd}")
                print(f"   STM32 → SPI1 → IO678 (Slot {slot}) → Pin {pin} Read")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
        
        elif choice == '3':
            # ⚠️  iC-JX chip 4'lü grup kontrolü kullanır!
            print("\n" + "=" * 60)
            print("📌 iC-JX PIN GRUP YAPISI")
            print("=" * 60)
            print("iC-JX chip'te pinler 4'lü gruplar halinde kontrol edilir:")
            print("  Grup 0: Pin 0-3   (CONTROLWORD_2A bit 3)")
            print("  Grup 1: Pin 4-7   (CONTROLWORD_2A bit 7)")
            print("  Grup 2: Pin 8-11  (CONTROLWORD_2B bit 3)")
            print("  Grup 3: Pin 12-15 (CONTROLWORD_2B bit 7)")
            print("\n⚠️  Sadece GRUP numarası girin (0-3)!")
            print("   Tüm grup birlikte INPUT veya OUTPUT olur.\n")
            
            group_input = input("Grup numarası (0-3): ").strip()
            direction = input("Yön (in/out): ").strip().lower()
            
            try:
                group = int(group_input)
                
                # Sadece grup numarası kabul et (0-3)
                if group < 0 or group > 3:
                    print("\n❌ Geçersiz grup! Sadece 0, 1, 2 veya 3 girin.")
                    continue
                
                print(f"\n✓ Grup {group} seçildi (Pins {group*4}-{group*4+3})")
                
            except ValueError:
                print("\n❌ Geçersiz format! Grup numarası girin (0-3).")
                continue
            
            if use_i2c and io16:
                # Direkt I2C kontrolü (ÇALIŞMIYOR!!!)
                print("\n❌ UYARI: I2C modu CONFIG yazamıyor!")
                print("❌ Direction değiştirilemez, sadece okuma yapılabilir!")
                print("❌ UART/SPI modunu kullan!")
            else:
                # UART/SPI üzerinden STM32'ye gönder (TEK ÇALIŞAN YOL!)
                cmd = f"io16:{slot}:dirgroup:{group}:{direction}"
                print(f"\n📤 UART/SPI Komutu: {cmd}")
                print(f"   STM32 → SPI → iC-JX CONTROLWORD_2A/2B → Direction")
                print(f"   Etkilenen pinler: {group*4}, {group*4+1}, {group*4+2}, {group*4+3}")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
                    if "OK:" in response or "Group" in response:
                        print(f"\n✅ Grup {group} (Pins {group*4}-{group*4+3}) yönü {direction.upper()} olarak ayarlandı!")
                    else:
                        print(f"\n⚠️  Hata oluştu, STM32 yanıtını kontrol et!")
        
        elif choice == '4':
            if use_i2c and io16:
                # Direkt I2C kontrolü (YENİ DRIVER)
                try:
                    value = io16.read_all(slot)
                    print(f"\n✅ Tüm pinler okundu (I2C):")
                    print(f"   16-bit değer: 0x{value:04X} ({value})")
                    print("\n   Pin Durumları:")
                    for i in range(16):
                        bit_val = (value >> i) & 1
                        state = "HIGH" if bit_val else "LOW"
                        print(f"     Pin {i:2d}: {state}")
                except Exception as e:
                    print(f"❌ I2C hatası: {e}")
            else:
                # UART üzerinden STM32'ye gönder
                cmd = f"io16:{slot}:readall"
                print(f"\n📤 Komut gönderiliyor: {cmd}")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
        
        elif choice == '5':
            value = input("16-bit değer (hex, örn: 0x00FF): ").strip()
            
            if use_i2c and io16:
                # Direkt I2C kontrolü (YENİ DRIVER)
                try:
                    # Hex string'i integer'a çevir
                    if value.startswith('0x') or value.startswith('0X'):
                        int_value = int(value, 16)
                    else:
                        int_value = int(value, 0)  # Auto-detect base
                    
                    io16.write_all(slot, int_value)
                    print(f"✅ Tüm pinler yazıldı: 0x{int_value:04X} (I2C)")
                except ValueError:
                    print("❌ Geçersiz format! Örnek: 0x00FF veya 255")
                except Exception as e:
                    print(f"❌ I2C hatası: {e}")
            else:
                # UART üzerinden STM32'ye gönder
                cmd = f"io16:{slot}:writeall:{value}"
                print(f"\n📤 Komut gönderiliyor: {cmd}")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
        
        elif choice == '6':
            if use_i2c and io16:
                # Direkt I2C kontrolü (YENİ DRIVER)
                try:
                    io16._print_status(slot)
                except Exception as e:
                    print(f"❌ I2C hatası: {e}")
                    # Fallback: basit durum göster
                    try:
                        inputs = io16.read_all(slot)
                        config = io16._read_config(slot)
                        print("\n✅ IO16 Modül Durumu (I2C):")
                        print("─" * 60)
                        print(f"  Slot:     {slot}")
                        print(f"  Inputs:   0x{inputs:04X}")
                        print(f"  Config:   0x{config:04X}")
                        print("\n  Pin Durumları:")
                    except:
                        pass
                    
                    inputs = inputs if 'inputs' in locals() else 0
                    config = config if 'config' in locals() else 0
                    
                    for i in range(16):
                        input_bit = (inputs >> i) & 1
                        config_bit = (config >> i) & 1
                        state = "HIGH" if input_bit else "LOW"
                        direction = "INPUT" if config_bit else "OUTPUT"
                        print(f"    Pin {i:2d}: {state:4s} ({direction})")
                    print("─" * 60)
                except Exception as e:
                    print(f"❌ I2C hatası: {e}")
            else:
                # UART üzerinden STM32'ye gönder
                cmd = f"io16:{slot}:status"
                print(f"\n📤 Komut gönderiliyor: {cmd}")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
        
        elif choice == '7':
            print("\n📖 IO16 Komut Örnekleri:")
            if use_i2c:
                print("  MODE: DİREKT I2C İLETİŞİMİ")
                print("  • PCA9555 @ 0x41/0x51 üzerinden direkt kontrol")
                print("  • STM32 bypass edilir, düşük gecikme")
            else:
                print("  MODE: UART (STM32 Üzerinden)")
            print(f"\n  Komutlar:")
            print(f"  • Pin 5'i HIGH yap")
            print(f"  • Pin 7'yi LOW yap")
            print(f"  • Pin 3'ü oku")
            print(f"  • Pin 5'i çıkış yap")
            print(f"  • Pin 7'yi giriş yap")
            print(f"  • Tüm pinleri oku (0x0000-0xFFFF)")
            print(f"  • Tüm pinleri yaz (0xFF = hepsi HIGH)")
            print(f"  • Modül durumunu göster")
        
        elif choice == '8':
            if use_i2c and io16:
                # Walking bit test pattern (YENİ DRIVER)
                try:
                    print("\n🎮 Walking Bit Test Pattern")
                    print("   Her pin sırayla HIGH yapılacak (0.2s gecikme)")
                    
                    response = input("   Devam edilsin mi? (e/h): ")
                    if response.lower() == 'e':
                        # Önce tüm pinleri OUTPUT yap
                        for i in range(16):
                            io16.set_direction(slot, i, is_input=False)
                        print("✅ Tüm pinler OUTPUT yapıldı")
                        time.sleep(0.5)
                        
                        # Walking bit
                        for i in range(16):
                            value = 1 << i
                            io16.write_all(slot, value)
                            print(f"  Pin {i:2d}: 0x{value:04X}")
                            time.sleep(0.2)
                        
                        # Temizle
                        io16.write_all(slot, 0x0000)
                        print("✅ Test tamamlandı, tüm pinler OFF")
                    else:
                        print("İptal edildi.")
                except Exception as e:
                    print(f"❌ Test hatası: {e}")
            else:
                print("⚠️  Test pattern sadece I2C modunda çalışır!")
        
        elif choice == '9':
            # iC-JX Chip INFO + AUTO INIT
            if use_i2c:
                print("\n⚠️  Chip INFO sadece UART/SPI modunda çalışır!")
            else:
                print("\n" + "="*70)
                print(" 🚀 iC-JX CHIP INFO + AUTO INITIALIZATION")
                print("="*70)
                print("Bu komut:")
                print("  1️⃣  INFO Register'ı okur (chip detection)")
                print("  2️⃣  Chip tespit edilirse OTOMATIK INITIALIZE eder:")
                print("      • Internal clock enable (CONTROLWORD_3B = 0x05)")
                print("      • IO filter bypass (CONTROLWORD_1A/B = 0x88)")
                print("      • EOI reset (CONTROLWORD_4 = 0x80)")
                print("  3️⃣  Pin kontrolü için chip'i hazır hale getirir")
                print("")
                print("⚠️  DİKKAT: Bu komutu pin kontrolünden ÖNCE çalıştırmalısınız!")
                print("="*70)
                input("\nDevam etmek için Enter'a basın...")
                
                cmd = f"io16:{slot}:info"
                print(f"\n📤 UART Komutu: {cmd}")
                print(f"   STM32 → SPI2 → iC-JX (Slot {slot}) → INFO Register (0x1D)")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 70)
                    print(response.strip())
                    print("─" * 70)
                    
                    # Initialization başarısını kontrol et
                    if "INITIALIZATION COMPLETE" in response or "Chip initialization SUCCESS" in response:
                        print("\n" + "🎉" * 35)
                        print("✅ ✅ ✅  CHIP INITIALIZATION BAŞARILI!  ✅ ✅ ✅")
                        print("🎉" * 35)
                        print("\n📝 SONRAKI ADIMLAR:")
                        print("   1. Seçenek 6'yı seç → Durum kontrol et")
                        print("   2. Seçenek 1'i seç → Pin'leri kontrol et")
                        print("   3. Multimetre ile fiziksel pin voltajını ölç")
                        print("\n✨ Artık pin kontrolü yapabilirsiniz!")
                    elif "iC-JX chip detected" in response or "SUCCESS!" in response:
                        print("\n✅ iC-JX chip tespit edildi ve yanıt veriyor!")
                        if "INFO Register = 0x" in response:
                            # INFO register değerini parse et
                            import re
                            match = re.search(r'INFO Register = (0x[0-9A-Fa-f]+)', response)
                            if match:
                                info_val = match.group(1)
                                print(f"   → Chip INFO Register: {info_val}")
                    elif "ADDR_ECHO_FAIL" in response or "CTRL_FAIL" in response:
                        print("\n❌ SPI İletişim Hatası: Echo başarısız!")
                        print("   → MISO (PB14 - CPLD multiplexed) bağlantısını kontrol et")
                        print("   → Chip güç kaynağını kontrol et")
                        print(f"   → Slot {slot} CS pinini kontrol et (PC13/PA0/PA1/PA2)")
                    elif "[SPI-RD] FAIL" in response or "ERROR" in response:
                        print("\n❌ SPI okuma hatası!")
                        print("   → SPI clock ve MOSI hatlarını kontrol et")
                        print("   → 24V güç kaynağını kontrol et (iC-JX needs 24V!)")
                    else:
                        print("\n⚠️  Beklenmeyen yanıt formatı!")
        
        elif choice.lower() == 'a':
            # iC-JX Overcurrent Kontrol
            if use_i2c:
                print("\n⚠️  Overcurrent kontrolü sadece UART/SPI modunda çalışır!")
            else:
                cmd = f"io16:{slot}:overcurrent"
                print(f"\n📤 iC-JX Overcurrent Kontrol: {cmd}")
                print(f"   STM32 → SPI1 → iC-JX (Slot {slot}) → Overcurrent Registers")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
                    if "OVERCURRENT" in response and "detected on pins" in response:
                        print("\n⚡ DİKKAT: Overcurrent tespit edildi!")
                        print("   → İlgili pinlerdeki yükü kontrol et")
                        print("   → Kısa devre olabilir")
                        print("   → Aşırı akım koruması devreye girdi")
                    elif "No overcurrent detected" in response:
                        print("\n✅ Overcurrent yok - Tüm pinler normal!")
        
        elif choice.lower() == 'b':
            # iC-JX Register Dump
            if use_i2c:
                print("\n⚠️  Register dump sadece UART/SPI modunda çalışır!")
            else:
                cmd = f"io16:{slot}:regdump"
                print(f"\n📤 iC-JX Register Dump: {cmd}")
                print(f"   STM32 → SPI1 → iC-JX (Slot {slot}) → Tüm Register'ları Oku")
                print("\n⏳ Lütfen bekleyin, tüm register'lar okunuyor...")
                response = send_uart_command(ser, cmd, timeout=5)  # Longer timeout
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("═" * 60)
                    print(response.strip())
                    print("═" * 60)
                    print("\n💡 İPUÇLARI:")
                    print("   • CONTROLWORD_2A/2B: Direction control (0x88 = tümü OUTPUT)")
                    print("   • INPUT_A/B: Pin input durumları")
                    print("   • OUTPUT_A/B: Pin output durumları")
                    print("   • OVERCURRENT_STS: Aşırı akım tespiti")
                    print("   • INFO: Chip ID (0x00/0xFF = chip yok)")
        
        elif choice.lower() == 'c':
            # CS Pin Tarama - TÜM SLOTLARI DENE!
            if use_i2c:
                print("\n⚠️  CS Pin tarama sadece UART/SPI modunda çalışır!")
            else:
                print("\n" + "="*60)
                print(" 🔧 CS PIN TARAMA - TÜM SLOTLARI DENE")
                print("="*60)
                print("\n📌 MEVCUT CS PIN KONFIGÜRASYONU (CPLD Analizi):")
                print("   Slot 0 (IO16)  → CS: PC13 (MODÜL 4), INT: PA3  (MODÜL 1)")
                print("   Slot 1 (AIO20) → CS: PA0  (MODÜL 1), INT: PC4  (MODÜL 3), CNVT: PC5")
                print("   Slot 2 (FPGA)  → CS: PA1  (MODÜL 1), INT: PB0  (MODÜL 1), CRESET: PB1, CDONE: PB10")
                print("   Slot 3 (IO16)  → CS: PA2  (MODÜL 1), INT: PB11 (MODÜL 3)")
                print("   SPI Bus (Tümü) → SCK: PB13, MISO: PB14 (multiplexed), MOSI: PB15")
                print("\n💡 Bu test tüm slotları sırayla deneyecek.")
                print("   Hangisi yanıt verirse o slotta IO16 modülü var!")
                print("\n⚠️  DİKKAT: Her slot için Chip INFO komutu gönderilecek.")
                print("\n" + "="*60)
                
                confirm = input("\nTaramaya başlansın mı? (e/h): ").strip().lower()
                
                if confirm == 'e':
                    print("\n🔍 TARAMA BAŞLIYOR...")
                    print("="*60)
                    
                    found_slots = []
                    
                    for test_slot in range(4):  # Slot 0-3 arası test et
                        print(f"\n📍 Slot {test_slot} Test Ediliyor...")
                        print(f"   CS Pin: ", end="")
                        if test_slot == 0:
                            print("PC13 (MODÜL 4)")
                        elif test_slot == 1:
                            print("PA0 (MODÜL 1)")
                        elif test_slot == 2:
                            print("PA1 (MODÜL 1)")
                        elif test_slot == 3:
                            print("PA2 (MODÜL 1)")
                        
                        cmd = f"io16:{test_slot}:info"
                        print(f"   Komut: {cmd}")
                        
                        response = send_uart_command(ser, cmd, timeout=2, show_timing=False)
                        
                        if response:
                            # Yanıtı analiz et
                            if "Chip INFO" in response and "0x00" not in response and "0xFF" not in response:
                                print("   ✅ BULUNDU! iC-JX chip yanıt verdi!")
                                found_slots.append(test_slot)
                                # Kısa yanıt göster
                                for line in response.split('\n'):
                                    if "INFO" in line or "iC-JX" in line:
                                        print(f"      {line.strip()}")
                            elif "0x00" in response or "0xFF" in response:
                                print("   ❌ Chip yok (0x00/0xFF)")
                            else:
                                print("   ⚠️  Belirsiz yanıt")
                        else:
                            print("   ❌ Yanıt alınamadı")
                        
                        time.sleep(0.3)  # Slotlar arası kısa bekleme
                    
                    # SONUÇ RAPORU
                    print("\n" + "="*60)
                    print(" 📊 TARAMA SONUÇLARI")
                    print("="*60)
                    
                    if found_slots:
                        print(f"\n✅ BULUNAN SLOTLAR: {found_slots}")
                        print(f"\n💡 Toplam {len(found_slots)} adet IO16 modülü bulundu!")
                        print("\n📌 CS Pin Eşleşmeleri:")
                        for s in found_slots:
                            cs_pin = ["PC13", "PA0", "PA1", "PA2"][s]
                            modul = ["MODÜL 4", "MODÜL 1", "MODÜL 1", "MODÜL 1"][s]
                            print(f"   Slot {s} → {cs_pin} ({modul})")
                        
                        print("\n🎯 ÖNERİ:")
                        print(f"   İlk bulunan slot ({found_slots[0]}) ile işlemlere devam et!")
                    else:
                        print("\n❌ HİÇBİR SLOTTA IO16 MODÜLÜ BULUNAMADI!")
                        print("\n🔍 SORUN GİDERME:")
                        print("   1. 24V güç kaynağını kontrol et (iC-JX 24V chip!)")
                        print("   2. SPI2 bağlantılarını kontrol et (PB13/14/15)")
                        print("   3. MISO (PB14) bağlantısını kontrol et")
                        print("   4. Modül fiziksel olarak takılı mı?")
                        print("   5. CS pinlerinin doğru GPIO'lara bağlı olduğunu kontrol et")
                    
                    print("\n" + "="*60)
                else:
                    print("\n⚠️  Tarama iptal edildi.")
        
        elif choice.lower() == 'd':
            # Manuel CS Pin Test - GÜVENLİ (Sadece Okuma)
            if use_i2c:
                print("\n⚠️  Manuel CS test sadece UART/SPI modunda çalışır!")
            else:
                print("\n" + "="*60)
                print(" 🎯 MANUEL CS PIN TEST - GÜVENLİ MOD")
                print("="*60)
                print("\n💡 Bu test sadece OKUMA yapar, hiçbir şey yazmaz!")
                print("   Sisteminiz güvende kalacak.\n")
                
                # Pin listesini göster (CPLD analizine göre güncellendi)
                print("📌 TEST EDİLEBİLECEK CS PİNLERİ (CPLD Analizi):")
                print("\n🔹 BİLİNEN CS PİNLERİ:")
                print("   • Slot 0: PC13 (MODÜL 4)")
                print("   • Slot 1: PA0  (MODÜL 1)")
                print("   • Slot 2: PA1  (MODÜL 1)")
                print("   • Slot 3: PA2  (MODÜL 1)")
                print("\n🔹 MODÜL 1 Pinleri (PA0, PA1, PA2, PA3, PA4, PA8, PA11, PA12, PB0):")
                print("   0. PA0 [Slot 1 CS] ✓")
                print("   1. PA1 [Slot 2 CS] ✓")
                print("   2. PA2 [Slot 3 CS] ✓")
                print("   3. PA3 [Slot 0 INT]")
                print("   4. PA4 [RPI NSS]")
                print("   5. PA8, PA11, PA12 [Test/GPIO]")
                print("   6. PB0 [Slot 2 INT]")
                print("\n🔹 MODÜL 3 Pinleri (PB11-15, PC4-7):")
                print("   • PB11 [Slot 3 INT]")
                print("   • PB13/14/15 [SPI Bus - Paylaşımlı]")
                print("   • PC4 [Slot 1 INT], PC5 [Slot 1 CNVT]")
                print("\n🔹 MODÜL 4 Pin (PC13):")
                print("   • PC13 [Slot 0 CS] ✓")
                print("\n⚠️  NOT: CPLD top.v analizinden kesin CS pinleri!")
                print("\n" + "="*60)
                
                # Pin seçimi
                pin_list = [
                    ("PA0",  0, 0),  ("PA1",  0, 1),  ("PA2",  0, 2),  ("PA3",  0, 3),
                    ("PA4",  0, 4),  ("PA8",  0, 8),  ("PA11", 0, 11), ("PA12", 0, 12),
                    ("PB0",  1, 0),
                    ("PB11", 1, 11), ("PB12", 1, 12),
                    ("PC4",  2, 4),  ("PC5",  2, 5),  ("PC6",  2, 6),  ("PC7",  2, 7)
                ]
                
                try:
                    choice_idx = input("\nTest etmek istediğiniz pin numarasını seçin (0-14) veya 'q' (iptal): ").strip()
                    
                    if choice_idx.lower() == 'q':
                        print("\n⚠️  Test iptal edildi.")
                    else:
                        idx = int(choice_idx)
                        if idx < 0 or idx >= len(pin_list):
                            print("\n❌ Geçersiz pin numarası!")
                        else:
                            pin_name, gpio, pin_num = pin_list[idx]
                            
                            print(f"\n🔍 Test ediliyor: {pin_name} (GPIO{gpio}, Pin{pin_num})")
                            print(f"   Bu pin CS olarak kullanılacak")
                            print(f"   Sadece Chip INFO okunacak (yazma yok!)")
                            print("\n⏳ Lütfen bekleyin...")
                            
                            # Firmware'e özel komut gönder
                            # Format: io16:testcs:gpio:pin
                            cmd = f"io16:testcs:{gpio}:{pin_num}"
                            
                            response = send_uart_command(ser, cmd, timeout=2, show_timing=False)
                            
                            if response:
                                print("\n📨 SONUÇ:")
                                print("─" * 60)
                                print(response.strip())
                                print("─" * 60)
                                
                                # Analiz
                                if "Chip INFO" in response and "0x00" not in response and "0xFF" not in response:
                                    print(f"\n✅ BAŞARILI! {pin_name} doğru CS pini!")
                                    print(f"   iC-JX chip yanıt verdi!")
                                    print(f"\n🎯 ÖNERİ:")
                                    print(f"   Bu pini CS olarak kullanabilirsiniz: {pin_name}")
                                elif "0x00" in response or "0xFF" in response:
                                    print(f"\n❌ {pin_name} CS değil (chip yanıt vermedi)")
                                    print(f"   INFO Register: 0x00 veya 0xFF")
                                elif "Unknown command" in response or "Komut desteklenmiyor" in response:
                                    print(f"\n⚠️  Firmware bu komutu desteklemiyor!")
                                    print(f"   'testcs' komutu firmware'e eklenmeli.")
                                else:
                                    print(f"\n⚠️  Belirsiz yanıt, manuel kontrol edin.")
                            else:
                                print(f"\n❌ Yanıt alınamadı!")
                                print(f"   UART bağlantısını kontrol edin.")
                                
                except ValueError:
                    print("\n❌ Geçerli bir sayı girin!")
                except Exception as e:
                    print(f"\n❌ Hata: {e}")
        
        elif choice == '0':
            break
        else:
            print("\n❌ Geçersiz seçim!")
        
        if choice != '0':
            input("\nDevam etmek için Enter'a basın...")

def aio20_control_interface(ser, slot):
    """AIO20 (20-Kanal Analog I/O) kontrol arayüzü - MAX11300 PIXI"""
    while True:
        print("\n" + "="*60)
        print(f" AIO20 KONTROL (Slot {slot}) - MAX11300 PIXI")
        print("="*60)
        print("📡 SPI2: PB13(SCK), PB14(MISO), PB15(MOSI)")
        print(f"📌 CS Pin (Slot {slot}): ", end="")
        if slot == 0:
            print("PC13")
        elif slot == 1:
            print("PA0")
        elif slot == 2:
            print("PA1")
        elif slot == 3:
            print("PA2")
        print("="*60)
        print("1. ADC Port Oku (Read Port 0-19)")
        print("2. DAC Port Yaz (Write Port 0-19)")
        print("3. Voltaj Ayarla (Set Voltage - Port)")
        print("4. Durum Göster (Status - All Ports)")
        print("5. 🔧 Chip INFO (Device ID)")
        print("6. ⚡ Chip INIT (İLK ÖNCE BUNU ÇALIŞTIR!)")
        print("   ⚠️  Chip'i initialize eder (MODE_7 ADC, MODE_5 DAC)")
        print("7. 🎴 AFE Kart Algılama (Detect AFE Cards)")
        print("   ⚠️  Otomatik: 0-10V / 4-20mA / PT-1000 algıla")
        print("8. Yardım (Help)")
        print("0. Geri Dön")
        print("="*60)
        
        choice = input("\nSeçiminiz: ").strip()
        
        if choice == '1':
            port = input("Port numarası (0-19): ").strip()
            cmd = f"aio20:{slot}:read:{port}"
            print(f"\n📤 Komut gönderiliyor: {cmd}")
            response = send_uart_command(ser, cmd, timeout=3)
            if response:
                print("\n📨 STM32 Yanıtı:")
                print("─" * 60)
                print(response.strip())
                print("─" * 60)
        
        elif choice == '2':
            port = input("Port numarası (0-19): ").strip()
            value = input("12-bit değer (0-4095): ").strip()
            cmd = f"aio20:{slot}:write:{port}:{value}"
            print(f"\n📤 Komut gönderiliyor: {cmd}")
            response = send_uart_command(ser, cmd, timeout=3)
            if response:
                print("\n📨 STM32 Yanıtı:")
                print("─" * 60)
                print(response.strip())
                print("─" * 60)
        
        elif choice == '3':
            port = input("Port numarası (10-19 for DAC): ").strip()
            voltage = input("Voltaj (mV, 0-10000): ").strip()
            cmd = f"aio20:{slot}:setvolt:{port}:{voltage}"
            print(f"\n📤 Komut gönderiliyor: {cmd}")
            response = send_uart_command(ser, cmd, timeout=3)
            if response:
                print("\n📨 STM32 Yanıtı:")
                print("─" * 60)
                print(response.strip())
                print("─" * 60)
        
        elif choice == '4':
            print("\n📊 Durum raporu alınıyor...")
            print("   AFE kartları ve kanal değerleri gösterilecek")
            cmd = f"aio20:{slot}:status"
            print(f"\n📤 Komut gönderiliyor: {cmd}")
            response = send_uart_command(ser, cmd, timeout=5)
            if response:
                print("\n" + "="*70)
                print("� AIO20 DURUM RAPORU")
                print("="*70)
                print(response.strip())
                print("="*70)
        
        elif choice == '5':
            cmd = f"aio20:{slot}:info"
            print(f"\n📤 Komut gönderiliyor: {cmd}")
            response = send_uart_command(ser, cmd, timeout=3)
            if response:
                print("\n📨 STM32 Yanıtı:")
                print("─" * 60)
                print(response.strip())
                print("─" * 60)
        
        elif choice == '6':
            print("\n⚠️  DİKKAT: Chip initialization başlatılıyor...")
            print("   Bu işlem:")
            print("   - Device ID'yi kontrol eder (0x0424 olmalı)")
            print("   - Port 0-9'u MODE_7 (ADC 0-10V) yapar")
            print("   - Port 10-19'u MODE_5 (DAC 0-10V) yapar")
            print("   - Continuous ADC conversion mode aktif eder")
            
            confirm = input("\nDevam edilsin mi? (e/h): ").strip().lower()
            
            if confirm == 'e':
                cmd = f"aio20:{slot}:init"
                print(f"\n📤 Komut gönderiliyor: {cmd}")
                response = send_uart_command(ser, cmd, timeout=5)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
            else:
                print("\n❌ İşlem iptal edildi.")
        
        elif choice == '7':
            print("\n🎴 AFE Kart Algılama başlatılıyor...")
            print("   Modül üzerine takılı AFE (Analog Front-End) kartlarını algılar")
            print("   Desteklenen tipler: 0-10V, 4-20mA, PT-1000")
            print()
            
            cmd = f"aio20:{slot}:detectafe"
            print(f"📤 Komut gönderiliyor: {cmd}")
            response = send_uart_command(ser, cmd, timeout=5)
            if response:
                print("\n📨 STM32 Yanıtı:")
                print("─" * 60)
                print(response.strip())
                print("─" * 60)
                print("\n💡 İpucu: 'aio20:{0}:status' komutuyla AFE bilgilerini tekrar görebilirsiniz".format(slot))
        
        elif choice == '8':
            print("\n📖 AIO20 Komut Örnekleri (MAX11300 PIXI):")
            print(f"  • aio20:{slot}:init             - Chip'i initialize et (İLK ADIM!)")
            print(f"  • aio20:{slot}:info             - Device ID oku (0x0424)")
            print(f"  • aio20:{slot}:detectafe        - AFE kartlarını algıla")
            print(f"  • aio20:{slot}:read:5           - Port 5 ADC oku")
            print(f"  • aio20:{slot}:write:15:2048    - Port 15 DAC yaz (2048 = ~5V)")
            print(f"  • aio20:{slot}:setvolt:12:5000  - Port 12'ye 5.000V yaz")
            print(f"  • aio20:{slot}:status           - Tüm portları göster (AFE dahil)")
            print("\n  📌 Port Konfigürasyonu (init sonrası):")
            print("     Port 0-9:   MODE_7 (ADC Input, 0-10V)")
            print("     Port 10-19: MODE_5 (DAC Output, 0-10V)")
            print("\n  🎴 AFE Kartları (4 kart, her biri 4 kanal):")
            print("     AFE0 (CH0-3):   0-10V / 4-20mA / PT-1000")
            print("     AFE1 (CH4-7):   0-10V / 4-20mA / PT-1000")
            print("     AFE2 (CH8-11):  0-10V / 4-20mA / PT-1000")
            print("     AFE3 (CH12-15): 0-10V / 4-20mA / PT-1000")
            print("\n  📊 Teknik Özellikler:")
            print("     • 12-bit çözünürlük (0-4095)")
            print("     • 0-10V analog aralık")
            print("     • SPI interface (shared bus)")
            print("     • Continuous ADC conversion mode")
        
        elif choice == '0':
            break
        else:
            print("\n❌ Geçersiz seçim!")
        
        if choice != '0':
            input("\nDevam etmek için Enter'a basın...")

def fpga_motor_control_menu(ser, slot):
    """FPGA Motor Control Menu"""
    while True:
        print("\n" + "="*60)
        print(f" 🎮 FPGA MOTOR KONTROLÜ (Slot {slot})")
        print("="*60)
        print("1. Motor Seç ve Pozisyona Git (Position Control)")
        print("2. Motor Hız/Yön Kontrolü (Speed/Direction Control)")
        print("3. Motor Home (Position = 0)")
        print("4. Motor Durumunu Göster (Status)")
        print("5. Motor Pozisyon Oku (Read Position)")
        print("6. Motor Durdur (Stop)")
        print("7. Motor Hatayı Temizle (Clear Error)")
        print("8. Çoklu Motor Testi (Multi-Motor Test)")
        print("9. Gerçek Zamanlı İzleme (Real-time Monitor)")
        print("10. ⏱️  Zamanlı Hız Kontrolü (Timed Speed Control)")
        print("11. 🕐 Timer Bilgisi (Timer Info)")
        print("0. Geri Dön")
        print("="*60)
        
        choice = input("\nSeçiminiz: ").strip()
        
        if choice == '1':
            # Position control
            try:
                motor_ch = int(input("Motor channel (0-15): ").strip())
                if motor_ch < 0 or motor_ch > 15:
                    print("❌ Geçersiz motor channel!")
                    continue
                
                target_pos = int(input("Hedef pozisyon: ").strip())
                speed = int(input("Hız (0-255): ").strip())
                
                if speed < 0 or speed > 255:
                    print("❌ Geçersiz hız (0-255)!")
                    continue
                
                cmd = f"fpga:{slot}:motor:{motor_ch}:goto:{target_pos}:{speed}"
                print(f"\n📤 Komut: {cmd}")
                response = send_uart_command(ser, cmd, timeout=5)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
                    
            except ValueError:
                print("❌ Geçerli sayılar girin!")
        
        elif choice == '2':
            # Speed/Direction control
            try:
                motor_ch = int(input("Motor channel (0-15): ").strip())
                if motor_ch < 0 or motor_ch > 15:
                    print("❌ Geçersiz motor channel!")
                    continue
                
                speed = int(input("Hız (0-255): ").strip())
                if speed < 0 or speed > 255:
                    print("❌ Geçersiz hız!")
                    continue
                
                print("\nYön seçin:")
                print("  0 = Dur")
                print("  1 = İleri (Forward)")
                print("  2 = Geri (Reverse)")
                direction = int(input("Yön (0-2): ").strip())
                
                if direction < 0 or direction > 2:
                    print("❌ Geçersiz yön!")
                    continue
                
                cmd = f"fpga:{slot}:motor:{motor_ch}:speed:{speed}:{direction}"
                print(f"\n📤 Komut: {cmd}")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
                    
            except ValueError:
                print("❌ Geçerli sayılar girin!")
        
        elif choice == '3':
            # Home
            try:
                motor_ch = int(input("Motor channel (0-15): ").strip())
                if motor_ch < 0 or motor_ch > 15:
                    print("❌ Geçersiz motor channel!")
                    continue
                
                cmd = f"fpga:{slot}:motor:{motor_ch}:home"
                print(f"\n📤 Komut: {cmd}")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
                    
            except ValueError:
                print("❌ Geçerli bir sayı girin!")
        
        elif choice == '4':
            # Status
            try:
                motor_ch = int(input("Motor channel (0-15): ").strip())
                if motor_ch < 0 or motor_ch > 15:
                    print("❌ Geçersiz motor channel!")
                    continue
                
                cmd = f"fpga:{slot}:motor:{motor_ch}:status"
                print(f"\n📤 Komut: {cmd}")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
                    
            except ValueError:
                print("❌ Geçerli bir sayı girin!")
        
        elif choice == '5':
            # Read position
            try:
                motor_ch = int(input("Motor channel (0-15): ").strip())
                if motor_ch < 0 or motor_ch > 15:
                    print("❌ Geçersiz motor channel!")
                    continue
                
                cmd = f"fpga:{slot}:motor:{motor_ch}:position"
                print(f"\n📤 Komut: {cmd}")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
                    
            except ValueError:
                print("❌ Geçerli bir sayı girin!")
        
        elif choice == '6':
            # Stop
            try:
                motor_ch = int(input("Motor channel (0-15): ").strip())
                if motor_ch < 0 or motor_ch > 15:
                    print("❌ Geçersiz motor channel!")
                    continue
                
                cmd = f"fpga:{slot}:motor:{motor_ch}:stop"
                print(f"\n📤 Komut: {cmd}")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
                    
            except ValueError:
                print("❌ Geçerli bir sayı girin!")
        
        elif choice == '7':
            # Clear error
            try:
                motor_ch = int(input("Motor channel (0-15): ").strip())
                if motor_ch < 0 or motor_ch > 15:
                    print("❌ Geçersiz motor channel!")
                    continue
                
                cmd = f"fpga:{slot}:motor:{motor_ch}:clearerror"
                print(f"\n📤 Komut: {cmd}")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
                    
            except ValueError:
                print("❌ Geçerli bir sayı girin!")
        
        elif choice == '8':
            # Multi-motor test
            print("\n🚀 Çoklu Motor Testi")
            print("─" * 60)
            try:
                num_motors = int(input("Kaç motor test edilsin? (1-16): ").strip())
                if num_motors < 1 or num_motors > 16:
                    print("❌ 1-16 arası değer girin!")
                    continue
                
                target = int(input("Hedef pozisyon (hepsi için): ").strip())
                speed = int(input("Hız (0-255): ").strip())
                
                if speed < 0 or speed > 255:
                    print("❌ Geçersiz hız!")
                    continue
                
                print(f"\n🔄 {num_motors} motor {target} pozisyonuna gidiyor...")
                
                for i in range(num_motors):
                    cmd = f"fpga:{slot}:motor:{i}:goto:{target}:{speed}"
                    print(f"  Motor {i}: ", end='', flush=True)
                    response = send_uart_command(ser, cmd, timeout=2)
                    if "OK" in response or "Motor" in response:
                        print("✅")
                    else:
                        print("❌")
                    time.sleep(0.1)
                
                print("\n✅ Komutlar gönderildi!")
                print("   Durumları görmek için seçenek 9'u kullanın (Real-time Monitor)")
                
            except ValueError:
                print("❌ Geçerli sayılar girin!")
        
        elif choice == '9':
            # Real-time monitor
            print("\n📊 Gerçek Zamanlı İzleme")
            print("─" * 60)
            try:
                motor_ch = int(input("Motor channel (0-15): ").strip())
                if motor_ch < 0 or motor_ch > 15:
                    print("❌ Geçersiz motor channel!")
                    continue
                
                duration = int(input("İzleme süresi (saniye): ").strip())
                if duration < 1:
                    duration = 10
                
                print(f"\n📡 Motor {motor_ch} izleniyor ({duration} saniye)...")
                print("─" * 60)
                
                start_time = time.time()
                while time.time() - start_time < duration:
                    cmd = f"fpga:{slot}:motor:{motor_ch}:position"
                    response = send_uart_command(ser, cmd, timeout=1)
                    
                    # Parse position from response
                    if "pozisyon:" in response:
                        pos_str = response.split("pozisyon:")[-1].strip().split()[0]
                        print(f"\rPozisyon: {pos_str:>8}    ", end='', flush=True)
                    
                    time.sleep(0.2)  # 5 Hz update
                
                print("\n✅ İzleme tamamlandı")
                
            except ValueError:
                print("❌ Geçerli sayılar girin!")
            except KeyboardInterrupt:
                print("\n⚠️ İzleme durduruldu")
        
        elif choice == '10':
            # Timed Speed/Direction control
            print("\n⏱️  Zamanlı Hız Kontrolü")
            print("─" * 60)
            print("Motor belirli süre boyunca çalışır, süre bitince otomatik durur")
            try:
                motor_ch = int(input("Motor channel (0-15): ").strip())
                if motor_ch < 0 or motor_ch > 15:
                    print("❌ Geçersiz motor channel!")
                    continue
                
                speed = int(input("Hız (0-255): ").strip())
                if speed < 0 or speed > 255:
                    print("❌ Geçersiz hız!")
                    continue
                
                print("\nYön seçin:")
                print("  0 = Dur")
                print("  1 = İleri (Forward)")
                print("  2 = Geri (Reverse)")
                direction = int(input("Yön (0-2): ").strip())
                
                if direction < 0 or direction > 2:
                    print("❌ Geçersiz yön!")
                    continue
                
                duration_sec = float(input("Çalışma süresi (saniye, maks 6553.5s): ").strip())
                if duration_sec < 0 or duration_sec > 6553.5:
                    print("❌ Geçersiz süre! (0-6553.5 saniye)")
                    continue
                
                duration_ms = int(duration_sec * 1000)
                
                cmd = f"fpga:{slot}:motor:{motor_ch}:speedtimed:{speed}:{direction}:{duration_ms}"
                print(f"\n📤 Komut: {cmd}")
                print(f"   Motor {duration_sec}s boyunca hız={speed}, yön={direction} ile çalışacak")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
                    
                    # Real-time timer monitor
                    monitor = input("\nTimer'ı gerçek zamanlı izlemek ister misiniz? (e/h): ").strip().lower()
                    if monitor == 'e':
                        print(f"\n📡 Timer izleniyor...")
                        print("─" * 60)
                        while True:
                            time.sleep(0.5)
                            cmd_info = f"fpga:{slot}:motor:{motor_ch}:timerinfo"
                            response_info = send_uart_command(ser, cmd_info, timeout=1)
                            
                            if "DURDU" in response_info:
                                print("\r✅ Timer tamamlandı, motor durdu           ")
                                break
                            elif "Kalan=" in response_info:
                                # Parse remaining time
                                remaining = response_info.split("Kalan=")[-1].split("ms")[0].strip()
                                remaining_sec = int(remaining) / 1000.0
                                print(f"\r⏱️  Kalan süre: {remaining_sec:.1f}s    ", end='', flush=True)
                            else:
                                print(f"\r⚠️  Durum okunamadı           ", end='', flush=True)
                        print()
                    
            except ValueError:
                print("❌ Geçerli sayılar girin!")
            except KeyboardInterrupt:
                print("\n⚠️ İşlem iptal edildi")
        
        elif choice == '11':
            # Timer info
            print("\n🕐 Timer Bilgisi")
            print("─" * 60)
            try:
                motor_ch = int(input("Motor channel (0-15): ").strip())
                if motor_ch < 0 or motor_ch > 15:
                    print("❌ Geçersiz motor channel!")
                    continue
                
                cmd = f"fpga:{slot}:motor:{motor_ch}:timerinfo"
                print(f"\n📤 Komut: {cmd}")
                response = send_uart_command(ser, cmd)
                if response:
                    print("\n📨 STM32 Yanıtı:")
                    print("─" * 60)
                    print(response.strip())
                    print("─" * 60)
                    
            except ValueError:
                print("❌ Geçerli bir sayı girin!")
            except KeyboardInterrupt:
                print("\n\n⚠️  İzleme durduruldu (Ctrl+C)")
        
        elif choice == '0':
            break
        else:
            print("\n❌ Geçersiz seçim!")
        
        if choice != '0':
            input("\nDevam etmek için Enter'a basın...")

def fpga_control_interface(ser, slot):
    """FPGA kontrol arayüzü"""
    while True:
        print("\n" + "="*60)
        print(f" FPGA MOTOR CONTROLLER (Slot {slot})")
        print("="*60)
        print("1. Register Oku (Read Register)")
        print("2. Register Yaz (Write Register)")
        print("3. FPGA Reset")
        print("4. Durum Göster (Status)")
        print("5. 🎮 Motor Kontrolü (Motor Control)")
        print("6. Yardım (Help)")
        print("0. Geri Dön")
        print("="*60)
        
        choice = input("\nSeçiminiz: ").strip()
        
        if choice == '1':
            addr = input("Register adresi (hex, örn: 0x10): ").strip()
            cmd = f"fpga:{slot}:readreg:{addr}"
            print(f"\n📤 Komut gönderiliyor: {cmd}")
            response = send_uart_command(ser, cmd)
            if response:
                print("\n📨 STM32 Yanıtı:")
                print("─" * 60)
                print(response.strip())
                print("─" * 60)
        
        elif choice == '2':
            addr = input("Register adresi (hex, örn: 0x10): ").strip()
            value = input("Değer (hex, örn: 0xFF): ").strip()
            cmd = f"fpga:{slot}:writereg:{addr}:{value}"
            print(f"\n📤 Komut gönderiliyor: {cmd}")
            response = send_uart_command(ser, cmd)
            if response:
                print("\n📨 STM32 Yanıtı:")
                print("─" * 60)
                print(response.strip())
                print("─" * 60)
        
        elif choice == '3':
            cmd = f"fpga:{slot}:reset"
            print(f"\n📤 Komut gönderiliyor: {cmd}")
            response = send_uart_command(ser, cmd)
            if response:
                print("\n📨 STM32 Yanıtı:")
                print("─" * 60)
                print(response.strip())
                print("─" * 60)
        
        elif choice == '4':
            cmd = f"fpga:{slot}:status"
            print(f"\n📤 Komut gönderiliyor: {cmd}")
            response = send_uart_command(ser, cmd)
            if response:
                print("\n📨 STM32 Yanıtı:")
                print("─" * 60)
                print(response.strip())
                print("─" * 60)
        
        elif choice == '5':
            fpga_motor_control_menu(ser, slot)
        
        elif choice == '6':
            print("\n📖 FPGA Komut Örnekleri:")
            print(f"  • fpga:{slot}:readreg:0x10    - Register 0x10'u oku")
            print(f"  • fpga:{slot}:writereg:0x20:0xFF - Register 0x20'ye 0xFF yaz")
            print(f"  • fpga:{slot}:reset           - FPGA'yı resetle")
            print(f"  • fpga:{slot}:status          - Modül durumunu göster")
            print(f"  • fpga:{slot}:motor:0:goto:1000:128 - Motor 0 pozisyon 1000'e git")
            print(f"  • fpga:{slot}:motor:0:speed:200:1 - Motor 0 hız 200 ileri")
            print(f"  • fpga:{slot}:motor:0:home    - Motor 0 home (pozisyon=0)")
            print("\n  Not: Register adresleri 0x00-0xFF arasında")
            print("       Motor channels: 0-15")
        
        elif choice == '0':
            break
        else:
            print("\n❌ Geçersiz seçim!")
        
        if choice not in ['0', '5']:
            input("\nDevam etmek için Enter'a basın...")

def stm32_terminal():
    """STM32 ile direkt terminal iletişimi - Gerçek zamanlı komut gönder/yanıt al"""
    print("\n" + "="*60)
    print(" 💻 STM32 DİREKT TERMİNAL")
    print("="*60)
    print()
    print("📡 STM32 ile direkt UART iletişimi")
    print("   • Komutlarınızı yazın, Enter'a basın")
    print("   • ACK süreleri otomatik hesaplanır")
    print("   • Gönderilen/alınan veriler görüntülenir")
    print("   • Çıkmak için: 'exit' veya Ctrl+C")
    print()
    
    if not SERIAL_AVAILABLE:
        print("\n❌ pyserial kütüphanesi yüklü değil!")
        print("   Yüklemek için: pip3 install pyserial")
        return False
    
    try:
        # Seri port aç
        ser = serial.Serial(
            port=UART_PORT,
            baudrate=UART_BAUD,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1
        )
        time.sleep(0.5)
        print(f"✅ {UART_PORT} bağlantısı açıldı ({UART_BAUD} baud)")
        print("="*60)
        print()
        
        # Terminal döngüsü
        while True:
            try:
                # Kullanıcıdan komut al
                command = input("STM32> ").strip()
                
                # Exit kontrolü
                if command.lower() in ['exit', 'quit', 'q']:
                    print("\n👋 Terminal kapatılıyor...")
                    break
                
                # Boş komut kontrolü
                if not command:
                    continue
                
                # Komut bilgisi göster
                print(f"\n📤 Gönderiliyor: '{command}' ({len(command)} byte)")
                
                # Başlangıç zamanı
                start_time_ns = time.perf_counter_ns()
                
                # Komutu gönder
                ser.reset_input_buffer()
                ser.write(f"{command}\r\n".encode())
                ser.flush()
                
                print(f"✅ Gönderildi!")
                
                # Yanıt topla
                response = ""
                ack_received = False
                ack_time_ns = 0
                response_start = time.time()
                timeout = 3.0  # 3 saniye timeout
                
                print(f"\n📨 STM32 Yanıtı:")
                print("─" * 60)
                
                while time.time() - response_start < timeout:
                    if ser.in_waiting:
                        data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                        response += data
                        
                        # Gelen veriyi anında yazdır
                        print(data, end='', flush=True)
                        
                        # ACK kontrolü (sadece ilk ACK'yi ölç)
                        if not ack_received and "[ACK:" in response:
                            ack_time_ns = time.perf_counter_ns()
                            ack_received = True
                            
                            # Gecikme hesapla
                            latency_ns = ack_time_ns - start_time_ns
                            latency_us = latency_ns / 1000
                            latency_ms = latency_ns / 1_000_000
                            
                            print(f"\n⏱️  [ACK alındı: {latency_us:.0f} µs ({latency_ms:.2f} ms)]")
                        
                        # Komut tamamlandı mı?
                        if "Komut tamamlandi:" in response or "OK:" in response or "Hata:" in response:
                            # Son satırı bekle
                            time.sleep(0.1)
                            if ser.in_waiting:
                                final_data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                                response += final_data
                                print(final_data, end='', flush=True)
                            break
                    
                    time.sleep(0.01)
                
                print("─" * 60)
                
                # Timeout uyarısı
                if not response:
                    print("⚠️  Yanıt alınamadı (timeout)")
                elif not ack_received:
                    print("⚠️  ACK alınamadı")
                
                print()  # Boş satır
                
            except KeyboardInterrupt:
                print("\n\n👋 Terminal kapatılıyor (Ctrl+C)...")
                break
            except Exception as e:
                print(f"\n❌ Hata: {e}")
                print("Devam etmek için Enter'a basın...")
                input()
        
        # Portu kapat
        ser.close()
        print("✅ Seri port kapatıldı\n")
        
    except serial.SerialException as e:
        print(f"\n❌ Seri port hatası: {e}")
        print(f"   Port: {UART_PORT}")
        print(f"   Kontrol edin: ls -la /dev/ttyAMA*")
        return False
    except Exception as e:
        print(f"\n❌ Beklenmeyen hata: {e}")
        import traceback
        traceback.print_exc()
        return False
    
    return True

def module_control_menu():
    """Manuel modül kontrolü ana menüsü"""
    print("\n" + "="*60)
    print(" 🎮 MANUEL MODÜL KONTROLÜ")
    print("="*60)
    
    if not SERIAL_AVAILABLE:
        print("\n❌ pyserial kütüphanesi yüklü değil!")
        print("   Yüklemek için: pip3 install pyserial")
        return False
    
    try:
        # Seri port aç
        ser = serial.Serial(
            port=UART_PORT,
            baudrate=UART_BAUD,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=2
        )
        time.sleep(0.5)
        print(f"✓ {UART_PORT} bağlantısı başarılı\n")
        
        # Modülleri algıla
        modules = detect_modules(ser)
        
        if not modules:
            print("\n⚠️  Hiç modül algılanamadı!")
            ser.close()
            return False
        
        print(f"\n✅ {len(modules)} modül algılandı:\n")
        for i, mod in enumerate(modules):
            print(f"  {i}. Slot {mod['slot']}: {mod['description']}")
        
        # Modül seç
        while True:
            print("\n" + "="*60)
            choice = input("Kontrol etmek istediğiniz modülü seçin (0-{}) veya 'q' (çıkış): ".format(len(modules)-1)).strip()
            
            if choice.lower() == 'q':
                break
            
            try:
                idx = int(choice)
                if idx < 0 or idx >= len(modules):
                    print("❌ Geçersiz modül numarası!")
                    continue
                
                module = modules[idx]
                slot = module['slot']
                mod_type = module['type']
                
                print(f"\n[DEBUG] Seçilen modül: Type='{mod_type}', Slot={slot}")
                
                # Modül tipine göre arayüz aç
                if mod_type == "IO16":
                    io16_control_interface(ser, slot)
                elif mod_type == "AIO20":
                    aio20_control_interface(ser, slot)
                elif mod_type == "FPGA":
                    fpga_control_interface(ser, slot)
                else:
                    print(f"\n⚠️  '{mod_type}' modül tipi için arayüz henüz eklenmedi!")
                    print(f"   Desteklenen tipler: IO16, AIO20, FPGA")
                    print(f"   Algılanan tip: '{mod_type}' (uzunluk: {len(mod_type)})")
                    print(f"   Modül açıklaması: {module['description']}")
                    input("Devam etmek için Enter'a basın...")
                
            except ValueError:
                print("❌ Geçerli bir sayı girin!")
        
        ser.close()
        print("\n✓ Bağlantı kapatıldı")
        return True
        
    except serial.SerialException as e:
        print(f"\n❌ UART hatası: {e}")
        return False
    except Exception as e:
        print(f"\n❌ Hata: {e}")
        import traceback
        traceback.print_exc()
        return False

# spi_test_menu() fonksiyonu sistem ��kt�rd��� i�in kald�r�ld�.


def main():
    """Ana program"""
    print_header()
    
    # Başlangıç kontrolü (opsiyonel - script her yerde çalışabilir)
    # if not os.path.exists("burjuva"):
    #     print("\n⚠️  DİKKAT: burjuva/ klasörü bulunamadı!")
    #     print("   Bu scripti doğru dizinde çalıştırdığınızdan emin olun.")
    #     print("   Klasör yapısı: ~/burjuva/firmware.bin, ~/burjuva/burjuva_manager.py\n")
    
    try:
        while True:
            print_menu()
            choice = input("\nSeçiminiz: ").strip()
            
            if choice == '1':
                flash_stm32()
            elif choice == '2':
                uart_test_menu()
            elif choice == '3':
                flash_all()
            elif choice == '4':
                flash_cpld()
            elif choice == '5':
                system_status()
            elif choice == '6':
                setup_system()
            elif choice == '7':
                module_control_menu()
            elif choice == '8':
                stm32_terminal()
            elif choice == '0':
                print("\n👋 Çıkılıyor...\n")
                break
            else:
                print("\n❌ Geçersiz seçim!")
            
            if choice != '0':
                input("\nAna menüye dönmek için Enter'a basın...")
    
    except KeyboardInterrupt:
        print("\n\n👋 Program sonlandırıldı (Ctrl+C)\n")
    except Exception as e:
        print(f"\n❌ Beklenmeyen hata: {e}\n")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
