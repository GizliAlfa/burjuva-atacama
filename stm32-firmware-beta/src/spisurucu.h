/**
 * Burjuva Pilot - SPI Sürücüsü
 * SPI Driver for Module Communication
 * 
 * Hardware: STM32F103RCT6
 * SPI1: PA5(SCK), PA6(MISO), PA7(MOSI)
 * 
 * Chip Select Pins (per slot):
 * - Slot 0: PB0
 * - Slot 1: PB1
 * - Slot 2: PB10
 * - Slot 3: PB11
 */

#ifndef SPISURUCU_H
#define SPISURUCU_H

#include <stdint.h>

// SPI ID'leri (4 slot)
typedef enum {
    SPI_SLOT_0 = 0,
    SPI_SLOT_1 = 1,
    SPI_SLOT_2 = 2,
    SPI_SLOT_3 = 3,
    SPI_SLOT_INVALID = -1
} spi_slot_t;

// Chip Select durumu
typedef enum {
    CS_ENABLE = 0,   // CS aktif (LOW)
    CS_DISABLE = 1   // CS pasif (HIGH)
} chip_select_t;

/**
 * SPI sistemini başlat
 * SPI1 ve GPIO pinlerini yapılandırır
 */
void SPI_Module_Init(void);

/**
 * Chip Select kontrolü
 * @param slot: Slot numarası (0-3)
 * @param cs: CS_ENABLE veya CS_DISABLE
 * @return 0: başarılı, -1: hata
 */
int SPI_SetCS(spi_slot_t slot, chip_select_t cs);

/**
 * SPI veri gönder
 * @param slot: Slot numarası (0-3)
 * @param data: Gönderilecek byte
 */
void SPI_Send(spi_slot_t slot, uint8_t data);

/**
 * SPI veri alışverişi
 * @param slot: Slot numarası (0-3)
 * @param mosi: Gönderilecek byte
 * @return Alınan byte (MISO)
 */
uint8_t SPI_DataExchange(spi_slot_t slot, uint8_t mosi);

/**
 * SPI transfer (çoklu byte)
 * @param slot: Slot numarası
 * @param tx_data: Gönderilecek veri buffer'ı
 * @param rx_data: Alınacak veri buffer'ı (NULL olabilir)
 * @param length: Transfer uzunluğu
 * @return 0: başarılı, -1: hata
 */
int SPI_Transfer(spi_slot_t slot, const uint8_t* tx_data, uint8_t* rx_data, uint16_t length);

#endif // SPISURUCU_H
