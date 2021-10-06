#ifndef TRX_H
#define TRX_H

#include <Arduino.h>
#include "config.h"

#define CHANNEL_COUNT  100

struct MemoInfo {
  long Freq;
  byte band;
  byte sideband;
};

class TRX {
  private:
    void Freq2Bound();
    void SwitchToBand(int band);
    void InitEEMemo();
  public:
    long BandData[BAND_COUNT]; 
    int BandIndex;
    long Freq;
    long FreqMemo; //!!
    bool Fast;
    bool Lock;
    uint8_t AttPre;    // 0-nothing; 1-ATT; 2-Preamp
    uint8_t sideband;
    uint8_t SMeter; // 0..15 
    uint16_t VCC;
    uint16_t VBAT;

    bool ChannelMode;
    uint8_t ChannelIndex;
    void SaveFreqToChannel(uint8_t idx);
    void DeleteFreqFromChannel(uint8_t idx);
    void SwitchFreqToNextChannel(bool up);
    void SwitchFreqToChannel(uint8_t idx);
    void SwitchChannelMode();
    void GetChannelInfo(uint8_t idx, struct MemoInfo* ci);

	  TRX();
    void NextBand();
    void SelectBand(int band);
    void ChangeFreq(long freq_delta);
    void SwitchAttPre();
    void SetFreqBand(long freq);
    const struct _Bands& GetBandInfo(uint8_t idx);

    uint16_t StateHash();
    void StateSave();
    void StateLoad();
};

#endif
