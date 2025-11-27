# ✅ CPLD Hazır - Quartus Lite Bekliyor

## 📦 Durum

- ✅ Verilog kaynak kodları hazır (top.v, rpi.v, testin.v, testout.v)
- ✅ Quartus project dosyaları hazır (cpld.qsf, cpld.qpf)
- ✅ OpenOCD programlama araçları hazır
- ⏳ **Quartus Lite kurulum bekleniyor**

## 🎯 Quartus Lite Kurulunca Yapılacaklar

### 1. Derleme (30 saniye)

```powershell
cd cpld-build

# VS Code'da: Ctrl+Shift+B
# veya Terminal'de:
quartus_sh --flow compile cpld
```

**Çıktı**: `output_files/cpld.svf`

### 2. Raspberry Pi'ye Gönder (5 saniye)

```powershell
scp output_files/cpld.svf burjuva@192.168.1.22:/tmp/
scp cpld_program.cfg burjuva@192.168.1.22:/tmp/
scp program_cpld.sh burjuva@192.168.1.22:/tmp/
```

### 3. CPLD Programla (10 saniye)

```bash
ssh burjuva@192.168.1.22
cd /tmp
chmod +x program_cpld.sh
sudo ./program_cpld.sh
```

### 4. Test Et

```bash
python3 /tmp/cpld_spi_test.py
# Başarılı ise: MISO'dan cevap gelecek
```

## 📁 Klasör Yapısı

```
cpld-build/
├── *.v files         ← Verilog kaynak kodları
├── cpld.qsf/qpf      ← Quartus project
├── *.cfg/*.sh        ← Programlama araçları
├── BUILD.md          ← Detaylı talimatlar
└── .vscode/          ← VS Code entegrasyonu
```

## ⏰ Toplam Süre

- Quartus kurulum: ~15-20 dakika
- İlk derleme: ~60 saniye
- Sonraki derlemeler: ~30 saniye
- Programlama: ~10 saniye

## 🎉 Sonuç

Quartus Lite kurulunca **hazırsınız**! 

`Ctrl+Shift+B` → Bekle → Gönder → Programla → Test! 🚀

---

**Şu an**: Quartus Lite kurulumunu bekliyor... ☕
