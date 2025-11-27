/**
 * @file fpga.c
 * @brief FPGA Motor Controller Module Driver
 * @version 2.0 - Enhanced STM32 Control
 * @date 23 Kasım 2025
 * 
 * FPGA-based motor controller with:
 * - Position control (closed-loop)
 * - Speed/direction control (open-loop)
 * - Homing
 * - Status reporting
 * - Error handling
 * - Multi-motor support (up to 16 channels)
 * 
 * Hardware: FPGA003 (mevcut sistemden)
 * Interface: Register-based via UART commands
 * 
 * Register Map: 16 bytes per motor (256 bytes total for 16 motors)
 * - Control flags, status flags, error code
 * - Current position (24-bit signed)
 * - Target position (24-bit signed)
 * - Speed, direction
 */

#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "fpga.h"
#include "uart_helper.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// FPGA Module Structure
// ============================================================================

// FPGA modül durumu
typedef struct {
    uint8_t slot;                // Modül slot numarası (0-3)
    uint8_t registers[256];      // FPGA register file (16 bytes × 16 motors)
    bool initialized;            // Initialization flag
} FPGA_Module;

// Maksimum 4 slot
static FPGA_Module fpga_modules[4];
static uint8_t fpga_module_count = 0;

// ============================================================================
// Module Management Functions
// ============================================================================

/**
 * @brief Register FPGA module in specified slot
 */
void FPGA_Register(uint8_t slot) {
    if (slot < 4 && fpga_module_count < 4) {
        fpga_modules[fpga_module_count].slot = slot;
        fpga_modules[fpga_module_count].initialized = false;
        
        // Register file'ı sıfırla
        for (uint16_t i = 0; i < 256; i++) {
            fpga_modules[fpga_module_count].registers[i] = 0;
        }
        
        fpga_module_count++;
        
        UART_SendString("FPGA modül kaydedildi: Slot ");
        UART_SendHex8(slot);
        UART_SendString("\r\n");
    }
}

/**
 * Slot'a göre modül bul
 */
static FPGA_Module* FPGA_GetModule(uint8_t slot) {
    for (uint8_t i = 0; i < fpga_module_count; i++) {
        if (fpga_modules[i].slot == slot) {
            return &fpga_modules[i];
        }
    }
    return NULL;
}

/**
 * @brief Read from FPGA register
 */
int FPGA_ReadRegister(uint8_t slot, uint8_t address, uint8_t* value) {
    FPGA_Module* module = FPGA_GetModule(slot);
    if (!module || !value) {
        return -1;
    }
    
    // TODO: Gerçek donanımdan oku (SPI üzerinden)
    // Şimdilik simüle ediyoruz
    
    *value = module->registers[address];
    return 0;
}

/**
 * @brief Write to FPGA register
 */
int FPGA_WriteRegister(uint8_t slot, uint8_t address, uint8_t value) {
    FPGA_Module* module = FPGA_GetModule(slot);
    if (!module) {
        return -1;
    }
    
    module->registers[address] = value;
    
    // TODO: Gerçek donanıma gönder (SPI üzerinden)
    
    return 0;
}

/**
 * @brief Reset FPGA module
 */
int FPGA_Reset(uint8_t slot) {
    FPGA_Module* module = FPGA_GetModule(slot);
    if (!module) {
        return -1;
    }
    
    // Register file'ı sıfırla
    for (uint16_t i = 0; i < 256; i++) {
        module->registers[i] = 0;
    }
    
    module->initialized = false;
    
    // TODO: Gerçek donanıma reset sinyali gönder (CRESET pin)
    
    UART_SendString("FPGA reset edildi: Slot ");
    UART_SendHex8(slot);
    UART_SendString("\r\n");
    
    return 0;
}

// ============================================================================
// Motor Control API - Position Control
// ============================================================================

/**
 * @brief Move motor to target position (closed-loop)
 */
int FPGA_Motor_GoToPosition(FPGA_Motor_t *motor, int32_t target_pos, uint8_t speed) {
    if (!motor || motor->channel > 15) {
        return -1;
    }
    
    uint8_t base = FPGA_MOTOR_REG_BASE(motor->channel);
    
    // 1. Hedef pozisyon yaz (24-bit signed)
    FPGA_WriteRegister(motor->slot, base + REG_TARGET_POS_HIGH, (target_pos >> 16) & 0xFF);
    FPGA_WriteRegister(motor->slot, base + REG_TARGET_POS_MID, (target_pos >> 8) & 0xFF);
    FPGA_WriteRegister(motor->slot, base + REG_TARGET_POS_LOW, target_pos & 0xFF);
    
    // 2. Hız ayarla
    FPGA_WriteRegister(motor->slot, base + REG_SPEED, speed);
    
    // 3. Position mode enable
    uint8_t ctrl = CTRL_FLAG_ENABLE;  // CONTROL_MODE = 0 (position)
    FPGA_WriteRegister(motor->slot, base + REG_CONTROL_FLAGS, ctrl);
    
    return 0;
}

/**
 * @brief Check if motor reached target position
 */
bool FPGA_Motor_IsPositionReached(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        return false;
    }
    
    uint8_t status = FPGA_Motor_GetStatus(motor);
    return (status & STATUS_FLAG_POSITION_REACHED) != 0;
}

// ============================================================================
// Motor Control API - Speed/Direction Control
// ============================================================================

/**
 * @brief Set motor speed and direction (open-loop)
 */
int FPGA_Motor_SetSpeedDirection(FPGA_Motor_t *motor, uint8_t speed, uint8_t direction) {
    if (!motor || motor->channel > 15) {
        return -1;
    }
    
    if (direction > DIRECTION_REVERSE) {
        return -1;  // Invalid direction
    }
    
    uint8_t base = FPGA_MOTOR_REG_BASE(motor->channel);
    
    // 1. Hız yaz
    FPGA_WriteRegister(motor->slot, base + REG_SPEED, speed);
    
    // 2. Yön yaz
    FPGA_WriteRegister(motor->slot, base + REG_DIRECTION, direction);
    
    // 3. Speed/Dir mode enable
    uint8_t ctrl = CTRL_FLAG_ENABLE | CTRL_FLAG_CONTROL_MODE;  // Speed/Dir mode
    FPGA_WriteRegister(motor->slot, base + REG_CONTROL_FLAGS, ctrl);
    
    return 0;
}

/**
 * @brief Stop motor immediately
 */
int FPGA_Motor_Stop(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        return -1;
    }
    
    uint8_t base = FPGA_MOTOR_REG_BASE(motor->channel);
    
    // Direction = STOP
    FPGA_WriteRegister(motor->slot, base + REG_DIRECTION, DIRECTION_STOP);
    
    return 0;
}

/**
 * @brief Emergency stop
 */
int FPGA_Motor_EmergencyStop(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        return -1;
    }
    
    uint8_t base = FPGA_MOTOR_REG_BASE(motor->channel);
    
    // EMERGENCY_STOP flag
    uint8_t ctrl = CTRL_FLAG_EMERGENCY_STOP;
    FPGA_WriteRegister(motor->slot, base + REG_CONTROL_FLAGS, ctrl);
    
    return 0;
}

/**
 * @brief Set motor speed/direction with timer (auto-stop after duration)
 */
int FPGA_Motor_SetSpeedDirectionTimed(FPGA_Motor_t *motor, uint8_t speed, 
                                       uint8_t direction, uint16_t duration_ms) {
    if (!motor || motor->channel > 15) {
        return -1;
    }
    
    if (direction > DIRECTION_REVERSE) {
        return -1;  // Invalid direction
    }
    
    // Duration in 100ms units (max 65535 * 100ms = 6553.5s)
    uint16_t duration_units = duration_ms / 100;
    if (duration_units == 0 && duration_ms > 0) {
        duration_units = 1;  // Minimum 100ms
    }
    
    uint8_t base = FPGA_MOTOR_REG_BASE(motor->channel);
    
    // 1. Hız yaz
    FPGA_WriteRegister(motor->slot, base + REG_SPEED, speed);
    
    // 2. Yön yaz
    FPGA_WriteRegister(motor->slot, base + REG_DIRECTION, direction);
    
    // 3. Timer duration yaz (16-bit)
    FPGA_WriteRegister(motor->slot, base + REG_TIMER_HIGH, (duration_units >> 8) & 0xFF);
    FPGA_WriteRegister(motor->slot, base + REG_TIMER_LOW, duration_units & 0xFF);
    
    // 4. Timer mode enable
    uint8_t ctrl = CTRL_FLAG_ENABLE | CTRL_FLAG_CONTROL_MODE | CTRL_FLAG_TIMER_MODE;
    FPGA_WriteRegister(motor->slot, base + REG_CONTROL_FLAGS, ctrl);
    
    return 0;
}

/**
 * @brief Check if timer-based control is running
 */
bool FPGA_Motor_IsTimerRunning(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        return false;
    }
    
    uint8_t status = FPGA_Motor_GetStatus(motor);
    return (status & STATUS_FLAG_TIMER_RUNNING) != 0;
}

/**
 * @brief Get remaining timer duration
 */
uint16_t FPGA_Motor_GetRemainingTime(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        return 0;
    }
    
    uint8_t base = FPGA_MOTOR_REG_BASE(motor->channel);
    
    uint8_t high, low;
    FPGA_ReadRegister(motor->slot, base + REG_TIMER_HIGH, &high);
    FPGA_ReadRegister(motor->slot, base + REG_TIMER_LOW, &low);
    
    uint16_t remaining_units = ((uint16_t)high << 8) | low;
    return remaining_units * 100;  // Convert to milliseconds
}

// ============================================================================
// Motor Control API - Homing
// ============================================================================

/**
 * @brief Home motor (set current position to zero)
 */
int FPGA_Motor_Home(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        return -1;
    }
    
    uint8_t base = FPGA_MOTOR_REG_BASE(motor->channel);
    
    // HOME_REQUEST flag
    uint8_t ctrl = CTRL_FLAG_ENABLE | CTRL_FLAG_HOME_REQUEST;
    FPGA_WriteRegister(motor->slot, base + REG_CONTROL_FLAGS, ctrl);
    
    return 0;
}

/**
 * @brief Check if motor is homed
 */
bool FPGA_Motor_IsHomed(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        return false;
    }
    
    uint8_t status = FPGA_Motor_GetStatus(motor);
    return (status & STATUS_FLAG_HOMED) != 0;
}

// ============================================================================
// Motor Status & Position Feedback
// ============================================================================

/**
 * @brief Get current motor position
 */
int32_t FPGA_Motor_GetPosition(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        return 0;
    }
    
    uint8_t base = FPGA_MOTOR_REG_BASE(motor->channel);
    
    uint8_t high, mid, low;
    FPGA_ReadRegister(motor->slot, base + REG_CURRENT_POS_HIGH, &high);
    FPGA_ReadRegister(motor->slot, base + REG_CURRENT_POS_MID, &mid);
    FPGA_ReadRegister(motor->slot, base + REG_CURRENT_POS_LOW, &low);
    
    int32_t position = ((int32_t)high << 16) | ((int32_t)mid << 8) | low;
    
    // 24-bit signed to 32-bit signed conversion
    if (position & 0x800000) {
        position |= 0xFF000000;
    }
    
    return position;
}

/**
 * @brief Get motor status flags
 */
uint8_t FPGA_Motor_GetStatus(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        return 0;
    }
    
    uint8_t base = FPGA_MOTOR_REG_BASE(motor->channel);
    uint8_t status;
    FPGA_ReadRegister(motor->slot, base + REG_STATUS_FLAGS, &status);
    
    return status;
}

/**
 * @brief Get motor error code
 */
uint8_t FPGA_Motor_GetError(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        return ERROR_INVALID_COMMAND;
    }
    
    uint8_t base = FPGA_MOTOR_REG_BASE(motor->channel);
    uint8_t error;
    FPGA_ReadRegister(motor->slot, base + REG_ERROR_CODE, &error);
    
    return error;
}

/**
 * @brief Check if motor is busy
 */
bool FPGA_Motor_IsBusy(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        return false;
    }
    
    uint8_t status = FPGA_Motor_GetStatus(motor);
    return (status & STATUS_FLAG_BUSY) != 0;
}

/**
 * @brief Check if motor has error
 */
bool FPGA_Motor_HasError(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        return false;
    }
    
    uint8_t status = FPGA_Motor_GetStatus(motor);
    return (status & STATUS_FLAG_ERROR) != 0;
}

/**
 * @brief Clear motor error flags
 */
int FPGA_Motor_ClearError(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        return -1;
    }
    
    uint8_t base = FPGA_MOTOR_REG_BASE(motor->channel);
    
    // CLEAR_ERROR flag
    uint8_t ctrl = CTRL_FLAG_CLEAR_ERROR;
    FPGA_WriteRegister(motor->slot, base + REG_CONTROL_FLAGS, ctrl);
    
    return 0;
}

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Convert error code to string
 */
const char* FPGA_Motor_ErrorToString(uint8_t error_code) {
    switch (error_code) {
        case ERROR_NONE:                return "No error";
        case ERROR_MOTOR_FAULT:         return "Motor driver fault";
        case ERROR_ENCODER_TIMEOUT:     return "Encoder timeout";
        case ERROR_POSITION_LIMIT:      return "Position limit exceeded";
        case ERROR_INVALID_COMMAND:     return "Invalid command";
        case ERROR_OVER_TEMPERATURE:    return "Over-temperature";
        default:                        return "Unknown error";
    }
}

/**
 * @brief Convert direction code to string
 */
const char* FPGA_Motor_DirectionToString(uint8_t direction) {
    switch (direction) {
        case DIRECTION_STOP:     return "STOP";
        case DIRECTION_FORWARD:  return "FORWARD";
        case DIRECTION_REVERSE:  return "REVERSE";
        default:                 return "UNKNOWN";
    }
}

/**
 * @brief Print motor status (detailed)
 */
void FPGA_Motor_PrintStatus(FPGA_Motor_t *motor) {
    if (!motor || motor->channel > 15) {
        UART_SendString("Hata: Geçersiz motor\r\n");
        return;
    }
    
    UART_SendString("\r\n=== Motor ");
    UART_SendHex8(motor->channel);
    UART_SendString(" Status ===\r\n");
    
    // Position
    int32_t pos = FPGA_Motor_GetPosition(motor);
    UART_SendString("Position: ");
    if (pos < 0) {
        UART_SendString("-");
        pos = -pos;
    }
    char buf[12];
    sprintf(buf, "%ld", pos);
    UART_SendString(buf);
    UART_SendString("\r\n");
    
    // Status flags
    uint8_t status = FPGA_Motor_GetStatus(motor);
    UART_SendString("Status: ");
    if (status & STATUS_FLAG_BUSY) UART_SendString("BUSY ");
    if (status & STATUS_FLAG_POSITION_REACHED) UART_SendString("REACHED ");
    if (status & STATUS_FLAG_HOMED) UART_SendString("HOMED ");
    if (status & STATUS_FLAG_ERROR) UART_SendString("ERROR ");
    if (status & STATUS_FLAG_FAULT) UART_SendString("FAULT ");
    if (status & STATUS_FLAG_TIMEOUT) UART_SendString("TIMEOUT ");
    if (status & STATUS_FLAG_OTW) UART_SendString("OTW ");
    if (status == 0) UART_SendString("IDLE");
    UART_SendString("\r\n");
    
    // Error
    if (status & STATUS_FLAG_ERROR) {
        uint8_t error = FPGA_Motor_GetError(motor);
        UART_SendString("Error: ");
        UART_SendString(FPGA_Motor_ErrorToString(error));
        UART_SendString("\r\n");
    }
    
    UART_SendString("====================\r\n");
}

/**
 * @brief Print FPGA module status
 */
void FPGA_PrintStatus(uint8_t slot) {
    FPGA_Module* module = FPGA_GetModule(slot);
    if (!module) {
        UART_SendString("Hata: Modül bulunamadı\r\n");
        return;
    }
    
    UART_SendString("\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString(" FPGA Motor Controller Module\r\n");
    UART_SendString("========================================\r\n");
    UART_SendString("Slot: ");
    UART_SendHex8(module->slot);
    UART_SendString("\r\n");
    UART_SendString("Initialized: ");
    UART_SendString(module->initialized ? "Yes" : "No");
    UART_SendString("\r\n\r\n");
    
    UART_SendString("Motor Channels: 0-15 (16 total)\r\n");
    UART_SendString("Register Map: 16 bytes per motor\r\n\r\n");
    
    UART_SendString("Active Motors:\r\n");
    UART_SendString("Ch  Pos     Status\r\n");
    UART_SendString("--  ------  ------\r\n");
    
    for (uint8_t ch = 0; ch < 16; ch++) {
        uint8_t base = FPGA_MOTOR_REG_BASE(ch);
        uint8_t ctrl;
        FPGA_ReadRegister(slot, base + REG_CONTROL_FLAGS, &ctrl);
        
        if (ctrl & CTRL_FLAG_ENABLE) {
            FPGA_Motor_t motor = { slot, ch };
            int32_t pos = FPGA_Motor_GetPosition(&motor);
            uint8_t status = FPGA_Motor_GetStatus(&motor);
            
            UART_SendHex8(ch);
            UART_SendString("  ");
            
            char buf[8];
            sprintf(buf, "%6ld", pos);
            UART_SendString(buf);
            UART_SendString("  ");
            
            if (status & STATUS_FLAG_BUSY) UART_SendString("BUSY ");
            if (status & STATUS_FLAG_ERROR) UART_SendString("ERR ");
            if (status & STATUS_FLAG_HOMED) UART_SendString("HOME ");
            
            UART_SendString("\r\n");
        }
    }
    
    UART_SendString("========================================\r\n");
    UART_SendString("Commands: motor:CH:goto/speed/home/status\r\n");
    UART_SendString("========================================\r\n");
}

// ============================================================================
// Command Handler
// ============================================================================

/**
 * @brief Parse integer from string
 */
static int32_t parse_int(const char** str) {
    const char* p = *str;
    bool negative = false;
    int32_t value = 0;
    
    // Skip whitespace
    while (*p == ' ' || *p == '\t') p++;
    
    // Check sign
    if (*p == '-') {
        negative = true;
        p++;
    } else if (*p == '+') {
        p++;
    }
    
    // Parse hex (0x prefix)
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
        while ((*p >= '0' && *p <= '9') || 
               (*p >= 'a' && *p <= 'f') || 
               (*p >= 'A' && *p <= 'F')) {
            uint8_t digit;
            if (*p >= '0' && *p <= '9') {
                digit = *p - '0';
            } else if (*p >= 'a' && *p <= 'f') {
                digit = *p - 'a' + 10;
            } else {
                digit = *p - 'A' + 10;
            }
            value = (value << 4) | digit;
            p++;
        }
    }
    // Parse decimal
    else {
        while (*p >= '0' && *p <= '9') {
            value = value * 10 + (*p - '0');
            p++;
        }
    }
    
    *str = p;
    return negative ? -value : value;
}

/**
 * @brief Handle FPGA commands from UART
 * Format: fpga:SLOT:KOMUT
 * 
 * Basic Commands:
 *   fpga:2:readreg:0x10
 *   fpga:2:writereg:0x20:0xFF
 *   fpga:2:reset
 *   fpga:2:status
 * 
 * Motor Commands:
 *   fpga:2:motor:0:goto:1000:128      - Position control
 *   fpga:2:motor:0:speed:200:1        - Speed/direction control (1=fwd, 2=rev)
 *   fpga:2:motor:0:stop               - Stop motor
 *   fpga:2:motor:0:home               - Home (position=0)
 *   fpga:2:motor:0:position           - Read position
 *   fpga:2:motor:0:status             - Read status
 *   fpga:2:motor:0:clearerror         - Clear error
 */
void FPGA_HandleCommand(const char* cmd) {
    // ACK gönder (komut alındı onayı)
    UART_SendString("[ACK:fpga:");
    UART_SendString(cmd);
    UART_SendString("]\r\n");
    
    // Slot'u parse et
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
    
    // Komutu kontrol et
    if (strncmp(cmd, "readreg:", 8) == 0) {
        // readreg:ADDRESS
        cmd += 8;
        
        // Hex address parse et (0x prefix varsa atla)
        if (cmd[0] == '0' && (cmd[1] == 'x' || cmd[1] == 'X')) {
            cmd += 2;
        }
        
        uint8_t address = 0;
        while ((*cmd >= '0' && *cmd <= '9') || 
               (*cmd >= 'a' && *cmd <= 'f') || 
               (*cmd >= 'A' && *cmd <= 'F')) {
            uint8_t digit;
            if (*cmd >= '0' && *cmd <= '9') {
                digit = *cmd - '0';
            } else if (*cmd >= 'a' && *cmd <= 'f') {
                digit = *cmd - 'a' + 10;
            } else {
                digit = *cmd - 'A' + 10;
            }
            address = (address << 4) | digit;
            cmd++;
        }
        
        uint8_t value;
        if (FPGA_ReadRegister(slot, address, &value) == 0) {
            UART_SendString("Register 0x");
            UART_SendHex8(address);
            UART_SendString(" = 0x");
            UART_SendHex8(value);
            UART_SendString("\r\n");
        } else {
            UART_SendString("Hata: Register okunamadı\r\n");
        }
    }
    else if (strncmp(cmd, "writereg:", 9) == 0) {
        // writereg:ADDRESS:VALUE
        cmd += 9;
        
        // Address parse
        if (cmd[0] == '0' && (cmd[1] == 'x' || cmd[1] == 'X')) {
            cmd += 2;
        }
        
        uint8_t address = 0;
        while ((*cmd >= '0' && *cmd <= '9') || 
               (*cmd >= 'a' && *cmd <= 'f') || 
               (*cmd >= 'A' && *cmd <= 'F')) {
            uint8_t digit;
            if (*cmd >= '0' && *cmd <= '9') {
                digit = *cmd - '0';
            } else if (*cmd >= 'a' && *cmd <= 'f') {
                digit = *cmd - 'a' + 10;
            } else {
                digit = *cmd - 'A' + 10;
            }
            address = (address << 4) | digit;
            cmd++;
        }
        
        if (*cmd != ':') {
            UART_SendString("Hata: Format hatası\r\n");
            return;
        }
        cmd++;
        
        // Value parse
        if (cmd[0] == '0' && (cmd[1] == 'x' || cmd[1] == 'X')) {
            cmd += 2;
        }
        
        uint8_t value = 0;
        while ((*cmd >= '0' && *cmd <= '9') || 
               (*cmd >= 'a' && *cmd <= 'f') || 
               (*cmd >= 'A' && *cmd <= 'F')) {
            uint8_t digit;
            if (*cmd >= '0' && *cmd <= '9') {
                digit = *cmd - '0';
            } else if (*cmd >= 'a' && *cmd <= 'f') {
                digit = *cmd - 'a' + 10;
            } else {
                digit = *cmd - 'A' + 10;
            }
            value = (value << 4) | digit;
            cmd++;
        }
        
        if (FPGA_WriteRegister(slot, address, value) == 0) {
            UART_SendString("OK: Register 0x");
            UART_SendHex8(address);
            UART_SendString(" = 0x");
            UART_SendHex8(value);
            UART_SendString("\r\n");
        } else {
            UART_SendString("Hata: Register yazılamadı\r\n");
        }
    }
    else if (strcmp(cmd, "reset") == 0) {
        if (FPGA_Reset(slot) == 0) {
            UART_SendString("OK: FPGA reset edildi\r\n");
        } else {
            UART_SendString("Hata: Reset başarısız\r\n");
        }
    }
    else if (strcmp(cmd, "status") == 0) {
        FPGA_PrintStatus(slot);
    }
    else if (strncmp(cmd, "motor:", 6) == 0) {
        // Motor komutları: motor:CH:COMMAND:PARAMS
        cmd += 6;
        
        // Motor channel parse et
        const char* ch_start = cmd;
        int32_t channel = parse_int(&cmd);
        
        if (channel < 0 || channel > 15) {
            UART_SendString("Hata: Geçersiz motor channel (0-15)\r\n");
            return;
        }
        
        if (*cmd != ':') {
            UART_SendString("Hata: Format hatası\r\n");
            return;
        }
        cmd++;
        
        FPGA_Motor_t motor = { slot, (uint8_t)channel };
        
        // Motor komutları
        if (strncmp(cmd, "goto:", 5) == 0) {
            // goto:POSITION:SPEED
            cmd += 5;
            
            int32_t target_pos = parse_int(&cmd);
            
            if (*cmd != ':') {
                UART_SendString("Hata: Format hatası (goto:POS:SPEED)\r\n");
                return;
            }
            cmd++;
            
            int32_t speed = parse_int(&cmd);
            
            if (speed < 0 || speed > 255) {
                UART_SendString("Hata: Geçersiz hız (0-255)\r\n");
                return;
            }
            
            if (FPGA_Motor_GoToPosition(&motor, target_pos, (uint8_t)speed) == 0) {
                UART_SendString("Motor ");
                UART_SendHex8(motor.channel);
                UART_SendString(": GoTo pozisyon ");
                char buf[12];
                sprintf(buf, "%ld", target_pos);
                UART_SendString(buf);
                UART_SendString(" @ hiz ");
                UART_SendHex8((uint8_t)speed);
                UART_SendString("\r\n");
            } else {
                UART_SendString("Hata: Pozisyon komutu gönderilemedi\r\n");
            }
        }
        else if (strncmp(cmd, "speed:", 6) == 0) {
            // speed:SPEED:DIRECTION
            cmd += 6;
            
            int32_t speed = parse_int(&cmd);
            
            if (*cmd != ':') {
                UART_SendString("Hata: Format hatası (speed:SPEED:DIR)\r\n");
                return;
            }
            cmd++;
            
            int32_t direction = parse_int(&cmd);
            
            if (speed < 0 || speed > 255) {
                UART_SendString("Hata: Geçersiz hız (0-255)\r\n");
                return;
            }
            
            if (direction < 0 || direction > 2) {
                UART_SendString("Hata: Geçersiz yön (0=stop, 1=ileri, 2=geri)\r\n");
                return;
            }
            
            if (FPGA_Motor_SetSpeedDirection(&motor, (uint8_t)speed, (uint8_t)direction) == 0) {
                UART_SendString("Motor ");
                UART_SendHex8(motor.channel);
                UART_SendString(": Hız=");
                UART_SendHex8((uint8_t)speed);
                UART_SendString(", Yön=");
                UART_SendString(FPGA_Motor_DirectionToString((uint8_t)direction));
                UART_SendString("\r\n");
            } else {
                UART_SendString("Hata: Hız komutu gönderilemedi\r\n");
            }
        }
        else if (strcmp(cmd, "stop") == 0) {
            if (FPGA_Motor_Stop(&motor) == 0) {
                UART_SendString("Motor ");
                UART_SendHex8(motor.channel);
                UART_SendString(": Durduruldu\r\n");
            } else {
                UART_SendString("Hata: Dur komutu gönderilemedi\r\n");
            }
        }
        else if (strcmp(cmd, "home") == 0) {
            if (FPGA_Motor_Home(&motor) == 0) {
                UART_SendString("Motor ");
                UART_SendHex8(motor.channel);
                UART_SendString(": Homing (pozisyon=0)\r\n");
            } else {
                UART_SendString("Hata: Home komutu gönderilemedi\r\n");
            }
        }
        else if (strcmp(cmd, "position") == 0) {
            int32_t pos = FPGA_Motor_GetPosition(&motor);
            UART_SendString("Motor ");
            UART_SendHex8(motor.channel);
            UART_SendString(" pozisyon: ");
            char buf[12];
            sprintf(buf, "%ld", pos);
            UART_SendString(buf);
            UART_SendString("\r\n");
        }
        else if (strcmp(cmd, "status") == 0) {
            FPGA_Motor_PrintStatus(&motor);
        }
        else if (strcmp(cmd, "clearerror") == 0) {
            if (FPGA_Motor_ClearError(&motor) == 0) {
                UART_SendString("Motor ");
                UART_SendHex8(motor.channel);
                UART_SendString(": Hata temizlendi\r\n");
            } else {
                UART_SendString("Hata: Clear error komutu gönderilemedi\r\n");
            }
        }
        else if (strncmp(cmd, "speedtimed:", 11) == 0) {
            // speedtimed:SPEED:DIRECTION:DURATION_MS
            cmd += 11;
            
            int32_t speed = parse_int(&cmd);
            
            if (*cmd != ':') {
                UART_SendString("Hata: Format hatası (speedtimed:SPEED:DIR:MS)\r\n");
                return;
            }
            cmd++;
            
            int32_t direction = parse_int(&cmd);
            
            if (*cmd != ':') {
                UART_SendString("Hata: Format hatası (speedtimed:SPEED:DIR:MS)\r\n");
                return;
            }
            cmd++;
            
            int32_t duration_ms = parse_int(&cmd);
            
            if (speed < 0 || speed > 255) {
                UART_SendString("Hata: Geçersiz hız (0-255)\r\n");
                return;
            }
            
            if (direction < 0 || direction > 2) {
                UART_SendString("Hata: Geçersiz yön (0=stop, 1=ileri, 2=geri)\r\n");
                return;
            }
            
            if (duration_ms < 0 || duration_ms > 6553500) {
                UART_SendString("Hata: Geçersiz süre (0-6553500ms)\r\n");
                return;
            }
            
            if (FPGA_Motor_SetSpeedDirectionTimed(&motor, (uint8_t)speed, (uint8_t)direction, (uint16_t)duration_ms) == 0) {
                UART_SendString("Motor ");
                UART_SendHex8(motor.channel);
                UART_SendString(": Zamanlı kontrol Hız=");
                UART_SendHex8((uint8_t)speed);
                UART_SendString(", Yön=");
                UART_SendString(FPGA_Motor_DirectionToString((uint8_t)direction));
                UART_SendString(", Süre=");
                char buf[12];
                sprintf(buf, "%ld", duration_ms);
                UART_SendString(buf);
                UART_SendString("ms\r\n");
            } else {
                UART_SendString("Hata: Zamanlı kontrol komutu gönderilemedi\r\n");
            }
        }
        else if (strcmp(cmd, "timerinfo") == 0) {
            bool running = FPGA_Motor_IsTimerRunning(&motor);
            uint16_t remaining = FPGA_Motor_GetRemainingTime(&motor);
            
            UART_SendString("Motor ");
            UART_SendHex8(motor.channel);
            UART_SendString(" Timer: ");
            
            if (running) {
                UART_SendString("ÇALIŞIYOR, Kalan=");
                char buf[12];
                sprintf(buf, "%u", remaining);
                UART_SendString(buf);
                UART_SendString("ms\r\n");
            } else {
                UART_SendString("DURDU\r\n");
            }
        }
        else {
            UART_SendString("Hata: Bilinmeyen motor komutu\r\n");
            UART_SendString("Kullanım:\r\n");
            UART_SendString("  fpga:SLOT:motor:CH:goto:POS:SPEED\r\n");
            UART_SendString("  fpga:SLOT:motor:CH:speed:SPEED:DIR\r\n");
            UART_SendString("  fpga:SLOT:motor:CH:speedtimed:SPEED:DIR:MS\r\n");
            UART_SendString("  fpga:SLOT:motor:CH:stop\r\n");
            UART_SendString("  fpga:SLOT:motor:CH:home\r\n");
            UART_SendString("  fpga:SLOT:motor:CH:position\r\n");
            UART_SendString("  fpga:SLOT:motor:CH:status\r\n");
            UART_SendString("  fpga:SLOT:motor:CH:timerinfo\r\n");
            UART_SendString("  fpga:SLOT:motor:CH:clearerror\r\n");
        }
    }
    else {
        UART_SendString("Hata: Bilinmeyen komut\r\n");
        UART_SendString("Kullanım:\r\n");
        UART_SendString("  fpga:SLOT:readreg:ADDR\r\n");
        UART_SendString("  fpga:SLOT:writereg:ADDR:VALUE\r\n");
        UART_SendString("  fpga:SLOT:reset\r\n");
        UART_SendString("  fpga:SLOT:status\r\n");
        UART_SendString("  fpga:SLOT:motor:CH:goto:POS:SPEED\r\n");
        UART_SendString("  fpga:SLOT:motor:CH:speed:SPEED:DIR\r\n");
        UART_SendString("  fpga:SLOT:motor:CH:stop\r\n");
        UART_SendString("  fpga:SLOT:motor:CH:home\r\n");
        UART_SendString("  fpga:SLOT:motor:CH:position\r\n");
        UART_SendString("  fpga:SLOT:motor:CH:status\r\n");
    }
    
    // Komut tamamlandı mesajı (burjuva_manager için)
    UART_SendString("\r\nKomut tamamlandi: fpga\r\n");
}
