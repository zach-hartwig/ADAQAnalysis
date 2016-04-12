/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                           Copyright (C) 2012-2016                           //
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
// name: AAInterpolation.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAInterpolation class handles the conversion of energy
//       deposited in liquid organic scintillators (obtained from
//       calibrated energy deposition spectra) into incident kinetic
//       energy of various particles. At present, only a single set of
//       response functions for EJ301/BC501A/NE213 is implemented;
//       future work will incorporate multiple response functions and
//       multiple scintillators for a more thorough set of analysis
//       tools.
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef __AAInterpolation_hh__
#define __AAInterpolation_hh__ 1

// ROOT
#include <TROOT.h>
#include <TObject.h>
#include <TGraph.h>

// C++
#include <vector>

class AAInterpolation : public TObject
{
public:
  AAInterpolation();
  ~AAInterpolation();

  static AAInterpolation *GetInstance();

  // Method to construct particle-dependent light responses
  void ConstructResponses();
  
  // Set/get methods for member data

  void SetConversionFactor(double CF) {ConversionFactor = CF;}
  double GetConversionFactor() {return ConversionFactor;}

  double GetElectronEnergy(double, int);
  double GetGammaEnergy(double);
  double GetProtonEnergy(double);
  double GetAlphaEnergy(double);
  double GetCarbonEnergy(double);
  
  TGraph *GetElectronResponse() {return Response[ELECTRON];}
  TGraph *GetProtonResponse() {return Response[PROTON];}
  TGraph *GetAlphaResponse() {return Response[ALPHA];}
  TGraph *GetCarbonResponse() {return Response[CARBON];}
  
private:
  const double m_e, MeV2GeV;
  const int NumParticles;

  enum {ELECTRON, PROTON, ALPHA, CARBON};

  double ConversionFactor;

  vector<const double *> Data;
  vector< vector<double> > Light;
  vector<TGraph *> Response, Inverse;

  static AAInterpolation *TheInterpolationManager;
};
  

#endif
