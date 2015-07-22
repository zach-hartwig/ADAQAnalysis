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
// name: AASpectrumSlots.cc
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AASpectrumSlots class contains widget slot methods to
//       handle signals generated from widgets contained on the
//       "spectrum" tab of the ADAQAnalysis GUI
//
/////////////////////////////////////////////////////////////////////////////////

// ROOT
#include <TGFileDialog.h>
#include <TList.h>

// C++
#include <sstream>

// ADAQAnalysis
#include "AASpectrumSlots.hh"
#include "AANontabSlots.hh"
#include "AAInterface.hh"
#include "AAGraphics.hh"


AASpectrumSlots::AASpectrumSlots(AAInterface *TI)
  : TheInterface(TI)
{
  ComputationMgr = AAComputation::GetInstance();
  GraphicsMgr = AAGraphics::GetInstance();
}


AASpectrumSlots::~AASpectrumSlots()
{;}


void AASpectrumSlots::HandleCheckButtons()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGCheckButton *CheckButton = (TGCheckButton *) gTQSender;
  int CheckButtonID = CheckButton->WidgetId();

  TheInterface->SaveSettings();
  
  switch(CheckButtonID){

  case SpectrumCalibration_CB_ID:{
    
    if(TheInterface->SpectrumCalibration_CB->IsDown()){
      TheInterface->SetCalibrationWidgetState(true, kButtonUp);
      TheInterface->NontabSlots->HandleTripleSliderPointer();
    }
    else{
      TheInterface->SetCalibrationWidgetState(false, kButtonDisabled);
    }
    break;
  }
  }
}


void AASpectrumSlots::HandleComboBoxes(int ComboBoxID, int SelectedID)
{
  if(!TheInterface->EnableInterface)
    return;

  TheInterface->SaveSettings();

  switch(ComboBoxID){

  case ASIMEventTree_CB_ID:{
    
    TList *ASIMEventTreeList = ComputationMgr->GetASIMEventTreeList();
    TString EventTreeName = TheInterface->ASIMEventTree_CB->GetSelectedEntry()->GetTitle();
    TTree *EventTree = (TTree *)ASIMEventTreeList->FindObject(EventTreeName);
    if(EventTree == NULL)
      return;
    
    TheInterface->WaveformsToHistogram_NEL->GetEntry()->SetNumber(EventTree->GetEntries());
    TheInterface->WaveformsToHistogram_NEL->GetEntry()->SetLimitValues(0, EventTree->GetEntries());
    break;
  }

    // The user can obtain the values used for each calibration point
    // by selecting the calibration point with the combo box
  case SpectrumCalibrationPoint_CBL_ID:{

    const int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();

    // Get the vector<TGraph *> of spectra calibrations from the
    // ComputationMgr (if they are set for this channel) and set the
    // {Energy,PulseUnit} value to the number entry displays
    if(ComputationMgr->GetUseSpectraCalibrations()[Channel]){
      vector<TGraph *> Calibrations = ComputationMgr->GetSpectraCalibrations();

      double *PulseUnit = Calibrations[Channel]->GetX();
      double *Energy = Calibrations[Channel]->GetY();

      // User selected present point ready for entry (not yet set)
      if(SelectedID+1 == TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->GetNumberOfEntries()){
	TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(0.0);
	TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(1.0);
      }
      
      // User selected calibration point (already set)
      else{
	TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(Energy[SelectedID]);
	TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(PulseUnit[SelectedID]);

	// Update the position of the calibration line
	TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->ValueSet(0);
      }
    }
    break;
  }

  default:
    break;
  }
}


void AASpectrumSlots::HandleNumberEntries()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGNumberEntry *NumberEntry = (TGNumberEntry *) gTQSender;
  int NumberEntryID = NumberEntry->WidgetId();
  
  TheInterface->SaveSettings();

  switch(NumberEntryID){

  case SpectrumMinBin_NEL_ID:
  case SpectrumMaxBin_NEL_ID:
    
    TheInterface->SpectrumAnalysisLowerLimit_NEL->GetEntry()->
      SetNumber(TheInterface->SpectrumMinBin_NEL->GetEntry()->GetNumber());
    
    TheInterface->SpectrumAnalysisUpperLimit_NEL->GetEntry()->
      SetNumber(TheInterface->SpectrumMaxBin_NEL->GetEntry()->GetNumber());
    
    break;
    
    
  case SpectrumCalibrationEnergy_NEL_ID:
  case SpectrumCalibrationPulseUnit_NEL_ID:{

    // Get the value set in the PulseUnit number entry
    double Value = 0.;
    if(SpectrumCalibrationEnergy_NEL_ID == NumberEntryID)
      Value = TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->GetNumber();
    else
      Value = TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->GetNumber();
    
    // Draw the new calibration line value ontop of the spectrum
    GraphicsMgr->PlotVCalibrationLine(Value);
    break;
  }
    
  default:
    break;
  }
}


void AASpectrumSlots::HandleRadioButtons()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGRadioButton *RadioButton = (TGRadioButton *) gTQSender;
  int RadioButtonID = RadioButton->WidgetId();
  
  TheInterface->SaveSettings();

  switch(RadioButtonID){

  case ADAQSpectrumTypePAS_RB_ID:
    if(RadioButton->IsDown())
      TheInterface->ADAQSpectrumTypePHS_RB->SetState(kButtonUp);
    break;

  case ADAQSpectrumTypePHS_RB_ID:
    if(RadioButton->IsDown())
      TheInterface->ADAQSpectrumTypePAS_RB->SetState(kButtonUp);
    break;

  case ADAQSpectrumAlgorithmSMS_RB_ID:
    if(RadioButton->IsDown()){
      TheInterface->ADAQSpectrumAlgorithmPF_RB->SetState(kButtonUp);
      TheInterface->ADAQSpectrumAlgorithmWD_RB->SetState(kButtonUp);
    }
    break;

  case ADAQSpectrumAlgorithmPF_RB_ID:
    if(RadioButton->IsDown()){
      TheInterface->ADAQSpectrumAlgorithmSMS_RB->SetState(kButtonUp);
      TheInterface->ADAQSpectrumAlgorithmWD_RB->SetState(kButtonUp);
    }
    break;

  case ADAQSpectrumAlgorithmWD_RB_ID:
    if(RadioButton->IsDown()){
      TheInterface->ADAQSpectrumAlgorithmPF_RB->SetState(kButtonUp);
      TheInterface->ADAQSpectrumAlgorithmSMS_RB->SetState(kButtonUp);
    }
    break;

  case ASIMSpectrumTypeEnergy_RB_ID:
    if(RadioButton->IsDown()){
      TheInterface->ASIMSpectrumTypePhotonsCreated_RB->SetState(kButtonUp);
      TheInterface->ASIMSpectrumTypePhotonsDetected_RB->SetState(kButtonUp);
    }
    break;

  case ASIMSpectrumTypePhotonsCreated_RB_ID:
    if(RadioButton->IsDown()){
      TheInterface->ASIMSpectrumTypeEnergy_RB->SetState(kButtonUp);
      TheInterface->ASIMSpectrumTypePhotonsDetected_RB->SetState(kButtonUp);
    }
    break;

  case ASIMSpectrumTypePhotonsDetected_RB_ID:
    if(RadioButton->IsDown()){
      TheInterface->ASIMSpectrumTypeEnergy_RB->SetState(kButtonUp);
      TheInterface->ASIMSpectrumTypePhotonsCreated_RB->SetState(kButtonUp);
    }
    break;
    
  case SpectrumCalibrationStandard_RB_ID:{
    if(TheInterface->SpectrumCalibrationStandard_RB->IsDown()){
      TheInterface->SpectrumCalibrationEdgeFinder_RB->SetState(kButtonUp);
      TheInterface->NontabSlots->HandleTripleSliderPointer();
    }
    break;
  }    
    
  case SpectrumCalibrationEdgeFinder_RB_ID:{
    if(TheInterface->SpectrumCalibrationEdgeFinder_RB->IsDown()){
      GraphicsMgr->PlotSpectrum();
      TheInterface->SpectrumCalibrationStandard_RB->SetState(kButtonUp);
    }
    break;
  }
  }
}


void AASpectrumSlots::HandleTextButtons()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGTextButton *TextButton = (TGTextButton *) gTQSender;
  int TextButtonID  = TextButton->WidgetId();
  
  TheInterface->SaveSettings();
  
  switch(TextButtonID){

    ///////////////////////////////////////////
    // Waveform processing and spectra creation
    
    // This slot handles creating a pulse spectrum by processing the
    // raw waveforms using the present user settings 
  case ProcessSpectrum_TB_ID:{
    
    // Sequential waveform processing
    if(TheInterface->ProcessingSeq_RB->IsDown()){
      
      if(TheInterface->ADAQFileLoaded)
	ComputationMgr->ProcessSpectrumWaveforms();
      
      if(ComputationMgr->GetSpectrumExists())
	GraphicsMgr->PlotSpectrum();
    }
    
    // Parallel waveform processing
    else{
      if(TheInterface->ADAQFileLoaded){
	TheInterface->SaveSettings(true);
	
	if(TheInterface->ADAQSpectrumAlgorithmWD_RB->IsDown())
	  TheInterface->CreateMessageBox("Error! ADAQ waveform data can only be processed sequentially!\n","Stop");
	else
	  ComputationMgr->ProcessWaveformsInParallel("histogramming");
	
	if(ComputationMgr->GetSpectrumExists())
	  GraphicsMgr->PlotSpectrum();
      }
      else if(TheInterface->ASIMFileLoaded){
	TheInterface->CreateMessageBox("Error! ASIM files can only be processed sequentially!\n","Stop");
      }
    }
    
    TheInterface->UpdateForSpectrumCreation();
    
    break;
  }
    
    // This slot handles creating a pulse spectrum by using previously
    // computed values from analyzed waveforms
  case CreateSpectrum_TB_ID:{
    
    if(TheInterface->ADAQFileLoaded)
      ComputationMgr->CreateSpectrum();
    
    else if(TheInterface->ASIMFileLoaded)
      ComputationMgr->CreateASIMSpectrum();
    
    if(ComputationMgr->GetSpectrumExists())
      GraphicsMgr->PlotSpectrum();

    TheInterface->UpdateForSpectrumCreation();
    
    break;
  }
    
    
    //////////////////////
    // Spectra calibration

  case SpectrumCalibrationSetPoint_TB_ID:{

    // Get the calibration point to be set
    uint SetPoint = TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->GetSelected();

    // Get the energy of the present calibration point
    double Energy = TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->GetNumber();
   
    // Get the pulse unit value of the present calibration point
    double PulseUnit = TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->GetNumber();
    
    // Get the current channel being histogrammed in DGScope
    int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();

    bool NewPoint = ComputationMgr->SetCalibrationPoint(Channel, SetPoint, Energy, PulseUnit);
    
    if(NewPoint){
      // Add a new point to the number of calibration points in case
      // the user wants to add subsequent points to the calibration
      stringstream ss;
      ss << (SetPoint+1);
      string NewPointLabel = "Calibration point " + ss.str();
      TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->AddEntry(NewPointLabel.c_str(),SetPoint+1);
      
      // Set the combo box to display the new calibration point...
      TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->Select(SetPoint+1);
      
      // ...and set the calibration energy and pulse unit ROOT number
      // entry widgets to their default "0.0" and "1.0" respectively,
      TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(0.0);
      TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(1.0);
    }
    break;
  }

    ////////////////////
    // Spectra calibrate

  case SpectrumCalibrationCalibrate_TB_ID:{
    
    // If there are 2 or more points in the current channel's
    // calibration data set then create a new TGraph object. The
    // TGraph object will have pulse units [ADC] on the X-axis and the
    // corresponding energies [in whatever units the user has entered
    // the energy] on the Y-axis. A TGraph is used because it provides
    // very easy but extremely powerful methods for interpolation,
    // which allows the pulse height/area to be converted in to energy
    // efficiently in the acquisition loop.

    // Get the current channel being histogrammed in 
    int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();

    bool Success = ComputationMgr->SetCalibration(Channel);

    if(Success){
      TheInterface->SpectrumCalibrationCalibrate_TB->SetText("Calibrated");
      TheInterface->SpectrumCalibrationCalibrate_TB->SetForegroundColor(TheInterface->ColorMgr->Number2Pixel(0));
      TheInterface->SpectrumCalibrationCalibrate_TB->SetBackgroundColor(TheInterface->ColorMgr->Number2Pixel(32));
    }
    else
      TheInterface->CreateMessageBox("The calibration could not be set!","Stop");

    break;
  }

    ////////////////////////////
    // Spectrum calibration plot

  case SpectrumCalibrationPlot_TB_ID:{
    
    int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();
    GraphicsMgr->PlotCalibration(Channel);

    break;
  }

    /////////////////////////////
    // Spectrum calibration reset

  case SpectrumCalibrationReset_TB_ID:{
    
    // Get the current channel being histogrammed in 
    int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();
    
    bool Success = ComputationMgr->ClearCalibration(Channel);

    // Reset the calibration widgets
    if(Success){
      TheInterface->SpectrumCalibrationCalibrate_TB->SetText("Calibrate");
      TheInterface->SpectrumCalibrationCalibrate_TB->SetForegroundColor(TheInterface->ColorMgr->Number2Pixel(1));
      TheInterface->SpectrumCalibrationCalibrate_TB->SetBackgroundColor(TheInterface->ThemeForegroundColor);

      TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->RemoveAll();
      TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->AddEntry("Calibration point 0", 0);
      TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->Select(0);
      TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(0.0);
      TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(1.0);
    }
    break;
  }

  case SpectrumCalibrationLoad_TB_ID:{

    const char *FileTypes[] = {"ADAQ calibration file", "*.acal",
			       "All files",             "*.*",
			       0, 0};
    
    TGFileInfo FileInformation;
    FileInformation.fFileTypes = FileTypes;
    FileInformation.fIniDir = StrDup(getenv("PWD"));

    new TGFileDialog(gClient->GetRoot(), TheInterface, kFDOpen, &FileInformation);

    if(FileInformation.fFilename == NULL)
      TheInterface->CreateMessageBox("A calibration file was not selected! No calibration has been made!","Stop");
    else{

      // Get the present channel
      int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();
      
      //      string CalibrationFileName = "/home/hartwig/aims/ADAQAnalysis/test/ExptNaIWaveforms.acal";
      string CalibrationFileName = FileInformation.fFilename;

      // Set the calibration file to an input stream
      ifstream In(CalibrationFileName.c_str());

      // Reset an preexisting calibration to make way for the new!
      TheInterface->SpectrumCalibrationReset_TB->Clicked();

      // Iterate through each line in the file and use the data to set
      // the calibration points sequentially
      int SetPoint = 0;
      while(In.good()){
	double Energy, PulseUnit;
	In >> Energy >> PulseUnit;

	if(In.eof()) break;

	ComputationMgr->SetCalibrationPoint(Channel, SetPoint, Energy, PulseUnit);
	
	// Add a new point to the number of calibration points in case
	// the user wants to add subsequent points to the calibration
	stringstream ss;
	ss << (SetPoint+1);
	string NewPointLabel = "Calibration point " + ss.str();
	TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->AddEntry(NewPointLabel.c_str(),SetPoint+1);
	
	// Set the combo box to display the next setable calibration point...
	TheInterface->SpectrumCalibrationPoint_CBL->GetComboBox()->Select(SetPoint+1);
	TheInterface->SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(0.0);
	TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(1.0);
	
	SetPoint++;
      }

      In.close();

      // Use the loaded calibration points to set the calibration
      bool Success = ComputationMgr->SetCalibration(Channel);
      if(Success){
	TheInterface->SpectrumCalibrationCalibrate_TB->SetText("Calibrated");
	TheInterface->SpectrumCalibrationCalibrate_TB->SetForegroundColor(TheInterface->ColorMgr->Number2Pixel(0));
	TheInterface->SpectrumCalibrationCalibrate_TB->SetBackgroundColor(TheInterface->ColorMgr->Number2Pixel(32));
      }
      else
	TheInterface->CreateMessageBox("The calibration could not be set! Please check file format.","Stop");
    }
    break;
  }
  }
}
