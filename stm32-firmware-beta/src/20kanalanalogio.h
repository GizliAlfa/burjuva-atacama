/**
 * Burjuva Pilot - 20 Kanal Analog I/O Modül Kontrolü
 * AIO20 - 20 Channel Analog I/O Module
 */

#ifndef AIO20_H
#define AIO20_H

#include <stdint.h>

// Modül yönetimi
void AIO20_Register(uint8_t slot);

// Chip operasyonları
int AIO20_ChipInit(uint8_t slot);

// ADC/DAC operasyonları
int AIO20_ReadADC(uint8_t slot, uint8_t port);
int AIO20_WriteDAC(uint8_t slot, uint8_t port, uint16_t value);
int AIO20_ReadAllADC(uint8_t slot, uint16_t* values);

// Conversion helpers
uint16_t AIO20_ToVoltage(uint16_t value);
uint16_t AIO20_FromVoltage(uint16_t voltage_mv);

// AFE (Analog Front-End) kart algılama
void AIO20_DetectAFECards(uint8_t slot);

// Status ve bilgi
void AIO20_PrintStatus(uint8_t slot);
void AIO20_PrintInfo(uint8_t slot);

// Komut işleyici
void AIO20_HandleCommand(const char* cmd);

#endif // AIO20_H
