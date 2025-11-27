# Burjuva Atacama - Modüler I/O Kontrol Sistemi# Burjuva Motor Control System

**6-Channel DC Motor Controller with FPGA Acceleration**  

**STM32F103 Tabanlı Endüstriyel I/O Kontrol Platformu**  **Date:** 13 Kasım 2025

**Tarih:** 27 Kasım 2025  

**Versiyon:** 2.0 (FPGA-free)---



---## 🎯 Overview



## 📋 Genel BakışBurjuva Motor, Raspberry Pi tabanlı endüstriyel motor kontrol sistemidir. iCE40 HX1K FPGA ile donanım hızlandırması ve STM32 mikrocontroller ile akıllı kontrol sağlar.



Burjuva Atacama, STM32F103RCT6 mikrocontroller üzerine inşa edilmiş modüler bir I/O kontrol sistemidir. 1-Wire modül algılama, SPI iletişimi ve UART komut arayüzü ile esnek ve genişletilebilir bir yapı sunar.### Key Features

- **6 Motor Kanalı** - Bağımsız DC motor kontrolü

### Temel Özellikler- **FPGA Acceleration** - 65.95 MHz'de PWM ve position tracking

- **Hall Sensor Feedback** - 24-bit pozisyon sayıcıları

✅ **Modüler Yapı** - Hot-swap modül desteği  - **SPI Communication** - 4.5 MHz STM32 ↔ FPGA

✅ **1-Wire Algılama** - Otomatik modül tanıma  - **UART Control** - 921600 baud debug/command interface (PA9/PA10)

✅ **SPI İletişim** - 4.5-9 MHz hızlı veri transferi  - **Analog I/O** - 8x input (0-10V, 4-20mA, PT100, TC-K) + 4x output (0-10V, 4-20mA)

✅ **UART Komut Arayüzü** - 115200 baud, kolay kontrol  - **Digital I/O** - 16x input (24V, switches, limit switches) + 16x output (relay, SSR, LED, solenoid)

✅ **3 Aktif Slot** - IO16 (2×), AIO20 (1×)  - **Real-time Control** - 93.75 kHz PWM frekansı

✅ **CPLD Multiplexer** - Akıllı CS ve MISO yönlendirme- **Interactive Shell** - UART komut satırı (motor + analog + digital I/O)

- **Safety Features** - Timeout, limit switch, fault detection, emergency stop

---

---

## 🔌 Desteklenen Modüller

## 📂 Project Structure

### IO16 - 16 Kanal Digital I/O

- **Chip:** iC-JX678```

- **Slots:** 0, 3burjuva-motor/

- **Özellikler:** ├── README.md              # Bu dosya

  - 16 GPIO (input/output yapılandırılabilir)├── firmware/              # STM32 firmware

  - Direction kontrolü (grup bazlı)│   ├── src/              # C kaynak kodları

  - Pull-up/down resistor desteği│   │   ├── main.c        # Ana program

- **SPI Hızı:** 4.5 MHz│   │   ├── fpga_driver.c # FPGA SPI driver

│   │   ├── motor.c       # Motor kontrol API

### AIO20 - 20 Kanal Analog I/O│   │   └── spi.c         # SPI HAL

- **Chip:** MAX11300 (PIXI)│   ├── inc/              # Header dosyaları

- **Slot:** 1│   │   ├── fpga_driver.h

- **Özellikler:**│   │   ├── motor.h

  - 12 Analog Input (0-10V, ±10V, 4-20mA)│   │   └── spi.h

  - 8 Analog Output (0-10V, ±10V, 4-20mA)│   ├── Makefile          # Build sistemi

  - 12-bit ADC/DAC çözünürlük│   └── stm32.ld          # Linker script

  - Dahili referans voltaj├── fpga/                  # FPGA bitstream

- **SPI Hızı:** 9.0 MHz│   └── burjuva-fpga.bin  # Compiled bitstream

├── scripts/               # Automation scripts

---│   ├── build.sh          # Full build (firmware + FPGA)

│   ├── program.sh        # RPI programlama

## 🏗️ Donanım Mimarisi│   └── test.sh           # Test otomasyonu

└── docs/                  # Dokümantasyon

```    ├── API.md            # Motor control API

┌─────────────────────────────────────────────────────┐    ├── HARDWARE.md       # Pin mapping & schematic

│              STM32F103RCT6 (72MHz)                  │    └── PROTOCOL.md       # SPI protocol spec

│  ┌──────────┐  ┌──────────┐  ┌──────────┐          │```

│  │   UART   │  │   SPI2   │  │ 1-Wire   │          │

│  │ PA9/PA10 │  │PB13/14/15│  │PC0/1/2/3 │          │---

│  └────┬─────┘  └────┬─────┘  └────┬─────┘          │

└───────┼─────────────┼─────────────┼────────────────┘## 🔧 Hardware

        │             │             │

        │             │             │ (Module Detection)### Components

        │             │             │- **MCU:** STM32F4 series (168 MHz)

        │             ▼             ▼- **FPGA:** Lattice iCE40 HX1K-CB132

        │    ┌─────────────────────────────┐- **Motor Drivers:** 3× TI DRV8434 (Dual H-Bridge)

        │    │   CPLD Multiplexer          │- **Motors:** 6× DC Motor with Hall sensors

        │    │  • CS Routing (4 slots)    │- **Host:** Raspberry Pi (any model with GPIO)

        │    │  • MISO Multiplexer        │

        │    └───┬─────┬─────┬─────┬──────┘### Pin Mapping

        │        │     │     │     │Detaylı pin mapping için: `docs/HARDWARE.md` ve `../fpga-build/PIN_MAPPING.md`

        │     Slot 0 Slot 1 Slot 2 Slot 3

        │        │     │     │     │**STM32 ↔ FPGA SPI:**

        ▼        ▼     ▼     ▼     ▼- PA4: CS (Chip Select)

    ┌──────┐ ┌─────┐ ┌────┐ ┌────┐ ┌─────┐- PA5: SCK (SPI Clock)

    │ USB  │ │IO16 │ │AIO20│ │ -- │ │IO16 │- PA6: MOSI (Master Out Slave In)

    │Serial│ │ #1  │ │     │ │    │ │ #2  │- PA7: MISO (Master In Slave Out)

    └──────┘ └─────┘ └─────┘ └────┘ └─────┘- PB0: INT (FPGA Interrupt)

             iC-JX   MAX11300  RSV   iC-JX

```**FPGA ↔ Motors:**

- 6 motor × 2 PWM (left/right direction)

---- 6 motor × 1 Hall sensor (position feedback)

- 3 driver IC × control signals (RESET, FAULT, OTW, MODE)

## 🚀 Hızlı Başlangıç

---

### 1. Gereksinimler

## 🚀 Quick Start

**Donanım:**

- STM32F103RCT6 Blue Pill (veya eşdeğeri)### Prerequisites

- USB-Serial dönüştürücü (programlama için)

- Burjuva I/O modülleri (opsiyonel)**WSL2/Linux:**

```bash

**Yazılım:**# ARM GCC toolchain

- ARM GCC Toolchain (`arm-none-eabi-gcc`)sudo apt install gcc-arm-none-eabi

- ST-Link programlayıcı (veya USB bootloader)

- Serial terminal (PuTTY, minicom, vb.)# Build tools

sudo apt install make cmake

### 2. Derleme

# FPGA tools (already installed)

```bashsudo apt install yosys nextpnr-ice40 fpga-icestorm

cd stm32-firmware-beta```

build.bat          # Windows

# veya**Python (for scripting):**

make               # Linux/WSL```bash

```pip3 install pyserial RPi.GPIO spidev

```

**Çıktı dosyaları:**

- `build/firmware.elf` - Debug sembollerle### Build

- `build/firmware.hex` - Intel HEX formatı

- `build/firmware.bin` - Raw binary```bash

cd burjuva-motor

### 3. Programlama

# Build firmware

**ST-Link ile:**make all

```bash

st-flash write build/firmware.bin 0x8000000# Clean

```make clean

```

**USB Bootloader ile:**

```bash### ⚠️ Programming (IMPORTANT!)

stm32flash -w build/firmware.bin -v -g 0x0 /dev/ttyUSB0

```**FPGA programlanması sadece BİR KERE gereklidir!**



### 4. Bağlantı ve Test#### İlk Kurulum (One-time Setup):

```bash

**Serial Bağlantısı:**# 1. FPGA + STM32 birlikte programa

```bashcd scripts

# Linuxchmod +x *.sh

minicom -D /dev/ttyUSB0 -b 115200./initial_setup.sh



# Windows# Bu script:

# PuTTY: COM3, 115200, 8N1# - FPGA'ya burjuva-fpga.bin flashlar (KALICI!)

```# - STM32'ye burjuva-motor.bin flashlar

# - Sistem doğrulaması yapar

**İlk Komutlar:**```

```

> help#### Firmware Güncelleme (Sadece STM32):

> modul-algila```bash

> io16:0:read:0# Kod değişikliklerinden sonra

> aio20:1:readin:0make clean && make all

```

# SADECE STM32'yi güncelle (FPGA dokunulmaz!)

---cd scripts

./update_firmware.sh

## 📡 UART Komut Arayüzü```



### Genel Komutlar**Not:** FPGA power-on'da otomatik olarak yüklenir, tekrar programlamaya gerek yok!



| Komut | Açıklama | Örnek |---

|-------|----------|-------|

| `help` | Komut listesi | `help` |## 💻 API Usage

| `modul-algila` | Modül taraması | `modul-algila` |

### Initialize System

### IO16 Komutları (Slot 0, 3)

```c

```bash#include "motor.h"

# Pin okuma#include "fpga_driver.h"

io16:SLOT:read:PIN#include "uart.h"

Örnek: io16:0:read:5#include "analog_io.h"

#include "digital_io.h"

# Pin yazma

io16:SLOT:write:PIN:VALUEint main(void) {

Örnek: io16:0:write:7:high    // Initialize hardware

    HAL_Init();

# Pin modu ayarla    SystemClock_Config();

io16:SLOT:mode:PIN:MODE    

Örnek: io16:0:mode:3:output    // Initialize UART (for debug/control)

    uart_init();  // 921600 baud, PA9/PA10

# Direction kontrolü (grup)    

io16:SLOT:direction:GROUP:DIR    // Initialize FPGA communication

Örnek: io16:0:direction:0:output    fpga_init();

    

# Çoklu pin okuma    // Initialize motor controller

io16:SLOT:readall    motor_init();

Örnek: io16:3:readall    

    // Initialize analog I/O

# Port yazma (16-bit)    analog_io_init();

io16:SLOT:writeport:VALUE    

Örnek: io16:0:writeport:0xFF00    // Initialize digital I/O

```    digital_io_init();

    

### AIO20 Komutları (Slot 1)    while(1) {

        // Process UART commands

```bash        uart_process_commands();

# Analog input okuma        

aio20:SLOT:readin:CHANNEL        // Update motors

Örnek: aio20:1:readin:3        motor_task();

        

# Analog output yazma        // Update analog I/O

aio20:SLOT:writeout:CHANNEL:VALUE        analog_input_task();

Örnek: aio20:1:writeout:5:7.5        analog_output_task();

        

# Kanal yapılandırma        // Update digital I/O

aio20:SLOT:config:CHANNEL:MODE        digital_io_task();

Örnek: aio20:1:config:0:ain_0_10v        

        delay_ms(10);

# ADC okuma (raw)    }

aio20:SLOT:readadc:CHANNEL}

Örnek: aio20:1:readadc:2```



# DAC yazma (raw)### Motor Control (C API)

aio20:SLOT:writedac:CHANNEL:VALUE

Örnek: aio20:1:writedac:4:2048```c

// Move motor 0 to position 10000 at speed 150

# Durum bilgisimotor_move_abs(0, 150, 10000);

aio20:SLOT:statusmotor_wait(0, 5000);

Örnek: aio20:1:status

```// Set continuous speed

motor_set_speed(0, 200, MOTOR_DIR_FORWARD);

---

// Read current position

## 📁 Proje Yapısıuint32_t pos;

motor_get_position(0, &pos);

```

burjuva-atacama/// Stop motor

├── README.md                    # Bu dosyamotor_stop(0);

├── HARDWARE.md                  # Pin mapping ve donanım detayları

├── API_GUIDE.md                 # Komut referansı ve örnekler// Emergency stop all motors

├── BUILD.md                     # Derleme ve programlamamotor_emergency_stop();

│```

├── stm32-firmware-beta/

│   ├── src/### Analog I/O Control (C API)

│   │   ├── main.c              # Ana program + UART handler

│   │   ├── modul_algilama.c    # 1-Wire modül algılama```c

│   │   ├── spisurucu.c         # SPI driver#include "analog_io.h"

│   │   ├── 16kanaldijital.c    # IO16 driver

│   │   ├── 20kanalanalogio.c   # AIO20 driver// Configure analog input as 0-10V

│   │   └── uart_helper.c       # UART utilitiesanalog_input_config_t ain_cfg = {

│   │    .channel = 0,

│   ├── spl/                    # STM32 Standard Peripheral Library    .type = ANALOG_INPUT_VOLTAGE_0_10V,

│   ├── build/                  # Derleme çıktıları    .scale = 1.0f,

│   ├── build.bat               # Windows build script    .offset = 0.0f,

│   └── Makefile                # Linux build script    .min_value = 0.0f,

│    .max_value = 10.0f,

├── cpld/                       # CPLD configuration (opsiyonel)    .filter_samples = 8,

└── burjuva_manager.py          # Python yönetim arayüzü    .enable_alarm = false

```};

analog_input_configure(&ain_cfg);

---analog_input_enable(0);



## 🔧 Teknik Detaylar// Read analog input

float voltage, scaled_value;

### Clock Yapılandırmasıanalog_input_read_voltage(0, &voltage);

- **Kaynak:** HSE (8 MHz kristal)analog_input_read_scaled(0, &scaled_value);

- **PLL:** ×9 çarpan

- **SYSCLK:** 72 MHz// Configure analog output as 0-10V

- **AHB:** 72 MHzanalog_output_config_t aout_cfg = {

- **APB1:** 36 MHz (SPI2 burada)    .channel = 0,

- **APB2:** 72 MHz (USART1 burada)    .type = ANALOG_OUTPUT_VOLTAGE_0_10V,

    .scale = 1.0f,

### SPI Ayarları    .offset = 0.0f,

- **Peripheral:** SPI2 (APB1)    .min_value = 0.0f,

- **Mode:** Master, Full-Duplex    .max_value = 10.0f,

- **Format:** MSB First, 8-bit    .enable_limit = true

- **CPOL/CPHA:** 0/0 (Mode 0)};

- **Prescaler:** Slot-specific (8, 4, 16)analog_output_configure(&aout_cfg);

analog_output_enable(0);

### GPIO Pin Kullanımı

// Write analog output

**UART (Debug/Control):**analog_output_write_scaled(0, 7.5f);  // 7.5V

- PA9: TX → Bilgisayar RX

- PA10: RX ← Bilgisayar TX// Read temperature sensor (PT100)

analog_input_config_t temp_cfg = {

**SPI (Modül İletişimi):**    .channel = 1,

- PB13: SCK (Clock)    .type = ANALOG_INPUT_PT100,

- PB14: MISO (Master In)    .scale = 1.0f,

- PB15: MOSI (Master Out)    .offset = 0.0f,

    .min_value = -50.0f,

**Chip Select (CS):**    .max_value = 200.0f,

- PC13: Slot 0 (IO16 #1)    .filter_samples = 16,

- PA0: Slot 1 (AIO20)    .enable_alarm = true

- PA1: Slot 2 (RESERVED)};

- PA2: Slot 3 (IO16 #2)analog_input_configure(&temp_cfg);

analog_input_enable(1);

**1-Wire (Modül Algılama):**

- PC0: Slot 1 algılamafloat temperature;

- PC1: Slot 3 algılamaanalog_read_temperature(1, &temperature);  // °C

- PC2: Slot 0 algılama

- PC3: Slot 2 algılama// Read 4-20mA current sensor

float current;

**Diğer:**analog_read_current(2, &current);  // mA

- PC13: LED (onboard)

// Control 4-20mA output

---analog_write_current(0, 12.0f);  // 12mA

```

## 🐛 Sorun Giderme

### Analog I/O Control (Python API)

### Derleme Hataları

```python

**Toolchain bulunamıyor:**from uart_control import MotorUART

```bash

# Windowsuart = MotorUART(port='/dev/ttyAMA0', baudrate=921600)

choco install gcc-arm-embeddeduart.connect()



# Linux# Read analog input

sudo apt install gcc-arm-none-eabiuart.read_analog_input(0)

```

# Write analog output

**Linking hatası:**uart.write_analog_output(1, 7.5)

- `stm32.ld` dosyasının `spl/` klasöründe olduğunu kontrol edin

- `FLASH` ve `RAM` boyutlarının STM32F103RCT6 ile uyumlu olduğunu doğrulayın# Configure input as PT100 temperature sensor

uart.configure_analog_input(2, 2)  # 2 = PT100

### Programlama Sorunları

# Show all analog I/O

**ST-Link tanınmıyor:**uart.show_analog_inputs()

```bashuart.show_analog_outputs()

# Linux udev kuralları

sudo cp 49-stlink.rules /etc/udev/rules.d/uart.disconnect()

sudo udevadm control --reload-rules```

```

### Digital I/O Control (C API)

**Flash yazma hatası:**

- BOOT0 pin'ini GND'ye çekin (normal çalışma modu)```c

- Kartı sıfırlayıp tekrar deneyin#include "digital_io.h"



### Çalışma Zamanı Sorunları// Configure digital input as limit switch

digital_input_config_t din_cfg = {

**Modül algılanmıyor:**    .channel = 0,

- 1-Wire pinlerinin bağlantısını kontrol edin    .type = DIGITAL_INPUT_LIMIT_SWITCH,

- Modüllerin doğru slot'a takıldığından emin olun    .pull = DIGITAL_PULL_UP,

- `modul-algila` komutunu tekrar çalıştırın    .edge_detect = DIGITAL_EDGE_BOTH,

    .invert = false,

**SPI iletişim hatası:**    .enable_debounce = true,

- CS pin bağlantılarını kontrol edin    .debounce_ms = 10,

- CPLD konfigürasyonunun yüklendiğini doğrulayın    .enable_interrupt = false

- SPI hızını düşürmeyi deneyin (prescaler'ı artırın)};

digital_input_configure(&din_cfg);

**UART yanıt vermiyor:**digital_input_enable(0);

- Baud rate: 115200, 8N1 olduğunu kontrol edin

- TX/RX pinlerinin yer değiştirmediğinden emin olun// Read digital input

- LED'in yanıp söndüğünü kontrol edin (sistem çalışıyor)bool state;

digital_input_read(0, &state);

---printf("Limit switch: %s\n", state ? "PRESSED" : "RELEASED");



## 📝 Geliştirme Notları// Get pulse count (encoder)

uint32_t count;

### Versiyon Geçmişidigital_input_get_count(0, &count);

printf("Encoder pulses: %lu\n", count);

**v2.0 (27 Kasım 2025)**

- ❌ FPGA desteği kaldırıldı// Configure digital output as relay

- ✅ Sadece IO16 ve AIO20 modülleridigital_output_config_t dout_cfg = {

- ✅ Temizlenmiş dokümantasyon    .channel = 0,

- ✅ Build sistemi optimize edildi    .type = DIGITAL_OUTPUT_RELAY,

    .invert = false,

**v1.x**    .initial_state = false,

- FPGA motor kontrolü    .enable_pwm = false,

- 4 slot desteği    .enable_safety = false

- İlk sürüm};

digital_output_configure(&dout_cfg);

### Katkıda Bulunmadigital_output_enable(0);



Bu proje Burjuva Pilot için geliştirilmiştir. Dahili kullanım için tasarlanmıştır.// Write digital output

digital_output_write(0, true);   // Turn ON

### Lisansdigital_output_write(0, false);  // Turn OFF



**Proprietary** - Tüm hakları saklıdır  // Toggle output

© 2025 Burjuva Pilotdigital_output_toggle(0);



---// Pulse output (500ms)

digital_output_pulse(0, 500);

## 📚 Ek Kaynaklar

// Blink LED (500ms on, 500ms off, 10 cycles)

- [HARDWARE.md](HARDWARE.md) - Detaylı pin mapping ve elektrik özellikleridigital_output_blink(0, 500, 500, 10);

- [API_GUIDE.md](API_GUIDE.md) - Komut referansı ve örnekler

- [BUILD.md](BUILD.md) - Gelişmiş derleme seçenekleri// Configure LED indicator with PWM

dout_cfg.channel = 1;

---dout_cfg.type = DIGITAL_OUTPUT_LED;

dout_cfg.enable_pwm = true;

**Son Güncelleme:** 27 Kasım 2025  dout_cfg.pwm_duty_cycle = 50;  // 50% brightness

**Geliştirici:** Burjuva Pilot Ekibidigital_output_configure(&dout_cfg);

digital_output_enable(1);

// Adjust LED brightness
digital_output_set_pwm(1, 75);  // 75% brightness

// Read all inputs as 16-bit mask
digital_port_mask_t inputs;
digital_input_read_all(&inputs);
printf("Input mask: 0x%04X\n", inputs);

// Write all outputs
digital_port_mask_t outputs = 0x00FF;  // Turn on first 8 outputs
digital_output_write_all(outputs);

// Emergency stop all outputs
digital_output_emergency_stop();
```

### Digital I/O Control (Python API)

```python
from uart_control import MotorUART

uart = MotorUART(port='/dev/ttyAMA0', baudrate=921600)
uart.connect()

# Read digital input
uart.read_digital_input(0)

# Write digital output
uart.write_digital_output(1, True)   # Turn ON
uart.write_digital_output(1, False)  # Turn OFF

# Toggle output
uart.toggle_digital_output(2)

# Pulse output (500ms)
uart.pulse_digital_output(3, 500)

# Blink LED (500ms on, 500ms off, infinite)
uart.blink_digital_output(4, 500, 500, 0)

# Configure input as limit switch
uart.configure_digital_input(5, 3)  # 3 = Limit switch

# Configure output as relay
uart.configure_digital_output(6, 0)  # 0 = Relay

# Show all digital I/O
uart.show_digital_inputs()
uart.show_digital_outputs()

uart.disconnect()
```

### Motor Control (UART Commands)

**Connect via UART:** 921600 baud, 8N1, PA9(TX)/PA10(RX)

```bash
# Method 1: Using provided Python script (recommended)
cd burjuva-motor/scripts
python3 uart_control.py --mode shell

# Method 2: Direct serial terminal
screen /dev/ttyAMA0 921600

# Method 3: Python miniterm
python3 -m serial.tools.miniterm /dev/ttyAMA0 921600

# Method 4: Single command
python3 uart_control.py -c "status"
```

**Available Commands:**
```
Motor Control:
  help                      - Show all commands
  status [motor_id]         - Show motor status (or all)
  move <id> <speed> <pos>   - Move motor to position
  speed <id> <val> <dir>    - Set continuous speed (dir: 0=fwd, 1=rev)
  stop [motor_id]           - Stop motor (or all)
  estop                     - EMERGENCY STOP ALL
  home <motor_id>           - Home motor (find zero)
  pos <motor_id>            - Get current position

Analog I/O:
  ain <ch>                  - Read analog input
  aout <ch> <val>           - Set analog output
  ains                      - Show all analog inputs
  aouts                     - Show all analog outputs
  aincfg <ch> <type>        - Configure analog input
                              (0=0-10V, 1=4-20mA, 2=PT100, 3=TC-K)
  aoutcfg <ch> <type>       - Configure analog output
                              (0=0-10V, 1=4-20mA)

Digital I/O:
  din <ch>                  - Read digital input
  dout <ch> <0|1>           - Set digital output
  dins                      - Show all digital inputs
  douts                     - Show all digital outputs
  dincfg <ch> <type>        - Configure digital input
                              (0=24V, 1=Switch-NO, 2=Switch-NC, 3=Limit)
  doutcfg <ch> <type>       - Configure digital output
                              (0=Relay, 1=SSR, 2=LED, 3=Solenoid)
  dtoggle <ch>              - Toggle digital output
  dpulse <ch> <ms>          - Pulse output for duration (ms)
  dblink <ch> <on> <off>    - Blink output (on/off times in ms)

System:
  info                      - System information
  test                      - Run motor tests
```

**Example Session:**
```
> help
Available Commands:
──────────────────────────────────────────────────────
  help                    - Show this help
  status [motor_id]       - Show motor status (or all)
  move <id> <speed> <pos> - Move motor to position
  ...

> status
┌───────────────────────────────────────────┐
│        Motor Status Summary               │
├──┬────────┬──────────┬──────────┬─────────┤
│ID│ State  │ Position │  Speed   │ Moving  │
├──┼────────┼──────────┼──────────┼─────────┤
│0 │ IDLE   │        0 │     0    │   NO    │
│1 │ IDLE   │        0 │     0    │   NO    │
...

> move 0 150 10000
Motor 0 moving to 10000 at speed 150

> pos 0
Motor 0 position: 5234

> stop 0
Motor 0 stopped

> ain 0
Analog Input CH0:
  Raw value:    2048
  Voltage:      1.650 V
  Scaled value: 5.000
  Status:       Valid

> aout 1 7.5
Analog Output CH1 set to 7.500
  Output voltage: 2.475V

> ains
╔══════════════════════════════════════════════════════════════════════╗
║                      ANALOG INPUT STATUS                             ║
╠════╦═════════╦══════════╦═════════╦══════════════╦═══════╦══════════╣
║ CH ║  Type   ║   Raw    ║ Voltage ║    Scaled    ║ Alarm ║  Status  ║
╠════╬═════════╬══════════╬═════════╬══════════════╬═══════╬══════════╣
║ 0  ║ 0-10V   ║  2048    ║ 1.65V   ║       5.00   ║  NO   ║  OK      ║
║ 1  ║ 4-20mA  ║  1638    ║ 1.32V   ║      12.00   ║  NO   ║  OK      ║
╚════╩═════════╩══════════╩═════════╩══════════════╩═══════╩══════════╝

> aincfg 2 2
Analog Input CH2: Configured as PT100 temperature
Analog Input CH2: Enabled

> ain 2
Analog Input CH2:
  Raw value:    2456
  Voltage:      1.980 V
  Scaled value: 25.3 °C
  Status:       Valid

> din 0
Digital Input CH0:
  State:       HIGH (1)
  Raw state:   1
  Pulse count: 42

> dout 1 1
Digital Output CH1: ON

> dtoggle 2
Digital Output CH2: Toggled to OFF

> dpulse 3 500
Digital Output CH3: Pulsed for 500 ms

> dblink 4 500 500 5
Digital Output CH4: Blinking (500/500 ms, 5 cycles)

> dins
╔══════════════════════════════════════════════════════════════════════╗
║                      DIGITAL INPUT STATUS                            ║
╠════╦════════════╦═══════╦═══════╦═════════╦══════════╦═════════════╣
║ CH ║    Type    ║ State ║  Raw  ║  Edges  ║  Pulses  ║   Status    ║
╠════╬════════════╬═══════╬═══════╬═════════╬══════════╬═════════════╣
║ 0  ║ Limit SW   ║  ON   ║  1    ║ Rising  ║       42 ║ OK          ║
║ 1  ║ Proximity  ║  OFF  ║  0    ║ None    ║        0 ║ OK          ║
║ 2  ║ E-STOP     ║  ON   ║  1    ║ None    ║        0 ║ OK          ║
╚════╩════════════╩═══════╩═══════╩═════════╩══════════╩═════════════╝

> douts
╔══════════════════════════════════════════════════════════════════════╗
║                     DIGITAL OUTPUT STATUS                            ║
╠════╦════════════╦═══════╦═══════╦══════════╦══════════╦═════════════╣
║ CH ║    Type    ║ State ║  Raw  ║   PWM    ║  Cycles  ║   Status    ║
╠════╬════════════╬═══════╬═══════╬══════════╬══════════╬═════════════╣
║ 0  ║ Relay      ║  ON   ║  1    ║ OFF      ║       15 ║ OK          ║
║ 1  ║ LED        ║  OFF  ║  0    ║ OFF      ║        3 ║ OK          ║
║ 2  ║ Solenoid   ║  ON   ║  1    ║ OFF      ║       27 ║ OK          ║
╚════╩════════════╩═══════╩═══════╩══════════╩══════════╩═════════════╝

> dincfg 5 3
Digital Input CH5: Configured as Limit switch
Digital Input CH5: Enabled

> doutcfg 6 2
Digital Output CH6: Configured as LED indicator
Digital Output CH6: Enabled
```

### FPGA Direct Access

```c
// Read motor status
uint8_t flags = fpga_read_register(FPGA_REG_MOTOR0_FLAGS);

// Write speed command
fpga_write_register(FPGA_REG_MOTOR0_SPEED, 255);

// Read 24-bit position
uint32_t pos = fpga_read_position(0);
```

---

## 📡 SPI Protocol

### Register Map

| Address | R/W | Description |
|---------|-----|-------------|
| 0x00-0x07 | R | Motor 0 (flags, position, velocity) |
| 0x08-0x0F | R | Motor 1 |
| 0x10-0x17 | R | Motor 2 |
| 0x18-0x1F | R | Motor 3 |
| 0x20-0x27 | R | Motor 4 |
| 0x28-0x2F | R | Motor 5 |
| 0x40-0x47 | W | Motor 0 control (speed, target) |
| 0x48-0x4F | W | Motor 1 control |
| 0x50-0x57 | W | Motor 2 control |
| 0x58-0x5F | W | Motor 3 control |
| 0x60-0x67 | W | Motor 4 control |
| 0x68-0x6F | W | Motor 5 control |

### Transaction Format

**Read (5 bytes):**
```
TX: [address, 0x00, 0x00, 0x00, 0x00]
RX: [  0x00, 0x00, data_h, data_m, data_l]
```

**Write (5 bytes):**
```
TX: [address, flags, data_h, data_m, data_l]
RX: [  0x00,  0x00,   0x00,   0x00,   0x00]
```

Detaylı protokol: `docs/PROTOCOL.md`

---

## 🧪 Testing

### Communication Test
```bash
# SPI loopback test
./scripts/test.sh --spi

# Motor status test
./scripts/test.sh --status

# Single motor test
./scripts/test.sh --motor 0
```

### Performance Benchmarks
- SPI transfer: ~222 µs (5 bytes @ 4.5 MHz)
- Position update rate: ~40 kHz per motor
- PWM frequency: 93.75 kHz
- Interrupt latency: <10 µs

---

## 📊 Performance

### FPGA Resources
- Logic Cells: 1038/1280 (81%)
- RAM: 1/16 (6%)
- I/O Pins: 44/112 (39%)
- Max Frequency: 65.95 MHz ⭐

### Timing
- Clock: 24 MHz
- SPI: 4.5 MHz
- PWM: 93.75 kHz
- Position update: 40 kHz per motor
- Timeout: 1.4 seconds

---

## 🔐 Safety Features

1. **Timeout Detection** - Motor otomatik durur (1.4s)
2. **Limit Switches** - Hardware limit algılama
3. **Fault Detection** - DRV8434 fault pinleri
4. **Over-Temperature Warning** - OTW sinyalleri
5. **Emergency Stop** - Tek komutla tüm motorlar durur
6. **Watchdog** - STM32 watchdog timer

---

## 📖 Documentation

- **[API.md](docs/API.md)** - Complete API reference
- **[HARDWARE.md](docs/HARDWARE.md)** - Hardware design & pin mapping
- **[PROTOCOL.md](docs/PROTOCOL.md)** - SPI protocol specification
- **[../fpga-build/PIN_MAPPING.md](../fpga-build/PIN_MAPPING.md)** - FPGA detailed pins
- **[../docs/08-FPGA-DCMCTRL-DETAYLI-ANALIZ.md](../docs/08-FPGA-DCMCTRL-DETAYLI-ANALIZ.md)** - FPGA code analysis

---

## 🛠️ Development

### Build System
- **Makefile** - GNU Make based build
- **ARM GCC** - arm-none-eabi-gcc toolchain
- **OpenOCD** - Programming & debugging
- **icestorm** - FPGA synthesis (WSL2)

### Directory Layout
```
firmware/src/
├── main.c           # Main program entry
├── fpga_driver.c    # Low-level FPGA SPI driver
├── motor.c          # High-level motor control API
├── spi.c            # STM32 SPI HAL wrapper
└── system.c         # Clock, init, interrupts

firmware/inc/
├── fpga_driver.h    # FPGA register definitions
├── motor.h          # Motor control API
├── spi.h            # SPI interface
└── config.h         # Configuration parameters
```

### Coding Standards
- C99 standard
- 4 spaces indentation
- Max 100 chars per line
- Doxygen comments
- MISRA-C guidelines (where applicable)

---

## 🐛 Troubleshooting

### SPI Communication Fails
```bash
# Check FPGA programming
ssh burjuva@192.168.1.22 "ls -l /sys/class/spi*"

# Test SPI loopback
./scripts/test.sh --spi

# Check clock polarity (CPOL=0, CPHA=0)
```

### Motor Not Moving
```bash
# Check DRV8434 FAULT pin
# Verify power supply (VM = motor voltage)
# Check RESET signals (should be HIGH)
# Read FPGA status registers
./scripts/test.sh --status
```

### FPGA Not Responding
```bash
# Re-program FPGA
./scripts/program.sh --fpga-only

# Check clock (should be 24 MHz)
# Verify SPI connections with multimeter
```

---

## 🎮 Jog Mode (Manual Control)

**Jog Mode** allows motor control **without homing** - perfect for testing, manual positioning, and setup.

### Quick Jog Commands

```bash
# Continuous jog (speed control)
> speed 0 128 f    # Motor 0, speed 128, forward
> speed 0 200 r    # Motor 0, speed 200, reverse
> stop 0           # Stop motor 0
> estop            # Emergency stop all

# Step-by-step jog (relative movement)
> move 0 150 +1000   # Move +1000 steps forward
> move 0 150 -500    # Move -500 steps backward

# Position reset (without homing)
# Use motor_set_position(0, 0) in C code
```

### Jog Mode Features

✅ **No homing required** - Start immediately  
✅ **Simple speed + direction control**  
✅ **Relative positioning available**  
✅ **Manual position reset**  
✅ **Perfect for testing and setup**  

### When to Use Jog Mode

| Use Case | Jog Mode | Absolute Mode |
|----------|----------|---------------|
| **Testing** | ✅ Perfect | Not needed |
| **Manual Control** | ✅ Ideal | Overkill |
| **Setup/Calibration** | ✅ Good | Better |
| **Production** | ❌ Not recommended | ✅ Required |
| **CNC/Automation** | ❌ Not suitable | ✅ Essential |

### Safety in Jog Mode

```c
// Enable soft limits
motor_config_t config;
config.motor_id = 0;
config.enable_limits = true;  // ✅ Check limit switches
config.min_position = 0;
config.max_position = 100000;
motor_configure(&config);

// Safe jog with timeout
motor_set_speed(0, 128, MOTOR_DIR_FORWARD);
delay_ms(2000);  // Run for 2 seconds
motor_stop(0);   // Auto-stop
```

### Detailed Guide

For complete jog mode documentation including:
- Joystick control examples
- Multi-motor jog
- Safety best practices
- Homing procedures

See: **[JOG_MODE_GUIDE.md](JOG_MODE_GUIDE.md)**

---

## 📝 TODO

- [ ] Implement PID control loops
- [ ] Add trajectory planning
- [ ] Ethernet/CAN communication
- [ ] Web-based configuration interface
- [ ] Motor calibration wizard
- [ ] Data logging & telemetry
- [ ] Multi-board synchronization

---

## 📜 License

**Proprietary** - Burjuva Pilot Project  
© 2025 All Rights Reserved

---

## 🤝 Contributing

This is a private project. For internal development only.

---

## 📞 Support

- **Documentation:** `docs/` directory
- **Issues:** Internal tracker
- **Contact:** Burjuva Pilot Team

---

**Built with ❤️ for precise motor control**  
**Version:** 1.0.0  
**Last Updated:** 13 Kasım 2025
