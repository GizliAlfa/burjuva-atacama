#!/bin/bash
# CPLD Hızlı Programlama Scripti
# Kullanım: ./quick_program.sh

echo "════════════════════════════════════════════════════════"
echo "  CPLD HIZLI PROGRAMLAMA"
echo "════════════════════════════════════════════════════════"
echo ""

# Dosya kontrolü
if [ ! -f "/tmp/cpld.svf" ]; then
    echo "❌ HATA: /tmp/cpld.svf bulunamadı!"
    exit 1
fi

if [ ! -f "/tmp/openocd_cpld.cfg" ]; then
    echo "❌ HATA: /tmp/openocd_cpld.cfg bulunamadı!"
    exit 1
fi

echo "✅ Dosyalar hazır"
echo ""

# OpenOCD versiyonu
echo "🔧 OpenOCD Versiyonu:"
openocd --version 2>&1 | head -1
echo ""

# Programlama başlat
echo "🚀 CPLD programlanıyor..."
echo ""
echo "────────────────────────────────────────────────────────"

START_TIME=$(date +%s)

sudo openocd -f /tmp/openocd_cpld.cfg \
  -c 'svf /tmp/cpld.svf; shutdown' 2>&1

EXIT_CODE=$?

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

echo "────────────────────────────────────────────────────────"
echo ""

# Sonuç
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ BAŞARILI! CPLD programlandı"
    echo "⏱️  Süre: ${DURATION} saniye"
else
    echo "❌ HATA! Programlama başarısız (exit code: $EXIT_CODE)"
    exit $EXIT_CODE
fi

echo ""
echo "════════════════════════════════════════════════════════"
echo ""

# SPI test önerisi
echo "💡 Test için:"
echo "   python3 /tmp/quick_spi_test.py"
echo ""
