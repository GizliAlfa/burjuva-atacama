/**
 * Burjuva Motor Controller - Modül Algılama Sistemi
 * Date: 17 Kasım 2025
 * 
 * GERÇEK 1-Wire modül algılama - module_detection_working.c bazlı
 * - Modül UID (1-Wire ROM - 8 byte)
 * - HID (Hardware ID - 0x00-0x07)
 * - FID (Firmware ID - 0x08-0x0F)
 * - 4 Slot: PC2(0), PC0(1), PC3(2), PC1(3)
 */

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "modul_algilama.h"
#include "16kanaldijital.h"
#include "20kanalanalogio.h"
#include "fpga.h"
#include <string.h>

// ========== Forward Declarations ==========
static void UART_SendString(const char* str);
static void UART_SendHex8(uint8_t val);
static const char* get_module_type(uint8_t* hid, uint8_t* fid);

// ========== DWT Delay (72MHz) ==========
static inline uint32_t dwt_get_cycles(void) {
    return *((volatile uint32_t*)0xE0001004);  // DWT_CYCCNT
}

static inline void delay_us(float us) {
    uint32_t start = dwt_get_cycles();
    uint32_t cycles = (uint32_t)(us * 72.0f);  // 72MHz = 72 cycles per μs
    while ((dwt_get_cycles() - start) < cycles);
}

// ========== 1-Wire Timing ==========
// OVERDRIVE SPEED - EXACT FROM module_detection_working.c (72MHz)
#define DELAY_A 1.0      // Write 1 low time
#define DELAY_B 7.5      // Write 1 release to end
#define DELAY_C 7.5      // Write 0 low time
#define DELAY_D 2.5      // Write 0 release to end
#define DELAY_E 1.0      // Read sampling delay
#define DELAY_F 7.0      // Read recovery time
#define DELAY_H 70.0     // Reset pulse (Overdrive Speed)
#define DELAY_I 8.5      // Presence detect sample delay
#define DELAY_J 40.0     // Presence detect to next operation

// OPTION 2: Overdrive Speed (uncomment if modules support overdrive)
// #define DELAY_A 1.0
// #define DELAY_B 7.5
// #define DELAY_C 7.5
// #define DELAY_D 2.5
// #define DELAY_E 1.0
// #define DELAY_F 7.0
// #define DELAY_H 70.0
// #define DELAY_I 8.5
// #define DELAY_J 40.0

// ========== Module Slot Mapping (EXACT from module_detection_working.c) ==========
static uint16_t module_to_pin(int module) {
    if (module == 0) return GPIO_Pin_2;  // PC2 - Slot 0
    if (module == 1) return GPIO_Pin_0;  // PC0 - Slot 1
    if (module == 2) return GPIO_Pin_3;  // PC3 - Slot 2
    return GPIO_Pin_1;                    // PC1 - Slot 3
}

// ========== 1-Wire Pin Control (EXACT from module_detection_working.c) ==========
static void set_pin_output(int module) {
    GPIO_InitTypeDef pin;
    GPIO_StructInit(&pin);  // Initialize with defaults
    pin.GPIO_Pin = module_to_pin(module);
    pin.GPIO_Speed = GPIO_Speed_50MHz;
    pin.GPIO_Mode = GPIO_Mode_Out_OD;  // Open drain - CRITICAL!
    GPIO_Init(GPIOC, &pin);
    
    // Set HIGH initially (internal pull-up assist)
    GPIO_WriteBit(GPIOC, pin.GPIO_Pin, Bit_SET);
}

static void set_pin_input(int module) {
    GPIO_InitTypeDef pin;
    GPIO_StructInit(&pin);  // Initialize with defaults
    pin.GPIO_Pin = module_to_pin(module);
    pin.GPIO_Speed = GPIO_Speed_50MHz;
    pin.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &pin);
}

static uint8_t read_bus(int module) {
    return GPIO_ReadInputDataBit(GPIOC, module_to_pin(module));
}

// ========== 1-Wire Protocol (EXACT from module_detection_working.c) ==========

// Reset pulse - returns 1 if device present
static uint8_t onewire_reset(int module) {
    uint8_t presence;
    uint16_t pin = module_to_pin(module);
    
    set_pin_output(module);
    
    // DEBUG: Check bus state BEFORE reset
    uint8_t bus_before = (GPIOC->IDR & pin) ? 1 : 0;
    
    // Drive LOW for reset pulse
    GPIO_WriteBit(GPIOC, pin, Bit_RESET);
    delay_us(DELAY_H);
    
    // Release bus
    GPIO_WriteBit(GPIOC, pin, Bit_SET);
    delay_us(DELAY_I);
    
    // Sample presence pulse (slave pulls LOW)
    uint8_t bus_after = (GPIOC->IDR & pin) ? 1 : 0;
    presence = (GPIOC->IDR & pin) ? 0 : 1;  // LOW = device present
    delay_us(DELAY_J);
    
    // DEBUG: Show bus levels
    UART_SendString(" [BUS:before=");
    UART_SendHex8(bus_before);
    UART_SendString(",after=");
    UART_SendHex8(bus_after);
    UART_SendString(",presence=");
    UART_SendHex8(presence);
    UART_SendString("]");
    
    return presence;
}

// Write bit (EXACT - stay in OUTPUT mode)
static void onewire_write_bit(int module, uint8_t bit) {
    uint16_t pin = module_to_pin(module);
    
    set_pin_output(module);
    
    if (bit) {
        // Write 1: SHORT pulse LOW, then release
        GPIO_WriteBit(GPIOC, pin, Bit_RESET);
        delay_us(DELAY_A);
        GPIO_WriteBit(GPIOC, pin, Bit_SET);  // Release (HIGH in open-drain = HIGH-Z)
        delay_us(DELAY_B);
    } else {
        // Write 0: LONG pulse LOW
        GPIO_WriteBit(GPIOC, pin, Bit_RESET);
        delay_us(DELAY_C);
        GPIO_WriteBit(GPIOC, pin, Bit_SET);  // Release
        delay_us(DELAY_D);
    }
}

// Read bit (CRITICAL: stay in OUTPUT mode!)
static uint8_t onewire_read_bit(int module) {
    uint8_t bit;
    uint16_t pin = module_to_pin(module);
    
    // Drive LOW
    set_pin_output(module);
    GPIO_WriteBit(GPIOC, pin, Bit_RESET);
    delay_us(DELAY_A);
    
    // Release bus (write HIGH in open-drain = HIGH-Z)
    GPIO_WriteBit(GPIOC, pin, Bit_SET);
    // DON'T switch to input! Stay in output mode with HIGH
    delay_us(DELAY_E);
    
    // Sample bus (read from IDR register directly)
    bit = (GPIOC->IDR & pin) ? 1 : 0;
    delay_us(DELAY_F);
    
    return bit;
}

// Write byte LSB first
static void onewire_write_byte(int module, uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        onewire_write_bit(module, (byte >> i) & 0x01);
    }
}

// Read byte LSB first
static uint8_t onewire_read_byte(int module) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        if (onewire_read_bit(module))
            byte |= (1 << i);
    }
    return byte;
}

// CRC8 calculation
static uint8_t crc8(uint8_t *data, int len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (int j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

// ========== UART Helper Functions ==========
static void UART_SendString(const char* str) {
    while (*str) {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *str++);
    }
}

static void UART_SendHex8(uint8_t val) {
    const char hex[] = "0123456789ABCDEF";
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, hex[val >> 4]);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, hex[val & 0x0F]);
}

// ========== Module Type Detection ==========
/**
 * Identify module type based on FID ASCII string
 * Returns human-readable module description
 * 
 * BURJUVA PILOT uses ASCII strings in FID for module identification
 * Examples from real system:
 * - "io16   !" → IO16 module
 * - "aio20  e" → AIO20 module
 * - "fpga   0" → FPGA module
 * - "i8      " → I8 module
 */
static const char* get_module_type(uint8_t* hid, uint8_t* fid) {
    // FID contains ASCII module name (first 2-5 chars are significant)
    // We'll do case-insensitive comparison
    
    // Check for "io16" (case insensitive)
    if ((fid[0] == 'i' || fid[0] == 'I') && 
        (fid[1] == 'o' || fid[1] == 'O') &&
        (fid[2] == '1') && (fid[3] == '6')) {
        return "IO16 - 16 Channel Digital I/O";
    }
    
    // Check for "aio20"
    if ((fid[0] == 'a' || fid[0] == 'A') && 
        (fid[1] == 'i' || fid[1] == 'I') &&
        (fid[2] == 'o' || fid[2] == 'O') &&
        (fid[3] == '2') && (fid[4] == '0')) {
        return "AIO20 - 20 Channel Analog I/O";
    }
    
    // Check for "fpga"
    if ((fid[0] == 'f' || fid[0] == 'F') && 
        (fid[1] == 'p' || fid[1] == 'P') &&
        (fid[2] == 'g' || fid[2] == 'G') &&
        (fid[3] == 'a' || fid[3] == 'A')) {
        return "FPGA - FPGA Extension Module";
    }
    
    // Check for "i8"
    if ((fid[0] == 'i' || fid[0] == 'I') && (fid[1] == '8')) {
        return "I8 - 8 Channel Digital Input";
    }
    
    // Check for "o8"
    if ((fid[0] == 'o' || fid[0] == 'O') && (fid[1] == '8')) {
        return "O8 - 8 Channel Digital Output";
    }
    
    // Check for "ai8"
    if ((fid[0] == 'a' || fid[0] == 'A') && 
        (fid[1] == 'i' || fid[1] == 'I') &&
        (fid[2] == '8')) {
        return "AI8 - 8 Channel Analog Input";
    }
    
    // Check for "gsm"
    if ((fid[0] == 'g' || fid[0] == 'G') && 
        (fid[1] == 's' || fid[1] == 'S') &&
        (fid[2] == 'm' || fid[2] == 'M')) {
        return "GSM - GSM/GPRS Module";
    }
    
    // Check for "can"
    if ((fid[0] == 'c' || fid[0] == 'C') && 
        (fid[1] == 'a' || fid[1] == 'A') &&
        (fid[2] == 'n' || fid[2] == 'N')) {
        return "CAN - CAN Bus Interface";
    }
    
    // Check for "rs485"
    if ((fid[0] == 'r' || fid[0] == 'R') && 
        (fid[1] == 's' || fid[1] == 'S') &&
        (fid[2] == '4') && (fid[3] == '8') && (fid[4] == '5')) {
        return "RS485 - RS485 Serial Interface";
    }
    
    // Check for "rs232"
    if ((fid[0] == 'r' || fid[0] == 'R') && 
        (fid[1] == 's' || fid[1] == 'S') &&
        (fid[2] == '2') && (fid[3] == '3') && (fid[4] == '2')) {
        return "RS232 - RS232 Serial Interface";
    }
    
    // Check for "onewire"
    if ((fid[0] == 'o' || fid[0] == 'O') && 
        (fid[1] == 'n' || fid[1] == 'N') &&
        (fid[2] == 'e' || fid[2] == 'E')) {
        return "ONEWIRE - 1-Wire Interface";
    }
    
    // Check for "counter8"
    if ((fid[0] == 'c' || fid[0] == 'C') && 
        (fid[1] == 'o' || fid[1] == 'O') &&
        (fid[2] == 'u' || fid[2] == 'U') &&
        (fid[3] == 'n' || fid[3] == 'N')) {
        return "COUNTER8 - 8 Channel Counter";
    }
    
    // Check for "gps"
    if ((fid[0] == 'g' || fid[0] == 'G') && 
        (fid[1] == 'p' || fid[1] == 'P') &&
        (fid[2] == 's' || fid[2] == 'S')) {
        return "GPS - GPS/GNSS Module";
    }
    
    // Check for "lora"
    if ((fid[0] == 'l' || fid[0] == 'L') && 
        (fid[1] == 'o' || fid[1] == 'O') &&
        (fid[2] == 'r' || fid[2] == 'R') &&
        (fid[3] == 'a' || fid[3] == 'A')) {
        return "LORA - LoRa Communication";
    }
    
    // Check for "pwm"
    if ((fid[0] == 'p' || fid[0] == 'P') && 
        (fid[1] == 'w' || fid[1] == 'W') &&
        (fid[2] == 'm' || fid[2] == 'M')) {
        return "PWM - PWM Motor Controller";
    }
    
    // Check for "slcd"
    if ((fid[0] == 's' || fid[0] == 'S') && 
        (fid[1] == 'l' || fid[1] == 'L') &&
        (fid[2] == 'c' || fid[2] == 'C') &&
        (fid[3] == 'd' || fid[3] == 'D')) {
        return "SLCD - Segment LCD Display";
    }
    
    // Check for "demo"
    if ((fid[0] == 'd' || fid[0] == 'D') && 
        (fid[1] == 'e' || fid[1] == 'E') &&
        (fid[2] == 'm' || fid[2] == 'M') &&
        (fid[3] == 'o' || fid[3] == 'O')) {
        return "DEMO - Demo/Test Module";
    }
    
    // Check for empty/test pattern
    if (fid[0] == 0xAA || fid[0] == 0x55 || fid[0] == 0xFF || fid[0] == 0x00) {
        return "EMPTY - No Module or Test Pattern";
    }
    
    return "UNKNOWN - Unrecognized Module";
}

// ========== Module Detection Functions (EXACT from module_detection_working.c) ==========

// Read 1-Wire ROM (8 bytes: family + serial + CRC)
static uint8_t read_module_uid(int module, uint8_t *uid) {
    uint8_t presence = onewire_reset(module);
    
    // DEBUG: Show presence detection
    UART_SendString(" [RESET:");
    if (presence) {
        UART_SendString("OK");
    } else {
        UART_SendString("NO_DEVICE");
    }
    UART_SendString("]");
    
    if (!presence)
        return 0;  // No device
    
    onewire_write_byte(module, 0x33);  // READ ROM command
    
    for (int i = 0; i < 8; i++) {
        uid[i] = onewire_read_byte(module);
    }
    
    // DEBUG: Show raw UID
    UART_SendString(" RAW:");
    for (int i = 0; i < 8; i++) {
        UART_SendHex8(uid[i]);
        UART_SendString(" ");
    }
    
    // Verify CRC
    uint8_t crc = crc8(uid, 7);
    UART_SendString("CRC:calc=");
    UART_SendHex8(crc);
    UART_SendString(" read=");
    UART_SendHex8(uid[7]);
    
    return (crc == uid[7]) ? 1 : 0;
}

// Read EEPROM memory
static uint8_t read_module_memory(int module, uint8_t addr, uint8_t *data, int len) {
    if (!onewire_reset(module))
        return 0;
    
    onewire_write_byte(module, 0xCC);  // SKIP ROM
    onewire_write_byte(module, 0xF0);  // READ MEMORY
    onewire_write_byte(module, addr);  // Address
    onewire_write_byte(module, 0x00);  // Address high byte
    
    for (int i = 0; i < len; i++) {
        data[i] = onewire_read_byte(module);
    }
    
    onewire_reset(module);
    return 1;
}

/**
 * Scan all 4 module slots with detailed debug info
 */
static void scan_modules(void) {
    uint8_t uid[8];
    uint8_t hid[8];
    uint8_t fid[8];
    
    UART_SendString("\r\n========================================\r\n");
    UART_SendString("  BURJUVA MODULE DETECTION\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("Protocol: 1-Wire OVERDRIVE SPEED\r\n");
    UART_SendString("Reset Pulse: 70us (Overdrive)\r\n");
    UART_SendString("Slots: PC2(0), PC0(1), PC3(2), PC1(3)\r\n");
    UART_SendString("Clock: 72MHz (HSE + PLL)\r\n");
    UART_SendString("DWT: 72 cycles/us\r\n");
    UART_SendString("========================================\r\n\r\n");
    
    // Check DWT status
    uint32_t dwt_ctrl = *((volatile uint32_t*)0xE0001000);
    UART_SendString("DWT_CTRL: ");
    UART_SendHex8((dwt_ctrl >> 0) & 0xFF);
    UART_SendString(" ");
    UART_SendHex8((dwt_ctrl >> 8) & 0xFF);
    UART_SendString(" ");
    UART_SendHex8((dwt_ctrl >> 16) & 0xFF);
    UART_SendString(" ");
    UART_SendHex8((dwt_ctrl >> 24) & 0xFF);
    UART_SendString("\r\n\r\n");
    
    for (int slot = 0; slot < 4; slot++) {
        UART_SendString("Slot ");
        UART_SendHex8(slot);
        UART_SendString(" (PC");
        if (slot == 0) UART_SendString("2");
        else if (slot == 1) UART_SendString("0");
        else if (slot == 2) UART_SendString("3");
        else UART_SendString("1");
        UART_SendString("):");
        
        // Try to read UID with debug info
        if (read_module_uid(slot, uid)) {
            UART_SendString(" -> FOUND!\r\n");
            
            // Show UID (ROM - 8 bytes)
            UART_SendString("  UID: ");
            for (int i = 0; i < 8; i++) {
                UART_SendHex8(uid[i]);
                UART_SendString(" ");
            }
            
            // Show 1-Wire device family
            UART_SendString("(Family: ");
            UART_SendHex8(uid[0]);
            if (uid[0] == 0x2B) UART_SendString("=DS2431");
            else if (uid[0] == 0x0D) UART_SendString("=Unknown-0D");
            else UART_SendString("=Unknown");
            UART_SendString(")\r\n");
            
            // Read HID (Hardware ID - 0x00-0x07)
            if (read_module_memory(slot, 0x00, hid, 8)) {
                UART_SendString("  HID: ");
                for (int i = 0; i < 8; i++) {
                    UART_SendHex8(hid[i]);
                    UART_SendString(" ");
                }
                UART_SendString("\r\n");
            } else {
                UART_SendString("  HID: Read FAILED\r\n");
            }
            
            // Read FID (Firmware ID - 0x08-0x0F)
            if (read_module_memory(slot, 0x08, fid, 8)) {
                UART_SendString("  FID: ");
                for (int i = 0; i < 8; i++) {
                    UART_SendHex8(fid[i]);
                    UART_SendString(" ");
                }
                UART_SendString("\r\n");
            } else {
                UART_SendString("  FID: Read FAILED\r\n");
            }
            
            // Show module type description (from HID mapping)
            const char* module_type = get_module_type(hid, fid);
            UART_SendString("  TYPE: ");
            UART_SendString(module_type);
            UART_SendString("\r\n");
            
            // Show FID as ASCII string (module name from EEPROM)
            UART_SendString("  NAME: ");
            for (int i = 0; i < 8; i++) {
                if (fid[i] >= 0x20 && fid[i] <= 0x7E) {  // Printable ASCII
                    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
                    USART_SendData(USART1, fid[i]);
                } else if (fid[i] == 0x00) {
                    break;  // Null terminator
                }
            }
            UART_SendString("\r\n");
            
            // DEBUG: Show HID as ASCII too (for unprogrammed modules)
            UART_SendString("  HID_ASCII: ");
            for (int i = 0; i < 8; i++) {
                if (hid[i] >= 0x20 && hid[i] <= 0x7E) {  // Printable ASCII
                    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
                    USART_SendData(USART1, hid[i]);
                } else if (hid[i] == 0x00) {
                    break;  // Null terminator
                } else {
                    UART_SendString(".");  // Non-printable
                }
            }
            UART_SendString("\r\n");
            
            // Show CON number (slot-based connection number)
            UART_SendString("  CON: CON");
            UART_SendHex8(slot);
            UART_SendString(" (Connector ");
            UART_SendHex8(slot);
            UART_SendString(")\r\n");
            
            // Register module to its handler
            if (strncmp((char*)fid, "io16", 4) == 0 || strncmp((char*)hid, "io16", 4) == 0) {
                IO16_Register(slot);
                UART_SendString("  [REGISTERED] IO16 module at slot ");
                UART_SendHex8(slot);
                UART_SendString("\r\n");
                
                // CRITICAL: Retry init like mevcut-sistem (up to 100 tries with 10ms delay)
                // iC-JX may need multiple attempts after power-on
                UART_SendString("  [INIT] Initializing IO678 chip (with retry)...\r\n");
                int init_status = -1;
                int tries = 0;
                while (init_status != 0 && tries < 100) {
                    if (tries > 0) {
                        for (volatile int d = 0; d < 80000; d++);  // ~10ms delay at 8MHz
                    }
                    init_status = IO16_ChipInit(slot);
                    tries++;
                }
                
                if (init_status == 0) {
                    UART_SendString("  [SUCCESS] IO16 chip initialized after ");
                    UART_SendHex8(tries);
                    UART_SendString(" tries!\r\n");
                } else {
                    UART_SendString("  [ERROR] IO16 chip initialization FAILED after 100 tries!\r\n");
                }
            }
            else if (strncmp((char*)fid, "aio20", 5) == 0 || strncmp((char*)hid, "aio20", 5) == 0) {
                AIO20_Register(slot);
                UART_SendString("  [REGISTERED] AIO20 module at slot ");
                UART_SendHex8(slot);
                UART_SendString("\r\n");
            }
            else if (strncmp((char*)fid, "fpga", 4) == 0 || strncmp((char*)hid, "fpga", 4) == 0) {
                FPGA_Register(slot);
                UART_SendString("  [REGISTERED] FPGA module at slot ");
                UART_SendHex8(slot);
                UART_SendString("\r\n");
            }
            
            UART_SendString("\r\n");
        } else {
            UART_SendString(" -> EMPTY\r\n\r\n");
        }
        
        // Small delay between slots (100ms at 8MHz)
        for (volatile int d = 0; d < 800000; d++);
    }
    
    UART_SendString("========================================\r\n");
    UART_SendString("Scan Complete!\r\n");
    UART_SendString("========================================\r\n\r\n");
}

/**
 * Initialize module detection (called once at startup)
 */
void Modul_Init(void) {
    // Enable DWT cycle counter for precise timing (CRITICAL for 1-Wire!)
    *((volatile uint32_t*)0xE000EDFC) |= (1 << 24);  // DEMCR |= DEMCR_TRCENA
    *((volatile uint32_t*)0xE0001004) = 0;           // DWT_CYCCNT = 0
    *((volatile uint32_t*)0xE0001000) |= 1;          // DWT_CTRL |= CYCCNTENA
    
    // Initialize GPIO for 1-Wire pins (PC0, PC1, PC2, PC3)
    GPIO_InitTypeDef inputs;
    GPIO_StructInit(&inputs);  // Initialize with defaults (EXACT from module_detection_working.c)
    inputs.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    inputs.GPIO_Speed = GPIO_Speed_50MHz;
    inputs.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &inputs);
    
    // Debug: Send init message
    UART_SendString("\r\n[INIT] Module detection system initialized\r\n");
    UART_SendString("[INIT] DWT cycle counter enabled\r\n");
    UART_SendString("[INIT] 1-Wire GPIO configured (PC0, PC1, PC2, PC3)\r\n");
    UART_SendString("[INIT] Clock: 72MHz (HSE + PLL x9)\r\n");
    UART_SendString("[INIT] Timing: Overdrive Speed (70us reset)\r\n\r\n");
}

/**
 * "modul-algila" command handler
 */
void Modul_Komut_Isle(void) {
    scan_modules();
    UART_SendString("Komut tamamlandi: modul-algila\r\n\r\n");
}
