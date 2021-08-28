#ifndef CONFIG_H
#define CONFIG_H

#define LSB 0
#define USB 1
#define AM  2

extern const struct _Bands {
  const char *name;
  long  start, startSSB, end;
  uint8_t sideband;
} Bands[];

// число диапазонов
#define BAND_COUNT  24

#define DEFINED_BANDS \
  {"160m",  1810000L,  1840000L,  2000000L, LSB}, \
  {"80m",   3500000L,  3600000L,  3800000L, LSB}, \
  {"40m",   7000000L,  7045000L,  7200000L, LSB}, \
  {"30m",  10100000L,        0,  10150000L, USB}, \
  {"20m",  14000000L, 14100000L, 14350000L, USB}, \
  {"17m",  18068000L, 18110000L, 18168000L, USB}, \
  {"15m",  21000000L, 21150000L, 21450000L, USB}, \
  {"12m",  24890000L, 24930000L, 25140000L, USB}, \
  {"10m",  28000000L, 28200000L, 29700000L, USB}, \
  {"SW",    525000L, 0,  1705000L, AM}, \
  {"100m",  2800000L, 0,  3300000L, AM}, \
  {"49m",   5900000L, 0,  6200000L, AM}, \
  {"41m",   7200000L, 0,  7450000L, AM}, \
  {"31m",   9400000L, 0,  9900000L, AM}, \
  {"25m",  11600000L, 0, 12100000L, AM}, \
  {"22m",  13570000L, 0, 13870000L, AM}, \
  {"CB1",  26000000L, 0, 27000000L, AM}, \
  {"CB2",  27000000L, 27135000L, 28000000L, AM}, \
  {"GC1",  2000000L, 0,  5000000L, LSB}, \
  {"GC2",  5000000L, 0, 10000000L, LSB}, \
  {"GC3", 10000000L, 0, 15000000L, USB}, \
  {"GC4", 15000000L, 0, 20000000L, USB}, \
  {"GC5", 20000000L, 0, 25000000L, USB}, \
  {"GC6", 25000000L, 0, 30000000L, USB}

struct _Settings {
  int def_value;
  int min_value;
  int max_value;
  int step;
  char title[16];
};

#define SETTINGS_COUNT  24

#define SETTINGS_DATA \
  {8,   0,   60,  1, "PWR DWN DELAY"},      /* через сколько времени переходить в режим сохранения питания, сек. 0 - постоянно включен*/ \
  {7,   1,   15,  1, "BRIGHT HIGH"},        /* яркость LCD - 0..15 в обычнойм режиме и powerdown (0 - погашен) */ \
  {1,   0,   15,  1, "BRIGHT LOW"},         \
  \
  {0, -10000, 10000, 10, "LSB SHIFT"},      /* доп.сдвиг второго гетеродина относительно констант BFO_LSB/BFO_USB */ \
  {0, -10000, 10000, 10, "USB SHIFT"}, \
  {0, -20000, 20000, 5, "SI5351 XTAL"}, \
  \
  {0, 0, 0, 0, "CONFIRM RESET"}, \
  {0, 0, 0, 0, "CANCEL RESET"}, \
  \
  {   0, 0, 0, 0, "S1"}, \
  { 300, 0, 0, 0, "S3"}, \
  { 800, 0, 0, 0, "S5"}, \
  {1700, 0, 0, 0, "S7"}, \
  {2300, 0, 0, 0, "S9"}, \
  {2800, 0, 0, 0, "+20"}, \
  {2900, 0, 0, 0, "+40"}, \
  {3000, 0, 0, 0, "+60"}, \
  \
  {   0, 0, 0, 0, "S1"}, \
  { 300, 0, 0, 0, "S3"}, \
  { 800, 0, 0, 0, "S5"}, \
  {1200, 0, 0, 0, "S7"}, \
  {1500, 0, 0, 0, "S9"}, \
  {1800, 0, 0, 0, "+20"}, \
  {2400, 0, 0, 0, "+40"}, \
  {2700, 0, 0, 0, "+60"}

// increase for reset stored to EEPROM settings values to default
#define SETTINGS_VERSION    0x01

// id for fast settings access
enum {
  ID_POWER_DOWN_DELAY = 0,
  ID_DISPLAY_BRIGHT_HIGH,
  ID_DISPLAY_BRIGHT_LOW,
  ID_LSB_SHIFT,
  ID_USB_SHIFT,
  ID_SI5351_XTAL,
  ID_FULL_RESET_CONFIRM,
  ID_FULL_RESET_CANCEL,
  ID_SMETER,
  ID_CLOCK = 200,
  ID_SPLIT
};

//  Конфиг для RX2121AM
//  Первая ПЧ 21.7МГц, вторая - 500кГц/465кГц

#define IF1FREQ 21700000L

// Частоты 2го гетеродина для ЭМФ на нижнюю боковую. Для верхней боковой поменять знаки местами
#define BFO_LSB   IF1FREQ-500000L
#define BFO_USB   IF1FREQ+500000L

#define CLK0_RX_SSB     FREQ+IF1FREQ
#define CLK1_RX_SSB     BFO
#define CLK2_RX_SSB     500000L

#define CLK0_RX_AM      FREQ+IF1FREQ
#define CLK1_RX_AM      IF1FREQ+465000L
#define CLK2_RX_AM      0

// конфиг "железа"
#include "config_hw.h"

#endif
