/////////////////////////////////////////////////////////////////////////////////
// 
// name: AAInterpolation.cc
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAInterpolation class handles the conversion of energy
//       deposited in liquid organic scintillators (obtained from
//       calibrated energy deposition spectra) into incident kinetic
//       energy of various particles. It is constructed as a Meyer's
//       singleton and made available throughout the code via static
//       methods. At present, only a single set of response functions
//       for EJ301/BC501A/NE213 is implemented; future work will
//       incorporate multiple response functions and multiple
//       scintillators for a more thorough set of analysis tools.
//
/////////////////////////////////////////////////////////////////////////////////

// C++
#include <iostream>
#include <cmath>
using namespace std;

// ADAQAnalysis
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

  // Put the data arrays into a vector for indexable access. There is
  // no electron data since it is constructed from the linear response

  Data.push_back(NULL);
  Data.push_back(ProtonData);
  Data.push_back(AlphaData);
  Data.push_back(CarbonData);

  // Initialize the vectors

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
  // Iterate to construct the scintillation light response
  // (E_deposited vs light output) and inverse response (light output
  // vs. E_deposited) for all particles
  for(int particle=0; particle<NumParticles; particle++){

    // Clear previous responses
    Light[particle].clear();
    delete Response[particle];
    delete Inverse[particle];

    for(int entry=0; entry<LightEntries; entry++){
      // Linear e- response is constructed from energy deposition
      if(particle == ELECTRON)
	Light[particle].push_back(EnergyDep[entry] * PhotonsPerMeVee);

      // Nonlinear p/a/c response is constructed from data
      else
	Light[particle].push_back(Data[particle][entry] * PhotonsPerMeVee * ConversionFactor);
    }

    // Construct the response and inversve functions
    Response[particle] = new TGraph(LightEntries, EnergyDep, &Light[particle][0]);
    Inverse[particle] = new TGraph(LightEntries, &Light[particle][0], EnergyDep);
  }
}


// Method to get the electron energy deposition based on the energy
// deposited by the specified particle
double AAInterpolation::GetElectronEnergy(double Energy, int Particle)
{
  double Light = Response[Particle]->Eval(Energy, 0, "S");
  return Inverse[ELECTRON]->Eval(Light, 0, "S");
}


// Method to get the gamma energy from electron equivalent energy (EE)
double AAInterpolation::GetGammaEnergy(double EE)
{
  double square_root = sqrt(pow(EE,2)-(4*-1*EE*m_e/2));
  return ((EE+square_root)/2);
}


// Method to get the proton/neutron energy from EE energy
double AAInterpolation::GetProtonEnergy(double EE)
{
  double Light = Response[ELECTRON]->Eval(EE, 0, "S");
  return Inverse[PROTON]->Eval(Light, 0, "S");
}


// Method to get the alpha energy from the EE energy
double AAInterpolation::GetAlphaEnergy(double EE)
{
  double Light = Response[ELECTRON]->Eval(EE, 0, "S");
  return Inverse[ALPHA]->Eval(Light, 0, "S");
}


// Method to get the carbon energy from the EE energy
double AAInterpolation::GetCarbonEnergy(double EE)
{
  double Light = Response[ELECTRON]->Eval(EE, 0, "S");
  return (Inverse[CARBON]->Eval(Light, 0, "S") * MeV2GeV);
}
