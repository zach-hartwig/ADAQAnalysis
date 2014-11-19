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
// name: AAProcessing.cc
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAProcessingSlots class contains widget slot methods to
//       handle signals generated from widgets contained on the
//       "processing" tab of the ADAQAnalysis GUI
//
/////////////////////////////////////////////////////////////////////////////////

// ROOT
#include <TGFileDialog.h>

// ADAQAnalysis
#include "AAProcessingSlots.hh"
#include "AAInterface.hh"
#include "AAGraphics.hh"


AAProcessingSlots::AAProcessingSlots(AAInterface *TI)
  : TheInterface(TI)
{  ComputationMgr = AAComputation::GetInstance();
  GraphicsMgr = AAGraphics::GetInstance();
  InterpolationMgr = AAInterpolation::GetInstance();
}


AAProcessingSlots::~AAProcessingSlots()
{;}


void AAProcessingSlots::HandleCheckButtons()
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;
  
  TGCheckButton *CheckButton = (TGCheckButton *) gTQSender;
  int CheckButtonID = CheckButton->WidgetId();

  TheInterface->SaveSettings();

  switch(CheckButtonID){

  case PSDEnable_CB_ID:{
    EButtonState ButtonState = kButtonDisabled;
    bool WidgetState = false;
    
    if(TheInterface->PSDEnable_CB->IsDown()){
      ButtonState = kButtonUp;
      WidgetState = true;
    }

    // Be sure to turn off PSD filtering if the user does not want to
    // discriminate by pulse shape
    else{
      TheInterface->PSDEnableFilter_CB->SetState(kButtonUp);
      if(GraphicsMgr->GetCanvasContentType() == zWaveform)
	GraphicsMgr->PlotWaveform();
    }

    TheInterface->SetPSDWidgetState(WidgetState, ButtonState);

    break;
  }

  case PSDEnableFilterCreation_CB_ID:{
    
    if(TheInterface->PSDEnableFilterCreation_CB->IsDown() and 
       GraphicsMgr->GetCanvasContentType() != zPSDHistogram){
      TheInterface->CreateMessageBox("The canvas does not presently contain a PSD histogram! PSD filter creation is not possible!","Stop");
      TheInterface->PSDEnableFilterCreation_CB->SetState(kButtonUp);
      break;
    }
    break;
  }

  case PSDEnableFilter_CB_ID:{
    if(TheInterface->PSDEnableFilter_CB->IsDown()){
      ComputationMgr->SetUsePSDFilter(TheInterface->PSDChannel_CBL->GetComboBox()->GetSelected(), true);
      TheInterface->FindPeaks_CB->SetState(kButtonDown);
    }
    else
      ComputationMgr->SetUsePSDFilter(TheInterface->PSDChannel_CBL->GetComboBox()->GetSelected(), false);
    break;
  }
    
  case PSDPlotTailIntegration_CB_ID:{
    if(!TheInterface->FindPeaks_CB->IsDown())
      TheInterface->FindPeaks_CB->SetState(kButtonDown);
    GraphicsMgr->PlotWaveform();
    break;
  }

  case PSDEnableHistogramSlicing_CB_ID:{
    if(TheInterface->PSDEnableHistogramSlicing_CB->IsDown()){
      TheInterface->PSDHistogramSliceX_RB->SetState(kButtonUp);
      TheInterface->PSDHistogramSliceY_RB->SetState(kButtonUp);
      
      // Temporary hack ZSH 12 Feb 13
      TheInterface->PSDHistogramSliceX_RB->SetState(kButtonDown);
    }
    else{
      // Disable histogram buttons
      TheInterface->PSDHistogramSliceX_RB->SetState(kButtonDisabled);
      TheInterface->PSDHistogramSliceY_RB->SetState(kButtonDisabled);

      // Replot the PSD histogram
      GraphicsMgr->PlotPSDHistogram();
      
      // Delete the canvas containing the PSD slice histogram and
      // close the window (formerly) containing the canvas
      TCanvas *PSDSlice_C = (TCanvas *)gROOT->GetListOfCanvases()->FindObject("PSDSlice_C");
      if(PSDSlice_C)
	PSDSlice_C->Close();
    }
    break;
  }

case IntegratePearson_CB_ID:{
    EButtonState ButtonState = kButtonDisabled;
    bool WidgetState = false;
    
    if(TheInterface->IntegratePearson_CB->IsDown()){
      // Set states to activate buttons
      ButtonState = kButtonUp;
      WidgetState = true;
    }

    TheInterface->SetPearsonWidgetState(WidgetState, ButtonState);

    if(TheInterface->IntegratePearson_CB->IsDown() and 
       TheInterface->PlotPearsonIntegration_CB->IsDown()){

      GraphicsMgr->PlotWaveform();
      
      double DeuteronsInWaveform = ComputationMgr->GetDeuteronsInWaveform();
      TheInterface->DeuteronsInWaveform_NEFL->GetEntry()->SetNumber(DeuteronsInWaveform);
      
      double DeuteronsInTotal = ComputationMgr->GetDeuteronsInTotal();
      TheInterface->DeuteronsInTotal_NEFL->GetEntry()->SetNumber(DeuteronsInTotal);
    }

    break;
 }
  
  default:
    break;
  }
}


void AAProcessingSlots::HandleComboBoxes(int ComboBoxID, int SelectedID)
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;
  
  TheInterface->SaveSettings();
  
  switch(ComboBoxID){

    // Plotting 2D/3D representations of the PSD histogram can be very
    // time consuming; thus, no automatic plotting enabled at present
  case PSDPlotType_CBL_ID:
    if(ComputationMgr->GetPSDHistogramExists())
      {}
    break;
  }
}


void AAProcessingSlots::HandleNumberEntries()
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;

  TGNumberEntry *NumberEntry = (TGNumberEntry *) gTQSender;
  int NumberEntryID = NumberEntry->WidgetId();

  TheInterface->SaveSettings();
  
  switch(NumberEntryID){

  default:
    break;
  }
}


void AAProcessingSlots::HandleRadioButtons()
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;
  
  TGRadioButton *RadioButton = (TGRadioButton *) gTQSender;
  int RadioButtonID = RadioButton->WidgetId();
  
  TheInterface->SaveSettings();
  
  switch(RadioButtonID){

  case ProcessingSeq_RB_ID:
    if(TheInterface->ProcessingSeq_RB->IsDown())
      TheInterface->ProcessingPar_RB->SetState(kButtonUp);
    
    TheInterface->NumProcessors_NEL->GetEntry()->SetState(false);
    TheInterface->NumProcessors_NEL->GetEntry()->SetNumber(1);
    break;
    
  case ProcessingPar_RB_ID:
    if(TheInterface->ProcessingPar_RB->IsDown())
      TheInterface->ProcessingSeq_RB->SetState(kButtonUp);

    TheInterface->NumProcessors_NEL->GetEntry()->SetState(true);
    TheInterface->NumProcessors_NEL->GetEntry()->SetNumber(TheInterface->NumProcessors);
    break;

  case PSDPositiveFilter_RB_ID:
    if(TheInterface->PSDPositiveFilter_RB->IsDown()){
      TheInterface->PSDNegativeFilter_RB->SetState(kButtonUp);
    }
    break;

  case PSDNegativeFilter_RB_ID:
    if(TheInterface->PSDNegativeFilter_RB->IsDown()){
      TheInterface->PSDPositiveFilter_RB->SetState(kButtonUp);
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

  case IntegrateRawPearson_RB_ID:
    TheInterface->IntegrateFitToPearson_RB->SetState(kButtonUp);
    TheInterface->PearsonMiddleLimit_NEL->GetEntry()->SetState(false);
    GraphicsMgr->PlotWaveform();
    break;

  case IntegrateFitToPearson_RB_ID:
    TheInterface->IntegrateRawPearson_RB->SetState(kButtonUp);
    TheInterface->PearsonMiddleLimit_NEL->GetEntry()->SetState(true);
    GraphicsMgr->PlotWaveform();
    break;

  case PearsonPolarityPositive_RB_ID:
    TheInterface->PearsonPolarityNegative_RB->SetState(kButtonUp);
    GraphicsMgr->PlotWaveform();
    break;

  case PearsonPolarityNegative_RB_ID:
    TheInterface->PearsonPolarityPositive_RB->SetState(kButtonUp);
    GraphicsMgr->PlotWaveform();
    break;
  
  default:
    break;
  }
  TheInterface->SaveSettings();
}


void AAProcessingSlots::HandleTextButtons()
{
  if(!TheInterface->ADAQFileLoaded and !TheInterface->ACRONYMFileLoaded)
    return;
  
  TGTextButton *TextButton = (TGTextButton *) gTQSender;
  int TextButtonID  = TextButton->WidgetId();
  
  TheInterface->SaveSettings();
  
  switch(TextButtonID){

  case PSDCalculate_TB_ID:

    if(TheInterface->ADAQFileLoaded){
      // Sequential processing
      if(TheInterface->ProcessingSeq_RB->IsDown())
	ComputationMgr->CreatePSDHistogram();
      
      // Parallel processing
      else{
	TheInterface->SaveSettings(true);
	ComputationMgr->ProcessWaveformsInParallel("discriminating");
      }
      GraphicsMgr->PlotPSDHistogram();
    }
    else if(TheInterface->ACRONYMFileLoaded)
      TheInterface->CreateMessageBox("ACRONYM files cannot be processed for pulse shape at this time!","Stop");
    
    break;

  case PSDClearFilter_TB_ID:

    if(TheInterface->ADAQFileLoaded){

      ComputationMgr->ClearPSDFilter(TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected());
      TheInterface->PSDEnableFilter_CB->SetState(kButtonUp);
      
      switch(GraphicsMgr->GetCanvasContentType()){
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

  case CountRate_TB_ID:
    if(TheInterface->ADAQFileLoaded)
      ComputationMgr->CalculateCountRate();
    break;

  case DesplicedFileSelection_TB_ID:{

    const char *FileTypes[] = {"ADAQ-formatted ROOT file", "*.root",
			       "All files",                "*",
			       0, 0};
    
    // Use the presently open ADAQ ROOT file name as the basis for the
    // default despliced ADAQ ROOT file name presented to the user in
    // the TGFileDialog. I have begun to denote standard ADAQ ROOT
    // files with ".root" extension and despliced ADAQ ROOT files with
    // ".ds.root" extension. 
    string InitialFileName;
    size_t Pos = TheInterface->ADAQFileName.find_last_of("/");
    if(Pos != string::npos){
      string RawFileName = TheInterface->ADAQFileName.substr(Pos+1,
							     TheInterface->ADAQFileName.size());

      Pos = RawFileName.find_last_of(".");
      if(Pos != string::npos)
	InitialFileName = RawFileName.substr(0,Pos) + ".ds.root";
    }

    TGFileInfo FileInformation;
    FileInformation.fFileTypes = FileTypes;
    FileInformation.fFilename = StrDup(InitialFileName.c_str());
    FileInformation.fIniDir = StrDup(TheInterface->DesplicedDirectory.c_str());
    new TGFileDialog(gClient->GetRoot(), TheInterface, kFDOpen, &FileInformation);
    
    if(FileInformation.fFilename==NULL)
      TheInterface->CreateMessageBox("A file was not selected so the data will not be saved!\nSelect a valid file to save the despliced waveforms","Stop");
    else{
      string DesplicedFileName, DesplicedFileExtension;
      size_t Found = string::npos;
      
      // Get the name for the despliced file. Note that
      // FileInformation.fFilename contains the absolute path to the
      // despliced file location
      DesplicedFileName = FileInformation.fFilename;
      
      // Strip the data file name off the absolute file path and set
      // the path to the DataDirectory variable. Thus, the "current"
      // directory from which a file was selected will become the new
      // default directory that automically opens
      Found = DesplicedFileName.find_last_of("/");
      if(Found != string::npos)
	TheInterface->DesplicedDirectory = DesplicedFileName.substr(0, Found);
      
      Found = DesplicedFileName.find_last_of(".");
      if(Found != string::npos)
	DesplicedFileName = DesplicedFileName.substr(0, Found);
     
      DesplicedFileExtension = FileInformation.fFileTypes[FileInformation.fFileTypeIdx+1];
      Found = DesplicedFileExtension.find_last_of("*");
      if(Found != string::npos)
	DesplicedFileExtension = DesplicedFileExtension.substr(Found+1, DesplicedFileExtension.size());

      string DesplicedFile = DesplicedFileName + DesplicedFileExtension;

      TheInterface->DesplicedFileName_TE->SetText(DesplicedFile.c_str());
    }
    break;
  }
    
  case DesplicedFileCreation_TB_ID:
    // Alert the user the filtering particles by PSD into the spectra
    // requires integration type peak finder to be used
    if(ComputationMgr->GetUsePSDFilters()[TheInterface->ADAQSettings->PSDChannel] and 
       TheInterface->ADAQSettings->ADAQSpectrumIntTypeWW)
      TheInterface->CreateMessageBox("Warning! Use of the PSD filter with spectra creation requires peak finding integration","Asterisk");

    if(TheInterface->ACRONYMFileLoaded){
      TheInterface->CreateMessageBox("Error! ACRONYM files cannot be despliced!","Stop");
      break;
    }
    
    // Sequential processing
    if(TheInterface->ProcessingSeq_RB->IsDown())
      ComputationMgr->CreateDesplicedFile();
    
    // Parallel processing
    else{
      if(TheInterface->ADAQFileLoaded){
	TheInterface->SaveSettings(true);
	ComputationMgr->ProcessWaveformsInParallel("desplicing");
      }
    }
    break;

  default:
    break;
  }
}
