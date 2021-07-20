#include "TRX.h"
#include "config.h"

const struct _Bands Bands[BAND_COUNT] = {
  DEFINED_BANDS
};

TRX::TRX() {
  for (byte i=0; i < BAND_COUNT; i++) {
	  if (Bands[i].startSSB != 0)
	    BandData[i] = Bands[i].startSSB;
	  else
	    BandData[i] = Bands[i].start;
  }
  SwitchToBand(0);
  #ifdef HARDWARE_3_1
  VCC = 0;
  #endif
}

const struct _Bands& TRX::GetBandInfo(uint8_t idx)
{
  return Bands[idx];
}

void TRX::Freq2Bound()
{
  // проверяем выход за пределы диапазона
  if (Freq < Bands[BandIndex].start) Freq = Bands[BandIndex].start;
  else if (Freq > Bands[BandIndex].end) Freq = Bands[BandIndex].end;
}

void TRX::SwitchToBand(int band) {
  BandIndex = band;
  Freq = BandData[BandIndex];
  Freq2Bound();
  FreqMemo = 0;
  Lock = 0;
  Fast = 0;
  sideband = Bands[BandIndex].sideband;
}

void TRX::SelectBand(int band)
{
  BandData[BandIndex] = Freq;
  SwitchToBand(band);
}

void TRX::SaveFreqToMemo()
{
  FreqMemo = Freq;
}

void TRX::SwitchFreqToMemo()
{
  long tmp = Freq;
  Freq = FreqMemo;
  FreqMemo = tmp;
  Freq2Bound();
}

void TRX::ChangeFreq(long freq_delta)
{
  Freq += freq_delta;
  Freq2Bound();
}

void TRX::SetFreqBand(long freq)
{
  for (byte i=0; i < BAND_COUNT; i++) {
    if (freq >= Bands[i].start && freq <= Bands[i].end) {
      if (i != BandIndex) SwitchToBand(i);
      Freq = freq;
      return;
    }
  }
}

void TRX::NextBand()
{
  BandData[BandIndex] = Freq;
  if (++BandIndex >= BAND_COUNT)
    BandIndex = 0;
  Freq = BandData[BandIndex];
  Freq2Bound();
  FreqMemo = 0;
  Lock=0;
  sideband = Bands[BandIndex].sideband;
}

void TRX::SwitchAttPre()
{
  AttPre++;
  if (AttPre > 2) AttPre = 0;
}

struct TRXState {
  long BandData[BAND_COUNT]; 
  int BandIndex;
  long Freq;
};

uint16_t hash_data(uint16_t hval, uint8_t* data, int sz)
{
  while (sz--) {
    hval ^= *data++;
    hval = (hval << 11) | (hval >> 5);
  }
  return hval;
}  

uint16_t TRX::StateHash()
{
  uint16_t hash = 0x5AC3;
  hash = hash_data(hash,(uint8_t*)BandData,sizeof(BandData));
  hash = hash_data(hash,(uint8_t*)&BandIndex,sizeof(BandIndex));
  hash = hash_data(hash,(uint8_t*)&Freq,sizeof(Freq));
  return hash;
}

struct TRXState EEMEM eeState;
uint16_t EEMEM eeStateVer;
#define STATE_VER     (0x5B6E ^ (BAND_COUNT))

void TRX::StateSave()
{
  struct TRXState st;
  for (byte i=0; i < BAND_COUNT; i++)
    st.BandData[i] = BandData[i];
  st.BandIndex = BandIndex;
  st.Freq = Freq;
  eeprom_write_block(&st, &eeState, sizeof(st));
  eeprom_write_word(&eeStateVer, STATE_VER);
}

void TRX::StateLoad()
{
  struct TRXState st;
  uint16_t ver;
  ver = eeprom_read_word(&eeStateVer);
  if (ver == STATE_VER) {
    eeprom_read_block(&st, &eeState, sizeof(st));
    for (byte i=0; i < BAND_COUNT; i++)
      BandData[i] = st.BandData[i];
    SwitchToBand(st.BandIndex);
    Freq = st.Freq;
  }
}
