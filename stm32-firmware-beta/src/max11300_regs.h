/**
 * Burjuva Pilot - MAX11300 Register Definitions
 * PIXI - Programmable Mixed-Signal I/O (20-port, 12-bit)
 * 
 * Used by AIO20 module for analog I/O
 */

#ifndef MAX11300_REGS_H
#define MAX11300_REGS_H

#include <stdint.h>

// MAX11300 Register Addresses
typedef enum {
    MAX11300_REG_DEV_ID                 = 0x00,  // Device ID (Read-only, should be 0x0424)
    MAX11300_REG_INTERRUPT_FLAG         = 0x01,  // Interrupt flags
    MAX11300_REG_ADC_STATUS_15_0        = 0x02,  // ADC status bits 15-0
    MAX11300_REG_ADC_STATUS_19_16       = 0x03,  // ADC status bits 19-16
    MAX11300_REG_DAC_STATUS_15_0        = 0x04,  // DAC status bits 15-0
    MAX11300_REG_DAC_STATUS_19_16       = 0x05,  // DAC status bits 19-16
    MAX11300_REG_DEVICE_CONTROL         = 0x10,  // Device control
    MAX11300_REG_INTERRUPT              = 0x11,  // Interrupt mask
    MAX11300_REG_GPO_DATA_15_0          = 0x0D,  // GPIO output data 15-0
    MAX11300_REG_GPO_DATA_19_16         = 0x0E,  // GPIO output data 19-16
    MAX11300_REG_GPI_DATA_15_0          = 0x0B,  // GPIO input data 15-0
    MAX11300_REG_GPI_DATA_19_16         = 0x0C,  // GPIO input data 19-16
    
    // Port Configuration (0x20-0x33, one per port 0-19)
    MAX11300_REG_PORT_CFG_00            = 0x20,
    MAX11300_REG_PORT_CFG_01            = 0x21,
    MAX11300_REG_PORT_CFG_02            = 0x22,
    MAX11300_REG_PORT_CFG_03            = 0x23,
    MAX11300_REG_PORT_CFG_04            = 0x24,
    MAX11300_REG_PORT_CFG_05            = 0x25,
    MAX11300_REG_PORT_CFG_06            = 0x26,
    MAX11300_REG_PORT_CFG_07            = 0x27,
    MAX11300_REG_PORT_CFG_08            = 0x28,
    MAX11300_REG_PORT_CFG_09            = 0x29,
    MAX11300_REG_PORT_CFG_10            = 0x2A,
    MAX11300_REG_PORT_CFG_11            = 0x2B,
    MAX11300_REG_PORT_CFG_12            = 0x2C,
    MAX11300_REG_PORT_CFG_13            = 0x2D,
    MAX11300_REG_PORT_CFG_14            = 0x2E,
    MAX11300_REG_PORT_CFG_15            = 0x2F,
    MAX11300_REG_PORT_CFG_16            = 0x30,
    MAX11300_REG_PORT_CFG_17            = 0x31,
    MAX11300_REG_PORT_CFG_18            = 0x32,
    MAX11300_REG_PORT_CFG_19            = 0x33,
    
    // ADC Data Registers (0x40-0x53, one per port)
    MAX11300_REG_ADC_DATA_PORT_00       = 0x40,
    MAX11300_REG_ADC_DATA_PORT_01       = 0x41,
    MAX11300_REG_ADC_DATA_PORT_02       = 0x42,
    MAX11300_REG_ADC_DATA_PORT_03       = 0x43,
    MAX11300_REG_ADC_DATA_PORT_04       = 0x44,
    MAX11300_REG_ADC_DATA_PORT_05       = 0x45,
    MAX11300_REG_ADC_DATA_PORT_06       = 0x46,
    MAX11300_REG_ADC_DATA_PORT_07       = 0x47,
    MAX11300_REG_ADC_DATA_PORT_08       = 0x48,
    MAX11300_REG_ADC_DATA_PORT_09       = 0x49,
    MAX11300_REG_ADC_DATA_PORT_10       = 0x4A,
    MAX11300_REG_ADC_DATA_PORT_11       = 0x4B,
    MAX11300_REG_ADC_DATA_PORT_12       = 0x4C,
    MAX11300_REG_ADC_DATA_PORT_13       = 0x4D,
    MAX11300_REG_ADC_DATA_PORT_14       = 0x4E,
    MAX11300_REG_ADC_DATA_PORT_15       = 0x4F,
    MAX11300_REG_ADC_DATA_PORT_16       = 0x50,
    MAX11300_REG_ADC_DATA_PORT_17       = 0x51,
    MAX11300_REG_ADC_DATA_PORT_18       = 0x52,
    MAX11300_REG_ADC_DATA_PORT_19       = 0x53,
    
    // DAC Data Registers (0x60-0x73, one per port)
    MAX11300_REG_DAC_DATA_PORT_00       = 0x60,
    MAX11300_REG_DAC_DATA_PORT_01       = 0x61,
    MAX11300_REG_DAC_DATA_PORT_02       = 0x62,
    MAX11300_REG_DAC_DATA_PORT_03       = 0x63,
    MAX11300_REG_DAC_DATA_PORT_04       = 0x64,
    MAX11300_REG_DAC_DATA_PORT_05       = 0x65,
    MAX11300_REG_DAC_DATA_PORT_06       = 0x66,
    MAX11300_REG_DAC_DATA_PORT_07       = 0x67,
    MAX11300_REG_DAC_DATA_PORT_08       = 0x68,
    MAX11300_REG_DAC_DATA_PORT_09       = 0x69,
    MAX11300_REG_DAC_DATA_PORT_10       = 0x6A,
    MAX11300_REG_DAC_DATA_PORT_11       = 0x6B,
    MAX11300_REG_DAC_DATA_PORT_12       = 0x6C,
    MAX11300_REG_DAC_DATA_PORT_13       = 0x6D,
    MAX11300_REG_DAC_DATA_PORT_14       = 0x6E,
    MAX11300_REG_DAC_DATA_PORT_15       = 0x6F,
    MAX11300_REG_DAC_DATA_PORT_16       = 0x70,
    MAX11300_REG_DAC_DATA_PORT_17       = 0x71,
    MAX11300_REG_DAC_DATA_PORT_18       = 0x72,
    MAX11300_REG_DAC_DATA_PORT_19       = 0x73,
} MAX11300_Register;

// SPI Command Format
#define MAX11300_SPI_WRITE(addr)    ((addr) << 1 | 0x00)  // Write: address << 1 | 0
#define MAX11300_SPI_READ(addr)     ((addr) << 1 | 0x01)  // Read:  address << 1 | 1

// Device ID expected value
#define MAX11300_DEVICE_ID          0x0424

// Port modes (bits 15-12 of PORT_CFG register)
#define MAX11300_MODE_0_HIGH_Z      0x0000  // High-impedance
#define MAX11300_MODE_1_GPI         0x1000  // Digital input with programmable threshold
#define MAX11300_MODE_2_BIDIR       0x2000  // Bidirectional level translator
#define MAX11300_MODE_3_GPO         0x3000  // Digital output (push-pull or open-drain)
#define MAX11300_MODE_4_UOUT        0x4000  // Unidirectional DAC output
#define MAX11300_MODE_5_AOUT        0x5000  // Analog output with ADC monitoring
#define MAX11300_MODE_6_AOUT_PD     0x6000  // Analog output with ADC monitoring and power-down
#define MAX11300_MODE_7_AIN_0_10V   0x7000  // ADC input, 0 to +10V
#define MAX11300_MODE_8_AIN_5_5V    0x8000  // ADC input, -5V to +5V
#define MAX11300_MODE_9_AIN_10_0V   0x9000  // ADC input, -10V to 0V
#define MAX11300_MODE_10_AIN_0_2_5  0xA000  // ADC input, 0 to +2.5V

// Device Control bits
#define MAX11300_BRST               0x0020  // Burst mode enable
#define MAX11300_THSHDN             0x0010  // Thermal shutdown
#define MAX11300_ADCCONV_IDLE       0x0000  // ADC conversion mode: idle
#define MAX11300_ADCCONV_SINGLE_    0x0001  // ADC conversion mode: single sweep
#define MAX11300_ADCCONV_SINGLE     0x0002  // ADC conversion mode: single conversion
#define MAX11300_ADCCONV_CONTINUOUS 0x0003  // ADC conversion mode: continuous sweep

#endif // MAX11300_REGS_H
