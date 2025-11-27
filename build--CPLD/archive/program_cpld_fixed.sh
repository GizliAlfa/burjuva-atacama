#!/bin/bash
# ============================================================
# PILOT CPLD PROGRAMMER - DÜZELTILMIŞ VERSIYONU
# Device: Altera MAX V 5M80ZT100C5
# ============================================================

set -e  # Hata durumunda dur

# Renkler
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}"
echo "============================================================"
echo "  PILOT CPLD PROGRAMMER (FIXED)"
echo "  Device: Altera MAX V 5M80ZT100C5"
echo "============================================================"
echo -e "${NC}"

# Dosya kontrolü
if [ ! -f "/tmp/cpld.svf" ]; then
    echo -e "${RED}✗ SVF dosyası bulunamadı: /tmp/cpld.svf${NC}"
    exit 1
fi
echo -e "${GREEN}✓ SVF file found: /tmp/cpld.svf${NC}"

# OpenOCD kontrolü
if ! command -v openocd &> /dev/null; then
    echo -e "${RED}✗ OpenOCD bulunamadı${NC}"
    exit 1
fi
echo -e "${GREEN}✓ OpenOCD found: $(which openocd)${NC}"

# Config dosyası kontrolü
if [ ! -f "/tmp/cpld_program_correct.cfg" ]; then
    echo -e "${RED}✗ Config dosyası bulunamadı: /tmp/cpld_program_correct.cfg${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Config file found: /tmp/cpld_program_correct.cfg${NC}"

echo ""
echo -e "${BLUE}"
echo "============================================================"
echo "  JTAG BAĞLANTISI (DÜZELTILMIŞ)"
echo "============================================================"
echo -e "${NC}"
echo "GPIO Pin Mapping (Fiziksel Pinlerden):"
echo -e "${YELLOW}  Pin 15 (GPIO 22) -> TMS  (Altera PIN 33)${NC}"
echo -e "${YELLOW}  Pin 22 (GPIO 27) -> TDI  (Altera PIN 34) ← DÜZELTILDI${NC}"
echo -e "${YELLOW}  Pin 23 (GPIO 24) -> TDO  (Altera PIN 36)${NC}"
echo -e "${YELLOW}  Pin 16 (GPIO 23) -> TCK  (Altera PIN 35) ← DÜZELTILDI${NC}"
echo -e "${YELLOW}  GND              -> GND${NC}"
echo ""
echo "Press ENTER to start programming (or Ctrl+C to cancel)..."
read

echo ""
echo -e "${BLUE}"
echo "============================================================"
echo "  PROGRAMMING CPLD"
echo "============================================================"
echo -e "${NC}"

# GPIO cleanup (eski kernel'de gerekli olabilir)
for pin in 22 23 24 27; do
    echo $pin > /sys/class/gpio/unexport 2>/dev/null || true
done

# OpenOCD ile programla
if openocd -f /tmp/cpld_program_correct.cfg -c "svf /tmp/cpld.svf; shutdown"; then
    echo ""
    echo -e "${GREEN}"
    echo "============================================================"
    echo "  ✓ PROGRAMMING SUCCESSFUL"
    echo "============================================================"
    echo -e "${NC}"
    echo ""
    echo "Next steps:"
    echo "  1. Test CPLD communication via SPI"
    echo "  2. Run hardware detection script"
    echo "  3. Check module connections"
    exit 0
else
    echo ""
    echo -e "${RED}"
    echo "============================================================"
    echo "  ✗ PROGRAMMING FAILED"
    echo "============================================================"
    echo -e "${NC}"
    echo ""
    echo "Possible causes:"
    echo "  1. JTAG pins not connected correctly (CHECK PIN 22 & 16!)"
    echo "  2. CPLD not powered"
    echo "  3. Wrong CPLD device ID"
    echo "  4. SVF file corrupt"
    echo ""
    echo "Check connections and try again"
    exit 1
fi
