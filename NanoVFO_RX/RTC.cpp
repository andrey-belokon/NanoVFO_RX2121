#include "config_hw.h"
#include "RTC.h"
#include <i2c.h>

#ifdef RTC_ENABLE

#define RTC_I2C_ADDRESS 0x68
#define REG_ADDR        0

void RTC_Write(RTCData* data)
{
  byte *p=(byte *)data;
  if (i2c_begin_write(RTC_I2C_ADDRESS)) {
    i2c_write(REG_ADDR);
    for (byte i=7; i > 0; i--) i2c_write(*p++);
    i2c_end();
  }
}

void RTC_Read(RTCData* data)
{
  if (i2c_begin_write(RTC_I2C_ADDRESS)) {
    i2c_write(REG_ADDR);
    i2c_begin_read(RTC_I2C_ADDRESS);
    i2c_read((uint8_t*)data,sizeof(RTCData));
    i2c_end();
    //uint8_t *p = (uint8_t*)data;
    //for (byte i=0; i < 7; i++) *p++ &= RTC_BITMASK[i];
  }
}

#else

void RTC_Write(RTCData* data)
{
}

void RTC_Read(RTCData* data)
{
}

#endif
