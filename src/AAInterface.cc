/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                            Copyright (C) 2012-2023                          //
//                  Zachary Seth Hartwig : All rights reserved                 //
//                                                                             //
//      The ADAQAnalysis source code is licensed under the GNU GPL v3.0.       //
//      You have the right to modify and/or redistribute this source code      //
//      under the terms specified in the license, which is found online        //
//      at http://www.gnu.org/licenses or at $ADAQANALYSIS/License.txt.        //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// 
// name: AAInterface.cc
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAInterface class is responsible for building the entire
//       graphical user interface, including the creation and
//       organization of all widgets and the connection of the widgets
//       to the "slot" methods that process the generated widget
//       "signals". The slot methods are organized into six associated
//       classes that are named "AA*Slots", where * refers to one of
//       the five GUI tabs or the remaining non-tab methods. The slot
//       handler classes are friend class data members of AAInterface
//       in order that they may manipulate other data members.
//
/////////////////////////////////////////////////////////////////////////////////


// ROOT
#include <TGClient.h>
#include <TGFileDialog.h>
#include <TGButtonGroup.h>
#include <TFile.h>
#include <TGText.h>

// C++
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

// Boost
#include <boost/thread.hpp>

// MPI
#ifdef MPI_ENABLED
#include <mpi.h>
#endif

// ADAQ
#include "AAInterface.hh"
#include "AAWaveformSlots.hh"
#include "AASpectrumSlots.hh"
#include "AAAnalysisSlots.hh"
#include "AAPSDSlots.hh"
#include "AAGraphics.hh"
#include "AAGraphicsSlots.hh"
#include "AAProcessingSlots.hh"
#include "AANontabSlots.hh"
#include "AAVersion.hh"


AAInterface::AAInterface(string CmdLineArg)
  : TGMainFrame(gClient->GetRoot()),
    NumDataChannels(16), NumProcessors(boost::thread::hardware_concurrency()),
    CanvasX(700), CanvasY(500), CanvasFrameWidth(700), 
    SliderBuffer(30), TotalX(1170), TotalY(800),
    TabFrameWidth(365), TabFrameLength(720),
    DataDirectory(getenv("PWD")), PrintDirectory(getenv("HOME")),
    DesplicedDirectory(getenv("HOME")), HistogramDirectory(getenv("HOME")),
    ADAQFileLoaded(false), ASIMFileLoaded(false), EnableInterface(false),
    ColorMgr(new TColor), RndmMgr(new TRandom3)
{
  SetCleanup(kDeepCleanup);

  // Allow env. variable to control small version of GUI
  if(getenv("ADAQANALYSIS_SMALL")!=NULL){
    CanvasX = 500;
    CanvasY = 300;
    CanvasFrameWidth = 400;
    TabFrameLength = 540;
    TotalX = 950;
    TotalY = 610;
  }

  // Create the slot handler classes for the widgets on each of the
  // five tabs and the remaining nontab widgets
  WaveformSlots = new AAWaveformSlots(this);
  SpectrumSlots = new AASpectrumSlots(this);
  AnalysisSlots = new AAAnalysisSlots(this);
  PSDSlots = new AAPSDSlots(this);
  GraphicsSlots = new AAGraphicsSlots(this);
  ProcessingSlots = new AAProcessingSlots(this);
  NontabSlots = new AANontabSlots(this);

  // Set the fore- and background colors
  ThemeForegroundColor = ColorMgr->Number2Pixel(18);
  ThemeBackgroundColor = ColorMgr->Number2Pixel(22);

  // Create the graphical user interface
  CreateTheMainFrames();

  // Get pointers to singleton managers
  ComputationMgr = AAComputation::GetInstance();
  ComputationMgr->SetProgressBarPointer(ProcessingProgress_PB);

  GraphicsMgr = AAGraphics::GetInstance();
  GraphicsMgr->SetCanvasPointer(Canvas_EC->GetCanvas());
  GraphicsMgr->SetInterfacePointer(this);

  InterpolationMgr = AAInterpolation::GetInstance();

  // If the user has specified an ADAQ or ASIM root file on the
  // command line then automatically process and load it
  if(CmdLineArg != "Unspecified"){

    // The following file name extensions are mandatory:
    //   ADAQ: .adaq.root (preferred), .adaq (legacy)
    //   ASIM: .asim.root (preferred), 
    
    size_t Pos = CmdLineArg.find_last_of(".adaq");
    if(Pos != string::npos){

      if(CmdLineArg.substr(Pos-5,10) == ".adaq.root" or
	 CmdLineArg.substr(Pos-4,5) == ".adaq"){
	
	// Ensure that if an ADAQ or ASIM file is specified from the
	// cmd line with a relative path that the absolute path to the
	// file is specified to ensure the universal access

	string CurrentDir = getenv("PWD");
	string RelativeFilePath = CmdLineArg;
	
	ADAQFileName = CurrentDir + "/" + RelativeFilePath;
	ADAQFileLoaded = ComputationMgr->LoadADAQFile(ADAQFileName);
	
	// If a valid ADAQ file was loaded then set the boolean that
	// controls whether or not the interface should be enabled
	EnableInterface = ADAQFileLoaded;
	
	if(ADAQFileLoaded)
	  UpdateForADAQFile();
	else
	  CreateMessageBox("The ADAQ ROOT file that you specified fail to load for some reason!\n","Stop");
	
	ASIMFileLoaded = false;
      }
      else if(CmdLineArg.substr(Pos,10) == ".asim.root" or
	      CmdLineArg.substr(Pos,5) == ".root"){
	
	ASIMFileName = CmdLineArg;
	ASIMFileLoaded = ComputationMgr->LoadASIMFile(ASIMFileName);
	
	// If a valid ASIM file was loaded then set the boolean that
	// controls whether or not the interface should be enabled
	EnableInterface = ASIMFileLoaded;
	
	if(ASIMFileLoaded)
	  UpdateForASIMFile();
	else
	  CreateMessageBox("The ADAQ Simulation (ASIM) ROOT file that you specified fail to load for some reason!\n","Stop");
      }
      else
	CreateMessageBox("Compatible files must end in: '.adaq.root' / '.adaq' (ADAQ); '.asim.root' (ASIM)","Stop");
    }
    else
      CreateMessageBox("Could not find an acceptable file to open. Please try again.","Stop");
  }

  // Initial the AASettings data member
  ADAQSettings = new AASettings;
  
  // Create a personalized temporary file name in /tmp that stores the
  // GUI widget values for parallel processing
  string USER = getenv("USER");
  ADAQSettingsFileName = "/tmp/ADAQSettings_" + USER + ".root";

  // Ensure the interface window closes properly when the "x" is clicked
  Connect("CloseWindow()", "AANontabSlots", NontabSlots, "HandleTerminate()");
}


AAInterface::~AAInterface()
{
  delete ADAQSettings;
  delete NontabSlots;
  delete ProcessingSlots;
  delete GraphicsSlots;
  delete AnalysisSlots;
  delete SpectrumSlots;
  delete WaveformSlots;
  delete RndmMgr;
  delete ColorMgr;
}


void AAInterface::CreateTheMainFrames()
{
  /////////////////////////
  // Create the menu bar //
  /////////////////////////

  TGHorizontalFrame *MenuFrame = new TGHorizontalFrame(this); 
  MenuFrame->SetBackgroundColor(ThemeBackgroundColor);
  
  TGPopupMenu *MenuFile = new TGPopupMenu(gClient->GetRoot());

  MenuFile->AddEntry("&Open ADAQ file ...", MenuFileOpenADAQ_ID);
  MenuFile->AddEntry("Ope&n ASIM file ...", MenuFileOpenASIM_ID);
  
  MenuFile->AddSeparator();

  MenuFile->AddEntry("Load spectrum ...", MenuFileLoadSpectrum_ID);
  MenuFile->AddEntry("Load PSD histogram ...", MenuFileLoadPSDHistogram_ID);
  
  MenuFile->AddSeparator();

  MenuFile->AddEntry("Save &waveform ...", MenuFileSaveWaveform_ID);

  TGPopupMenu *SaveSpectrumSubMenu = new TGPopupMenu(gClient->GetRoot());
  SaveSpectrumSubMenu->AddEntry("&raw", MenuFileSaveSpectrum_ID);
  SaveSpectrumSubMenu->AddEntry("&background", MenuFileSaveSpectrumBackground_ID);
  SaveSpectrumSubMenu->AddEntry("&derivative", MenuFileSaveSpectrumDerivative_ID);
  MenuFile->AddPopup("Save &spectrum ...", SaveSpectrumSubMenu);
  
  TGPopupMenu *SavePSDSubMenu = new TGPopupMenu(gClient->GetRoot());
  SavePSDSubMenu->AddEntry("histo&gram", MenuFileSavePSDHistogram_ID);
  SavePSDSubMenu->AddEntry("sli&ce", MenuFileSavePSDHistogramSlice_ID);
  MenuFile->AddPopup("Save &PSD ...",SavePSDSubMenu);

  TGPopupMenu *SaveCalibrationSubMenu = new TGPopupMenu(gClient->GetRoot());
  SaveCalibrationSubMenu->AddEntry("&values to file", MenuFileSaveSpectrumCalibration_ID);
  MenuFile->AddPopup("Save &calibration ...", SaveCalibrationSubMenu);

  TGPopupMenu *SaveAnalysisSubMenu = new TGPopupMenu(gClient->GetRoot());
  SaveAnalysisSubMenu->AddEntry("&Spectrum analysis results to file", MenuFileSaveSpectrumAnalysisResults_ID);
  MenuFile->AddPopup("Save &analysis ...", SaveAnalysisSubMenu);

  MenuFile->AddSeparator();

  MenuFile->AddEntry("&Print canvas ...", MenuFilePrint_ID);

  MenuFile->AddSeparator();

  MenuFile->AddEntry("E&xit", MenuFileExit_ID);

  MenuFile->Connect("Activated(int)", "AANontabSlots", NontabSlots, "HandleMenu(int)");

  TGMenuBar *MenuBar = new TGMenuBar(MenuFrame, 100, 20, kHorizontalFrame);
  MenuBar->SetBackgroundColor(ThemeBackgroundColor);
  MenuBar->AddPopup("&File", MenuFile, new TGLayoutHints(kLHintsTop | kLHintsLeft, 0,0,0,0));
  MenuFrame->AddFrame(MenuBar, new TGLayoutHints(kLHintsTop | kLHintsLeft, 0,0,0,0));
  AddFrame(MenuFrame, new TGLayoutHints(kLHintsTop | kLHintsExpandX));


  //////////////////////////////////////////////////////////
  // Create the main frame to hold the options and canvas //
  //////////////////////////////////////////////////////////

  AddFrame(OptionsAndCanvas_HF = new TGHorizontalFrame(this),
	   new TGLayoutHints(kLHintsTop, 5,5,5,5));
  
  OptionsAndCanvas_HF->SetBackgroundColor(ThemeBackgroundColor);

  
  //////////////////////////////////////////////////////////////
  // Create the left-side vertical options frame with tab holder 

  TGVerticalFrame *Options_VF = new TGVerticalFrame(OptionsAndCanvas_HF);
  Options_VF->SetBackgroundColor(ThemeBackgroundColor);
  OptionsAndCanvas_HF->AddFrame(Options_VF);
  
  OptionsTabs_T = new TGTab(Options_VF);
  OptionsTabs_T->SetBackgroundColor(ThemeBackgroundColor);
  Options_VF->AddFrame(OptionsTabs_T, new TGLayoutHints(kLHintsLeft, 15,15,5,5));


  //////////////////////////////////////
  // Create the individual tabs + frames

  // The tabbed frame to hold waveform widgets
  WaveformOptionsTab_CF = OptionsTabs_T->AddTab("Waveform");
  WaveformOptionsTab_CF->AddFrame(WaveformOptions_CF = new TGCompositeFrame(WaveformOptionsTab_CF, 1, 1, kVerticalFrame));
  WaveformOptions_CF->Resize(TabFrameWidth, TabFrameLength);
  WaveformOptions_CF->ChangeOptions(WaveformOptions_CF->GetOptions() | kFixedSize);

  // The tabbed frame to hold spectrum widgets
  SpectrumOptionsTab_CF = OptionsTabs_T->AddTab("Spectrum");
  SpectrumOptionsTab_CF->AddFrame(SpectrumOptions_CF = new TGCompositeFrame(SpectrumOptionsTab_CF, 1, 1, kVerticalFrame));
  SpectrumOptions_CF->Resize(TabFrameWidth, TabFrameLength);
  SpectrumOptions_CF->ChangeOptions(SpectrumOptions_CF->GetOptions() | kFixedSize);

  // The tabbed frame to hold analysis widgets
  AnalysisOptionsTab_CF = OptionsTabs_T->AddTab("Analysis");
  AnalysisOptionsTab_CF->AddFrame(AnalysisOptions_CF = new TGCompositeFrame(AnalysisOptionsTab_CF, 1, 1, kVerticalFrame));
  AnalysisOptions_CF->Resize(TabFrameWidth, TabFrameLength);
  AnalysisOptions_CF->ChangeOptions(AnalysisOptions_CF->GetOptions() | kFixedSize);

  // The tabbed frame to hold PSD widgets
  PSDOptionsTab_CF = OptionsTabs_T->AddTab("  PSD  ");
  PSDOptionsTab_CF->AddFrame(PSDOptions_CF = new TGCompositeFrame(PSDOptionsTab_CF, 1, 1, kVerticalFrame));
  PSDOptions_CF->Resize(TabFrameWidth, TabFrameLength);
  PSDOptions_CF->ChangeOptions(PSDOptions_CF->GetOptions() | kFixedSize);

  // The tabbed frame to hold graphical widgets
  GraphicsOptionsTab_CF = OptionsTabs_T->AddTab("Graphics");
  GraphicsOptionsTab_CF->AddFrame(GraphicsOptions_CF = new TGCompositeFrame(GraphicsOptionsTab_CF, 1, 1, kVerticalFrame));
  GraphicsOptions_CF->Resize(TabFrameWidth, TabFrameLength);
  GraphicsOptions_CF->ChangeOptions(GraphicsOptions_CF->GetOptions() | kFixedSize);

  // The tabbed frame to hold parallel processing widgets
  ProcessingOptionsTab_CF = OptionsTabs_T->AddTab("Processing");
  ProcessingOptionsTab_CF->AddFrame(ProcessingOptions_CF = new TGCompositeFrame(ProcessingOptionsTab_CF, 1, 1, kVerticalFrame));
  ProcessingOptions_CF->Resize(TabFrameWidth, TabFrameLength);
  ProcessingOptions_CF->ChangeOptions(ProcessingOptions_CF->GetOptions() | kFixedSize);

  FillWaveformFrame();
  FillSpectrumFrame();
  FillAnalysisFrame();
  FillPSDFrame();
  FillGraphicsFrame();
  FillProcessingFrame();
  FillCanvasFrame();

  //////////////////////////////////////
  // Finalize options and map windows //
  //////////////////////////////////////
  
  SetBackgroundColor(ThemeBackgroundColor);
  
  // Create a string that will be located in the title bar of the
  // ADAQAnalysisGUI to identify the version number of the code
  string TitleString;
  if(VersionString == "Development")
    TitleString = "ADAQAnalysis (Development version)               Fear is the mind-killer.";
  else
    TitleString = "ADAQAnalysis (Production version " + VersionString + ")               Fear is the mind-killer.";

  SetWindowName(TitleString.c_str());
  MapSubwindows();
  Resize(TotalX, TotalY);
  MapWindow();
}


void AAInterface::FillWaveformFrame()
{
  /////////////////////////////////////////
  // Fill the waveform options tabbed frame
  
  TGCanvas *WaveformFrame_C = new TGCanvas(WaveformOptions_CF, TabFrameWidth-10, TabFrameLength-20, kDoubleBorder);
  WaveformOptions_CF->AddFrame(WaveformFrame_C, new TGLayoutHints(kLHintsCenterX, 0,0,10,10));
  
  TGVerticalFrame *WaveformFrame_VF = new TGVerticalFrame(WaveformFrame_C->GetViewPort(), TabFrameWidth-10, TabFrameLength);
  WaveformFrame_C->SetContainer(WaveformFrame_VF);
  
  TGHorizontalFrame *WaveformSelection_HF = new TGHorizontalFrame(WaveformFrame_VF);
  WaveformFrame_VF->AddFrame(WaveformSelection_HF, new TGLayoutHints(kLHintsLeft, 15,5,15,5));
  
  WaveformSelection_HF->AddFrame(ChannelSelector_CBL = new ADAQComboBoxWithLabel(WaveformSelection_HF, "", ChannelSelector_CBL_ID),
				 new TGLayoutHints(kLHintsLeft, 0,5,5,5));
  stringstream ss;
  string entry;
  for(int ch=0; ch<NumDataChannels; ch++){
    ss << ch;
    entry.assign("Channel " + ss.str());
    ChannelSelector_CBL->GetComboBox()->AddEntry(entry.c_str(),ch);
    ss.str("");
  }
  ChannelSelector_CBL->GetComboBox()->Connect("Selected(int,int)", "AAWaveformSlots", WaveformSlots, "HandleComboBoxes(int,int)");
  ChannelSelector_CBL->GetComboBox()->Select(0);

  WaveformSelection_HF->AddFrame(WaveformSelector_NEL = new ADAQNumberEntryWithLabel(WaveformSelection_HF, "Waveform", WaveformSelector_NEL_ID),
				 new TGLayoutHints(kLHintsLeft, 5,5,5,5));
  WaveformSelector_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  WaveformSelector_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  WaveformSelector_NEL->GetEntry()->Connect("ValueSet(long)", "AAWaveformSlots", WaveformSlots, "HandleNumberEntries()");
  
  // Waveform specification (type and polarity)

  TGHorizontalFrame *WaveformSpecification_HF = new TGHorizontalFrame(WaveformFrame_VF);
  WaveformFrame_VF->AddFrame(WaveformSpecification_HF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  
  WaveformSpecification_HF->AddFrame(WaveformType_BG = new TGButtonGroup(WaveformSpecification_HF, "Type", kVerticalFrame),
				     new TGLayoutHints(kLHintsLeft, 0,5,0,5));
  RawWaveform_RB = new TGRadioButton(WaveformType_BG, "Raw voltage", RawWaveform_RB_ID);
  RawWaveform_RB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleRadioButtons()");
  RawWaveform_RB->SetState(kButtonDown);
  
  BaselineSubtractedWaveform_RB = new TGRadioButton(WaveformType_BG, "Baseline-subtracted", BaselineSubtractedWaveform_RB_ID);
  BaselineSubtractedWaveform_RB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleRadioButtons()");

  ZeroSuppressionWaveform_RB = new TGRadioButton(WaveformType_BG, "Zero suppression", ZeroSuppressionWaveform_RB_ID);
  ZeroSuppressionWaveform_RB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleRadioButtons()");

  WaveformSpecification_HF->AddFrame(WaveformPolarity_BG = new TGButtonGroup(WaveformSpecification_HF, "Polarity", kVerticalFrame),
				     new TGLayoutHints(kLHintsLeft, 5,5,0,5));
  
  PositiveWaveform_RB = new TGRadioButton(WaveformPolarity_BG, "Positive", PositiveWaveform_RB_ID);
  PositiveWaveform_RB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleRadioButtons()");
  
  NegativeWaveform_RB = new TGRadioButton(WaveformPolarity_BG, "Negative", NegativeWaveform_RB_ID);
  NegativeWaveform_RB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleRadioButtons()");
  NegativeWaveform_RB->SetState(kButtonDown);

TGGroupFrame *PeakFindingOptions_GF = new TGGroupFrame(WaveformFrame_VF, "Peak finding options", kVerticalFrame);
  WaveformFrame_VF->AddFrame(PeakFindingOptions_GF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  
  TGHorizontalFrame *PeakFinding_HF0 = new TGHorizontalFrame(PeakFindingOptions_GF);
  PeakFindingOptions_GF->AddFrame(PeakFinding_HF0, new TGLayoutHints(kLHintsLeft, 0,0,0,5));
  
  PeakFinding_HF0->AddFrame(FindPeaks_CB = new TGCheckButton(PeakFinding_HF0, "Find peaks", FindPeaks_CB_ID),
			    new TGLayoutHints(kLHintsLeft, 5,5,5,0));
  FindPeaks_CB->SetState(kButtonDisabled);
  FindPeaks_CB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleCheckButtons()");
  
  PeakFinding_HF0->AddFrame(UseMarkovSmoothing_CB = new TGCheckButton(PeakFinding_HF0, "Use Markov smoothing", UseMarkovSmoothing_CB_ID),
			    new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  UseMarkovSmoothing_CB->SetState(kButtonDown);
  UseMarkovSmoothing_CB->SetState(kButtonDisabled);
  UseMarkovSmoothing_CB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleCheckButtons()");


  TGHorizontalFrame *PeakFinding_HF1 = new TGHorizontalFrame(PeakFindingOptions_GF);
  PeakFindingOptions_GF->AddFrame(PeakFinding_HF1, new TGLayoutHints(kLHintsLeft, 0,0,0,5));

  PeakFinding_HF1->AddFrame(MaxPeaks_NEL = new ADAQNumberEntryWithLabel(PeakFinding_HF1, "Max. peaks", MaxPeaks_NEL_ID),
			    new TGLayoutHints(kLHintsLeft, 5,5,0,0));
  MaxPeaks_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  MaxPeaks_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  MaxPeaks_NEL->GetEntry()->SetNumber(1);
  MaxPeaks_NEL->GetEntry()->Resize(65, 20);
  MaxPeaks_NEL->GetEntry()->SetState(false);
  MaxPeaks_NEL->GetEntry()->Connect("ValueSet(long)", "AAWaveformSlots", WaveformSlots, "HandleNumberEntries()");
  
  PeakFinding_HF1->AddFrame(Sigma_NEL = new ADAQNumberEntryWithLabel(PeakFinding_HF1, "Sigma", Sigma_NEL_ID),
			    new TGLayoutHints(kLHintsLeft, 10,5,0,0));
  Sigma_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  Sigma_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  Sigma_NEL->GetEntry()->SetNumber(5);
  Sigma_NEL->GetEntry()->Resize(65, 20);
  Sigma_NEL->GetEntry()->SetState(false);
  Sigma_NEL->GetEntry()->Connect("ValueSet(long)", "AAWaveformSlots", WaveformSlots, "HandleNumberEntries()");


  TGHorizontalFrame *PeakFinding_HF2 = new TGHorizontalFrame(PeakFindingOptions_GF);
  PeakFindingOptions_GF->AddFrame(PeakFinding_HF2, new TGLayoutHints(kLHintsLeft, 0,0,0,5));

  PeakFinding_HF2->AddFrame(Resolution_NEL = new ADAQNumberEntryWithLabel(PeakFinding_HF2, "Resolution", Resolution_NEL_ID),
			    new TGLayoutHints(kLHintsLeft, 5,5,0,0));
  Resolution_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  Resolution_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  Resolution_NEL->GetEntry()->SetNumber(0.005);
  Resolution_NEL->GetEntry()->Resize(65, 20);
  Resolution_NEL->GetEntry()->SetState(false);
  Resolution_NEL->GetEntry()->Connect("ValueSet(long)", "AAWaveformSlots", WaveformSlots, "HandleNumberEntries()");
  
  PeakFinding_HF2->AddFrame(Floor_NEL = new ADAQNumberEntryWithLabel(PeakFinding_HF2, "Floor", Floor_NEL_ID),
			    new TGLayoutHints(kLHintsLeft, 10,5,0,0));
  Floor_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  Floor_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  Floor_NEL->GetEntry()->SetNumber(50);
  Floor_NEL->GetEntry()->Resize(65, 20);
  Floor_NEL->GetEntry()->SetState(false);
  Floor_NEL->GetEntry()->Connect("ValueSet(long)", "AAWaveformSlots", WaveformSlots, "HandleNumberEntries()");

  
  TGVerticalFrame *PeakFinding_VF0 = new TGVerticalFrame(PeakFindingOptions_GF);
  PeakFindingOptions_GF->AddFrame(PeakFinding_VF0, new TGLayoutHints(kLHintsLeft, 5,5,0,0));

  PeakFinding_VF0->AddFrame(PlotFloor_CB = new TGCheckButton(PeakFinding_VF0, "Plot the floor", PlotFloor_CB_ID),
			    new TGLayoutHints(kLHintsLeft, 0,0,5,0));
  PlotFloor_CB->SetState(kButtonDisabled);
  PlotFloor_CB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleCheckButtons()");
  
  PeakFinding_VF0->AddFrame(PlotCrossings_CB = new TGCheckButton(PeakFinding_VF0, "Plot waveform intersections", PlotCrossings_CB_ID),
			    new TGLayoutHints(kLHintsLeft, 0,0,5,0));
  PlotCrossings_CB->SetState(kButtonDisabled);
  PlotCrossings_CB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleCheckButtons()");
  
  PeakFinding_VF0->AddFrame(PlotPeakIntegratingRegion_CB = new TGCheckButton(PeakFinding_VF0, "Plot integration region", PlotPeakIntegratingRegion_CB_ID),
			    new TGLayoutHints(kLHintsLeft, 0,0,5,0));
  PlotPeakIntegratingRegion_CB->SetState(kButtonDisabled);
  PlotPeakIntegratingRegion_CB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleCheckButtons()");


  //////////////////////////////
  // Waveform analysis region //
  //////////////////////////////

  WaveformFrame_VF->AddFrame(new TGLabel(WaveformFrame_VF, "Waveform analysis region (samples)"),
			     new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  
  TGHorizontalFrame *AnalysisRegion_HF = new TGHorizontalFrame(WaveformFrame_VF);
  WaveformFrame_VF->AddFrame(AnalysisRegion_HF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));

  AnalysisRegion_HF->AddFrame(AnalysisRegionMin_NEL = new ADAQNumberEntryWithLabel(AnalysisRegion_HF, "Min", AnalysisRegionMin_NEL_ID),
			      new TGLayoutHints(kLHintsLeft, 0,5,0,0));
  AnalysisRegionMin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  AnalysisRegionMin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  AnalysisRegionMin_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  AnalysisRegionMin_NEL->GetEntry()->SetLimitValues(0,1); // Set when ADAQRootFile loaded
  AnalysisRegionMin_NEL->GetEntry()->SetNumber(0); // Set when ADAQRootFile loaded
  AnalysisRegionMin_NEL->GetEntry()->Resize(55, 20);
  AnalysisRegionMin_NEL->GetEntry()->Connect("ValueSet(long)", "AAWaveformSlots", WaveformSlots, "HandleNumberEntries()");
  
  AnalysisRegion_HF->AddFrame(AnalysisRegionMax_NEL = new ADAQNumberEntryWithLabel(AnalysisRegion_HF, "Max", AnalysisRegionMax_NEL_ID),
			      new TGLayoutHints(kLHintsLeft, 0,5,0,5)); 
  AnalysisRegionMax_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  AnalysisRegionMax_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  AnalysisRegionMax_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  AnalysisRegionMax_NEL->GetEntry()->SetLimitValues(1,2); // Set When ADAQRootFile loaded
  AnalysisRegionMax_NEL->GetEntry()->SetNumber(1); // Set when ADAQRootFile loaded
  AnalysisRegionMax_NEL->GetEntry()->Resize(55, 20);
  AnalysisRegionMax_NEL->GetEntry()->Connect("ValueSet(long)", "AAWaveformSlots", WaveformSlots, "HandleNumberEntries()");
  
  AnalysisRegion_HF->AddFrame(PlotAnalysisRegion_CB = new TGCheckButton(AnalysisRegion_HF, "Plot", PlotAnalysisRegion_CB_ID),
			      new TGLayoutHints(kLHintsLeft, 5,5,5,0));
  PlotAnalysisRegion_CB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleCheckButtons()");
  

  ////////////////////////
  // Waveform baseline  //
  ////////////////////////

  WaveformFrame_VF->AddFrame(new TGLabel(WaveformFrame_VF, "Waveform baseline region (samples)"),
			     new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  
  TGHorizontalFrame *BaselineRegion_HF = new TGHorizontalFrame(WaveformFrame_VF);
  WaveformFrame_VF->AddFrame(BaselineRegion_HF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  
  BaselineRegion_HF->AddFrame(BaselineRegionMin_NEL = new ADAQNumberEntryWithLabel(BaselineRegion_HF, "Min", BaselineRegionMin_NEL_ID),
			      new TGLayoutHints(kLHintsLeft, 0,5,0,0));
  BaselineRegionMin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  BaselineRegionMin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  BaselineRegionMin_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  BaselineRegionMin_NEL->GetEntry()->SetLimitValues(0,1); // Set when ADAQRootFile loaded
  BaselineRegionMin_NEL->GetEntry()->SetNumber(0); // Set when ADAQRootFile loaded
  BaselineRegionMin_NEL->GetEntry()->Resize(55, 20);
  BaselineRegionMin_NEL->GetEntry()->Connect("ValueSet(long)", "AAWaveformSlots", WaveformSlots, "HandleNumberEntries()");
  
  BaselineRegion_HF->AddFrame(BaselineRegionMax_NEL = new ADAQNumberEntryWithLabel(BaselineRegion_HF, "Max", BaselineRegionMax_NEL_ID),
			      new TGLayoutHints(kLHintsLeft, 0,5,0,5)); 
  BaselineRegionMax_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  BaselineRegionMax_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  BaselineRegionMax_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  BaselineRegionMax_NEL->GetEntry()->SetLimitValues(1,2); // Set When ADAQRootFile loaded
  BaselineRegionMax_NEL->GetEntry()->SetNumber(1); // Set when ADAQRootFile loaded
  BaselineRegionMax_NEL->GetEntry()->Resize(55, 20);
  BaselineRegionMax_NEL->GetEntry()->Connect("ValueSet(long)", "AAWaveformSlots", WaveformSlots, "HandleNumberEntries()");
  
  BaselineRegion_HF->AddFrame(PlotBaselineRegion_CB = new TGCheckButton(BaselineRegion_HF, "Plot", PlotBaselineRegion_CB_ID),
			      new TGLayoutHints(kLHintsLeft, 5,5,5,0));
  PlotBaselineRegion_CB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleCheckButtons()");
    

  //////////////////////////////
  // Zero suppression options //
  //////////////////////////////
  
  WaveformFrame_VF->AddFrame(new TGLabel(WaveformFrame_VF, "Zero suppression settings (ADC, sample)"),
			     new TGLayoutHints(kLHintsLeft, 15,5,5,0));

  TGHorizontalFrame *ZLE_HF = new TGHorizontalFrame(WaveformFrame_VF);
  WaveformFrame_VF->AddFrame(ZLE_HF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  
  ZLE_HF->AddFrame(ZeroSuppressionCeiling_NEL = new ADAQNumberEntryWithLabel(ZLE_HF, "Ceiling", ZeroSuppressionCeiling_NEL_ID),
		   new TGLayoutHints(kLHintsLeft, 0,5,0,0));
  ZeroSuppressionCeiling_NEL->GetEntry()->SetNumber(15);
  ZeroSuppressionCeiling_NEL->GetEntry()->Resize(55, 20);
  ZeroSuppressionCeiling_NEL->GetEntry()->Connect("ValueSet(long)", "AAWaveformSlots", WaveformSlots, "HandleNumberEntries()");
  
  ZLE_HF->AddFrame(ZeroSuppressionBuffer_NEL = new ADAQNumberEntryWithLabel(ZLE_HF, "Buffer", ZeroSuppressionBuffer_NEL_ID),
		   new TGLayoutHints(kLHintsLeft, 0,5,0,5));
  ZeroSuppressionBuffer_NEL->GetEntry()->SetNumber(15);
  ZeroSuppressionBuffer_NEL->GetEntry()->Resize(55, 20);
  ZeroSuppressionBuffer_NEL->GetEntry()->Connect("ValueSet(long)", "AAWaveformSlots", WaveformSlots, "HandleNumberEntries()");

  ZLE_HF->AddFrame(PlotZeroSuppressionCeiling_CB = new TGCheckButton(ZLE_HF, "Plot", PlotZeroSuppressionCeiling_CB_ID),
		   new TGLayoutHints(kLHintsLeft, 5,5,5,0));
  PlotZeroSuppressionCeiling_CB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleCheckButtons()");
  

  /////////////////////
  // Trigger options //
  /////////////////////
  
  TGHorizontalFrame *Trigger_HF = new TGHorizontalFrame(WaveformFrame_VF);
  WaveformFrame_VF->AddFrame(Trigger_HF, new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  
  Trigger_HF->AddFrame(TriggerLevel_NEFL = new ADAQNumberEntryFieldWithLabel(Trigger_HF, "Trigger (ADC)  ", -1),
		       new TGLayoutHints(kLHintsNormal, 0,5,0,5));
  TriggerLevel_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESInteger, TGNumberFormat::kNEAAnyNumber);
  TriggerLevel_NEFL->GetEntry()->Resize(70, 20);
  TriggerLevel_NEFL->GetEntry()->SetState(false);
  
  Trigger_HF->AddFrame(PlotTrigger_CB = new TGCheckButton(Trigger_HF, "Plot", PlotTrigger_CB_ID),
		       new TGLayoutHints(kLHintsLeft, 5,5,5,0));
  PlotTrigger_CB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleCheckButtons()");
  

  ///////////////////////
  // Selection options //
  ///////////////////////

  TGHorizontalFrame *Selection_HF = new TGHorizontalFrame(WaveformFrame_VF);
  WaveformFrame_VF->AddFrame(Selection_HF, new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  
  Selection_HF->AddFrame(UsePileupRejection_CB = new TGCheckButton(Selection_HF, "Use pileup rejection", UsePileupRejection_CB_ID),
			 new TGLayoutHints(kLHintsNormal, 0,5,0,5));
  UsePileupRejection_CB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleCheckButtons()");
  UsePileupRejection_CB->SetState(kButtonUp);
  
  Selection_HF->AddFrame(UsePSDRejection_CB = new TGCheckButton(Selection_HF, "Use PSD rejection", UsePSDRejection_CB_ID),
			 new TGLayoutHints(kLHintsNormal, 0,5,0,5));
  UsePSDRejection_CB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleCheckButtons()");
  UsePSDRejection_CB->SetState(kButtonUp);
  
  
  ///////////////////////
  // Graphical options //
  ///////////////////////
  
  WaveformFrame_VF->AddFrame(AutoYAxisRange_CB = new TGCheckButton(WaveformFrame_VF, "Auto. Y Axis Range", AutoYAxisRange_CB_ID),
			     new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  AutoYAxisRange_CB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleCheckButtons()");
  			       

  ///////////////////////
  // Waveform analysis //
  ///////////////////////
  
  TGGroupFrame *WaveformAnalysis_GF = new TGGroupFrame(WaveformFrame_VF, "Waveform analysis", kVerticalFrame);
  WaveformFrame_VF->AddFrame(WaveformAnalysis_GF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  
  WaveformAnalysis_GF->AddFrame(WaveformAnalysis_CB = new TGCheckButton(WaveformAnalysis_GF, "Analyze waveform", WaveformAnalysis_CB_ID),
				new TGLayoutHints(kLHintsLeft, 0,5,5,5));
  WaveformAnalysis_CB->SetState(kButtonDisabled);
  WaveformAnalysis_CB->Connect("Clicked()", "AAWaveformSlots", WaveformSlots, "HandleCheckButtons()");
  
  WaveformAnalysis_GF->AddFrame(WaveformIntegral_NEL = new ADAQNumberEntryWithLabel(WaveformAnalysis_GF, "Integral (ADC)", -1),
				new TGLayoutHints(kLHintsLeft, 0,5,0,0));
  WaveformIntegral_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  WaveformIntegral_NEL->GetEntry()->SetNumber(0.);
  WaveformIntegral_NEL->GetEntry()->SetState(false);
  WaveformIntegral_NEL->GetEntry()->Resize(125,20);
  
  WaveformAnalysis_GF->AddFrame(WaveformHeight_NEL = new ADAQNumberEntryWithLabel(WaveformAnalysis_GF, "Height (ADC)", -1),
				new TGLayoutHints(kLHintsLeft, 0,5,0,0));
  WaveformHeight_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  WaveformHeight_NEL->GetEntry()->SetNumber(0.);
  WaveformHeight_NEL->GetEntry()->SetState(false);
  WaveformHeight_NEL->GetEntry()->Resize(125,20);
}


void AAInterface::FillSpectrumFrame()
{
  /////////////////////////////////////////
  // Fill the spectrum options tabbed frame
  
  TGCanvas *SpectrumFrame_C = new TGCanvas(SpectrumOptions_CF, TabFrameWidth-10, TabFrameLength-20, kDoubleBorder);
  SpectrumOptions_CF->AddFrame(SpectrumFrame_C, new TGLayoutHints(kLHintsCenterX, 0,0,10,10));
  
  TGVerticalFrame *SpectrumFrame_VF = new TGVerticalFrame(SpectrumFrame_C->GetViewPort(), TabFrameWidth-10, TabFrameLength);
  SpectrumFrame_C->SetContainer(SpectrumFrame_VF);
  
  Int_t LOffset = 15;
  
  SpectrumFrame_VF->AddFrame(new TGLabel(SpectrumFrame_VF, "Histogram information"),
			     new TGLayoutHints(kLHintsNormal, LOffset,0,0,0));
  
  TGHorizontalFrame *WaveformAndBins_HF = new TGHorizontalFrame(SpectrumFrame_VF);
  SpectrumFrame_VF->AddFrame(WaveformAndBins_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));

  // Number of waveforms to bin in the histogram
  WaveformAndBins_HF->AddFrame(WaveformsToHistogram_NEL = new ADAQNumberEntryWithLabel(WaveformAndBins_HF, "Waveforms", WaveformsToHistogram_NEL_ID),
			       new TGLayoutHints(kLHintsLeft, LOffset,0,8,5));
  WaveformsToHistogram_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  WaveformsToHistogram_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  WaveformsToHistogram_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  WaveformsToHistogram_NEL->GetEntry()->SetLimitValues(1,2);
  WaveformsToHistogram_NEL->GetEntry()->SetNumber(0);
  
  // Number of spectrum bins number entry
  WaveformAndBins_HF->AddFrame(SpectrumNumBins_NEL = new ADAQNumberEntryWithLabel(WaveformAndBins_HF, "Bins", SpectrumNumBins_NEL_ID),
			       new TGLayoutHints(kLHintsLeft, 8,15,8,5));
  SpectrumNumBins_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  SpectrumNumBins_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  SpectrumNumBins_NEL->GetEntry()->SetNumber(100);

  SpectrumFrame_VF->AddFrame(new TGLabel(SpectrumFrame_VF, "Limits"),
			     new TGLayoutHints(kLHintsNormal, LOffset,0,0,0));
  
  TGHorizontalFrame *SpectrumBinning_HF = new TGHorizontalFrame(SpectrumFrame_VF);
  SpectrumFrame_VF->AddFrame(SpectrumBinning_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  
  // Minimum spectrum bin number entry
  SpectrumBinning_HF->AddFrame(SpectrumMinBin_NEL = new ADAQNumberEntryWithLabel(SpectrumBinning_HF, "Mininum  ", SpectrumMinBin_NEL_ID),
			       new TGLayoutHints(kLHintsLeft, LOffset,0,5,0));
  SpectrumMinBin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumMinBin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  SpectrumMinBin_NEL->GetEntry()->SetNumber(0);
  SpectrumMinBin_NEL->GetEntry()->Connect("ValueSet(long)", "AASpectrumSlots", SpectrumSlots, "HandleNumberEntries()");
  
  // Maximum spectrum bin number entry
  SpectrumBinning_HF->AddFrame(SpectrumMaxBin_NEL = new ADAQNumberEntryWithLabel(SpectrumBinning_HF, "Maximum", SpectrumMaxBin_NEL_ID),
			       new TGLayoutHints(kLHintsRight, 15,0,5,0));
  SpectrumMaxBin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumMaxBin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumMaxBin_NEL->GetEntry()->SetNumber(50000);
  SpectrumMaxBin_NEL->GetEntry()->Connect("ValueSet(long)", "AASpectrumSlots", SpectrumSlots, "HandleNumberEntries()");
  
  SpectrumFrame_VF->AddFrame(new TGLabel(SpectrumFrame_VF, "Thresholds"),
			     new TGLayoutHints(kLHintsNormal, LOffset, 0, 5, 0));
  
  TGHorizontalFrame *SpectrumThresholds_HF = new TGHorizontalFrame(SpectrumFrame_VF);
  SpectrumFrame_VF->AddFrame(SpectrumThresholds_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  
  // Minimum/lower threshold for histogramming
  SpectrumThresholds_HF->AddFrame(SpectrumMinThresh_NEL = new ADAQNumberEntryWithLabel(SpectrumThresholds_HF, "Mininum  ", SpectrumMinThresh_NEL_ID),
				  new TGLayoutHints(kLHintsLeft, LOffset,0,5,0));
  SpectrumMinThresh_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumMinThresh_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumMinThresh_NEL->GetEntry()->SetNumber(0);
  SpectrumMinThresh_NEL->GetEntry()->Connect("ValueSet(long)", "AASpectrumSlots", SpectrumSlots, "HandleNumberEntries()");
  
  // Maximum/upper threshold for histogramming
  SpectrumThresholds_HF->AddFrame(SpectrumMaxThresh_NEL = new ADAQNumberEntryWithLabel(SpectrumThresholds_HF, "Maximum", SpectrumMaxThresh_NEL_ID),
				  new TGLayoutHints(kLHintsRight, 15,0,5,0));
  SpectrumMaxThresh_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumMaxThresh_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumMaxThresh_NEL->GetEntry()->SetNumber(50000);
  SpectrumMaxThresh_NEL->GetEntry()->Connect("ValueSet(long)", "AASpectrumSlots", SpectrumSlots, "HandleNumberEntries()");
  
  
  ///////////////////////
  // ADAQ spectra options

  TGGroupFrame *ADAQSpectrumOptions_GF = new TGGroupFrame(SpectrumFrame_VF, "Experimental spectra (ADAQ)", kHorizontalFrame);
  SpectrumFrame_VF->AddFrame(ADAQSpectrumOptions_GF, new TGLayoutHints(kLHintsLeft, 15,0,10,0));

  TGVerticalFrame *ADAQSpectrumQuantities_VF = new TGVerticalFrame(ADAQSpectrumOptions_GF);
  ADAQSpectrumOptions_GF->AddFrame(ADAQSpectrumQuantities_VF, new TGLayoutHints(kLHintsNormal, 5,15,5,0));

  // Spectrum quantity radio buttons

  ADAQSpectrumQuantities_VF->AddFrame(new TGLabel(ADAQSpectrumQuantities_VF, "Quantity"),
				      new TGLayoutHints(kLHintsNormal, 0,0,0,0));  

  ADAQSpectrumQuantities_VF->AddFrame(ADAQSpectrumTypePAS_RB = new TGRadioButton(ADAQSpectrumQuantities_VF, "Pulse area", ADAQSpectrumTypePAS_RB_ID),
				      new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  ADAQSpectrumTypePAS_RB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleRadioButtons()");
  ADAQSpectrumTypePAS_RB->SetState(kButtonDown);
  
  ADAQSpectrumQuantities_VF->AddFrame(ADAQSpectrumTypePHS_RB = new TGRadioButton(ADAQSpectrumQuantities_VF, "Pulse height", ADAQSpectrumTypePHS_RB_ID),
				      new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  ADAQSpectrumTypePHS_RB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleRadioButtons()");

  // Spectrum algorithm

  TGVerticalFrame *ADAQSpectrumAlgorithm_VF = new TGVerticalFrame(ADAQSpectrumOptions_GF);
  ADAQSpectrumOptions_GF->AddFrame(ADAQSpectrumAlgorithm_VF, new TGLayoutHints(kLHintsNormal, 15,5,5,0));

  ADAQSpectrumAlgorithm_VF->AddFrame(new TGLabel(ADAQSpectrumAlgorithm_VF, "Algorithm"),
				new TGLayoutHints(kLHintsNormal, 0,0,0,0));  
  
  ADAQSpectrumAlgorithm_VF->AddFrame(ADAQSpectrumAlgorithmSMS_RB = new TGRadioButton(ADAQSpectrumAlgorithm_VF, "Simple max/sum", ADAQSpectrumAlgorithmSMS_RB_ID),
				     new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  ADAQSpectrumAlgorithmSMS_RB->SetState(kButtonDown);
  ADAQSpectrumAlgorithmSMS_RB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleRadioButtons()");
  
  ADAQSpectrumAlgorithm_VF->AddFrame(ADAQSpectrumAlgorithmPF_RB = new TGRadioButton(ADAQSpectrumAlgorithm_VF, "Peak finder", ADAQSpectrumAlgorithmPF_RB_ID),
				     new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  ADAQSpectrumAlgorithmPF_RB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleRadioButtons()");
  
  ADAQSpectrumAlgorithm_VF->AddFrame(ADAQSpectrumAlgorithmWD_RB = new TGRadioButton(ADAQSpectrumAlgorithm_VF, "Waveform data", ADAQSpectrumAlgorithmWD_RB_ID),
				     new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  ADAQSpectrumAlgorithmWD_RB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleRadioButtons()");
  

  //////////////////////////
  // ASIM spectra options

  TGGroupFrame *ASIMSpectrumOptions_GF = new TGGroupFrame(SpectrumFrame_VF, "Simulated spectra (ASIM)", kHorizontalFrame);
  SpectrumFrame_VF->AddFrame(ASIMSpectrumOptions_GF, new TGLayoutHints(kLHintsLeft, 15,0,5,0));

  TGVerticalFrame *ASIMSpectrumQuantities_VF = new TGVerticalFrame(ASIMSpectrumOptions_GF);
  ASIMSpectrumOptions_GF->AddFrame(ASIMSpectrumQuantities_VF, new TGLayoutHints(kLHintsNormal, 5,15,5,0));

  // Spectrum type radio buttons

  ASIMSpectrumQuantities_VF->AddFrame(new TGLabel(ASIMSpectrumQuantities_VF, "Type"),
				      new TGLayoutHints(kLHintsNormal, 0,0,0,0));  
  
  ASIMSpectrumQuantities_VF->AddFrame(ASIMSpectrumTypeEnergy_RB = new TGRadioButton(ASIMSpectrumQuantities_VF, "Energy dep.", ASIMSpectrumTypeEnergy_RB_ID),
				      new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  ASIMSpectrumTypeEnergy_RB->SetState(kButtonDown);
  ASIMSpectrumTypeEnergy_RB->SetState(kButtonDisabled);
  ASIMSpectrumTypeEnergy_RB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleRadioButtons()");
  
  ASIMSpectrumQuantities_VF->AddFrame(ASIMSpectrumTypePhotonsCreated_RB = new TGRadioButton(ASIMSpectrumQuantities_VF, "Photons cre.", ASIMSpectrumTypePhotonsCreated_RB_ID),
				      new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  ASIMSpectrumTypePhotonsCreated_RB->SetState(kButtonDisabled);
  ASIMSpectrumTypePhotonsCreated_RB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleRadioButtons()");

  ASIMSpectrumQuantities_VF->AddFrame(ASIMSpectrumTypePhotonsDetected_RB = new TGRadioButton(ASIMSpectrumQuantities_VF, "Photons det.", ASIMSpectrumTypePhotonsDetected_RB_ID),
				      new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  ASIMSpectrumTypePhotonsDetected_RB->SetState(kButtonDisabled);
  ASIMSpectrumTypePhotonsDetected_RB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleRadioButtons()");
				      
  // Detector type radio buttons

  TGVerticalFrame *ASIMEventTree_VF = new TGVerticalFrame(ASIMSpectrumOptions_GF);
  ASIMSpectrumOptions_GF->AddFrame(ASIMEventTree_VF, new TGLayoutHints(kLHintsNormal, 5,5,5,0));

  ASIMEventTree_VF->AddFrame(new TGLabel(ASIMEventTree_VF, "Event Tree"),
			     new TGLayoutHints(kLHintsNormal, 0,0,0,0));  
  
  ASIMEventTree_VF->AddFrame(ASIMEventTree_CB = new TGComboBox(ASIMEventTree_VF, ASIMEventTree_CB_ID),
			     new TGLayoutHints(kLHintsNormal, 0,0,0,0));  
  ASIMEventTree_CB->AddEntry("None available!",0);
  ASIMEventTree_CB->Select(0);
  ASIMEventTree_CB->Resize(120,20);
  ASIMEventTree_CB->SetEnabled(false);
  ASIMEventTree_CB->Connect("Selected(int,int)", "AASpectrumSlots", SpectrumSlots, "HandleComboBoxes(int,int)");
  

  //////////////////////////////
  // Spectrum energy calibration

  TGGroupFrame *SpectrumCalibration_GF = new TGGroupFrame(SpectrumFrame_VF, "Energy calibration", kVerticalFrame);
  SpectrumFrame_VF->AddFrame(SpectrumCalibration_GF, new TGLayoutHints(kLHintsLeft, 15,5,10,0));

  TGHorizontalFrame *SpectrumCalibration_HF0 = new TGHorizontalFrame(SpectrumCalibration_GF);
  SpectrumCalibration_GF->AddFrame(SpectrumCalibration_HF0, new TGLayoutHints(kLHintsNormal, 0,0,0,0));

  // Energy calibration 
  SpectrumCalibration_HF0->AddFrame(SpectrumCalibration_CB = new TGCheckButton(SpectrumCalibration_HF0, "Make it so!", SpectrumCalibration_CB_ID),
				    new TGLayoutHints(kLHintsLeft, 0,0,10,0));
  SpectrumCalibration_CB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleCheckButtons()");
  SpectrumCalibration_CB->SetState(kButtonUp);

  // Load from file text button
  SpectrumCalibration_HF0->AddFrame(SpectrumCalibrationLoad_TB = new TGTextButton(SpectrumCalibration_HF0, "Load from file", SpectrumCalibrationLoad_TB_ID),
				    new TGLayoutHints(kLHintsRight, 15,0,5,5));
  SpectrumCalibrationLoad_TB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleTextButtons()");
  SpectrumCalibrationLoad_TB->Resize(120,25);
  SpectrumCalibrationLoad_TB->ChangeOptions(SpectrumCalibrationLoad_TB->GetOptions() | kFixedSize);
  SpectrumCalibrationLoad_TB->SetState(kButtonDisabled);

  TGHorizontalFrame *SpectrumCalibration_HF5 = new TGHorizontalFrame(SpectrumCalibration_GF);
  SpectrumCalibration_GF->AddFrame(SpectrumCalibration_HF5, new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  
  SpectrumCalibration_HF5->AddFrame(SpectrumCalibrationManualSlider_RB = new TGRadioButton(SpectrumCalibration_HF5, "Slider", SpectrumCalibrationManualSlider_RB_ID),
				    new TGLayoutHints(kLHintsNormal, 10,5,5,5));
  SpectrumCalibrationManualSlider_RB->SetState(kButtonDown);
  SpectrumCalibrationManualSlider_RB->SetState(kButtonDisabled);
  SpectrumCalibrationManualSlider_RB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleRadioButtons()");
  
  SpectrumCalibration_HF5->AddFrame(SpectrumCalibrationPeakFinder_RB = new TGRadioButton(SpectrumCalibration_HF5, "Peak finder", SpectrumCalibrationPeakFinder_RB_ID),
				    new TGLayoutHints(kLHintsNormal, 15,5,5,5));
  SpectrumCalibrationPeakFinder_RB->SetState(kButtonDisabled);
  SpectrumCalibrationPeakFinder_RB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleRadioButtons()");
  
  SpectrumCalibration_HF5->AddFrame(SpectrumCalibrationEdgeFinder_RB = new TGRadioButton(SpectrumCalibration_HF5, "Edge finder", SpectrumCalibrationEdgeFinder_RB_ID),
				    new TGLayoutHints(kLHintsNormal, 15,5,5,5));
  SpectrumCalibrationEdgeFinder_RB->SetState(kButtonDisabled);
  SpectrumCalibrationEdgeFinder_RB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleRadioButtons()");

  
  SpectrumCalibration_GF->AddFrame(SpectrumCalibrationType_CBL = new ADAQComboBoxWithLabel(SpectrumCalibration_GF, "", SpectrumCalibrationType_CBL_ID),
				   new TGLayoutHints(kLHintsNormal, 0,0,10,0));
  SpectrumCalibrationType_CBL->GetComboBox()->Resize(130,20);
  SpectrumCalibrationType_CBL->GetComboBox()->AddEntry("Linear fit",0);
  SpectrumCalibrationType_CBL->GetComboBox()->AddEntry("Quadratic fit",1);
  SpectrumCalibrationType_CBL->GetComboBox()->AddEntry("Lin. interp.",2);
  SpectrumCalibrationType_CBL->GetComboBox()->Select(0);
  SpectrumCalibrationType_CBL->GetComboBox()->SetEnabled(false);
  SpectrumCalibrationType_CBL->GetComboBox()->Connect("Selected(int,int)", "AASpectrumSlots", SpectrumSlots, "HandleComboBoxes(int,int)");

  
  TGHorizontalFrame *SpectrumCalibrationRange_HF = new TGHorizontalFrame(SpectrumCalibration_GF);
  SpectrumCalibration_GF->AddFrame(SpectrumCalibrationRange_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));

  SpectrumCalibrationRange_HF->AddFrame(SpectrumCalibrationMin_NEL = new ADAQNumberEntryWithLabel(SpectrumCalibrationRange_HF,
												  "Min (ADC)",
												  SpectrumCalibrationMin_NEL_ID),
					new TGLayoutHints(kLHintsLeft,0,5,0,0));
  SpectrumCalibrationMin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumCalibrationMin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  SpectrumCalibrationMin_NEL->GetEntry()->SetNumber(0.0);
  SpectrumCalibrationMin_NEL->GetEntry()->SetState(false);
  SpectrumCalibrationMin_NEL->GetEntry()->Resize(60,20);
  SpectrumCalibrationMin_NEL->GetEntry()->Connect("ValueSet(long)", "AASpectrumSlots", SpectrumSlots, "HandleNumberEntries()");
  
  SpectrumCalibrationRange_HF->AddFrame(SpectrumCalibrationMax_NEL = new ADAQNumberEntryWithLabel(SpectrumCalibrationRange_HF,
												  "Max (ADC)",
												  SpectrumCalibrationMax_NEL_ID),
					new TGLayoutHints(kLHintsLeft,0,0,0,0));
  SpectrumCalibrationMax_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumCalibrationMax_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  SpectrumCalibrationMax_NEL->GetEntry()->SetNumber(1.0);
  SpectrumCalibrationMax_NEL->GetEntry()->SetState(false);
  SpectrumCalibrationMax_NEL->GetEntry()->Resize(60,20);
  SpectrumCalibrationMax_NEL->GetEntry()->Connect("ValueSet(long)", "AASpectrumSlots", SpectrumSlots, "HandleNumberEntries()");


  TGHorizontalFrame *SpectrumCalibrationType_HF = new TGHorizontalFrame(SpectrumCalibration_GF);
  SpectrumCalibration_GF->AddFrame(SpectrumCalibrationType_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  
  SpectrumCalibrationType_HF->AddFrame(SpectrumCalibrationPoint_CBL = new ADAQComboBoxWithLabel(SpectrumCalibrationType_HF, "", SpectrumCalibrationPoint_CBL_ID),
				       new TGLayoutHints(kLHintsNormal, 0,0,10,0));
  SpectrumCalibrationPoint_CBL->GetComboBox()->Resize(171,20);
  SpectrumCalibrationPoint_CBL->GetComboBox()->AddEntry("Calibration point 0",0);
  SpectrumCalibrationPoint_CBL->GetComboBox()->Select(0);
  SpectrumCalibrationPoint_CBL->GetComboBox()->SetEnabled(false);
  SpectrumCalibrationPoint_CBL->GetComboBox()->Connect("Selected(int,int)", "AASpectrumSlots", SpectrumSlots, "HandleComboBoxes(int,int)");

  
  TGHorizontalFrame *SpectrumCalibration_HF6 = new TGHorizontalFrame(SpectrumCalibration_GF);
  SpectrumCalibration_GF->AddFrame(SpectrumCalibration_HF6, new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  
  SpectrumCalibration_HF6->AddFrame(SpectrumCalibrationEnergy_NEL = new ADAQNumberEntryWithLabel(SpectrumCalibration_HF6, "Energy", SpectrumCalibrationEnergy_NEL_ID),
				    new TGLayoutHints(kLHintsLeft,0,5,0,0));
  SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(0.0);
  SpectrumCalibrationEnergy_NEL->GetEntry()->SetState(false);
  SpectrumCalibrationEnergy_NEL->GetEntry()->Resize(80,20);
  SpectrumCalibrationEnergy_NEL->GetEntry()->Connect("ValueSet(long)", "AASpectrumSlots", SpectrumSlots, "HandleNumberEntries()");
  
  SpectrumCalibration_HF6->AddFrame(SpectrumCalibrationUnit_CBL = new ADAQComboBoxWithLabel(SpectrumCalibration_HF6, "", -1),
				    new TGLayoutHints(kLHintsLeft, 0,0,0,0));
  SpectrumCalibrationUnit_CBL->GetComboBox()->Resize(60, 20);
  SpectrumCalibrationUnit_CBL->GetComboBox()->AddEntry("keV", 0);
  SpectrumCalibrationUnit_CBL->GetComboBox()->AddEntry("MeV", 1);
  SpectrumCalibrationUnit_CBL->GetComboBox()->Select(1);
  SpectrumCalibrationUnit_CBL->GetComboBox()->SetEnabled(false);


  SpectrumCalibration_GF->AddFrame(SpectrumCalibrationPulseUnit_NEL = new ADAQNumberEntryWithLabel(SpectrumCalibration_GF, "Pulse unit (ADC)", SpectrumCalibrationPulseUnit_NEL_ID),
				       new TGLayoutHints(kLHintsLeft,0,0,0,5));
  SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(1.0);
  SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetState(false);
  SpectrumCalibrationPulseUnit_NEL->GetEntry()->Resize(80,20);
  SpectrumCalibrationPulseUnit_NEL->GetEntry()->Connect("ValueSet(long)", "AASpectrumSlots", SpectrumSlots, "HandleNumberEntries()");

  TGHorizontalFrame *SpectrumCalibration_HF1 = new TGHorizontalFrame(SpectrumCalibration_GF);
  SpectrumCalibration_GF->AddFrame(SpectrumCalibration_HF1);
  
  // Set point text button
  SpectrumCalibration_HF1->AddFrame(SpectrumCalibrationSetPoint_TB = new TGTextButton(SpectrumCalibration_HF1, "Set Pt.", SpectrumCalibrationSetPoint_TB_ID),
				    new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  SpectrumCalibrationSetPoint_TB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleTextButtons()");
  SpectrumCalibrationSetPoint_TB->Resize(100,25);
  SpectrumCalibrationSetPoint_TB->ChangeOptions(SpectrumCalibrationSetPoint_TB->GetOptions() | kFixedSize);
  SpectrumCalibrationSetPoint_TB->SetState(kButtonDisabled);

  // Calibrate text button
  SpectrumCalibration_HF1->AddFrame(SpectrumCalibrationCalibrate_TB = new TGTextButton(SpectrumCalibration_HF1, "Calibrate", SpectrumCalibrationCalibrate_TB_ID),
				    new TGLayoutHints(kLHintsNormal, 0,0,5,0));
  SpectrumCalibrationCalibrate_TB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleTextButtons()");
  SpectrumCalibrationCalibrate_TB->Resize(100,25);
  SpectrumCalibrationCalibrate_TB->ChangeOptions(SpectrumCalibrationCalibrate_TB->GetOptions() | kFixedSize);
  SpectrumCalibrationCalibrate_TB->SetState(kButtonDisabled);

  TGHorizontalFrame *SpectrumCalibration_HF2 = new TGHorizontalFrame(SpectrumCalibration_GF);
  SpectrumCalibration_GF->AddFrame(SpectrumCalibration_HF2);
  
  // Plot text button
  SpectrumCalibration_HF2->AddFrame(SpectrumCalibrationPlot_TB = new TGTextButton(SpectrumCalibration_HF2, "Plot", SpectrumCalibrationPlot_TB_ID),
				    new TGLayoutHints(kLHintsNormal, 5,5,5,5));
  SpectrumCalibrationPlot_TB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleTextButtons()");
  SpectrumCalibrationPlot_TB->Resize(100,25);
  SpectrumCalibrationPlot_TB->ChangeOptions(SpectrumCalibrationPlot_TB->GetOptions() | kFixedSize);
  SpectrumCalibrationPlot_TB->SetState(kButtonDisabled);

  // Reset text button
  SpectrumCalibration_HF2->AddFrame(SpectrumCalibrationReset_TB = new TGTextButton(SpectrumCalibration_HF2, "Reset", SpectrumCalibrationReset_TB_ID),
					  new TGLayoutHints(kLHintsNormal, 0,0,5,5));
  SpectrumCalibrationReset_TB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleTextButtons()");
  SpectrumCalibrationReset_TB->Resize(100,25);
  SpectrumCalibrationReset_TB->ChangeOptions(SpectrumCalibrationReset_TB->GetOptions() | kFixedSize);
  SpectrumCalibrationReset_TB->SetState(kButtonDisabled);

  TGHorizontalFrame *SpectrumCalibration_HF3 = new TGHorizontalFrame(SpectrumCalibration_GF);
  SpectrumCalibration_GF->AddFrame(SpectrumCalibration_HF3);

  /////////////////////////
  // Create spectrum button
  
  TGHorizontalFrame *ProcessCreateSpectrum_HF = new TGHorizontalFrame(SpectrumFrame_VF);
  SpectrumFrame_VF->AddFrame(ProcessCreateSpectrum_HF, new TGLayoutHints(kLHintsNormal, 0,5,5,5));
  
  ProcessCreateSpectrum_HF->AddFrame(ProcessSpectrum_TB = new TGTextButton(ProcessCreateSpectrum_HF, "Process waveforms", ProcessSpectrum_TB_ID),
				     new TGLayoutHints(kLHintsNormal, LOffset+35,5,15,0));
  ProcessSpectrum_TB->Resize(120, 30);
  ProcessSpectrum_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  ProcessSpectrum_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  ProcessSpectrum_TB->ChangeOptions(ProcessSpectrum_TB->GetOptions() | kFixedSize);
  ProcessSpectrum_TB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleTextButtons()");
  
  ProcessCreateSpectrum_HF->AddFrame(CreateSpectrum_TB = new TGTextButton(ProcessCreateSpectrum_HF, "Create spectrum", CreateSpectrum_TB_ID),
				     new TGLayoutHints(kLHintsNormal, 5,5,15,0));
  CreateSpectrum_TB->Resize(120, 30);
  CreateSpectrum_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  CreateSpectrum_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  CreateSpectrum_TB->ChangeOptions(CreateSpectrum_TB->GetOptions() | kFixedSize);
  CreateSpectrum_TB->Connect("Clicked()", "AASpectrumSlots", SpectrumSlots, "HandleTextButtons()");
  CreateSpectrum_TB->SetState(kButtonDisabled);
}


void AAInterface::FillAnalysisFrame()
{
  TGCanvas *AnalysisFrame_C = new TGCanvas(AnalysisOptions_CF, TabFrameWidth-10, TabFrameLength-20, kDoubleBorder);
  AnalysisOptions_CF->AddFrame(AnalysisFrame_C, new TGLayoutHints(kLHintsCenterX, 0,0,10,10));

  TGVerticalFrame *AnalysisFrame_VF = new TGVerticalFrame(AnalysisFrame_C->GetViewPort(), TabFrameWidth-10, TabFrameLength);
  AnalysisFrame_C->SetContainer(AnalysisFrame_VF);

  
  //////////////////////////////
  // Spectrum background fitting 
  
  TGGroupFrame *BackgroundAnalysis_GF = new TGGroupFrame(AnalysisFrame_VF, "Background fitting", kVerticalFrame);
  AnalysisFrame_VF->AddFrame(BackgroundAnalysis_GF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  
  TGHorizontalFrame *SpectrumBackgroundOptions0_HF = new TGHorizontalFrame(BackgroundAnalysis_GF);
  BackgroundAnalysis_GF->AddFrame(SpectrumBackgroundOptions0_HF);
  
  SpectrumBackgroundOptions0_HF->AddFrame(SpectrumFindBackground_CB = new TGCheckButton(SpectrumBackgroundOptions0_HF, "Find background", SpectrumFindBackground_CB_ID),
					  new TGLayoutHints(kLHintsNormal, 5,5,6,0));
  SpectrumFindBackground_CB->Connect("Clicked()", "AAAnalysisSlots", AnalysisSlots, "HandleCheckButtons()");

  SpectrumBackgroundOptions0_HF->AddFrame(SpectrumBackgroundIterations_NEL = new ADAQNumberEntryWithLabel(SpectrumBackgroundOptions0_HF, "Iterations", SpectrumBackgroundIterations_NEL_ID),
					  new TGLayoutHints(kLHintsNormal,10,0,5,0));
  SpectrumBackgroundIterations_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  SpectrumBackgroundIterations_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumBackgroundIterations_NEL->GetEntry()->SetNumber(5);
  SpectrumBackgroundIterations_NEL->GetEntry()->Resize(40,20);
  SpectrumBackgroundIterations_NEL->GetEntry()->Connect("ValueSet(long)", "AAAnalysisSlots", AnalysisSlots, "HandleNumberEntries()");
  SpectrumBackgroundIterations_NEL->GetEntry()->SetState(false);
  
  ////

  TGHorizontalFrame *SpectrumBackgroundOptions1_HF = new TGHorizontalFrame(BackgroundAnalysis_GF);
  BackgroundAnalysis_GF->AddFrame(SpectrumBackgroundOptions1_HF);
  
  SpectrumBackgroundOptions1_HF->AddFrame(SpectrumBackgroundCompton_CB = new TGCheckButton(SpectrumBackgroundOptions1_HF, "Compton", SpectrumBackgroundCompton_CB_ID),
					  new TGLayoutHints(kLHintsNormal,5,5,0,5));
  SpectrumBackgroundCompton_CB->Connect("Clicked()", "AAAnalysisSlots", AnalysisSlots, "HandleCheckButtons()");
  SpectrumBackgroundCompton_CB->SetState(kButtonDisabled);
  

  SpectrumBackgroundOptions1_HF->AddFrame(SpectrumBackgroundSmoothing_CB = new TGCheckButton(SpectrumBackgroundOptions1_HF, "Smoothing", SpectrumBackgroundSmoothing_CB_ID),
					  new TGLayoutHints(kLHintsNormal,70,5,0,5));
  SpectrumBackgroundSmoothing_CB->Connect("Clicked()", "AAAnalysisSlots", AnalysisSlots, "HandleCheckButtons()");
  SpectrumBackgroundSmoothing_CB->SetState(kButtonDisabled);

  ////

  TGHorizontalFrame *SpectrumBackgroundOptions2_HF = new TGHorizontalFrame(BackgroundAnalysis_GF);
  BackgroundAnalysis_GF->AddFrame(SpectrumBackgroundOptions2_HF);
  
  SpectrumBackgroundOptions2_HF->AddFrame(SpectrumRangeMin_NEL = new ADAQNumberEntryWithLabel(SpectrumBackgroundOptions2_HF, "Min.", SpectrumRangeMin_NEL_ID),
					  new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumRangeMin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumRangeMin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumRangeMin_NEL->GetEntry()->SetNumber(0);
  SpectrumRangeMin_NEL->GetEntry()->Resize(75,20);
  SpectrumRangeMin_NEL->GetEntry()->SetState(false);
  SpectrumRangeMin_NEL->GetEntry()->Connect("ValueSet(long)", "AAAnalysisSlots", AnalysisSlots, "HandleNumberEntries()");

  
  SpectrumBackgroundOptions2_HF->AddFrame(SpectrumRangeMax_NEL = new ADAQNumberEntryWithLabel(SpectrumBackgroundOptions2_HF, "Max.", SpectrumRangeMax_NEL_ID),
					  new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumRangeMax_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumRangeMax_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumRangeMax_NEL->GetEntry()->SetNumber(2000);
  SpectrumRangeMax_NEL->GetEntry()->Resize(75,20);
  SpectrumRangeMax_NEL->GetEntry()->SetState(false);
  SpectrumRangeMax_NEL->GetEntry()->Connect("ValueSet(long)", "AAAnalysisSlots", AnalysisSlots, "HandleNumberEntries()");

  ////

  BackgroundAnalysis_GF->AddFrame(SpectrumBackgroundDirection_CBL = new ADAQComboBoxWithLabel(BackgroundAnalysis_GF, "Filter direction", SpectrumBackgroundDirection_CBL_ID),
				new TGLayoutHints(kLHintsNormal,5,5,0,0));
  SpectrumBackgroundDirection_CBL->GetComboBox()->AddEntry("Increase",0);
  SpectrumBackgroundDirection_CBL->GetComboBox()->AddEntry("Decrease",1);
  SpectrumBackgroundDirection_CBL->GetComboBox()->Resize(75,20);
  SpectrumBackgroundDirection_CBL->GetComboBox()->Select(1);
  SpectrumBackgroundDirection_CBL->GetComboBox()->Connect("Selected(int,int)", "AAAnalysisSlots", AnalysisSlots, "HandleComboBoxes(int,int)");
  SpectrumBackgroundDirection_CBL->GetComboBox()->SetEnabled(false);
  
  BackgroundAnalysis_GF->AddFrame(SpectrumBackgroundFilterOrder_CBL = new ADAQComboBoxWithLabel(BackgroundAnalysis_GF, "Filter order", SpectrumBackgroundFilterOrder_CBL_ID),
				new TGLayoutHints(kLHintsNormal,5,5,0,0));
  SpectrumBackgroundFilterOrder_CBL->GetComboBox()->Resize(75,20);
  SpectrumBackgroundFilterOrder_CBL->GetComboBox()->AddEntry("2",2);
  SpectrumBackgroundFilterOrder_CBL->GetComboBox()->AddEntry("4",4);
  SpectrumBackgroundFilterOrder_CBL->GetComboBox()->AddEntry("6",6);
  SpectrumBackgroundFilterOrder_CBL->GetComboBox()->AddEntry("8",8);
  SpectrumBackgroundFilterOrder_CBL->GetComboBox()->Select(2);
  SpectrumBackgroundFilterOrder_CBL->GetComboBox()->Connect("Selected(int,int)", "AAAnalysisSlots", AnalysisSlots, "HandleComboBoxes(int,int)");
  SpectrumBackgroundFilterOrder_CBL->GetComboBox()->SetEnabled(false);
  
  BackgroundAnalysis_GF->AddFrame(SpectrumBackgroundSmoothingWidth_CBL = new ADAQComboBoxWithLabel(BackgroundAnalysis_GF, "Smoothing width", SpectrumBackgroundSmoothingWidth_CBL_ID),
				new TGLayoutHints(kLHintsNormal,5,5,0,0));
  SpectrumBackgroundSmoothingWidth_CBL->GetComboBox()->Resize(75,20);
  SpectrumBackgroundSmoothingWidth_CBL->GetComboBox()->AddEntry("3",3);
  SpectrumBackgroundSmoothingWidth_CBL->GetComboBox()->AddEntry("5",5);
  SpectrumBackgroundSmoothingWidth_CBL->GetComboBox()->AddEntry("7",7);
  SpectrumBackgroundSmoothingWidth_CBL->GetComboBox()->AddEntry("9",9);
  SpectrumBackgroundSmoothingWidth_CBL->GetComboBox()->AddEntry("11",11);
  SpectrumBackgroundSmoothingWidth_CBL->GetComboBox()->AddEntry("13",13);
  SpectrumBackgroundSmoothingWidth_CBL->GetComboBox()->AddEntry("15",15);
  SpectrumBackgroundSmoothingWidth_CBL->GetComboBox()->Select(3);
  SpectrumBackgroundSmoothingWidth_CBL->GetComboBox()->Connect("Selected(int,int)", "AAAnalysisSlots", AnalysisSlots, "HandleComboBoxes(int,int)");
  SpectrumBackgroundSmoothingWidth_CBL->GetComboBox()->SetEnabled(false);
  
  ////

  TGHorizontalFrame *BackgroundPlotting_HF = new TGHorizontalFrame(BackgroundAnalysis_GF);
  BackgroundAnalysis_GF->AddFrame(BackgroundPlotting_HF, new TGLayoutHints(kLHintsNormal, 0,0,10,0));

  BackgroundPlotting_HF->AddFrame(SpectrumWithBackground_RB = new TGRadioButton(BackgroundPlotting_HF, "Plot with bckgnd", SpectrumWithBackground_RB_ID),
				  new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumWithBackground_RB->SetState(kButtonDown);
  SpectrumWithBackground_RB->SetState(kButtonDisabled);
  SpectrumWithBackground_RB->Connect("Clicked()", "AAAnalysisSlots", AnalysisSlots, "HandleRadioButtons()");
  
  BackgroundPlotting_HF->AddFrame(SpectrumLessBackground_RB = new TGRadioButton(BackgroundPlotting_HF, "Plot less bckgnd", SpectrumLessBackground_RB_ID),
				  new TGLayoutHints(kLHintsNormal, 5,5,0,5));
  SpectrumLessBackground_RB->SetState(kButtonDisabled);
  SpectrumLessBackground_RB->Connect("Clicked()", "AAAnalysisSlots", AnalysisSlots, "HandleRadioButtons()");

  ////////////////////////////////////////
  // Spectrum integration and peak fitting
  
  TGGroupFrame *SpectrumAnalysis_GF = new TGGroupFrame(AnalysisFrame_VF, "Integration and peak fitting", kVerticalFrame);
  AnalysisFrame_VF->AddFrame(SpectrumAnalysis_GF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  
  TGHorizontalFrame *Horizontal0_HF = new TGHorizontalFrame(SpectrumAnalysis_GF);
  SpectrumAnalysis_GF->AddFrame(Horizontal0_HF, new TGLayoutHints(kLHintsNormal, 0,0,5,5));
  
  Horizontal0_HF->AddFrame(SpectrumFindIntegral_CB = new TGCheckButton(Horizontal0_HF, "Find integral", SpectrumFindIntegral_CB_ID),
			   new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumFindIntegral_CB->Connect("Clicked()", "AAAnalysisSlots", AnalysisSlots, "HandleCheckButtons()");
  
  Horizontal0_HF->AddFrame(SpectrumIntegralInCounts_CB = new TGCheckButton(Horizontal0_HF, "Integral in counts", SpectrumIntegralInCounts_CB_ID),
			   new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumIntegralInCounts_CB->Connect("Clicked()", "AAAnalysisSlots", AnalysisSlots, "HandleCheckButtons()");
  SpectrumIntegralInCounts_CB->SetState(kButtonDown);
  
  SpectrumAnalysis_GF->AddFrame(SpectrumIntegral_NEFL = new ADAQNumberEntryFieldWithLabel(SpectrumAnalysis_GF, "Integral", -1),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumIntegral_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal, TGNumberFormat::kNEANonNegative);
  SpectrumIntegral_NEFL->GetEntry()->Resize(100,20);
  SpectrumIntegral_NEFL->GetEntry()->SetState(true);
  SpectrumIntegral_NEFL->GetEntry()->SetBackgroundColor(ColorMgr->Number2Pixel(19));
  
  SpectrumAnalysis_GF->AddFrame(SpectrumIntegralError_NEFL = new ADAQNumberEntryFieldWithLabel(SpectrumAnalysis_GF, "Error", -1),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumIntegralError_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal, TGNumberFormat::kNEANonNegative);
  SpectrumIntegralError_NEFL->GetEntry()->Resize(100,20);
  SpectrumIntegralError_NEFL->GetEntry()->SetState(true);
  SpectrumIntegralError_NEFL->GetEntry()->SetBackgroundColor(ColorMgr->Number2Pixel(19));


  TGHorizontalFrame *Horizontal2_HF = new TGHorizontalFrame(SpectrumAnalysis_GF);
  SpectrumAnalysis_GF->AddFrame(Horizontal2_HF, new TGLayoutHints(kLHintsNormal, 0,0,10,5));

  Horizontal2_HF->AddFrame(SpectrumUseGaussianFit_CB = new TGCheckButton(Horizontal2_HF, "Use gaussian fit", SpectrumUseGaussianFit_CB_ID),
			   new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumUseGaussianFit_CB->Connect("Clicked()", "AAAnalysisSlots", AnalysisSlots, "HandleCheckButtons()");
  
  Horizontal2_HF->AddFrame(SpectrumUseVerboseFit_CB = new TGCheckButton(Horizontal2_HF, "Verbose", SpectrumUseVerboseFit_CB_ID),
			   new TGLayoutHints(kLHintsNormal, 10,5,0,0));
  SpectrumUseVerboseFit_CB->Connect("Clicked()", "AAAnalysisSlots", AnalysisSlots, "HandleCheckButtons()");


  // Fit height

  TGHorizontalFrame *Horizontal3_HF = new TGHorizontalFrame(SpectrumAnalysis_GF);
  SpectrumAnalysis_GF->AddFrame(Horizontal3_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  
  Horizontal3_HF->AddFrame(SpectrumFitHeight_NEFL = new ADAQNumberEntryFieldWithLabel(Horizontal3_HF, "Height   +/-",-1),
			   new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumFitHeight_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal);
  SpectrumFitHeight_NEFL->GetEntry()->SetState(true);
  SpectrumFitHeight_NEFL->GetEntry()->SetBackgroundColor(ColorMgr->Number2Pixel(19));
  SpectrumFitHeight_NEFL->GetEntry()->Resize(60, 20);
  
  Horizontal3_HF->AddFrame(SpectrumFitHeightErr_NEFL = new ADAQNumberEntryFieldWithLabel(Horizontal3_HF, "",-1),
			   new TGLayoutHints(kLHintsNormal, 10,5,0,0));
  SpectrumFitHeightErr_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal);
  SpectrumFitHeightErr_NEFL->GetEntry()->SetState(true);
  SpectrumFitHeightErr_NEFL->GetEntry()->SetBackgroundColor(ColorMgr->Number2Pixel(19));
  SpectrumFitHeightErr_NEFL->GetEntry()->Resize(60, 20);

  // Fit mean

  TGHorizontalFrame *Horizontal4_HF = new TGHorizontalFrame(SpectrumAnalysis_GF);
  SpectrumAnalysis_GF->AddFrame(Horizontal4_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  
  Horizontal4_HF->AddFrame(SpectrumFitMean_NEFL = new ADAQNumberEntryFieldWithLabel(Horizontal4_HF, "Mean     +/-",-1),
			   new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumFitMean_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal);
  SpectrumFitMean_NEFL->GetEntry()->SetState(true);
  SpectrumFitMean_NEFL->GetEntry()->SetBackgroundColor(ColorMgr->Number2Pixel(19));
  SpectrumFitMean_NEFL->GetEntry()->Resize(60, 20);
  
  Horizontal4_HF->AddFrame(SpectrumFitMeanErr_NEFL = new ADAQNumberEntryFieldWithLabel(Horizontal4_HF, "",-1),
			   new TGLayoutHints(kLHintsNormal, 10,5,0,0));
  SpectrumFitMeanErr_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal);
  SpectrumFitMeanErr_NEFL->GetEntry()->SetState(true);
  SpectrumFitMeanErr_NEFL->GetEntry()->SetBackgroundColor(ColorMgr->Number2Pixel(19));
  SpectrumFitMeanErr_NEFL->GetEntry()->Resize(60, 20);

  // Fit sigma
  
  TGHorizontalFrame *Horizontal5_HF = new TGHorizontalFrame(SpectrumAnalysis_GF);
  SpectrumAnalysis_GF->AddFrame(Horizontal5_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  
  Horizontal5_HF->AddFrame(SpectrumFitSigma_NEFL = new ADAQNumberEntryFieldWithLabel(Horizontal5_HF, "Sigma    +/-",-1),
			   new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumFitSigma_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal);
  SpectrumFitSigma_NEFL->GetEntry()->SetState(true);
  SpectrumFitSigma_NEFL->GetEntry()->SetBackgroundColor(ColorMgr->Number2Pixel(19));
  SpectrumFitSigma_NEFL->GetEntry()->Resize(60, 20);

  Horizontal5_HF->AddFrame(SpectrumFitSigmaErr_NEFL = new ADAQNumberEntryFieldWithLabel(Horizontal5_HF, "",-1),
				new TGLayoutHints(kLHintsNormal, 10,5,0,0));
  SpectrumFitSigmaErr_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal);
  SpectrumFitSigmaErr_NEFL->GetEntry()->SetState(true);
  SpectrumFitSigmaErr_NEFL->GetEntry()->SetBackgroundColor(ColorMgr->Number2Pixel(19));
  SpectrumFitSigmaErr_NEFL->GetEntry()->Resize(60, 20);

  // Fit resolution

  TGHorizontalFrame *Horizontal6_HF = new TGHorizontalFrame(SpectrumAnalysis_GF);
  SpectrumAnalysis_GF->AddFrame(Horizontal6_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  
  Horizontal6_HF->AddFrame(SpectrumFitRes_NEFL = new ADAQNumberEntryFieldWithLabel(Horizontal6_HF, "Res (%)  +/-",-1),
			   new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumFitRes_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal);
  SpectrumFitRes_NEFL->GetEntry()->SetState(true);
  SpectrumFitRes_NEFL->GetEntry()->SetBackgroundColor(ColorMgr->Number2Pixel(19));
  SpectrumFitRes_NEFL->GetEntry()->Resize(60, 20);
  
  Horizontal6_HF->AddFrame(SpectrumFitResErr_NEFL = new ADAQNumberEntryFieldWithLabel(Horizontal6_HF, "",-1),
				new TGLayoutHints(kLHintsNormal, 10,5,0,0));
  SpectrumFitResErr_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal);
  SpectrumFitResErr_NEFL->GetEntry()->SetState(true);
  SpectrumFitResErr_NEFL->GetEntry()->SetBackgroundColor(ColorMgr->Number2Pixel(19));
  SpectrumFitResErr_NEFL->GetEntry()->Resize(60, 20);

  // Spectrum analysis limits

  SpectrumAnalysis_GF->AddFrame(new TGLabel(SpectrumAnalysis_GF, "Spectrum analysis limits"),
				new TGLayoutHints(kLHintsNormal,5,10,5,5));

  TGHorizontalFrame *Horizontal7_HF = new TGHorizontalFrame(SpectrumAnalysis_GF);
  SpectrumAnalysis_GF->AddFrame(Horizontal7_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,5));

  Horizontal7_HF->AddFrame(SpectrumAnalysisLowerLimit_NEL = new ADAQNumberEntryWithLabel(Horizontal7_HF, "Lower", SpectrumAnalysisLowerLimit_NEL_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumAnalysisLowerLimit_NEL->GetEntry()->SetFormat(TGNumberFormat::kNESReal);
  SpectrumAnalysisLowerLimit_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  SpectrumAnalysisLowerLimit_NEL->GetEntry()->SetLimitValues(0,20000);
  SpectrumAnalysisLowerLimit_NEL->GetEntry()->SetNumber(0);
  SpectrumAnalysisLowerLimit_NEL->GetEntry()->Resize(70, 20);
  SpectrumAnalysisLowerLimit_NEL->GetEntry()->Connect("ValueSet(long)", "AAAnalysisSlots", AnalysisSlots, "HandleNumberEntries()");

  Horizontal7_HF->AddFrame(SpectrumAnalysisUpperLimit_NEL = new ADAQNumberEntryWithLabel(Horizontal7_HF, "Upper", SpectrumAnalysisUpperLimit_NEL_ID),
				new TGLayoutHints(kLHintsNormal, 10,5,0,0));
  SpectrumAnalysisUpperLimit_NEL->GetEntry()->SetFormat(TGNumberFormat::kNESReal);
  SpectrumAnalysisUpperLimit_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  SpectrumAnalysisUpperLimit_NEL->GetEntry()->SetLimitValues(0,20000);
  SpectrumAnalysisUpperLimit_NEL->GetEntry()->SetNumber(20000);
  SpectrumAnalysisUpperLimit_NEL->GetEntry()->Resize(70, 20);
  SpectrumAnalysisUpperLimit_NEL->GetEntry()->Connect("ValueSet(long)", "AAAnalysisSlots", AnalysisSlots, "HandleNumberEntries()");


  ////////////////////////////////
  // Spectrum energy analysis (EA)
  
  TGGroupFrame *EA_GF = new TGGroupFrame(AnalysisFrame_VF, "Energy analysis", kVerticalFrame);
  AnalysisFrame_VF->AddFrame(EA_GF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));

  TGHorizontalFrame *EA_HF0 = new TGHorizontalFrame(EA_GF);
  EA_GF->AddFrame(EA_HF0, new TGLayoutHints(kLHintsNormal, 5,5,5,5));
  
  EA_HF0->AddFrame(EAEnable_CB = new TGCheckButton(EA_HF0, "Enable", EAEnable_CB_ID),
		   new TGLayoutHints(kLHintsNormal, 0,0,7,5));
  EAEnable_CB->Connect("Clicked()", "AAAnalysisSlots", AnalysisSlots, "HandleCheckButtons()");
  
  EA_HF0->AddFrame(EASpectrumType_CBL = new ADAQComboBoxWithLabel(EA_HF0, "", EASpectrumType_CBL_ID),
		   new TGLayoutHints(kLHintsNormal, 20,5,5,5));
  EASpectrumType_CBL->GetComboBox()->AddEntry("Gamma spectrum", 0);
  EASpectrumType_CBL->GetComboBox()->AddEntry("Neutron spectrum",1);
  EASpectrumType_CBL->GetComboBox()->Select(0);
  EASpectrumType_CBL->GetComboBox()->Resize(130,20);
  EASpectrumType_CBL->GetComboBox()->SetEnabled(false);
  EASpectrumType_CBL->GetComboBox()->Connect("Selected(int,int)", "AAAnalysisSlots", AnalysisSlots, "HandleComboBoxes(int,int)");
  

  EA_GF->AddFrame(EAGammaEDep_NEL = new ADAQNumberEntryWithLabel(EA_GF, "Energy deposited", EAGammaEDep_NEL_ID),
		  new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  EAGammaEDep_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  EAGammaEDep_NEL->GetEntry()->SetNumber(0.);
  EAGammaEDep_NEL->GetEntry()->SetState(false);
  EAGammaEDep_NEL->GetEntry()->Connect("ValueSet(long)", "AAAnalysisSlots", AnalysisSlots, "HandleNumberEntries()");

  EA_GF->AddFrame(EAEscapePeaks_CB = new TGCheckButton(EA_GF, "Predict escape peaks", EAEscapePeaks_CB_ID),
		  new TGLayoutHints(kLHintsNormal, 5,5,0,10));
  EAEscapePeaks_CB->Connect("Clicked()", "AAAnalysisSlots", AnalysisSlots, "HandleCheckButtons()");
  EAEscapePeaks_CB->SetState(kButtonDisabled);


  EA_GF->AddFrame(EALightConversionFactor_NEL = new ADAQNumberEntryWithLabel(EA_GF, "Conversion factor", EALightConversionFactor_NEL_ID),
		   new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  EALightConversionFactor_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  EALightConversionFactor_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  EALightConversionFactor_NEL->GetEntry()->SetNumber(1.);
  EALightConversionFactor_NEL->GetEntry()->SetState(false);
  EALightConversionFactor_NEL->GetEntry()->Connect("ValueSet(long", "AAAnalysisSlots", AnalysisSlots, "HandleNumberEntries()");
    
  EA_GF->AddFrame(EAErrorWidth_NEL = new ADAQNumberEntryWithLabel(EA_GF, "Error width [%]", EAErrorWidth_NEL_ID),
		   new TGLayoutHints(kLHintsNormal, 5,5,0,5));
  EAErrorWidth_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  EAErrorWidth_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  EAErrorWidth_NEL->GetEntry()->SetNumber(5);
  EAErrorWidth_NEL->GetEntry()->Connect("ValueSet(long)", "AAAnalysisSlots", AnalysisSlots, "HandleNumberEntries()");
  EAErrorWidth_NEL->GetEntry()->SetState(false);
  
  EA_GF->AddFrame(EAElectronEnergy_NEL = new ADAQNumberEntryWithLabel(EA_GF, "Electron [MeV]", EAElectronEnergy_NEL_ID),
		   new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  EAElectronEnergy_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESRealThree);
  EAElectronEnergy_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  EAElectronEnergy_NEL->GetEntry()->Resize(70,20);
  EAElectronEnergy_NEL->GetEntry()->Connect("ValueSet(long)", "AAAnalysisSlots", AnalysisSlots, "HandleNumberEntries()");
  EAElectronEnergy_NEL->GetEntry()->SetState(false);

  EA_GF->AddFrame(EAGammaEnergy_NEL = new ADAQNumberEntryWithLabel(EA_GF, "Gamma [MeV]", EAGammaEnergy_NEL_ID),
		   new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  EAGammaEnergy_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESRealThree);
  EAGammaEnergy_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  EAGammaEnergy_NEL->GetEntry()->Resize(70,20);
  EAGammaEnergy_NEL->GetEntry()->Connect("ValueSet(long)", "AAAnalysisSlots", AnalysisSlots, "HandleNumberEntries()");
  EAGammaEnergy_NEL->GetEntry()->SetState(false);

  EA_GF->AddFrame(EAProtonEnergy_NEL = new ADAQNumberEntryWithLabel(EA_GF, "Proton (neutron) [MeV]", EAProtonEnergy_NEL_ID),
		   new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  EAProtonEnergy_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESRealThree);
  EAProtonEnergy_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  EAProtonEnergy_NEL->GetEntry()->Resize(70,20);
  EAProtonEnergy_NEL->GetEntry()->Connect("ValueSet(long)", "AAAnalysisSlots", AnalysisSlots, "HandleNumberEntries()");
  EAProtonEnergy_NEL->GetEntry()->SetState(false);

  EA_GF->AddFrame(EAAlphaEnergy_NEL = new ADAQNumberEntryWithLabel(EA_GF, "Alpha [MeV]", EAAlphaEnergy_NEL_ID),
		   new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  EAAlphaEnergy_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESRealThree);
  EAAlphaEnergy_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  EAAlphaEnergy_NEL->GetEntry()->Resize(70,20);
  EAAlphaEnergy_NEL->GetEntry()->Connect("ValueSet(long)", "AAAnalysisSlots", AnalysisSlots, "HandleNumberEntries()");
  EAAlphaEnergy_NEL->GetEntry()->SetState(false);

  EA_GF->AddFrame(EACarbonEnergy_NEL = new ADAQNumberEntryWithLabel(EA_GF, "Carbon [GeV]", EACarbonEnergy_NEL_ID),
		   new TGLayoutHints(kLHintsNormal, 5,5,0,5));
  EACarbonEnergy_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESRealThree);
  EACarbonEnergy_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  EACarbonEnergy_NEL->GetEntry()->Resize(70,20);
  EACarbonEnergy_NEL->GetEntry()->Connect("ValueSet(long)", "AAAnalysisSlots", AnalysisSlots, "HandleNumberEntries()");
  EACarbonEnergy_NEL->GetEntry()->SetState(false);
}


void AAInterface::FillPSDFrame()
{
  TGCanvas *PSDFrame_C = new TGCanvas(PSDOptions_CF, TabFrameWidth-10, TabFrameLength-20, kDoubleBorder);
  PSDOptions_CF->AddFrame(PSDFrame_C, new TGLayoutHints(kLHintsCenterX, 0,0,10,10));

  TGVerticalFrame *PSDFrame_VF = new TGVerticalFrame(PSDFrame_C->GetViewPort(), TabFrameWidth-10, TabFrameLength);
  PSDFrame_C->SetContainer(PSDFrame_VF);

  Int_t LOffset0 = 15;


  //////////////////////////
  // PSD creation widgets //
  //////////////////////////
  
  PSDFrame_VF->AddFrame(new TGLabel(PSDFrame_VF, "Histogram information"),
			new TGLayoutHints(kLHintsLeft, LOffset0,0,5,0));
  
  TGHorizontalFrame *PSDWaveformsAndThreshold_HF = new TGHorizontalFrame(PSDFrame_VF);
  PSDFrame_VF->AddFrame(PSDWaveformsAndThreshold_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  
  PSDWaveformsAndThreshold_HF->AddFrame(PSDWaveforms_NEL = new ADAQNumberEntryWithLabel(PSDWaveformsAndThreshold_HF,
											"Waveforms",
											-1),
					new TGLayoutHints(kLHintsNormal, LOffset0,5,5,0));
  PSDWaveforms_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDWaveforms_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  PSDWaveforms_NEL->GetEntry()->SetLimitValues(0,1); // Updated when ADAQ ROOT file loaded
  PSDWaveforms_NEL->GetEntry()->SetNumber(0); // Updated when ADAQ ROOT file loaded

  PSDWaveformsAndThreshold_HF->AddFrame(PSDThreshold_NEL = new ADAQNumberEntryWithLabel(PSDWaveformsAndThreshold_HF,
											"Threshold (ADC)",
											-1),
					new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  PSDThreshold_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PSDThreshold_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  PSDThreshold_NEL->GetEntry()->SetNumber(0);

  ////////////////////////
  // PSD histogram binning

  PSDFrame_VF->AddFrame(new TGLabel(PSDFrame_VF, "X-axis binning"),
			new TGLayoutHints(kLHintsLeft, LOffset0,0,5,0));

  TGHorizontalFrame *PSDXAxis_HF = new TGHorizontalFrame(PSDFrame_VF);
  PSDFrame_VF->AddFrame(PSDXAxis_HF);
  
  PSDXAxis_HF->AddFrame(PSDNumTotalBins_NEL = new ADAQNumberEntryWithLabel(PSDXAxis_HF, "Bins ", -1),
			new TGLayoutHints(kLHintsNormal, LOffset0,5,5,0));
  PSDNumTotalBins_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDNumTotalBins_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PSDNumTotalBins_NEL->GetEntry()->SetNumber(100);
  
  PSDXAxis_HF->AddFrame(PSDXAxisADC_RB = new TGRadioButton(PSDXAxis_HF, "ADC", PSDXAxisADC_RB_ID),
			new TGLayoutHints(kLHintsNormal,25,0,10,0));
  PSDXAxisADC_RB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleRadioButtons()");
  PSDXAxisADC_RB->SetState(kButtonDown);
  
  PSDXAxis_HF->AddFrame(PSDXAxisEnergy_RB = new TGRadioButton(PSDXAxis_HF, "Energy", PSDXAxisEnergy_RB_ID),
			new TGLayoutHints(kLHintsNormal,10,0,10,0));
  PSDXAxisEnergy_RB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleRadioButtons()");
  PSDXAxisEnergy_RB->SetState(kButtonDisabled);
  
  TGHorizontalFrame *TotalBins_HF = new TGHorizontalFrame(PSDFrame_VF);
  PSDFrame_VF->AddFrame(TotalBins_HF);

  TotalBins_HF->AddFrame(PSDMinTotalBin_NEL = new ADAQNumberEntryWithLabel(TotalBins_HF, "Minimum  ", -1),
			 new TGLayoutHints(kLHintsNormal, LOffset0,5,0,0));
  PSDMinTotalBin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PSDMinTotalBin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  PSDMinTotalBin_NEL->GetEntry()->SetNumber(0);

  TotalBins_HF->AddFrame(PSDMaxTotalBin_NEL = new ADAQNumberEntryWithLabel(TotalBins_HF, "Maximum", -1),
			 new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PSDMaxTotalBin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PSDMaxTotalBin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PSDMaxTotalBin_NEL->GetEntry()->SetNumber(20000);


  PSDFrame_VF->AddFrame(new TGLabel(PSDFrame_VF, "Y-axis binning"),
			new TGLayoutHints(kLHintsLeft, LOffset0,0,5,0));

  TGHorizontalFrame *PSDYAxis_HF = new TGHorizontalFrame(PSDFrame_VF);
  PSDFrame_VF->AddFrame(PSDYAxis_HF);
  
  PSDYAxis_HF->AddFrame(PSDNumTailBins_NEL = new ADAQNumberEntryWithLabel(PSDYAxis_HF, "Bins ", -1),
			new TGLayoutHints(kLHintsNormal, LOffset0,5,5,0));
  PSDNumTailBins_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDNumTailBins_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PSDNumTailBins_NEL->GetEntry()->SetNumber(100);
  
  PSDYAxis_HF->AddFrame(PSDYAxisTail_RB = new TGRadioButton(PSDYAxis_HF, "Tail", PSDYAxisTail_RB_ID),
			new TGLayoutHints(kLHintsNormal,25,0,10,0));
  PSDYAxisTail_RB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleRadioButtons()");
  
  PSDYAxis_HF->AddFrame(PSDYAxisTailTotal_RB = new TGRadioButton(PSDYAxis_HF, "Tail/Total", PSDYAxisTailTotal_RB_ID),
			new TGLayoutHints(kLHintsNormal,10,0,10,0));
  PSDYAxisTailTotal_RB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleRadioButtons()");
  PSDYAxisTailTotal_RB->SetState(kButtonDown);

  TGHorizontalFrame *TailBins_HF = new TGHorizontalFrame(PSDFrame_VF);
  PSDFrame_VF->AddFrame(TailBins_HF);
  
  TailBins_HF->AddFrame(PSDMinTailBin_NEL = new ADAQNumberEntryWithLabel(TailBins_HF, "Minimum  ", PSDMinTailBin_NEL_ID),
			new TGLayoutHints(kLHintsNormal, LOffset0,5,0,0));
  PSDMinTailBin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PSDMinTailBin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  PSDMinTailBin_NEL->GetEntry()->SetNumber(0.);
  PSDMinTailBin_NEL->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleNumberEntries()");
  
  TailBins_HF->AddFrame(PSDMaxTailBin_NEL = new ADAQNumberEntryWithLabel(TailBins_HF, "Maximum", PSDMaxTailBin_NEL_ID),
			new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PSDMaxTailBin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PSDMaxTailBin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PSDMaxTailBin_NEL->GetEntry()->SetNumber(0.4);
  PSDMaxTailBin_NEL->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleNumberEntries()");
  
  
  /////////////////////////
  // Waveform PSD integrals
  
  PSDFrame_VF->AddFrame(new TGLabel(PSDFrame_VF, "Waveform integrals (sample rel. to peak)"),
			new TGLayoutHints(kLHintsLeft, LOffset0,0,5,5));
  
  TGHorizontalFrame *PSDTotal_HF = new TGHorizontalFrame(PSDFrame_VF);
  PSDFrame_VF->AddFrame(PSDTotal_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  
  PSDTotal_HF->AddFrame(PSDTotalStart_NEL = new ADAQNumberEntryWithLabel(PSDTotal_HF,
									 "Total start  ",
									 PSDTotalStart_NEL_ID),
			  new TGLayoutHints(kLHintsNormal, LOffset0,10,0,0));
  PSDTotalStart_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDTotalStart_NEL->GetEntry()->SetNumber(-5);
  PSDTotalStart_NEL->GetEntry()->Resize(50,20);
  PSDTotalStart_NEL->GetEntry()->Connect("ValueSet(long)", "AAPSDSlots", ProcessingSlots, "HandleNumberEntries()");
    
  PSDTotal_HF->AddFrame(PSDTotalStop_NEL = new ADAQNumberEntryWithLabel(PSDTotal_HF,
									"Total stop",
									PSDTotalStop_NEL_ID),
			  new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  PSDTotalStop_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDTotalStop_NEL->GetEntry()->SetNumber(150);
  PSDTotalStop_NEL->GetEntry()->Resize(50,20);
  PSDTotalStop_NEL->GetEntry()->Connect("ValueSet(long)", "AAPSDSlots", ProcessingSlots, "HandleNumberEntries()");


  TGHorizontalFrame *DGPSDTail_HF = new TGHorizontalFrame(PSDFrame_VF);
  PSDFrame_VF->AddFrame(DGPSDTail_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));


  DGPSDTail_HF->AddFrame(PSDTailStart_NEL = new ADAQNumberEntryWithLabel(DGPSDTail_HF,
									 "Tail start    ",
									 PSDTailStart_NEL_ID),
			 new TGLayoutHints(kLHintsNormal, LOffset0,10,0,0));
  PSDTailStart_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDTailStart_NEL->GetEntry()->SetNumber(7);
  PSDTailStart_NEL->GetEntry()->Resize(50,20);
  PSDTailStart_NEL->GetEntry()->Connect("ValueSet(long)", "AAPSDSlots", ProcessingSlots, "HandleNumberEntries()");
    
  DGPSDTail_HF->AddFrame(PSDTailStop_NEL = new ADAQNumberEntryWithLabel(DGPSDTail_HF,
									"Tail stop",
									PSDTailStop_NEL_ID),
			 new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  PSDTailStop_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDTailStop_NEL->GetEntry()->SetNumber(150);
  PSDTailStop_NEL->GetEntry()->Resize(50,20);
  PSDTailStop_NEL->GetEntry()->Connect("ValueSet(long)", "AAPSDSlots", ProcessingSlots, "HandleNumberEntries()");
  
  PSDFrame_VF->AddFrame(PSDPlotIntegrationLimits_CB = new TGCheckButton(PSDFrame_VF, "Plot integration limits", PSDPlotIntegrationLimits_CB_ID),
			new TGLayoutHints(kLHintsNormal, LOffset0,5,5,5));
  PSDPlotIntegrationLimits_CB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleCheckButtons()");


  ////////////////
  // PSD algorithm
  
  PSDFrame_VF->AddFrame(new TGLabel(PSDFrame_VF, "Analysis algorithm"),
			new TGLayoutHints(kLHintsLeft, LOffset0,0,5,5));

  TGHorizontalFrame *PSDAlgorithm_HF = new TGHorizontalFrame(PSDFrame_VF);
  PSDFrame_VF->AddFrame(PSDAlgorithm_HF, new TGLayoutHints(kLHintsNormal, LOffset0,0,0,0));
  
  PSDAlgorithm_HF->AddFrame(PSDAlgorithmSMS_RB = new TGRadioButton(PSDAlgorithm_HF,"Simple\nmax/sum",PSDAlgorithmSMS_RB_ID),
				 new TGLayoutHints(kLHintsNormal, 0,0,0,5));
  PSDAlgorithmSMS_RB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleRadioButtons()");
  PSDAlgorithmSMS_RB->SetState(kButtonDown);
  
  PSDAlgorithm_HF->AddFrame(PSDAlgorithmPF_RB = new TGRadioButton(PSDAlgorithm_HF,"Peak\nfinder",PSDAlgorithmPF_RB_ID),
				 new TGLayoutHints(kLHintsNormal,35,0,0,5));
  PSDAlgorithmPF_RB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleRadioButtons()");
  
  PSDAlgorithm_HF->AddFrame(PSDAlgorithmWD_RB = new TGRadioButton(PSDAlgorithm_HF,"Waveform\ndata",PSDAlgorithmWD_RB_ID),
				 new TGLayoutHints(kLHintsNormal,35,0,0,5));
  PSDAlgorithmWD_RB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleRadioButtons()");


  ////////////////////////
  // PSD histogram options
  
  PSDFrame_VF->AddFrame(new TGLabel(PSDFrame_VF, "Histogram options"),
			new TGLayoutHints(kLHintsLeft, LOffset0,0,5,5));
  
  TGHorizontalFrame *PSDPlotOptions_HF = new TGHorizontalFrame(PSDFrame_VF);
  PSDFrame_VF->AddFrame(PSDPlotOptions_HF, new TGLayoutHints(kLHintsNormal, LOffset0,0,0,0));
  
  PSDPlotOptions_HF->AddFrame(PSDPlotType_CBL = new ADAQComboBoxWithLabel(PSDPlotOptions_HF, "Style", PSDPlotType_CBL_ID),
			      new TGLayoutHints(kLHintsNormal, 0,5,0,5));
  PSDPlotType_CBL->GetComboBox()->AddEntry("COLZ",0);
  PSDPlotType_CBL->GetComboBox()->AddEntry("LEGO",1);
  PSDPlotType_CBL->GetComboBox()->AddEntry("LEGO1",2);
  PSDPlotType_CBL->GetComboBox()->AddEntry("LEGO2Z0",3);
  PSDPlotType_CBL->GetComboBox()->AddEntry("SURF",4);
  PSDPlotType_CBL->GetComboBox()->AddEntry("SURF1Z0",5);
  PSDPlotType_CBL->GetComboBox()->AddEntry("SURF3Z0",6);
  PSDPlotType_CBL->GetComboBox()->AddEntry("CONTZ0",7);
  PSDPlotType_CBL->GetComboBox()->AddEntry("CONT1Z",8);
  PSDPlotType_CBL->GetComboBox()->AddEntry("SCAT",9);
  PSDPlotType_CBL->GetComboBox()->AddEntry("TEXT",10);
  PSDPlotType_CBL->GetComboBox()->Select(0);
  PSDPlotType_CBL->GetComboBox()->Resize(80, 20);
  PSDPlotType_CBL->GetComboBox()->Connect("Selected(int,int)", "AAPSDSlots", ProcessingSlots, "HandleComboBoxes(int,int)");

  PSDPlotOptions_HF->AddFrame(PSDPlotPalette_CBL = new ADAQComboBoxWithLabel(PSDPlotOptions_HF, "Palette", PSDPlotPalette_CBL_ID),
			      new TGLayoutHints(kLHintsNormal, LOffset0,5,0,5));
  PSDPlotPalette_CBL->GetComboBox()->AddEntry("Bird", kBird);
  PSDPlotPalette_CBL->GetComboBox()->AddEntry("Rainbow", kRainBow);
  PSDPlotPalette_CBL->GetComboBox()->AddEntry("Radiator0", kDarkBodyRadiator);
  PSDPlotPalette_CBL->GetComboBox()->AddEntry("Radiator1", kInvertedDarkBodyRadiator);
  PSDPlotPalette_CBL->GetComboBox()->AddEntry("BRY", kBlueRedYellow);
  PSDPlotPalette_CBL->GetComboBox()->AddEntry("Avocado", kAvocado);
  PSDPlotPalette_CBL->GetComboBox()->AddEntry("BlueRed", kBlackBody);
  PSDPlotPalette_CBL->GetComboBox()->AddEntry("Cool", kCool);
  PSDPlotPalette_CBL->GetComboBox()->AddEntry("Mint", kMint);
  PSDPlotPalette_CBL->GetComboBox()->Select(kBird);
  PSDPlotPalette_CBL->GetComboBox()->Resize(80, 20);
  PSDPlotPalette_CBL->GetComboBox()->Connect("Selected(int,int)", "AAPSDSlots", ProcessingSlots, "HandleComboBoxes(int,int)");
  

  /////////////////
  // Create/process 
  
  TGHorizontalFrame *ProcessCreatePSDHistogram_HF = new TGHorizontalFrame(PSDFrame_VF);
  PSDFrame_VF->AddFrame(ProcessCreatePSDHistogram_HF, new TGLayoutHints(kLHintsCenterX, 5,5,5,5));
  
  ProcessCreatePSDHistogram_HF->AddFrame(ProcessPSDHistogram_TB = new TGTextButton(ProcessCreatePSDHistogram_HF, "Process waveforms", ProcessPSDHistogram_TB_ID),
					 new TGLayoutHints(kLHintsCenterX | kLHintsTop, 0,5,5,0));
  ProcessPSDHistogram_TB->Resize(120, 30);
  ProcessPSDHistogram_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  ProcessPSDHistogram_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  ProcessPSDHistogram_TB->ChangeOptions(ProcessPSDHistogram_TB->GetOptions() | kFixedSize);
  ProcessPSDHistogram_TB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleTextButtons()");
  
  ProcessCreatePSDHistogram_HF->AddFrame(CreatePSDHistogram_TB = new TGTextButton(ProcessCreatePSDHistogram_HF, "Create PSD hist", CreatePSDHistogram_TB_ID),
					 new TGLayoutHints(kLHintsCenterX | kLHintsTop, 5,0,5,0));
  CreatePSDHistogram_TB->Resize(120, 30);
  CreatePSDHistogram_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  CreatePSDHistogram_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  CreatePSDHistogram_TB->ChangeOptions(CreatePSDHistogram_TB->GetOptions() | kFixedSize);
  CreatePSDHistogram_TB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleTextButtons()");
  CreatePSDHistogram_TB->SetState(kButtonDisabled);
  
  
  //////////////////////////
  // PSD analysis widgets //
  //////////////////////////
  
  //////////////////////
  // PSD region creation
  
  TGGroupFrame *PSDRegion_GF = new TGGroupFrame(PSDFrame_VF, "PSD region creation", kVerticalFrame);
  PSDFrame_VF->AddFrame(PSDRegion_GF, new TGLayoutHints(kLHintsCenterX | kLHintsExpandX, 5,5,10,5));
  
  PSDRegion_GF->AddFrame(PSDEnableRegionCreation_CB = new TGCheckButton(PSDRegion_GF, "Enable PSD region creation", PSDEnableRegionCreation_CB_ID),
			   new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  PSDEnableRegionCreation_CB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleCheckButtons()");
  PSDEnableRegionCreation_CB->SetState(kButtonDisabled);

  TGHorizontalFrame *PSDCreateClear_HF = new TGHorizontalFrame(PSDRegion_GF);
  PSDRegion_GF->AddFrame(PSDCreateClear_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  
  PSDCreateClear_HF->AddFrame(PSDCreateRegion_TB = new TGTextButton(PSDCreateClear_HF, "Create PSD region", PSDCreateRegion_TB_ID),
			   new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  PSDCreateRegion_TB->Resize(115,30);
  PSDCreateRegion_TB->ChangeOptions(PSDCreateRegion_TB->GetOptions() | kFixedSize);
  PSDCreateRegion_TB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleTextButtons()");
  PSDCreateRegion_TB->SetState(kButtonDisabled);

  PSDCreateClear_HF->AddFrame(PSDClearRegion_TB = new TGTextButton(PSDCreateClear_HF, "Clear PSD region", PSDClearRegion_TB_ID),
			   new TGLayoutHints(kLHintsLeft, 5,5,5,5));
  PSDClearRegion_TB->Resize(115,30);
  PSDClearRegion_TB->ChangeOptions(PSDClearRegion_TB->GetOptions() | kFixedSize);
  PSDClearRegion_TB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleTextButtons()");
  PSDClearRegion_TB->SetState(kButtonDisabled);

  
  PSDRegion_GF->AddFrame(PSDEnableRegion_CB = new TGCheckButton(PSDRegion_GF, "Enable PSD region", PSDEnableRegion_CB_ID),
			   new TGLayoutHints(kLHintsNormal, 0,5,10,0));
  PSDEnableRegion_CB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleCheckButtons()");
  PSDEnableRegion_CB->SetState(kButtonDisabled);

  TGHorizontalFrame *PSDRegionPolarity_HF = new TGHorizontalFrame(PSDRegion_GF);
  PSDRegion_GF->AddFrame(PSDRegionPolarity_HF, new TGLayoutHints(kLHintsNormal, 15,5,0,5));
  
  PSDRegionPolarity_HF->AddFrame(PSDInsideRegion_RB = new TGRadioButton(PSDRegionPolarity_HF, "Inside  ", PSDInsideRegion_RB_ID),
				 new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  PSDInsideRegion_RB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleRadioButtons()");
  PSDInsideRegion_RB->SetState(kButtonDown);
  PSDInsideRegion_RB->SetState(kButtonDisabled);

  PSDRegionPolarity_HF->AddFrame(PSDOutsideRegion_RB = new TGRadioButton(PSDRegionPolarity_HF, "Outside", PSDOutsideRegion_RB_ID),
				 new TGLayoutHints(kLHintsNormal, 20,0,0,0));
  PSDOutsideRegion_RB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleRadioButtons()");
  PSDOutsideRegion_RB->SetState(kButtonDisabled);


  ///////////////////////////
  // PSD slicing and analysis
  
  TGGroupFrame *PSDSlicing_GF = new TGGroupFrame(PSDFrame_VF, "PSD slicing and 1D analysis", kVerticalFrame);
  PSDFrame_VF->AddFrame(PSDSlicing_GF, new TGLayoutHints(kLHintsCenterX | kLHintsExpandX, 5,5,10,5));
  
  PSDSlicing_GF->AddFrame(PSDEnableHistogramSlicing_CB = new TGCheckButton(PSDSlicing_GF, "Enable histogram slicing", PSDEnableHistogramSlicing_CB_ID),
			   new TGLayoutHints(kLHintsNormal, 0,5,10,0));
  PSDEnableHistogramSlicing_CB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleCheckButtons()");
  PSDEnableHistogramSlicing_CB->SetState(kButtonDisabled);

  TGHorizontalFrame *PSDHistogramSlicing_HF = new TGHorizontalFrame(PSDSlicing_GF);
  PSDSlicing_GF->AddFrame(PSDHistogramSlicing_HF, new TGLayoutHints(kLHintsNormal, 15,5,0,5));
  
  PSDHistogramSlicing_HF->AddFrame(PSDHistogramSliceX_RB = new TGRadioButton(PSDHistogramSlicing_HF, "X slice", PSDHistogramSliceX_RB_ID),
				   new TGLayoutHints(kLHintsNormal, 0,0,3,0));
  PSDHistogramSliceX_RB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleRadioButtons()");
  PSDHistogramSliceX_RB->SetState(kButtonDown);
  PSDHistogramSliceX_RB->SetState(kButtonDisabled);

  PSDHistogramSlicing_HF->AddFrame(PSDHistogramSliceY_RB = new TGRadioButton(PSDHistogramSlicing_HF, "Y slice", PSDHistogramSliceY_RB_ID),
				   new TGLayoutHints(kLHintsNormal, 20,0,3,0));
  PSDHistogramSliceY_RB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleRadioButtons()");
  PSDHistogramSliceY_RB->SetState(kButtonDisabled);

  
  PSDSlicing_GF->AddFrame(PSDCalculateFOM_CB = new TGCheckButton(PSDSlicing_GF, "Calculate FOM", PSDCalculateFOM_CB_ID),
			  new TGLayoutHints(kLHintsNormal, 0,5,10,0));
  PSDCalculateFOM_CB->Connect("Clicked()", "AAPSDSlots", ProcessingSlots, "HandleCheckButtons()");
  PSDCalculateFOM_CB->SetState(kButtonDisabled);

  
  PSDSlicing_GF->AddFrame(new TGLabel(PSDSlicing_GF, "Lower PSD fit range (ADC)"),
			  new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  
  TGHorizontalFrame *PSDFOM_HF0 = new TGHorizontalFrame(PSDSlicing_GF);
  PSDSlicing_GF->AddFrame(PSDFOM_HF0, new TGLayoutHints(kLHintsNormal, 15,0,0,0));

  PSDFOM_HF0->AddFrame(PSDLowerFOMFitMin_NEL = new ADAQNumberEntryWithLabel(PSDFOM_HF0, "Minimum  ", PSDLowerFOMFitMin_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 0,0,5,0));
  PSDLowerFOMFitMin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PSDLowerFOMFitMin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  PSDLowerFOMFitMin_NEL->GetEntry()->SetNumber(0);
  PSDLowerFOMFitMin_NEL->GetEntry()->Connect("ValueSet(long)", "AAPSDSlots", SpectrumSlots, "HandleNumberEntries()");
  PSDLowerFOMFitMin_NEL->GetEntry()->SetState(false);
  
  PSDFOM_HF0->AddFrame(PSDLowerFOMFitMax_NEL = new ADAQNumberEntryWithLabel(PSDFOM_HF0, "Maximum  ", PSDLowerFOMFitMax_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 0,0,5,0));
  PSDLowerFOMFitMax_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PSDLowerFOMFitMax_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  PSDLowerFOMFitMax_NEL->GetEntry()->SetNumber(0.15);
  PSDLowerFOMFitMax_NEL->GetEntry()->Connect("ValueSet(long)", "AAPSDSlots", SpectrumSlots, "HandleNumberEntries()");
  PSDLowerFOMFitMax_NEL->GetEntry()->SetState(false);

  
  PSDSlicing_GF->AddFrame(new TGLabel(PSDSlicing_GF, "Upper PSD fit range (ADC)"),
			  new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  
  TGHorizontalFrame *PSDFOM_HF1 = new TGHorizontalFrame(PSDSlicing_GF);
  PSDSlicing_GF->AddFrame(PSDFOM_HF1, new TGLayoutHints(kLHintsNormal, 15,0,0,0));

  PSDFOM_HF1->AddFrame(PSDUpperFOMFitMin_NEL = new ADAQNumberEntryWithLabel(PSDFOM_HF1, "Minimum  ", PSDUpperFOMFitMin_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 0,0,5,0));
  PSDUpperFOMFitMin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PSDUpperFOMFitMin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  PSDUpperFOMFitMin_NEL->GetEntry()->SetNumber(0.15);
  PSDUpperFOMFitMin_NEL->GetEntry()->Connect("ValueSet(long)", "AAPSDSlots", SpectrumSlots, "HandleNumberEntries()");
  PSDUpperFOMFitMin_NEL->GetEntry()->SetState(false);
  
  PSDFOM_HF1->AddFrame(PSDUpperFOMFitMax_NEL = new ADAQNumberEntryWithLabel(PSDFOM_HF1, "Maximum  ", PSDUpperFOMFitMax_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 0,0,5,0));
  PSDUpperFOMFitMax_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PSDUpperFOMFitMax_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  PSDUpperFOMFitMax_NEL->GetEntry()->SetNumber(0.35);
  PSDUpperFOMFitMax_NEL->GetEntry()->Connect("ValueSet(long)", "AAPSDSlots", SpectrumSlots, "HandleNumberEntries()");
  PSDUpperFOMFitMax_NEL->GetEntry()->SetState(false);

  PSDSlicing_GF->AddFrame(new TGLabel(PSDSlicing_GF, "PSD figure of merit"),
			  new TGLayoutHints(kLHintsLeft, 15,5,5,0));

  PSDSlicing_GF->AddFrame(PSDFigureOfMerit_NEFL = new ADAQNumberEntryFieldWithLabel(PSDSlicing_GF, "", -1),
			  new TGLayoutHints(kLHintsNormal, 15,0,5,0));
  PSDFigureOfMerit_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal, TGNumberFormat::kNEAAnyNumber);
  PSDFigureOfMerit_NEFL->GetEntry()->Resize(75, 20);
  PSDFigureOfMerit_NEFL->GetEntry()->SetBackgroundColor(ColorMgr->Number2Pixel(19));
  PSDFigureOfMerit_NEFL->GetEntry()->SetState(false);
}


void AAInterface::FillGraphicsFrame()
{
  /////////////////////////////////////////
  // Fill the graphics options tabbed frame

  TGCanvas *GraphicsFrame_C = new TGCanvas(GraphicsOptions_CF, TabFrameWidth-10, TabFrameLength-20, kDoubleBorder);
  GraphicsOptions_CF->AddFrame(GraphicsFrame_C, new TGLayoutHints(kLHintsCenterX, 0,0,10,10));
  
  TGVerticalFrame *GraphicsFrame_VF = new TGVerticalFrame(GraphicsFrame_C->GetViewPort(), TabFrameWidth-10, TabFrameLength);
  GraphicsFrame_C->SetContainer(GraphicsFrame_VF);

  ////////////////////////////
  // Waveform graphics options

  TGGroupFrame *WaveformDrawOptions_GF = new TGGroupFrame(GraphicsFrame_VF, "Waveform draw options", kVerticalFrame);
  GraphicsFrame_VF->AddFrame(WaveformDrawOptions_GF, new TGLayoutHints(kLHintsLeft, 5,5,5,5));

  TGHorizontalFrame *WaveformDrawOptions_HF0 = new TGHorizontalFrame(WaveformDrawOptions_GF);
  WaveformDrawOptions_GF->AddFrame(WaveformDrawOptions_HF0, new TGLayoutHints(kLHintsCenterX, 3,-10,3,0));
  
  WaveformDrawOptions_HF0->AddFrame(DrawWaveformWithLine_RB = new TGRadioButton(WaveformDrawOptions_HF0, "Line   ", DrawWaveformWithLine_RB_ID),
				    new TGLayoutHints(kLHintsLeft, 0,0,5,5));
  DrawWaveformWithLine_RB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleRadioButtons()");
  DrawWaveformWithLine_RB->SetState(kButtonDown);

  WaveformDrawOptions_HF0->AddFrame(DrawWaveformWithCurve_RB = new TGRadioButton(WaveformDrawOptions_HF0, "Curve   ", DrawWaveformWithCurve_RB_ID),
				    new TGLayoutHints(kLHintsLeft, 0,0,5,5));
  DrawWaveformWithCurve_RB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleRadioButtons()");
  
  WaveformDrawOptions_HF0->AddFrame(DrawWaveformWithMarkers_RB = new TGRadioButton(WaveformDrawOptions_HF0, "Markers   ", DrawWaveformWithMarkers_RB_ID),
				    new TGLayoutHints(kLHintsLeft, 0,0,5,5));
  DrawWaveformWithMarkers_RB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleRadioButtons()");
  
  WaveformDrawOptions_HF0->AddFrame(DrawWaveformWithBoth_RB = new TGRadioButton(WaveformDrawOptions_HF0, "Both", DrawWaveformWithBoth_RB_ID),
				    new TGLayoutHints(kLHintsLeft, 0,0,5,-10));
  DrawWaveformWithBoth_RB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleRadioButtons()");


  TGHorizontalFrame *WaveformDrawOptions_HF1 = new TGHorizontalFrame(WaveformDrawOptions_GF);
  WaveformDrawOptions_GF->AddFrame(WaveformDrawOptions_HF1, new TGLayoutHints(kLHintsCenterX, 5,5,3,3));

  WaveformDrawOptions_HF1->AddFrame(WaveformColor_TB = new TGTextButton(WaveformDrawOptions_HF1, "Color", WaveformColor_TB_ID),
				    new TGLayoutHints(kLHintsCenterX, 10,0,0,0));
  WaveformColor_TB->SetBackgroundColor(ColorMgr->Number2Pixel(kBlue));
  WaveformColor_TB->SetForegroundColor(ColorMgr->Number2Pixel(kWhite));
  WaveformColor_TB->Resize(60, 20);
  WaveformColor_TB->ChangeOptions(WaveformColor_TB->GetOptions() | kFixedSize);
  WaveformColor_TB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleTextButtons()");

  WaveformDrawOptions_HF1->AddFrame(WaveformLineWidth_NEL = new ADAQNumberEntryWithLabel(WaveformDrawOptions_HF1, "Line", WaveformLineWidth_NEL_ID),
				    new TGLayoutHints(kLHintsCenterX, 10,0,0,0));
  WaveformLineWidth_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  WaveformLineWidth_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  WaveformLineWidth_NEL->GetEntry()->SetNumber(1);
  WaveformLineWidth_NEL->GetEntry()->Resize(40, 20);
  WaveformLineWidth_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");

  WaveformDrawOptions_HF1->AddFrame(WaveformMarkerSize_NEL = new ADAQNumberEntryWithLabel(WaveformDrawOptions_HF1, "Marker", WaveformMarkerSize_NEL_ID),
				    new TGLayoutHints(kLHintsCenterX, 5,0,0,0));
  WaveformMarkerSize_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  WaveformMarkerSize_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  WaveformMarkerSize_NEL->GetEntry()->SetNumber(1);
  WaveformMarkerSize_NEL->GetEntry()->Resize(40, 20);
  WaveformMarkerSize_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");


  ////////////////////////////
  // Spectrum graphics options

  TGGroupFrame *SpectrumDrawOptions_GF = new TGGroupFrame(GraphicsFrame_VF, "Spectrum draw options", kVerticalFrame);
  GraphicsFrame_VF->AddFrame(SpectrumDrawOptions_GF, new TGLayoutHints(kLHintsLeft, 5,5,5,5));

  TGHorizontalFrame *SpectrumDrawOptions_HF0 = new TGHorizontalFrame(SpectrumDrawOptions_GF);
  SpectrumDrawOptions_GF->AddFrame(SpectrumDrawOptions_HF0, new TGLayoutHints(kLHintsCenterX, 3,0,3,0));

  SpectrumDrawOptions_HF0->AddFrame(DrawSpectrumWithLine_RB = new TGRadioButton(SpectrumDrawOptions_HF0, "Line  ", DrawSpectrumWithLine_RB_ID),
				    new TGLayoutHints(kLHintsCenterX, 15,0,5,5));
  DrawSpectrumWithLine_RB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleRadioButtons()");
  DrawSpectrumWithLine_RB->SetState(kButtonDown);

  SpectrumDrawOptions_HF0->AddFrame(DrawSpectrumWithCurve_RB = new TGRadioButton(SpectrumDrawOptions_HF0, "Curve  ", DrawSpectrumWithCurve_RB_ID),
				    new TGLayoutHints(kLHintsCenterX, 5,0,5,5));
  DrawSpectrumWithCurve_RB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleRadioButtons()");
  
  SpectrumDrawOptions_HF0->AddFrame(DrawSpectrumWithError_RB = new TGRadioButton(SpectrumDrawOptions_HF0, "Error  ", DrawSpectrumWithError_RB_ID),
				    new TGLayoutHints(kLHintsCenterX, 5,0,5,5));
  DrawSpectrumWithError_RB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleRadioButtons()");

  SpectrumDrawOptions_HF0->AddFrame(DrawSpectrumWithBars_RB = new TGRadioButton(SpectrumDrawOptions_HF0, "Bars", DrawSpectrumWithBars_RB_ID),
				    new TGLayoutHints(kLHintsCenterX, 5,-10,5,5));
  DrawSpectrumWithBars_RB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleRadioButtons()");

  
  TGHorizontalFrame *SpectrumDrawOptions_HF1 = new TGHorizontalFrame(SpectrumDrawOptions_GF);
  SpectrumDrawOptions_GF->AddFrame(SpectrumDrawOptions_HF1, new TGLayoutHints(kLHintsCenterX, 3,0,3,0));

  SpectrumDrawOptions_HF1->AddFrame(SpectrumLineColor_TB = new TGTextButton(SpectrumDrawOptions_HF1, "Line Color", SpectrumLineColor_TB_ID),
				    new TGLayoutHints(kLHintsCenterX, 10,0,0,0));
  SpectrumLineColor_TB->SetBackgroundColor(ColorMgr->Number2Pixel(kBlue));
  SpectrumLineColor_TB->SetForegroundColor(ColorMgr->Number2Pixel(kWhite));
  SpectrumLineColor_TB->Resize(80, 20);
  SpectrumLineColor_TB->ChangeOptions(SpectrumLineColor_TB->GetOptions() | kFixedSize);
  SpectrumLineColor_TB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleTextButtons()");

  SpectrumDrawOptions_HF1->AddFrame(SpectrumLineWidth_NEL = new ADAQNumberEntryWithLabel(SpectrumDrawOptions_HF1, "Line width", SpectrumLineWidth_NEL_ID),
				    new TGLayoutHints(kLHintsCenterX, 10,0,0,0));
  SpectrumLineWidth_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  SpectrumLineWidth_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumLineWidth_NEL->GetEntry()->SetNumber(2);
  SpectrumLineWidth_NEL->GetEntry()->Resize(50, 20);
  SpectrumLineWidth_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");


  TGHorizontalFrame *SpectrumDrawOptions_HF2 = new TGHorizontalFrame(SpectrumDrawOptions_GF);
  SpectrumDrawOptions_GF->AddFrame(SpectrumDrawOptions_HF2, new TGLayoutHints(kLHintsCenterX, 3,0,3,0));

  SpectrumDrawOptions_HF2->AddFrame(SpectrumFillColor_TB = new TGTextButton(SpectrumDrawOptions_HF2, "Fill Color", SpectrumFillColor_TB_ID),
				    new TGLayoutHints(kLHintsCenterX, 10,0,0,0));
  SpectrumFillColor_TB->SetBackgroundColor(ColorMgr->Number2Pixel(kRed));
  SpectrumFillColor_TB->SetForegroundColor(ColorMgr->Number2Pixel(kWhite));
  SpectrumFillColor_TB->Resize(80, 20);
  SpectrumFillColor_TB->ChangeOptions(SpectrumFillColor_TB->GetOptions() | kFixedSize);
  SpectrumFillColor_TB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleTextButtons()");

  SpectrumDrawOptions_HF2->AddFrame(SpectrumFillStyle_NEL = new ADAQNumberEntryWithLabel(SpectrumDrawOptions_HF2, "Fill style", SpectrumFillStyle_NEL_ID),
				    new TGLayoutHints(kLHintsCenterX, 10,0,0,0));
  SpectrumFillStyle_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  SpectrumFillStyle_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumFillStyle_NEL->GetEntry()->SetNumber(3001);
  SpectrumFillStyle_NEL->GetEntry()->Resize(50, 20);
  SpectrumFillStyle_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");


  TGHorizontalFrame *CanvasOptions_HF0 = new TGHorizontalFrame(GraphicsFrame_VF);
  GraphicsFrame_VF->AddFrame(CanvasOptions_HF0, new TGLayoutHints(kLHintsLeft, 5,5,5,0));

  CanvasOptions_HF0->AddFrame(HistogramStats_CB = new TGCheckButton(CanvasOptions_HF0, "Histogram stats", HistogramStats_CB_ID),
			      new TGLayoutHints(kLHintsLeft, 0,0,0,0));
  HistogramStats_CB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleCheckButtons()");
  
  CanvasOptions_HF0->AddFrame(CanvasGrid_CB = new TGCheckButton(CanvasOptions_HF0, "Grid", CanvasGrid_CB_ID),
			      new TGLayoutHints(kLHintsLeft, 25,0,0,0));
  CanvasGrid_CB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleCheckButtons()");
  CanvasGrid_CB->SetState(kButtonDown);


  TGHorizontalFrame *CanvasOptions_HF1 = new TGHorizontalFrame(GraphicsFrame_VF);
  GraphicsFrame_VF->AddFrame(CanvasOptions_HF1, new TGLayoutHints(kLHintsLeft, 5,5,5,0));
  
  CanvasOptions_HF1->AddFrame(CanvasXAxisLog_CB = new TGCheckButton(CanvasOptions_HF1, "X-axis log", CanvasXAxisLog_CB_ID),
			     new TGLayoutHints(kLHintsLeft, 0,0,0,0));
  CanvasXAxisLog_CB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleCheckButtons()");
  
  CanvasOptions_HF1->AddFrame(CanvasYAxisLog_CB = new TGCheckButton(CanvasOptions_HF1, "Y-axis log", CanvasYAxisLog_CB_ID),
			      new TGLayoutHints(kLHintsLeft, 15,0,0,0));
  CanvasYAxisLog_CB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleCheckButtons()");
  
  CanvasOptions_HF1->AddFrame(CanvasZAxisLog_CB = new TGCheckButton(CanvasOptions_HF1, "Z-axis log", CanvasZAxisLog_CB_ID),
			      new TGLayoutHints(kLHintsLeft, 5,5,0,0));
  CanvasZAxisLog_CB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleCheckButtons()");
  

  GraphicsFrame_VF->AddFrame(PlotSpectrumDerivativeError_CB = new TGCheckButton(GraphicsFrame_VF, "Plot spectrum derivative error", PlotSpectrumDerivativeError_CB_ID),
			     new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  PlotSpectrumDerivativeError_CB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleCheckButtons()");
  
  GraphicsFrame_VF->AddFrame(PlotAbsValueSpectrumDerivative_CB = new TGCheckButton(GraphicsFrame_VF, "Plot abs. value of spectrum derivative ", PlotAbsValueSpectrumDerivative_CB_ID),
			     new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  PlotAbsValueSpectrumDerivative_CB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleCheckButtons()");
  
  TGHorizontalFrame *ResetAxesLimits_HF = new TGHorizontalFrame(GraphicsFrame_VF);
  GraphicsFrame_VF->AddFrame(ResetAxesLimits_HF, new TGLayoutHints(kLHintsLeft,15,5,5,5));

  TGHorizontalFrame *PlotButtons_HF0 = new TGHorizontalFrame(GraphicsFrame_VF);
  GraphicsFrame_VF->AddFrame(PlotButtons_HF0, new TGLayoutHints(kLHintsCenterX, 5,5,5,5));
  
  PlotButtons_HF0->AddFrame(ReplotWaveform_TB = new TGTextButton(PlotButtons_HF0, "Plot waveform", ReplotWaveform_TB_ID),
			    new TGLayoutHints(kLHintsCenterX, 5,5,0,0));
  ReplotWaveform_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  ReplotWaveform_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  ReplotWaveform_TB->Resize(130, 30);
  ReplotWaveform_TB->ChangeOptions(ReplotWaveform_TB->GetOptions() | kFixedSize);
  ReplotWaveform_TB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleTextButtons()");
  
  PlotButtons_HF0->AddFrame(ReplotSpectrum_TB = new TGTextButton(PlotButtons_HF0, "Plot spectrum", ReplotSpectrum_TB_ID),
			    new TGLayoutHints(kLHintsCenterX, 5,5,0,0));
  ReplotSpectrum_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  ReplotSpectrum_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  ReplotSpectrum_TB->Resize(130, 30);
  ReplotSpectrum_TB->ChangeOptions(ReplotSpectrum_TB->GetOptions() | kFixedSize);
  ReplotSpectrum_TB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleTextButtons()");
  

  TGHorizontalFrame *PlotButtons_HF1 = new TGHorizontalFrame(GraphicsFrame_VF);
  GraphicsFrame_VF->AddFrame(PlotButtons_HF1, new TGLayoutHints(kLHintsCenterX, 5,5,5,20));

  PlotButtons_HF1->AddFrame(ReplotSpectrumDerivative_TB = new TGTextButton(PlotButtons_HF1, "Plot derivative", ReplotSpectrumDerivative_TB_ID),
			    new TGLayoutHints(kLHintsCenterX, 5,5,0,0));
  ReplotSpectrumDerivative_TB->Resize(130, 30);
  ReplotSpectrumDerivative_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  ReplotSpectrumDerivative_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  ReplotSpectrumDerivative_TB->ChangeOptions(ReplotSpectrumDerivative_TB->GetOptions() | kFixedSize);
  ReplotSpectrumDerivative_TB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleTextButtons()");

  PlotButtons_HF1->AddFrame(ReplotPSDHistogram_TB = new TGTextButton(PlotButtons_HF1, "Plot PSD histogram", ReplotPSDHistogram_TB_ID),
			    new TGLayoutHints(kLHintsCenterX, 5,5,0,0));
  ReplotPSDHistogram_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  ReplotPSDHistogram_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  ReplotPSDHistogram_TB->Resize(130, 30);
  ReplotPSDHistogram_TB->ChangeOptions(ReplotPSDHistogram_TB->GetOptions() | kFixedSize);
  ReplotPSDHistogram_TB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleTextButtons()");

  // Override default plot options
  GraphicsFrame_VF->AddFrame(OverrideTitles_CB = new TGCheckButton(GraphicsFrame_VF, "Override default titles", OverrideTitles_CB_ID),
			       new TGLayoutHints(kLHintsCenterX, 5,5,5,5));
  OverrideTitles_CB->Connect("Clicked()", "AAGraphicsSlots", GraphicsSlots, "HandleCheckButtons()");

  // Plot title text entry
  GraphicsFrame_VF->AddFrame(Title_TEL = new ADAQTextEntryWithLabel(GraphicsFrame_VF, "Title", Title_TEL_ID),
			       new TGLayoutHints(kLHintsCenterX, 5,5,5,5));
  Title_TEL->GetEntry()->Resize(220,20);

  // X-axis title options

  TGGroupFrame *XAxis_GF = new TGGroupFrame(GraphicsFrame_VF, "X axis title and labels", kVerticalFrame);
  GraphicsFrame_VF->AddFrame(XAxis_GF, new TGLayoutHints(kLHintsCenterX, 5,5,5,5));

  XAxis_GF->AddFrame(XAxisTitle_TEL = new ADAQTextEntryWithLabel(XAxis_GF, "", XAxisTitle_TEL_ID),
		     new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  XAxisTitle_TEL->GetEntry()->Resize(240, 20);

  TGHorizontalFrame *XAxisTitleOptions_HF = new TGHorizontalFrame(XAxis_GF);
  XAxis_GF->AddFrame(XAxisTitleOptions_HF, new TGLayoutHints(kLHintsNormal, 0,5,0,5));
  
  XAxisTitleOptions_HF->AddFrame(XAxisSize_NEL = new ADAQNumberEntryWithLabel(XAxisTitleOptions_HF, "Size", XAxisSize_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  XAxisSize_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  XAxisSize_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  XAxisSize_NEL->GetEntry()->SetNumber(0.055);
  XAxisSize_NEL->GetEntry()->Resize(60, 20);
  XAxisSize_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");
  
  XAxisTitleOptions_HF->AddFrame(XAxisOffset_NEL = new ADAQNumberEntryWithLabel(XAxisTitleOptions_HF, "Offset", XAxisOffset_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 5,2,0,0));
  XAxisOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  XAxisOffset_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  XAxisOffset_NEL->GetEntry()->SetNumber(1.0);
  XAxisOffset_NEL->GetEntry()->Resize(60, 20);
  XAxisOffset_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");

  XAxis_GF->AddFrame(XAxisDivs_NEL = new ADAQNumberEntryWithLabel(XAxis_GF, "Divisions", XAxisDivs_NEL_ID),		
		     new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  XAxisDivs_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  XAxisDivs_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  XAxisDivs_NEL->GetEntry()->SetNumber(505);
  XAxisDivs_NEL->GetEntry()->Resize(60, 20);
  XAxisDivs_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");

  // Y-axis title options

  TGGroupFrame *YAxis_GF = new TGGroupFrame(GraphicsFrame_VF, "Y axis title and labels", kVerticalFrame);
  GraphicsFrame_VF->AddFrame(YAxis_GF, new TGLayoutHints(kLHintsCenterX, 5,5,5,5));
  
  YAxis_GF->AddFrame(YAxisTitle_TEL = new ADAQTextEntryWithLabel(YAxis_GF, "", YAxisTitle_TEL_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  YAxisTitle_TEL->GetEntry()->Resize(240, 20);
  
  TGHorizontalFrame *YAxisTitleOptions_HF = new TGHorizontalFrame(YAxis_GF);
  YAxis_GF->AddFrame(YAxisTitleOptions_HF, new TGLayoutHints(kLHintsNormal, 0,5,0,5));

  YAxisTitleOptions_HF->AddFrame(YAxisSize_NEL = new ADAQNumberEntryWithLabel(YAxisTitleOptions_HF, "Size", YAxisSize_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  YAxisSize_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  YAxisSize_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  YAxisSize_NEL->GetEntry()->SetNumber(0.055);
  YAxisSize_NEL->GetEntry()->Resize(60, 20);
  YAxisSize_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");
  
  YAxisTitleOptions_HF->AddFrame(YAxisOffset_NEL = new ADAQNumberEntryWithLabel(YAxisTitleOptions_HF, "Offset", YAxisOffset_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 5,2,0,0));
  YAxisOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  YAxisOffset_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  YAxisOffset_NEL->GetEntry()->SetNumber(1.2);
  YAxisOffset_NEL->GetEntry()->Resize(60, 20);
  YAxisOffset_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");

  YAxis_GF->AddFrame(YAxisDivs_NEL = new ADAQNumberEntryWithLabel(YAxis_GF, "Divisions", YAxisDivs_NEL_ID),		
		     new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  YAxisDivs_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  YAxisDivs_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  YAxisDivs_NEL->GetEntry()->SetNumber(505);
  YAxisDivs_NEL->GetEntry()->Resize(60, 20);
  YAxisDivs_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");

  // Z-axis options

  TGGroupFrame *ZAxis_GF = new TGGroupFrame(GraphicsFrame_VF, "Z axis titles and labels", kVerticalFrame);
  GraphicsFrame_VF->AddFrame(ZAxis_GF, new TGLayoutHints(kLHintsCenterX, 5,5,5,5));

  ZAxis_GF->AddFrame(ZAxisTitle_TEL = new ADAQTextEntryWithLabel(ZAxis_GF, "", ZAxisTitle_TEL_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  ZAxisTitle_TEL->GetEntry()->Resize(240,20);

  TGHorizontalFrame *ZAxisTitleOptions_HF = new TGHorizontalFrame(ZAxis_GF);
  ZAxis_GF->AddFrame(ZAxisTitleOptions_HF, new TGLayoutHints(kLHintsNormal, 0,5,0,5));

  ZAxisTitleOptions_HF->AddFrame(ZAxisSize_NEL = new ADAQNumberEntryWithLabel(ZAxisTitleOptions_HF, "Size", ZAxisSize_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  ZAxisSize_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  ZAxisSize_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  ZAxisSize_NEL->GetEntry()->SetNumber(0.055);
  ZAxisSize_NEL->GetEntry()->Resize(60, 20);
  ZAxisSize_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");
  
  ZAxisTitleOptions_HF->AddFrame(ZAxisOffset_NEL = new ADAQNumberEntryWithLabel(ZAxisTitleOptions_HF, "Offset", ZAxisOffset_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 5,2,0,0));
  ZAxisOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  ZAxisOffset_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  ZAxisOffset_NEL->GetEntry()->SetNumber(1.0);
  ZAxisOffset_NEL->GetEntry()->Resize(60, 20);
  ZAxisOffset_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");

  ZAxis_GF->AddFrame(ZAxisDivs_NEL = new ADAQNumberEntryWithLabel(ZAxis_GF, "Divisions", ZAxisDivs_NEL_ID),		
		     new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  ZAxisDivs_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  ZAxisDivs_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  ZAxisDivs_NEL->GetEntry()->SetNumber(510);
  ZAxisDivs_NEL->GetEntry()->Resize(60, 20);
  ZAxisDivs_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");

  // Color palette options

  TGGroupFrame *PaletteAxis_GF = new TGGroupFrame(GraphicsFrame_VF, "Palette axis title and labels", kVerticalFrame);
  GraphicsFrame_VF->AddFrame(PaletteAxis_GF, new TGLayoutHints(kLHintsCenterX, 5,5,5,5));

  PaletteAxis_GF->AddFrame(PaletteAxisTitle_TEL = new ADAQTextEntryWithLabel(PaletteAxis_GF, "", PaletteAxisTitle_TEL_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  PaletteAxisTitle_TEL->GetEntry()->Resize(240,20);
  
  // A horizontal frame for the Z-axis title/label size and offset
  TGHorizontalFrame *PaletteAxisTitleOptions_HF = new TGHorizontalFrame(PaletteAxis_GF);
  PaletteAxis_GF->AddFrame(PaletteAxisTitleOptions_HF, new TGLayoutHints(kLHintsNormal, 0,5,0,5));

  PaletteAxisTitleOptions_HF->AddFrame(PaletteAxisSize_NEL = new ADAQNumberEntryWithLabel(PaletteAxisTitleOptions_HF, "Size", PaletteAxisSize_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PaletteAxisSize_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PaletteAxisSize_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PaletteAxisSize_NEL->GetEntry()->SetNumber(0.055);
  PaletteAxisSize_NEL->GetEntry()->Resize(60, 20);
  PaletteAxisSize_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");
  
  PaletteAxisTitleOptions_HF->AddFrame(PaletteAxisOffset_NEL = new ADAQNumberEntryWithLabel(PaletteAxisTitleOptions_HF, "Offset", PaletteAxisOffset_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 5,2,0,0));
  PaletteAxisOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PaletteAxisOffset_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PaletteAxisOffset_NEL->GetEntry()->SetNumber(1.0);
  PaletteAxisOffset_NEL->GetEntry()->Resize(60, 20);
  PaletteAxisOffset_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");

  TGHorizontalFrame *PaletteAxisPositions_HF1 = new TGHorizontalFrame(PaletteAxis_GF);
  PaletteAxis_GF->AddFrame(PaletteAxisPositions_HF1, new TGLayoutHints(kLHintsNormal, 0,5,0,5));

  PaletteAxisPositions_HF1->AddFrame(PaletteX1_NEL = new ADAQNumberEntryWithLabel(PaletteAxisPositions_HF1, "X1", PaletteX1_NEL_ID),
				    new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PaletteX1_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PaletteX1_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PaletteX1_NEL->GetEntry()->SetNumber(0.85);
  PaletteX1_NEL->GetEntry()->Resize(50, 20);
  PaletteX1_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");

  PaletteAxisPositions_HF1->AddFrame(PaletteX2_NEL = new ADAQNumberEntryWithLabel(PaletteAxisPositions_HF1, "X2", PaletteX2_NEL_ID),
				    new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PaletteX2_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PaletteX2_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PaletteX2_NEL->GetEntry()->SetNumber(0.89);
  PaletteX2_NEL->GetEntry()->Resize(50, 20);
  PaletteX2_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");

  TGHorizontalFrame *PaletteAxisPositions_HF2 = new TGHorizontalFrame(PaletteAxis_GF);
  PaletteAxis_GF->AddFrame(PaletteAxisPositions_HF2, new TGLayoutHints(kLHintsNormal, 0,5,0,5));

  PaletteAxisPositions_HF2->AddFrame(PaletteY1_NEL = new ADAQNumberEntryWithLabel(PaletteAxisPositions_HF2, "Y1", PaletteY1_NEL_ID),
				    new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PaletteY1_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PaletteY1_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PaletteY1_NEL->GetEntry()->SetNumber(0.16);
  PaletteY1_NEL->GetEntry()->Resize(50, 20);
  PaletteY1_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");

  PaletteAxisPositions_HF2->AddFrame(PaletteY2_NEL = new ADAQNumberEntryWithLabel(PaletteAxisPositions_HF2, "Y2", PaletteY2_NEL_ID),
				    new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PaletteY2_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PaletteY2_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PaletteY2_NEL->GetEntry()->SetNumber(0.85);
  PaletteY2_NEL->GetEntry()->Resize(50, 20);
  PaletteY2_NEL->GetEntry()->Connect("ValueSet(long)", "AAGraphicsSlots", GraphicsSlots, "HandleNumberEntries()");
}


void AAInterface::FillProcessingFrame()
{
  TGCanvas *ProcessingFrame_C = new TGCanvas(ProcessingOptions_CF, TabFrameWidth-10, TabFrameLength-20, kDoubleBorder);
  ProcessingOptions_CF->AddFrame(ProcessingFrame_C, new TGLayoutHints(kLHintsCenterX, 0,0,10,10));
  
  TGVerticalFrame *ProcessingFrame_VF = new TGVerticalFrame(ProcessingFrame_C->GetViewPort(), TabFrameWidth-10, TabFrameLength);
  ProcessingFrame_C->SetContainer(ProcessingFrame_VF);


  //////////////////////////////
  // Waveform processing options
  
  TGGroupFrame *ProcessingOptions_GF = new TGGroupFrame(ProcessingFrame_VF, "Waveform processing options", kVerticalFrame);
  ProcessingFrame_VF->AddFrame(ProcessingOptions_GF, new TGLayoutHints(kLHintsLeft, 5,5,5,5));

  TGHorizontalFrame *Architecture_HF = new TGHorizontalFrame(ProcessingOptions_GF);
  ProcessingOptions_GF->AddFrame(Architecture_HF, new TGLayoutHints(kLHintsCenterX, 5,5,5,5));

  Architecture_HF->AddFrame(ProcessingSeq_RB = new TGRadioButton(Architecture_HF, "Sequential", ProcessingSeq_RB_ID),
			    new TGLayoutHints(kLHintsLeft, 5,5,0,0));
  ProcessingSeq_RB->Connect("Clicked()", "AAProcessingSlots", ProcessingSlots, "HandleRadioButtons()");
  ProcessingSeq_RB->SetState(kButtonDown);

  Architecture_HF->AddFrame(ProcessingPar_RB = new TGRadioButton(Architecture_HF, "Parallel", ProcessingPar_RB_ID),
			    new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  ProcessingPar_RB->Connect("Clicked()", "AAProcessingSlots", ProcessingSlots, "HandleRadioButtons()");
  
  ProcessingOptions_GF->AddFrame(NumProcessors_NEL = new ADAQNumberEntryWithLabel(ProcessingOptions_GF, "Number of Processors", -1),
				 new TGLayoutHints(kLHintsNormal, 0,0,5,0));
  NumProcessors_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  NumProcessors_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  NumProcessors_NEL->GetEntry()->SetLimitValues(1,NumProcessors);
  NumProcessors_NEL->GetEntry()->SetNumber(1);
  NumProcessors_NEL->GetEntry()->SetState(false);
  
  ProcessingOptions_GF->AddFrame(UpdateFreq_NEL = new ADAQNumberEntryWithLabel(ProcessingOptions_GF, "Update freq (% done)", -1),
				 new TGLayoutHints(kLHintsNormal, 0,0,5,0));
  UpdateFreq_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  UpdateFreq_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  UpdateFreq_NEL->GetEntry()->SetLimitValues(1,100);
  UpdateFreq_NEL->GetEntry()->SetNumber(2);


  // Despliced file creation options
  
  TGGroupFrame *WaveformDesplicer_GF = new TGGroupFrame(ProcessingFrame_VF, "Despliced file creation", kVerticalFrame);
  ProcessingFrame_VF->AddFrame(WaveformDesplicer_GF, new TGLayoutHints(kLHintsLeft, 5,5,5,5));
  
  WaveformDesplicer_GF->AddFrame(DesplicedWaveformNumber_NEL = new ADAQNumberEntryWithLabel(WaveformDesplicer_GF, "Number (Waveforms)", -1),
				 new TGLayoutHints(kLHintsLeft, 0,5,5,0));
  DesplicedWaveformNumber_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  DesplicedWaveformNumber_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  DesplicedWaveformNumber_NEL->GetEntry()->SetLimitValues(0,1); // Updated when ADAQ ROOT file loaded
  DesplicedWaveformNumber_NEL->GetEntry()->SetNumber(0); // Updated when ADAQ ROOT file loaded
  
  WaveformDesplicer_GF->AddFrame(DesplicedWaveformBuffer_NEL = new ADAQNumberEntryWithLabel(WaveformDesplicer_GF, "Buffer size (sample)", -1),
				 new TGLayoutHints(kLHintsLeft, 0,5,0,0));
  DesplicedWaveformBuffer_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  DesplicedWaveformBuffer_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  DesplicedWaveformBuffer_NEL->GetEntry()->SetNumber(100);
  
  WaveformDesplicer_GF->AddFrame(DesplicedWaveformLength_NEL = new ADAQNumberEntryWithLabel(WaveformDesplicer_GF, "Despliced length (sample)", -1),
				 new TGLayoutHints(kLHintsLeft, 0,5,0,0));
  DesplicedWaveformLength_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  DesplicedWaveformLength_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  DesplicedWaveformLength_NEL->GetEntry()->SetNumber(512);
  
  TGHorizontalFrame *WaveformDesplicerName_HF = new TGHorizontalFrame(WaveformDesplicer_GF);
  WaveformDesplicer_GF->AddFrame(WaveformDesplicerName_HF, new TGLayoutHints(kLHintsLeft, 0,0,0,0));
  
  WaveformDesplicerName_HF->AddFrame(DesplicedFileSelection_TB = new TGTextButton(WaveformDesplicerName_HF, "File ... ", DesplicedFileSelection_TB_ID),
				     new TGLayoutHints(kLHintsLeft, 0,5,5,0));
  DesplicedFileSelection_TB->Resize(60,25);
  DesplicedFileSelection_TB->SetBackgroundColor(ThemeForegroundColor);
  DesplicedFileSelection_TB->ChangeOptions(DesplicedFileSelection_TB->GetOptions() | kFixedSize);
  DesplicedFileSelection_TB->Connect("Clicked()", "AAProcessingSlots", ProcessingSlots, "HandleTextButtons()");
  
  WaveformDesplicerName_HF->AddFrame(DesplicedFileName_TE = new TGTextEntry(WaveformDesplicerName_HF, "<No file currently selected>", -1),
				     new TGLayoutHints(kLHintsLeft, 5,0,5,5));
  DesplicedFileName_TE->Resize(180,25);
  DesplicedFileName_TE->SetAlignment(kTextRight);
  DesplicedFileName_TE->SetBackgroundColor(ThemeForegroundColor);
  DesplicedFileName_TE->ChangeOptions(DesplicedFileName_TE->GetOptions() | kFixedSize);
  
  WaveformDesplicer_GF->AddFrame(DesplicedFileCreation_TB = new TGTextButton(WaveformDesplicer_GF, "Create despliced file", DesplicedFileCreation_TB_ID),
				 new TGLayoutHints(kLHintsLeft, 20,0,10,5));
  DesplicedFileCreation_TB->Resize(200, 30);
  DesplicedFileCreation_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  DesplicedFileCreation_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  DesplicedFileCreation_TB->ChangeOptions(DesplicedFileCreation_TB->GetOptions() | kFixedSize);
  DesplicedFileCreation_TB->Connect("Clicked()", "AAProcessingSlots", ProcessingSlots, "HandleTextButtons()");
}


void AAInterface::FillCanvasFrame()
{
  /////////////////////////////////////
  // Canvas and associated widget frame

  TGVerticalFrame *Canvas_VF = new TGVerticalFrame(OptionsAndCanvas_HF);
  Canvas_VF->SetBackgroundColor(ThemeBackgroundColor);
  OptionsAndCanvas_HF->AddFrame(Canvas_VF, new TGLayoutHints(kLHintsNormal, 5,5,5,5));

  // ADAQ ROOT file name text entry
  Canvas_VF->AddFrame(FileName_TE = new TGTextEntry(Canvas_VF, "<No file currently selected>", -1),
		      new TGLayoutHints(kLHintsTop | kLHintsCenterX, 30,5,0,5));
  FileName_TE->Resize(CanvasFrameWidth,20);
  FileName_TE->SetState(false);
  FileName_TE->SetAlignment(kTextCenterX);
  FileName_TE->SetBackgroundColor(ThemeForegroundColor);
  FileName_TE->ChangeOptions(FileName_TE->GetOptions() | kFixedSize);

  // File statistics horizontal frame

  TGHorizontalFrame *FileStats_HF = new TGHorizontalFrame(Canvas_VF,CanvasFrameWidth,30);
  FileStats_HF->SetBackgroundColor(ThemeBackgroundColor);
  Canvas_VF->AddFrame(FileStats_HF, new TGLayoutHints(kLHintsTop | kLHintsCenterX, 5,5,0,5));

  // CAEN firmware type

  TGHorizontalFrame *FirmwareTEAndLabel_HF = new TGHorizontalFrame(FileStats_HF);
  FirmwareTEAndLabel_HF->SetBackgroundColor(ThemeBackgroundColor);
  FileStats_HF->AddFrame(FirmwareTEAndLabel_HF, new TGLayoutHints(kLHintsLeft, 5,35,0,0));
  
  FirmwareTEAndLabel_HF->AddFrame(FirmwareType_TE = new TGTextEntry(FirmwareTEAndLabel_HF, " <None>", -1),
				  new TGLayoutHints(kLHintsTop | kLHintsCenterX, 30,5,0,5));
  FirmwareType_TE->Resize(80,20);
  FirmwareType_TE->SetState(false);
  FirmwareType_TE->SetAlignment(kTextCenterX);
  FirmwareType_TE->SetBackgroundColor(ThemeForegroundColor);
  FirmwareType_TE->ChangeOptions(FirmwareType_TE->GetOptions() | kFixedSize);
  
  FirmwareTEAndLabel_HF->AddFrame(FirmwareLabel_L = new TGLabel(FirmwareTEAndLabel_HF, " Firmware type "),
				  new TGLayoutHints(kLHintsLeft,0,5,3,0));
  FirmwareLabel_L->SetBackgroundColor(ThemeForegroundColor);
  
  // Waveforms

  TGHorizontalFrame *WaveformNELAndLabel_HF = new TGHorizontalFrame(FileStats_HF);
  WaveformNELAndLabel_HF->SetBackgroundColor(ThemeBackgroundColor);
  FileStats_HF->AddFrame(WaveformNELAndLabel_HF, new TGLayoutHints(kLHintsLeft, 5,35,0,0));

  WaveformNELAndLabel_HF->AddFrame(Waveforms_NEL = new TGNumberEntryField(WaveformNELAndLabel_HF,-1),
				  new TGLayoutHints(kLHintsLeft,5,5,0,0));
  Waveforms_NEL->Resize(80,20);
  Waveforms_NEL->SetBackgroundColor(ThemeForegroundColor);
  Waveforms_NEL->SetAlignment(kTextCenterX);
  Waveforms_NEL->SetState(false);
  
  WaveformNELAndLabel_HF->AddFrame(WaveformLabel_L = new TGLabel(WaveformNELAndLabel_HF, " Waveforms "),
				   new TGLayoutHints(kLHintsLeft,0,5,3,0));
  WaveformLabel_L->SetBackgroundColor(ThemeForegroundColor);

  // Record length

  TGHorizontalFrame *RecordLengthNELAndLabel_HF = new TGHorizontalFrame(FileStats_HF);
  RecordLengthNELAndLabel_HF->SetBackgroundColor(ThemeBackgroundColor);
  FileStats_HF->AddFrame(RecordLengthNELAndLabel_HF, new TGLayoutHints(kLHintsLeft, 5,5,0,0));

  RecordLengthNELAndLabel_HF->AddFrame(RecordLength_NEL = new TGNumberEntryField(RecordLengthNELAndLabel_HF,-1),
				      new TGLayoutHints(kLHintsLeft,5,5,0,0));
  RecordLength_NEL->Resize(80,20);
  RecordLength_NEL->SetBackgroundColor(ThemeForegroundColor);
  RecordLength_NEL->SetAlignment(kTextCenterX);
  RecordLength_NEL->SetState(false);

  RecordLengthNELAndLabel_HF->AddFrame(RecordLengthLabel_L = new TGLabel(RecordLengthNELAndLabel_HF, " Record length "),
				       new TGLayoutHints(kLHintsLeft,0,5,3,0));
  RecordLengthLabel_L->SetBackgroundColor(ThemeForegroundColor);

  // The embedded canvas

  TGHorizontalFrame *Canvas_HF = new TGHorizontalFrame(Canvas_VF);
  Canvas_VF->AddFrame(Canvas_HF, new TGLayoutHints(kLHintsCenterX));

  Canvas_HF->AddFrame(YAxisLimits_DVS = new TGDoubleVSlider(Canvas_HF, CanvasY, kDoubleScaleBoth, YAxisLimits_DVS_ID),
		      new TGLayoutHints(kLHintsCenterY));
  YAxisLimits_DVS->SetRange(0,1);
  YAxisLimits_DVS->SetPosition(0,1);
  YAxisLimits_DVS->Connect("PositionChanged()", "AANontabSlots", NontabSlots, "HandleDoubleSliders()");
  
  Canvas_HF->AddFrame(Canvas_EC = new TRootEmbeddedCanvas("TheCanvas_EC", Canvas_HF, CanvasX, CanvasY),
		      new TGLayoutHints(kLHintsCenterX, 5,5,5,5));
  Canvas_EC->GetCanvas()->SetFillColor(0);
  Canvas_EC->GetCanvas()->SetFrameFillColor(0);
  Canvas_EC->GetCanvas()->SetGrid();
  Canvas_EC->GetCanvas()->SetBorderMode(0);
  Canvas_EC->GetCanvas()->SetLeftMargin(0.13);
  Canvas_EC->GetCanvas()->SetBottomMargin(0.12);
  Canvas_EC->GetCanvas()->SetRightMargin(0.05);
  Canvas_EC->GetCanvas()->Connect("ProcessedEvent(int, int, int, TObject *)", "AANontabSlots", NontabSlots, "HandleCanvas(int, int, int, TObject *)");

  Canvas_VF->AddFrame(XAxisLimits_THS = new TGTripleHSlider(Canvas_VF, CanvasX, kDoubleScaleBoth, XAxisLimits_THS_ID),
		      new TGLayoutHints(kLHintsNormal, SliderBuffer,5,5,0));
  XAxisLimits_THS->SetRange(0,1);
  XAxisLimits_THS->SetPosition(0,1);
  XAxisLimits_THS->SetPointerPosition(0.5);
  XAxisLimits_THS->Connect("PositionChanged()", "AANontabSlots", NontabSlots, "HandleDoubleSliders()");
  XAxisLimits_THS->Connect("PointerPositionChanged()", "AANontabSlots", NontabSlots, "HandleTripleSliderPointer()");
    
  Canvas_VF->AddFrame(new TGLabel(Canvas_VF, "  Waveform selector  "),
		      new TGLayoutHints(kLHintsCenterX, 5,5,10,0));

  Canvas_VF->AddFrame(WaveformSelector_HS = new TGHSlider(Canvas_VF, CanvasX, WaveformSelector_HS_ID),
		      new TGLayoutHints(kLHintsNormal, SliderBuffer,5,0,0));
  WaveformSelector_HS->SetRange(1,100);
  WaveformSelector_HS->SetPosition(1);
  WaveformSelector_HS->Connect("PositionChanged(int)", "AANontabSlots", NontabSlots, "HandleSliders(int)");

  Canvas_VF->AddFrame(new TGLabel(Canvas_VF, "  Spectrum analysis limits  "),
		      new TGLayoutHints(kLHintsCenterX, 5,5,10,0));

  Canvas_VF->AddFrame(SpectrumIntegrationLimits_DHS = new TGDoubleHSlider(Canvas_VF, CanvasX, kDoubleScaleBoth, SpectrumIntegrationLimits_DHS_ID),
		      new TGLayoutHints(kLHintsNormal, SliderBuffer,5,0,5));
  SpectrumIntegrationLimits_DHS->SetRange(0,1);
  SpectrumIntegrationLimits_DHS->SetPosition(0,1);
  SpectrumIntegrationLimits_DHS->Connect("PositionChanged()", "AANontabSlots", NontabSlots, "HandleDoubleSliders()");


  
  TGHorizontalFrame *SubCanvas_HF = new TGHorizontalFrame(Canvas_VF);
  SubCanvas_HF->SetBackgroundColor(ThemeBackgroundColor);
  Canvas_VF->AddFrame(SubCanvas_HF, new TGLayoutHints(kLHintsRight | kLHintsBottom, 5,30,20,5));

  
  SubCanvas_HF->AddFrame(ProcessingProgress_PB = new TGHProgressBar(SubCanvas_HF, TGProgressBar::kFancy, CanvasX-300),
			 new TGLayoutHints(kLHintsLeft, 5,55,7,5));
  ProcessingProgress_PB->SetBarColor(ColorMgr->Number2Pixel(29));
  ProcessingProgress_PB->ShowPosition(kTRUE, kFALSE, "%0.1f percent waveforms processed");


  SubCanvas_HF->AddFrame(Quit_TB = new TGTextButton(SubCanvas_HF, "Access standby", Quit_TB_ID),
			 new TGLayoutHints(kLHintsRight, 5,5,0,5));
  Quit_TB->Resize(200, 40);
  Quit_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  Quit_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  Quit_TB->ChangeOptions(Quit_TB->GetOptions() | kFixedSize);
  Quit_TB->Connect("Clicked()", "AANontabSlots", NontabSlots, "HandleTerminate()");
}


void AAInterface::SaveSettings(bool SaveToFile)
{
  delete ADAQSettings;
  ADAQSettings = new AASettings;
  
  TFile *ADAQSettingsFile = 0;
  if(SaveToFile)
    ADAQSettingsFile = new TFile(ADAQSettingsFileName.c_str(), "recreate");
  
  //////////////////////////////////////////
  // Values from the "Waveform" tabbed frame 

  ADAQSettings->WaveformChannel = ChannelSelector_CBL->GetComboBox()->GetSelected();
  ADAQSettings->WaveformToPlot = WaveformSelector_NEL->GetEntry()->GetIntNumber();

  ADAQSettings->RawWaveform = RawWaveform_RB->IsDown();
  ADAQSettings->BSWaveform = BaselineSubtractedWaveform_RB->IsDown();
  ADAQSettings->ZSWaveform = ZeroSuppressionWaveform_RB->IsDown();

  ADAQSettings->PlotZeroSuppressionCeiling = PlotZeroSuppressionCeiling_CB->IsDown();
  ADAQSettings->ZeroSuppressionCeiling = ZeroSuppressionCeiling_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->ZeroSuppressionBuffer = ZeroSuppressionBuffer_NEL->GetEntry()->GetIntNumber();
  
  if(PositiveWaveform_RB->IsDown())
    ADAQSettings->WaveformPolarity = 1.0;
  else
    ADAQSettings->WaveformPolarity = -1.0;
  
  ADAQSettings->FindPeaks = FindPeaks_CB->IsDown();
  ADAQSettings->UseMarkovSmoothing = UseMarkovSmoothing_CB->IsDown();
  ADAQSettings->MaxPeaks = MaxPeaks_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->Sigma = Sigma_NEL->GetEntry()->GetNumber();
  ADAQSettings->Resolution = Resolution_NEL->GetEntry()->GetNumber();
  ADAQSettings->Floor = Floor_NEL->GetEntry()->GetIntNumber();

  ADAQSettings->PlotFloor = PlotFloor_CB->IsDown();
  ADAQSettings->PlotCrossings = PlotCrossings_CB->IsDown();
  ADAQSettings->PlotPeakIntegrationRegion = PlotPeakIntegratingRegion_CB->IsDown();

  ADAQSettings->UsePileupRejection = UsePileupRejection_CB->IsDown();
  ADAQSettings->UsePSDRejection = UsePSDRejection_CB->IsDown();

  ADAQSettings->PlotAnalysisRegion = PlotAnalysisRegion_CB->IsDown();
  ADAQSettings->AnalysisRegionMin = AnalysisRegionMin_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->AnalysisRegionMax = AnalysisRegionMax_NEL->GetEntry()->GetIntNumber();
  
  ADAQSettings->PlotBaselineRegion = PlotBaselineRegion_CB->IsDown();
  ADAQSettings->BaselineRegionMin = BaselineRegionMin_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->BaselineRegionMax = BaselineRegionMax_NEL->GetEntry()->GetIntNumber();
  
  ADAQSettings->PlotTrigger = PlotTrigger_CB->IsDown();
  
  ADAQSettings->WaveformAnalysis = WaveformAnalysis_CB->IsDown();


  //////////////////////////////////////
  // Values from "Spectrum" tabbed frame

  ADAQSettings->WaveformsToHistogram = WaveformsToHistogram_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->SpectrumNumBins = SpectrumNumBins_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->SpectrumMinBin = SpectrumMinBin_NEL->GetEntry()->GetNumber();
  ADAQSettings->SpectrumMaxBin = SpectrumMaxBin_NEL->GetEntry()->GetNumber();
  ADAQSettings->SpectrumMinThresh = SpectrumMinThresh_NEL->GetEntry()->GetNumber();
  ADAQSettings->SpectrumMaxThresh = SpectrumMaxThresh_NEL->GetEntry()->GetNumber();

  ADAQSettings->ADAQSpectrumTypePAS = ADAQSpectrumTypePAS_RB->IsDown();
  ADAQSettings->ADAQSpectrumTypePHS = ADAQSpectrumTypePHS_RB->IsDown();
  ADAQSettings->ADAQSpectrumAlgorithmSMS = ADAQSpectrumAlgorithmSMS_RB->IsDown();
  ADAQSettings->ADAQSpectrumAlgorithmPF = ADAQSpectrumAlgorithmPF_RB->IsDown();
  ADAQSettings->ADAQSpectrumAlgorithmWD = ADAQSpectrumAlgorithmWD_RB->IsDown();

  ADAQSettings->ASIMSpectrumTypeEnergy = ASIMSpectrumTypeEnergy_RB->IsDown();
  ADAQSettings->ASIMSpectrumTypePhotonsCreated = ASIMSpectrumTypePhotonsCreated_RB->IsDown();
  ADAQSettings->ASIMSpectrumTypePhotonsDetected = ASIMSpectrumTypePhotonsDetected_RB->IsDown();
  ADAQSettings->ASIMEventTreeName = ASIMEventTree_CB->GetSelectedEntry()->GetTitle();

  ADAQSettings->CalibrationMin = SpectrumCalibrationMin_NEL->GetEntry()->GetNumber();
  ADAQSettings->CalibrationMax = SpectrumCalibrationMax_NEL->GetEntry()->GetNumber();
  ADAQSettings->CalibrationType = SpectrumCalibrationType_CBL->GetComboBox()->GetSelectedEntry()->GetTitle();
  ADAQSettings->CalibrationUnit = SpectrumCalibrationUnit_CBL->GetComboBox()->GetSelectedEntry()->GetTitle();;
  
  
  //////////////////////////////////////////
  // Values from the "Analysis" tabbed frame 

  ADAQSettings->FindBackground = SpectrumFindBackground_CB->IsDown();
  ADAQSettings->BackgroundIterations = SpectrumBackgroundIterations_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->BackgroundCompton = SpectrumBackgroundCompton_CB->IsDown();
  ADAQSettings->BackgroundSmoothing = SpectrumBackgroundSmoothing_CB->IsDown();
  ADAQSettings->BackgroundMinBin = SpectrumRangeMin_NEL->GetEntry()->GetNumber();
  ADAQSettings->BackgroundMaxBin = SpectrumRangeMax_NEL->GetEntry()->GetNumber();
  ADAQSettings->BackgroundDirection = SpectrumBackgroundDirection_CBL->GetComboBox()->GetSelected();
  ADAQSettings->BackgroundFilterOrder = SpectrumBackgroundFilterOrder_CBL->GetComboBox()->GetSelected();
  ADAQSettings->BackgroundSmoothingWidth = SpectrumBackgroundSmoothingWidth_CBL->GetComboBox()->GetSelected();

  ADAQSettings->PlotWithBackground = SpectrumWithBackground_RB->IsDown();
  ADAQSettings->PlotLessBackground = SpectrumLessBackground_RB->IsDown();
  
  ADAQSettings->SpectrumFindIntegral = SpectrumFindIntegral_CB->IsDown();
  ADAQSettings->SpectrumIntegralInCounts = SpectrumIntegralInCounts_CB->IsDown();
  ADAQSettings->SpectrumUseGaussianFit = SpectrumUseGaussianFit_CB->IsDown();
  ADAQSettings->SpectrumUseVerboseFit = SpectrumUseVerboseFit_CB->IsDown();


  /////////////////////////////////////
  // Values from the "PSD" tabbed frame
  
  ADAQSettings->PSDWaveformsToDiscriminate = PSDWaveforms_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PSDThreshold = PSDThreshold_NEL->GetEntry()->GetNumber();

  ADAQSettings->PSDNumTotalBins = PSDNumTotalBins_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PSDMinTotalBin = PSDMinTotalBin_NEL->GetEntry()->GetNumber();
  ADAQSettings->PSDMaxTotalBin = PSDMaxTotalBin_NEL->GetEntry()->GetNumber();

  ADAQSettings->PSDXAxisADC = PSDXAxisADC_RB->IsDown();
  ADAQSettings->PSDXAxisEnergy = PSDXAxisEnergy_RB->IsDown();

  ADAQSettings->PSDNumTailBins = PSDNumTailBins_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PSDMinTailBin = PSDMinTailBin_NEL->GetEntry()->GetNumber();
  ADAQSettings->PSDMaxTailBin = PSDMaxTailBin_NEL->GetEntry()->GetNumber();

  ADAQSettings->PSDYAxisTail = PSDYAxisTail_RB->IsDown();
  ADAQSettings->PSDYAxisTailTotal = PSDYAxisTailTotal_RB->IsDown();

  ADAQSettings->PSDTotalStart = PSDTotalStart_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PSDTotalStop = PSDTotalStop_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PSDTailStart = PSDTailStart_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PSDTailStop = PSDTailStop_NEL->GetEntry()->GetIntNumber();
  
  ADAQSettings->PSDAlgorithmPF = PSDAlgorithmPF_RB->IsDown();
  ADAQSettings->PSDAlgorithmSMS = PSDAlgorithmSMS_RB->IsDown();
  ADAQSettings->PSDAlgorithmWD = PSDAlgorithmWD_RB->IsDown();

  ADAQSettings->PSDPlotType = PSDPlotType_CBL->GetComboBox()->GetSelectedEntry()->GetTitle();
  ADAQSettings->PSDPlotIntegrationLimits = PSDPlotIntegrationLimits_CB->IsDown();
  
  ADAQSettings->PSDInsideRegion = PSDInsideRegion_RB->IsDown();
  ADAQSettings->PSDOutsideRegion = PSDOutsideRegion_RB->IsDown();

  ADAQSettings->PSDXSlice = PSDHistogramSliceX_RB->IsDown();
  ADAQSettings->PSDYSlice = PSDHistogramSliceY_RB->IsDown();

  ADAQSettings->PSDCalculateFOM = PSDCalculateFOM_CB->IsDown();
  ADAQSettings->PSDLowerFOMFitMin = PSDLowerFOMFitMin_NEL->GetEntry()->GetNumber();
  ADAQSettings->PSDLowerFOMFitMax = PSDLowerFOMFitMax_NEL->GetEntry()->GetNumber();
  ADAQSettings->PSDUpperFOMFitMin = PSDUpperFOMFitMin_NEL->GetEntry()->GetNumber();
  ADAQSettings->PSDUpperFOMFitMax = PSDUpperFOMFitMax_NEL->GetEntry()->GetNumber();

  //////////////////////////////////////////
  // Values from the "Graphics" tabbed frame

  ADAQSettings->WaveformLine = DrawWaveformWithLine_RB->IsDown();
  ADAQSettings->WaveformCurve = DrawWaveformWithCurve_RB->IsDown();
  ADAQSettings->WaveformMarkers = DrawWaveformWithMarkers_RB->IsDown();
  ADAQSettings->WaveformBoth = DrawWaveformWithBoth_RB->IsDown();
  ADAQSettings->WaveformLineWidth = WaveformLineWidth_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->WaveformMarkerSize = WaveformMarkerSize_NEL->GetEntry()->GetNumber();
  
  ADAQSettings->SpectrumLine = DrawSpectrumWithLine_RB->IsDown();
  ADAQSettings->SpectrumCurve = DrawSpectrumWithCurve_RB->IsDown();
  ADAQSettings->SpectrumError = DrawSpectrumWithError_RB->IsDown();
  ADAQSettings->SpectrumBars = DrawSpectrumWithBars_RB->IsDown();
  ADAQSettings->SpectrumLineWidth = SpectrumLineWidth_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->SpectrumFillStyle = SpectrumFillStyle_NEL->GetEntry()->GetIntNumber();

  ADAQSettings->HistogramStats = HistogramStats_CB->IsDown();
  ADAQSettings->CanvasGrid = CanvasGrid_CB->IsDown();
  ADAQSettings->CanvasXAxisLog = CanvasXAxisLog_CB->IsDown();
  ADAQSettings->CanvasYAxisLog = CanvasYAxisLog_CB->IsDown();
  ADAQSettings->CanvasZAxisLog = CanvasZAxisLog_CB->IsDown();

  ADAQSettings->PlotSpectrumDerivativeError = PlotSpectrumDerivativeError_CB->IsDown();
  ADAQSettings->PlotAbsValueSpectrumDerivative = PlotAbsValueSpectrumDerivative_CB->IsDown();
  ADAQSettings->PlotYAxisWithAutoRange = AutoYAxisRange_CB->IsDown();
  
  ADAQSettings->OverrideGraphicalDefault = OverrideTitles_CB->IsDown();

  ADAQSettings->PlotTitle = Title_TEL->GetEntry()->GetText();
  ADAQSettings->XAxisTitle = XAxisTitle_TEL->GetEntry()->GetText();
  ADAQSettings->YAxisTitle = YAxisTitle_TEL->GetEntry()->GetText();
  ADAQSettings->ZAxisTitle = ZAxisTitle_TEL->GetEntry()->GetText();
  ADAQSettings->PaletteTitle = PaletteAxisTitle_TEL->GetEntry()->GetText();

  ADAQSettings->XSize = XAxisSize_NEL->GetEntry()->GetNumber();
  ADAQSettings->YSize = YAxisSize_NEL->GetEntry()->GetNumber();
  ADAQSettings->ZSize = ZAxisSize_NEL->GetEntry()->GetNumber();
  ADAQSettings->PaletteSize = PaletteAxisSize_NEL->GetEntry()->GetNumber();

  ADAQSettings->XOffset = XAxisOffset_NEL->GetEntry()->GetNumber();
  ADAQSettings->YOffset = YAxisOffset_NEL->GetEntry()->GetNumber();
  ADAQSettings->ZOffset = ZAxisOffset_NEL->GetEntry()->GetNumber();
  ADAQSettings->PaletteOffset = PaletteAxisOffset_NEL->GetEntry()->GetNumber();

  ADAQSettings->XDivs = XAxisDivs_NEL->GetEntry()->GetNumber();
  ADAQSettings->YDivs = YAxisDivs_NEL->GetEntry()->GetNumber();
  ADAQSettings->ZDivs = ZAxisDivs_NEL->GetEntry()->GetNumber();

  ADAQSettings->PaletteX1 = PaletteX1_NEL->GetEntry()->GetNumber();
  ADAQSettings->PaletteX2 = PaletteX2_NEL->GetEntry()->GetNumber();
  ADAQSettings->PaletteY1 = PaletteY1_NEL->GetEntry()->GetNumber();
  ADAQSettings->PaletteY2 = PaletteY2_NEL->GetEntry()->GetNumber();


  ////////////////////////////////////////
  // Values from "Processing" tabbed frame

  ADAQSettings->SeqProcessing = ProcessingSeq_RB->IsDown();
  ADAQSettings->ParProcessing = ProcessingSeq_RB->IsDown();

  ADAQSettings->NumProcessors = NumProcessors_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->UpdateFreq = UpdateFreq_NEL->GetEntry()->GetIntNumber();

  ADAQSettings->WaveformsToDesplice = DesplicedWaveformNumber_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->DesplicedWaveformBuffer = DesplicedWaveformBuffer_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->DesplicedWaveformLength = DesplicedWaveformLength_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->DesplicedFileName = DesplicedFileName_TE->GetText();

  
  /////////////////////////////////
  // Values from the "Canvas" frame

  float Min,Max;

  XAxisLimits_THS->GetPosition(Min, Max);
  ADAQSettings->XAxisMin = Min;
  ADAQSettings->XAxisMax = Max;

  YAxisLimits_DVS->GetPosition(Min, Max);
  ADAQSettings->YAxisMin = Min;
  ADAQSettings->YAxisMax = Max;

  SpectrumIntegrationLimits_DHS->GetPosition(Min, Max);
  ADAQSettings->SpectrumIntegrationMin = Min;
  ADAQSettings->SpectrumIntegrationMax = Max;


  ////////////////////////////////////////////////////////
  // Miscellaneous values required for parallel processing
  
  ADAQSettings->ADAQFileName = ADAQFileName;
  
  // Spectrum calibration objects
  ADAQSettings->UseSpectraCalibrations = ComputationMgr->GetUseSpectraCalibrations();
  ADAQSettings->SpectraCalibrationData = ComputationMgr->GetSpectraCalibrationData();
  ADAQSettings->SpectraCalibrations = ComputationMgr->GetSpectraCalibrations();
  
  // PSD filter objects
  ADAQSettings->UsePSDRegions = ComputationMgr->GetUsePSDRegions();
  ADAQSettings->PSDRegions = ComputationMgr->GetPSDRegions();

  // Update the settings object pointer in the manager classes
  ComputationMgr->SetADAQSettings(ADAQSettings);
  GraphicsMgr->SetADAQSettings(ADAQSettings);

  // Write the ADAQSettings object to a ROOT file for parallel access
  if(SaveToFile){
    ADAQSettings->Write("ADAQSettings");
    ADAQSettingsFile->Write();
    ADAQSettingsFile->Close();
  }
  
  delete ADAQSettingsFile;
}


// Creates a separate pop-up box with a message for the user. Function
// is modular to allow flexibility in use.
void AAInterface::CreateMessageBox(string Message, string IconName)
{
  // Choose either a "stop" icon or an "asterisk" icon
  EMsgBoxIcon IconType = kMBIconStop;
  if(IconName == "Asterisk")
    IconType = kMBIconAsterisk;
  
  const int NumTitles = 8;
  
  string BoxTitlesAsterisk[NumTitles] = {"ADAQAnalysis says 'good job!", 
					 "Oh, so you are competent!",
					 "This is a triumph of science!",
					 "Excellent work. You're practically a PhD now.",
					 "For you ARE the Kwisatz Haderach!",
					 "There will be a parade in your honor.",
					 "Oh, well, bra-VO!",
					 "Top notch."};
  
  string BoxTitlesStop[NumTitles] = {"ADAQAnalysis is disappointed in you...", 
				     "Seriously? I'd like another operator, please.",
				     "Unacceptable. Just totally unacceptable.",
				     "That was about as successful as the Hindenburg...",
				     "You blew it!",
				     "Abominable! Off with your head!",
				     "Do, or do not. There is no try.",
				     "You fucked it up, Walter! You always fuck it up!"};
  
  // Surprise the user!
  int RndmInt = RndmMgr->Integer(NumTitles);
  
  string Title = BoxTitlesStop[RndmInt];
  if(IconType==kMBIconAsterisk)
    Title = BoxTitlesAsterisk[RndmInt];

  
  // Create the message box with the desired message and icon
  new TGMsgBox(gClient->GetRoot(), this, Title.c_str(), Message.c_str(), IconType, kMBOk);

}


// Method to update all the relevant widget settings for operating on
// an ADAQ-formatted ROOT file
void AAInterface::UpdateForADAQFile()
{
  // Get the current channel to be analyzed
  int Channel = ChannelSelector_CBL->GetComboBox()->GetSelected();

  // Update widgets with settings from the ADAQ file

  // Get the total number of waveforms stored in the ADAQWaveformTree
  int WaveformsInFile = ComputationMgr->GetADAQNumberOfWaveforms();

  // Get the following acquisition settings that are stored in the
  // ADAQ file in order to correctly update the GUI widgets
  Int_t RecordLength, Trigger, BaselineRegionMin, BaselineRegionMax;

  // In order to ensure backwards compatibility with preexisting data
  // sets taken before Feb 2015, we must differentiate between the
  // "legacy" and "production" ADAQ file formats.

  TString FWType;

  // "Legacy" ADAQ file (old and busted)
  if(ComputationMgr->GetADAQLegacyFileLoaded()){
    ADAQRootMeasParams *AMP = ComputationMgr->GetADAQMeasurementParameters();

    RecordLength = AMP->RecordLength;
    Trigger = AMP->TriggerThreshold[Channel];
    BaselineRegionMin = AMP->BaselineCalcMin[Channel];
    BaselineRegionMax = AMP->BaselineCalcMax[Channel];

    FWType = "Standard";
  }

  // "Production" ADAQ file (new hotness)
  else{
    ADAQReadoutInformation *ARI = ComputationMgr->GetADAQReadoutInformation();

    // Standard firmware has a global record length setings
    if(ARI->GetDGFWType() == "Standard"){
      RecordLength = ARI->GetRecordLength();
      FWType = "Standard";
    }

    // DPP-PSD firmware has channel specific record length settings
    else if(ARI->GetDGFWType() == "DPP-PSD"){
      RecordLength = ARI->GetChRecordLength().at(Channel);
      FWType = ARI->GetDGFWType();
    }
    
    // Account for ADAQ v1.0 files produced before DPP-PSD update
    else{
      RecordLength = ARI->GetRecordLength();
      FWType = "Standard";
    }
    
    Trigger = ARI->GetTrigger().at(Channel);
    BaselineRegionMin = ARI->GetBaselineCalcMin().at(Channel);
    BaselineRegionMax = ARI->GetBaselineCalcMax().at(Channel);
  }
  
  WaveformSelector_HS->SetRange(1, WaveformsInFile);

  WaveformsToHistogram_NEL->GetEntry()->SetLimitValues(1, WaveformsInFile);
  WaveformsToHistogram_NEL->GetEntry()->SetNumber(WaveformsInFile);

  FileName_TE->SetText(ADAQFileName.c_str());
  FirmwareType_TE->SetText(FWType);
  Waveforms_NEL->SetNumber(WaveformsInFile);
  RecordLength_NEL->SetNumber(RecordLength);
  
  AnalysisRegionMin_NEL->GetEntry()->SetLimitValues(0, RecordLength-2);
  AnalysisRegionMax_NEL->GetEntry()->SetLimitValues(1, RecordLength-1);
  
  BaselineRegionMin_NEL->GetEntry()->SetLimitValues(0,RecordLength-2);
  BaselineRegionMax_NEL->GetEntry()->SetLimitValues(1,RecordLength-1);

  // Set the baseline calculation region to the values used during the
  // data acquisition as the default; user may update afterwards

  // Update the waveform trigger level display
  TriggerLevel_NEFL->GetEntry()->SetNumber(Trigger);

  AnalysisRegionMin_NEL->GetEntry()->SetNumber(0);
  AnalysisRegionMax_NEL->GetEntry()->SetNumber(RecordLength-1);

  BaselineRegionMin_NEL->GetEntry()->SetNumber(BaselineRegionMin);
  BaselineRegionMax_NEL->GetEntry()->SetNumber(BaselineRegionMax);

  DesplicedWaveformNumber_NEL->GetEntry()->SetLimitValues(1, WaveformsInFile);
  DesplicedWaveformNumber_NEL->GetEntry()->SetNumber(WaveformsInFile);
  
  PSDWaveforms_NEL->GetEntry()->SetLimitValues(1, WaveformsInFile);
  PSDWaveforms_NEL->GetEntry()->SetNumber(WaveformsInFile);

  // Resetting radio buttons must account for enabling/disabling
  
  if(ADAQSpectrumTypePAS_RB->IsDown()){
    ADAQSpectrumTypePAS_RB->SetEnabled(true);
    ADAQSpectrumTypePAS_RB->SetState(kButtonDown);
  }
  else
    ADAQSpectrumTypePAS_RB->SetEnabled(true);
  
  if(ADAQSpectrumTypePHS_RB->IsDown()){
    ADAQSpectrumTypePHS_RB->SetEnabled(true);
    ADAQSpectrumTypePHS_RB->SetState(kButtonDown);
  }
  else
    ADAQSpectrumTypePHS_RB->SetEnabled(true);
  
  if(ADAQSpectrumAlgorithmSMS_RB->IsDown()){
    ADAQSpectrumAlgorithmSMS_RB->SetEnabled(true);
    ADAQSpectrumAlgorithmSMS_RB->SetState(kButtonDown);
  }
  else
    ADAQSpectrumAlgorithmSMS_RB->SetEnabled(true);
  
  if(ADAQSpectrumAlgorithmPF_RB->IsDown()){
    ADAQSpectrumAlgorithmPF_RB->SetEnabled(true);
    ADAQSpectrumAlgorithmPF_RB->SetState(kButtonDown);
  }
  else
    ADAQSpectrumAlgorithmPF_RB->SetEnabled(true);

  if(ADAQSpectrumAlgorithmWD_RB->IsDown()){
    ADAQSpectrumAlgorithmWD_RB->SetEnabled(true);
    ADAQSpectrumAlgorithmWD_RB->SetState(kButtonDown);
  }
  else
    ADAQSpectrumAlgorithmWD_RB->SetEnabled(true);

  
  // Disable all ASIM-specific analysis widgets

  ASIMSpectrumTypeEnergy_RB->SetEnabled(false);
  ASIMSpectrumTypePhotonsDetected_RB->SetEnabled(false);
  ASIMSpectrumTypePhotonsCreated_RB->SetEnabled(false);
  
  // A new file means that there are no procssed waveforms yet so
  // disable the ability to create spectra from them and set the
  // process spectrum button color to blue
  CreateSpectrum_TB->SetState(kButtonDisabled);
  ProcessSpectrum_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));

  // Reset the PSD widgets to prevent accessing previous PSD histogram
  // CHECK THIS FOR PSDOVERHAUL : ZSH (10 APR 16)
  if(true){
    SetPSDWidgetState(false, kButtonDisabled);
    ProcessPSDHistogram_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
    CreatePSDHistogram_TB->SetState(kButtonDisabled);
  }

  // Reenable all ADAQ-specific tab frames

  WaveformOptionsTab_CF->ShowFrame(WaveformOptions_CF);
}


// Method to update all the relevant widget settings for operating on
// an ASIM-formatted ROOT file
void AAInterface::UpdateForASIMFile()
{
  // Update header widgets appropriately
  FileName_TE->SetText(ASIMFileName.c_str());
  Waveforms_NEL->SetNumber(0);
  WaveformsToHistogram_NEL->GetEntry()->SetNumber(0);
  RecordLength_NEL->SetNumber(0);
 
  // Disable tabs that are not required for ASIM files
  WaveformOptionsTab_CF->HideFrame(WaveformOptions_CF);
  ProcessingOptionsTab_CF->HideFrame(ProcessingOptions_CF);

  // Disable all ADAQ-specific analysis widgets
  ADAQSpectrumTypePAS_RB->SetState(kButtonDisabled);
  ADAQSpectrumTypePHS_RB->SetState(kButtonDisabled);
  ADAQSpectrumAlgorithmSMS_RB->SetState(kButtonDisabled);
  ADAQSpectrumAlgorithmPF_RB->SetState(kButtonDisabled);
  ADAQSpectrumAlgorithmWD_RB->SetState(kButtonDisabled);

  if(ASIMSpectrumTypeEnergy_RB->IsDown()){
    ASIMSpectrumTypeEnergy_RB->SetEnabled(true);
    ASIMSpectrumTypeEnergy_RB->SetState(kButtonDown);
  }
  else
    ASIMSpectrumTypeEnergy_RB->SetEnabled(true);
    
  if(ASIMSpectrumTypePhotonsDetected_RB->IsDown()){
    ASIMSpectrumTypePhotonsDetected_RB->SetEnabled(true);
    ASIMSpectrumTypePhotonsDetected_RB->SetState(kButtonDown);
  }
  else
    ASIMSpectrumTypePhotonsDetected_RB->SetEnabled(true);
   
  if(ASIMSpectrumTypePhotonsCreated_RB->IsDown()){
    ASIMSpectrumTypePhotonsCreated_RB->SetEnabled(true);
    ASIMSpectrumTypePhotonsCreated_RB->SetState(kButtonDown);
  }
  else
    ASIMSpectrumTypePhotonsCreated_RB->SetEnabled(true);

  TList *ASIMEventTreeList = ComputationMgr->GetASIMEventTreeList();

  ASIMEventTree_CB->SetEnabled(true);
  ASIMEventTree_CB->RemoveAll();

  TIter It(ASIMEventTreeList);
  TTree *EventTree = NULL;
  int EventTreeID = 0;
  int EventTreeEntries = 0;
  while((EventTree = (TTree *)It.Next())){
    // Add the event tree name to the combo box with integer ID
    ASIMEventTree_CB->AddEntry(EventTree->GetName(), EventTreeID);

    // Get the total entries in the first TTree
    if(EventTreeID == 0)
      EventTreeEntries = EventTree->GetEntries();

    // Increment the integer ID
    EventTreeID++;
  }

  ASIMEventTree_CB->Select(0,false);

  Waveforms_NEL->SetNumber(EventTreeEntries);
  WaveformsToHistogram_NEL->GetEntry()->SetNumber(EventTreeEntries);
  WaveformsToHistogram_NEL->GetEntry()->SetLimitValues(0, EventTreeEntries);

  // There are no waveforms in ASIM files (as of yet!) so disabled the
  // ability to process the waveforms
  ProcessSpectrum_TB->SetState(kButtonDisabled);
  ProcessSpectrum_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  CreateSpectrum_TB->SetState(kButtonUp);
  
  // Processing frame
  NumProcessors_NEL->GetEntry()->SetState(false);      
  UpdateFreq_NEL->GetEntry()->SetState(false);
  
  OptionsTabs_T->SetTab("Spectrum");
}


void AAInterface::UpdateForSpectrumCreation()
{
  // Refresh the background, integration, and gaussian for the new
  // spectrum if those options are already selected

  if(SpectrumFindBackground_CB->IsDown()){
    SpectrumFindBackground_CB->SetState(kButtonUp, true);
    SpectrumFindBackground_CB->SetState(kButtonDown, true);
  }

  if(SpectrumFindIntegral_CB->IsDown()){
    SpectrumFindIntegral_CB->SetState(kButtonUp, true);
    SpectrumFindIntegral_CB->SetState(kButtonDown, true);
  }
  
  if(SpectrumUseGaussianFit_CB->IsDown()){
    SpectrumUseGaussianFit_CB->SetState(kButtonUp, true);
    SpectrumUseGaussianFit_CB->SetState(kButtonDown, true);
  }

  // Update the spectrum binning limits

  int SpectrumMinBin = SpectrumMinBin_NEL->GetEntry()->GetNumber();
  SpectrumRangeMin_NEL->GetEntry()->SetNumber(SpectrumMinBin);
    
  int SpectrumMaxBin = SpectrumMaxBin_NEL->GetEntry()->GetNumber();
  SpectrumRangeMax_NEL->GetEntry()->SetNumber(SpectrumMaxBin);
  
  // Update the spectrum analysis limits

  SpectrumAnalysisLowerLimit_NEL->GetEntry()->SetLimitValues(SpectrumMinBin, SpectrumMaxBin);
  SpectrumAnalysisUpperLimit_NEL->GetEntry()->SetLimitValues(SpectrumMinBin, SpectrumMaxBin);
    
  // Activate the CreateSpectrum_TB button since processed waveform
  // values have been created and stored in vectors in AAComputation
  CreateSpectrum_TB->SetState(kButtonUp);
  
  // Alert the user that waveforms have been processed by updating
  // the ProcessSpectrum_TB button color
  ProcessSpectrum_TB->SetBackgroundColor(ColorMgr->Number2Pixel(32));
}


void AAInterface::UpdateForPSDHistogramCreation()
{
  CreatePSDHistogram_TB->SetState(kButtonUp);
  ProcessPSDHistogram_TB->SetBackgroundColor(ColorMgr->Number2Pixel(32));

  PSDEnableRegionCreation_CB->SetState(kButtonUp);

  PSDEnableHistogramSlicing_CB->SetState(kButtonUp);
}


void AAInterface::UpdateForPSDHistogramSlicingFinished()
{
  PSDEnableHistogramSlicing_CB->SetState(kButtonUp);
  PSDHistogramSliceX_RB->SetState(kButtonDisabled);
  PSDHistogramSliceY_RB->SetState(kButtonDisabled);
  PSDCalculateFOM_CB->SetState(kButtonDisabled);
  PSDLowerFOMFitMin_NEL->GetEntry()->SetState(false);
  PSDLowerFOMFitMax_NEL->GetEntry()->SetState(false);
  PSDUpperFOMFitMin_NEL->GetEntry()->SetState(false);
  PSDUpperFOMFitMax_NEL->GetEntry()->SetState(false);
  PSDFigureOfMerit_NEFL->GetEntry()->SetState(false);
}


void AAInterface::SetPeakFindingWidgetState(bool WidgetState, EButtonState ButtonState)
{
  UseMarkovSmoothing_CB->SetState(ButtonState);
  MaxPeaks_NEL->GetEntry()->SetState(WidgetState);
  Sigma_NEL->GetEntry()->SetState(WidgetState);
  Resolution_NEL->GetEntry()->SetState(WidgetState);
  Floor_NEL->GetEntry()->SetState(WidgetState);
  PlotFloor_CB->SetState(ButtonState);
  PlotCrossings_CB->SetState(ButtonState);
  PlotPeakIntegratingRegion_CB->SetState(ButtonState);
}


void AAInterface::SetPSDWidgetState(bool WidgetState, EButtonState ButtonState)
{
  /*
  PSDEnableHistogramSlicing_CB->SetState(ButtonState);
  PSDEnableRegionCreation_CB->SetState(ButtonState);

  PSDCreateRegion_TB->SetState(ButtonState);
  PSDClearRegion_TB->SetState(ButtonState);
  */
}


void AAInterface::SetCalibrationWidgetState(bool WidgetState, EButtonState ButtonState)
{
  SpectrumCalibrationLoad_TB->SetState(ButtonState);
  SpectrumCalibrationManualSlider_RB->SetState(ButtonState);
  SpectrumCalibrationPeakFinder_RB->SetState(ButtonState);
  SpectrumCalibrationEdgeFinder_RB->SetState(ButtonState);
  SpectrumCalibrationType_CBL->GetComboBox()->SetEnabled(WidgetState);
  SpectrumCalibrationMin_NEL->GetEntry()->SetState(WidgetState);
  SpectrumCalibrationMax_NEL->GetEntry()->SetState(WidgetState);
  SpectrumCalibrationPoint_CBL->GetComboBox()->SetEnabled(WidgetState);
  SpectrumCalibrationUnit_CBL->GetComboBox()->SetEnabled(WidgetState);
  SpectrumCalibrationEnergy_NEL->GetEntry()->SetState(WidgetState);
  SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetState(WidgetState);
  SpectrumCalibrationSetPoint_TB->SetState(ButtonState);
  SpectrumCalibrationCalibrate_TB->SetState(ButtonState);
  SpectrumCalibrationPlot_TB->SetState(ButtonState);
  SpectrumCalibrationReset_TB->SetState(ButtonState);

  Double_t Min = SpectrumMinBin_NEL->GetEntry()->GetNumber();
  SpectrumCalibrationMin_NEL->GetEntry()->SetNumber(Min);

  Double_t Max = SpectrumMaxBin_NEL->GetEntry()->GetNumber();
  SpectrumCalibrationMax_NEL->GetEntry()->SetNumber(Max);
}


void AAInterface::SetEAGammaWidgetState(bool WidgetState, EButtonState ButtonState)
{
  EAGammaEDep_NEL->GetEntry()->SetState(WidgetState);
  EAEscapePeaks_CB->SetState(ButtonState);
}


void AAInterface::SetEANeutronWidgetState(bool WidgetState, EButtonState ButtonState)
{
  EALightConversionFactor_NEL->GetEntry()->SetState(WidgetState);
  EAErrorWidth_NEL->GetEntry()->SetState(WidgetState);
  EAElectronEnergy_NEL->GetEntry()->SetState(WidgetState);
  EAGammaEnergy_NEL->GetEntry()->SetState(WidgetState);
  EAProtonEnergy_NEL->GetEntry()->SetState(WidgetState);
  EAAlphaEnergy_NEL->GetEntry()->SetState(WidgetState);
  EACarbonEnergy_NEL->GetEntry()->SetState(WidgetState);
}


void AAInterface::SetSpectrumBackgroundWidgetState(bool WidgetState, EButtonState ButtonState)
{
  SpectrumBackgroundIterations_NEL->GetEntry()->SetState(WidgetState);
  SpectrumBackgroundCompton_CB->SetState(ButtonState);
  SpectrumBackgroundSmoothing_CB->SetState(ButtonState);
  SpectrumRangeMin_NEL->GetEntry()->SetState(WidgetState);
  SpectrumRangeMax_NEL->GetEntry()->SetState(WidgetState);
  SpectrumBackgroundDirection_CBL->GetComboBox()->SetEnabled(WidgetState);
  SpectrumBackgroundFilterOrder_CBL->GetComboBox()->SetEnabled(WidgetState);
  SpectrumBackgroundSmoothingWidth_CBL->GetComboBox()->SetEnabled(WidgetState);
  SpectrumWithBackground_RB->SetState(ButtonState);
  SpectrumLessBackground_RB->SetState(ButtonState);
}
