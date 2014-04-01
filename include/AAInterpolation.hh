//////////////////////////////////////////////////////////////////////////////////////////
//
// name: AAInterface.hh
// date: 01 Mar 14
// auth: Zach Hartwig
//
// desc: 
//
//////////////////////////////////////////////////////////////////////////////////////////

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
