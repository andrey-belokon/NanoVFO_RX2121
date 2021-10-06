#include "TRX.h"
#include "config.h"

const struct _Bands Bands[BAND_COUNT] = {
  DEFINED_BANDS
};

TRX::TRX() {
  InitEEMemo();
  for (byte i=0; i < BAND_COUNT; i++) {
	  if (Bands[i].startSSB != 0)
	    BandData[i] = Bands[i].startSSB;
	  else
	    BandData[i] = Bands[i].start;
  }
  SwitchToBand(0);
  VCC = VBAT = 0;
  ChannelMode = false;
  ChannelIndex = 0;
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
  Lock = 0;
  Fast = 0;
  sideband = Bands[BandIndex].sideband;
}

void TRX::SelectBand(int band)
{
  BandData[BandIndex] = Freq;
  SwitchToBand(band);
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

struct MemoInfo EEMEM eeMemoInfo[CHANNEL_COUNT];
uint16_t EEMEM eeMemoInfoVer;
#define MEMO_VER    0x7A61

void TRX::InitEEMemo()
{
  if (eeprom_read_word(&eeMemoInfoVer) != MEMO_VER) {
    struct MemoInfo mi;
    mi.Freq = 0;
    for (byte j=0; j < CHANNEL_COUNT; j++)
      eeprom_write_block(&mi, eeMemoInfo+j, sizeof(mi));
    eeprom_write_word(&eeMemoInfoVer, MEMO_VER);
  }
}

void TRX::SwitchChannelMode()
{
  if (ChannelMode) 
    ChannelMode = false;
  else {
    struct MemoInfo mi;
    eeprom_read_block(&mi, eeMemoInfo+ChannelIndex, sizeof(mi));
    ChannelMode = true;
    if (mi.Freq == 0) 
      SwitchFreqToNextChannel(true);
    else {
      Freq = mi.Freq;
      BandIndex = mi.band;
      sideband = mi.sideband;
      Lock = Fast = 0;
    }
  }
}

void TRX::SaveFreqToChannel(uint8_t idx)
{
  struct MemoInfo mi;
  mi.Freq = Freq;
  mi.band = BandIndex;
  mi.sideband = sideband;
  eeprom_write_block(&mi, eeMemoInfo+idx, sizeof(mi));
}

void TRX::SwitchFreqToChannel(uint8_t idx)
{
  struct MemoInfo mi;
  eeprom_read_block(&mi, eeMemoInfo+idx, sizeof(mi));
  if (mi.Freq != 0) {
    ChannelIndex = idx;
    Freq = mi.Freq;
    BandIndex = mi.band;
    sideband = mi.sideband;
    Lock = Fast = 0;
    return;
  }
}

void TRX::SwitchFreqToNextChannel(bool up)
{
  struct MemoInfo mi;
  uint8_t i;
  if (up) {
    // find next non-zero memo freq
    i = ChannelIndex+1;
    if (i >= CHANNEL_COUNT) i=0;
  } else 
    i = (ChannelIndex > 0 ? ChannelIndex-1 : CHANNEL_COUNT-1);
  while (i != ChannelIndex) {
    eeprom_read_block(&mi, eeMemoInfo+i, sizeof(mi));
    if (mi.Freq != 0) {
      ChannelIndex = i;
      Freq = mi.Freq;
      BandIndex = mi.band;
      sideband = mi.sideband;
      Lock = Fast = 0;
      return;
    }
    if (up) {
      i++;
      if (i >= CHANNEL_COUNT) i=0;
    } else 
      i = (i > 0 ? i-1 : CHANNEL_COUNT-1);
  }
  ChannelMode = false; // memo is empty
}

void TRX::DeleteFreqFromChannel(uint8_t idx)
{
  struct MemoInfo mi;
  mi.Freq = 0;
  mi.band = 0;
  mi.sideband = 0;
  eeprom_write_block(&mi, eeMemoInfo+idx, sizeof(mi));
}

void TRX::GetChannelInfo(uint8_t idx, struct MemoInfo* ci)
{
  eeprom_read_block(ci, eeMemoInfo+idx, sizeof(struct MemoInfo));
}