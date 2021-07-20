void vfo_set_freq(long f1, long f2, long f3)
{
  int FQGRAN = (trx.sideband == AM ? 100 : 50);
  f1 = ((f1+FQGRAN/2)/FQGRAN)*FQGRAN;
#ifdef VFO_SI570  
  #ifdef VFO_SI5351
    vfo570.set_freq(f1);
    vfo5351.set_freq(f2,f3);
  #else
    // single Si570
    vfo570.set_freq(f1);
  #endif
#else
  #ifdef VFO_SI5351
    vfo5351.set_freq(f1,f2,f3);
  #endif
#endif
}

void UpdateFreq() 
{
  #define FREQ  (trx.Freq)
  long VFO,BFO;
  BFO = (trx.sideband == LSB ? ((BFO_USB)+Settings[ID_USB_SHIFT]) : (trx.sideband == USB ? ((BFO_LSB)+Settings[ID_LSB_SHIFT]) : 0));
  VFO = FREQ + BFO;        
  if (trx.sideband == AM) 
    vfo_set_freq(CLK0_RX_AM, CLK1_RX_AM, CLK2_RX_AM);
  else
    vfo_set_freq(CLK0_RX_SSB,CLK1_RX_SSB,CLK2_RX_SSB);
}