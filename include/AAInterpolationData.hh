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
//       extracted carefully from the


Note that I have introduced reasonable extrapolations to 0.01
//       MeV to account for low energy interactions (as no data can be
//       found in this energy region).
//
//////////////////////////////////////////////////////////////////////

namespace{


  const int lightEntries = 33;

  // The number of scintillation photons produced per MeV of
  // electron-equivalent energy [MeVee] deposited into EJ301 (From
  // Eljen Tech. EJ301 product data sheet and confirmed by C. Hurlbut
  // at Eljen via email conversation)
  const double photonsPerMeVee = 12000.;

  // The conversion factor to convert from Verbinski's "light units"
  // to "MeVee" (See above ref, Section 7); Note that various authors
  // have used +/- ~5% of this value so there is some uncertainty
  //const double conversionFactor = // 1.2307;
  const double conversionFactor = 1.;
  
  // The energy deposited in the scintillator by particles [MeV]
  const double EnergyDep[lightEntries] = {0.010,
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

  double eLight[Entries];
  for(Int_t i=0; i<lightEntries; i++)
    eLight[i] = EnergyDep[i] * photonsPerMeVee;

  ////////////////
  // Protons - "p"

  double factor = 1.;

  const double pLight[lightEntries] = {15, // Cubic spline extrapolation from subsequent data
				       0.006710*photonsPerMeVee*conversionFactor*factor,
				       0.008860*photonsPerMeVee*conversionFactor*factor,
				       0.012070*photonsPerMeVee*conversionFactor*factor,
				       0.014650*photonsPerMeVee*conversionFactor*factor,
				       0.018380*photonsPerMeVee*conversionFactor*factor,
				       0.024600*photonsPerMeVee*conversionFactor*factor,
				       0.029000*photonsPerMeVee*conversionFactor*factor,
				       0.036500*photonsPerMeVee*conversionFactor*factor,
				       0.048300*photonsPerMeVee*conversionFactor*factor,
				       0.067800*photonsPerMeVee*conversionFactor*factor,
				       0.091000*photonsPerMeVee*conversionFactor*factor,
				       0.117500*photonsPerMeVee*conversionFactor*factor,
				       0.156200*photonsPerMeVee*conversionFactor*factor,
				       0.238500*photonsPerMeVee*conversionFactor*factor,
				       0.366000*photonsPerMeVee*conversionFactor*factor,
				       0.472500*photonsPerMeVee*conversionFactor*factor,
				       0.625000*photonsPerMeVee*conversionFactor*factor,
				       0.866000*photonsPerMeVee*conversionFactor*factor,
				       1.042000*photonsPerMeVee*conversionFactor*factor,
				       1.327000*photonsPerMeVee*conversionFactor*factor,
				       1.718000*photonsPerMeVee*conversionFactor*factor,
				       2.310000*photonsPerMeVee*conversionFactor*factor,
				       2.950000*photonsPerMeVee*conversionFactor*factor,
				       3.620000*photonsPerMeVee*conversionFactor*factor,
				       4.550000*photonsPerMeVee*conversionFactor*factor,
				       6.360000*photonsPerMeVee*conversionFactor*factor,
				       8.830000*photonsPerMeVee*conversionFactor*factor,
				       10.800000*photonsPerMeVee*conversionFactor*factor,
				       13.500000*photonsPerMeVee*conversionFactor*factor,
				       17.700000*photonsPerMeVee*conversionFactor*factor,
				       20.500000*photonsPerMeVee*conversionFactor*factor,
				       24.800000*photonsPerMeVee*conversionFactor*factor};
  
  ///////////////
  // Alphas - "a"

  const double aLight[lightEntries] = {5, // Cubic spline extrapolation from subsequent data
				       0.001640*photonsPerMeVee*conversionFactor,
				       0.002090*photonsPerMeVee*conversionFactor,
				       0.002720*photonsPerMeVee*conversionFactor,
				       0.003200*photonsPerMeVee*conversionFactor,
				       0.003860*photonsPerMeVee*conversionFactor,
				       0.004900*photonsPerMeVee*conversionFactor,
				       0.005640*photonsPerMeVee*conversionFactor,
				       0.006750*photonsPerMeVee*conversionFactor,
				       0.008300*photonsPerMeVee*conversionFactor,
				       0.010800*photonsPerMeVee*conversionFactor,
				       0.013500*photonsPerMeVee*conversionFactor,
				       0.016560*photonsPerMeVee*conversionFactor,
				       0.021000*photonsPerMeVee*conversionFactor,
				       0.030200*photonsPerMeVee*conversionFactor,
				       0.044100*photonsPerMeVee*conversionFactor,
				       0.056200*photonsPerMeVee*conversionFactor,
				       0.075000*photonsPerMeVee*conversionFactor,
				       0.110000*photonsPerMeVee*conversionFactor,
				       0.136500*photonsPerMeVee*conversionFactor,
				       0.181500*photonsPerMeVee*conversionFactor,
				       0.255500*photonsPerMeVee*conversionFactor,
				       0.407000*photonsPerMeVee*conversionFactor,
				       0.607000*photonsPerMeVee*conversionFactor,
				       0.870000*photonsPerMeVee*conversionFactor,
				       1.320000*photonsPerMeVee*conversionFactor,
				       2.350000*photonsPerMeVee*conversionFactor,
				       4.030000*photonsPerMeVee*conversionFactor,
				       5.440000*photonsPerMeVee*conversionFactor,
				       7.410000*photonsPerMeVee*conversionFactor,
				       10.420000*photonsPerMeVee*conversionFactor,
				       12.440000*photonsPerMeVee*conversionFactor,
				       15.500000*photonsPerMeVee*conversionFactor};


  ////////////////////
  // Carbon ions - "c"

  const double cLight[lightEntries] = {3, // Cubic spline extrapolation from subsequent data
				       0.001038*photonsPerMeVee*conversionFactor,
				       0.001270*photonsPerMeVee*conversionFactor,
				       0.001573*photonsPerMeVee*conversionFactor,
				       0.001788*photonsPerMeVee*conversionFactor,
				       0.002076*photonsPerMeVee*conversionFactor,
				       0.002506*photonsPerMeVee*conversionFactor,
				       0.002793*photonsPerMeVee*conversionFactor,
				       0.003191*photonsPerMeVee*conversionFactor,
				       0.003676*photonsPerMeVee*conversionFactor,
				       0.004361*photonsPerMeVee*conversionFactor,
				       0.005023*photonsPerMeVee*conversionFactor,
				       0.005686*photonsPerMeVee*conversionFactor,
				       0.006569*photonsPerMeVee*conversionFactor,
				       0.008128*photonsPerMeVee*conversionFactor,
				       0.010157*photonsPerMeVee*conversionFactor,
				       0.011647*photonsPerMeVee*conversionFactor,
				       0.013634*photonsPerMeVee*conversionFactor,
				       0.016615*photonsPerMeVee*conversionFactor,
				       0.018713*photonsPerMeVee*conversionFactor,
				       0.021859*photonsPerMeVee*conversionFactor,
				       0.026054*photonsPerMeVee*conversionFactor,
				       0.032347*photonsPerMeVee*conversionFactor,
				       0.038750*photonsPerMeVee*conversionFactor,
				       0.045154*photonsPerMeVee*conversionFactor,
				       0.053986*photonsPerMeVee*conversionFactor,
				       0.071346*photonsPerMeVee*conversionFactor,
				       0.098808*photonsPerMeVee*conversionFactor,
				       0.121440*photonsPerMeVee*conversionFactor,
				       0.153456*photonsPerMeVee*conversionFactor,
				       0.206448*photonsPerMeVee*conversionFactor,
				       0.246192*photonsPerMeVee*conversionFactor,
				       0.312432*photonsPerMeVee*conversionFactor};

}
