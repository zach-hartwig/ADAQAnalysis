#include "AAWaveformSlots.hh"
#include "AAInterface.hh"
#include "AAGraphics.hh"

AAWaveformSlots::AAWaveformSlots(AAInterface *TI)
  : TheInterface(TI)
{;}

AAWaveformSlots::~AAWaveformSlots()
{;}


void AAWaveformSlots::HandleCheckButtons()
{;}


void AAWaveformSlots::HandleComboBoxes()
{;}


void AAWaveformSlots::HandleNumberEntries()
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;

  TheInterface->SaveSettings();

    // Get the widget and ID from which the signal was sent
  TGNumberEntry *NumberEntry = (TGNumberEntry *) gTQSender;
  int NumberEntryID = NumberEntry->WidgetId();

  switch(NumberEntryID){
  case WaveformSelector_NEL_ID:{
    int Num = TheInterface->WaveformSelector_NEL->GetEntry()->GetIntNumber();
    TheInterface->WaveformSelector_HS->SetPosition(Num);
    
    AAGraphics::GetInstance()->PlotWaveform();

    break;
  }

  default:
    break;
  }
}


void AAWaveformSlots::HandleRadioButtons()
{;}
