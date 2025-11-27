/**
 * Burjuva Pilot - UART Helper Functions Implementation
 */

#include "uart_helper.h"
#include "stm32f10x.h"
#include "stm32f10x_usart.h"

/**
 * UART üzerinden string gönder
 */
void UART_SendString(const char* str) {
    while (*str) {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *str++);
    }
}

/**
 * UART üzerinden 8-bit hex gönder
 */
void UART_SendHex8(uint8_t data) {
    char hex[3];
    const char hex_chars[] = "0123456789ABCDEF";
    
    hex[0] = hex_chars[(data >> 4) & 0x0F];
    hex[1] = hex_chars[data & 0x0F];
    hex[2] = '\0';
    
    UART_SendString(hex);
}

/**
 * UART üzerinden 16-bit hex gönder
 */
void UART_SendHex16(uint16_t data) {
    UART_SendHex8((data >> 8) & 0xFF);
    UART_SendHex8(data & 0xFF);
}
