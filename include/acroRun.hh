/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                           Copyright (C) 2012-2023                           //
//                 Zachary Seth Hartwig : All rights reserved                  //
//                                                                             //
//      The ADAQAnalysis source code is licensed under the GNU GPL v3.0.       //
//      You have the right to modify and/or redistribute this source code      //      
//      under the terms specified in the license, which may be found online    //
//      at http://www.gnu.org/licenses or at $ADAQANALYSIS/License.txt.       //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

#ifndef acroRun_hh
#define acroRun_hh 1

#include "TObject.h"

class acroRun : public TObject {
public:
  acroRun(){;}
  ~acroRun(){;}

  /////////////
  // Geant4 run
  Int_t NumberOfEvents;


  //////////////////
  // RFQ accelerator

  Double_t RFQ_DutyCycle;
  Double_t RFQ_InstCurrent;
  Double_t RFQ_ExposTime;

  
  /////////////////////////
  // PFC surface conditions

  
  //////////////
  // LS detector

  Int_t LSDetector_Hits;
  Double_t LSDetector_Efficiency, LSDetector_Threshold;

  Int_t LSDetector_ScintPhotonsCreated;
  Int_t LSDetector_ScintPhotonsCounted;
  Double_t LSDetector_OpticalEfficiency;


  //////////////
  // ES detector

  Int_t ESDetector_Hits;
  Double_t ESDetector_Efficiency, ESDetector_Threshold;

  Int_t ESDetector_ScintPhotonsCreated;
  Int_t ESDetector_ScintPhotonsCounted;
  Double_t ESDetector_OpticalEfficiency;


  //////////////////
  // Fission chamber
  
  Int_t FissionChamber_NeutronHits;

  ClassDef(acroRun,1)
};

#endif
