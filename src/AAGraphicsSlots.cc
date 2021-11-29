/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                           Copyright (C) 2012-2021                           //
//                 Zachary Seth Hartwig : All rights reserved                  //
//                                                                             //
//      The ADAQAnalysis source code is licensed under the GNU GPL v3.0.       //
//      You have the right to modify and/or redistribute this source code      //      
//      under the terms specified in the license, which may be found online    //
//      at http://www.gnu.org/licenses or at $ADAQANALYSIS/License.txt.       //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// 
// name: AAGraphicsSlots.cc
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAGraphicsSlots class contains widget slot methods to
//       handle signals generated from widgets contained on the
//       "graphics" tab of the ADAQAnalysis GUI
//
/////////////////////////////////////////////////////////////////////////////////

#include <TGClient.h>

#include "AAGraphicsSlots.hh"
#include "AAInterface.hh"
#include "AAGraphics.hh"


AAGraphicsSlots::AAGraphicsSlots(AAInterface *TI)
  : TheInterface(TI)
{
  ComputationMgr = AAComputation::GetInstance();
  GraphicsMgr = AAGraphics::GetInstance();
  InterpolationMgr = AAInterpolation::GetInstance();
}


AAGraphicsSlots::~AAGraphicsSlots()
{;}


void AAGraphicsSlots::HandleCheckButtons()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGCheckButton *CheckButton = (TGCheckButton *) gTQSender;
  int CheckButtonID = CheckButton->WidgetId();

  TheInterface->SaveSettings();

  switch(CheckButtonID){

  case HistogramStats_CB_ID:
  case CanvasGrid_CB_ID:
  case OverrideTitles_CB_ID:
  case CanvasXAxisLog_CB_ID:
  case CanvasYAxisLog_CB_ID:
  case CanvasZAxisLog_CB_ID:

    switch(GraphicsMgr->GetCanvasContentType()){
    case zEmpty:
      break;
      
    case zWaveform:
      if(!TheInterface->ADAQFileLoaded)
	break;
      GraphicsMgr->PlotWaveform();
      break;
      
    case zSpectrum:{
      GraphicsMgr->PlotSpectrum();
      break;
    }
      
    case zSpectrumDerivative:
      GraphicsMgr->PlotSpectrumDerivative();
      break;
      
    case zPSDHistogram:
      GraphicsMgr->PlotPSDHistogram();
      break;
    }
    break;
    
  case PlotSpectrumDerivativeError_CB_ID:
  case PlotAbsValueSpectrumDerivative_CB_ID:
    if(!ComputationMgr->GetSpectrumExists()){
      TheInterface->CreateMessageBox("A valid spectrum does not yet exists! The calculation of a spectrum derivative is, therefore, moot!", "Stop");
      break;
    }
    else
      if(GraphicsMgr->GetCanvasContentType() == zSpectrumDerivative)
	GraphicsMgr->PlotSpectrumDerivative();
    
    break;
    
  default:
    break;
  }
}


void AAGraphicsSlots::HandleNumberEntries()
{
  if(!TheInterface->EnableInterface)
    return;

  TGNumberEntry *NumberEntry = (TGNumberEntry *) gTQSender;
  int NumberEntryID = NumberEntry->WidgetId();

  TheInterface->SaveSettings();
  
  switch(NumberEntryID){

  case WaveformLineWidth_NEL_ID:
  case WaveformMarkerSize_NEL_ID:
  case SpectrumLineWidth_NEL_ID:
  case SpectrumFillStyle_NEL_ID:
  case XAxisSize_NEL_ID:
  case XAxisOffset_NEL_ID:
  case XAxisDivs_NEL_ID:
  case YAxisSize_NEL_ID:
  case YAxisOffset_NEL_ID:
  case YAxisDivs_NEL_ID:
  case ZAxisSize_NEL_ID:
  case ZAxisOffset_NEL_ID:
  case ZAxisDivs_NEL_ID:
  case PaletteAxisSize_NEL_ID:
  case PaletteAxisOffset_NEL_ID:
  case PaletteAxisDivs_NEL_ID:  
  case PaletteX1_NEL_ID:  
  case PaletteX2_NEL_ID:  
  case PaletteY1_NEL_ID:  
  case PaletteY2_NEL_ID:  
    
    switch((int)GraphicsMgr->GetCanvasContentType()){
    case zWaveform:
      GraphicsMgr->PlotWaveform();
      break;
      
    case zSpectrum:
      GraphicsMgr->PlotSpectrum();
      break;

    case zSpectrumDerivative:
      GraphicsMgr->PlotSpectrumDerivative();
      break;
      
    case zPSDHistogram:
      GraphicsMgr->PlotPSDHistogram();
      break;
    }
    break;

  default:
    break;
  }
}


void AAGraphicsSlots::HandleRadioButtons()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGRadioButton *RadioButton = (TGRadioButton *) gTQSender;
  int RadioButtonID = RadioButton->WidgetId();
  
  TheInterface->SaveSettings();

  switch(RadioButtonID){

  case DrawWaveformWithLine_RB_ID:
    if(!TheInterface->ADAQFileLoaded)
      break;
    TheInterface->DrawWaveformWithCurve_RB->SetState(kButtonUp);
    TheInterface->DrawWaveformWithMarkers_RB->SetState(kButtonUp);
    TheInterface->DrawWaveformWithBoth_RB->SetState(kButtonUp);
    TheInterface->SaveSettings();
    GraphicsMgr->PlotWaveform();
    break;

  case DrawWaveformWithCurve_RB_ID:
    if(!TheInterface->ADAQFileLoaded)
      break;
    TheInterface->DrawWaveformWithLine_RB->SetState(kButtonUp);
    TheInterface->DrawWaveformWithMarkers_RB->SetState(kButtonUp);
    TheInterface->DrawWaveformWithBoth_RB->SetState(kButtonUp);
    TheInterface->SaveSettings();
    GraphicsMgr->PlotWaveform();
    break;

  case DrawWaveformWithMarkers_RB_ID:
    if(!TheInterface->ADAQFileLoaded)
      break;
    TheInterface->DrawWaveformWithLine_RB->SetState(kButtonUp);
    TheInterface->DrawWaveformWithCurve_RB->SetState(kButtonUp);
    TheInterface->DrawWaveformWithBoth_RB->SetState(kButtonUp);
    TheInterface->SaveSettings();
    GraphicsMgr->PlotWaveform();
    break;

  case DrawWaveformWithBoth_RB_ID:
    if(!TheInterface->ADAQFileLoaded)
      break;
    TheInterface->DrawWaveformWithLine_RB->SetState(kButtonUp);
    TheInterface->DrawWaveformWithCurve_RB->SetState(kButtonUp);
    TheInterface->DrawWaveformWithMarkers_RB->SetState(kButtonUp);
    TheInterface->SaveSettings();
    GraphicsMgr->PlotWaveform();
    break;

  case DrawSpectrumWithBars_RB_ID:
    TheInterface->DrawSpectrumWithCurve_RB->SetState(kButtonUp);
    TheInterface->DrawSpectrumWithError_RB->SetState(kButtonUp);
    TheInterface->DrawSpectrumWithLine_RB->SetState(kButtonUp);
    TheInterface->SaveSettings();
    if(ComputationMgr->GetSpectrumExists())
      GraphicsMgr->PlotSpectrum();
    break;

  case DrawSpectrumWithCurve_RB_ID:
    TheInterface->DrawSpectrumWithBars_RB->SetState(kButtonUp);
    TheInterface->DrawSpectrumWithError_RB->SetState(kButtonUp);
    TheInterface->DrawSpectrumWithLine_RB->SetState(kButtonUp);
    TheInterface->SaveSettings();
    if(ComputationMgr->GetSpectrumExists())
      GraphicsMgr->PlotSpectrum();
    break;

  case DrawSpectrumWithError_RB_ID:
    TheInterface->DrawSpectrumWithCurve_RB->SetState(kButtonUp);
    TheInterface->DrawSpectrumWithBars_RB->SetState(kButtonUp);
    TheInterface->DrawSpectrumWithLine_RB->SetState(kButtonUp);
    TheInterface->SaveSettings();
    if(ComputationMgr->GetSpectrumExists())
      GraphicsMgr->PlotSpectrum();
    break;

  case DrawSpectrumWithLine_RB_ID:
    TheInterface->DrawSpectrumWithCurve_RB->SetState(kButtonUp);
    TheInterface->DrawSpectrumWithError_RB->SetState(kButtonUp);
    TheInterface->DrawSpectrumWithBars_RB->SetState(kButtonUp);
    TheInterface->SaveSettings();
    if(ComputationMgr->GetSpectrumExists())
      GraphicsMgr->PlotSpectrum();
    break;

  default:
    break;
  }
}


void AAGraphicsSlots::HandleTextButtons()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGTextButton *TextButton = (TGTextButton *) gTQSender;
  int TextButtonID  = TextButton->WidgetId();
  
  TheInterface->SaveSettings();
  
  switch(TextButtonID){

  case WaveformColor_TB_ID:{
    
    if(!TheInterface->ADAQFileLoaded)
      break;
    
    GraphicsMgr->SetWaveformColor();
    int Color = GraphicsMgr->GetWaveformColor();
    TheInterface->WaveformColor_TB->SetBackgroundColor(TheInterface->ColorMgr->Number2Pixel(Color));
    GraphicsMgr->PlotWaveform();
    break;
  }

  case SpectrumLineColor_TB_ID:{
    GraphicsMgr->SetSpectrumLineColor();
    int Color = GraphicsMgr->GetSpectrumLineColor();
    TheInterface->SpectrumLineColor_TB->SetBackgroundColor(TheInterface->ColorMgr->Number2Pixel(Color));
    if(ComputationMgr->GetSpectrumExists())
      GraphicsMgr->PlotSpectrum();
    break;
  }

  case SpectrumFillColor_TB_ID:{
    GraphicsMgr->SetSpectrumFillColor();
    int Color = GraphicsMgr->GetSpectrumFillColor();
    TheInterface->SpectrumFillColor_TB->SetBackgroundColor(TheInterface->ColorMgr->Number2Pixel(Color));
    if(ComputationMgr->GetSpectrumExists())
      GraphicsMgr->PlotSpectrum();
    break;
  }

  case ReplotWaveform_TB_ID:
    
    if(!TheInterface->ADAQFileLoaded)
      break;
    
    GraphicsMgr->PlotWaveform();
    break;
    
  case ReplotSpectrum_TB_ID:
    if(ComputationMgr->GetSpectrumExists())
      GraphicsMgr->PlotSpectrum();
    else
      TheInterface->CreateMessageBox("A valid spectrum does not yet exist; therefore, it is difficult to replot it!","Stop");
    break;
    
  case ReplotSpectrumDerivative_TB_ID:
    if(ComputationMgr->GetSpectrumExists()){
      GraphicsMgr->PlotSpectrumDerivative();
    }
    else
      TheInterface->CreateMessageBox("A valid spectrum does not yet exist; therefore, the spectrum derivative cannot be plotted!","Stop");
    break;
    
  case ReplotPSDHistogram_TB_ID:
    if(TheInterface->ADAQFileLoaded){
      if(ComputationMgr->GetPSDHistogramExists())
	GraphicsMgr->PlotPSDHistogram();
      else
	TheInterface->CreateMessageBox("A valid PSD histogram does not yet exist; therefore, replotting cannot be achieved!","Stop");
    }
    break;

  default:
    break;
  }
}

