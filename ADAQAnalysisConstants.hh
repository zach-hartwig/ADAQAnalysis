#ifndef __ADAQAnalysisConstants_hh__
#define __ADAQAnalysisConstants_hh__ 1

namespace{
  double DynamicRange_V1720 = 2.; // [V]
  int BitDepth_1720 = 12; 
  int Bits_V1720 = 4096; // [ADC]
  int SampleRate_V1720 = 250e6; // [samples / s]

  double adc2volts_V1720 = DynamicRange_V1720 / Bits_V1720; // [V / ADC]
  double sample2seconds_V1720 = 1./SampleRate_V1720;

  double volts2amps_Pearson = 1.; // [A / V]
  double amplification_Pearson = 100.;
  
  double electron_charge = 1.602e-19;
}

#endif
