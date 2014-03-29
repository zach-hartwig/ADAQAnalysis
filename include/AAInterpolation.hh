//////////////////////////////////////////////////////////////////////////////////////////
//
// name: AAInterface.hh
// date: 28 Mar 14
// auth: Zach Hartwig
//
// desc: 
//
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __AAInterpolation_hh__
#define __AAInterpolation_hh__ 1

#include <TROOT.h>
#include <TObject.h>
#include <TGraph.h>


class AAInterpolation : public TObject
{
public:
  AAInterpolation();
  ~AAInterpolation();

  static AAInterpolation *GetInstance();

  double GetGammaEnergy(double);
  double GetProtonEnergy(double);
  double GetAlphaEnergy(double);
  double GetCarbonEnergy(double);

  TGraph *GetElectronResponse() {return ElectronResponse;}
  TGraph *GetProtonResponse() {return ProtonResponse;}
  TGraph *GetAlphaResponse() {return AlphaResponse;}
  TGraph *GetCarbonResponse() {return CarbonResponse;}

private:
  const double m_e;
  const double MeV2GeV;

  static AAInterpolation *TheInterpolationManager;

  TGraph *ElectronResponse, *ElectronInverse;
  TGraph *ProtonResponse, *ProtonInverse;
  TGraph *AlphaResponse, *AlphaInverse;
  TGraph *CarbonResponse, *CarbonInverse;
};
  

#endif
