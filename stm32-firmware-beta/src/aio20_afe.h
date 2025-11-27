/**
 * Burjuva Pilot - AIO20 AFE (Analog Front-End) Card Types
 * 
 * MAX11300 modülünün üzerine 4 adet AFE kartı takılır.
 * Her kart 4-5 kanalı kontrol eder ve tipini otomatik algılar.
 * 
 * Kart Tipleri:
 * - 0-10V: Voltage input/output
 * - 4-20mA: Current loop
 * - PT-1000: RTD temperature sensor
 */

#ifndef AIO20_AFE_H
#define AIO20_AFE_H

#include <stdint.h>

// AFE Kart Tipleri
typedef enum {
    AFE_TYPE_NONE = 0,       // Kart takılı değil
    AFE_TYPE_0_10V = 1,      // 0-10V voltage kartı
    AFE_TYPE_4_20MA = 2,     // 4-20mA current loop kartı
    AFE_TYPE_PT1000 = 3,     // PT-1000 RTD kartı
    AFE_TYPE_UNKNOWN = 0xFF  // Tanınmayan değer
} AIO20_AFE_Type;

// AFE Kart Pozisyonları (her biri 4 kanal kontrol eder)
typedef enum {
    AFE_CARD_0 = 0,    // Kanal 0-3
    AFE_CARD_1 = 1,    // Kanal 4-7
    AFE_CARD_2 = 2,    // Kanal 8-11
    AFE_CARD_3 = 3,    // Kanal 12-15
    AFE_CARD_COUNT = 4
} AIO20_AFE_Card;

// AFE Algılama Değerleri (ADC raw değeri)
#define AFE_DETECT_4_20MA_MIN       4000    // 4-20mA: > 4000
#define AFE_DETECT_0_10V_MIN        980     // 0-10V: 980-1180
#define AFE_DETECT_0_10V_MAX        1180
#define AFE_DETECT_PT1000_MIN       2060    // PT-1000: 2060-2260
#define AFE_DETECT_PT1000_MAX       2260

// Kanal grupları (her AFE kartı 4 kanal kontrol eder)
#define AFE_CHANNELS_PER_CARD       4

/**
 * AFE kartını ADC değerine göre tanımla
 * @param adc_value: Algılama kanalından okunan ADC değeri
 * @return: AFE kart tipi
 */
static inline AIO20_AFE_Type AIO20_DetectAFE(uint16_t adc_value) {
    if (adc_value > AFE_DETECT_4_20MA_MIN) {
        return AFE_TYPE_4_20MA;
    } else if (adc_value > AFE_DETECT_0_10V_MIN && adc_value < AFE_DETECT_0_10V_MAX) {
        return AFE_TYPE_0_10V;
    } else if (adc_value > AFE_DETECT_PT1000_MIN && adc_value < AFE_DETECT_PT1000_MAX) {
        return AFE_TYPE_PT1000;
    } else {
        return AFE_TYPE_NONE;
    }
}

/**
 * AFE tipi string'e çevir
 */
static inline const char* AIO20_AFE_ToString(AIO20_AFE_Type type) {
    switch (type) {
        case AFE_TYPE_0_10V:   return "0-10V";
        case AFE_TYPE_4_20MA:  return "4-20mA";
        case AFE_TYPE_PT1000:  return "PT-1000";
        case AFE_TYPE_NONE:    return "none";
        default:               return "unknown";
    }
}

/**
 * AFE kartına göre kanal aralığını al
 * @param card: AFE kart numarası (0-3)
 * @param start_ch: Başlangıç kanalı (çıkış)
 * @param end_ch: Bitiş kanalı (çıkış)
 */
static inline void AIO20_AFE_GetChannelRange(AIO20_AFE_Card card, uint8_t* start_ch, uint8_t* end_ch) {
    if (start_ch) *start_ch = card * AFE_CHANNELS_PER_CARD;
    if (end_ch) *end_ch = (card * AFE_CHANNELS_PER_CARD) + AFE_CHANNELS_PER_CARD - 1;
}

#endif // AIO20_AFE_H
