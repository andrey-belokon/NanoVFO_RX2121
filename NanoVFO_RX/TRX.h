#ifndef TRX_H
#define TRX_H

#include <Arduino.h>
#include "config.h"

class TRX {
  private:
    void Freq2Bound();
    void SwitchToBand(int band);
  public:
    long BandData[BAND_COUNT]; 
    int BandIndex;
    long Freq;
    long FreqMemo;
    uint8_t Fast;
    uint8_t Lock;
    uint8_t AttPre;    // 0-nothing; 1-ATT; 2-Preamp
    uint8_t sideband;
    uint8_t SMeter; // 0..15 
    uint16_t VCC;

	  TRX();
    void NextBand();
    void SelectBand(int band);
    void ChangeFreq(long freq_delta);
    void SaveFreqToMemo();
    void SwitchFreqToMemo();
    void SwitchAttPre();
    void SetFreqBand(long freq);
    const struct _Bands& GetBandInfo(uint8_t idx);

    uint16_t StateHash();
    void StateSave();
    void StateLoad();
};

#endif
