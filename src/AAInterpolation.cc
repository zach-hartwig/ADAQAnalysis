#include <iostream>
using namespace std;


#include "AAInterpolation.hh"
#include "AAInterpolationData.hh"


AAInterpolation *AAInterpolation::TheInterpolationManager = 0;


AAInterpolation *AAInterpolation::GetInstance()
{ return TheInterpolationManager;}


AAInterpolation::AAInterpolation()
{
  if(TheInterpolationManager)
    cout << "\nError! TheInterpolationManager was constructed twice!\n" << endl;
  else
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
