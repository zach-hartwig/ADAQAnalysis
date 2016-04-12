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
