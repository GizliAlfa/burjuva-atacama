/**
 ******************************************************************************
 * @file    main.c
 * @brief   STM32F103RCT6 UART Command Firmware (SPL)
 * @date    17 Kasım 2025
 * 
 * @description
 * UART command processing firmware using Standard Peripheral Library (SPL)
 * - USART1: PA9 (TX), PA10 (RX)  
 * - Baud: 115200, 8N1, No flow control
 * - Echoes back any received data
 * - Komutlar:
 *   - "modul-algila" -> Modül algılama sistemi
 * 
 * Pin Configuration:
 * - PA9  (USART1_TX) -> RPi GPIO15 (RXD)
 * - PA10 (USART1_RX) -> RPi GPIO14 (TXD)
 ******************************************************************************
 */

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_flash.h"
#include "modul_algilama.h"
#include "16kanaldijital.h"
#include "20kanalanalogio.h"
#include "fpga.h"
#include "spisurucu.h"
#include <string.h>

/* Private function prototypes */
void RCC_Configuration(void);
void GPIO_Configuration(void);
void USART1_Configuration(void);
void Delay(__IO uint32_t nCount);
void Process_Command(char* cmd);

/**
 * @brief  Main program
 */
int main(void)
{
    uint16_t rxData;
    static char cmdBuffer[64];
    static uint8_t cmdIndex = 0;
    
    /* Configure clocks */
    RCC_Configuration();
    
    /* Configure GPIO */
    GPIO_Configuration();
    
    /* Configure USART1 */
    USART1_Configuration();
    
    /* Initialize SPI for module communication */
    SPI_Module_Init();
    
    /* Initialize Module Detection System */
    Modul_Init();
    
    /* Send welcome message */
    const char* welcome = "\r\n========================================\r\n"
                         "  BURJUVA MOTOR CONTROLLER v1.0\r\n"
                         "  STM32F103RCT6 - UART Command System\r\n"
                         "========================================\r\n"
                         "Komutlar:\r\n"
                         "  modul-algila  -> Modul algilama\r\n"
                         "========================================\r\n\r\n";
    while (*welcome)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *welcome++);
    }
    
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
            
            /* Toggle LED to show activity */
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, 
                (BitAction)(1 - GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13)));
            
            /* Process command on Enter (CR or LF) */
            if (rxData == '\r' || rxData == '\n')
            {
                if (cmdIndex > 0)
                {
                    cmdBuffer[cmdIndex] = '\0';
                    
                    /* Echo newline */
                    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
                    USART_SendData(USART1, '\r');
                    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
                    USART_SendData(USART1, '\n');
                    
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
                    /* Echo backspace sequence: BS + SPACE + BS */
                    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
                    USART_SendData(USART1, ' ');
                    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
                    USART_SendData(USART1, 0x08);
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
 * @brief  Send ACK message
 */
void Send_ACK(const char* cmd)
{
    const char* ack_prefix = "\r\n[ACK] Komut alindi: ";
    const char* ack_suffix = "\r\n";
    
    /* Send ACK prefix */
    while (*ack_prefix)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *ack_prefix++);
    }
    
    /* Send command name */
    while (*cmd)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *cmd++);
    }
    
    /* Send suffix */
    while (*ack_suffix)
    {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *ack_suffix++);
    }
}

/**
 * @brief  Process received command
 */
void Process_Command(char* cmd)
{
    /* Convert to lowercase for case-insensitive comparison */
    char lowerCmd[64];
    int i = 0;
    while (cmd[i] && i < 63)
    {
        lowerCmd[i] = (cmd[i] >= 'A' && cmd[i] <= 'Z') ? (cmd[i] + 32) : cmd[i];
        i++;
    }
    lowerCmd[i] = '\0';
    
    /* Process commands */
    if (strcmp(lowerCmd, "modul-algila") == 0)
    {
        Send_ACK("modul-algila");
        Modul_Komut_Isle();
    }
    else if (strncmp(lowerCmd, "io16:", 5) == 0)
    {
        Send_ACK("io16");
        IO16_HandleCommand(lowerCmd + 5);  // "io16:" sonrasını gönder
    }
    else if (strncmp(lowerCmd, "aio20:", 6) == 0)
    {
        Send_ACK("aio20");
        AIO20_HandleCommand(lowerCmd + 6);  // "aio20:" sonrasını gönder
    }
    else if (strncmp(lowerCmd, "fpga:", 5) == 0)
    {
        Send_ACK("fpga");
        FPGA_HandleCommand(lowerCmd + 5);  // "fpga:" sonrasını gönder
    }
    else if (strcmp(lowerCmd, "help") == 0 || strcmp(lowerCmd, "yardim") == 0)
    {
        Send_ACK("help");
        const char* help = "\r\nMevcut Komutlar:\r\n"
                          "  modul-algila              -> Bagli modulleri tara\r\n"
                          "  io16:SLOT:KOMUT           -> IO16 modul kontrolu\r\n"
                          "  aio20:SLOT:KOMUT          -> AIO20 modul kontrolu\r\n"
                          "  fpga:SLOT:KOMUT           -> FPGA modul kontrolu\r\n"
                          "  help                      -> Bu yardim mesaji\r\n"
                          "\r\n"
                          "Ornek:\r\n"
                          "  io16:0:set:5:high         -> Slot 0, Pin 5 = HIGH\r\n"
                          "  aio20:1:readin:3          -> Slot 1, AI3 oku\r\n"
                          "  fpga:2:status             -> Slot 2 durumu\r\n"
                          "\r\n";
        while (*help)
        {
            while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
            USART_SendData(USART1, *help++);
        }
    }
    else
    {
        const char* unknown = "\r\nBilinmeyen komut! 'help' yazin.\r\n\r\n";
        while (*unknown)
        {
            while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
            USART_SendData(USART1, *unknown++);
        }
    }
}

/**
 * @brief  Configure system clocks  
 *         Using HSE (8MHz external crystal) + PLL to 72MHz
 *         EXACTLY MATCHING module_detection_working.c configuration
 */
void RCC_Configuration(void)
{
    ErrorStatus HSEStartUpStatus;
    
    /* RCC system reset (to default configuration) */
    RCC_DeInit();
    
    /* Enable HSE (8MHz external crystal) */
    RCC_HSEConfig(RCC_HSE_ON);
    
    /* Wait till HSE is ready */
    HSEStartUpStatus = RCC_WaitForHSEStartUp();
    
    if (HSEStartUpStatus == SUCCESS)
    {
        /* Enable Prefetch Buffer */
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
        
        /* Flash 2 wait state (required for 72MHz operation) */
        FLASH_SetLatency(FLASH_Latency_2);
        
        /* HCLK = SYSCLK (72MHz) */
        RCC_HCLKConfig(RCC_SYSCLK_Div1);
        
        /* PCLK2 = HCLK (72MHz) */
        RCC_PCLK2Config(RCC_HCLK_Div1);
        
        /* PCLK1 = HCLK/2 (36MHz - max for APB1) */
        RCC_PCLK1Config(RCC_HCLK_Div2);
        
        /* Configure PLL: PLLCLK = HSE * 9 = 8MHz * 9 = 72MHz */
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
        
        /* Enable PLL */
        RCC_PLLCmd(ENABLE);
        
        /* Wait till PLL is ready */
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
        
        /* Select PLL as system clock source */
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        
        /* Wait till PLL is used as system clock source */
        while (RCC_GetSYSCLKSource() != 0x08);
    }
    else
    {
        /* HSE failed - stay on HSI (8MHz) as fallback */
        /* This shouldn't happen with proper hardware */
    }
    
    /* Enable peripheral clocks */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | 
                           RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1, ENABLE);
}

/**
 * @brief  Configure GPIO
 */
void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* Configure PA9 (USART1 TX) as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* Configure PA10 (USART1 RX) as input floating */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /* Configure PC13 (LED) as output push-pull */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    /* Turn LED off (PC13 is active low on Blue Pill) */
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

/**
 * @brief  Configure USART1
 */
void USART1_Configuration(void)
{
    USART_InitTypeDef USART_InitStructure;
    
    /* USART1 configuration:
     * - BaudRate = 115200
     * - Word Length = 8 Bits
     * - One Stop Bit
     * - No parity
     * - Hardware flow control disabled
     * - Receive and transmit enabled
     */
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    
    /* Configure USART1 */
    USART_Init(USART1, &USART_InitStructure);
    
    /* Enable USART1 */
    USART_Cmd(USART1, ENABLE);
}

/**
 * @brief  Simple delay function
 */
void Delay(__IO uint32_t nCount)
{
    for (; nCount != 0; nCount--);
}
