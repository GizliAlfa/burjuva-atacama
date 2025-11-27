# STM32F103 UART Echo Firmware

## 📋 Özet
STM32F103RCT6 minimal UART echo firmware (SPL tabanlı)

## ✅ Durum
- ✅ **Derleme**: Başarılı (1.5KB firmware)
- ⏳ **Test**: RPi'de test edilecek
- 📅 **Tarih**: 16 Kasım 2025

## 🚀 Hızlı Kullanım

### 1. Derleme (Windows)
```powershell
.\build.bat
```

### 2. RPi'ye Yükleme
```bash
scp build/firmware.bin pi@raspberrypi.local:~/
python3 burjuva_flash.py --stm32_only
```

### 3. Test
```bash
python3 test_stm32_uart.py
```

## 📝 Özellikler
- **UART**: 115200 baud, PA9/PA10
- **LED**: PC13 yanıp söner
- **Fonksiyon**: Echo (aldığı her byte'ı geri gönderir)

## 🔧 Sonraki Adımlar
1. RPi'de test et
2. Komut protokolü ekle
3. Motor kontrol ekle

Detaylı bilgi için kodlara bakın!
