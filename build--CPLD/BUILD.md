# 🚀 CPLD Derleme - Quartus Lite ile

## 📦 Kurulum

### 1. Quartus Lite İndir

**Link**: https://www.intel.com/content/www/us/en/software-kit/825278/intel-quartus-prime-lite-edition-design-software-version-23-1-1-for-windows.html

**Kurulum**:
- Quartus Prime Lite Edition ✓
- MAX V device support ✓
- ModelSim (opsiyonel, simülasyon için)

**Boyut**: ~2-3 GB (sadece MAX V)

### 2. PATH Ayarı

Quartus kurulduktan sonra:

```powershell
# Windows ortam değişkenlerine ekle:
C:\intelFPGA_lite\23.1\quartus\bin
```

veya geçici olarak:

```powershell
$env:PATH += ";C:\intelFPGA_lite\23.1\quartus\bin"
```

### 3. Doğrulama

```powershell
quartus_sh --version
# Quartus Prime Shell bilgilerini görmeli
```

## 🎯 Derleme

### VS Code İçinde (Önerilen)

```
1. VS Code'da cpld-build/ klasörünü aç
2. Ctrl+Shift+B  (veya Terminal → Run Build Task)
3. Bekle (~30-60 saniye)
4. ✓ output_files/cpld.svf oluştu!
```

### Terminal İle

```powershell
cd cpld-build
quartus_sh --flow compile cpld
```

**Çıktılar**:
- `output_files/cpld.pof` - Programming Object File
- `output_files/cpld.jam` - JTAG programming file
- `output_files/cpld.svf` - Serial Vector Format (kullanacağımız)

## 📤 Raspberry Pi'ye Gönder

```powershell
# SVF dosyasını gönder
scp output_files/cpld.svf burjuva@192.168.1.22:/tmp/

# OpenOCD config gönder
scp cpld_program.cfg burjuva@192.168.1.22:/tmp/

# Programlama scripti gönder
scp program_cpld.sh burjuva@192.168.1.22:/tmp/
chmod +x /tmp/program_cpld.sh
```

## 🔌 CPLD Programla

```bash
# SSH ile bağlan
ssh burjuva@192.168.1.22

# Programla
cd /tmp
sudo ./program_cpld.sh
```

## ✅ Test

CPLD programlandıktan sonra:

```bash
# SPI iletişimini test et
python3 /tmp/cpld_spi_test.py

# Modülleri tara
python3 /tmp/hardware_detection.py
```

**Başarılı ise**:
- MISO pini cevap verecek (0x00 değil)
- Modül EEPROM'ları okunabilecek
- I2C cihazlar görünecek

## 🎨 VS Code Kısayolları

- **Ctrl+Shift+B** - CPLD'yi derle
- **F1 → Tasks: Run Task** - Tüm görevleri göster
  - CPLD: Compile
  - CPLD: Upload to Raspberry Pi
  - CPLD: Program via SSH
  - CPLD: Full Build & Program (hepsi otomatik)
  - CPLD: Clean

## 📁 Dosya Yapısı

```
cpld-build/
├── top.v              # Ana CPLD modülü
├── rpi.v              # RPI SPI bridge
├── testin.v           # Input routing
├── testout.v          # Output routing
├── cpld.qsf           # Quartus settings
├── cpld.qpf           # Quartus project
├── cpld_program.cfg   # OpenOCD config
├── program_cpld.sh    # Programlama scripti
├── .vscode/
│   ├── tasks.json     # VS Code tasks
│   └── settings.json  # VS Code settings
└── output_files/      # Derleme çıktıları (oluşacak)
    ├── cpld.svf       # ← Kullanacağımız
    ├── cpld.jam
    └── cpld.pof
```

## ⚠️ Sorun Giderme

### "quartus_sh not recognized"

PATH'e Quartus ekleyin:

```powershell
$env:PATH += ";C:\intelFPGA_lite\23.1\quartus\bin"
```

### Derleme hatası

```powershell
# Temizle ve tekrar dene
quartus_sh --flow clean cpld
quartus_sh --flow compile cpld
```

### JTAG bağlantı hatası

- JTAG pinleri doğru bağlı mı kontrol et
- CPLD power alıyor mu kontrol et
- OpenOCD config'de device ID doğru mu: `0x020a10dd`

---

**Quartus Lite kurulduktan sonra**: `Ctrl+Shift+B` → Bekle → SVF gönder → Programla! 🎉
