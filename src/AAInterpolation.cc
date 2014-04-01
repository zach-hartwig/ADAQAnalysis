#include <iostream>
#include <cmath>
using namespace std;


#include "AAInterpolation.hh"
#include "AAInterpolationData.hh"


AAInterpolation *AAInterpolation::TheInterpolationManager = 0;


AAInterpolation *AAInterpolation::GetInstance()
{ return TheInterpolationManager;}


AAInterpolation::AAInterpolation()
  : m_e(0.511), MeV2GeV(0.001), NumParticles(4),
    ConversionFactor(1.)
{
  if(TheInterpolationManager)
    cout << "\nError! TheInterpolationManager was constructed twice!\n" << endl;
  TheInterpolationManager = this;

  Data.push_back(ProtonData);
  Data.push_back(ProtonData);
  Data.push_back(AlphaData);
  Data.push_back(CarbonData);

  vector<double> Init;
  for(int i=0; i<NumParticles; i++){
    Light.push_back(Init);
    Response.push_back(new TGraph);
    Inverse.push_back(new TGraph);
  }
  ConstructResponses();
}


AAInterpolation::~AAInterpolation()
{
  for(int particle=0; particle<NumParticles; particle++){
    delete Response[particle];
    delete Inverse[particle];
  }  
}


void AAInterpolation::ConstructResponses()
{
  // Clear any previous light response curves and light responses
  for(int particle=0; particle<NumParticles; particle++){

    Light[particle].clear();
    delete Response[particle];
    delete Inverse[particle];

    for(int entry=0; entry<LightEntries; entry++){

      if(particle == ELECTRON)
	Light[particle].push_back(EnergyDep[entry] * PhotonsPerMeVee);
      else
	Light[particle].push_back(Data[particle][entry] * PhotonsPerMeVee * ConversionFactor);
      
      Response[particle] = new TGraph(LightEntries, EnergyDep, &Light[particle][0]);
      Inverse[particle] = new TGraph(LightEntries, &Light[particle][0], EnergyDep);
    }
  }
}


double AAInterpolation::GetElectronEnergy(double Energy, int Particle)
{
  double Light = Response[Particle]->Eval(Energy, 0, "S");
  return Inverse[ELECTRON]->Eval(Light, 0, "S");
}


double AAInterpolation::GetGammaEnergy(double EE)
{
  double square_root = sqrt(pow(EE,2)-(4*-1*EE*m_e/2));
  return ((EE+square_root)/2);
}


double AAInterpolation::GetProtonEnergy(double EE)
{
  double Light = Response[ELECTRON]->Eval(EE, 0, "S");
  return Inverse[PROTON]->Eval(Light, 0, "S");
}


double AAInterpolation::GetAlphaEnergy(double EE)
{
  double Light = Response[ELECTRON]->Eval(EE, 0, "S");
  return Inverse[ALPHA]->Eval(Light, 0, "S");
}


double AAInterpolation::GetCarbonEnergy(double EE)
{
  double Light = Response[ELECTRON]->Eval(EE, 0, "S");
  return (Inverse[CARBON]->Eval(Light, 0, "S") * MeV2GeV);
}
