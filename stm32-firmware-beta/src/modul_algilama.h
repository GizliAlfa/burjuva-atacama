/**
 * Burjuva Motor Controller - Module Detection Header
 * Date: 17 Kasım 2025
 * 
 * REAL 1-Wire module detection based on module_detection_working.c
 */

#ifndef MODUL_ALGILAMA_H
#define MODUL_ALGILAMA_H

#include "stm32f10x.h"

/**
 * Initialize module detection system (DWT + GPIO)
 * Call once at startup
 */
void Modul_Init(void);

/**
 * Process "modul-algila" command
 * Scans all 4 slots and reports results
 */
void Modul_Komut_Isle(void);

#endif // MODUL_ALGILAMA_H
