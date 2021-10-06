//
// UR5FFR Si5351 NanoVFO for RX2121AM receiver
// v1.0 from 20.07.2020
// Copyright (c) Andrey Belokon, UR5FFR, 2017-2021 Odessa
// https://github.com/andrey-belokon/
// http://www.ur5ffr.com
// GNU GPL license
//

#include <avr/power.h>
#include <avr/eeprom.h> 

// !!! all user setting defined in config.h, config_hw.h and config_sw.h files !!!
#include "config.h"

#include "pins.h"
#include "utils.h"
#include <i2c.h>
#include "TRX.h"
#include "Encoder.h"

#ifdef VFO_SI5351
  #include <si5351a.h>
#endif
#ifdef VFO_SI570  
  #include <Si570.h>
#endif

#include "disp_OLED128x64.h"

#ifdef RTC_ENABLE
  #include "RTC.h"
#endif

#ifdef VFO_SI5351
  Si5351 vfo5351;
#endif
#ifdef VFO_SI570  
  Si570 vfo570;
#endif

Encoder encoder;
TRX trx;
Display_OLED128x64 disp;

InputPullUpPin inPTT(PIN_IN_PTT);
InputAnalogPin inSMeter(PIN_SMETER);

#ifdef HARDWARE_3_1
InputAnalogPin inCWDec(PIN_CWDEC);
InputAnalogPin inPower(PIN_POWER);
InputAnalogPin inSWRF(PIN_SWR_F);
InputAnalogPin inSWRR(PIN_SWR_R);
#endif

/*  button hardcode place
    [3]  [1]
                [5]
    [4]  [2]
*/
int16_t KeyLevels[] = {130, 320, 525, 745, 940};
InputAnalogKeypad keypad(PIN_ANALOG_KEYPAD, 6, KeyLevels);

const struct _Settings SettingsDef[SETTINGS_COUNT] PROGMEM = {
  SETTINGS_DATA
};

uint16_t EEMEM eeSettingsVersion = 0;
int EEMEM eeSettings[SETTINGS_COUNT] = {0};  

int Settings[SETTINGS_COUNT] = {0};  

void writeSettings()
{
  eeprom_write_block(Settings, eeSettings, sizeof(Settings));
}

void resetSettings()
{
  for (uint8_t j=0; j < SETTINGS_COUNT; j++)
    Settings[j] = (int)pgm_read_word(&SettingsDef[j].def_value);
}

void readSettings()
{
  uint16_t ver;
  ver = eeprom_read_word(&eeSettingsVersion);
  if (ver == ((SETTINGS_VERSION << 8) | SETTINGS_COUNT))
    eeprom_read_block(Settings, eeSettings, sizeof(Settings));
  else {
    // fill with defaults
    resetSettings();
    writeSettings();
    ver = (SETTINGS_VERSION << 8) | SETTINGS_COUNT;
    eeprom_write_word(&eeSettingsVersion, ver);
  }
}

#ifdef HARDWARE_3_1
void sendBandData(byte data)
{
  digitalWrite(PIN_SR_LATCH, LOW);
  shiftOut(PIN_SR_DATA, PIN_SR_SHIFT, MSBFIRST, data); 
  digitalWrite(PIN_SR_LATCH, HIGH);
}
#endif

void setup()
{
  Serial.begin(CAT_BAUND_RATE);
  i2c_init();
  readSettings();
  inPTT.setup();
  inSMeter.setup();
  keypad.setup();
  pinMode(PIN_OUT_TONE, OUTPUT);
  pinMode(PIN_OUT_TX, OUTPUT);
  digitalWrite(PIN_OUT_TX, !OUT_TX_ACTIVE_LEVEL);
  pinMode(PIN_OUT_KEY, OUTPUT); 
  digitalWrite(PIN_OUT_KEY, !OUT_KEY_ACTIVE_LEVEL);
  pinMode(PIN_IN_DIT, INPUT_PULLUP);
  pinMode(PIN_IN_DAH, INPUT_PULLUP);
  pinMode(PIN_OUT_USR, OUTPUT);
  pinMode(PIN_SR_DATA, OUTPUT);
  pinMode(PIN_SR_SHIFT, OUTPUT);
  pinMode(PIN_SR_LATCH, OUTPUT);
  sendBandData(0);
  inCWDec.setup();
  inPower.setup();
  inSWRF.setup();
  inSWRR.setup();
#ifdef VFO_SI5351
  vfo5351.VCOFreq_Max = 800000000; // для использования "кривых" SI-шек с нестабильной генерацией
  // change for required output level
  vfo5351.setup(
    SI5351_CLK0_DRIVE,
    SI5351_CLK1_DRIVE,
    SI5351_CLK2_DRIVE
  );
  vfo5351.set_xtal_freq((SI5351_CALIBRATION/10000)*10000+Settings[ID_SI5351_XTAL]);
#endif  
#ifdef VFO_SI570  
  vfo570.setup(SI570_CALIBRATION);
#endif  
  encoder.Setup();
  trx.StateLoad();
  disp.setup();
}

#include "freq_calc.h"

void UpdateBandCtrl()
{
  static byte last_data = 0xFF;
  byte data;

  data = 0;

  if (trx.AttPre == 1) data |= B010;
  else if (trx.AttPre == 2) data |= B001;
  if (trx.sideband == AM) data |= B100;
  
  if (trx.Freq < 5100000)         data |= B0001000;
  else if (trx.Freq < 9350000)    data |= B0010000;
  else if (trx.Freq < 19000000)   data |= B0100000;
  else                            data |= B1000000;

  if (data != last_data) {
    sendBandData(data);
    last_data = data;
  }
}

long last_event = 0;
long last_pool = 0;
uint8_t in_power_save = 0;
uint8_t last_key = 0;
long last_key_press = 0;

uint8_t keyb_key = 0;
uint8_t keyb_long = 0;

// fill keyb_key & keyb_long
void PoolKeyboard()
{
  uint8_t key = keypad.Read();
  keyb_key = 0;
  if (keyb_long && key) return; // wait unpress
  keyb_long = 0;
  if (!last_key && key) {
    // KEY DOWN
    last_key = key;
    last_key_press = millis();
  } else if (last_key && key) {
    // KEY 
    if (millis()-last_key_press > LONG_PRESS_DELAY) {
      keyb_key = last_key;
      keyb_long = 1;
      last_key = 0;
      last_key_press = 0;
    }
  } else if (last_key && !key) {
    // KEY UP
    keyb_key = last_key;
    keyb_long = 0;
    last_key = 0;
    last_key_press = 0;
  }
}

void power_save(uint8_t enable)
{
  if (enable) {
    if (!in_power_save) {
      disp.setBright(Settings[ID_DISPLAY_BRIGHT_LOW]);
      in_power_save = 1;
    }
  } else {
    if (in_power_save) {
      disp.setBright(Settings[ID_DISPLAY_BRIGHT_HIGH]);
      in_power_save = 0;
    }
    last_event = millis();
  }
}

#include "menu.h"
#include "cat.h"

void loop()
{
  if (!trx.Lock) {
    long delta = encoder.GetDelta();
    if (delta) {
      if (trx.ChannelMode) {
        delta /= 30;
        if (delta > 0) trx.SwitchFreqToNextChannel(true);
        else if (delta < 0) trx.SwitchFreqToNextChannel(false);
        if (delta != 0) {
          disp.Draw(trx);
          delay(300);
          encoder.GetDelta();
        }
      } else {
        //if (last_key == 3 || (keyb_long && keypad.Read() == 3)) delta *= 10;
        if (trx.Fast) {
          delta *= 20;
          if (delta < -20000) delta = -20000;
          if (delta > 20000)  delta = 20000;
        } else if (trx.sideband == AM) delta *= 2;
        trx.ChangeFreq(delta);
      }
      power_save(0);
    }
  }

  if (Serial.available() > 0) 
    ExecCAT();

  if (millis()-last_pool > POOL_INTERVAL) {
    PoolKeyboard();
    if (keyb_key != 0) power_save(0);
    switch (keyb_key)
    {
        /*  button hardcode place
            [3]  [1]
                        [5]
            [4]  [2] 
        */
      case 1:
        if (keyb_long) {
          if (trx.ChannelMode) {
            trx.DeleteFreqFromChannel(trx.ChannelIndex);
            trx.SwitchFreqToNextChannel(true);
          } else {
            int ch = select_channel(0);
            if (ch >= 0) trx.SaveFreqToChannel(ch);
            keypad.waitUnpress();
            disp.clear();
            power_save(0);
          }
        } else {
          trx.SwitchChannelMode();
        }
        break;
      case 2:
        if (keyb_long) {
          if (++trx.sideband > AM) trx.sideband = 0;
        } else {
            trx.SwitchAttPre();
        }
        break;
      case 3:
        if (keyb_long) {
          show_menu();
          keypad.waitUnpress();
          disp.clear();
          power_save(0);
        } else {
          trx.ChannelMode = false;
          select_band();
          keypad.waitUnpress();
          disp.clear();
          power_save(0);
        }
        break;
      case 4:
        if (keyb_long) trx.Lock = !trx.Lock;
        else if (not trx.ChannelMode) trx.Fast = !trx.Fast;
        else {
          int ch = select_channel(trx.ChannelIndex);
          if (ch >= 0) trx.SwitchFreqToChannel(ch);
          keypad.waitUnpress();
          disp.clear();
          power_save(0);
        }
        break;
      case 5:
        break;
    }
  
    // read and convert smeter
    int val = inSMeter.Read();
    byte smidx = ID_SMETER;
    if (trx.sideband == AM) smidx += 8;
    bool rev_order = Settings[smidx+1] > Settings[smidx+4];
    for (int8_t i=14; i >= 0; i--) {
      int8_t ii = i>>1;
      int treshold;
      if (i&1) treshold = (Settings[smidx+ii]+Settings[smidx+ii+1]) >> 1;
      else treshold = Settings[smidx+ii];
      if ((!rev_order && val >= treshold) || (rev_order && val <= treshold)) {
        trx.SMeter = i+1;
        break;
      }
    }

    const long alpha = 75;
    long newval = (long)inPower.Read()*58/10; // в мв
    trx.VCC = (trx.VCC*alpha + newval*(100-alpha))/100;
    newval = inSWRR.Read(); // в мв
    trx.VBAT = (trx.VBAT*alpha + newval*(100-alpha))/100;

    UpdateFreq();
    UpdateBandCtrl();
    disp.Draw(trx);
    
    if (Settings[ID_POWER_DOWN_DELAY] > 0 && millis()-last_event > Settings[ID_POWER_DOWN_DELAY]*1000)
      power_save(1);
    last_pool = millis();
  }

  static long state_poll_tm = 0;
  if (millis()-state_poll_tm > 500) {
    static uint16_t state_hash = 0;
    static uint8_t state_changed = false;
    static long state_changed_tm = 0;
    uint16_t new_state_hash = trx.StateHash();
    if (new_state_hash != state_hash) {
      state_hash = new_state_hash;
      state_changed = true;
      state_changed_tm = millis();
    } else if (state_changed && (millis()-state_changed_tm > 5000)) {
      // save state
      trx.StateSave();
      state_changed = false;
    }
    state_poll_tm = millis();
  }
}
