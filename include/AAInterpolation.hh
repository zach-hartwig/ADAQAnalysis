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

  TGraph *GetElectronResponse() {return ElectronResponse;}
  TGraph *GetProtonResponse() {return ProtonResponse;}
  TGraph *GetAlphaResponse() {return AlphaResponse;}
  TGraph *GetCarbonResponse() {return CarbonResponse;}

  double GetProtonEnergyDeposition(double ElectronEquivalent)
  { return ProtonInverse->Eval(ElectronEquivalent, 0, "S"); }
  
  double GetAlphaEnergyDeposition(double ElectronEquivalent)
  { return AlphaInverse->Eval(ElectronEquivalent, 0, "S"); }
  
  double GetCarbonDeposition(double ElectronEquivalent)
  { return CarbonInverse->Eval(ElectronEquivalent, 0, "S"); }

private:
  static AAInterpolation *TheInterpolationManager;

  TGraph *ElectronResponse, *ElectronInverse;
  TGraph *ProtonResponse, *ProtonInverse;
  TGraph *AlphaResponse, *AlphaInverse;
  TGraph *CarbonResponse, *CarbonInverse;
};
  

#endif
