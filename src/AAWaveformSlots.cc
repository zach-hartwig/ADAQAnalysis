/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                           Copyright (C) 2012-2015                           //
//                 Zachary Seth Hartwig : All rights reserved                  //
//                                                                             //
//      The ADAQAnalysis source code is licensed under the GNU GPL v3.0.       //
//      You have the right to modify and/or redistribute this source code      //      
//      under the terms specified in the license, which may be found online    //
//      at http://www.gnu.org/licenses or at $ADAQANALYSIS/License.txt.        //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// 
// name: AAWaveformSlots.cc
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAWaveformSlots class contains widget slot methods to
//       handle signals generated from widgets contained on the
//       "waveform" tab of the ADAQAnalysis GUI
//
/////////////////////////////////////////////////////////////////////////////////

#include "AAWaveformSlots.hh"
#include "AAInterface.hh"
#include "AAGraphics.hh"


AAWaveformSlots::AAWaveformSlots(AAInterface *TI)
  : TheInterface(TI)
{
  ComputationMgr = AAComputation::GetInstance();
  GraphicsMgr = AAGraphics::GetInstance();
}


AAWaveformSlots::~AAWaveformSlots()
{;}


void AAWaveformSlots::HandleCheckButtons()
{
  if(!TheInterface->EnableInterface or !TheInterface->ADAQFileLoaded)
    return;
  
  TGCheckButton *CheckButton = (TGCheckButton *) gTQSender;
  int CheckButtonID = CheckButton->WidgetId();

  TheInterface->SaveSettings();

  switch(CheckButtonID){

  case FindPeaks_CB_ID:
    if(TheInterface->FindPeaks_CB->IsDown()){
      ComputationMgr->CreateNewPeakFinder(TheInterface->ADAQSettings->MaxPeaks);
      TheInterface->SetPeakFindingWidgetState(true, kButtonUp);
    }
    else
      TheInterface->SetPeakFindingWidgetState(false, kButtonDisabled);
    
    GraphicsMgr->PlotWaveform();
    break;

  case UseMarkovSmoothing_CB_ID:
    GraphicsMgr->PlotWaveform();
    break;

  case PlotFloor_CB_ID:
  case PlotCrossings_CB_ID:
  case PlotPeakIntegratingRegion_CB_ID:
  case PlotAnalysisRegion_CB_ID:
  case PlotBaselineRegion_CB_ID:
  case PlotZeroSuppressionCeiling_CB_ID:
  case PlotTrigger_CB_ID:
  case UsePileupRejection_CB_ID:
  case AutoYAxisRange_CB_ID:
  case WaveformAnalysis_CB_ID:
    
    if(TheInterface->PlotPearsonIntegration_CB->IsDown()){
      
      // Reset the total number of deuterons
      ComputationMgr->SetDeuteronsInTotal(0.);
      TheInterface->DeuteronsInTotal_NEFL->GetEntry()->SetNumber(0.);

      GraphicsMgr->PlotWaveform();
      
      double DeuteronsInWaveform = ComputationMgr->GetDeuteronsInWaveform();
      TheInterface->DeuteronsInWaveform_NEFL->GetEntry()->SetNumber(DeuteronsInWaveform);
      
      double DeuteronsInTotal = ComputationMgr->GetDeuteronsInTotal();
      TheInterface->DeuteronsInTotal_NEFL->GetEntry()->SetNumber(DeuteronsInTotal);
    }
    else
      GraphicsMgr->PlotWaveform();
    break;
  }
}


void AAWaveformSlots::HandleComboBoxes(int ComboBoxID, int SelectedID)
{
  if(!TheInterface->EnableInterface or !TheInterface->ADAQFileLoaded)
    return;
  
  TheInterface->SaveSettings();

  switch(ComboBoxID){

  case ChannelSelector_CBL_ID:
    break;

  default:
    break;
  }
}


void AAWaveformSlots::HandleNumberEntries()
{
  if(!TheInterface->EnableInterface or !TheInterface->ADAQFileLoaded)
    return;
  
  TheInterface->SaveSettings();
  
    // Get the widget and ID from which the signal was sent
  TGNumberEntry *NumberEntry = (TGNumberEntry *) gTQSender;
  int NumberEntryID = NumberEntry->WidgetId();

  switch(NumberEntryID){
  case WaveformSelector_NEL_ID:{
    int Num = TheInterface->WaveformSelector_NEL->GetEntry()->GetIntNumber();
    TheInterface->WaveformSelector_HS->SetPosition(Num);
    
    GraphicsMgr->PlotWaveform();

    if(TheInterface->IntegratePearson_CB->IsDown()){
      double DeuteronsInWaveform = ComputationMgr->GetDeuteronsInWaveform();
      TheInterface->DeuteronsInWaveform_NEFL->GetEntry()->SetNumber(DeuteronsInWaveform);

      double DeuteronsInTotal = ComputationMgr->GetDeuteronsInTotal();
      TheInterface->DeuteronsInTotal_NEFL->GetEntry()->SetNumber(DeuteronsInTotal);
    }

    if(TheInterface->WaveformAnalysis_CB->IsDown()){
      double Height = ComputationMgr->GetWaveformAnalysisHeight();
      TheInterface->WaveformHeight_NEL->GetEntry()->SetNumber(Height);

      double Area = ComputationMgr->GetWaveformAnalysisArea();
      TheInterface->WaveformIntegral_NEL->GetEntry()->SetNumber(Area);
    }
    break;
  }

  case MaxPeaks_NEL_ID:
    ComputationMgr->CreateNewPeakFinder(TheInterface->ADAQSettings->MaxPeaks);
    GraphicsMgr->PlotWaveform();
    break;

  case Sigma_NEL_ID:
  case Resolution_NEL_ID:
  case Floor_NEL_ID:
  case AnalysisRegionMin_NEL_ID:
  case AnalysisRegionMax_NEL_ID:
  case BaselineRegionMin_NEL_ID:
  case BaselineRegionMax_NEL_ID:
  case ZeroSuppressionCeiling_NEL_ID:
  case ZeroSuppressionBuffer_NEL_ID:
    GraphicsMgr->PlotWaveform();
    break;

  default:
    break;
  }
}


void AAWaveformSlots::HandleRadioButtons()
{
  if(!TheInterface->EnableInterface or !TheInterface->ADAQFileLoaded)
    return;
  
  TGRadioButton *RadioButton = (TGRadioButton *) gTQSender;
  int RadioButtonID = RadioButton->WidgetId();
  
  TheInterface->SaveSettings();

  switch(RadioButtonID){

  case RawWaveform_RB_ID:
    TheInterface->FindPeaks_CB->SetState(kButtonUp);
    TheInterface->FindPeaks_CB->SetState(kButtonDisabled);
    TheInterface->WaveformAnalysis_CB->SetState(kButtonUp);
    TheInterface->WaveformAnalysis_CB->SetState(kButtonDisabled);

    TheInterface->SaveSettings();

    GraphicsMgr->PlotWaveform();
    break;
    
  case BaselineSubtractedWaveform_RB_ID:
    TheInterface->FindPeaks_CB->SetState(kButtonUp);
    TheInterface->WaveformAnalysis_CB->SetState(kButtonUp);

    GraphicsMgr->PlotWaveform();
    break;
    
  case ZeroSuppressionWaveform_RB_ID:
    TheInterface->FindPeaks_CB->SetState(kButtonUp);
    TheInterface->WaveformAnalysis_CB->SetState(kButtonUp);
    
    GraphicsMgr->PlotWaveform();
    break;
    
  case PositiveWaveform_RB_ID:
  case NegativeWaveform_RB_ID:
    GraphicsMgr->PlotWaveform();
    break;
  }
}
