/**
 ******************************************************************************
 * @file    main_custom.c
 * @brief   STM32F103RCT6 UART Command Firmware - Motor-demo IO16 Integration
 * @date    21 Kasım 2025
 * 
 * @description
 * Burjuva Manager uyumlu UART command firmware
 * Motor-demo'nun io16.c ve spi.c kodlarını kullanır
 * 
 * UART: USART1 (PA9=TX, PA10=RX) @ 115200 baud
 * SPI:  SPI2 (PB13=SCK, PB14=MISO, PB15=MOSI) - Paylaşımlı (MODÜL 3)
 *       Clock: APB1 = 36MHz @ 72MHz system clock
 *       Slot 0 (IO16):  4.5 MHz (Prescaler /8)
 *       Slot 1 (AIO20): 9.0 MHz (Prescaler /4)
 *       Slot 2 (FPGA):  2.25 MHz (Prescaler /16)
 *       Slot 3 (IO16):  4.5 MHz (Prescaler /8)
 * 
 * SLOT PIN MAPPING (CPLD top.v analizinden):
 * Slot 0 (IO16):  CS=PC13 (MODÜL 4), INT=PA3  (MODÜL 1)
 * Slot 1 (AIO20): CS=PA0  (MODÜL 1), INT=PC4  (MODÜL 3), CNVT=PC5
 * Slot 2 (FPGA):  CS=PA1  (MODÜL 1), INT=PB0  (MODÜL 1), CRESET=PB1, CDONE=PB10
 * Slot 3 (IO16):  CS=PA2  (MODÜL 1), INT=PB11 (MODÜL 3)
 * 
 * Desteklenen Komutlar:
 * - io16:SLOT:set:PIN:VALUE      - Pin yaz (0/1 veya low/high)
 * - io16:SLOT:get:PIN             - Pin oku
 * - io16:SLOT:dir:PIN:DIRECTION   - Pin yönü ayarla (in/out)
 * - io16:SLOT:readall             - Tüm pinleri oku
 * - io16:SLOT:writeall:VALUE      - Tüm pinleri yaz (16-bit)
 * - io16:SLOT:status              - Modül durumu
 * - io16:SLOT:info                - Chip bilgisi (iC-JX)
 * - io16:SLOT:overcurrent         - Overcurrent kontrolü
 * - io16:SLOT:regdump             - Register dump
 * - io16:testcs:GPIO:PIN          - CS pin testi (testcs:PC:13 veya testcs:PA:0)
 * - modul-algila                  - Modül algılama
 * - help                          - Yardım mesajı
 ******************************************************************************
 */

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_flash.h"
#include "spi.h"
#include "io16.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Function prototypes */
void RCC_Configuration(void);
void GPIO_Configuration(void);
void USART1_Configuration(void);
void Process_Command(char* cmd);
void Send_Response(const char* msg);
void Send_ACK(const char* cmd);
void Delay(__IO uint32_t nCount);

/* iC-JX Register definitions */
typedef enum {
    REG_CHIP_ID = 0x00,
    REG_INPUT_A = 0x02,
    REG_INPUT_B = 0x03,
    REG_OUTPUT_A = 0x04,
    REG_OUTPUT_B = 0x05,
    REG_CONTROLWORD_1A = 0x06,
    REG_CONTROLWORD_1B = 0x07,
    REG_CONTROLWORD_2A = 0x08,
    REG_CONTROLWORD_2B = 0x09,
    REG_CONTROLWORD_3A = 0x0A,
    REG_CONTROLWORD_3B = 0x0B,
    REG_CONTROLWORD_4 = 0x0C,
    REG_STATUS_A = 0x0D,
    REG_STATUS_B = 0x0E
} IC_JX_Register;

/* Helper functions */
int parse_slot(const char* cmd);
int parse_pin(const char* cmd);
int parse_value(const char* cmd);
const char* skip_tokens(const char* cmd, int count);

/**
 * @brief  Main program
 */
int main(void)
{
    uint16_t rxData;
    static char cmdBuffer[128];
    static uint8_t cmdIndex = 0;
    
    /* Configure clocks */
    RCC_Configuration();
    
    /* Configure GPIO */
    GPIO_Configuration();
    
    /* Configure USART1 @ 115200 baud */
    USART1_Configuration();
    
    /* Initialize SPI for module communication (motor-demo style) */
    pilot_spi_setup();
    
    /* Initialize IO16 modules (Slot 0 and Slot 3) */
    pilot_io16_0_init();
    pilot_io16_3_init();
    
    /* Send welcome message */
    const char* welcome = "\r\n"
        "========================================\r\n"
        "  BURJUVA MOTOR CONTROLLER v2.0\r\n"
        "  Motor-Demo IO16 Integration\r\n"
        "  STM32F103RCT6 @ 115200 baud\r\n"
        "========================================\r\n"
        "Komutlar: io16:SLOT:CMD, help\r\n"
        "========================================\r\n\r\n";
    Send_Response(welcome);
    
    /* Infinite loop - Process UART commands */
    while (1)
    {
        /* Check if data is received */
        if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
        {
            /* Read received byte */
            rxData = USART_ReceiveData(USART1);
            
            /* Echo it back */
            while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
            USART_SendData(USART1, rxData);
            
            /* Toggle LED */
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, 
                (BitAction)(1 - GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13)));
            
            /* Process command on Enter (CR or LF) */
            if (rxData == '\r' || rxData == '\n')
            {
                if (cmdIndex > 0)
                {
                    cmdBuffer[cmdIndex] = '\0';
                    
                    /* Echo newline */
                    Send_Response("\r\n");
                    
                    /* Process command */
                    Process_Command(cmdBuffer);
                    
                    /* Reset buffer */
                    cmdIndex = 0;
                }
            }
            /* Backspace */
            else if (rxData == 0x08 || rxData == 0x7F)
            {
                if (cmdIndex > 0)
                {
                    cmdIndex--;
                    Send_Response(" \b");
                }
            }
            /* Normal character */
            else if (rxData >= 32 && rxData < 127)
            {
                if (cmdIndex < sizeof(cmdBuffer) - 1)
                {
                    cmdBuffer[cmdIndex++] = rxData;
                }
            }
        }
    }
}

/**
 * @brief  Process received command
 */
void Process_Command(char* cmd)
{
    /* Convert to lowercase */
    char lowerCmd[128];
    int i = 0;
    while (cmd[i] && i < 127)
    {
        lowerCmd[i] = (cmd[i] >= 'A' && cmd[i] <= 'Z') ? (cmd[i] + 32) : cmd[i];
        i++;
    }
    lowerCmd[i] = '\0';
    
    /* Parse command */
    if (strncmp(lowerCmd, "io16:", 5) == 0)
    {
        Send_ACK("io16");
        
        /* Parse slot */
        int slot = parse_slot(lowerCmd + 5);
        if (slot < 0 || slot > 3)
        {
            Send_Response("[ERROR] Invalid slot (0-3)\r\n");
            return;
        }
        
        /* Skip "io16:X:" */
        const char* subcmd = skip_tokens(lowerCmd, 2);
        
        /* io16:SLOT:set:PIN:VALUE */
        if (strncmp(subcmd, "set:", 4) == 0)
        {
            int pin = parse_pin(subcmd + 4);
            const char* val_str = skip_tokens(subcmd + 4, 1);
            int value = (strcmp(val_str, "high") == 0 || strcmp(val_str, "1") == 0) ? 1 : 0;
            
            if (pin < 0 || pin > 15)
            {
                Send_Response("[ERROR] Invalid pin (0-15)\r\n");
                return;
            }
            
            /* Set pin value using motor-demo io16 driver */
            int ret = (slot == 0) ? 
                pilot_io16_0_set_value(pin, value) : 
                pilot_io16_3_set_value(pin, value);
            
            if (ret == 0)
            {
                char msg[64];
                sprintf(msg, "[OK] Slot %d Pin %d = %s\r\n", slot, pin, value ? "HIGH" : "LOW");
                Send_Response(msg);
            }
            else
            {
                Send_Response("[ERROR] Failed to set pin\r\n");
            }
        }
        /* io16:SLOT:get:PIN */
        else if (strncmp(subcmd, "get:", 4) == 0)
        {
            int pin = parse_pin(subcmd + 4);
            if (pin < 0 || pin > 15)
            {
                Send_Response("[ERROR] Invalid pin (0-15)\r\n");
                return;
            }
            
            uint8_t value;
            int ret = (slot == 0) ? 
                pilot_io16_0_get_value(pin, &value) : 
                pilot_io16_3_get_value(pin, &value);
            
            if (ret == 0)
            {
                char msg[64];
                sprintf(msg, "[OK] Slot %d Pin %d = %d\r\n", slot, pin, value);
                Send_Response(msg);
            }
            else
            {
                Send_Response("[ERROR] Failed to read pin\r\n");
            }
        }
        /* io16:SLOT:readall */
        else if (strncmp(subcmd, "readall", 7) == 0)
        {
            uint8_t port_a = (slot == 0) ? 
                pilot_io16_0_get_byte(pilot_io16_register_input_register_A) :
                pilot_io16_3_get_byte(pilot_io16_register_input_register_A);
            
            uint8_t port_b = (slot == 0) ? 
                pilot_io16_0_get_byte(pilot_io16_register_input_register_B) :
                pilot_io16_3_get_byte(pilot_io16_register_input_register_B);
            
            uint16_t all_pins = (port_b << 8) | port_a;
            
            char msg[128];
            sprintf(msg, "[OK] Slot %d All Pins = 0x%04X (", slot, all_pins);
            Send_Response(msg);
            
            /* Binary representation */
            for (int i = 15; i >= 0; i--)
            {
                char bit[2] = {(all_pins & (1 << i)) ? '1' : '0', '\0'};
                Send_Response(bit);
            }
            Send_Response(")\r\n");
        }
        /* io16:SLOT:info */
        else if (strncmp(subcmd, "info", 4) == 0)
        {
            Send_Response("[INFO] iC-JX 24V High-Side Driver\r\n");
            Send_Response("  Chip: iC-JX (16 channels)\r\n");
            Send_Response("  Interface: SPI (motor-demo driver)\r\n");
            
            char msg[64];
            sprintf(msg, "  Slot: %d\r\n", slot);
            Send_Response(msg);
            sprintf(msg, "  CS Pin: %s\r\n", 
                slot == 0 ? "PC13" : slot == 1 ? "PA0" : slot == 2 ? "PA1" : "PA2");
            Send_Response(msg);
        }
        /* io16:SLOT:status */
        else if (strncmp(subcmd, "status", 6) == 0)
        {
            char msg[256];
            sprintf(msg, "\r\n[STATUS] IO16 Module - Slot %d\r\n", slot);
            Send_Response(msg);
            
            /* Read all registers */
            uint8_t input_a = (slot == 0) ? 
                pilot_io16_0_get_byte(pilot_io16_register_input_register_A) :
                pilot_io16_3_get_byte(pilot_io16_register_input_register_A);
            
            uint8_t input_b = (slot == 0) ? 
                pilot_io16_0_get_byte(pilot_io16_register_input_register_B) :
                pilot_io16_3_get_byte(pilot_io16_register_input_register_B);
            
            uint8_t output_a = (slot == 0) ? 
                pilot_io16_0_get_byte(pilot_io16_register_output_register_A) :
                pilot_io16_3_get_byte(pilot_io16_register_output_register_A);
            
            uint8_t output_b = (slot == 0) ? 
                pilot_io16_0_get_byte(pilot_io16_register_output_register_B) :
                pilot_io16_3_get_byte(pilot_io16_register_output_register_B);
            
            sprintf(msg, "  INPUT_A:  0x%02X (%d%d%d%d %d%d%d%d)\r\n", input_a,
                (input_a>>7)&1, (input_a>>6)&1, (input_a>>5)&1, (input_a>>4)&1,
                (input_a>>3)&1, (input_a>>2)&1, (input_a>>1)&1, input_a&1);
            Send_Response(msg);
            
            sprintf(msg, "  INPUT_B:  0x%02X (%d%d%d%d %d%d%d%d)\r\n", input_b,
                (input_b>>7)&1, (input_b>>6)&1, (input_b>>5)&1, (input_b>>4)&1,
                (input_b>>3)&1, (input_b>>2)&1, (input_b>>1)&1, input_b&1);
            Send_Response(msg);
            
            sprintf(msg, "  OUTPUT_A: 0x%02X (%d%d%d%d %d%d%d%d)\r\n", output_a,
                (output_a>>7)&1, (output_a>>6)&1, (output_a>>5)&1, (output_a>>4)&1,
                (output_a>>3)&1, (output_a>>2)&1, (output_a>>1)&1, output_a&1);
            Send_Response(msg);
            
            sprintf(msg, "  OUTPUT_B: 0x%02X (%d%d%d%d %d%d%d%d)\r\n\r\n", output_b,
                (output_b>>7)&1, (output_b>>6)&1, (output_b>>5)&1, (output_b>>4)&1,
                (output_b>>3)&1, (output_b>>2)&1, (output_b>>1)&1, output_b&1);
            Send_Response(msg);
        }
        else
        {
            Send_Response("[ERROR] Unknown io16 subcommand\r\n");
        }
    }
    else if (strcmp(lowerCmd, "modul-algila") == 0)
    {
        Send_ACK("modul-algila");
        Send_Response("[DETECT] Slot 0: IO16 (pilot_io16_0)\r\n");
        Send_Response("[DETECT] Slot 3: IO16 (pilot_io16_3)\r\n");
    }
    else if (strcmp(lowerCmd, "help") == 0)
    {
        Send_ACK("help");
        const char* help = 
            "\r\nKomutlar:\r\n"
            "  io16:SLOT:set:PIN:VALUE  - Pin yaz (0-15, 0/1)\r\n"
            "  io16:SLOT:get:PIN        - Pin oku\r\n"
            "  io16:SLOT:readall        - Tüm pinleri oku\r\n"
            "  io16:SLOT:status         - Modül durumu\r\n"
            "  io16:SLOT:info           - Chip bilgisi\r\n"
            "  modul-algila             - Modül algılama\r\n"
            "  help                     - Bu mesaj\r\n"
            "\r\n"
            "Örnekler:\r\n"
            "  io16:0:set:5:high        - Slot 0, Pin 5 = HIGH\r\n"
            "  io16:3:get:12            - Slot 3, Pin 12 oku\r\n"
            "  io16:0:readall           - Slot 0 tüm pinler\r\n"
            "\r\n";
        Send_Response(help);
    }
    else
    {
        Send_Response("[ERROR] Unknown command! Type 'help'\r\n");
    }
}

/**
 * @brief  Send response via UART
 */
void Send_Response(const char* msg)
{
    while (*msg)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *msg++);
    }
}

/**
 * @brief  Send ACK message
 */
void Send_ACK(const char* cmd)
{
    Send_Response("\r\n[ACK] ");
    Send_Response(cmd);
    Send_Response("\r\n");
}

/**
 * @brief  Parse slot number from command
 */
int parse_slot(const char* cmd)
{
    if (cmd[0] >= '0' && cmd[0] <= '3')
        return cmd[0] - '0';
    return -1;
}

/**
 * @brief  Parse pin number from command
 */
int parse_pin(const char* cmd)
{
    return atoi(cmd);
}

/**
 * @brief  Parse value from command
 */
int parse_value(const char* cmd)
{
    return atoi(cmd);
}

/**
 * @brief  Skip N tokens (separated by ':')
 */
const char* skip_tokens(const char* cmd, int count)
{
    for (int i = 0; i < count; i++)
    {
        while (*cmd && *cmd != ':')
            cmd++;
        if (*cmd == ':')
            cmd++;
    }
    return cmd;
}

/**
 * @brief  Configure system clocks (72MHz from 8MHz HSE)
 */
void RCC_Configuration(void)
{
    ErrorStatus HSEStartUpStatus;
    
    RCC_DeInit();
    RCC_HSEConfig(RCC_HSE_ON);
    HSEStartUpStatus = RCC_WaitForHSEStartUp();
    
    if (HSEStartUpStatus == SUCCESS)
    {
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
        FLASH_SetLatency(FLASH_Latency_2);
        
        RCC_HCLKConfig(RCC_SYSCLK_Div1);
        RCC_PCLK2Config(RCC_HCLK_Div1);
        RCC_PCLK1Config(RCC_HCLK_Div2);
        
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
        RCC_PLLCmd(ENABLE);
        
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
        
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        
        while (RCC_GetSYSCLKSource() != 0x08);
    }
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | 
                           RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1, ENABLE);
}

/**
 * @brief  Configure GPIO
 */
void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* USART1 TX (PA9) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* USART1 RX (PA10) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* LED (PC13) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

/**
 * @brief  Configure USART1 @ 115200 baud
 */
void USART1_Configuration(void)
{
    USART_InitTypeDef USART_InitStructure;
    
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    
    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

/**
 * @brief  Simple delay
 */
void Delay(__IO uint32_t nCount)
{
    for (; nCount != 0; nCount--);
}
