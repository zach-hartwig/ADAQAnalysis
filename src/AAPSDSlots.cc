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
// name: AAPSDSlots.cc
// date: 10 Apr 16
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAPSDSlots class contains widget slot methods to handle
//       signals generated from widgets contained on the "PSD" tab of
//       the ADAQAnalysis GUI
//
/////////////////////////////////////////////////////////////////////////////////

// ROOT
#include <TGFileDialog.h>
#include <TList.h>
#include <TStyle.h>

// C++
#include <sstream>

// ADAQAnalysis
#include "AAPSDSlots.hh"
#include "AANontabSlots.hh"
#include "AAInterface.hh"
#include "AAGraphics.hh"


AAPSDSlots::AAPSDSlots(AAInterface *TI)
  : TheInterface(TI)
{
  ComputationMgr = AAComputation::GetInstance();
  GraphicsMgr = AAGraphics::GetInstance();
}


AAPSDSlots::~AAPSDSlots()
{;}


void AAPSDSlots::HandleCheckButtons()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGCheckButton *CheckButton = (TGCheckButton *) gTQSender;
  int CheckButtonID = CheckButton->WidgetId();

  TheInterface->SaveSettings();
  
  switch(CheckButtonID){
    
  case PSDEnableRegionCreation_CB_ID:{
    
    if(TheInterface->PSDEnableRegionCreation_CB->IsDown()){
      
      // No PSD histogram is presently available
      if(GraphicsMgr->GetCanvasContentType() != zPSDHistogram){
	TheInterface->CreateMessageBox("The canvas does not presently contain a PSD histogram! PSD filter creation is not possible!","Stop");
	TheInterface->PSDEnableRegionCreation_CB->SetState(kButtonUp);
      }

      // Enable necessary widgets
      else{
	TheInterface->PSDCreateRegion_TB->SetState(kButtonUp);
	TheInterface->PSDClearRegion_TB->SetState(kButtonUp);
      }
    }
    else{
      TheInterface->PSDCreateRegion_TB->SetState(kButtonDisabled);
      TheInterface->PSDClearRegion_TB->SetState(kButtonDisabled);
    }
    break;
  }
    
  case PSDEnableRegion_CB_ID:{
    if(TheInterface->PSDEnableRegion_CB->IsDown()){
      ComputationMgr->SetUsePSDRegions(TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected(), true);
      TheInterface->FindPeaks_CB->SetState(kButtonDown);
    }
    else
      ComputationMgr->SetUsePSDRegions(TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected(), false);
    break;
  }
    
  case PSDPlotIntegrationLimits_CB_ID:{
    if(TheInterface->PSDPlotIntegrationLimits_CB->IsDown())
      GraphicsMgr->PlotWaveform();
    else if(ComputationMgr->GetPSDHistogramExists())
      GraphicsMgr->PlotPSDHistogram();
    break;
  }

  case PSDEnableHistogramSlicing_CB_ID:{
    
    if(TheInterface->PSDEnableHistogramSlicing_CB->IsDown() and 
       GraphicsMgr->GetCanvasContentType() != zPSDHistogram){
      TheInterface->CreateMessageBox("The canvas does not presently contain a PSD histogram! PSD filter creation is not possible!","Stop");
      TheInterface->PSDEnableHistogramSlicing_CB->SetState(kButtonUp);
    }
    else{
      if(TheInterface->PSDEnableHistogramSlicing_CB->IsDown()){
	// Enable PSD histogram slicing buttons
	TheInterface->PSDHistogramSliceX_RB->SetState(kButtonUp);
	TheInterface->PSDHistogramSliceY_RB->SetState(kButtonUp);
	TheInterface->PSDCalculateFOM_CB->SetState(kButtonUp);
	TheInterface->PSDLowerFOMFitMin_NEL->GetEntry()->SetState(true);
	TheInterface->PSDLowerFOMFitMax_NEL->GetEntry()->SetState(true);
	TheInterface->PSDUpperFOMFitMin_NEL->GetEntry()->SetState(true);
	TheInterface->PSDUpperFOMFitMax_NEL->GetEntry()->SetState(true);
	TheInterface->PSDFigureOfMerit_NEFL->GetEntry()->SetState(true);
	
	// Temporary hack ZSH 12 Feb 13
	TheInterface->PSDHistogramSliceX_RB->SetState(kButtonDown);
      }
      else{
	// Disable PSD histogram slicing buttons
	TheInterface->PSDHistogramSliceX_RB->SetState(kButtonDisabled);
	TheInterface->PSDHistogramSliceY_RB->SetState(kButtonDisabled);
	TheInterface->PSDCalculateFOM_CB->SetState(kButtonDisabled);
	TheInterface->PSDLowerFOMFitMin_NEL->GetEntry()->SetState(false);
	TheInterface->PSDLowerFOMFitMax_NEL->GetEntry()->SetState(false);
	TheInterface->PSDUpperFOMFitMin_NEL->GetEntry()->SetState(false);
	TheInterface->PSDUpperFOMFitMax_NEL->GetEntry()->SetState(false);
	TheInterface->PSDFigureOfMerit_NEFL->GetEntry()->SetState(false);
	
	// Replot the PSD histogram
	GraphicsMgr->PlotPSDHistogram();
	
	// Delete the canvas containing the PSD slice histogram and
	// close the window (formerly) containing the canvas
	TCanvas *PSDSlice_C = (TCanvas *)gROOT->GetListOfCanvases()->FindObject("PSDSlice_C");
	if(PSDSlice_C)
	  PSDSlice_C->Close();
      }
    }
    break;
  }
  }
}
  

void AAPSDSlots::HandleComboBoxes(int ComboBoxID, int SelectedID)
{
  if(!TheInterface->EnableInterface)
    return;

  TheInterface->SaveSettings();

  switch(ComboBoxID){
    
  case PSDPlotType_CBL_ID:
    if(ComputationMgr->GetPSDHistogramExists())
      GraphicsMgr->PlotPSDHistogram();
    break;
    
  case PSDPlotPalette_CBL_ID:
    gStyle->SetPalette(SelectedID);
    if(ComputationMgr->GetPSDHistogramExists())
      GraphicsMgr->PlotPSDHistogram();
    break;
  }
}


void AAPSDSlots::HandleNumberEntries()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGNumberEntry *NumberEntry = (TGNumberEntry *) gTQSender;
  int NumberEntryID = NumberEntry->WidgetId();
  
  TheInterface->SaveSettings();

  switch(NumberEntryID){
    
  case PSDTotalStart_NEL_ID:
  case PSDTotalStop_NEL_ID:
  case PSDTailStart_NEL_ID:
  case PSDTailStop_NEL_ID:
    if(TheInterface->PSDPlotIntegrationLimits_CB->IsDown())
      GraphicsMgr->PlotWaveform();
    break;

  case PSDLowerFOMFitMin_NEL_ID:
  case PSDLowerFOMFitMax_NEL_ID:
  case PSDUpperFOMFitMin_NEL_ID:
  case PSDUpperFOMFitMax_NEL_ID:
    break;
  }
}


void AAPSDSlots::HandleRadioButtons()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGRadioButton *RadioButton = (TGRadioButton *) gTQSender;
  int RadioButtonID = RadioButton->WidgetId();
  
  TheInterface->SaveSettings();

  switch(RadioButtonID){
    
  case PSDXAxisADC_RB_ID:
    if(TheInterface->PSDXAxisADC_RB->IsDown()){
      TheInterface->PSDXAxisEnergy_RB->SetState(kButtonUp);
    }
    break;
    
  case PSDXAxisEnergy_RB_ID:
    if(TheInterface->PSDXAxisEnergy_RB->IsDown()){
      TheInterface->PSDXAxisADC_RB->SetState(kButtonUp);
    }
    break;
    
  case PSDYAxisTail_RB_ID:
    if(TheInterface->PSDYAxisTail_RB->IsDown()){
      TheInterface->PSDYAxisTailTotal_RB->SetState(kButtonUp);
    }
    break;

  case PSDYAxisTailTotal_RB_ID:
    if(TheInterface->PSDYAxisTailTotal_RB->IsDown()){
      TheInterface->PSDYAxisTail_RB->SetState(kButtonUp);
    }
    break;

  case PSDAlgorithmPF_RB_ID:
    if(TheInterface->PSDAlgorithmPF_RB->IsDown()){
      TheInterface->PSDAlgorithmSMS_RB->SetState(kButtonUp);
      TheInterface->PSDAlgorithmWD_RB->SetState(kButtonUp);
      TheInterface->SaveSettings();
    }
    break;
    
  case PSDAlgorithmSMS_RB_ID:
    if(TheInterface->PSDAlgorithmSMS_RB->IsDown()){
      TheInterface->PSDAlgorithmPF_RB->SetState(kButtonUp);
      TheInterface->PSDAlgorithmWD_RB->SetState(kButtonUp);
      TheInterface->SaveSettings();
    }
    break;
    
  case PSDAlgorithmWD_RB_ID:
    if(TheInterface->PSDAlgorithmWD_RB->IsDown()){
      TheInterface->PSDAlgorithmSMS_RB->SetState(kButtonUp);
      TheInterface->PSDAlgorithmPF_RB->SetState(kButtonUp);
      TheInterface->SaveSettings();
    }
    break;

  case PSDInsideRegion_RB_ID:
    if(TheInterface->PSDInsideRegion_RB->IsDown()){
      TheInterface->PSDOutsideRegion_RB->SetState(kButtonUp);
    }
    break;

  case PSDOutsideRegion_RB_ID:
    if(TheInterface->PSDOutsideRegion_RB->IsDown()){
      TheInterface->PSDInsideRegion_RB->SetState(kButtonUp);
    }
    break;

  case PSDHistogramSliceX_RB_ID:
    if(TheInterface->PSDHistogramSliceX_RB->IsDown())
      TheInterface->PSDHistogramSliceY_RB->SetState(kButtonUp);
    break;
    
  case PSDHistogramSliceY_RB_ID:
    if(TheInterface->PSDHistogramSliceY_RB->IsDown())
      TheInterface->PSDHistogramSliceX_RB->SetState(kButtonUp);
    break;
  }
}


void AAPSDSlots::HandleTextButtons()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGTextButton *TextButton = (TGTextButton *) gTQSender;
  int TextButtonID  = TextButton->WidgetId();
  
  TheInterface->SaveSettings();
  
  switch(TextButtonID){

  case ProcessPSDHistogram_TB_ID:
    
    if(TheInterface->ADAQFileLoaded){
      
      // Sequential waveform processing
      if(TheInterface->ProcessingSeq_RB->IsDown()){
	
	if(TheInterface->ADAQFileLoaded)
	  ComputationMgr->ProcessPSDHistogramWaveforms();
	
	if(ComputationMgr->GetPSDHistogramExists())
	  GraphicsMgr->PlotPSDHistogram();
      }
      
      // Parallel waveform processing
      else{
	TheInterface->SaveSettings(true);
	
	if(TheInterface->PSDAlgorithmWD_RB->IsDown())
	  TheInterface->CreateMessageBox("Error! Waveform data can only be processed sequentially!\n","Stop");
	else
	  ComputationMgr->ProcessWaveformsInParallel("discriminating");
      }
      GraphicsMgr->PlotPSDHistogram();
    }
    else if(TheInterface->ASIMFileLoaded)
      TheInterface->CreateMessageBox("ASIM files cannot be processed for pulse shape at this time!","Stop");

    TheInterface->UpdateForPSDHistogramCreation();
    
    break;


  case CreatePSDHistogram_TB_ID:
    
    if(TheInterface->ADAQFileLoaded)
      ComputationMgr->CreatePSDHistogram();
    else
      TheInterface->CreateMessageBox("ASIM files cannot be processed for PSD at this time!","Stop");
    
    if(ComputationMgr->GetPSDHistogramExists())
      GraphicsMgr->PlotPSDHistogram();
    
    break;
    

  case PSDCreateRegion_TB_ID:

    if(TheInterface->ADAQFileLoaded){
      ComputationMgr->CreatePSDRegion();
      GraphicsMgr->PlotPSDRegion();
      TheInterface->PSDEnableRegionCreation_CB->SetState(kButtonUp);
      
      TheInterface->PSDCreateRegion_TB->SetText("Region created!");
      TheInterface->PSDCreateRegion_TB->SetForegroundColor(TheInterface->ColorMgr->Number2Pixel(0));
      TheInterface->PSDCreateRegion_TB->SetBackgroundColor(TheInterface->ColorMgr->Number2Pixel(32));						   

      TheInterface->PSDEnableRegion_CB->SetState(kButtonUp);
      TheInterface->PSDInsideRegion_RB->SetState(kButtonUp);
      TheInterface->PSDOutsideRegion_RB->SetState(kButtonUp);
   }
    break;
    
  case PSDClearRegion_TB_ID:
    
    if(TheInterface->ADAQFileLoaded){
      
      ComputationMgr->ClearPSDRegion();
      TheInterface->PSDEnableRegion_CB->SetState(kButtonUp);
      TheInterface->PSDEnableRegion_CB->SetState(kButtonDisabled);
      TheInterface->PSDInsideRegion_RB->SetState(kButtonDisabled);
      TheInterface->PSDOutsideRegion_RB->SetState(kButtonDisabled);
      
      TheInterface->PSDCreateRegion_TB->SetText("Create PSD region");
      TheInterface->PSDCreateRegion_TB->SetForegroundColor(TheInterface->ColorMgr->Number2Pixel(1));
      TheInterface->PSDCreateRegion_TB->SetBackgroundColor(TheInterface->ColorMgr->Number2Pixel(18));						   
      
      switch((int)GraphicsMgr->GetCanvasContentType()){
      case zWaveform:
	GraphicsMgr->PlotWaveform();
	break;
	
      case zSpectrum:
	GraphicsMgr->PlotSpectrum();
	break;
	
      case zPSDHistogram:
	GraphicsMgr->PlotPSDHistogram();
	break;
      }
    }
    break;
  }
}
