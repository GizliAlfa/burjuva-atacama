/**
 * Burjuva Pilot - UART Helper Functions
 * Shared UART utility functions for all modules
 */

#ifndef UART_HELPER_H
#define UART_HELPER_H

#include <stdint.h>

// UART send functions
void UART_SendString(const char* str);
void UART_SendHex8(uint8_t data);
void UART_SendHex16(uint16_t data);

#endif // UART_HELPER_H
