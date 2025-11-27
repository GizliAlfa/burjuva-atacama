# Burjuva-Atacama Qt Kontrol Paneli

STM32 tabanlı Burjuva-Atacama endüstriyel I/O sistemi için Qt5 kontrol panel uygulaması.

## Özellikler

- ✅ Otomatik modül algılama (IO16, AIO20)
- ✅ 4 slot görüntüleme ve dinamik UI oluşturma
- ✅ IO16 dijital modül kontrolü
  - 4 grup halinde (her grup 4 pin)
  - Input/Output yön kontrolü
  - Pin bazında okuma/yazma
  - Her değer için ACK gösterimi
- ✅ AIO20 analog modül kontrolü
  - 12 giriş kanalı (0-11)
  - 8 çıkış kanalı (12-19)
  - Voltaj/akım desteği (0-10V, 4-20mA)
  - Slider ve spinbox ile değer ayarlama
- ✅ Ayarlanabilir güncelleme hızı (cycle time)
- ✅ Gerçek zamanlı UART iletişimi (115200 baud)
- ✅ Modern Qt5 arayüzü

## Gereksinimler

### Derleme İçin
- CMake 3.16+
- Qt5 (Core, Widgets, SerialPort)
- C++17 uyumlu derleyici
  - Linux: GCC 7+ veya Clang 5+
  - Windows: MSVC 2017+ veya MinGW 7+
  - Raspberry Pi: GCC 7+ (apt ile kurulabilir)

### Çalışma İçin
- STM32 firmware (stm32-firmware-beta dizininde)
- USB-UART dönüştürücü veya doğrudan UART bağlantısı
- En az bir IO16 veya AIO20 modülü

## Kurulum

### Raspberry Pi OS

```bash
# Qt5 ve gerekli paketleri kur
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    qt5-default \
    qtbase5-dev \
    libqt5serialport5-dev \
    git

# Kaynak kodunu derle
cd burjuva-qt-ui
mkdir build
cd build
cmake ..
make -j$(nproc)

# Çalıştır
./burjuva-ui
```

### Ubuntu/Debian

```bash
sudo apt install -y \
    build-essential \
    cmake \
    qtbase5-dev \
    libqt5serialport5-dev

cd burjuva-qt-ui
mkdir build && cd build
cmake ..
make -j$(nproc)
./burjuva-ui
```

### Windows

1. Qt5 kurulumu:
   - https://www.qt.io/download adresinden Qt 5.15 LTS indirin
   - Qt Creator ile açın veya manuel derleme yapın

2. CMake ile derleme:
```powershell
cd burjuva-qt-ui
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
burjuva-ui.exe
```

## Kullanım

### İlk Başlatma

1. **Port Seçimi**: Açılır menüden STM32'nin bağlı olduğu COM/ttyUSB portunu seçin
2. **Bağlan**: "Bağlan" butonuna tıklayın
3. **Otomatik Algılama**: Program otomatik olarak `modul-algila` komutu gönderir
4. **Slot Görüntüsü**: Sağ üstte 4 slot görüntülenir (yeşil = modül var)

### IO16 Modülü Kontrolü

1. IO16 modülü olan slot butonuna tıklayın
2. 4 grup görüntülenir (Grup 0-3)
3. **Yön Değiştirme**:
   - "Input" butonu: Okuma modu (pin değerleri otomatik güncellenir)
   - "Output" butonu: Yazma modu (ON/OFF toggle butonları görünür)
4. **Pin Kontrolü**:
   - Input modda: Değerler (0/1) otomatik gösterilir
   - Output modda: ON/OFF butonları ile kontrol edin
5. **ACK Gösterimi**: Her pin altında son işlem sonucu gösterilir

### AIO20 Modülü Kontrolü

1. AIO20 modülü olan slot butonuna tıklayın
2. **Giriş Kanalları (0-11)**:
   - Otomatik güncellenen voltaj/akım değerleri
   - Yeşil renk = aktif değer (>0.1V)
   - Mod bilgisi (0-10V, 4-20mA vb.)
3. **Çıkış Kanalları (12-19)**:
   - Slider veya spinbox ile değer ayarlayın
   - "Uygula" butonuna tıklayın
   - Turuncu renk = çıkış kanalı
4. **ACK Gösterimi**: Her kanal altında işlem sonucu

### Ayarlar

- **Cycle Time**: Güncelleme hızını 10-5000ms aralığında ayarlayın
  - Varsayılan: 100ms
  - Hızlı yanıt için: 50ms
  - Yavaş haberleşme için: 500ms+

## UART Komut Protokolü

Uygulama aşağıdaki komutları kullanır:

### Modül Algılama
```
modul-algila
```
Yanıt örneği:
```
Slot 0: IO16_DIJITAL (UID: 5A3B1C9D)
Slot 1: AIO20_ANALOG (UID: 8F2E4A6C)
Slot 2: BOS
Slot 3: IO16_DIJITAL (UID: 2D9F6B1A)
```

### IO16 Komutları
```
io16:slot0:grup0:direction:input     # Grup 0'ı input yap
io16:slot0:grup1:direction:output    # Grup 1'i output yap
io16:slot0:pin5:1                    # Pin 5'i HIGH yap
io16:slot0:pin5:0                    # Pin 5'i LOW yap
io16:slot0:grup2:oku                 # Grup 2'yi oku
```

### AIO20 Komutları
```
aio20:slot1:kanal5:oku               # Kanal 5'i oku
aio20:slot1:kanal15:set:5.25         # Kanal 15'e 5.25V yaz
aio20:slot1:kanal0:mode:0-10V        # Kanal 0 modunu ayarla
```

## Proje Yapısı

```
burjuva-qt-ui/
├── CMakeLists.txt              # Derleme yapılandırması
├── README.md                   # Bu dosya
└── src/
    ├── main.cpp                # Giriş noktası
    ├── mainwindow.h/cpp        # Ana pencere
    ├── serialcontroller.h/cpp  # UART iletişim
    ├── moduledetector.h/cpp    # Modül algılama
    ├── moduletypes.h           # Veri yapıları
    ├── io16widget.h/cpp        # IO16 arayüzü
    ├── io16group.h/cpp         # IO16 grup widget
    ├── aio20widget.h/cpp       # AIO20 arayüzü
    └── aio20channel.h/cpp      # AIO20 kanal widget
```

## Geliştirici Notları

### Kod Mimarisi

1. **SerialController**: QSerialPort üzerinden asenkron haberleşme, komut kuyruğu yönetimi
2. **ModuleDetector**: `modul-algila` çıktısını parse eder, ModuleInfo listesi oluşturur
3. **MainWindow**: Ana pencere, slot görüntüleme, widget yönetimi
4. **IO16Widget/Group**: Dijital modül UI, grup bazlı yön kontrolü
5. **AIO20Widget/Channel**: Analog modül UI, kanal bazlı değer kontrolü

### Signal/Slot Yapısı

```cpp
// Serial → ModuleDetector
SerialController::dataReceived() → ModuleDetector::handleDataReceived()

// ModuleDetector → MainWindow
ModuleDetector::detectionCompleted() → MainWindow::onDetectionCompleted()

// MainWindow → Widgets
MainWindow::switchToModule() → IO16Widget/AIO20Widget gösterilir

// Widgets → Serial
IO16Group::pinToggled() → SerialController::sendCommand()
AIO20Channel::valueChanged() → SerialController::sendCommand()
```

### Özelleştirme

#### Cycle Time Değiştirme
```cpp
m_serial->setCycleTime(50);  // 50ms cycle time
```

#### Yeni Modül Tipi Ekleme
1. `moduletypes.h` içinde yeni enum ekle
2. `ModuleDetector::parseModuleType()` güncelle
3. Yeni widget sınıfı oluştur
4. `MainWindow::switchToModule()` içinde case ekle

#### Port Ayarları Değiştirme
```cpp
m_serial->connectToPort(portName, 9600);  // 9600 baud
```

## Sorun Giderme

### Port Bulunamıyor

**Linux**:
```bash
# Port izinlerini kontrol et
ls -l /dev/ttyUSB*
# Kullanıcıyı dialout grubuna ekle
sudo usermod -a -G dialout $USER
# Oturumu yeniden başlat
```

**Windows**:
- Cihaz Yöneticisi'nden COM portunu kontrol edin
- Driver yüklü mü kontrol edin (CH340, FTDI vb.)

### Modüller Algılanmıyor

1. UART bağlantısını kontrol edin (TX↔RX, GND)
2. STM32 firmware'in yüklü olduğundan emin olun
3. Modüllerin doğru takılı olduğunu kontrol edin
4. Terminal ile `modul-algila` komutunu manuel test edin:
```bash
screen /dev/ttyUSB0 115200
modul-algila
```

### Derleme Hataları

**Qt5 bulunamadı**:
```bash
# Qt5 kurulum yolunu belirt
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/5.15.2/gcc_64 ..
```

**SerialPort bulunamadı**:
```bash
sudo apt install libqt5serialport5-dev
```

### Çalışma Zamanı Hataları

**Cannot open /dev/ttyUSB0**:
- İzin hatası: `sudo chmod 666 /dev/ttyUSB0`
- Kalıcı çözüm: Kullanıcıyı dialout grubuna ekleyin

**Widget oluşturulamıyor**:
- Log çıktısını kontrol edin (`qDebug` mesajları)
- Slot numarasının 0-3 aralığında olduğundan emin olun

## Lisans

Bu proje Burjuva endüstriyel otomasyon sistemleri için geliştirilmiştir.

## Versiyon Geçmişi

### v1.0.0 (2024)
- İlk sürüm
- IO16 ve AIO20 modül desteği
- Otomatik modül algılama
- Qt5 tabanlı modern arayüz
- Çapraz platform desteği (Linux, Windows, Raspberry Pi)

## İletişim

Teknik destek ve sorular için proje dokümantasyonuna bakın.
