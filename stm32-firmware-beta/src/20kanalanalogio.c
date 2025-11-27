/**
 * Burjuva Pilot - 20 Kanal Analog I/O Modül Kontrolü
 * AIO20 - 20 Channel Analog I/O Module
 * 
 * Hardware: MAX11300 PIXI (Programmable Mixed-Signal I/O)
 * Interface: SPI (shared with IO16/FPGA via CPLD multiplexer)
 * 
 * Özellikler:
 * - 20x Programmable ports (ADC/DAC/GPIO)
 * - 12-bit çözünürlük (0-4095)
 * - 0-10V analog range (MODE_7 ADC, MODE_5 DAC)
 * - SPI register-based control
 * 
 * Mevcut-sistem referansı: pilotfirmware/stm/files/aio20.c
 */

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "uart_helper.h"
#include "spisurucu.h"
#include "max11300_regs.h"
#include "aio20_afe.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>  // abs() için

// AIO20 modül durumu
typedef struct {
    uint8_t slot;                  // Modül slot numarası (0-3)
    uint16_t adc_values[20];       // ADC değerleri (port 0-19)
    uint16_t dac_values[20];       // DAC değerleri (port 0-19)
    uint8_t port_modes[20];        // Port modları (MODE_0-MODE_10)
    AIO20_AFE_Type afe_types[4];   // AFE kart tipleri (4 kart, her biri 4 kanal)
} AIO20_Module;

// Maksimum 4 slot
static AIO20_Module aio20_modules[4];
static uint8_t aio20_module_count = 0;

// Forward declarations
static int AIO20_WriteRegister(uint8_t slot, uint8_t reg, uint16_t data);
static int AIO20_ReadRegister(uint8_t slot, uint8_t reg, uint16_t* data);
static uint16_t AIO20_GetDeviceID(uint8_t slot);

/**
 * MAX11300 register yazma (SPI)
 * Format: [ADDR<<1 | WRITE_BIT] [DATA_MSB] [DATA_LSB]
 * return: 0=success, -1=error
 */
static int AIO20_WriteRegister(uint8_t slot, uint8_t reg, uint16_t data) {
    // CS enable
    SPI_SetCS(slot, CS_ENABLE);
    
    // Write address (addr << 1 | 0)
    uint8_t addr_byte = MAX11300_SPI_WRITE(reg);
    SPI_DataExchange(slot, addr_byte);
    
    // Write data MSB first
    SPI_DataExchange(slot, (data >> 8) & 0xFF);
    SPI_DataExchange(slot, data & 0xFF);
    
    // CS disable
    SPI_SetCS(slot, CS_DISABLE);
    
    return 0;
}

/**
 * MAX11300 register okuma (SPI)
 * Format: [ADDR<<1 | READ_BIT] [DUMMY/MSB] [DUMMY/LSB]
 * return: 0=success, -1=error
 */
static int AIO20_ReadRegister(uint8_t slot, uint8_t reg, uint16_t* data) {
    if (!data) return -1;
    
    // CS enable
    SPI_SetCS(slot, CS_ENABLE);
    
    // Write read address (addr << 1 | 1)
    uint8_t addr_byte = MAX11300_SPI_READ(reg);
    SPI_DataExchange(slot, addr_byte);
    
    // Read data MSB first
    uint8_t msb = SPI_DataExchange(slot, 0x00);
    uint8_t lsb = SPI_DataExchange(slot, 0x00);
    
    // CS disable
    SPI_SetCS(slot, CS_DISABLE);
    
    *data = ((uint16_t)msb << 8) | lsb;
    return 0;
}

/**
 * Device ID okuma (0x0424 olmalı)
 */
static uint16_t AIO20_GetDeviceID(uint8_t slot) {
    uint16_t dev_id = 0;
    AIO20_ReadRegister(slot, MAX11300_REG_DEV_ID, &dev_id);
    return dev_id;
}

/**
 * AIO20 modülünü kaydet
 */
void AIO20_Register(uint8_t slot) {
    if (slot < 4 && aio20_module_count < 4) {
        aio20_modules[aio20_module_count].slot = slot;
        
        // Başlangıç değerleri
        for (uint8_t i = 0; i < 20; i++) {
            aio20_modules[aio20_module_count].adc_values[i] = 0;
            aio20_modules[aio20_module_count].dac_values[i] = 0;
            aio20_modules[aio20_module_count].port_modes[i] = 0;  // MODE_0 (High-Z)
        }
        
        aio20_module_count++;
    }
}

/**
 * Slot'a göre modül bul
 */
static AIO20_Module* AIO20_GetModule(uint8_t slot) {
    for (uint8_t i = 0; i < aio20_module_count; i++) {
        if (aio20_modules[i].slot == slot) {
            return &aio20_modules[i];
        }
    }
    return NULL;
}

/**
 * MAX11300 chip initialization
 * Mevcut-sistem referansı: AIO20_init()
 * 
 * Default yapılandırma:
 * - Port 0-9: MODE_7 (ADC input, 0-10V)
 * - Port 10-19: MODE_5 (DAC output, 0-10V)
 */
int AIO20_ChipInit(uint8_t slot) {
    uint16_t dev_id;
    
    UART_SendString("\r\n");
    UART_SendString("====================================\r\n");
    UART_SendString("[MAX11300-INIT] Slot ");
    UART_SendHex8(slot);
    UART_SendString(" - Initializing PIXI chip...\r\n");
    UART_SendString("====================================\r\n");
    
    // STEP 0: Read Device ID
    UART_SendString("[MAX11300-INIT] Step 0: Reading Device ID...\r\n");
    dev_id = AIO20_GetDeviceID(slot);
    
    UART_SendString("[MAX11300-INIT] Device ID = 0x");
    UART_SendHex16(dev_id);
    UART_SendString("\r\n");
    
    if (dev_id != MAX11300_DEVICE_ID) {
        UART_SendString("[MAX11300-INIT] ERROR: Invalid Device ID (expected 0x0424)!\r\n");
        return -1;
    }
    UART_SendString("[MAX11300-INIT] Device ID OK!\r\n");
    
    // STEP 1: Configure device control (continuous ADC conversion mode)
    UART_SendString("[MAX11300-INIT] Step 1: Configuring device control...\r\n");
    uint16_t device_ctrl = MAX11300_ADCCONV_CONTINUOUS;  // Continuous ADC sweep
    AIO20_WriteRegister(slot, MAX11300_REG_DEVICE_CONTROL, device_ctrl);
    UART_SendString("[MAX11300-INIT] Device control configured - OK!\r\n");
    
    // STEP 2: Configure ALL ports 0-19 as ADC inputs (MODE_7: 0-10V)
    // Mevcut-sistem referansı: Tüm portlar 0x71e0 (MODE_7 + config bits)
    // Port mapping: io_to_port array ile physical IO → logical port
    UART_SendString("[MAX11300-INIT] Step 2: Configuring ALL 20 ports as ADC (MODE_7)...\r\n");
    for (uint8_t port = 0; port < 20; port++) {
        // 0x71e0 = MODE_7 (bits 15-12: 0111) + configuration bits
        // Matches mevcut-sistem configuration
        uint16_t port_cfg = 0x71e0;  // MODE_7 ADC, 0-10V + extra config
        AIO20_WriteRegister(slot, MAX11300_REG_PORT_CFG_00 + port, port_cfg);
    }
    UART_SendString("[MAX11300-INIT] All 20 ports configured as ADC - OK!\r\n");
    
    // Save port modes in module structure
    AIO20_Module* module = AIO20_GetModule(slot);
    if (module) {
        for (uint8_t i = 0; i < 20; i++) {
            module->port_modes[i] = 7;  // MODE_7 (ADC)
        }
    }
    
    UART_SendString("====================================\r\n");
    UART_SendString("[MAX11300-INIT] Chip ready for operation!\r\n");
    UART_SendString("====================================\r\n\r\n");
    
    return 0;
}

/**
 * ADC port okuma (MODE_7 configured ports)
 * port: 0-19
 * return: 0-4095 (12-bit), -1=error
 */
int AIO20_ReadADC(uint8_t slot, uint8_t port) {
    if (port >= 20) return -1;
    
    uint16_t adc_data = 0;
    if (AIO20_ReadRegister(slot, MAX11300_REG_ADC_DATA_PORT_00 + port, &adc_data) != 0) {
        return -1;
    }
    
    // MAX11300 ADC is 12-bit, right-aligned
    adc_data &= 0x0FFF;
    
    // Cache the value
    AIO20_Module* module = AIO20_GetModule(slot);
    if (module) {
        module->adc_values[port] = adc_data;
    }
    
    return adc_data;
}

/**
 * DAC port yazma - DISABLED (Tüm portlar ADC modunda)
 * Mevcut-sistem: DAC yok, tüm portlar ADC olarak yapılandırılmış
 * 
 * NOT: DAC fonksiyonu kullanılmıyor çünkü MAX11300 tüm portları
 * ADC (MODE_7) modunda. Eğer DAC gerekirse, init kodunu değiştir
 * ve belirli portları MODE_5 (AOUT) yap.
 */
int AIO20_WriteDAC(uint8_t slot, uint8_t port, uint16_t value) {
    // DAC disabled - all ports in ADC mode
    UART_SendString("ERROR: DAC not available (all ports in ADC mode)\r\n");
    return -1;
    
    /* ORIGINAL CODE (if DAC needed):
    if (port >= 20) return -1;
    value &= 0x0FFF;
    if (AIO20_WriteRegister(slot, MAX11300_REG_DAC_DATA_PORT_00 + port, value) != 0) {
        return -1;
    }
    AIO20_Module* module = AIO20_GetModule(slot);
    if (module) {
        module->dac_values[port] = value;
    }
    return 0;
    */
}

/**
 * Tüm ADC portları oku (block read for efficiency)
 */
int AIO20_ReadAllADC(uint8_t slot, uint16_t* values) {
    if (!values) return -1;
    
    for (uint8_t port = 0; port < 20; port++) {
        int result = AIO20_ReadADC(slot, port);
        if (result >= 0) {
            values[port] = (uint16_t)result;
        } else {
            values[port] = 0;
        }
    }
    
    return 0;
}

/**
 * 12-bit değeri voltage'a çevir (0-10V)
 * return: mV cinsinden (0-10000)
 */
uint16_t AIO20_ToVoltage(uint16_t value) {
    // 4095 = 10V = 10000mV
    return (uint16_t)(((uint32_t)value * 10000) / 4095);
}

/**
 * Voltage'ı 12-bit değere çevir (0-10V)
 * voltage: mV cinsinden (0-10000)
 */
uint16_t AIO20_FromVoltage(uint16_t voltage_mv) {
    if (voltage_mv > 10000) voltage_mv = 10000;
    return (uint16_t)(((uint32_t)voltage_mv * 4095) / 10000);
}

/**
 * AFE (Analog Front-End) kartlarını algıla
 * 
 * ÖNEMLI: Physical IO to Logical Port Mapping!
 * Mevcut-sistem referansı: io_to_port[20] array
 * 
 * AFE Algılama Portları (Physical IO → MAX11300 Port):
 * - IO16 → Port 4  (AFE0: CH0-3)
 * - IO17 → Port 7  (AFE1: CH4-7)
 * - IO18 → Port 12 (AFE2: CH8-11)
 * - IO19 → Port 17 (AFE3: CH12-15)
 * 
 * Algılama Değerleri:
 * - >4000: 4-20mA kartı
 * - 980-1180: 0-10V kartı
 * - 2060-2260: PT-1000 kartı
 * - Diğer: Kart yok
 */
void AIO20_DetectAFECards(uint8_t slot) {
    AIO20_Module* module = AIO20_GetModule(slot);
    if (!module) return;
    
    // Physical IO to MAX11300 Port mapping (mevcut-sistem'den)
    const uint8_t afe_detect_ports[4] = {4, 7, 12, 17};  // IO16→4, IO17→7, IO18→12, IO19→17
    
    UART_SendString("\r\n");
    UART_SendString("====================================\r\n");
    UART_SendString("[AIO20-AFE] AFE Kart Algılama\r\n");
    UART_SendString("[AIO20-AFE] Physical IO16-19 → Port 4,7,12,17\r\n");
    UART_SendString("====================================\r\n");
    
    // 4 AFE kartı algıla
    for (uint8_t afe_card = 0; afe_card < 4; afe_card++) {
        uint8_t detect_port = afe_detect_ports[afe_card];
        uint8_t physical_io = 16 + afe_card;
        
        // Algılama portunu oku
        int adc_val = AIO20_ReadADC(slot, detect_port);
        
        // Debug: Raw değeri göster
        char debug_buf[96];
        sprintf(debug_buf, "[AIO20-AFE] AFE%d (IO%d→Port%d): Reading ADC...\r\n",
                afe_card, physical_io, detect_port);
        UART_SendString(debug_buf);
        
        if (adc_val < 0) {
            module->afe_types[afe_card] = AFE_TYPE_UNKNOWN;
            UART_SendString("[AIO20-AFE]   ERROR: ADC read failed!\r\n");
            continue;
        }
        
        // ADC değerine göre kart tipini belirle
        module->afe_types[afe_card] = AIO20_DetectAFE((uint16_t)adc_val);
        
        // Kanal aralığını hesapla
        uint8_t start_ch = afe_card * 4;
        uint8_t end_ch = start_ch + 3;
        
        // Sonucu yazdır
        char buf[96];
        sprintf(buf, "[AIO20-AFE]   AFE%d (CH%d-%d): ADC=%d → %s\r\n",
                afe_card, start_ch, end_ch, adc_val,
                AIO20_AFE_ToString(module->afe_types[afe_card]));
        UART_SendString(buf);
    }
    
    UART_SendString("====================================\r\n\r\n");
}

/**
 * Modül durumunu yazdır (AFE bilgisi dahil)
 */
void AIO20_PrintStatus(uint8_t slot) {
    AIO20_Module* module = AIO20_GetModule(slot);
    if (!module) {
        UART_SendString("Hata: Modül bulunamadı\r\n");
        return;
    }
    
    char buf[128];
    
    UART_SendString("\r\n");
    UART_SendString("============================================================\r\n");
    UART_SendString(" AIO20 - MAX11300 PIXI (20-Channel Analog I/O)\r\n");
    UART_SendString("============================================================\r\n");
    sprintf(buf, "Slot: %d\r\n", module->slot);
    UART_SendString(buf);
    UART_SendString("------------------------------------------------------------\r\n");
    
    // AFE Kartları ve ilgili kanallar
    for (uint8_t afe = 0; afe < 4; afe++) {
        uint8_t start_ch = afe * 4;
        uint8_t end_ch = start_ch + 3;
        AIO20_AFE_Type afe_type = module->afe_types[afe];
        
        // AFE Kart Başlığı
        UART_SendString("\r\n");
        sprintf(buf, "🎴 AFE%d KARTI: %s\r\n", afe, AIO20_AFE_ToString(afe_type));
        UART_SendString(buf);
        sprintf(buf, "   Kanallar: CH%d - CH%d (Physical IO %d - IO %d)\r\n",
                start_ch, end_ch, start_ch, end_ch);
        UART_SendString(buf);
        
        // Kart tipine göre açıklama
        if (afe_type == AFE_TYPE_0_10V) {
            UART_SendString("   Tip: Voltaj Girişi (0-10V Analog Input)\r\n");
            UART_SendString("   Kullanım: Sensör okuma, PLC sinyalleri\r\n");
        } else if (afe_type == AFE_TYPE_4_20MA) {
            UART_SendString("   Tip: Akım Girişi (4-20mA Analog Input)\r\n");
            UART_SendString("   Kullanım: Endüstriyel sensörler, flow meter\r\n");
        } else if (afe_type == AFE_TYPE_PT1000) {
            UART_SendString("   Tip: Sıcaklık Sensörü (PT-1000 RTD Input)\r\n");
            UART_SendString("   Kullanım: Hassas sıcaklık ölçümü\r\n");
        } else {
            UART_SendString("   Tip: Boş (Kart takılı değil)\r\n");
            UART_SendString("   Kanallar kullanılabilir değil\r\n");
            continue;  // Boş kart ise kanal değerlerini gösterme
        }
        
        UART_SendString("   ----------------------------------------------------\r\n");
        
        // Bu AFE'ye ait kanalların değerlerini göster
        if (afe_type == AFE_TYPE_0_10V) {
            // 0-10V: Doğrudan voltaj göster
            UART_SendString("   Kanal    ADC Raw    Voltaj      Durum\r\n");
            UART_SendString("   -----    -------    -------     -----\r\n");
            
            for (uint8_t ch = start_ch; ch <= end_ch; ch++) {
                int raw = AIO20_ReadADC(slot, ch);
                if (raw < 0) raw = 0;
                uint16_t voltage = AIO20_ToVoltage(raw);
                
                sprintf(buf, "   CH%-2d     %4d       %2d.%03dV     %s\r\n",
                        ch, raw, voltage / 1000, voltage % 1000,
                        (raw > 100) ? "AKTIF" : "Düşük");
                UART_SendString(buf);
            }
            
        } else if (afe_type == AFE_TYPE_4_20MA) {
            // 4-20mA: Akım değeri göster (ADC değerinden hesaplanan)
            UART_SendString("   Kanal    ADC Raw    Akım        Durum\r\n");
            UART_SendString("   -----    -------    -------     -----\r\n");
            
            for (uint8_t ch = start_ch; ch <= end_ch; ch++) {
                int raw = AIO20_ReadADC(slot, ch);
                if (raw < 0) raw = 0;
                // 4-20mA → 12-bit ADC: 4mA ≈ 1638, 20mA ≈ 4095
                // Current (mA) = 4 + (raw - 1638) * 16 / (4095 - 1638)
                int current_ma_x10 = 40 + ((raw > 1638) ? ((raw - 1638) * 160 / 2457) : 0);
                
                sprintf(buf, "   CH%-2d     %4d       %2d.%dmA     %s\r\n",
                        ch, raw, current_ma_x10 / 10, current_ma_x10 % 10,
                        (raw > 1638) ? "AKTIF" : "Açık");
                UART_SendString(buf);
            }
            
        } else if (afe_type == AFE_TYPE_PT1000) {
            // PT-1000: Direnç → Sıcaklık dönüşümü (basitleştirilmiş)
            UART_SendString("   Kanal    ADC Raw    Sıcaklık    Durum\r\n");
            UART_SendString("   -----    -------    --------    -----\r\n");
            
            for (uint8_t ch = start_ch; ch <= end_ch; ch++) {
                int raw = AIO20_ReadADC(slot, ch);
                if (raw < 0) raw = 0;
                // PT-1000 yaklaşık: 1000Ω @ 0°C, ~3.85Ω/°C
                // Basit linear yaklaşım: Temp ≈ (raw - 2048) / 10
                int temp_x10 = (raw - 2048) * 10 / 100;  // Yaklaşık
                
                sprintf(buf, "   CH%-2d     %4d       %+3d.%d°C     %s\r\n",
                        ch, raw, temp_x10 / 10, abs(temp_x10 % 10),
                        (raw > 1800 && raw < 2400) ? "Normal" : "Hata?");
                UART_SendString(buf);
            }
        }
    }
    
    UART_SendString("\r\n============================================================\r\n");
    UART_SendString("💡 İpucu: 'aio20:SLOT:read:KANAL' ile tek kanal okuyabilirsin\r\n");
    UART_SendString("============================================================\r\n");
}

/**
 * Modül bilgilerini göster
 */
void AIO20_PrintInfo(uint8_t slot) {
    uint16_t dev_id = AIO20_GetDeviceID(slot);
    
    UART_SendString("\r\n");
    UART_SendString("====================================\r\n");
    UART_SendString(" AIO20 - Chip Information\r\n");
    UART_SendString("====================================\r\n");
    UART_SendString("Slot: ");
    UART_SendHex8(slot);
    UART_SendString("\r\n");
    
    UART_SendString("Device ID: 0x");
    UART_SendHex16(dev_id);
    
    if (dev_id == MAX11300_DEVICE_ID) {
        UART_SendString(" (OK - MAX11300 PIXI)\r\n");
    } else {
        UART_SendString(" (ERROR - Invalid ID!)\r\n");
    }
    
    UART_SendString("Expected: 0x0424\r\n");
    UART_SendString("====================================\r\n");
}

/**
 * Modül komutunu işle
 * Format: aio20:SLOT:KOMUT
 * Örnekler:
 *   aio20:1:read:5          - Port 5 ADC oku
 *   aio20:1:write:15:2048   - Port 15 DAC yaz (2048 = ~5V)
 *   aio20:1:setvolt:12:5000 - Port 12'ye 5.000V yaz
 *   aio20:1:status          - Tüm portlar
 *   aio20:1:info            - Chip bilgisi
 *   aio20:1:init            - Manuel chip init
 */
void AIO20_HandleCommand(const char* cmd) {
    // ACK gönder
    UART_SendString("[ACK:aio20:");
    UART_SendString(cmd);
    UART_SendString("]\r\n");
    
    // Slot parse
    if (cmd[0] < '0' || cmd[0] > '3') {
        UART_SendString("Hata: Geçersiz slot (0-3)\r\n");
        return;
    }
    uint8_t slot = cmd[0] - '0';
    
    if (cmd[1] != ':') {
        UART_SendString("Hata: Format hatası\r\n");
        return;
    }
    cmd += 2;
    
    // Komut kontrolü
    if (strncmp(cmd, "read:", 5) == 0) {
        // read:PORT
        cmd += 5;
        uint8_t port = 0;
        
        // Port numarası parse
        while (*cmd >= '0' && *cmd <= '9') {
            port = port * 10 + (*cmd - '0');
            cmd++;
        }
        
        if (port >= 20) {
            UART_SendString("Hata: Geçersiz port (0-19)\r\n");
            return;
        }
        
        int value = AIO20_ReadADC(slot, port);
        if (value >= 0) {
            uint16_t voltage = AIO20_ToVoltage(value);
            
            char buf[64];
            sprintf(buf, "Port %d: Raw=%d, Voltage=%d.%03dV\r\n",
                    port, value, voltage / 1000, voltage % 1000);
            UART_SendString(buf);
        } else {
            UART_SendString("Hata: ADC okuma başarısız\r\n");
        }
    }
    else if (strncmp(cmd, "write:", 6) == 0) {
        // write:PORT:VALUE
        cmd += 6;
        uint8_t port = 0;
        uint16_t value = 0;
        
        // Port parse
        while (*cmd >= '0' && *cmd <= '9') {
            port = port * 10 + (*cmd - '0');
            cmd++;
        }
        
        if (*cmd != ':') {
            UART_SendString("Hata: Format hatası\r\n");
            return;
        }
        cmd++;
        
        // Value parse
        while (*cmd >= '0' && *cmd <= '9') {
            value = value * 10 + (*cmd - '0');
            cmd++;
        }
        
        if (port >= 20 || value > 4095) {
            UART_SendString("Hata: Geçersiz parametre\r\n");
            return;
        }
        
        if (AIO20_WriteDAC(slot, port, value) == 0) {
            uint16_t voltage = AIO20_ToVoltage(value);
            
            char buf[64];
            sprintf(buf, "OK: Port %d = %d (Voltage=%d.%03dV)\r\n",
                    port, value, voltage / 1000, voltage % 1000);
            UART_SendString(buf);
        } else {
            UART_SendString("Hata: DAC yazma başarısız\r\n");
        }
    }
    else if (strncmp(cmd, "setvolt:", 8) == 0) {
        // setvolt:PORT:VOLTAGE_MV
        cmd += 8;
        uint8_t port = 0;
        uint16_t voltage_mv = 0;
        
        // Port parse
        while (*cmd >= '0' && *cmd <= '9') {
            port = port * 10 + (*cmd - '0');
            cmd++;
        }
        
        if (*cmd != ':') {
            UART_SendString("Hata: Format hatası\r\n");
            return;
        }
        cmd++;
        
        // Voltage parse (mV)
        while (*cmd >= '0' && *cmd <= '9') {
            voltage_mv = voltage_mv * 10 + (*cmd - '0');
            cmd++;
        }
        
        if (port >= 20 || voltage_mv > 10000) {
            UART_SendString("Hata: Geçersiz parametre\r\n");
            return;
        }
        
        uint16_t value = AIO20_FromVoltage(voltage_mv);
        
        if (AIO20_WriteDAC(slot, port, value) == 0) {
            char buf[64];
            sprintf(buf, "OK: Port %d = %d.%03dV (Raw=%d)\r\n",
                    port, voltage_mv / 1000, voltage_mv % 1000, value);
            UART_SendString(buf);
        } else {
            UART_SendString("Hata: DAC yazma başarısız\r\n");
        }
    }
    else if (strcmp(cmd, "status") == 0) {
        AIO20_PrintStatus(slot);
    }
    else if (strcmp(cmd, "info") == 0) {
        AIO20_PrintInfo(slot);
    }
    else if (strcmp(cmd, "init") == 0) {
        if (AIO20_ChipInit(slot) == 0) {
            UART_SendString("OK: Chip initialized\r\n");
            // Init sonrası AFE kartlarını algıla
            AIO20_DetectAFECards(slot);
        } else {
            UART_SendString("Hata: Init failed\r\n");
        }
    }
    else if (strcmp(cmd, "detectafe") == 0) {
        AIO20_DetectAFECards(slot);
    }
    else {
        UART_SendString("Hata: Bilinmeyen komut\r\n");
        UART_SendString("Kullanım:\r\n");
        UART_SendString("  aio20:SLOT:read:PORT\r\n");
        UART_SendString("  aio20:SLOT:write:PORT:VALUE\r\n");
        UART_SendString("  aio20:SLOT:setvolt:PORT:MV\r\n");
        UART_SendString("  aio20:SLOT:status\r\n");
        UART_SendString("  aio20:SLOT:info\r\n");
        UART_SendString("  aio20:SLOT:init\r\n");
        UART_SendString("  aio20:SLOT:detectafe\r\n");
    }
}
