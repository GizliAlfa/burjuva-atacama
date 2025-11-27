/**
 * Burjuva Pilot - 16 Kanal Dijital I/O Modül Kontrolü
 * IO16 - 16 Channel Digital I/O Module
 * 
 * Hardware: IO16H01H (mevcut sistemden)
 * Interface: SPI üzerinden register kontrolü
 */

#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "uart_helper.h"
#include "spisurucu.h"
#include <string.h>
#include <stdio.h>

// iC-JX Register Adresleri - COMPLETE MAP (32 registers)
// INPUT/OUTPUT Registers
#define IO16_REG_INPUT_A                    0x00  // Port A input states (pins 0-7)
#define IO16_REG_INPUT_B                    0x01  // Port B input states (pins 8-15)
#define IO16_REG_CHANGE_A                   0x02  // Port A input change detection
#define IO16_REG_CHANGE_B                   0x03  // Port B input change detection
#define IO16_REG_INTERRUPT_A                0x04  // Port A interrupt status
#define IO16_REG_INTERRUPT_B                0x05  // Port B interrupt status

// OVERCURRENT Detection
#define IO16_REG_OVERCURRENT_NOTIFICATION_A 0x06  // Port A overcurrent notification
#define IO16_REG_OVERCURRENT_NOTIFICATION_B 0x07  // Port B overcurrent notification
#define IO16_REG_OVERCURRENT_STATUS_A       0x08  // Port A overcurrent status
#define IO16_REG_OVERCURRENT_STATUS_B       0x09  // Port B overcurrent status

// ADC Data
#define IO16_REG_AD_DATA_1                  0x0A  // ADC data channel 1
#define IO16_REG_AD_DATA_2                  0x0B  // ADC data channel 2

// OUTPUT Control
#define IO16_REG_OUTPUT_A                   0x0C  // Port A output states (pins 0-7)
#define IO16_REG_OUTPUT_B                   0x0D  // Port B output states (pins 8-15)
#define IO16_REG_PULSE_ENABLE_A             0x0E  // Port A pulse enable
#define IO16_REG_PULSE_ENABLE_B             0x0F  // Port B pulse enable

// INTERRUPT Enable
#define IO16_REG_IRQ_ENABLE_INPUTCHANGE_A   0x10  // Port A input change IRQ enable
#define IO16_REG_IRQ_ENABLE_INPUTCHANGE_B   0x11  // Port B input change IRQ enable
#define IO16_REG_IRQ_ENABLE_OVERCURRENT_A   0x12  // Port A overcurrent IRQ enable
#define IO16_REG_IRQ_ENABLE_OVERCURRENT_B   0x13  // Port B overcurrent IRQ enable

// CONTROL WORDS
#define IO16_REG_CONTROLWORD_1A             0x14  // Port A filter settings
#define IO16_REG_CONTROLWORD_1B             0x15  // Port B filter settings
#define IO16_REG_CONTROLWORD_2A             0x16  // Port A direction control (CRITICAL!)
#define IO16_REG_CONTROLWORD_2B             0x17  // Port B direction control (CRITICAL!)
#define IO16_REG_CONTROLWORD_3A             0x18  // Port A clock/current settings
#define IO16_REG_CONTROLWORD_3B             0x19  // Port B clock/current settings (clock enable)
#define IO16_REG_CONTROLWORD_4              0x1A  // IRQ reset and global settings
#define IO16_REG_CONTROLWORD_5              0x1B  // Additional control settings
#define IO16_REG_CONTROLWORD_6              0x1C  // Additional control settings

// INFO/TEST
#define IO16_REG_INFO                       0x1D  // Chip information register
#define IO16_REG_TEST1                      0x1E  // Test register 1
#define IO16_REG_TEST2                      0x1F  // Test register 2

// SPI Protokol Sabit değerleri
#define CTRL_BYTE               0x59
#define NOP_BYTE                0x0F

// IO16 modül durumu
typedef struct {
    uint8_t slot;               // Modül slot numarası (0-3)
    uint16_t input_state;       // Giriş durumları (16 bit)
    uint16_t output_state;      // Çıkış durumları (16 bit)
    uint16_t direction_mask;    // Yön: 1=çıkış, 0=giriş
} IO16_Module;

// Maksimum 4 slot
static IO16_Module io16_modules[4];
static uint8_t io16_module_count = 0;

// Forward declarations
static int IO16_SetDirection(uint8_t slot, uint8_t pin, uint8_t direction);
static int IO16_WriteRegister(uint8_t slot, uint8_t reg, uint8_t count, uint8_t* value);
static int IO16_ReadRegister(uint8_t slot, uint8_t reg, uint8_t count, uint8_t* value);
static uint8_t IO16_GetChipInfo(uint8_t slot);
static uint16_t IO16_CheckOvercurrent(uint8_t slot);
static void IO16_DumpRegisters(uint8_t slot);

/**
 * iC-JX Chip Info Read
 * Returns chip identification and version from INFO register (0x1D)
 */
static uint8_t IO16_GetChipInfo(uint8_t slot) {
    uint8_t info = 0xFF;
    
    if (IO16_ReadRegister(slot, IO16_REG_INFO, 1, &info) == 0) {
        UART_SendString("[CHIP-INFO] iC-JX INFO Register = 0x");
        UART_SendHex8(info);
        UART_SendString("\r\n");
        
        // INFO register decode
        if (info == 0x00) {
            UART_SendString("[CHIP-INFO] WARNING: Chip not responding!\r\n");
        } else if (info == 0xFF) {
            UART_SendString("[CHIP-INFO] WARNING: No chip or bus error!\r\n");
        } else {
            UART_SendString("[CHIP-INFO] iC-JX chip detected and responding!\r\n");
        }
    } else {
        UART_SendString("[CHIP-INFO] ERROR: Cannot read INFO register!\r\n");
    }
    
    return info;
}

/**
 * iC-JX Overcurrent Detection Check
 * Reads OVERCURRENT_STATUS registers and reports any overcurrent conditions
 * Returns 16-bit mask (bit=1 means overcurrent on that pin)
 */
static uint16_t IO16_CheckOvercurrent(uint8_t slot) {
    uint8_t over_a = 0, over_b = 0;
    
    // Read overcurrent status registers
    IO16_ReadRegister(slot, IO16_REG_OVERCURRENT_STATUS_A, 1, &over_a);
    IO16_ReadRegister(slot, IO16_REG_OVERCURRENT_STATUS_B, 1, &over_b);
    
    uint16_t overcurrent = ((uint16_t)over_b << 8) | over_a;
    
    if (overcurrent != 0) {
        UART_SendString("[OVERCURRENT] DETECTED on pins: 0x");
        UART_SendHex16(overcurrent);
        UART_SendString("\r\n");
        
        // Report individual pins
        for (uint8_t i = 0; i < 16; i++) {
            if (overcurrent & (1 << i)) {
                UART_SendString("[OVERCURRENT] Pin ");
                UART_SendHex8(i);
                UART_SendString(" has overcurrent!\r\n");
            }
        }
        
        // Clear overcurrent status by writing 1s
        IO16_WriteRegister(slot, IO16_REG_OVERCURRENT_STATUS_A, 1, &over_a);
        IO16_WriteRegister(slot, IO16_REG_OVERCURRENT_STATUS_B, 1, &over_b);
        UART_SendString("[OVERCURRENT] Status cleared.\r\n");
    } else {
        UART_SendString("[OVERCURRENT] No overcurrent detected - OK!\r\n");
    }
    
    return overcurrent;
}

/**
 * iC-JX Register Dump
 * Reads and displays all important registers for debugging
 */
static void IO16_DumpRegisters(uint8_t slot) {
    uint8_t reg_value;
    
    UART_SendString("\r\n");
    UART_SendString("====================================\r\n");
    UART_SendString(" iC-JX Register Dump - Slot ");
    UART_SendHex8(slot);
    UART_SendString("\r\n");
    UART_SendString("====================================\r\n");
    
    // Input registers
    UART_SendString("INPUT_A (0x00):        0x");
    IO16_ReadRegister(slot, IO16_REG_INPUT_A, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString("\r\n");
    
    UART_SendString("INPUT_B (0x01):        0x");
    IO16_ReadRegister(slot, IO16_REG_INPUT_B, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString("\r\n");
    
    // Output registers
    UART_SendString("OUTPUT_A (0x0C):       0x");
    IO16_ReadRegister(slot, IO16_REG_OUTPUT_A, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString("\r\n");
    
    UART_SendString("OUTPUT_B (0x0D):       0x");
    IO16_ReadRegister(slot, IO16_REG_OUTPUT_B, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString("\r\n");
    
    // Control words
    UART_SendString("CONTROLWORD_1A (0x14): 0x");
    IO16_ReadRegister(slot, IO16_REG_CONTROLWORD_1A, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString(" (Filter)\r\n");
    
    UART_SendString("CONTROLWORD_1B (0x15): 0x");
    IO16_ReadRegister(slot, IO16_REG_CONTROLWORD_1B, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString(" (Filter)\r\n");
    
    UART_SendString("CONTROLWORD_2A (0x16): 0x");
    IO16_ReadRegister(slot, IO16_REG_CONTROLWORD_2A, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString(" (Direction)\r\n");
    
    UART_SendString("CONTROLWORD_2B (0x17): 0x");
    IO16_ReadRegister(slot, IO16_REG_CONTROLWORD_2B, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString(" (Direction)\r\n");
    
    UART_SendString("CONTROLWORD_3A (0x18): 0x");
    IO16_ReadRegister(slot, IO16_REG_CONTROLWORD_3A, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString(" (Clock/Current)\r\n");
    
    UART_SendString("CONTROLWORD_3B (0x19): 0x");
    IO16_ReadRegister(slot, IO16_REG_CONTROLWORD_3B, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString(" (Clock/Current)\r\n");
    
    UART_SendString("CONTROLWORD_4 (0x1A):  0x");
    IO16_ReadRegister(slot, IO16_REG_CONTROLWORD_4, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString(" (IRQ)\r\n");
    
    // Overcurrent status
    UART_SendString("OVERCURRENT_STS_A (0x08): 0x");
    IO16_ReadRegister(slot, IO16_REG_OVERCURRENT_STATUS_A, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString("\r\n");
    
    UART_SendString("OVERCURRENT_STS_B (0x09): 0x");
    IO16_ReadRegister(slot, IO16_REG_OVERCURRENT_STATUS_B, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString("\r\n");
    
    // Chip INFO
    UART_SendString("INFO (0x1D):           0x");
    IO16_ReadRegister(slot, IO16_REG_INFO, 1, &reg_value);
    UART_SendHex8(reg_value);
    UART_SendString(" (Chip ID)\r\n");
    
    UART_SendString("====================================\r\n\r\n");
}

/**
 * iC-JX Chip Initialization - UPDATED FOR iC-JX
 * Init sequence for iC-JX high-side driver:
 * 0. Read chip INFO to verify presence
 * 1. Disable all interrupts initially
 * 2. Internal clock enable + stabilize (0x0F to CONTROLWORD_3B)
 * 3. Set all pins to OUTPUT (CONTROLWORD_2A/2B = 0x88)
 * 4. Clear outputs (OUTPUT_A/B = 0x00)
 * 5. Filter bypass (CONTROLWORD_1A/1B = 0x88)
 * 6. IRQ reset (CONTROLWORD_4 = 0x80)
 * 7. Clear overcurrent status
 * 8. Verify by reading INPUT registers
 */
int IO16_ChipInit(uint8_t slot) {
    uint8_t value;
    int ret = 0;
    
    UART_SendString("\r\n");
    UART_SendString("====================================\r\n");
    UART_SendString("[iC-JX-INIT] Slot ");
    UART_SendHex8(slot);
    UART_SendString(" - Initializing iC-JX chip...\r\n");
    UART_SendString("====================================\r\n");
    
    // STEP 0: Read chip INFO to verify presence
    UART_SendString("[iC-JX-INIT] Step 0: Reading chip INFO...\r\n");
    uint8_t chip_info = IO16_GetChipInfo(slot);
    if (chip_info == 0x00 || chip_info == 0xFF) {
        UART_SendString("[iC-JX-INIT] WARNING: Chip may not be present or not responding!\r\n");
        UART_SendString("[iC-JX-INIT] Continuing with init anyway...\r\n");
    }
    
    // STEP 1: Disable all interrupts initially
    UART_SendString("[iC-JX-INIT] Step 1: Disabling all interrupts...\r\n");
    value = 0x00;
    IO16_WriteRegister(slot, IO16_REG_IRQ_ENABLE_INPUTCHANGE_A, 1, &value);
    IO16_WriteRegister(slot, IO16_REG_IRQ_ENABLE_INPUTCHANGE_B, 1, &value);
    IO16_WriteRegister(slot, IO16_REG_IRQ_ENABLE_OVERCURRENT_A, 1, &value);
    IO16_WriteRegister(slot, IO16_REG_IRQ_ENABLE_OVERCURRENT_B, 1, &value);
    UART_SendString("[iC-JX-INIT] Interrupts disabled - OK!\r\n");
    
    // STEP 2: Enable internal clock (CONTROLWORD_3B = 0x05)
    // This is critical - chip won't work without clock
    UART_SendString("[iC-JX-INIT] Step 2: Enabling internal clock (0x05)...\r\n");
    value = 0x05;  // Internal clock enable
    ret = IO16_WriteRegister(slot, IO16_REG_CONTROLWORD_3B, 1, &value);
    if (ret != 0) {
        UART_SendString("[iC-JX-INIT] ERROR: Clock enable FAILED!\r\n");
        return ret;
    }
    UART_SendString("[iC-JX-INIT] Clock enabled - OK!\r\n");
    
    // STEP 3: Enable IO filter bypass (CONTROLWORD_1A/1B = 0x88)
    // This improves response time for outputs
    UART_SendString("[iC-JX-INIT] Step 3: Enabling IO filter bypass (0x88)...\r\n");
    value = 0x88;
    
    ret = IO16_WriteRegister(slot, IO16_REG_CONTROLWORD_1A, 1, &value);
    if (ret != 0) {
        UART_SendString("[iC-JX-INIT] WARNING: CONTROLWORD_1A bypass failed\r\n");
    }
    
    ret = IO16_WriteRegister(slot, IO16_REG_CONTROLWORD_1B, 1, &value);
    if (ret != 0) {
        UART_SendString("[iC-JX-INIT] WARNING: CONTROLWORD_1B bypass failed\r\n");
    } else {
        UART_SendString("[iC-JX-INIT] Filter bypass enabled - OK!\r\n");
    }
    
    // STEP 4: Reset interrupt state (CONTROLWORD_4 = 0x80)
    // Clear any pending interrupts
    UART_SendString("[iC-JX-INIT] Step 4: Resetting interrupt state (EOI)...\r\n");
    value = 0x80;
    ret = IO16_WriteRegister(slot, IO16_REG_CONTROLWORD_4, 1, &value);
    if (ret != 0) {
        UART_SendString("[iC-JX-INIT] WARNING: IRQ reset failed\r\n");
    } else {
        UART_SendString("[iC-JX-INIT] IRQ reset - OK!\r\n");
    }
    
    // STEP 5: Enable input change interrupts (optional, for input monitoring)
    // This allows the chip to detect input changes
    UART_SendString("[iC-JX-INIT] Step 5: Enabling input change interrupts...\r\n");
    value = 0xFF;  // Enable all pins for input change detection
    ret = IO16_WriteRegister(slot, IO16_REG_IRQ_ENABLE_INPUTCHANGE_A, 1, &value);
    ret |= IO16_WriteRegister(slot, IO16_REG_IRQ_ENABLE_INPUTCHANGE_B, 1, &value);
    if (ret != 0) {
        UART_SendString("[iC-JX-INIT] WARNING: Input change IRQ enable failed\r\n");
    } else {
        UART_SendString("[iC-JX-INIT] Input change IRQ enabled - OK!\r\n");
    }
    
    // NOTE: Direction (CONTROLWORD_2A/B) is NOT set here!
    // Direction will be set automatically by IO16_SetPin() when pins are used
    // This prevents the 24V stuck issue during init
    UART_SendString("[iC-JX-INIT] Direction will be set by IO16_SetPin() commands\r\n");
    
    // STEP 8: Verify by reading INPUT registers
    UART_SendString("[iC-JX-INIT] Step 8: Verifying chip response...\r\n");
    uint8_t test_a = 0xAA, test_b = 0xBB;
    
    if (IO16_ReadRegister(slot, IO16_REG_INPUT_A, 1, &test_a) == 0) {
        UART_SendString("[iC-JX-INIT] INPUT_A = 0x");
        UART_SendHex8(test_a);
        UART_SendString("\r\n");
    }
    
    if (IO16_ReadRegister(slot, IO16_REG_INPUT_B, 1, &test_b) == 0) {
        UART_SendString("[iC-JX-INIT] INPUT_B = 0x");
        UART_SendHex8(test_b);
        UART_SendString("\r\n");
    }
    
    // Check for overcurrent after init
    UART_SendString("[iC-JX-INIT] Step 9: Checking for overcurrent...\r\n");
    IO16_CheckOvercurrent(slot);
    
    UART_SendString("====================================\r\n");
    UART_SendString("[iC-JX-INIT] Chip ready for operation!\r\n");
    UART_SendString("====================================\r\n\r\n");
    
    return 0;
}

/**
 * IO16 modülünü kaydet
 */
void IO16_Register(uint8_t slot) {
    if (slot < 4 && io16_module_count < 4) {
        io16_modules[io16_module_count].slot = slot;
        io16_modules[io16_module_count].input_state = 0;
        io16_modules[io16_module_count].output_state = 0;
        io16_modules[io16_module_count].direction_mask = 0x0000;  // Tümü giriş
        io16_module_count++;
    }
}

/**
 * Slot'a göre modül bul
 */
static IO16_Module* IO16_GetModule(uint8_t slot) {
    for (uint8_t i = 0; i < io16_module_count; i++) {
        if (io16_modules[i].slot == slot) {
            return &io16_modules[i];
        }
    }
    return NULL;
}

/**
 * SPI Address Byte Oluştur
 * Format: BA[7:6] | RA[5:1] | RNW[0]
 * ba: base address (0-3), normally 0
 * ra: register address (0-31)
 * rnw: 0=write, 1=read
 */
static uint8_t get_address_byte(uint8_t ra, uint8_t rnw) {
    uint8_t ba = 0;  // Base address = 0 (normal mode)
    return (ba << 6) | (ra << 1) | (rnw & 0x01);
}

/**
 * SPI Count Byte Oluştur
 * count: transfer edilecek byte sayısı (1-16)
 */
static uint8_t get_count_byte(uint8_t count) {
    return ((count - 1) << 4) | (0x0F & ~(count - 1));
}

// Forward declarations
static int IO16_WriteRegister(uint8_t slot, uint8_t reg, uint8_t count, uint8_t* value);

/**
 * iC-JX Chip Initialization
 * 
 * CRITICAL: Bu fonksiyon chip kullanılmadan önce MUTLAKA çağrılmalı!
 * 
 * Mevcut-sistem referansı:
 * - CONTROLWORD_3B = 0x05 → Internal clock enable
 * - CONTROLWORD_1A = 0x88 → IO filter bypass (pins 0-7)
 * - CONTROLWORD_1B = 0x88 → IO filter bypass (pins 8-15)
 * - CONTROLWORD_4 = 0x80 → Reset EOI (End of Interrupt)
 * 
 * @param slot: Slot numarası (0-3)
 * @return 0: başarılı, -1: hata
 */
static int IO16_InitChip(uint8_t slot) {
    UART_SendString("\r\n========================================\r\n");
    UART_SendString("iC-JX CHIP INITIALIZATION - Slot ");
    UART_SendHex8(slot);
    UART_SendString("\r\n========================================\r\n");
    
    uint8_t value;
    
    // 1. Internal Clock Enable (CONTROLWORD_3B = 0x85)
    // 0x05 = Internal clock enable
    // 0x80 = Bit 7 (possible OUTPUT driver enable!)
    UART_SendString("[INIT] Step 1: Enable internal clock + OUTPUT drivers (CW3B=0x85)...\r\n");
    value = 0x85;  // TEST: Trying 0x85 instead of 0x05 (bit 7 = OUTPUT enable?)
    if (IO16_WriteRegister(slot, IO16_REG_CONTROLWORD_3B, 1, &value) != 0) {
        UART_SendString("[INIT] ❌ FAILED: Internal clock enable\r\n");
        return -1;
    }
    UART_SendString("[INIT] ✅ Internal clock + OUTPUT drivers enabled (0x85)\r\n");
    
    // 2. IO Filter Bypass Enable - Port A (CONTROLWORD_1A = 0x88)
    UART_SendString("[INIT] Step 2: Bypass IO filter Port A (CW1A=0x88)...\r\n");
    value = 0x88;
    if (IO16_WriteRegister(slot, IO16_REG_CONTROLWORD_1A, 1, &value) != 0) {
        UART_SendString("[INIT] ❌ FAILED: Filter bypass Port A\r\n");
        return -1;
    }
    UART_SendString("[INIT] ✅ Filter bypass Port A\r\n");
    
    // 3. IO Filter Bypass Enable - Port B (CONTROLWORD_1B = 0x88)
    UART_SendString("[INIT] Step 3: Bypass IO filter Port B (CW1B=0x88)...\r\n");
    value = 0x88;
    if (IO16_WriteRegister(slot, IO16_REG_CONTROLWORD_1B, 1, &value) != 0) {
        UART_SendString("[INIT] ❌ FAILED: Filter bypass Port B\r\n");
        return -1;
    }
    UART_SendString("[INIT] ✅ Filter bypass Port B\r\n");
    
    // 4. Reset EOI (End of Interrupt) - CONTROLWORD_4 = 0x80
    UART_SendString("[INIT] Step 4: Reset EOI (CW4=0x80)...\r\n");
    value = 0x80;
    if (IO16_WriteRegister(slot, IO16_REG_CONTROLWORD_4, 1, &value) != 0) {
        UART_SendString("[INIT] ❌ FAILED: Reset EOI\r\n");
        return -1;
    }
    UART_SendString("[INIT] ✅ EOI reset complete\r\n");
    
    UART_SendString("\r\n[INIT] 🎉 iC-JX INITIALIZATION COMPLETE!\r\n");
    UART_SendString("========================================\r\n\r\n");
    
    return 0;
}

/**
 * IO16 Register Yaz (SPI)
 * @param slot: Slot numarası (0-3)
 * @param reg: Register adresi
 * @param count: Yazılacak byte sayısı
 * @param value: Yazılacak değerler
 * @return 0: başarılı, -1: hata
 */
static int IO16_WriteRegister(uint8_t slot, uint8_t reg, uint8_t count, uint8_t* value) {
    uint8_t address_byte = get_address_byte(reg, 0);  // Write
    uint8_t count_byte = get_count_byte(count);
    uint8_t miso;
    
    // DEBUG
    UART_SendString("[SPI-WR] Slot=");
    UART_SendHex8(slot);
    UART_SendString(" Reg=0x");
    UART_SendHex8(reg);
    UART_SendString(" Count=");
    UART_SendHex8(count);
    UART_SendString("\r\n");
    
    // CS'i aktif et
    if (SPI_SetCS(slot, CS_ENABLE) != 0) {
        UART_SendString("[SPI-WR] CS Enable FAILED!\r\n");
        return -1;
    }
    
    // 1. Adres gönder
    miso = SPI_DataExchange(slot, address_byte);
    UART_SendString("[SPI-WR] Sent addr=0x");
    UART_SendHex8(address_byte);
    UART_SendString(" Got=0x");
    UART_SendHex8(miso);
    UART_SendString("\r\n");
    
    // 2. Count gönder
    miso = SPI_DataExchange(slot, count_byte);
    UART_SendString("[SPI-WR] Sent count=0x");
    UART_SendHex8(count_byte);
    UART_SendString(" Got=0x");
    UART_SendHex8(miso);
    
    // Adres echo kontrolü
    if (miso != address_byte) {
        UART_SendString(" ADDR_ECHO_FAIL!\r\n");
        SPI_SetCS(slot, CS_DISABLE);
        return -1;
    }
    UART_SendString(" OK\r\n");
    
    // 3. Data gönder
    for (uint8_t i = 0; i < count; i++) {
        miso = SPI_DataExchange(slot, value[i]);
        UART_SendString("[SPI-WR] Data[");
        UART_SendHex8(i);
        UART_SendString("]=0x");
        UART_SendHex8(value[i]);
        UART_SendString(" Got=0x");
        UART_SendHex8(miso);
        
        if (i == 0) {
            // İlk byte'ta count echo olmalı
            if (miso != count_byte) {
                UART_SendString(" COUNT_ECHO_FAIL!\r\n");
                SPI_SetCS(slot, CS_DISABLE);
                return -1;
            }
        }
        UART_SendString("\r\n");
    }
    
    // 4. Son adres gönder
    uint8_t end_address = get_address_byte(reg + count - 1, 0);
    miso = SPI_DataExchange(slot, end_address);
    UART_SendString("[SPI-WR] End_addr=0x");
    UART_SendHex8(end_address);
    UART_SendString(" Got=0x");
    UART_SendHex8(miso);
    
    // Son veri echo kontrolü
    if (miso != value[count - 1]) {
        UART_SendString(" DATA_ECHO_FAIL!\r\n");
        SPI_SetCS(slot, CS_DISABLE);
        return -1;
    }
    UART_SendString(" OK\r\n");
    
    // 5. CTRL byte gönder
    miso = SPI_DataExchange(slot, CTRL_BYTE);
    UART_SendString("[SPI-WR] CTRL=0x59 Got=0x");
    UART_SendHex8(miso);
    
    // CTRL echo kontrolü
    if (miso != CTRL_BYTE) {
        UART_SendString(" CTRL_ECHO_FAIL!\r\n");
        SPI_SetCS(slot, CS_DISABLE);
        return -1;
    }
    UART_SendString(" OK\r\n[SPI-WR] SUCCESS!\r\n");
    
    // CS'i pasif et
    SPI_SetCS(slot, CS_DISABLE);
    
    return 0;
}

/**
 * IO16 Register Oku (SPI)
 * @param slot: Slot numarası (0-3)
 * @param reg: Register adresi
 * @param count: Okunacak byte sayısı
 * @param value: Okunan değerler (output)
 * @return 0: başarılı, -1: hata
 */
static int IO16_ReadRegister(uint8_t slot, uint8_t reg, uint8_t count, uint8_t* value) {
    uint8_t address_byte = get_address_byte(reg, 1);  // Read
    uint8_t count_byte = get_count_byte(count);
    uint8_t miso;
    
    // DEBUG
    UART_SendString("[SPI-RD] Slot=");
    UART_SendHex8(slot);
    UART_SendString(" Reg=0x");
    UART_SendHex8(reg);
    UART_SendString(" Count=");
    UART_SendHex8(count);
    UART_SendString("\r\n");
    
    // CS'i aktif et
    if (SPI_SetCS(slot, CS_ENABLE) != 0) {
        UART_SendString("[SPI-RD] CS Enable FAILED!\r\n");
        return -1;
    }
    
    // 1. Adres gönder
    miso = SPI_DataExchange(slot, address_byte);
    UART_SendString("[SPI-RD] Sent addr=0x");
    UART_SendHex8(address_byte);
    UART_SendString(" Got=0x");
    UART_SendHex8(miso);
    UART_SendString("\r\n");
    
    // 2. NOP gönder
    miso = SPI_DataExchange(slot, NOP_BYTE);
    UART_SendString("[SPI-RD] Sent NOP=0x0F Got=0x");
    UART_SendHex8(miso);
    
    // Adres echo kontrolü
    if (miso != address_byte) {
        UART_SendString(" ADDR_ECHO_FAIL!\r\n");
        SPI_SetCS(slot, CS_DISABLE);
        return -1;
    }
    UART_SendString(" OK\r\n");
    
    // 3. Count gönder
    miso = SPI_DataExchange(slot, count_byte);
    UART_SendString("[SPI-RD] Sent count=0x");
    UART_SendHex8(count_byte);
    UART_SendString(" Got=0x");
    UART_SendHex8(miso);
    UART_SendString("\r\n");
    
    // 4. Data oku
    for (uint8_t i = 0; i < count; i++) {
        value[i] = miso;
        
        UART_SendString("[SPI-RD] Data[");
        UART_SendHex8(i);
        UART_SendString("]=0x");
        UART_SendHex8(value[i]);
        
        // Echo gönder
        miso = SPI_DataExchange(slot, miso);
        UART_SendString(" Echo got=0x");
        UART_SendHex8(miso);
        UART_SendString("\r\n");
    }
    
    // 5. CTRL byte gönder
    miso = SPI_DataExchange(slot, CTRL_BYTE);
    UART_SendString("[SPI-RD] CTRL=0x59 Got=0x");
    UART_SendHex8(miso);
    
    // CTRL echo kontrolü
    if (miso != CTRL_BYTE) {
        UART_SendString(" CTRL_ECHO_FAIL!\r\n");
        SPI_SetCS(slot, CS_DISABLE);
        return -1;
    }
    UART_SendString(" OK\r\n[SPI-RD] SUCCESS!\r\n");
    
    // CS'i pasif et
    SPI_SetCS(slot, CS_DISABLE);
    
    return 0;
}

/**
 * Tek bir pin'i ayarla (0-15)
 * state: 0=LOW, 1=HIGH
 */
int IO16_SetPin(uint8_t slot, uint8_t pin, uint8_t state) {
    IO16_Module* module = IO16_GetModule(slot);
    if (!module || pin >= 16) {
        return -1;
    }
    
    // Yönü kontrol et - output değilse otomatik OUTPUT yap
    UART_SendString("[SetPin] Checking direction for pin ");
    UART_SendHex8(pin);
    UART_SendString("...\r\n");
    UART_SendString("[SetPin] Current direction_mask: 0x");
    UART_SendHex16(module->direction_mask);
    UART_SendString("\r\n");
    
    if (!(module->direction_mask & (1 << pin))) {
        UART_SendString("[SetPin] ⚠️  Pin ");
        UART_SendHex8(pin);
        UART_SendString(" is INPUT! Auto-setting to OUTPUT...\r\n");
        if (IO16_SetDirection(slot, pin, 1) != 0) {
            UART_SendString("[SetPin] ❌ Direction change FAILED!\r\n");
            return -1;
        }
        UART_SendString("[SetPin] ✅ Direction changed to OUTPUT\r\n");
    } else {
        UART_SendString("[SetPin] ✓ Pin ");
        UART_SendHex8(pin);
        UART_SendString(" is already OUTPUT\r\n");
    }
    
    // ⚠️  MEVCUT-SİSTEM YÖNTEM: Single-byte read/write (OUTPUT_A or OUTPUT_B)
    // Reference: pilot_io16_set_value() reads/writes 1 byte at a time
    // NOT: PilotConfig 2-byte kullanıyor ama set_value() 1-byte kullanıyor!
    
    UART_SendString("[SetPin] 📖 Reading OUTPUT register (");
    UART_SendString((pin < 8) ? "A" : "B");
    UART_SendString(")...\r\n");
    
    // Pin hangi register'da (A veya B)
    uint8_t reg = (pin < 8) ? IO16_REG_OUTPUT_A : IO16_REG_OUTPUT_B;
    uint8_t reg_value = 0;
    
    // Sadece ilgili register'ı oku (1 byte)
    if (IO16_ReadRegister(slot, reg, 1, &reg_value) != 0) {
        UART_SendString("[SetPin] ❌ Read FAILED!\r\n");
        return -1;
    }
    
    UART_SendString("[SetPin] Current value: 0x");
    UART_SendHex8(reg_value);
    UART_SendString("\r\n");
    
    // Pin değerini güncelle (8-bit içinde, pin % 8)
    uint8_t bit_mask = 1 << (pin % 8);
    uint8_t old_value = reg_value;
    
    if (state) {
        reg_value |= bit_mask;
    } else {
        reg_value &= ~bit_mask;
    }
    
    UART_SendString("[SetPin] New value: 0x");
    UART_SendHex8(reg_value);
    UART_SendString(" (bit ");
    UART_SendHex8(pin % 8);
    UART_SendString(state ? " SET" : " CLR");
    UART_SendString(")\r\n");
    
    // Değişiklik var mı kontrol et
    if (old_value == reg_value) {
        UART_SendString("[SetPin] ⚠️  No change needed!\r\n");
    }
    
    // Register'a geri yaz (1 byte - mevcut-sistem yöntemi!)
    UART_SendString("[SetPin] ✍️  Writing OUTPUT register (");
    UART_SendString((pin < 8) ? "A" : "B");
    UART_SendString(")...\r\n");
    
    if (IO16_WriteRegister(slot, reg, 1, &reg_value) != 0) {
        UART_SendString("[SetPin] ❌ Write FAILED!\r\n");
        return -1;
    }
    
    UART_SendString("[SetPin] ✅ Write SUCCESS! (value=0x");
    UART_SendHex8(reg_value);
    UART_SendString(")\r\n");
    
    // VERIFICATION: Register'ı tekrar oku ve kontrol et!
    UART_SendString("[SetPin] 🔍 VERIFICATION: Reading back OUTPUT register...\r\n");
    uint8_t verify_value = 0;
    if (IO16_ReadRegister(slot, reg, 1, &verify_value) == 0) {
        UART_SendString("[SetPin] Readback value: 0x");
        UART_SendHex8(verify_value);
        if (verify_value == reg_value) {
            UART_SendString(" ✅ MATCH!\r\n");
        } else {
            UART_SendString(" ❌ MISMATCH! Expected 0x");
            UART_SendHex8(reg_value);
            UART_SendString("\r\n");
            UART_SendString("[SetPin] ⚠️  WARNING: Register yazıldı ama verify başarısız!\r\n");
        }
    } else {
        UART_SendString("[SetPin] ⚠️  Readback FAILED!\r\n");
    }
    
    // Cache'i güncelle
    if (state) {
        module->output_state |= (1 << pin);
    } else {
        module->output_state &= ~(1 << pin);
    }
    
    return 0;
}

/**
 * Tek bir pin'i oku (0-15)
 * return: 0=LOW, 1=HIGH, -1=hata
 */
int IO16_GetPin(uint8_t slot, uint8_t pin) {
    IO16_Module* module = IO16_GetModule(slot);
    if (!module || pin >= 16) {
        return -1;
    }
    
    // Output pin ise cached değer döndür
    if (module->direction_mask & (1 << pin)) {
        return (module->output_state & (1 << pin)) ? 1 : 0;
    }
    
    // MEVCUT-SİSTEM: Single-byte read (INPUT_A or INPUT_B)
    
    // Pin hangi register'da (A veya B)
    uint8_t reg = (pin < 8) ? IO16_REG_INPUT_A : IO16_REG_INPUT_B;
    uint8_t reg_value = 0;
    
    // Sadece ilgili register'ı oku (1 byte)
    if (IO16_ReadRegister(slot, reg, 1, &reg_value) != 0) {
        return -1;
    }
    
    // Pin değerini çıkar (8-bit içinden, pin % 8)
    uint8_t bit_mask = 1 << (pin % 8);
    int state = (reg_value & bit_mask) ? 1 : 0;
    
    // Cache'i güncelle
    if (pin < 8) {
        module->input_state = (module->input_state & 0xFF00) | reg_value;
    } else {
        module->input_state = (module->input_state & 0x00FF) | (reg_value << 8);
    }
    
    return state;
}

/**
 * Pin yönünü ayarla (0-15)
 * direction: 0=INPUT, 1=OUTPUT
 * 
 * IO16'da yön 4'lü bloklar halinde ayarlanır:
 * - Block 0-3:   CONTROLWORD_2A bit 3
 * - Block 4-7:   CONTROLWORD_2A bit 7
 * - Block 8-11:  CONTROLWORD_2B bit 3
 * - Block 12-15: CONTROLWORD_2B bit 7
 */
int IO16_SetDirection(uint8_t slot, uint8_t pin, uint8_t direction) {
    IO16_Module* module = IO16_GetModule(slot);
    if (!module || pin >= 16) {
        return -1;
    }
    
    UART_SendString("\r\n[DIR] ========================================\r\n");
    UART_SendString("[DIR] SETTING DIRECTION - Pin ");
    UART_SendHex8(pin);
    UART_SendString(" → ");
    UART_SendString(direction ? "OUTPUT" : "INPUT");
    UART_SendString("\r\n[DIR] ========================================\r\n");
    
    // Control register'ı belirle (A veya B)
    uint8_t reg = (pin < 8) ? IO16_REG_CONTROLWORD_2A : IO16_REG_CONTROLWORD_2B;
    uint8_t reg_value;
    
    UART_SendString("[DIR] Target register: CONTROLWORD_2");
    UART_SendString((pin < 8) ? "A" : "B");
    UART_SendString(" (0x");
    UART_SendHex8(reg);
    UART_SendString(")\r\n");
    
    // Mevcut değeri oku
    UART_SendString("[DIR] Reading current direction register...\r\n");
    if (IO16_ReadRegister(slot, reg, 1, &reg_value) != 0) {
        UART_SendString("[DIR] ❌ Read FAILED!\r\n");
        return -1;
    }
    
    UART_SendString("[DIR] Current CONTROLWORD_2 value: 0x");
    UART_SendHex8(reg_value);
    UART_SendString("\r\n");
    
    // Block'a göre bit pozisyonunu belirle
    // Pin 0-3 veya 8-11: bit 3
    // Pin 4-7 veya 12-15: bit 7
    uint8_t bit_pos = ((pin % 8) < 4) ? 3 : 7;
    
    UART_SendString("[DIR] Control bit position: ");
    UART_SendHex8(bit_pos);
    UART_SendString(" (controls pins ");
    uint8_t block_start = (pin / 4) * 4;
    UART_SendHex8(block_start);
    UART_SendString("-");
    UART_SendHex8(block_start + 3);
    UART_SendString(")\r\n");
    
    // Direction bit'ini güncelle (1=OUTPUT, 0=INPUT)
    uint8_t old_value = reg_value;
    if (direction) {
        reg_value |= (1 << bit_pos);
    } else {
        reg_value &= ~(1 << bit_pos);
    }
    
    UART_SendString("[DIR] Old value: 0x");
    UART_SendHex8(old_value);
    UART_SendString(" → New value: 0x");
    UART_SendHex8(reg_value);
    UART_SendString("\r\n");
    
    if (old_value == reg_value) {
        UART_SendString("[DIR] ⚠️  No change needed (already ");
        UART_SendString(direction ? "OUTPUT" : "INPUT");
        UART_SendString(")\r\n");
    }
    
    // Register'a yaz
    UART_SendString("[DIR] Writing CONTROLWORD_2");
    UART_SendString((pin < 8) ? "A" : "B");
    UART_SendString("...\r\n");
    
    if (IO16_WriteRegister(slot, reg, 1, &reg_value) != 0) {
        UART_SendString("[DIR] ❌ Write FAILED!\r\n");
        return -1;
    }
    
    UART_SendString("[DIR] ✅ Direction register write SUCCESS!\r\n");
    
    // Cache'i güncelle (4'lü blok olarak - block_start zaten line 918'de tanımlı)
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t block_pin = block_start + i;
        if (direction) {
            module->direction_mask |= (1 << block_pin);
        } else {
            module->direction_mask &= ~(1 << block_pin);
        }
    }
    
    return 0;
}

/**
 * Tüm pinleri oku (16 bit)
 * INPUT_A (pins 0-7) ve INPUT_B (pins 8-15) register'larından okur
 */
uint16_t IO16_ReadAll(uint8_t slot) {
    IO16_Module* module = IO16_GetModule(slot);
    if (!module) {
        return 0;
    }
    
    uint8_t input_a = 0, input_b = 0;
    
    // INPUT_A register'ını oku (pins 0-7)
    if (IO16_ReadRegister(slot, IO16_REG_INPUT_A, 1, &input_a) != 0) {
        return module->input_state; // Hata durumunda cache'den dön
    }
    
    // INPUT_B register'ını oku (pins 8-15)
    if (IO16_ReadRegister(slot, IO16_REG_INPUT_B, 1, &input_b) != 0) {
        return module->input_state; // Hata durumunda cache'den dön
    }
    
    // 16 bit değer oluştur (input_b high byte, input_a low byte)
    uint16_t value = ((uint16_t)input_b << 8) | input_a;
    
    // Cache'i güncelle
    module->input_state = value;
    
    return value;
}

/**
 * Tüm output pin'leri yaz (16 bit)
 * OUTPUT_A (pins 0-7) ve OUTPUT_B (pins 8-15) register'larına yazar
 */
int IO16_WriteAll(uint8_t slot, uint16_t state) {
    IO16_Module* module = IO16_GetModule(slot);
    if (!module) {
        return -1;
    }
    
    // Lower byte (pins 0-7) - OUTPUT_A
    uint8_t output_a = state & 0xFF;
    if (IO16_WriteRegister(slot, IO16_REG_OUTPUT_A, 1, &output_a) != 0) {
        return -1;
    }
    
    // Upper byte (pins 8-15) - OUTPUT_B
    uint8_t output_b = (state >> 8) & 0xFF;
    if (IO16_WriteRegister(slot, IO16_REG_OUTPUT_B, 1, &output_b) != 0) {
        return -1;
    }
    
    // Cache'i güncelle (sadece output pin'leri)
    module->output_state = state & module->direction_mask;
    
    return 0;
}

/**
 * Modül durumunu yazdır
 */
void IO16_PrintStatus(uint8_t slot) {
    IO16_Module* module = IO16_GetModule(slot);
    if (!module) {
        UART_SendString("Hata: Modül bulunamadı\r\n");
        return;
    }
    
    // ✅ FIX: Chip'ten gerçek durumu oku!
    uint8_t dir_a, dir_b;
    uint8_t input_a, input_b;
    uint8_t output_a, output_b;
    
    // Direction registers oku (CONTROLWORD_2A/2B - 0x16/0x17)
    if (IO16_ReadRegister(slot, IO16_REG_CONTROLWORD_2A, 1, &dir_a) != 0) {
        UART_SendString("Hata: Direction A register okunamadı\r\n");
        return;
    }
    if (IO16_ReadRegister(slot, IO16_REG_CONTROLWORD_2B, 1, &dir_b) != 0) {
        UART_SendString("Hata: Direction B register okunamadı\r\n");
        return;
    }
    
    // Output registers oku (OUTPUT_A/B - 0x0C/0x0D)
    if (IO16_ReadRegister(slot, IO16_REG_OUTPUT_A, 1, &output_a) != 0) {
        UART_SendString("Hata: Output A register okunamadı\r\n");
        return;
    }
    if (IO16_ReadRegister(slot, IO16_REG_OUTPUT_B, 1, &output_b) != 0) {
        UART_SendString("Hata: Output B register okunamadı\r\n");
        return;
    }
    
    // Input registers oku (INPUT_A/B - 0x04/0x05)
    if (IO16_ReadRegister(slot, IO16_REG_INPUT_A, 1, &input_a) != 0) {
        UART_SendString("Hata: Input A register okunamadı\r\n");
        return;
    }
    if (IO16_ReadRegister(slot, IO16_REG_INPUT_B, 1, &input_b) != 0) {
        UART_SendString("Hata: Input B register okunamadı\r\n");
        return;
    }
    
    // CONTROLWORD_2 register'larını 16-bit direction mask'e çevir
    // CONTROLWORD_2A: bit 3 = pins 0-3, bit 7 = pins 4-7
    // CONTROLWORD_2B: bit 3 = pins 8-11, bit 7 = pins 12-15
    uint16_t direction = 0;
    
    // Grup 0 (pins 0-3): CONTROLWORD_2A bit 3
    if (dir_a & (1 << 3)) {
        direction |= 0x000F;  // Pins 0-3 OUTPUT
    }
    
    // Grup 1 (pins 4-7): CONTROLWORD_2A bit 7
    if (dir_a & (1 << 7)) {
        direction |= 0x00F0;  // Pins 4-7 OUTPUT
    }
    
    // Grup 2 (pins 8-11): CONTROLWORD_2B bit 3
    if (dir_b & (1 << 3)) {
        direction |= 0x0F00;  // Pins 8-11 OUTPUT
    }
    
    // Grup 3 (pins 12-15): CONTROLWORD_2B bit 7
    if (dir_b & (1 << 7)) {
        direction |= 0xF000;  // Pins 12-15 OUTPUT
    }
    
    // OUTPUT ve INPUT register'larını 16-bit'e çevir
    uint16_t output = ((uint16_t)output_b << 8) | output_a;
    uint16_t input = ((uint16_t)input_b << 8) | input_a;
    
    // Cache'i güncelle
    module->direction_mask = direction;
    module->output_state = output;
    module->input_state = input;
    
    UART_SendString("\r\n");
    UART_SendString("====================================\r\n");
    UART_SendString(" IO16 - 16 Kanal Dijital I/O\r\n");
    UART_SendString("====================================\r\n");
    UART_SendString("Slot: ");
    UART_SendHex8(module->slot);
    UART_SendString("\r\n\r\n");
    
    UART_SendString("Pin  Dir  Durum\r\n");
    UART_SendString("---  ---  -----\r\n");
    
    for (uint8_t i = 0; i < 16; i++) {
        // Pin numarası
        if (i < 10) UART_SendString(" ");
        char pin_str[4];
        sprintf(pin_str, "%d", i);
        UART_SendString(pin_str);
        UART_SendString("   ");
        
        // Yön
        if (direction & (1 << i)) {
            UART_SendString("OUT  ");
            // Çıkış değeri
            if (output & (1 << i)) {
                UART_SendString("HIGH");
            } else {
                UART_SendString("LOW ");
            }
        } else {
            UART_SendString("IN   ");
            // Giriş değeri
            if (input & (1 << i)) {
                UART_SendString("HIGH");
            } else {
                UART_SendString("LOW ");
            }
        }
        UART_SendString("\r\n");
    }
    
    UART_SendString("\r\nDurum Özeti:\r\n");
    UART_SendString("  INPUT:  0x");
    UART_SendHex16(input);
    UART_SendString("\r\n  OUTPUT: 0x");
    UART_SendHex16(output);
    UART_SendString("\r\n  DIR:    0x");
    UART_SendHex16(direction);
    UART_SendString(" (1=OUT, 0=IN)\r\n");
    UART_SendString("====================================\r\n");
}

/**
 * Modül komutunu işle
 * Format: io16:SLOT:KOMUT
 * Örnekler:
 *   io16:0:set:5:high
 *   io16:0:get:7
 *   io16:0:dir:3:out
 *   io16:0:status
 *   io16:0:readall
 */
void IO16_HandleCommand(const char* cmd) {
    // ACK gönder (komut alındı onayı)
    UART_SendString("[ACK:io16:");
    UART_SendString(cmd);
    UART_SendString("]\r\n");
    
    // Slot'u parse et
    if (cmd[0] < '0' || cmd[0] > '3') {
        UART_SendString("Hata: Geçersiz slot (0-3)\r\n");
        return;
    }
    uint8_t slot = cmd[0] - '0';
    
    // Slot atla (':' geç)
    if (cmd[1] != ':') {
        UART_SendString("Hata: Format hatası\r\n");
        return;
    }
    cmd += 2;
    
    // Komutu kontrol et
    if (strncmp(cmd, "set:", 4) == 0) {
        // set:PIN:STATE
        cmd += 4;
        uint8_t pin = cmd[0] - '0';
        if (cmd[1] >= '0' && cmd[1] <= '9') {
            pin = pin * 10 + (cmd[1] - '0');
            cmd += 2;
        } else {
            cmd += 1;
        }
        
        if (cmd[0] != ':') {
            UART_SendString("Hata: Format hatası\r\n");
            return;
        }
        cmd += 1;
        
        uint8_t state = (strncmp(cmd, "high", 4) == 0) ? 1 : 0;
        
        if (IO16_SetPin(slot, pin, state) == 0) {
            UART_SendString("OK: Pin ");
            UART_SendHex8(pin);
            UART_SendString(" = ");
            UART_SendString(state ? "HIGH" : "LOW");
            UART_SendString("\r\n");
        } else {
            UART_SendString("Hata: Pin ayarlanamadı\r\n");
        }
    }
    else if (strncmp(cmd, "get:", 4) == 0) {
        // get:PIN
        cmd += 4;
        uint8_t pin = cmd[0] - '0';
        if (cmd[1] >= '0' && cmd[1] <= '9') {
            pin = pin * 10 + (cmd[1] - '0');
        }
        
        int state = IO16_GetPin(slot, pin);
        if (state >= 0) {
            UART_SendString("Pin ");
            UART_SendHex8(pin);
            UART_SendString(" = ");
            UART_SendString(state ? "HIGH" : "LOW");
            UART_SendString("\r\n");
        } else {
            UART_SendString("Hata: Pin okunamadı\r\n");
        }
    }
    else if (strncmp(cmd, "dirgroup:", 9) == 0) {
        // dirgroup:GROUP:DIRECTION (iC-JX 4'lü grup kontrolü!)
        // GROUP: 0-3 (0=pins 0-3, 1=pins 4-7, 2=pins 8-11, 3=pins 12-15)
        cmd += 9;
        
        uint8_t group = cmd[0] - '0';
        if (group > 3) {
            UART_SendString("Hata: Geçersiz grup (0-3)\r\n");
            UART_SendString("  Grup 0: Pins 0-3\r\n");
            UART_SendString("  Grup 1: Pins 4-7\r\n");
            UART_SendString("  Grup 2: Pins 8-11\r\n");
            UART_SendString("  Grup 3: Pins 12-15\r\n");
            return;
        }
        
        if (cmd[1] != ':') {
            UART_SendString("Hata: Format hatası\r\n");
            return;
        }
        cmd += 2;
        
        uint8_t direction = (strncmp(cmd, "out", 3) == 0) ? 1 : 0;
        
        // Register ve bit pozisyonu hesapla
        uint8_t reg = (group < 2) ? IO16_REG_CONTROLWORD_2A : IO16_REG_CONTROLWORD_2B;
        uint8_t bit_pos = (group % 2 == 0) ? 3 : 7;
        
        // Mevcut değeri oku
        uint8_t reg_value;
        if (IO16_ReadRegister(slot, reg, 1, &reg_value) != 0) {
            UART_SendString("Hata: Register okunamadı\r\n");
            return;
        }
        
        // Bit'i güncelle
        if (direction) {
            reg_value |= (1 << bit_pos);
        } else {
            reg_value &= ~(1 << bit_pos);
        }
        
        // Yaz
        if (IO16_WriteRegister(slot, reg, 1, &reg_value) != 0) {
            UART_SendString("Hata: Register yazılamadı\r\n");
            return;
        }
        
        UART_SendString("OK: Group ");
        UART_SendHex8(group);
        UART_SendString(" (Pins ");
        UART_SendHex8(group * 4);
        UART_SendString("-");
        UART_SendHex8(group * 4 + 3);
        UART_SendString(") = ");
        UART_SendString(direction ? "OUTPUT" : "INPUT");
        UART_SendString("\r\n");
    }
    else if (strcmp(cmd, "status") == 0) {
        IO16_PrintStatus(slot);
    }
    else if (strcmp(cmd, "readall") == 0) {
        uint16_t state = IO16_ReadAll(slot);
        UART_SendString("Tüm pinler: 0x");
        UART_SendHex16(state);
        UART_SendString("\r\n");
    }
    else if (strncmp(cmd, "writeall:", 9) == 0) {
        // writeall:VALUE (hex format: 0x00FF veya decimal)
        cmd += 9;
        
        uint16_t value = 0;
        // Hex format mı kontrol et
        if (cmd[0] == '0' && (cmd[1] == 'x' || cmd[1] == 'X')) {
            cmd += 2;  // "0x" atla
            // Hex parse
            while (*cmd) {
                if (*cmd >= '0' && *cmd <= '9') {
                    value = (value << 4) | (*cmd - '0');
                } else if (*cmd >= 'A' && *cmd <= 'F') {
                    value = (value << 4) | (*cmd - 'A' + 10);
                } else if (*cmd >= 'a' && *cmd <= 'f') {
                    value = (value << 4) | (*cmd - 'a' + 10);
                } else {
                    break;  // Geçersiz karakter
                }
                cmd++;
            }
        } else {
            // Decimal parse
            while (*cmd >= '0' && *cmd <= '9') {
                value = value * 10 + (*cmd - '0');
                cmd++;
            }
        }
        
        if (IO16_WriteAll(slot, value) == 0) {
            UART_SendString("OK: Tüm pinler yazıldı = 0x");
            UART_SendHex16(value);
            UART_SendString("\r\n");
        } else {
            UART_SendString("Hata: Yazma başarısız\r\n");
        }
    }
    else if (strcmp(cmd, "info") == 0) {
        // Read chip INFO register
        uint8_t info = IO16_GetChipInfo(slot);
        UART_SendString("iC-JX Chip INFO: 0x");
        UART_SendHex8(info);
        UART_SendString("\r\n");
        
        // Chip detected ve başarılı? → AUTO-INITIALIZE!
        if (info != 0x00 && info != 0xFF) {
            UART_SendString("\r\n💡 Chip detected! Auto-initializing...\r\n");
            if (IO16_InitChip(slot) == 0) {
                UART_SendString("✅ Chip initialization SUCCESS!\r\n");
                UART_SendString("📝 TIP: Now run 'io16:0:status' to verify settings\r\n\r\n");
            } else {
                UART_SendString("❌ Chip initialization FAILED!\r\n\r\n");
            }
        }
    }
    else if (strcmp(cmd, "overcurrent") == 0) {
        // Check overcurrent status
        uint16_t over = IO16_CheckOvercurrent(slot);
        if (over == 0) {
            UART_SendString("No overcurrent detected\r\n");
        }
    }
    else if (strcmp(cmd, "regdump") == 0) {
        // Dump all important registers
        IO16_DumpRegisters(slot);
    }
    else if (strncmp(cmd, "testcs:", 7) == 0) {
        // testcs:GPIO:PIN - Test a specific pin as CS (SAFE - READ ONLY!)
        // Format: io16:testcs:0:13 (test GPIOA Pin 13)
        cmd += 7;
        
        // Parse GPIO number
        uint8_t gpio = cmd[0] - '0';
        if (cmd[1] != ':') {
            UART_SendString("Hata: Format hatası (beklenen: testcs:GPIO:PIN)\r\n");
            return;
        }
        cmd += 2;
        
        // Parse Pin number
        uint8_t pin = cmd[0] - '0';
        if (cmd[1] >= '0' && cmd[1] <= '9') {
            pin = pin * 10 + (cmd[1] - '0');
        }
        
        // Validate GPIO range
        if (gpio > 3) {  // GPIOA=0, GPIOB=1, GPIOC=2, GPIOD=3
            UART_SendString("Hata: GPIO geçersiz (0-3)\r\n");
            return;
        }
        
        // Validate pin range
        if (pin > 15) {
            UART_SendString("Hata: Pin geçersiz (0-15)\r\n");
            return;
        }
        
        UART_SendString("\r\n[TEST-CS] Testing GPIO");
        UART_SendHex8(gpio);
        UART_SendString(" Pin ");
        UART_SendHex8(pin);
        UART_SendString(" as CS\r\n");
        
        // Temporarily set this pin as CS
        // CRITICAL: Only do READ operations!
        GPIO_TypeDef* gpio_port;
        uint16_t gpio_pin_mask;
        
        switch(gpio) {
            case 0: gpio_port = GPIOA; UART_SendString("[TEST-CS] GPIO = GPIOA\r\n"); break;
            case 1: gpio_port = GPIOB; UART_SendString("[TEST-CS] GPIO = GPIOB\r\n"); break;
            case 2: gpio_port = GPIOC; UART_SendString("[TEST-CS] GPIO = GPIOC\r\n"); break;
            case 3: gpio_port = GPIOD; UART_SendString("[TEST-CS] GPIO = GPIOD\r\n"); break;
            default: 
                UART_SendString("Hata: GPIO dönüşüm hatası\r\n");
                return;
        }
        
        gpio_pin_mask = (1 << pin);
        
        UART_SendString("[TEST-CS] Pin mask = 0x");
        UART_SendHex16(gpio_pin_mask);
        UART_SendString("\r\n");
        
        // Configure pin as OUTPUT if not already
        GPIO_InitTypeDef gpio_init;
        gpio_init.GPIO_Pin = gpio_pin_mask;
        gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
        gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_Init(gpio_port, &gpio_init);
        
        // Set CS HIGH (inactive)
        GPIO_SetBits(gpio_port, gpio_pin_mask);
        UART_SendString("[TEST-CS] CS set HIGH (inactive)\r\n");
        
        // Small delay
        for (volatile int i = 0; i < 10000; i++);
        
        // Set CS LOW (active)
        GPIO_ResetBits(gpio_port, gpio_pin_mask);
        UART_SendString("[TEST-CS] CS set LOW (active)\r\n");
        
        // Small delay for chip to respond
        for (volatile int i = 0; i < 10000; i++);
        
        // Try to read chip INFO register (SAFE - READ ONLY!)
        uint8_t info = IO16_GetChipInfo(slot);
        
        UART_SendString("[CHIP-INFO] INFO Register = 0x");
        UART_SendHex8(info);
        UART_SendString("\r\n");
        
        if (info != 0x00 && info != 0xFF) {
            UART_SendString("[CHIP-INFO] ✓ iC-JX chip detected and responding!\r\n");
            UART_SendString("[SUCCESS] This pin is the correct CS: GPIO");
            UART_SendHex8(gpio);
            UART_SendString(" Pin ");
            UART_SendHex8(pin);
            UART_SendString("\r\n");
        } else {
            UART_SendString("[CHIP-INFO] ✗ No valid chip response (0x00 or 0xFF)\r\n");
            UART_SendString("[FAIL] This pin is NOT the CS pin\r\n");
        }
        
        // Set CS HIGH (inactive) again
        GPIO_SetBits(gpio_port, gpio_pin_mask);
        UART_SendString("[TEST-CS] CS set HIGH (inactive) - Test complete\r\n");
    }
    else {
        UART_SendString("Hata: Bilinmeyen komut\r\n");
        UART_SendString("Kullanım:\r\n");
        UART_SendString("  io16:SLOT:set:PIN:high/low\r\n");
        UART_SendString("  io16:SLOT:get:PIN\r\n");
        UART_SendString("  io16:SLOT:dir:PIN:in/out\r\n");
        UART_SendString("  io16:SLOT:status\r\n");
        UART_SendString("  io16:SLOT:readall\r\n");
        UART_SendString("  io16:SLOT:info         - Read chip INFO register\r\n");
        UART_SendString("  io16:SLOT:overcurrent  - Check overcurrent status\r\n");
        UART_SendString("  io16:SLOT:regdump      - Dump all registers\r\n");
        UART_SendString("  io16:SLOT:writeall:VAL - Write all 16 pins (hex or decimal)\r\n");
        UART_SendString("  io16:testcs:GPIO:PIN   - Test pin as CS (SAFE - READ ONLY!)\r\n");
    }
    
    // Komut tamamlandı mesajı (burjuva_manager için)
    UART_SendString("\r\nKomut tamamlandi: io16\r\n");
}
