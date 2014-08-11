//////////////////////////////////////////////////////////////////////
// name: AAInterpolationData.hh
// date: 28 Mar 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
//
// desc: This namespace contains the EJ301 scintillation light
//       response function data for electrons, protons, alphas, and
//       carbon ions. The data is used by the AAInterpolation class to
//       provide on-the-fly calculations of the energy deposited in an
//       EJ301 scintillation detector by different particles given a
//       known energy in electron equivalent units (usually keVee or
//       MeVee). The scintillation light output (proton, alpha carbon)
//       data is taken directly from:
//
//          V.V Verbinski, Nucl. Instr. and Meth. 65 (1968) 8-25, Table 1
//
//       The electron response is known to be linear to data was
//       extracted carefully from the Note that I have introduced
//       reasonable extrapolations to 0.01 MeV to account for low
//       energy interactions (as no data can be found in this energy
//       region).
//
//       The 'conversion factor' used in AAInterplation class is used
//       to convert from Verbinksi's "light units" to "MeVee (See the
//       above reference, Section 7). There is some discrepancy in the
//       academic and product literature about the value to use
//       here. For example, in N.P. Hawkes et al, NIM A 476 (2002)
//       190, the value of 1,2307 was used, whereas is appears the
//       Eljen Technology (See EJ301 light response product
//       documention) used a value of 1.0000. The user is given the
//       freedom to choose the conversion factor so must know what
//       he/she is doing! A scary thought...
//
//////////////////////////////////////////////////////////////////////

namespace{

  const int LightEntries = 33;

  // The number of scintillation photons produced per MeV of
  // electron-equivalent energy [MeVee] deposited into EJ301 (From
  // Eljen Tech. EJ301 product data sheet and confirmed by C. Hurlbut
  // at Eljen via email conversation)
  const double PhotonsPerMeVee = 12000.;

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

  // The following for protons, alpha, and carbon ions is taken
  // directly from V.V. Verbinksi's paper and is in his published
  // "light units", which will be converted to light output in the
  // AAInterpolation class based on the user's choice of conversion
  // factor. The first point in each array is a cubic spline
  // extrapoloation down to 10 keV based on Verbinski's data.

  ////////////////
  // Protons - "p"


  const double ProtonData[LightEntries] = {0.001250,
					   0.006710,
					   0.008860,
					   0.012070,
					   0.014650,
					   0.018380,
					   0.024600,
					   0.029000,
					   0.036500,
					   0.048300,
					   0.067800,
					   0.091000,
					   0.117500,
					   0.156200,
					   0.238500,
					   0.366000,
					   0.472500,
					   0.625000,
					   0.866000,
					   1.042000,
					   1.327000,
					   1.718000,
					   2.310000,
					   2.950000,
					   3.620000,
					   4.550000,
					   6.360000,
					   8.830000,
					   10.800000,
					   13.500000,
					   17.700000,
					   20.500000,
					   24.800000};
  
  ///////////////
  // Alphas - "a"

  const double AlphaData[LightEntries] = {0.000417, 
					  0.001640,
					  0.002090,
					  0.002720,
					  0.003200,
					  0.003860,
					  0.004900,
					  0.005640,
					  0.006750,
					  0.008300,
					  0.010800,
					  0.013500,
					  0.016560,
					  0.021000,
					  0.030200,
					  0.044100,
					  0.056200,
					  0.075000,
					  0.110000,
					  0.136500,
					  0.181500,
					  0.255500,
					  0.407000,
					  0.607000,
					  0.870000,
					  1.320000,
					  2.350000,
					  4.030000,
					  5.440000,
					  7.410000,
					  10.420000,
					  12.440000,
					  15.500000};


  ////////////////////
  // Carbon ions - "c"

  const double CarbonData[LightEntries] = {0.000250,
					   0.001038,
					   0.001270,
					   0.001573,
					   0.001788,
					   0.002076,
					   0.002506,
					   0.002793,
					   0.003191,
					   0.003676,
					   0.004361,
					   0.005023,
					   0.005686,
					   0.006569,
					   0.008128,
					   0.010157,
					   0.011647,
					   0.013634,
					   0.016615,
					   0.018713,
					   0.021859,
					   0.026054,
					   0.032347,
					   0.038750,
					   0.045154,
					   0.053986,
					   0.071346,
					   0.098808,
					   0.121440,
					   0.153456,
					   0.206448,
					   0.246192,
					   0.312432};

}
