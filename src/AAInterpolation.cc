#include <iostream>
#include <cmath>
using namespace std;


#include "AAInterpolation.hh"
#include "AAInterpolationData.hh"


AAInterpolation *AAInterpolation::TheInterpolationManager = 0;


AAInterpolation *AAInterpolation::GetInstance()
{ return TheInterpolationManager;}


AAInterpolation::AAInterpolation()
  : m_e(0.511), MeV2GeV(0.001)
{
  if(TheInterpolationManager)
    cout << "\nError! TheInterpolationManager was constructed twice!\n" << endl;
  TheInterpolationManager = this;

  ElectronResponse = new TGraph(LightEntries, EnergyDep, ElectronLight);
  ElectronInverse = new TGraph(LightEntries, ElectronLight, EnergyDep);

  ProtonResponse = new TGraph(LightEntries, EnergyDep, ProtonLight);
  ProtonInverse = new TGraph(LightEntries, ProtonLight, EnergyDep);
  
  AlphaResponse = new TGraph(LightEntries, EnergyDep, AlphaLight);
  AlphaInverse = new TGraph(LightEntries, AlphaLight, EnergyDep);

  CarbonResponse = new TGraph(LightEntries, EnergyDep, CarbonLight);
  CarbonInverse = new TGraph(LightEntries, CarbonLight, EnergyDep);
}


AAInterpolation::~AAInterpolation()
{
  delete ElectronResponse;
  delete ElectronInverse;
  delete ProtonResponse;
  delete ProtonInverse;
  delete AlphaResponse;
  delete AlphaInverse;
  delete CarbonResponse;
  delete CarbonInverse;
}

double AAInterpolation::GetGammaEnergy(double EE)
{
  double square_root = sqrt(pow(EE,2)-(4*-1*EE*m_e/2));
  return ((EE+square_root)/2);
}


double AAInterpolation::GetProtonEnergy(double EE)
{
  double Light = ElectronResponse->Eval(EE, 0, "S");
  return ProtonInverse->Eval(Light, 0, "S");
}


double AAInterpolation::GetAlphaEnergy(double EE)
{
  double Light = ElectronResponse->Eval(EE, 0, "S");
  return AlphaInverse->Eval(Light, 0, "S");
}


double AAInterpolation::GetCarbonEnergy(double EE)
{
  double Light = ElectronResponse->Eval(EE, 0, "S");
  return (CarbonInverse->Eval(Light, 0, "S")*MeV2GeV);
}
