#ifndef acroEvent_hh
#define acroEvent_hh 1

#include "TObject.h"
#include "TString.h"

class acroEvent : public TObject {
public:
  acroEvent(){;}
  ~acroEvent(){;}

  void Reset(){;}

  // The recoil particle
  Double_t recoilEnergyDep;


  // The scintillation photons
  Int_t scintPhotonsCreated; // in the scintillator
  Int_t scintPhotonsCounted; // by the readout device

  ClassDef(acroEvent,1)
};

#endif
