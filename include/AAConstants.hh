/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                           Copyright (C) 2012-2015                           //
//                 Zachary Seth Hartwig : All rights reserved                  //
//                                                                             //
//      The ADAQAnalysis source code is licensed under the GNU GPL v3.0.       //
//      You have the right to modify and/or redistribute this source code      //      
//      under the terms specified in the license, which may be found online    //
//      at http://www.gnu.org/licenses or at $ADAQANALYSIS/License.txt.        //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// 
// name: AAConstants.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAContants header file contains a (mostly anemic at this
//       point!) namespace with constants that are used throughout the
//       code.
//
/////////////////////////////////////////////////////////////////////////////////

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
}

#endif
