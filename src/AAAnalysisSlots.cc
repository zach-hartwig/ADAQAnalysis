#include "AAAnalysisSlots.hh"
#include "AAInterface.hh"
#include "AAGraphics.hh"

AAAnalysisSlots::AAAnalysisSlots(AAInterface *TI)
  : TheInterface(TI)
{
  ComputationMgr = AAComputation::GetInstance();
  GraphicsMgr = AAGraphics::GetInstance();
  InterpolationMgr = AAInterpolation::GetInstance();
}

AAAnalysisSlots::~AAAnalysisSlots()
{;}


void AAAnalysisSlots::HandleCheckButtons()
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;
  
  TGCheckButton *CheckButton = (TGCheckButton *) gTQSender;
  int CheckButtonID = CheckButton->WidgetId();

  TheInterface->SaveSettings();


  switch(CheckButtonID){
    
  case SpectrumFindBackground_CB_ID:{
    if(!ComputationMgr->GetSpectrumExists())
      break;
    
    bool WidgetState = false;
    EButtonState ButtonState = kButtonDisabled;
    
    if(TheInterface->SpectrumFindBackground_CB->IsDown()){
      ButtonState = kButtonUp;
      WidgetState = true;

      ComputationMgr->CalculateSpectrumBackground();
      GraphicsMgr->PlotSpectrum();
    }
    else{
      TheInterface->SpectrumWithBackground_RB->SetState(kButtonDown, true);

      GraphicsMgr->PlotSpectrum();
    }

    TheInterface->SetSpectrumBackgroundWidgetState(WidgetState, ButtonState);

    break;
  }

    
  case SpectrumBackgroundCompton_CB_ID:
  case SpectrumBackgroundSmoothing_CB_ID:
    ComputationMgr->CalculateSpectrumBackground();
    GraphicsMgr->PlotSpectrum();
    break;
    

  case SpectrumFindIntegral_CB_ID:
  case SpectrumIntegralInCounts_CB_ID:
    if(TheInterface->SpectrumFindIntegral_CB->IsDown())
      TheInterface->SpectrumIntegrationLimits_DHS->PositionChanged();
    else
      GraphicsMgr->PlotSpectrum();
    break;
    
  case SpectrumUseGaussianFit_CB_ID:
    if(TheInterface->SpectrumUseGaussianFit_CB->IsDown())
      TheInterface->SpectrumIntegrationLimits_DHS->PositionChanged();
    else{
      if(TheInterface->SpectrumFindIntegral_CB->IsDown())
	TheInterface->SpectrumIntegrationLimits_DHS->PositionChanged();
      else
	GraphicsMgr->PlotSpectrum();
    }
    break;

  case EAEnable_CB_ID:{
    
    int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();
    
    bool SpectrumIsCalibrated = ComputationMgr->GetUseSpectraCalibrations()[Channel];
    
    int Type = TheInterface->EASpectrumType_CBL->GetComboBox()->GetSelected();
    
    if(TheInterface->EAEnable_CB->IsDown() and SpectrumIsCalibrated){
      TheInterface->SpectrumCalibration_CB->SetState(kButtonUp, true);
      TheInterface->SetCalibrationWidgetState(false, kButtonDisabled);
      
      TheInterface->EASpectrumType_CBL->GetComboBox()->SetEnabled(true);
      
      if(Type == 0){
	TheInterface->SetEAGammaWidgetState(true, kButtonUp);
	TheInterface->SetEANeutronWidgetState(false, kButtonDisabled);
      }
      else if(Type == 1){
	TheInterface->SetEAGammaWidgetState(false, kButtonDisabled);
	TheInterface->SetEANeutronWidgetState(true, kButtonUp);
      }
      TheInterface->HandleTripleSliderPointer();
    }
    else{
      TheInterface->EASpectrumType_CBL->GetComboBox()->SetEnabled(false);

      TheInterface->SetEAGammaWidgetState(false, kButtonDisabled);
      TheInterface->SetEANeutronWidgetState(false, kButtonDisabled);
      
      if(SpectrumIsCalibrated)
	GraphicsMgr->PlotSpectrum();
    }
    break;
  }
    
  case EAEscapePeaks_CB_ID:
    TheInterface->HandleTripleSliderPointer();
    break;
  }
}


void AAAnalysisSlots::HandleComboBoxes(int ComboBoxID, int SelectedID)
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;
  
  TheInterface->SaveSettings();
  
  switch(ComboBoxID){

  case SpectrumBackgroundDirection_CBL_ID:
  case SpectrumBackgroundFilterOrder_CBL_ID:
  case SpectrumBackgroundSmoothingWidth_CBL_ID:
    ComputationMgr->CalculateSpectrumBackground();
    GraphicsMgr->PlotSpectrum();
    break;
    
  case EASpectrumType_CBL_ID:
    if(SelectedID == 0){
      TheInterface->SetEAGammaWidgetState(true, kButtonUp);
      TheInterface->SetEANeutronWidgetState(false, kButtonDisabled);
    }
    else if(SelectedID == 1){
      TheInterface->SetEAGammaWidgetState(false, kButtonDisabled);
      TheInterface->SetEANeutronWidgetState(true, kButtonUp);
    }
    break;
    
  default:
    break;
    
  }
}


void AAAnalysisSlots::HandleNumberEntries()
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;

  TGNumberEntry *NumberEntry = (TGNumberEntry *) gTQSender;
  int NumberEntryID = NumberEntry->WidgetId();

  TheInterface->SaveSettings();

  switch(NumberEntryID){

  case SpectrumBackgroundIterations_NEL_ID:
  case SpectrumRangeMin_NEL_ID:
  case SpectrumRangeMax_NEL_ID:
    ComputationMgr->CalculateSpectrumBackground();
    GraphicsMgr->PlotSpectrum();
    break;
    
  case EALightConversionFactor_NEL_ID:{
    double CF = TheInterface->EALightConversionFactor_NEL->GetEntry()->GetNumber();
    InterpolationMgr->SetConversionFactor(CF);
    InterpolationMgr->ConstructResponses();
    TheInterface->HandleTripleSliderPointer();
    break;
  }
    
  case EAErrorWidth_NEL_ID:
    TheInterface->HandleTripleSliderPointer();
    break;

  case EAElectronEnergy_NEL_ID:
  case EAGammaEnergy_NEL_ID:
  case EAProtonEnergy_NEL_ID:
  case EAAlphaEnergy_NEL_ID:
  case EACarbonEnergy_NEL_ID:
    break;
  }
}


void AAAnalysisSlots::HandleRadioButtons()
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;
  
  TGRadioButton *RadioButton = (TGRadioButton *) gTQSender;
  int RadioButtonID = RadioButton->WidgetId();
  
  TheInterface->SaveSettings();

  switch(RadioButtonID){

  case SpectrumWithBackground_RB_ID:
    TheInterface->SpectrumLessBackground_RB->SetState(kButtonUp);
    TheInterface->SaveSettings();
    ComputationMgr->CalculateSpectrumBackground();
    GraphicsMgr->PlotSpectrum();

    if(TheInterface->SpectrumFindIntegral_CB->IsDown()){
      TheInterface->SpectrumFindIntegral_CB->SetState(kButtonUp, true);
      TheInterface->SpectrumFindIntegral_CB->SetState(kButtonDown, true);
    }
    break;

  case SpectrumLessBackground_RB_ID:
    TheInterface->SpectrumWithBackground_RB->SetState(kButtonUp);
    TheInterface->SaveSettings();
    ComputationMgr->CalculateSpectrumBackground();
    GraphicsMgr->PlotSpectrum();

    if(TheInterface->SpectrumFindIntegral_CB->IsDown()){
      TheInterface->SpectrumFindIntegral_CB->SetState(kButtonUp, true);
      TheInterface->SpectrumFindIntegral_CB->SetState(kButtonDown, true);
    }
    break;

  default:
    break;

  }
}
