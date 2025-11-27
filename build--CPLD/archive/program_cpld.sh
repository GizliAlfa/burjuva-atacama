#!/bin/bash
# ============================================================================
# CPLD Programming Script for Raspberry Pi
# Bu script CPLD'yi OpenOCD ile programlar
# ============================================================================

set -e

echo "============================================================"
echo "  PILOT CPLD PROGRAMMER"
echo "  Device: Altera MAX V 5M80ZT100C5"
echo "============================================================"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Error: Please run as root (sudo)"
    exit 1
fi

# Check if OpenOCD is installed
if ! command -v openocd &> /dev/null; then
    echo "Error: OpenOCD not found"
    echo "Install: sudo apt install openocd"
    exit 1
fi

# Check if SVF file exists
SVF_FILE="/tmp/cpld.svf"
if [ ! -f "$SVF_FILE" ]; then
    echo "Error: SVF file not found: $SVF_FILE"
    echo ""
    echo "Please compile CPLD first and copy SVF file:"
    echo "  scp cpld-build/output_files/cpld.svf pi@192.168.1.22:/tmp/"
    exit 1
fi

echo "✓ OpenOCD found: $(which openocd)"
echo "✓ SVF file found: $SVF_FILE"
echo ""

# Check if config file exists
CONFIG_FILE="/tmp/cpld_program.cfg"
if [ ! -f "$CONFIG_FILE" ]; then
    echo "Warning: OpenOCD config not found at $CONFIG_FILE"
    echo "Creating default config..."
    
    cat > "$CONFIG_FILE" << 'EOF'
# OpenOCD configuration for MAX V CPLD via Raspberry Pi GPIO
interface bcm2835gpio
bcm2835gpio_peripheral_base 0xFE000000
bcm2835gpio_jtag_nums 22 25 24 23
adapter_khz 1000
jtag newtap maxv tap -irlen 10 -expected-id 0x020a10dd
init
scan_chain
svf /tmp/cpld.svf progress
shutdown
EOF
    
    echo "✓ Config created: $CONFIG_FILE"
fi

echo ""
echo "============================================================"
echo "  JTAG CONNECTION"
echo "============================================================"
echo ""
echo "GPIO Pin Mapping:"
echo "  GPIO 22 -> TMS"
echo "  GPIO 23 -> TDI"
echo "  GPIO 24 -> TDO"
echo "  GPIO 25 -> TCK"
echo "  GND     -> GND"
echo ""
echo "Press ENTER to start programming (or Ctrl+C to cancel)..."
read

echo ""
echo "============================================================"
echo "  PROGRAMMING CPLD..."
echo "============================================================"
echo ""

# Run OpenOCD
if openocd -f "$CONFIG_FILE"; then
    echo ""
    echo "============================================================"
    echo "  ✓ CPLD PROGRAMMED SUCCESSFULLY!"
    echo "============================================================"
    echo ""
    echo "Next steps:"
    echo "  1. Test SPI communication: python3 /tmp/cpld_spi_test.py"
    echo "  2. Scan for modules: python3 /tmp/hardware_detection.py"
    echo ""
else
    echo ""
    echo "============================================================"
    echo "  ✗ PROGRAMMING FAILED"
    echo "============================================================"
    echo ""
    echo "Possible causes:"
    echo "  1. JTAG pins not connected correctly"
    echo "  2. CPLD not powered"
    echo "  3. Wrong CPLD device ID"
    echo "  4. SVF file corrupt"
    echo ""
    echo "Check connections and try again"
    exit 1
fi
