/**
 * @file fpga.h
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
 */

#ifndef __FPGA_H
#define __FPGA_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Register Map Definitions (16 bytes per motor)
// ============================================================================

// Base address calculation
#define FPGA_MOTOR_REG_BASE(ch)     ((ch) * 16)

// Register offsets (relative to base)
#define REG_CONTROL_FLAGS       0x00    // R/W: Control flags
#define REG_STATUS_FLAGS        0x01    // R  : Status flags
#define REG_ERROR_CODE          0x02    // R  : Error code
#define REG_RESERVED_03         0x03    // Reserved
#define REG_CURRENT_POS_HIGH    0x04    // R  : Current position[23:16]
#define REG_CURRENT_POS_MID     0x05    // R  : Current position[15:8]
#define REG_CURRENT_POS_LOW     0x06    // R  : Current position[7:0]
#define REG_RESERVED_07         0x07    // Reserved
#define REG_TARGET_POS_HIGH     0x08    // R/W: Target position[23:16]
#define REG_TARGET_POS_MID      0x09    // R/W: Target position[15:8]
#define REG_TARGET_POS_LOW      0x0A    // R/W: Target position[7:0]
#define REG_RESERVED_0B         0x0B    // Reserved
#define REG_SPEED               0x0C    // R/W: Speed (PWM duty cycle, 0-255)
#define REG_DIRECTION           0x0D    // R/W: Direction (0=stop, 1=fwd, 2=rev)
#define REG_TIMER_HIGH          0x0E    // R/W: Timer duration[15:8] (in 100ms units)
#define REG_TIMER_LOW           0x0F    // R/W: Timer duration[7:0] (max 6553.5s)

// ============================================================================
// Control Flags (REG_CONTROL_FLAGS) Bits
// ============================================================================

#define CTRL_FLAG_ENABLE            (1 << 7)    // Enable motor
#define CTRL_FLAG_CONTROL_MODE      (1 << 6)    // 0=position, 1=speed/dir
#define CTRL_FLAG_HOME_REQUEST      (1 << 5)    // Request homing
#define CTRL_FLAG_EMERGENCY_STOP    (1 << 4)    // Emergency stop
#define CTRL_FLAG_CLEAR_ERROR       (1 << 3)    // Clear error flags
#define CTRL_FLAG_TIMER_MODE        (1 << 2)    // Timer-based control (speed/dir for duration)

// ============================================================================
// Status Flags (REG_STATUS_FLAGS) Bits
// ============================================================================

#define STATUS_FLAG_BUSY            (1 << 7)    // Motor moving
#define STATUS_FLAG_POSITION_REACHED (1 << 6)   // Target position reached
#define STATUS_FLAG_HOMED           (1 << 5)    // Position zeroed
#define STATUS_FLAG_ERROR           (1 << 4)    // Error present
#define STATUS_FLAG_FAULT           (1 << 3)    // Motor driver fault
#define STATUS_FLAG_TIMEOUT         (1 << 2)    // Encoder timeout
#define STATUS_FLAG_OTW             (1 << 1)    // Over-temperature warning
#define STATUS_FLAG_TIMER_RUNNING   (1 << 0)    // Timer-based control active

// ============================================================================
// Error Codes (REG_ERROR_CODE)
// ============================================================================

#define ERROR_NONE                  0x00        // No error
#define ERROR_MOTOR_FAULT           0x01        // Motor driver fault
#define ERROR_ENCODER_TIMEOUT       0x02        // Encoder timeout
#define ERROR_POSITION_LIMIT        0x03        // Position limit exceeded
#define ERROR_INVALID_COMMAND       0x04        // Invalid command
#define ERROR_OVER_TEMPERATURE      0x05        // Over-temperature

// ============================================================================
// Direction Codes (REG_DIRECTION)
// ============================================================================

#define DIRECTION_STOP              0x00        // Stop motor
#define DIRECTION_FORWARD           0x01        // Forward (CW)
#define DIRECTION_REVERSE           0x02        // Reverse (CCW)

// ============================================================================
// Motor Handle Structure
// ============================================================================

typedef struct {
    uint8_t slot;                   // FPGA module slot
    uint8_t channel;                // Motor channel (0-15)
} FPGA_Motor_t;

// ============================================================================
// Module Management Functions
// ============================================================================

void FPGA_Register(uint8_t slot);
int FPGA_ReadRegister(uint8_t slot, uint8_t address, uint8_t* value);
int FPGA_WriteRegister(uint8_t slot, uint8_t address, uint8_t value);
int FPGA_Reset(uint8_t slot);
void FPGA_PrintStatus(uint8_t slot);
void FPGA_HandleCommand(const char* cmd);

// ============================================================================
// Motor Control API - Position Control (Closed-Loop)
// ============================================================================

/**
 * @brief Move motor to target position
 * @param motor Motor handle
 * @param target_pos Target position (24-bit signed, ±8,388,608)
 * @param speed Speed (0-255, PWM duty cycle)
 * @return 0 if successful, -1 on error
 */
int FPGA_Motor_GoToPosition(FPGA_Motor_t *motor, int32_t target_pos, uint8_t speed);

/**
 * @brief Check if motor has reached target position
 * @param motor Motor handle
 * @return true if position reached
 */
bool FPGA_Motor_IsPositionReached(FPGA_Motor_t *motor);

// ============================================================================
// Motor Control API - Speed/Direction Control (Open-Loop)
// ============================================================================

/**
 * @brief Set motor speed and direction (open-loop)
 * @param motor Motor handle
 * @param speed Speed (0-255, PWM duty cycle)
 * @param direction Direction (DIRECTION_STOP/FORWARD/REVERSE)
 * @return 0 if successful, -1 on error
 */
int FPGA_Motor_SetSpeedDirection(FPGA_Motor_t *motor, uint8_t speed, uint8_t direction);

/**
 * @brief Stop motor immediately
 * @param motor Motor handle
 * @return 0 if successful, -1 on error
 */
int FPGA_Motor_Stop(FPGA_Motor_t *motor);

/**
 * @brief Emergency stop (works in any mode)
 * @param motor Motor handle
 * @return 0 if successful, -1 on error
 */
int FPGA_Motor_EmergencyStop(FPGA_Motor_t *motor);

/**
 * @brief Set motor speed/direction with timer (auto-stop after duration)
 * @param motor Motor handle
 * @param speed Speed (0-255, PWM duty cycle)
 * @param direction Direction (DIRECTION_STOP/FORWARD/REVERSE)
 * @param duration_ms Duration in milliseconds (max 6553500ms = ~109 minutes)
 * @return 0 if successful, -1 on error
 */
int FPGA_Motor_SetSpeedDirectionTimed(FPGA_Motor_t *motor, uint8_t speed, 
                                       uint8_t direction, uint16_t duration_ms);

/**
 * @brief Check if timer-based control is running
 * @param motor Motor handle
 * @return true if timer running
 */
bool FPGA_Motor_IsTimerRunning(FPGA_Motor_t *motor);

/**
 * @brief Get remaining timer duration
 * @param motor Motor handle
 * @return Remaining time in milliseconds
 */
uint16_t FPGA_Motor_GetRemainingTime(FPGA_Motor_t *motor);

// ============================================================================
// Motor Control API - Homing
// ============================================================================

/**
 * @brief Home motor (set current position to zero)
 * @param motor Motor handle
 * @return 0 if successful, -1 on error
 */
int FPGA_Motor_Home(FPGA_Motor_t *motor);

/**
 * @brief Check if motor is homed
 * @param motor Motor handle
 * @return true if homed
 */
bool FPGA_Motor_IsHomed(FPGA_Motor_t *motor);

// ============================================================================
// Motor Status & Position Feedback
// ============================================================================

/**
 * @brief Get current motor position
 * @param motor Motor handle
 * @return Current position (24-bit signed)
 */
int32_t FPGA_Motor_GetPosition(FPGA_Motor_t *motor);

/**
 * @brief Get motor status flags
 * @param motor Motor handle
 * @return Status flags byte
 */
uint8_t FPGA_Motor_GetStatus(FPGA_Motor_t *motor);

/**
 * @brief Get motor error code
 * @param motor Motor handle
 * @return Error code
 */
uint8_t FPGA_Motor_GetError(FPGA_Motor_t *motor);

/**
 * @brief Check if motor is busy (moving)
 * @param motor Motor handle
 * @return true if busy
 */
bool FPGA_Motor_IsBusy(FPGA_Motor_t *motor);

/**
 * @brief Check if motor has error
 * @param motor Motor handle
 * @return true if error present
 */
bool FPGA_Motor_HasError(FPGA_Motor_t *motor);

/**
 * @brief Clear motor error flags
 * @param motor Motor handle
 * @return 0 if successful, -1 on error
 */
int FPGA_Motor_ClearError(FPGA_Motor_t *motor);

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Convert error code to string
 * @param error_code Error code
 * @return Error description string
 */
const char* FPGA_Motor_ErrorToString(uint8_t error_code);

/**
 * @brief Convert direction code to string
 * @param direction Direction code
 * @return Direction string
 */
const char* FPGA_Motor_DirectionToString(uint8_t direction);

/**
 * @brief Print motor status (detailed)
 * @param motor Motor handle
 */
void FPGA_Motor_PrintStatus(FPGA_Motor_t *motor);

#endif // __FPGA_H
