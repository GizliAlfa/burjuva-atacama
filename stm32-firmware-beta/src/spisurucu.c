/**
 * Burjuva Pilot - SPI Sürücüsü Implementasyonu
 * SPI Driver Implementation for Module Communication
 */

#include "spisurucu.h"
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"

// Chip Select pin tanımları
typedef struct {
    GPIO_TypeDef* gpio;
    uint16_t pin;
    const char* name;
} cs_pin_t;

// Her slot için CS pin mapping  
// ✅ CPLD TOP.V ANALİZİNDEN DOĞRULANDI (build--CPLD/top.v)
// 📌 KESIN PIN EŞLEŞTIRME:
//
// SLOT YAPISI (CPLD içinde 4 modül instance):
// Slot 0: io16_0  → CS: PC13 (MODÜL 4), INT: PA3  (MODÜL 1)
// Slot 1: aio20_1 → CS: PA0  (MODÜL 1), INT: PC4  (MODÜL 3), CNVT: PC5
// Slot 2: fpga_2  → CS: PA1  (MODÜL 1), INT: PB0  (MODÜL 1), CRESET: PB1, CDONE: PB10
// Slot 3: io16_3  → CS: PA2  (MODÜL 1), INT: PB11 (MODÜL 3)
//
// SPI BUS (Paylaşımlı - MODÜL 3):
// SCK: PB13, MISO: PB14 (CPLD multiplexed), MOSI: PB15
//
// MISO MULTIPLEXER (CPLD içinde):
// PB_14 = !PC13 ? IO16_MISO_0 : !PA0 ? AIO20_MISO_1 : !PA1 ? FPGA_MISO_2 : !PA2 ? IO16_MISO_3 : 0
// CS pini LOW olunca otomatik olarak o slot'un MISO'su PB14'e bağlanır!
static const cs_pin_t cs_pins[5] = {
    { GPIOC, GPIO_Pin_13, "PC13" },  // Slot 0 (Kullanıcı Slot 1) - IO16 #1
    { GPIOA, GPIO_Pin_0,  "PA0"  },  // Slot 1 (Kullanıcı Slot 2) - AIO20
    { GPIOA, GPIO_Pin_1,  "PA1"  },  // Slot 2 (Kullanıcı Slot 3) - FPGA
    { GPIOA, GPIO_Pin_2,  "PA2"  },  // Slot 3 (Kullanıcı Slot 4) - IO16 #2
    { GPIOA, GPIO_Pin_3,  "PA3"  }   // Slot 4 (Yedek - kullanılmıyor)
};

// Şu anda seçili slot (-1 = hiçbiri)
static int current_cs_slot = -1;

/**
 * SPI GPIO pinlerini yapılandır
 * MOTOR-DEMO AYARLARI (stmmodel.json'dan):
 * SPI: "GPIO":1 "ClkPin":13 "MisoPin":14 "MosiPin":15 = GPIOB Pin 13/14/15
 * SPIPeriph:1 = SPI2 (APB1)
 */
static void SPI_GPIO_Init(void) {
    GPIO_InitTypeDef gpio;
    
    // GPIOA, GPIOB, GPIOC clock'larını aktif et
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | 
                           RCC_APB2Periph_GPIOC, ENABLE);
    
    // SPI2 pinleri (MEVCUT SİSTEM): PB13(SCK), PB14(MISO), PB15(MOSI)
    // SCK ve MOSI: Alternate Function Push-Pull
    gpio.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &gpio);
    
    // MISO: Input Floating
    gpio.GPIO_Pin = GPIO_Pin_14;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &gpio);
    
    // Chip Select pinleri: Output Push-Pull
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    
    // CS GPIOA (PA0, PA1, PA2 - Slot 1, 2, 3)
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_Init(GPIOA, &gpio);
    
    // CS GPIOC (PC13 - Slot 0)
    gpio.GPIO_Pin = GPIO_Pin_13;
    GPIO_Init(GPIOC, &gpio);
    
    // Tüm CS pinlerini HIGH yap (deaktif - aktif LOW standard)
    for (int i = 0; i < 5; i++) {
        GPIO_SetBits(cs_pins[i].gpio, cs_pins[i].pin);
    }
}

/**
 * Simple delay function (microseconds)
 * At 72MHz: ~72 cycles = 1us
 */
static void delay_us(uint32_t us) {
    volatile uint32_t count = us * 18; // Approx 72 cycles per us / 4 cycles per loop
    while (count--) {
        __asm volatile ("nop");
    }
}

/**
 * SPI peripheral'i yapılandır
 * SPI2 kullanıyoruz (PB13/14/15) - MEVCUT SİSTEM KONFİGÜRASYONU!
 * 
 * CRITICAL FIX: iC-JX chip requires slower SPI speed and proper timing!
 * - BaudRate: Prescaler_256 (slowest) instead of _16
 * - Mode: CPOL=Low, CPHA=1Edge (Mode 0)
 * - CS timing: Delays added between transactions
 */
static void SPI_Peripheral_Init(void) {
    SPI_InitTypeDef spi;
    
    // SPI2 clock'u aktif et (APB1'de!)
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    
    // SPI yapılandırması - Motor-demo stmmodel.json ayarlarına göre
    SPI_StructInit(&spi);
    spi.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode              = SPI_Mode_Master;
    spi.SPI_DataSize          = SPI_DataSize_8b;
    spi.SPI_FirstBit          = SPI_FirstBit_MSB;
    spi.SPI_NSS               = SPI_NSS_Soft;
    
    // ✅ MOTOR-DEMO UYUMLU: APB1 = 36MHz @ 72MHz system clock
    // Slot 0 (IO16):  BaudRatePrescaler_8  → 36MHz/8  = 4.5 MHz
    // Slot 1 (AIO20): BaudRatePrescaler_4  → 36MHz/4  = 9.0 MHz
    // Slot 2 (FPGA):  BaudRatePrescaler_16 → 36MHz/16 = 2.25 MHz
    // Slot 3 (IO16):  BaudRatePrescaler_8  → 36MHz/8  = 4.5 MHz
    // 
    // Default başlangıç: Prescaler_8 (4.5 MHz - IO16 için)
    // Her slot için optimize edilmiş prescaler ayrı ayarlanacak
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
    
    // Mode 0: CPOL=0 (clock idle low), CPHA=0 (sample on first edge)
    // Motor-demo: CPOL=null, CPHA=null → Default Mode 0 kullanılıyor
    spi.SPI_CPOL              = SPI_CPOL_Low;
    spi.SPI_CPHA              = SPI_CPHA_1Edge;
    
    SPI_Init(SPI2, &spi);
    SPI_Cmd(SPI2, ENABLE);
    
    // Small delay after SPI enable
    delay_us(10);
}

/**
 * Slot-specific SPI prescaler ayarla
 * Motor-demo stmmodel.json'dan alınan değerler
 */
static void SPI_SetPrescalerForSlot(spi_slot_t slot) {
    uint16_t prescaler;
    
    switch(slot) {
        case 0:  // IO16 - Slot 0
            prescaler = SPI_BaudRatePrescaler_8;  // 4.5 MHz (36MHz/8)
            break;
        case 1:  // AIO20 - Slot 1
            prescaler = SPI_BaudRatePrescaler_4;  // 9.0 MHz (36MHz/4)
            break;
        case 2:  // FPGA - Slot 2
            prescaler = SPI_BaudRatePrescaler_16; // 2.25 MHz (36MHz/16)
            break;
        case 3:  // IO16 - Slot 3
            prescaler = SPI_BaudRatePrescaler_8;  // 4.5 MHz (36MHz/8)
            break;
        default:
            prescaler = SPI_BaudRatePrescaler_8;  // Default
            break;
    }
    
    // SPI disable → prescaler change → SPI enable
    SPI_Cmd(SPI2, DISABLE);
    SPI2->CR1 = (SPI2->CR1 & ~SPI_BaudRatePrescaler_256) | prescaler;
    SPI_Cmd(SPI2, ENABLE);
}

/**
 * SPI sistemini başlat
 */
void SPI_Module_Init(void) {
    SPI_GPIO_Init();
    SPI_Peripheral_Init();
    current_cs_slot = -1;
}

/**
 * Chip Select kontrolü
 * ✅ MOTOR-DEMO UYUMLU: Her slot için optimize edilmiş prescaler
 * Mevcut sistem mantığı: Her CS enable'da SPI yeniden yapılandırılır
 */
int SPI_SetCS(spi_slot_t slot, chip_select_t cs) {
    // Geçerlilik kontrolü (0-4 arası, ama 4 kullanılmıyor)
    if (slot < 0 || slot > 4) {
        return -1;
    }
    
    // CS durumuna göre pin kontrolü
    if (cs == CS_ENABLE) {
        // Eğer hiçbir CS aktif değilse veya farklı bir CS ise
        if (current_cs_slot == -1) {
            // Önce mevcut CS'leri kapat (HIGH - deaktif)
            for (int i = 0; i < 5; i++) {
                GPIO_SetBits(cs_pins[i].gpio, cs_pins[i].pin);
            }
            
            // Small delay after disabling old CS
            delay_us(10);
            
            // ✅ MOTOR-DEMO: Slot'a özel SPI prescaler ayarla
            SPI_SetPrescalerForSlot(slot);
            
            // CRITICAL: Delay before enabling new CS (chip setup time)
            delay_us(50);
            
            // Yeni CS'i aç (LOW - aktif!)
            GPIO_ResetBits(cs_pins[slot].gpio, cs_pins[slot].pin);
            
            // CRITICAL: Delay after CS enable (IO678 needs time to wake up)
            delay_us(100);
            
            current_cs_slot = slot;
            return 0;
        }
        else if (current_cs_slot == slot) {
            // Zaten seçili
            return 0;
        }
        else {
            // Farklı bir slot seçilmeye çalışılıyor - hata
            return -1;
        }
    }
    else {
        // CRITICAL: Delay before disabling CS (IO678 hold time)
        delay_us(50);
        
        // CS'i kapat (HIGH - deaktif)
        if (current_cs_slot == slot) {
            GPIO_SetBits(cs_pins[slot].gpio, cs_pins[slot].pin);
            current_cs_slot = -1;
            
            // Delay after CS disable
            delay_us(10);
            return 0;
        }
        else if (current_cs_slot == -1) {
            // Zaten kapalı
            return 0;
        }
        else {
            // Farklı bir slot aktif - hata
            return -1;
        }
    }
}

/**
 * SPI veri gönder (tek yönlü)
 * SPI2 kullanıyoruz
 */
void SPI_Send(spi_slot_t slot, uint8_t data) {
    // TXE bayrağını bekle (TX buffer boş)
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
    
    // Veriyi gönder
    SPI_I2S_SendData(SPI2, data);
}

/**
 * SPI veri alışverişi (iki yönlü)
 * SPI2 kullanıyoruz
 * 
 * CRITICAL FIX: Added inter-byte delay for iC-JX chip
 */
uint8_t SPI_DataExchange(spi_slot_t slot, uint8_t mosi) {
    // TXE bayrağını bekle (TX buffer boş)
    uint32_t timeout = 100000;
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET && timeout--);
    if (timeout == 0) return 0xFF; // Timeout protection
    
    // Veriyi gönder
    SPI_I2S_SendData(SPI2, mosi);
    
    // RXNE bayrağını bekle (RX buffer dolu)
    timeout = 100000;
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET && timeout--);
    if (timeout == 0) return 0xFF; // Timeout protection
    
    // Alınan veriyi oku
    uint8_t miso = (uint8_t)SPI_I2S_ReceiveData(SPI2);
    
    // CRITICAL: Inter-byte delay for IO678 chip processing time
    delay_us(20);
    
    return miso;
}

/**
 * SPI transfer (çoklu byte)
 */
int SPI_Transfer(spi_slot_t slot, const uint8_t* tx_data, uint8_t* rx_data, uint16_t length) {
    if (slot < 0 || slot > 4 || !tx_data || length == 0) {
        return -1;
    }
    
    for (uint16_t i = 0; i < length; i++) {
        uint8_t received = SPI_DataExchange(slot, tx_data[i]);
        
        if (rx_data) {
            rx_data[i] = received;
        }
    }
    
    return 0;
}
