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

/////////////////////////////////////////////////////////////////////////////////
// 
// name: AANontabSlots.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AANontabSlots class contains widget slot methods to
//       handle signals generated from widgets that are not contained
//       on one of the five tabs. This includes the file
//       menu, the sliders, and the canvas amongst others.
//
/////////////////////////////////////////////////////////////////////////////////

// ROOT
#include <TGFileDialog.h>
#include <TApplication.h>

// ADAQAnalysis
#include "AANontabSlots.hh"
#include "AAInterface.hh"
#include "AAGraphics.hh"


AANontabSlots::AANontabSlots(AAInterface *TI)
  : TheInterface(TI)
{
  ComputationMgr = AAComputation::GetInstance();
  GraphicsMgr = AAGraphics::GetInstance();
  InterpolationMgr = AAInterpolation::GetInstance();
}


AANontabSlots::~AANontabSlots()
{;}


void AANontabSlots::HandleCanvas(int EventID, int XPixel, int YPixel, TObject *Selected)
{
  if(!TheInterface->EnableInterface)
    return;
  
  // For an unknown reason, the XPixel value appears (erroneously) to
  // be two pixels too low for a given cursor selection, I have
  // examined this issue in detail but cannot determine the reason nor
  // is it treated in the ROOT forums. At present, the fix is simply
  // to add two pixels for a given XPixel cursor selection.
  XPixel += 2;

  // If the user has enabled the creation of a PSD filter and the
  // canvas event is equal to "1" (which represents a down-click
  // somewhere on the canvas pad) then send the pixel coordinates of
  // the down-click to the PSD filter creation function
  if(TheInterface->PSDEnableRegionCreation_CB->IsDown() and EventID == 1){
    ComputationMgr->AddPSDRegionPoint(XPixel, YPixel);
    GraphicsMgr->PlotPSDRegionProgress();
  }
  
  if(TheInterface->PSDEnableHistogramSlicing_CB->IsDown()){
    
    // The user may click the canvas to "freeze" the PSD histogram
    // slice position, which ensures the PSD histogram slice at that
    // point remains plotted in the standalone canvas
    if(EventID == 1){
      TheInterface->PSDEnableHistogramSlicing_CB->SetState(kButtonUp);

      // Properly disabled PSD histogram widgets
      TheInterface->PSDHistogramSliceX_RB->SetState(kButtonDisabled);
      TheInterface->PSDHistogramSliceY_RB->SetState(kButtonDisabled);
      TheInterface->PSDCalculateFOM_CB->SetState(kButtonDisabled);
      TheInterface->PSDLowerFOMFitMin_NEL->GetEntry()->SetState(false);
      TheInterface->PSDLowerFOMFitMax_NEL->GetEntry()->SetState(false);
      TheInterface->PSDUpperFOMFitMin_NEL->GetEntry()->SetState(false);
      TheInterface->PSDUpperFOMFitMax_NEL->GetEntry()->SetState(false);
      TheInterface->PSDFigureOfMerit_NEFL->GetEntry()->SetState(false);

      return;
    }
    else{
      ComputationMgr->CreatePSDHistogramSlice(XPixel, YPixel);
      GraphicsMgr->PlotPSDHistogramSlice(XPixel, YPixel);
      
      if(TheInterface->PSDCalculateFOM_CB->IsDown()){
	Double_t FOM = GraphicsMgr->GetPSDFigureOfMerit();
	TheInterface->PSDFigureOfMerit_NEFL->GetEntry()->SetNumber(FOM);
      }
    }
  }

  // The user has the option of using an automated edge location
  // finder for setting the calibration of EJ301/9 liq. organic
  // scintillators. The user set two points that must "bound" the
  // spectral edge:
  // point 0 : top height of edge; leftmost pulse unit
  // point 1 : bottom height of edge; rightmost pulse unit
  if(TheInterface->SpectrumCalibrationEdgeFinder_RB->IsDown()){

    if(ComputationMgr->GetSpectrumExists() and
       GraphicsMgr->GetCanvasContentType() == zSpectrum){

      double X = gPad->AbsPixeltoX(XPixel);
      double Y = gPad->AbsPixeltoY(YPixel);

      // Enables the a semi-transparent box to be drawn over the edge
      // to be calibrated once the user has clicked for the first point
      if(TheInterface->NumEdgeBoundingPoints == 1)
	GraphicsMgr->PlotEdgeBoundingBox(TheInterface->EdgeBoundX0, 
					 TheInterface->EdgeBoundY0, 
					 X, 
					 Y);

      // The bound point is set once the user clicks the canvas at the
      // desired (X,Y) == (Pulse unit, Counts) location. Note that
      // AAComputation class automatically keep track of which point
      // is set and when to calculate the edge
      if(EventID == 1){
	ComputationMgr->SetEdgeBound(X, Y);

	// Keep track of the number of times the user has clicked
	if(TheInterface->NumEdgeBoundingPoints == 0){
	  TheInterface->EdgeBoundX0 = X;
	  TheInterface->EdgeBoundY0 = Y;
	}
	
	TheInterface->NumEdgeBoundingPoints++;
	
	// Once the edge position is found (after two points are set)
	// then update the number entry so the user may set a
	// calibration points and draw the point on screen for
	// verification. Reset number of edge bounding points.
	if(ComputationMgr->GetEdgePositionFound()){
	  double HalfHeight = ComputationMgr->GetHalfHeight();
	  double EdgePos = ComputationMgr->GetEdgePosition();
	  TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(EdgePos);
	  
	  GraphicsMgr->PlotCalibrationCross(EdgePos, HalfHeight);
	  
	  TheInterface->NumEdgeBoundingPoints = 0;
	}
      }
    }
  }
}


void AANontabSlots::HandleDoubleSliders()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TheInterface->SaveSettings();
  
  // Get the currently active widget and its ID, i.e. the widget from
  // which the user has just sent a signal...
  TGDoubleSlider *DoubleSlider = (TGDoubleSlider *) gTQSender;
  int SliderID = DoubleSlider->WidgetId();

  switch(SliderID){
    
  case XAxisLimits_THS_ID:
  case YAxisLimits_DVS_ID:

    if(GraphicsMgr->GetCanvasContentType() == zWaveform)
      GraphicsMgr->PlotWaveform();
    
    else if(GraphicsMgr->GetCanvasContentType() == zSpectrum and ComputationMgr->GetSpectrumExists())
      GraphicsMgr->PlotSpectrum();
    
    else if(GraphicsMgr->GetCanvasContentType() == zSpectrumDerivative and ComputationMgr->GetSpectrumExists())
      GraphicsMgr->PlotSpectrumDerivative();
    
    else if(GraphicsMgr->GetCanvasContentType() == zPSDHistogram and ComputationMgr->GetPSDHistogramExists())
      GraphicsMgr->PlotPSDHistogram();
    
    break;

  case SpectrumIntegrationLimits_DHS_ID:
    ComputationMgr->IntegrateSpectrum();
    GraphicsMgr->PlotSpectrum();
    
    if(TheInterface->SpectrumFindIntegral_CB->IsDown()){
      TheInterface->SpectrumIntegral_NEFL->GetEntry()->SetNumber( ComputationMgr->GetSpectrumIntegralValue() );
      TheInterface->SpectrumIntegralError_NEFL->GetEntry()->SetNumber (ComputationMgr->GetSpectrumIntegralError () );
    }
    else{
      TheInterface->SpectrumIntegral_NEFL->GetEntry()->SetNumber(0.);
      TheInterface->SpectrumIntegralError_NEFL->GetEntry()->SetNumber(0.);
    }
    
    if(TheInterface->SpectrumUseGaussianFit_CB->IsDown()){
      double Const = ComputationMgr->GetSpectrumFit()->GetParameter(0);
      double Mean = ComputationMgr->GetSpectrumFit()->GetParameter(1);
      double Sigma = ComputationMgr->GetSpectrumFit()->GetParameter(2);
      
      TheInterface->SpectrumFitHeight_NEFL->GetEntry()->SetNumber(Const);
      TheInterface->SpectrumFitMean_NEFL->GetEntry()->SetNumber(Mean);
      TheInterface->SpectrumFitSigma_NEFL->GetEntry()->SetNumber(Sigma);
      TheInterface->SpectrumFitRes_NEFL->GetEntry()->SetNumber(2.35 * Sigma / Mean * 100);
    }

    // Update the NEL's that are used for manual entry of the analysis
    // range with the current value of the double slider

    double MinBin = TheInterface->SpectrumMinBin_NEL->GetEntry()->GetNumber();
    double MaxBin = TheInterface->SpectrumMaxBin_NEL->GetEntry()->GetNumber();
    double Range = MaxBin - MinBin;

    float Min,Max;
    TheInterface->SpectrumIntegrationLimits_DHS->GetPosition(Min, Max);

    double Lower = MinBin + (Range * Min);
    double Upper = MinBin + (Range * Max);

    TheInterface->SpectrumAnalysisLowerLimit_NEL->GetEntry()->SetNumber(Lower);
    TheInterface->SpectrumAnalysisUpperLimit_NEL->GetEntry()->SetNumber(Upper);

    break;
  }
}


void AANontabSlots::HandleMenu(int MenuID)
{
  switch(MenuID){
    
    // Action that enables the user to select a ROOT file with
    // waveforms using the nicely prebuilt ROOT interface for file
    // selection. Only ROOT files are displayed in the dialog.
  case MenuFileOpenADAQ_ID:
  case MenuFileOpenASIM_ID:{

    string Desc[2], Type[2];
    if(MenuFileOpenADAQ_ID == MenuID){
      Desc[0] = "ADAQ Experiment (ADAQ) file";
      Type[0] = "*.adaq.root";
      
      Desc[1] = "ADAQ Experiment (ADAQ) file";
      Type[1] = "*.adaq";
      
    }
    else if(MenuFileOpenASIM_ID == MenuID){
      Desc[0] = "ADAQ Simulation (ASIM) file";
      Type[0] = "*.asim.root";
      
      Desc[1] = "ADAQ Simulation (ASIM) file";
      Type[1] = "*.root";
    }
    
    const char *FileTypes[] = {Desc[0].c_str(), Type[0].c_str(),
			       Desc[1].c_str(), Type[1].c_str(),
			       "All files",                    "*",
			       0,                              0};
    
    // Use the ROOT prebuilt file dialog machinery
    TGFileInfo FileInformation;
    FileInformation.fFileTypes = FileTypes;
    FileInformation.fIniDir = StrDup(TheInterface->DataDirectory.c_str());
    new TGFileDialog(gClient->GetRoot(), TheInterface, kFDOpen, &FileInformation);
    
    // If the selected file is not found...
    if(FileInformation.fFilename==NULL)
      TheInterface->CreateMessageBox("No valid ROOT file was selected so there's nothing to load!\nPlease select a valid file!","Stop");
    else{
      // Note that the FileInformation.fFilename variable storeds the
      // absolute path to the data file selected by the user
      string FileName = FileInformation.fFilename;
      
      // Strip the data file name off the absolute file path and set
      // the path to the DataDirectory variable. Thus, the "current"
      // directory from which a file was selected will become the new
      // default directory that automatically open
      size_t pos = FileName.find_last_of("/");
      if(pos != string::npos)
	TheInterface->DataDirectory  = FileName.substr(0,pos);
      
      // Load the ROOT file and set the bool depending on outcome
      if(MenuID == MenuFileOpenADAQ_ID){
	TheInterface->ADAQFileName = FileName;
	TheInterface->ADAQFileLoaded = ComputationMgr->LoadADAQFile(FileName);
	
	// Set whether or not the interface should be enabled
	TheInterface->EnableInterface = TheInterface->ADAQFileLoaded;
	
	if(TheInterface->ADAQFileLoaded)
	  TheInterface->UpdateForADAQFile();
	else
	  TheInterface->CreateMessageBox("The ADAQ ROOT file that you specified fail to load for some reason!\n","Stop");
	
	TheInterface->ASIMFileLoaded = false;
      }
      else if(MenuID == MenuFileOpenASIM_ID){
	TheInterface->ASIMFileName = FileName;
	TheInterface->ASIMFileLoaded = ComputationMgr->LoadASIMFile(FileName);

	// Set whether or not the interface should be enabled
	TheInterface->EnableInterface = TheInterface->ASIMFileLoaded;
	
	if(TheInterface->ASIMFileLoaded)
	  TheInterface->UpdateForASIMFile();
	else
	  TheInterface->CreateMessageBox("The ADAQ Simulation (ASIM) ROOT file that you specified fail to load for some reason!\n","Stop");
	
	TheInterface->ADAQFileLoaded = false;
      }
    }
    break;
  }

  case MenuFileLoadSpectrum_ID:{

    const char *FileTypes[] = {"ROOT file",   "*.root",
			       0,             0};
    
    TGFileInfo FileInformation;
    FileInformation.fFileTypeIdx = 0;
    FileInformation.fFileTypes = FileTypes;
    FileInformation.fIniDir = StrDup(TheInterface->HistogramDirectory.c_str());
    
    new TGFileDialog(gClient->GetRoot(), TheInterface, kFDOpen, &FileInformation);

    if(FileInformation.fFilename==NULL)
      TheInterface->CreateMessageBox("No file was selected! Nothing will be saved to file!","Stop");
    else{

      TString FileName = FileInformation.fFilename;
      TFile *F = new TFile(FileName, "read");
      
      TH1F *H = NULL;
      H = (TH1F *)F->Get("Spectrum");

      if(H == NULL){
	TheInterface->CreateMessageBox("No TH1F object named 'Spectrum' was found in the specific file!","Stop");
	
	// If there is no valid ADAQ or ASIM file already loaded and
	// loading a TH1F spectrum failed, we need to disable the
	// interface to prevent seg faults since there is nothing for
	// the interface and underlying algorithms to operate on
	if(!TheInterface->ADAQFileLoaded or !TheInterface->ASIMFileLoaded)
	  TheInterface->EnableInterface = false;
      }
      else{
	
	// Get the lower edge of the lowest non-underflow bin
	Double_t MinBin = H->GetBinLowEdge(1);
	
	// Get the upper edge of the highest non-overflow bin
	Double_t MaxBin = H->GetBinLowEdge(H->GetNbinsX()+1);
	
	// Update various spectral and analysis values with the new
	// TH1F parameters
	TheInterface->SpectrumNumBins_NEL->GetEntry()->SetNumber(H->GetNbinsX());
	TheInterface->SpectrumMinBin_NEL->GetEntry()->SetNumber(MinBin);
	TheInterface->SpectrumMaxBin_NEL->GetEntry()->SetNumber(MaxBin);
	TheInterface->SpectrumRangeMin_NEL->GetEntry()->SetNumber(MinBin);
	TheInterface->SpectrumRangeMax_NEL->GetEntry()->SetNumber(MaxBin);
	TheInterface->SpectrumAnalysisLowerLimit_NEL->GetEntry()->SetNumber(MinBin);
	TheInterface->SpectrumAnalysisUpperLimit_NEL->GetEntry()->SetNumber(MaxBin);
	
	
	// Ensure that the interface is enabled since a valid TH1F
	// spectrum was loaded
	TheInterface->EnableInterface = true;
	
	TheInterface->SaveSettings();
	ComputationMgr->SetSpectrum(H);
	GraphicsMgr->PlotSpectrum();
      }
    }
    
    break;
  }

  case MenuFileLoadPSDHistogram_ID:
    TheInterface->CreateMessageBox("Loading a PSD histogram is not yet implemented!","Stop");
    break;

  case MenuFileSaveWaveform_ID:
  case MenuFileSaveSpectrum_ID:
  case MenuFileSaveSpectrumBackground_ID:
  case MenuFileSaveSpectrumDerivative_ID:
  case MenuFileSavePSDHistogram_ID:
  case MenuFileSavePSDHistogramSlice_ID:{

    // Create character arrays that enable file type selection (.dat
    // files have data columns separated by spaces and .csv have data
    // columns separated by commas)
    const char *FileTypes[] = {"ASCII file",  "*.dat",
			       "CSV file",    "*.csv",
			       "ROOT file",   "*.root",
			       0,             0};
  
    TGFileInfo FileInformation;
    FileInformation.fFileTypeIdx = 4;
    FileInformation.fFileTypes = FileTypes;
    FileInformation.fIniDir = StrDup(TheInterface->HistogramDirectory.c_str());
    
    new TGFileDialog(gClient->GetRoot(), TheInterface, kFDSave, &FileInformation);
    
    if(FileInformation.fFilename==NULL)
      TheInterface->CreateMessageBox("No file was selected! Nothing will be saved to file!","Stop");
    else{
      string FileName, FileExtension;
      size_t Found = string::npos;
      
      // Get the file name for the output histogram data. Note that
      // FileInformation.fFilename is the absolute path to the file.
      FileName = FileInformation.fFilename;

      // Strip the data file name off the absolute file path and set
      // the path to the DataDirectory variable. Thus, the "current"
      // directory from which a file was selected will become the new
      // default directory that automically opens
      size_t pos = FileName.find_last_of("/");
      if(pos != string::npos)
	TheInterface->HistogramDirectory  = FileName.substr(0,pos);

      // Strip the file extension (the start of the file extension is
      // assumed here to begin with the final period) to extract just
      // the save file name.
      Found = FileName.find_last_of(".");
      if(Found != string::npos)
	FileName = FileName.substr(0, Found);

      // Extract only the "." with the file extension. Note that anove
      // the "*" character precedes the file extension string when
      // passed to the FileInformation class in order for files
      // containing that expression to be displaced to the
      // user. However, I strip the "*" such that the "." plus file
      // extension can be used by the SaveSpectrumData() function to
      // determine the format of spectrum save file.
      FileExtension = FileInformation.fFileTypes[FileInformation.fFileTypeIdx+1];
      Found = FileExtension.find_last_of("*");
      
      if(Found != string::npos)
	FileExtension = FileExtension.substr(Found+1, FileExtension.size());
      
      bool Success = false;
      
      if(MenuID == MenuFileSaveWaveform_ID){
	Success = ComputationMgr->SaveHistogramData("Waveform", FileName, FileExtension);
      }
      
      else if(MenuID == MenuFileSaveSpectrum_ID){
	if(!ComputationMgr->GetSpectrumExists()){
	  TheInterface->CreateMessageBox("No spectra have been created yet and, therefore, there is nothing to save!","Stop");
	  break;
	}
	else
	  Success = ComputationMgr->SaveHistogramData("Spectrum", FileName, FileExtension);
      }
      
      else if(MenuID == MenuFileSaveSpectrumBackground_ID){
	if(!ComputationMgr->GetSpectrumBackgroundExists()){
	  TheInterface->CreateMessageBox("No spectrum derivatives have been created yet and, therefore, there is nothing to save!","Stop");
	  break;
	}
	else
	  Success = ComputationMgr->SaveHistogramData("SpectrumBackground", FileName, FileExtension);
      }

      else if(MenuID == MenuFileSaveSpectrumDerivative_ID){
	if(!ComputationMgr->GetSpectrumDerivativeExists()){
	  TheInterface->CreateMessageBox("No spectrum derivatives have been created yet and, therefore, there is nothing to save!","Stop");
	  break;
	}
	else
	  Success = ComputationMgr->SaveHistogramData("SpectrumDerivative", FileName, FileExtension);
      }

      else if(MenuID == MenuFileSavePSDHistogram_ID){
	if(!ComputationMgr->GetPSDHistogramExists()){
	  TheInterface->CreateMessageBox("A PSD histogram has not been created yet and, therefore, there is nothing to save!","Stop");
	  break;
	}

	if(FileExtension != ".root"){
	  TheInterface->CreateMessageBox("PSD histograms may only be saved to a ROOT TFile (*.root). Please reselect file type!","Stop");
	  break;
	}

	Success = ComputationMgr->SaveHistogramData("PSDHistogram", FileName, FileExtension);
      }
      
      else if(MenuID == MenuFileSavePSDHistogramSlice_ID){
	if(!ComputationMgr->GetPSDHistogramSliceExists()){
	  TheInterface->CreateMessageBox("A PSD histogram slice has not been created yet and, therefore, there is nothing to save!","Stop");
	  break;
	}
	else
	  Success = ComputationMgr->SaveHistogramData("PSDHistogramSlice", FileName, FileExtension);
      }
      
      if(Success){
	if(FileExtension == ".dat")
	  TheInterface->CreateMessageBox("The histogram was successfully saved to the .dat file","Asterisk");
	else if(FileExtension == ".csv")
	  TheInterface->CreateMessageBox("The histogram was successfully saved to the .csv file","Asterisk");
	else if(FileExtension == ".root")
	  TheInterface->CreateMessageBox("The histogram (named 'Waveform','Spectrum',or 'PSDHistogram') \nwas successfully saved to the .root file!\n","Asterisk");
      }
      else
	TheInterface->CreateMessageBox("The histogram failed to save to the file for unknown reasons!","Stop");
    }
    break;
  }

  case MenuFileSaveSpectrumCalibration_ID:{
    
    const char *FileTypes[] = {"ADAQ calibration file", "*.acal",
			       "All files"            , "*.*",
			       0, 0};
    
    TGFileInfo FileInformation;
    FileInformation.fFileTypes = FileTypes;
    FileInformation.fIniDir = StrDup(getenv("PWD"));

    new TGFileDialog(gClient->GetRoot(), TheInterface, kFDSave, &FileInformation);

    if(FileInformation.fFilename==NULL)
      TheInterface->CreateMessageBox("No file was selected and, therefore, nothing will be saved!","Stop");
    else{
      
      string FName = FileInformation.fFilename;

      size_t Found = FName.find_last_of(".acal");
      if(Found == string::npos)
	FName += ".acal";

      const int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();
      bool Success = ComputationMgr->WriteCalibrationFile(Channel, FName);
      if(Success)
	TheInterface->CreateMessageBox("The calibration file was successfully written to file.","Asterisk");
      else
	TheInterface->CreateMessageBox("There was an unknown error in writing the calibration file!","Stop");
    }
    break;
  }
    
    // Action that enables the user to print the currently displayed
    // canvas to a file of the user's choice. But really, it's not a
    // choice. Vector-based graphics are the only way to go. Do
    // everyone a favor, use .eps or .ps or .pdf if you want to be a
    // serious scientist.
  case MenuFilePrint_ID:{
    
    // List the available graphical file options
    const char *FileTypes[] = {"EPS file",          "*.eps",
			       "JPG file",          "*.jpeg",
			       "PS file",           "*.ps",
			       "PDF file",          "*.pdf",
			       "PNG file",          "*.png",
			       0,                  0 };

    // Use the ROOT prebuilt file dialog machinery
    TGFileInfo FileInformation;
    FileInformation.fFileTypes = FileTypes;
    FileInformation.fIniDir = StrDup(TheInterface->PrintDirectory.c_str());

    new TGFileDialog(gClient->GetRoot(), TheInterface, kFDSave, &FileInformation);
    
    if(FileInformation.fFilename==NULL)
      TheInterface->CreateMessageBox("No file was selected to the canvas graphics will not be saved!\nSelect a valid file to save the canvas graphis!","Stop");
    else{

      string GraphicFileName, GraphicFileExtension;

      size_t Found = string::npos;
      
      GraphicFileName = FileInformation.fFilename;

      // Strip the data file name off the absolute file path and set
      // the path to the DataDirectory variable. Thus, the "current"
      // directory from which a file was selected will become the new
      // default directory that automically opens
      Found = GraphicFileName.find_last_of("/");
      if(Found != string::npos)
	TheInterface->PrintDirectory  = GraphicFileName.substr(0,Found);
      
      Found = GraphicFileName.find_last_of(".");
      if(Found != string::npos)
	GraphicFileName = GraphicFileName.substr(0, Found);
      
      GraphicFileExtension = FileInformation.fFileTypes[FileInformation.fFileTypeIdx+1];
      Found = GraphicFileExtension.find_last_of("*");
      if(Found != string::npos)
	GraphicFileExtension = GraphicFileExtension.substr(Found+1, GraphicFileExtension.size());
      
      string GraphicFile = GraphicFileName+GraphicFileExtension;
      
      TheInterface->Canvas_EC->GetCanvas()->Update();
      TheInterface->Canvas_EC->GetCanvas()->Print(GraphicFile.c_str(), GraphicFileExtension.c_str());
      
      string SuccessMessage = "The canvas graphics have been successfully saved to the following file:\n" + GraphicFileName + GraphicFileExtension;
      TheInterface->CreateMessageBox(SuccessMessage,"Asterisk");
    }
    break;
  }
    
    // Action that enables the Quit_TB and File->Exit selections to
    // quit the ROOT application
  case MenuFileExit_ID:
    gApplication->Terminate();
    
  default:
    break;
  }
}


void AANontabSlots::HandleSliders(int SliderPosition)
{
  if(!TheInterface->ADAQFileLoaded or TheInterface->ASIMFileLoaded)
    return;
  
  TheInterface->SaveSettings();
  
  TheInterface->WaveformSelector_NEL->GetEntry()->SetNumber(SliderPosition);
  
  GraphicsMgr->PlotWaveform();

  // Update the waveform analysis widgets
  if(TheInterface->WaveformAnalysis_CB->IsDown()){
    double Height = ComputationMgr->GetWaveformAnalysisHeight();
    TheInterface->WaveformHeight_NEL->GetEntry()->SetNumber(Height);
    
    double Area = ComputationMgr->GetWaveformAnalysisArea();
    TheInterface->WaveformIntegral_NEL->GetEntry()->SetNumber(Area);
  }
}


void AANontabSlots::HandleTerminate()
{ gApplication->Terminate(); }


void AANontabSlots::HandleTripleSliderPointer()
{
  if(!TheInterface->EnableInterface)
    return;
  
  if(ComputationMgr->GetSpectrumExists() and
     GraphicsMgr->GetCanvasContentType() == zSpectrum){

    // Get the current spectrum
    TH1F *Spectrum_H = ComputationMgr->GetSpectrum();

    // Calculate the position along the X-axis of the pulse spectrum
    // (the "area" or "height" in ADC units) based on the current
    // X-axis maximum and the triple slider's pointer position
    double XPos = TheInterface->XAxisLimits_THS->GetPointerPosition() * Spectrum_H->GetXaxis()->GetXmax();

    // To enable easy spectra calibration, the user has the options of
    // dragging the pointer of the X-axis horizontal slider just below
    // the canvas, which results in a "calibration line" drawn over
    // the plotted pulse spectrum. As the user drags the line, the
    // pulse unit number entry widget in the calibration group frame
    // is update, allowing the user to easily set the value of pulse
    // units (in ADC) of a feature that appears in the spectrum with a
    // known calibration energy, entered in the number entry widget
    // above.

    // If the pulse spectrum object (Spectrum_H) exists and the user has
    // selected calibration mode via the appropriate buttons ...
    if(TheInterface->SpectrumCalibration_CB->IsDown() and 
       TheInterface->SpectrumCalibrationStandard_RB->IsDown()){
      
      // Plot the calibration line
      GraphicsMgr->PlotVCalibrationLine(XPos);
      
      // Set the calibration pulse units for the current calibration
      // point based on the X position of the calibration line
      TheInterface->SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(XPos);
    }
    
    // The user can drag the triple slider point in order to calculate
    // the energy deposition from other particle to produce the
    // equivalent amount of light as electrons. This feature is only
    // intended for use for EJ301/EJ309 liqoid organic scintillators.
    // Note that the spectra must be calibrated in MeVee.
    
    const int Channel = TheInterface->ChannelSelector_CBL->GetComboBox()->GetSelected();
    bool SpectrumIsCalibrated = ComputationMgr->GetUseSpectraCalibrations()[Channel];
    
    if(TheInterface->EAEnable_CB->IsDown()){

      if((TheInterface->ADAQFileLoaded and SpectrumIsCalibrated) or
	 TheInterface->ASIMFileLoaded){
      
	int Type = TheInterface->EASpectrumType_CBL->GetComboBox()->GetSelected();

	double Error = 0.;
	bool ErrorBox = false;
	bool EscapePeaks = TheInterface->EAEscapePeaks_CB->IsDown();
	
	if(Type == 0){
	  GraphicsMgr->PlotEALine(XPos, Error, ErrorBox, EscapePeaks);
	  
	  TheInterface->EAGammaEDep_NEL->GetEntry()->SetNumber(XPos);
	}
	
	else if(Type == 1){

	  Error = TheInterface->EAErrorWidth_NEL->GetEntry()->GetNumber();
	  ErrorBox = true;
	  EscapePeaks = false;
	  
	  GraphicsMgr->PlotEALine(XPos, Error, ErrorBox, EscapePeaks);
	  
	  TheInterface->EAElectronEnergy_NEL->GetEntry()->SetNumber(XPos);
	  
	  double GE = InterpolationMgr->GetGammaEnergy(XPos);
	  TheInterface->EAGammaEnergy_NEL->GetEntry()->SetNumber(GE);
	  
	  double PE = InterpolationMgr->GetProtonEnergy(XPos);
	  TheInterface->EAProtonEnergy_NEL->GetEntry()->SetNumber(PE);
	  
	  double AE = InterpolationMgr->GetAlphaEnergy(XPos);
	  TheInterface->EAAlphaEnergy_NEL->GetEntry()->SetNumber(AE);
	  
	  double CE = InterpolationMgr->GetCarbonEnergy(XPos);
	  TheInterface->EACarbonEnergy_NEL->GetEntry()->SetNumber(CE);
	}
      }
    }
  }
}


