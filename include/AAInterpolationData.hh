//////////////////////////////////////////////////////////////////////
// name: AAInterpolationData.hh
// date: 28 Mar 14
// auth: Zach Hartwig
//
// desc: This namespace contains the EJ301/EJ309 scintillation light
//       response function data for electrons, protons, alphas, and
//       carbon ions. The data is used by the AAInterpolation class to
//       provide on-the-fly calculations of the energy deposited in an
//       EJ301/EJ309 scintillation detector by different particles
//       given a known energy in electron equivalent units (usually
//       keVee or MeVee). The scintillation light output (proton,
//       alpha carbon) data is taken directly from:
//
//          V.V Verbinski, Nucl. Instr. and Meth. 65 (1968) 8-25, Table 1
//
//       The electron response is known to be linear to data was
//       extracted carefully from the Note that I have introduced
//       reasonable extrapolations to 0.01 MeV to account for low
//       energy interactions (as no data can be found in this energy
//       region).
//
//////////////////////////////////////////////////////////////////////

namespace{


  const int LightEntries = 33;

  // The number of scintillation photons produced per MeV of
  // electron-equivalent energy [MeVee] deposited into EJ301 (From
  // Eljen Tech. EJ301 product data sheet and confirmed by C. Hurlbut
  // at Eljen via email conversation)
  const double PhotonsPerMeVee = 12000.;

  // The conversion factor to convert from Verbinski's "light units"
  // to "MeVee" (See above ref, Section 7); Note that various authors
  // have used +/- ~5% of this value so there is some uncertainty
  //const double conversionFactor = // 1.2307;
  const double ConversionFactor = 1.;
  
  // The energy deposited in the scintillator by particles [MeV]
  const double EnergyDep[LightEntries] = {0.010,
					  0.100,
					  0.130,
					  0.170,
					  0.200,
					  0.240,
					  0.300,
					  0.340,
					  0.400,
					  0.480,
					  0.600,
					  0.720,
					  0.840,
					  1.000,
					  1.300,
					  1.700,
					  2.000,
					  2.400,
					  3.000,
					  3.400,
					  4.000,
					  4.800,
					  6.000,
					  7.200,
					  8.400,
					  10.00,
					  13.00,
					  17.00,
					  20.00,
					  24.00,
					  30.00,
					  34.00,
					  40.00};

  // The light produced (scintillation photons) is calculated for each
  // particle by multiplying the amount of energy deposited into the
  // scintillator in MeVee by (photons/MeVee). The numerical values
  // listed first in each array entry for protons/alphas/carbon are
  // the electron-equivalent energies that the particle deposits for
  // the corresponding energy deposition in the EnergyDep array.


  //////////////////
  // Electrons - "e"

  const double ElectronLight[LightEntries] = {EnergyDep[0]*PhotonsPerMeVee,
					      EnergyDep[1]*PhotonsPerMeVee,
					      EnergyDep[2]*PhotonsPerMeVee,
					      EnergyDep[3]*PhotonsPerMeVee,
					      EnergyDep[4]*PhotonsPerMeVee,
					      EnergyDep[5]*PhotonsPerMeVee,
					      EnergyDep[6]*PhotonsPerMeVee,
					      EnergyDep[7]*PhotonsPerMeVee,
					      EnergyDep[8]*PhotonsPerMeVee,
					      EnergyDep[9]*PhotonsPerMeVee,
					      EnergyDep[10]*PhotonsPerMeVee,
					      EnergyDep[11]*PhotonsPerMeVee,
					      EnergyDep[12]*PhotonsPerMeVee,
					      EnergyDep[13]*PhotonsPerMeVee,
					      EnergyDep[14]*PhotonsPerMeVee,
					      EnergyDep[15]*PhotonsPerMeVee,
					      EnergyDep[16]*PhotonsPerMeVee,
					      EnergyDep[17]*PhotonsPerMeVee,
					      EnergyDep[18]*PhotonsPerMeVee,
					      EnergyDep[19]*PhotonsPerMeVee,
					      EnergyDep[20]*PhotonsPerMeVee,
					      EnergyDep[21]*PhotonsPerMeVee,
					      EnergyDep[22]*PhotonsPerMeVee,
					      EnergyDep[23]*PhotonsPerMeVee,
					      EnergyDep[24]*PhotonsPerMeVee,
					      EnergyDep[25]*PhotonsPerMeVee,
					      EnergyDep[26]*PhotonsPerMeVee,
					      EnergyDep[27]*PhotonsPerMeVee,
					      EnergyDep[28]*PhotonsPerMeVee,
					      EnergyDep[29]*PhotonsPerMeVee,
					      EnergyDep[30]*PhotonsPerMeVee,
					      EnergyDep[31]*PhotonsPerMeVee,
					      EnergyDep[32]*PhotonsPerMeVee};


  ////////////////
  // Protons - "p"

  const double ProtonLight[LightEntries] = {15, // Cubic spline extrapolation from subsequent data
					    0.006710*PhotonsPerMeVee*ConversionFactor,
					    0.008860*PhotonsPerMeVee*ConversionFactor,
					    0.012070*PhotonsPerMeVee*ConversionFactor,
					    0.014650*PhotonsPerMeVee*ConversionFactor,
					    0.018380*PhotonsPerMeVee*ConversionFactor,
					    0.024600*PhotonsPerMeVee*ConversionFactor,
					    0.029000*PhotonsPerMeVee*ConversionFactor,
					    0.036500*PhotonsPerMeVee*ConversionFactor,
					    0.048300*PhotonsPerMeVee*ConversionFactor,
					    0.067800*PhotonsPerMeVee*ConversionFactor,
					    0.091000*PhotonsPerMeVee*ConversionFactor,
					    0.117500*PhotonsPerMeVee*ConversionFactor,
					    0.156200*PhotonsPerMeVee*ConversionFactor,
					    0.238500*PhotonsPerMeVee*ConversionFactor,
					    0.366000*PhotonsPerMeVee*ConversionFactor,
					    0.472500*PhotonsPerMeVee*ConversionFactor,
					    0.625000*PhotonsPerMeVee*ConversionFactor,
					    0.866000*PhotonsPerMeVee*ConversionFactor,
					    1.042000*PhotonsPerMeVee*ConversionFactor,
					    1.327000*PhotonsPerMeVee*ConversionFactor,
					    1.718000*PhotonsPerMeVee*ConversionFactor,
					    2.310000*PhotonsPerMeVee*ConversionFactor,
					    2.950000*PhotonsPerMeVee*ConversionFactor,
					    3.620000*PhotonsPerMeVee*ConversionFactor,
					    4.550000*PhotonsPerMeVee*ConversionFactor,
					    6.360000*PhotonsPerMeVee*ConversionFactor,
					    8.830000*PhotonsPerMeVee*ConversionFactor,
					    10.800000*PhotonsPerMeVee*ConversionFactor,
					    13.500000*PhotonsPerMeVee*ConversionFactor,
					    17.700000*PhotonsPerMeVee*ConversionFactor,
					    20.500000*PhotonsPerMeVee*ConversionFactor,
					    24.800000*PhotonsPerMeVee*ConversionFactor};
  
  ///////////////
  // Alphas - "a"

  const double AlphaLight[LightEntries] = {5, // Cubic spline extrapolation from subsequent data
					   0.001640*PhotonsPerMeVee*ConversionFactor,
					   0.002090*PhotonsPerMeVee*ConversionFactor,
					   0.002720*PhotonsPerMeVee*ConversionFactor,
					   0.003200*PhotonsPerMeVee*ConversionFactor,
					   0.003860*PhotonsPerMeVee*ConversionFactor,
					   0.004900*PhotonsPerMeVee*ConversionFactor,
					   0.005640*PhotonsPerMeVee*ConversionFactor,
					   0.006750*PhotonsPerMeVee*ConversionFactor,
					   0.008300*PhotonsPerMeVee*ConversionFactor,
					   0.010800*PhotonsPerMeVee*ConversionFactor,
					   0.013500*PhotonsPerMeVee*ConversionFactor,
					   0.016560*PhotonsPerMeVee*ConversionFactor,
					   0.021000*PhotonsPerMeVee*ConversionFactor,
					   0.030200*PhotonsPerMeVee*ConversionFactor,
					   0.044100*PhotonsPerMeVee*ConversionFactor,
					   0.056200*PhotonsPerMeVee*ConversionFactor,
					   0.075000*PhotonsPerMeVee*ConversionFactor,
					   0.110000*PhotonsPerMeVee*ConversionFactor,
					   0.136500*PhotonsPerMeVee*ConversionFactor,
					   0.181500*PhotonsPerMeVee*ConversionFactor,
					   0.255500*PhotonsPerMeVee*ConversionFactor,
					   0.407000*PhotonsPerMeVee*ConversionFactor,
					   0.607000*PhotonsPerMeVee*ConversionFactor,
					   0.870000*PhotonsPerMeVee*ConversionFactor,
					   1.320000*PhotonsPerMeVee*ConversionFactor,
					   2.350000*PhotonsPerMeVee*ConversionFactor,
					   4.030000*PhotonsPerMeVee*ConversionFactor,
					   5.440000*PhotonsPerMeVee*ConversionFactor,
					   7.410000*PhotonsPerMeVee*ConversionFactor,
					   10.420000*PhotonsPerMeVee*ConversionFactor,
					   12.440000*PhotonsPerMeVee*ConversionFactor,
					   15.500000*PhotonsPerMeVee*ConversionFactor};


  ////////////////////
  // Carbon ions - "c"

  const double CarbonLight[LightEntries] = {3, // Cubic spline extrapolation from subsequent data
					    0.001038*PhotonsPerMeVee*ConversionFactor,
					    0.001270*PhotonsPerMeVee*ConversionFactor,
					    0.001573*PhotonsPerMeVee*ConversionFactor,
					    0.001788*PhotonsPerMeVee*ConversionFactor,
					    0.002076*PhotonsPerMeVee*ConversionFactor,
					    0.002506*PhotonsPerMeVee*ConversionFactor,
					    0.002793*PhotonsPerMeVee*ConversionFactor,
					    0.003191*PhotonsPerMeVee*ConversionFactor,
					    0.003676*PhotonsPerMeVee*ConversionFactor,
					    0.004361*PhotonsPerMeVee*ConversionFactor,
					    0.005023*PhotonsPerMeVee*ConversionFactor,
					    0.005686*PhotonsPerMeVee*ConversionFactor,
					    0.006569*PhotonsPerMeVee*ConversionFactor,
					    0.008128*PhotonsPerMeVee*ConversionFactor,
					    0.010157*PhotonsPerMeVee*ConversionFactor,
					    0.011647*PhotonsPerMeVee*ConversionFactor,
					    0.013634*PhotonsPerMeVee*ConversionFactor,
					    0.016615*PhotonsPerMeVee*ConversionFactor,
					    0.018713*PhotonsPerMeVee*ConversionFactor,
					    0.021859*PhotonsPerMeVee*ConversionFactor,
					    0.026054*PhotonsPerMeVee*ConversionFactor,
					    0.032347*PhotonsPerMeVee*ConversionFactor,
					    0.038750*PhotonsPerMeVee*ConversionFactor,
					    0.045154*PhotonsPerMeVee*ConversionFactor,
					    0.053986*PhotonsPerMeVee*ConversionFactor,
					    0.071346*PhotonsPerMeVee*ConversionFactor,
					    0.098808*PhotonsPerMeVee*ConversionFactor,
					    0.121440*PhotonsPerMeVee*ConversionFactor,
					    0.153456*PhotonsPerMeVee*ConversionFactor,
					    0.206448*PhotonsPerMeVee*ConversionFactor,
					    0.246192*PhotonsPerMeVee*ConversionFactor,
					    0.312432*PhotonsPerMeVee*ConversionFactor};

}
