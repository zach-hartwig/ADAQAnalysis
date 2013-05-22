#ifndef __AAConstants_hh__
#define __AAConstants_hh__ 1

namespace{
  const double DynamicRange_V1720 = 2.; // [V]
  const int BitDepth_1720 = 12; 
  const int Bits_V1720 = pow(2.,BitDepth_1720);
  const int SampleRate_V1720 = 250e6; // [samples / s]

  const double adc2volts_V1720 = DynamicRange_V1720 / Bits_V1720; // [V / ADC]
  const double sample2seconds_V1720 = 1./SampleRate_V1720;

  const double volts2amps_Pearson = 1.; // [A / V]
  const double amplification_Pearson = 100.;
  
  const double electron_charge = 1.602e-19;

  const double us2s = 1e-6; // Conversion: microseconds to second


  // The following variables are used to create the CalibrationManager
  // TGraph object when the user has selected the "EP detector" option
  // under spectrum calibration. The two data points define the least
  // square fit result line to the EP detector calibration. Details
  // can be found in $ADAQREPO/analysis/root/EPCalibration
  
  const int NumCalibrationPoints_FixedEP = 2;
  const double PulseUnitPoints_FixedEP[2] = {1676.32, 19509.10};
  const double EnergyPoints_FixedEP[2] = {200., 2500.};
}

#endif
