/**
 * Burjuva Pilot - 16 Kanal Dijital I/O Modül Kontrolü
 * IO16 - 16 Channel Digital I/O Module Header
 */

#ifndef IO16_DIGITAL_H
#define IO16_DIGITAL_H

#include <stdint.h>

// Modül kaydetme ve initialization
void IO16_Register(uint8_t slot);
int IO16_ChipInit(uint8_t slot);  // CRITICAL: Initialize IO678 chip (internal clock)

// Pin kontrolü
int IO16_SetPin(uint8_t slot, uint8_t pin, uint8_t state);
int IO16_GetPin(uint8_t slot, uint8_t pin);
int IO16_SetDirection(uint8_t slot, uint8_t pin, uint8_t direction);

// Toplu işlemler
uint16_t IO16_ReadAll(uint8_t slot);
int IO16_WriteAll(uint8_t slot, uint16_t state);

// Durum ve komut
void IO16_PrintStatus(uint8_t slot);
void IO16_HandleCommand(const char* cmd);

#endif // IO16_DIGITAL_H
