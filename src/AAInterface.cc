//////////////////////////////////////////////////////////////////////////////////////////
//
// name: AAInterface.cc
// date: 21 May 13
// auth: Zach Hartwig
//
//////////////////////////////////////////////////////////////////////////////////////////

// ROOT
#include <TApplication.h>
#include <TGClient.h>
#include <TGFileDialog.h>
#include <TGTab.h>
#include <TGButtonGroup.h>
#include <TGMsgBox.h>
#include <TFile.h>

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
#include "ADAQAnalysisConstants.hh"
#include "ADAQAnalysisVersion.hh"


AAInterface::AAInterface(string CmdLineArg)
  : TGMainFrame(gClient->GetRoot()),
    NumDataChannels(8), NumProcessors(boost::thread::hardware_concurrency()),
    DataDirectory(getenv("PWD")), PrintDirectory(getenv("HOME")),
    DesplicedDirectory(getenv("HOME")), HistogramDirectory(getenv("HOME")),
    ADAQFileLoaded(false), ACRONYMFileLoaded(false)
{
  SetCleanup(kDeepCleanup);

  CreateTheMainFrames();

  ColorMgr = new TColor;
  
  ComputationMgr = AAComputation::GetInstance();
  ComputationMgr->SetProgressBarPointer(ProcessingProgress_PB);
  
  GraphicsMgr = AAGraphics::GetInstance();
  GraphicsMgr->SetCanvasPointer(Canvas_EC->GetCanvas());

  if(CmdLineArg != "Unspecified"){
    ADAQFileName = CmdLineArg;
    
    ADAQFileLoaded = ComputationMgr->LoadADAQRootFile(ADAQFileName);
    
    if(ADAQFileLoaded)
      UpdateForNewFile(ADAQFileName);
    else
      CreateMessageBox("The ADAQ ROOT file that you specified fail to load for some reason!\n","Stop");
  }
  
  string USER = getenv("USER");
  ADAQSettingsFileName = "/tmp/ADAQSettings_" + USER + ".root";
}


AAInterface::~AAInterface()
{;}


void AAInterface::CreateTheMainFrames()
{
  /////////////////////////
  // Create the menu bar //
  /////////////////////////

  TGHorizontalFrame *MenuFrame = new TGHorizontalFrame(this); 

  TGPopupMenu *MenuFile = new TGPopupMenu(gClient->GetRoot());
  MenuFile->AddEntry("&Open ADAQ ROOT file ...", MenuFileOpen_ID);
  MenuFile->AddEntry("&Save spectrum to file ...", MenuFileSaveSpectrum_ID);
  MenuFile->AddEntry("Save spectrum &derivative to file ...", MenuFileSaveSpectrumDerivative_ID);
  MenuFile->AddEntry("Save PSD &histogram slice to file ...", MenuFileSavePSDHistogramSlice_ID);
  MenuFile->AddEntry("&Print canvas ...", MenuFilePrint_ID);
  MenuFile->AddSeparator();
  MenuFile->AddEntry("E&xit", MenuFileExit_ID);
  MenuFile->Connect("Activated(int)", "AAInterface", this, "HandleMenu(int)");

  TGMenuBar *MenuBar = new TGMenuBar(MenuFrame, 100, 20, kHorizontalFrame);
  MenuBar->AddPopup("&File", MenuFile, new TGLayoutHints(kLHintsTop | kLHintsLeft, 0,0,0,0));
  MenuFrame->AddFrame(MenuBar, new TGLayoutHints(kLHintsTop | kLHintsLeft, 0,0,0,0));
  AddFrame(MenuFrame, new TGLayoutHints(kLHintsTop | kLHintsExpandX));


  //////////////////////////////////////////////////////////
  // Create the main frame to hold the options and canvas //
  //////////////////////////////////////////////////////////

  AddFrame(OptionsAndCanvas_HF = new TGHorizontalFrame(this),
	   new TGLayoutHints(kLHintsTop, 5, 5,5,5));
  OptionsAndCanvas_HF->SetBackgroundColor(ColorMgr->Number2Pixel(16));

  
  //////////////////////////////////////////////////////////////
  // Create the left-side vertical options frame with tab holder 

  TGVerticalFrame *Options_VF = new TGVerticalFrame(OptionsAndCanvas_HF);
  Options_VF->SetBackgroundColor(ColorMgr->Number2Pixel(16));
  OptionsAndCanvas_HF->AddFrame(Options_VF);
  
  TGTab *OptionsTabs_T = new TGTab(Options_VF);
  OptionsTabs_T->SetBackgroundColor(ColorMgr->Number2Pixel(16));
  Options_VF->AddFrame(OptionsTabs_T, new TGLayoutHints(kLHintsLeft, 15,15,5,5));


  //////////////////////////////////////
  // Create the individual tabs + frames

  // The tabbed frame to hold waveform widgets
  TGCompositeFrame *WaveformOptionsTab_CF = OptionsTabs_T->AddTab("Waveform");
  WaveformOptionsTab_CF->AddFrame(WaveformOptions_CF = new TGCompositeFrame(WaveformOptionsTab_CF, 1, 1, kVerticalFrame));
  WaveformOptions_CF->Resize(320,735);
  WaveformOptions_CF->ChangeOptions(WaveformOptions_CF->GetOptions() | kFixedSize);

  // The tabbed frame to hold spectrum widgets
  TGCompositeFrame *SpectrumOptionsTab_CF = OptionsTabs_T->AddTab("Spectrum");
  SpectrumOptionsTab_CF->AddFrame(SpectrumOptions_CF = new TGCompositeFrame(SpectrumOptionsTab_CF, 1, 1, kVerticalFrame));
  SpectrumOptions_CF->Resize(320,735);
  SpectrumOptions_CF->ChangeOptions(SpectrumOptions_CF->GetOptions() | kFixedSize);

  // The tabbed frame to hold analysis widgets
  TGCompositeFrame *AnalysisOptionsTab_CF = OptionsTabs_T->AddTab("Analysis");
  AnalysisOptionsTab_CF->AddFrame(AnalysisOptions_CF = new TGCompositeFrame(AnalysisOptionsTab_CF, 1, 1, kVerticalFrame));
  AnalysisOptions_CF->Resize(320,735);
  AnalysisOptions_CF->ChangeOptions(AnalysisOptions_CF->GetOptions() | kFixedSize);

  // The tabbed frame to hold graphical widgets
  TGCompositeFrame *GraphicsOptionsTab_CF = OptionsTabs_T->AddTab("Graphics");
  GraphicsOptionsTab_CF->AddFrame(GraphicsOptions_CF = new TGCompositeFrame(GraphicsOptionsTab_CF, 1, 1, kVerticalFrame));
  GraphicsOptions_CF->Resize(320,735);
  GraphicsOptions_CF->ChangeOptions(GraphicsOptions_CF->GetOptions() | kFixedSize);

  // The tabbed frame to hold parallel processing widgets
  TGCompositeFrame *ProcessingOptionsTab_CF = OptionsTabs_T->AddTab("Processing");
  ProcessingOptionsTab_CF->AddFrame(ProcessingOptions_CF = new TGCompositeFrame(ProcessingOptionsTab_CF, 1, 1, kVerticalFrame));
  ProcessingOptions_CF->Resize(320,735);
  ProcessingOptions_CF->ChangeOptions(ProcessingOptions_CF->GetOptions() | kFixedSize);

  FillWaveformFrame();
  FillSpectrumFrame();
  FillAnalysisFrame();
  FillGraphicsFrame();
  FillProcessingFrame();
  FillCanvasFrame();

  //////////////////////////////////////
  // Finalize options and map windows //
  //////////////////////////////////////
  
  SetBackgroundColor(ColorMgr->Number2Pixel(16));
  
  // Create a string that will be located in the title bar of the
  // ADAQAnalysisGUI to identify the version number of the code
  string TitleString;
  if(VersionString == "Development")
    TitleString = "ADAQ Analysis Code (Development version)                                             Fear is the mind-killer.";
  else
    TitleString = "ADAQ Analysis Code (Production version " + VersionString + ")                                       Fear is the mind-killer.";

  SetWindowName(TitleString.c_str());
  MapSubwindows();
  Resize(1125,800);
  MapWindow();
}


void AAInterface::FillWaveformFrame()
{
  /////////////////////////////////////////
  // Fill the waveform options tabbed frame
  
  // Waveform selection (data channel and waveform number)
  
  TGHorizontalFrame *WaveformSelection_HF = new TGHorizontalFrame(WaveformOptions_CF);
  WaveformOptions_CF->AddFrame(WaveformSelection_HF, new TGLayoutHints(kLHintsLeft, 15,5,15,5));
  
  WaveformSelection_HF->AddFrame(ChannelSelector_CBL = new ADAQComboBoxWithLabel(WaveformSelection_HF, "", -1),
				 new TGLayoutHints(kLHintsLeft, 0,5,5,5));
  stringstream ss;
  string entry;
  for(int ch=0; ch<NumDataChannels; ch++){
    ss << ch;
    entry.assign("Channel " + ss.str());
    ChannelSelector_CBL->GetComboBox()->AddEntry(entry.c_str(),ch);
    ss.str("");
  }
  ChannelSelector_CBL->GetComboBox()->Select(0);

  WaveformSelection_HF->AddFrame(WaveformSelector_NEL = new ADAQNumberEntryWithLabel(WaveformSelection_HF, "Waveform", WaveformSelector_NEL_ID),
				 new TGLayoutHints(kLHintsLeft, 5,5,5,5));
  WaveformSelector_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  WaveformSelector_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  WaveformSelector_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");
  
  // Waveform specification (type and polarity)

  TGHorizontalFrame *WaveformSpecification_HF = new TGHorizontalFrame(WaveformOptions_CF);
  WaveformOptions_CF->AddFrame(WaveformSpecification_HF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  
  WaveformSpecification_HF->AddFrame(WaveformType_BG = new TGButtonGroup(WaveformSpecification_HF, "Type", kVerticalFrame),
				     new TGLayoutHints(kLHintsLeft, 0,5,0,5));
  RawWaveform_RB = new TGRadioButton(WaveformType_BG, "Raw voltage", RawWaveform_RB_ID);
  RawWaveform_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  //RawWaveform_RB->SetState(kButtonDown);
  
  BaselineSubtractedWaveform_RB = new TGRadioButton(WaveformType_BG, "Baseline-subtracted", BaselineSubtractedWaveform_RB_ID);
  BaselineSubtractedWaveform_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  BaselineSubtractedWaveform_RB->SetState(kButtonDown);

  ZeroSuppressionWaveform_RB = new TGRadioButton(WaveformType_BG, "Zero suppression", ZeroSuppressionWaveform_RB_ID);
  ZeroSuppressionWaveform_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");

  WaveformSpecification_HF->AddFrame(WaveformPolarity_BG = new TGButtonGroup(WaveformSpecification_HF, "Polarity", kVerticalFrame),
				     new TGLayoutHints(kLHintsLeft, 5,5,0,5));
  
  PositiveWaveform_RB = new TGRadioButton(WaveformPolarity_BG, "Positive", PositiveWaveform_RB_ID);
  PositiveWaveform_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  //PositiveWaveform_RB->SetState(kButtonDown);
  
  NegativeWaveform_RB = new TGRadioButton(WaveformPolarity_BG, "Negative", NegativeWaveform_RB_ID);
  NegativeWaveform_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  NegativeWaveform_RB->SetState(kButtonDown);

  // Zero suppression options

  WaveformOptions_CF->AddFrame(PlotZeroSuppressionCeiling_CB = new TGCheckButton(WaveformOptions_CF, "Plot zero suppression ceiling", PlotZeroSuppressionCeiling_CB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  PlotZeroSuppressionCeiling_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  
  WaveformOptions_CF->AddFrame(ZeroSuppressionCeiling_NEL = new ADAQNumberEntryWithLabel(WaveformOptions_CF, "Zero suppression ceiling", ZeroSuppressionCeiling_NEL_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,0,5));
  ZeroSuppressionCeiling_NEL->GetEntry()->SetNumber(15);
  ZeroSuppressionCeiling_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  // Peak finding (ROOT TSpectrum) options

  WaveformOptions_CF->AddFrame(FindPeaks_CB = new TGCheckButton(WaveformOptions_CF, "Find peaks", FindPeaks_CB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  FindPeaks_CB->SetState(kButtonDisabled);
  FindPeaks_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");

  WaveformOptions_CF->AddFrame(MaxPeaks_NEL = new ADAQNumberEntryWithLabel(WaveformOptions_CF, "Maximum peaks", MaxPeaks_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  MaxPeaks_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  MaxPeaks_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  MaxPeaks_NEL->GetEntry()->SetNumber(15);
  MaxPeaks_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  WaveformOptions_CF->AddFrame(Sigma_NEL = new ADAQNumberEntryWithLabel(WaveformOptions_CF, "Sigma", Sigma_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  Sigma_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  Sigma_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  Sigma_NEL->GetEntry()->SetNumber(15);
  Sigma_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  WaveformOptions_CF->AddFrame(Resolution_NEL = new ADAQNumberEntryWithLabel(WaveformOptions_CF, "Resolution", Resolution_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  Resolution_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  Resolution_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  Resolution_NEL->GetEntry()->SetNumber(0.005);
  Resolution_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  WaveformOptions_CF->AddFrame(Floor_NEL = new ADAQNumberEntryWithLabel(WaveformOptions_CF, "Floor", Floor_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  Floor_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  Floor_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  Floor_NEL->GetEntry()->SetNumber(50);
  Floor_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");
  
  TGGroupFrame *PeakPlottingOptions_GF = new TGGroupFrame(WaveformOptions_CF, "Peak finding plotting options", kHorizontalFrame);
  WaveformOptions_CF->AddFrame(PeakPlottingOptions_GF, new TGLayoutHints(kLHintsLeft, 15,5,0,5));

  PeakPlottingOptions_GF->AddFrame(PlotFloor_CB = new TGCheckButton(PeakPlottingOptions_GF, "Floor", PlotFloor_CB_ID),
				   new TGLayoutHints(kLHintsLeft, 0,0,5,0));
  PlotFloor_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  
  PeakPlottingOptions_GF->AddFrame(PlotCrossings_CB = new TGCheckButton(PeakPlottingOptions_GF, "Crossings", PlotCrossings_CB_ID),
				   new TGLayoutHints(kLHintsLeft, 5,0,5,0));
  PlotCrossings_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  
  PeakPlottingOptions_GF->AddFrame(PlotPeakIntegratingRegion_CB = new TGCheckButton(PeakPlottingOptions_GF, "Int. region", PlotPeakIntegratingRegion_CB_ID),
		       new TGLayoutHints(kLHintsLeft, 5,0,5,0));
  PlotPeakIntegratingRegion_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");

  // Pileup options
  
  WaveformOptions_CF->AddFrame(UsePileupRejection_CB = new TGCheckButton(WaveformOptions_CF, "Use pileup rejection", UsePileupRejection_CB_ID),
			       new TGLayoutHints(kLHintsNormal, 15,5,5,5));
  UsePileupRejection_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  UsePileupRejection_CB->SetState(kButtonDown);
			       
  // Baseline calculation options
  
  WaveformOptions_CF->AddFrame(PlotBaseline_CB = new TGCheckButton(WaveformOptions_CF, "Plot baseline calc. region.", PlotBaseline_CB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  PlotBaseline_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  
  TGHorizontalFrame *BaselineRegion_HF = new TGHorizontalFrame(WaveformOptions_CF);
  WaveformOptions_CF->AddFrame(BaselineRegion_HF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  
  BaselineRegion_HF->AddFrame(BaselineCalcMin_NEL = new ADAQNumberEntryWithLabel(BaselineRegion_HF, "Min.", BaselineCalcMin_NEL_ID),
			      new TGLayoutHints(kLHintsLeft, 0,5,0,0));
  BaselineCalcMin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  BaselineCalcMin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  BaselineCalcMin_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  BaselineCalcMin_NEL->GetEntry()->SetLimitValues(0,1); // Set when ADAQRootFile loaded
  BaselineCalcMin_NEL->GetEntry()->SetNumber(0); // Set when ADAQRootFile loaded
  BaselineCalcMin_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");
  
  BaselineRegion_HF->AddFrame(BaselineCalcMax_NEL = new ADAQNumberEntryWithLabel(BaselineRegion_HF, "Max.", BaselineCalcMax_NEL_ID),
			      new TGLayoutHints(kLHintsLeft, 0,5,0,5)); 
  BaselineCalcMax_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  BaselineCalcMax_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  BaselineCalcMax_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  BaselineCalcMax_NEL->GetEntry()->SetLimitValues(1,2); // Set When ADAQRootFile loaded
  BaselineCalcMax_NEL->GetEntry()->SetNumber(1); // Set when ADAQRootFile loaded
  BaselineCalcMax_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  // Trigger

  WaveformOptions_CF->AddFrame(PlotTrigger_CB = new TGCheckButton(WaveformOptions_CF, "Plot trigger", PlotTrigger_CB_ID),
		       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  PlotTrigger_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
}


void AAInterface::FillSpectrumFrame()
{
  /////////////////////////////////////////
  // Fill the spectrum options tabbed frame
  
  TGCanvas *SpectrumFrame_C = new TGCanvas(SpectrumOptions_CF, 320, 705, kDoubleBorder);
  SpectrumOptions_CF->AddFrame(SpectrumFrame_C, new TGLayoutHints(kLHintsCenterX, 0,0,10,10));
  
  TGVerticalFrame *SpectrumFrame_VF = new TGVerticalFrame(SpectrumFrame_C->GetViewPort(), 320, 705);
  SpectrumFrame_C->SetContainer(SpectrumFrame_VF);
  
  // Number of waveforms to bin in the histogram
  SpectrumFrame_VF->AddFrame(WaveformsToHistogram_NEL = new ADAQNumberEntryWithLabel(SpectrumFrame_VF, "Waveforms to histogram", WaveformsToHistogram_NEL_ID),
			     new TGLayoutHints(kLHintsLeft, 15,5,10,5));
  WaveformsToHistogram_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  WaveformsToHistogram_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  WaveformsToHistogram_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  WaveformsToHistogram_NEL->GetEntry()->SetLimitValues(1,2);
  WaveformsToHistogram_NEL->GetEntry()->SetNumber(0);
  
  // Number of spectrum bins number entry
  SpectrumFrame_VF->AddFrame(SpectrumNumBins_NEL = new ADAQNumberEntryWithLabel(SpectrumFrame_VF, "Number of bins", SpectrumNumBins_NEL_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  SpectrumNumBins_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  SpectrumNumBins_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  SpectrumNumBins_NEL->GetEntry()->SetNumber(200);

  TGHorizontalFrame *SpectrumBinning_HF = new TGHorizontalFrame(SpectrumFrame_VF);
  SpectrumFrame_VF->AddFrame(SpectrumBinning_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));

  // Minimum spectrum bin number entry
  SpectrumBinning_HF->AddFrame(SpectrumMinBin_NEL = new ADAQNumberEntryWithLabel(SpectrumBinning_HF, "Min. bin", SpectrumMinBin_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 15,0,0,0));
  SpectrumMinBin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumMinBin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  SpectrumMinBin_NEL->GetEntry()->SetNumber(0);

  // Maximum spectrum bin number entry
  SpectrumBinning_HF->AddFrame(SpectrumMaxBin_NEL = new ADAQNumberEntryWithLabel(SpectrumBinning_HF, "Max. bin", SpectrumMaxBin_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 15,0,0,0));
  SpectrumMaxBin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumMaxBin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumMaxBin_NEL->GetEntry()->SetNumber(22000);

  TGHorizontalFrame *SpectrumOptions_HF = new TGHorizontalFrame(SpectrumFrame_VF);
  SpectrumFrame_VF->AddFrame(SpectrumOptions_HF, new TGLayoutHints(kLHintsNormal, 15,0,0,0));

  // Spectrum type radio buttons
  SpectrumOptions_HF->AddFrame(SpectrumType_BG = new TGButtonGroup(SpectrumOptions_HF, "Spectrum type", kVerticalFrame),
			       new TGLayoutHints(kLHintsLeft, 0,5,0,0));
  SpectrumTypePAS_RB = new TGRadioButton(SpectrumType_BG, "Pulse area", -1);
  SpectrumTypePAS_RB->SetState(kButtonDown);
  
  SpectrumTypePHS_RB = new TGRadioButton(SpectrumType_BG, "Pulse height", -1);
  //SpectrumTypePHS_RB->SetState(kButtonDown);

  // Integration type radio buttons
  SpectrumOptions_HF->AddFrame(IntegrationType_BG = new TGButtonGroup(SpectrumOptions_HF, "Integration type", kVerticalFrame),
			       new TGLayoutHints(kLHintsLeft, 5,5,0,0));
  IntegrationTypeWholeWaveform_RB = new TGRadioButton(IntegrationType_BG, "Whole waveform", -1);
  IntegrationTypeWholeWaveform_RB->SetState(kButtonDown);
  
  IntegrationTypePeakFinder_RB = new TGRadioButton(IntegrationType_BG, "Peak finder", -1);
  //IntegrationTypePeakFinder_RB->SetState(kButtonDown);
   
  TGGroupFrame *SpectrumCalibration_GF = new TGGroupFrame(SpectrumFrame_VF, "Energy calibration", kVerticalFrame);
  SpectrumFrame_VF->AddFrame(SpectrumCalibration_GF, new TGLayoutHints(kLHintsNormal,15,5,5,5));

  // Energy calibration 
  SpectrumCalibration_GF->AddFrame(SpectrumCalibration_CB = new TGCheckButton(SpectrumCalibration_GF, "Make it so", SpectrumCalibration_CB_ID),
				   new TGLayoutHints(kLHintsLeft, 0,0,0,5));
  SpectrumCalibration_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  SpectrumCalibration_CB->SetState(kButtonUp);

  TGHorizontalFrame *SpectrumCalibrationType_HF = new TGHorizontalFrame(SpectrumCalibration_GF);
  SpectrumCalibration_GF->AddFrame(SpectrumCalibrationType_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));

  SpectrumCalibrationType_HF->AddFrame(SpectrumCalibrationManual_RB = new TGRadioButton(SpectrumCalibrationType_HF, "Manual entry", SpectrumCalibrationManual_RB_ID),
				       new TGLayoutHints(kLHintsNormal, 0,15,0,5));
  SpectrumCalibrationManual_RB->SetState(kButtonDown);
  SpectrumCalibrationManual_RB->SetState(kButtonDisabled);
  SpectrumCalibrationManual_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  
  SpectrumCalibrationType_HF->AddFrame(SpectrumCalibrationFixedEP_RB = new TGRadioButton(SpectrumCalibrationType_HF, "EP detector", SpectrumCalibrationFixedEP_RB_ID),
				       new TGLayoutHints(kLHintsNormal, 0,0,0,5));
  SpectrumCalibrationFixedEP_RB->SetState(kButtonDisabled);
  SpectrumCalibrationFixedEP_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  
  
  SpectrumCalibration_GF->AddFrame(SpectrumCalibrationPoint_CBL = new ADAQComboBoxWithLabel(SpectrumCalibration_GF, "", SpectrumCalibrationPoint_CBL_ID),
				   new TGLayoutHints(kLHintsNormal, 0,0,5,5));
  SpectrumCalibrationPoint_CBL->GetComboBox()->AddEntry("Point 0",0);
  SpectrumCalibrationPoint_CBL->GetComboBox()->Select(0);
  SpectrumCalibrationPoint_CBL->GetComboBox()->SetEnabled(false);
    
  SpectrumCalibration_GF->AddFrame(SpectrumCalibrationEnergy_NEL = new ADAQNumberEntryWithLabel(SpectrumCalibration_GF, "Energy (keV or MeV)", SpectrumCalibrationEnergy_NEL_ID),
					  new TGLayoutHints(kLHintsLeft,0,0,5,0));
  SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(0.0);
  SpectrumCalibrationEnergy_NEL->GetEntry()->SetState(false);

  SpectrumCalibration_GF->AddFrame(SpectrumCalibrationPulseUnit_NEL = new ADAQNumberEntryWithLabel(SpectrumCalibration_GF, "Pulse Unit (ADC)", SpectrumCalibrationPulseUnit_NEL_ID),
				       new TGLayoutHints(kLHintsLeft,0,0,0,5));
  SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(1.0);
  SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetState(false);

  TGHorizontalFrame *SpectrumCalibration_HF1 = new TGHorizontalFrame(SpectrumCalibration_GF);
  SpectrumCalibration_GF->AddFrame(SpectrumCalibration_HF1);
  
  // Set point text button
  SpectrumCalibration_HF1->AddFrame(SpectrumCalibrationSetPoint_TB = new TGTextButton(SpectrumCalibration_HF1, "Set Pt.", SpectrumCalibrationSetPoint_TB_ID),
				    new TGLayoutHints(kLHintsNormal, 5,0,5,5));
  SpectrumCalibrationSetPoint_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");
  SpectrumCalibrationSetPoint_TB->Resize(100,25);
  SpectrumCalibrationSetPoint_TB->ChangeOptions(SpectrumCalibrationSetPoint_TB->GetOptions() | kFixedSize);
  SpectrumCalibrationSetPoint_TB->SetState(kButtonDisabled);

  // Calibrate text button
  SpectrumCalibration_HF1->AddFrame(SpectrumCalibrationCalibrate_TB = new TGTextButton(SpectrumCalibration_HF1, "Calibrate", SpectrumCalibrationCalibrate_TB_ID),
				    new TGLayoutHints(kLHintsNormal, 0,0,5,5));
  SpectrumCalibrationCalibrate_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");
  SpectrumCalibrationCalibrate_TB->Resize(100,25);
  SpectrumCalibrationCalibrate_TB->ChangeOptions(SpectrumCalibrationCalibrate_TB->GetOptions() | kFixedSize);
  SpectrumCalibrationCalibrate_TB->SetState(kButtonDisabled);

  TGHorizontalFrame *SpectrumCalibration_HF2 = new TGHorizontalFrame(SpectrumCalibration_GF);
  SpectrumCalibration_GF->AddFrame(SpectrumCalibration_HF2);
  
  // Plot text button
  SpectrumCalibration_HF2->AddFrame(SpectrumCalibrationPlot_TB = new TGTextButton(SpectrumCalibration_HF2, "Plot", SpectrumCalibrationPlot_TB_ID),
				    new TGLayoutHints(kLHintsNormal, 5,0,5,5));
  SpectrumCalibrationPlot_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");
  SpectrumCalibrationPlot_TB->Resize(100,25);
  SpectrumCalibrationPlot_TB->ChangeOptions(SpectrumCalibrationPlot_TB->GetOptions() | kFixedSize);
  SpectrumCalibrationPlot_TB->SetState(kButtonDisabled);

  // Reset text button
  SpectrumCalibration_HF2->AddFrame(SpectrumCalibrationReset_TB = new TGTextButton(SpectrumCalibration_HF2, "Reset", SpectrumCalibrationReset_TB_ID),
					  new TGLayoutHints(kLHintsNormal, 0,0,5,5));
  SpectrumCalibrationReset_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");
  SpectrumCalibrationReset_TB->Resize(100,25);
  SpectrumCalibrationReset_TB->ChangeOptions(SpectrumCalibrationReset_TB->GetOptions() | kFixedSize);
  SpectrumCalibrationReset_TB->SetState(kButtonDisabled);

  // Spectrum analysis
  
  TGGroupFrame *SpectrumAnalysis_GF = new TGGroupFrame(SpectrumFrame_VF, "Analysis", kVerticalFrame);
  SpectrumFrame_VF->AddFrame(SpectrumAnalysis_GF, new TGLayoutHints(kLHintsNormal, 15,5,5,5));

  /*
  SpectrumAnalysis_GF->AddFrame(SpectrumFindPeaks_CB = new TGCheckButton(SpectrumAnalysis_GF, "Find peaks", SpectrumFindPeaks_CB_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  SpectrumFindPeaks_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");

  SpectrumAnalysis_GF->AddFrame(SpectrumNumPeaks_NEL = new ADAQNumberEntryWithLabel(SpectrumAnalysis_GF, "Num. peaks", SpectrumNumPeaks_NEL_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumNumPeaks_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  SpectrumNumPeaks_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumNumPeaks_NEL->GetEntry()->SetNumber(1);
  SpectrumNumPeaks_NEL->GetEntry()->Resize(75,20);
  SpectrumNumPeaks_NEL->GetEntry()->SetState(false);
  SpectrumNumPeaks_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");
    
  SpectrumAnalysis_GF->AddFrame(SpectrumSigma_NEL = new ADAQNumberEntryWithLabel(SpectrumAnalysis_GF, "Sigma", SpectrumSigma_NEL_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumSigma_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  SpectrumSigma_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumSigma_NEL->GetEntry()->SetNumber(2);
  SpectrumSigma_NEL->GetEntry()->Resize(75,20);
  SpectrumSigma_NEL->GetEntry()->SetState(false);
  SpectrumSigma_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");
  
  SpectrumAnalysis_GF->AddFrame(SpectrumResolution_NEL = new ADAQNumberEntryWithLabel(SpectrumAnalysis_GF, "Resolution", SpectrumResolution_NEL_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,5));
  SpectrumResolution_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumResolution_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumResolution_NEL->GetEntry()->SetNumber(0.05);
  SpectrumResolution_NEL->GetEntry()->Resize(75,20);
  SpectrumResolution_NEL->GetEntry()->SetState(false);
  SpectrumResolution_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");
  */
  
  SpectrumAnalysis_GF->AddFrame(SpectrumFindBackground_CB = new TGCheckButton(SpectrumAnalysis_GF, "Find background", SpectrumFindBackground_CB_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  SpectrumFindBackground_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");

  TGHorizontalFrame *SpectrumBackgroundRange_HF = new TGHorizontalFrame(SpectrumAnalysis_GF);
  SpectrumAnalysis_GF->AddFrame(SpectrumBackgroundRange_HF);
  
  SpectrumBackgroundRange_HF->AddFrame(SpectrumRangeMin_NEL = new ADAQNumberEntryWithLabel(SpectrumBackgroundRange_HF, "Min.", SpectrumRangeMin_NEL_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumRangeMin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  SpectrumRangeMin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumRangeMin_NEL->GetEntry()->SetNumber(0);
  SpectrumRangeMin_NEL->GetEntry()->Resize(75,20);
  SpectrumRangeMin_NEL->GetEntry()->SetState(false);
  SpectrumRangeMin_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  SpectrumBackgroundRange_HF->AddFrame(SpectrumRangeMax_NEL = new ADAQNumberEntryWithLabel(SpectrumBackgroundRange_HF, "Max.", SpectrumRangeMax_NEL_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumRangeMax_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  SpectrumRangeMax_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumRangeMax_NEL->GetEntry()->SetNumber(2000);
  SpectrumRangeMax_NEL->GetEntry()->Resize(75,20);
  SpectrumRangeMax_NEL->GetEntry()->SetState(false);
  SpectrumRangeMax_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  TGHorizontalFrame *BackgroundPlotting_HF = new TGHorizontalFrame(SpectrumAnalysis_GF);
  SpectrumAnalysis_GF->AddFrame(BackgroundPlotting_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));

  BackgroundPlotting_HF->AddFrame(SpectrumWithBackground_RB = new TGRadioButton(BackgroundPlotting_HF, "Plot with bckgnd", SpectrumWithBackground_RB_ID),
				  new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumWithBackground_RB->SetState(kButtonDown);
  SpectrumWithBackground_RB->SetState(kButtonDisabled);
  SpectrumWithBackground_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  
  BackgroundPlotting_HF->AddFrame(SpectrumLessBackground_RB = new TGRadioButton(BackgroundPlotting_HF, "Plot less bckgnd", SpectrumLessBackground_RB_ID),
				  new TGLayoutHints(kLHintsNormal, 5,5,0,5));
  SpectrumLessBackground_RB->SetState(kButtonDisabled);
  SpectrumLessBackground_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");

  SpectrumAnalysis_GF->AddFrame(SpectrumFindIntegral_CB = new TGCheckButton(SpectrumAnalysis_GF, "Find integral", SpectrumFindIntegral_CB_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  SpectrumFindIntegral_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  
  SpectrumAnalysis_GF->AddFrame(SpectrumIntegralInCounts_CB = new TGCheckButton(SpectrumAnalysis_GF, "Integral in counts", SpectrumIntegralInCounts_CB_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumIntegralInCounts_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  
  SpectrumAnalysis_GF->AddFrame(SpectrumUseGaussianFit_CB = new TGCheckButton(SpectrumAnalysis_GF, "Use gaussian fit", SpectrumUseGaussianFit_CB_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumUseGaussianFit_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");

  SpectrumAnalysis_GF->AddFrame(SpectrumNormalizePeakToCurrent_CB = new TGCheckButton(SpectrumAnalysis_GF, "Normalize peak to current", SpectrumNormalizePeakToCurrent_CB_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumNormalizePeakToCurrent_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");

  SpectrumAnalysis_GF->AddFrame(SpectrumIntegral_NEFL = new ADAQNumberEntryFieldWithLabel(SpectrumAnalysis_GF, "Integral", -1),
				new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  SpectrumIntegral_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal, TGNumberFormat::kNEANonNegative);
  SpectrumIntegral_NEFL->GetEntry()->Resize(100,20);
  SpectrumIntegral_NEFL->GetEntry()->SetState(false);

  SpectrumAnalysis_GF->AddFrame(SpectrumIntegralError_NEFL = new ADAQNumberEntryFieldWithLabel(SpectrumAnalysis_GF, "Error", -1),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumIntegralError_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal, TGNumberFormat::kNEANonNegative);
  SpectrumIntegralError_NEFL->GetEntry()->Resize(100,20);
  SpectrumIntegralError_NEFL->GetEntry()->SetState(false);

  SpectrumAnalysis_GF->AddFrame(SpectrumOverplotDerivative_CB = new TGCheckButton(SpectrumAnalysis_GF, "Overplot spectrum derivative", SpectrumOverplotDerivative_CB_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  SpectrumOverplotDerivative_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");

  SpectrumFrame_VF->AddFrame(CreateSpectrum_TB = new TGTextButton(SpectrumFrame_VF, "Create spectrum", CreateSpectrum_TB_ID),
		       new TGLayoutHints(kLHintsCenterX | kLHintsTop, 5,5,0,0));
  CreateSpectrum_TB->Resize(200, 30);
  CreateSpectrum_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  CreateSpectrum_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  CreateSpectrum_TB->ChangeOptions(CreateSpectrum_TB->GetOptions() | kFixedSize);
  CreateSpectrum_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");
}


void AAInterface::FillAnalysisFrame()
{
  /////////////////////////////////////////
  // Fill the analysis options tabbed frame

  TGCanvas *AnalysisFrame_C = new TGCanvas(AnalysisOptions_CF, 320, 705, kDoubleBorder);
  AnalysisOptions_CF->AddFrame(AnalysisFrame_C, new TGLayoutHints(kLHintsCenterX, 0,0,10,10));

  TGVerticalFrame *AnalysisFrame_VF = new TGVerticalFrame(AnalysisFrame_C->GetViewPort(), 320, 705);
  AnalysisFrame_C->SetContainer(AnalysisFrame_VF);

  // Pulse Shape Discrimination (PSD)

  TGGroupFrame *PSDAnalysis_GF = new TGGroupFrame(AnalysisFrame_VF, "Pulse Shape Discrim.", kVerticalFrame);
  AnalysisFrame_VF->AddFrame(PSDAnalysis_GF, new TGLayoutHints(kLHintsNormal, 15,5,5,5));
  
  PSDAnalysis_GF->AddFrame(PSDEnable_CB = new TGCheckButton(PSDAnalysis_GF, "Discriminate pulse shapes", PSDEnable_CB_ID),
                           new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  PSDEnable_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");

  PSDAnalysis_GF->AddFrame(PSDChannel_CBL = new ADAQComboBoxWithLabel(PSDAnalysis_GF, "", -1),
                           new TGLayoutHints(kLHintsNormal, 0,5,0,0));

  stringstream ss;
  string entry;
  for(int ch=0; ch<NumDataChannels; ch++){
    ss << ch;
    entry.assign("Channel " + ss.str());
    PSDChannel_CBL->GetComboBox()->AddEntry(entry.c_str(),ch);
    ss.str("");
  }
  PSDChannel_CBL->GetComboBox()->Select(0);
  PSDChannel_CBL->GetComboBox()->SetEnabled(false);

  PSDAnalysis_GF->AddFrame(PSDWaveforms_NEL = new ADAQNumberEntryWithLabel(PSDAnalysis_GF, "Number of waveforms", -1),
                           new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PSDWaveforms_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDWaveforms_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  PSDWaveforms_NEL->GetEntry()->SetLimitValues(0,1); // Updated when ADAQ ROOT file loaded
  PSDWaveforms_NEL->GetEntry()->SetNumber(0); // Updated when ADAQ ROOT file loaded
  PSDWaveforms_NEL->GetEntry()->SetState(false);

  PSDAnalysis_GF->AddFrame(PSDNumTotalBins_NEL = new ADAQNumberEntryWithLabel(PSDAnalysis_GF, "Num. total bins (X axis)", -1),
			   new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  PSDNumTotalBins_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDNumTotalBins_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PSDNumTotalBins_NEL->GetEntry()->SetNumber(150);
  PSDNumTotalBins_NEL->GetEntry()->SetState(false);

  TGHorizontalFrame *TotalBins_HF = new TGHorizontalFrame(PSDAnalysis_GF);
  PSDAnalysis_GF->AddFrame(TotalBins_HF);

  TotalBins_HF->AddFrame(PSDMinTotalBin_NEL = new ADAQNumberEntryWithLabel(TotalBins_HF, "Min.", -1),
			 new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PSDMinTotalBin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDMinTotalBin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PSDMinTotalBin_NEL->GetEntry()->SetNumber(0);
  PSDMinTotalBin_NEL->GetEntry()->SetState(false);

  TotalBins_HF->AddFrame(PSDMaxTotalBin_NEL = new ADAQNumberEntryWithLabel(TotalBins_HF, "Max.", -1),
			 new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PSDMaxTotalBin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDMaxTotalBin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PSDMaxTotalBin_NEL->GetEntry()->SetNumber(20000);
  PSDMaxTotalBin_NEL->GetEntry()->SetState(false);
  
  PSDAnalysis_GF->AddFrame(PSDNumTailBins_NEL = new ADAQNumberEntryWithLabel(PSDAnalysis_GF, "Num. tail bins (Y axis)", -1),
			   new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  PSDNumTailBins_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDNumTailBins_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PSDNumTailBins_NEL->GetEntry()->SetNumber(150);
  PSDNumTailBins_NEL->GetEntry()->SetState(false);

  TGHorizontalFrame *TailBins_HF = new TGHorizontalFrame(PSDAnalysis_GF);
  PSDAnalysis_GF->AddFrame(TailBins_HF);

  TailBins_HF->AddFrame(PSDMinTailBin_NEL = new ADAQNumberEntryWithLabel(TailBins_HF, "Min.", -1),
			   new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PSDMinTailBin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDMinTailBin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PSDMinTailBin_NEL->GetEntry()->SetNumber(0);
  PSDMinTailBin_NEL->GetEntry()->SetState(false);

  TailBins_HF->AddFrame(PSDMaxTailBin_NEL = new ADAQNumberEntryWithLabel(TailBins_HF, "Max.", -1),
			   new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PSDMaxTailBin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDMaxTailBin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PSDMaxTailBin_NEL->GetEntry()->SetNumber(5000);
  PSDMaxTailBin_NEL->GetEntry()->SetState(false);


  PSDAnalysis_GF->AddFrame(PSDThreshold_NEL = new ADAQNumberEntryWithLabel(PSDAnalysis_GF, "Threshold (ADC)", -1),
                           new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  PSDThreshold_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDThreshold_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  PSDThreshold_NEL->GetEntry()->SetLimitValues(0,4095); // Updated when ADAQ ROOT file loaded
  PSDThreshold_NEL->GetEntry()->SetNumber(1500); // Updated when ADAQ ROOT file loaded
  PSDThreshold_NEL->GetEntry()->SetState(false);
  
  PSDAnalysis_GF->AddFrame(PSDPeakOffset_NEL = new ADAQNumberEntryWithLabel(PSDAnalysis_GF, "Peak offset (sample)", PSDPeakOffset_NEL_ID),
                           new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PSDPeakOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDPeakOffset_NEL->GetEntry()->SetNumber(7);
  PSDPeakOffset_NEL->GetEntry()->SetState(false);
  PSDPeakOffset_NEL->GetEntry()->Connect("ValueSet(long", "AAInterface", this, "HandleNumberEntries()");

  PSDAnalysis_GF->AddFrame(PSDTailOffset_NEL = new ADAQNumberEntryWithLabel(PSDAnalysis_GF, "Tail offset (sample)", PSDTailOffset_NEL_ID),
                           new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PSDTailOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDTailOffset_NEL->GetEntry()->SetNumber(29);
  PSDTailOffset_NEL->GetEntry()->SetState(false);
  PSDTailOffset_NEL->GetEntry()->Connect("ValueSet(long", "AAInterface", this, "HandleNumberEntries()");
  
  PSDAnalysis_GF->AddFrame(PSDPlotType_CBL = new ADAQComboBoxWithLabel(PSDAnalysis_GF, "Plot type", PSDPlotType_CBL_ID),
			   new TGLayoutHints(kLHintsNormal, 0,5,5,5));
  PSDPlotType_CBL->GetComboBox()->AddEntry("COL",0);
  PSDPlotType_CBL->GetComboBox()->AddEntry("LEGO2",1);
  PSDPlotType_CBL->GetComboBox()->AddEntry("SURF3",2);
  PSDPlotType_CBL->GetComboBox()->AddEntry("SURF4",3);
  PSDPlotType_CBL->GetComboBox()->AddEntry("CONT",4);
  PSDPlotType_CBL->GetComboBox()->Select(0);
  PSDPlotType_CBL->GetComboBox()->Connect("Selected(int,int)", "AAInterface", this, "HandleComboBox(int,int)");
  PSDPlotType_CBL->GetComboBox()->SetEnabled(false);

  PSDAnalysis_GF->AddFrame(PSDPlotTailIntegration_CB = new TGCheckButton(PSDAnalysis_GF, "Plot tail integration region", PSDPlotTailIntegration_CB_ID),
                           new TGLayoutHints(kLHintsNormal, 0,5,0,5));
  PSDPlotTailIntegration_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  PSDPlotTailIntegration_CB->SetState(kButtonDisabled);
  
  PSDAnalysis_GF->AddFrame(PSDEnableHistogramSlicing_CB = new TGCheckButton(PSDAnalysis_GF, "Enable histogram slicing", PSDEnableHistogramSlicing_CB_ID),
			   new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  PSDEnableHistogramSlicing_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");

  TGHorizontalFrame *PSDHistogramSlicing_HF = new TGHorizontalFrame(PSDAnalysis_GF);
  PSDAnalysis_GF->AddFrame(PSDHistogramSlicing_HF, new TGLayoutHints(kLHintsCenterX, 0,5,0,5));
  
  PSDHistogramSlicing_HF->AddFrame(PSDHistogramSliceX_RB = new TGRadioButton(PSDHistogramSlicing_HF, "X slice", PSDHistogramSliceX_RB_ID),
				   new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  PSDHistogramSliceX_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  PSDHistogramSliceX_RB->SetState(kButtonDown);
  PSDHistogramSliceX_RB->SetState(kButtonDisabled);

  PSDHistogramSlicing_HF->AddFrame(PSDHistogramSliceY_RB = new TGRadioButton(PSDHistogramSlicing_HF, "Y slice", PSDHistogramSliceY_RB_ID),
				   new TGLayoutHints(kLHintsNormal, 20,0,0,0));
  PSDHistogramSliceY_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  PSDHistogramSliceY_RB->SetState(kButtonDisabled);
  
  PSDAnalysis_GF->AddFrame(PSDEnableFilterCreation_CB = new TGCheckButton(PSDAnalysis_GF, "Enable filter creation", PSDEnableFilterCreation_CB_ID),
			   new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  PSDEnableFilterCreation_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  PSDEnableFilterCreation_CB->SetState(kButtonDisabled);

  
  PSDAnalysis_GF->AddFrame(PSDEnableFilter_CB = new TGCheckButton(PSDAnalysis_GF, "Enable filter use", PSDEnableFilter_CB_ID),
			   new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PSDEnableFilter_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  PSDEnableFilter_CB->SetState(kButtonDisabled);

  TGHorizontalFrame *PSDFilterPolarity_HF = new TGHorizontalFrame(PSDAnalysis_GF);
  PSDAnalysis_GF->AddFrame(PSDFilterPolarity_HF);

  PSDFilterPolarity_HF->AddFrame(PSDPositiveFilter_RB = new TGRadioButton(PSDFilterPolarity_HF, "Positive  ", PSDPositiveFilter_RB_ID),
				 new TGLayoutHints(kLHintsNormal, 5,5,0,5));
  PSDPositiveFilter_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  PSDPositiveFilter_RB->SetState(kButtonDown);
  PSDPositiveFilter_RB->SetState(kButtonDisabled);

  PSDFilterPolarity_HF->AddFrame(PSDNegativeFilter_RB = new TGRadioButton(PSDFilterPolarity_HF, "Negative", PSDNegativeFilter_RB_ID),
				 new TGLayoutHints(kLHintsNormal, 5,5,0,5));
  PSDNegativeFilter_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  PSDNegativeFilter_RB->SetState(kButtonDisabled);

  
  PSDAnalysis_GF->AddFrame(PSDClearFilter_TB = new TGTextButton(PSDAnalysis_GF, "Clear filter", PSDClearFilter_TB_ID),
			   new TGLayoutHints(kLHintsNormal, 0,5,0,5));
  PSDClearFilter_TB->Resize(200,30);
  PSDClearFilter_TB->ChangeOptions(PSDClearFilter_TB->GetOptions() | kFixedSize);
  PSDClearFilter_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");
  PSDClearFilter_TB->SetState(kButtonDisabled);
  
  PSDAnalysis_GF->AddFrame(PSDCalculate_TB = new TGTextButton(PSDAnalysis_GF, "Create PSD histogram", PSDCalculate_TB_ID),
                           new TGLayoutHints(kLHintsNormal, 0,5,5,5));
  PSDCalculate_TB->Resize(200,30);
  PSDCalculate_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  PSDCalculate_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  PSDCalculate_TB->ChangeOptions(PSDCalculate_TB->GetOptions() | kFixedSize);
  PSDCalculate_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");
  PSDCalculate_TB->SetState(kButtonDisabled);

  // Count rate

  TGGroupFrame *CountRate_GF = new TGGroupFrame(AnalysisFrame_VF, "Count rate", kVerticalFrame);
  AnalysisFrame_VF->AddFrame(CountRate_GF, new TGLayoutHints(kLHintsNormal, 15,5,5,5));

  CountRate_GF->AddFrame(RFQPulseWidth_NEL = new ADAQNumberEntryWithLabel(CountRate_GF, "RFQ pulse width [us]", -1),
			 new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  RFQPulseWidth_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  RFQPulseWidth_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  RFQPulseWidth_NEL->GetEntry()->SetNumber(50);
  RFQPulseWidth_NEL->GetEntry()->Resize(70,20);

  CountRate_GF->AddFrame(RFQRepRate_NEL = new ADAQNumberEntryWithLabel(CountRate_GF, "RFQ rep. rate [Hz]", -1),
			 new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  RFQRepRate_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  RFQRepRate_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  RFQRepRate_NEL->GetEntry()->SetNumber(30);
  RFQRepRate_NEL->GetEntry()->Resize(70,20);

  CountRate_GF->AddFrame(CountRateWaveforms_NEL = new ADAQNumberEntryWithLabel(CountRate_GF, "Waveforms to use", -1),
                         new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  CountRateWaveforms_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  CountRateWaveforms_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  CountRateWaveforms_NEL->GetEntry()->SetNumber(1000);
  CountRateWaveforms_NEL->GetEntry()->Resize(70,20);

  CountRate_GF->AddFrame(CalculateCountRate_TB = new TGTextButton(CountRate_GF, "Calculate count rate", CountRate_TB_ID),
                         new TGLayoutHints(kLHintsNormal, 5,5,5,5));
  CalculateCountRate_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  CalculateCountRate_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  CalculateCountRate_TB->Resize(200,30);
  CalculateCountRate_TB->ChangeOptions(CalculateCountRate_TB->GetOptions() | kFixedSize);
  CalculateCountRate_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");

  CountRate_GF->AddFrame(InstCountRate_NEFL = new ADAQNumberEntryFieldWithLabel(CountRate_GF, "Inst. count rate [1/s]", -1),
                         new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  InstCountRate_NEFL->GetEntry()->Resize(100,20);
  InstCountRate_NEFL->GetEntry()->SetState(kButtonDisabled);
  InstCountRate_NEFL->GetEntry()->SetAlignment(kTextCenterX);
  InstCountRate_NEFL->GetEntry()->SetBackgroundColor(ColorMgr->Number2Pixel(19));
  InstCountRate_NEFL->GetEntry()->ChangeOptions(InstCountRate_NEFL->GetEntry()->GetOptions() | kFixedSize);

  CountRate_GF->AddFrame(AvgCountRate_NEFL = new ADAQNumberEntryFieldWithLabel(CountRate_GF, "Avg. count rate [1/s]", -1),
                         new TGLayoutHints(kLHintsNormal, 5,5,0,5));
  AvgCountRate_NEFL->GetEntry()->Resize(100,20);
  AvgCountRate_NEFL->GetEntry()->SetState(kButtonDisabled);
  AvgCountRate_NEFL->GetEntry()->SetAlignment(kTextCenterX);
  AvgCountRate_NEFL->GetEntry()->SetBackgroundColor(ColorMgr->Number2Pixel(19));
  AvgCountRate_NEFL->GetEntry()->ChangeOptions(AvgCountRate_NEFL->GetEntry()->GetOptions() | kFixedSize);
}


void AAInterface::FillGraphicsFrame()
{
  /////////////////////////////////////////
  // Fill the graphics options tabbed frame

  TGCanvas *GraphicsFrame_C = new TGCanvas(GraphicsOptions_CF, 320, 705, kDoubleBorder);
  GraphicsOptions_CF->AddFrame(GraphicsFrame_C, new TGLayoutHints(kLHintsCenterX, 0,0,10,10));
  
  TGVerticalFrame *GraphicsFrame_VF = new TGVerticalFrame(GraphicsFrame_C->GetViewPort(), 320, 705);
  GraphicsFrame_C->SetContainer(GraphicsFrame_VF);
  
  GraphicsFrame_VF->AddFrame(WaveformDrawOptions_BG = new TGButtonGroup(GraphicsFrame_VF, "Waveform draw options", kHorizontalFrame),
			      new TGLayoutHints(kLHintsNormal, 5,5,5,5));
  
  DrawWaveformWithCurve_RB = new TGRadioButton(WaveformDrawOptions_BG, "Smooth curve   ", DrawWaveformWithCurve_RB_ID);
  DrawWaveformWithCurve_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  DrawWaveformWithCurve_RB->SetState(kButtonDown);
  
  DrawWaveformWithMarkers_RB = new TGRadioButton(WaveformDrawOptions_BG, "Markers   ", DrawWaveformWithMarkers_RB_ID);
  DrawWaveformWithMarkers_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  
  DrawWaveformWithBoth_RB = new TGRadioButton(WaveformDrawOptions_BG, "Both", DrawWaveformWithBoth_RB_ID);
  DrawWaveformWithBoth_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  
  GraphicsFrame_VF->AddFrame(SpectrumDrawOptions_BG = new TGButtonGroup(GraphicsFrame_VF, "Spectrum draw options", kHorizontalFrame),
			      new TGLayoutHints(kLHintsNormal, 5,5,5,5));
  
  DrawSpectrumWithCurve_RB = new TGRadioButton(SpectrumDrawOptions_BG, "Smooth curve   ", DrawSpectrumWithCurve_RB_ID);
  DrawSpectrumWithCurve_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  DrawSpectrumWithCurve_RB->SetState(kButtonDown);
  
  DrawSpectrumWithMarkers_RB = new TGRadioButton(SpectrumDrawOptions_BG, "Markers   ", DrawSpectrumWithMarkers_RB_ID);
  DrawSpectrumWithMarkers_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");

  DrawSpectrumWithBars_RB = new TGRadioButton(SpectrumDrawOptions_BG, "Bars", DrawSpectrumWithBars_RB_ID);
  DrawSpectrumWithBars_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");

  GraphicsFrame_VF->AddFrame(SetStatsOff_CB = new TGCheckButton(GraphicsFrame_VF, "Set statistics off", SetStatsOff_CB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  SetStatsOff_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  
  GraphicsFrame_VF->AddFrame(PlotVerticalAxisInLog_CB = new TGCheckButton(GraphicsFrame_VF, "Vertical axis in log.", PlotVerticalAxisInLog_CB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  PlotVerticalAxisInLog_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");

  GraphicsFrame_VF->AddFrame(PlotSpectrumDerivativeError_CB = new TGCheckButton(GraphicsFrame_VF, "Plot spectrum derivative error", PlotSpectrumDerivativeError_CB_ID),
			     new TGLayoutHints(kLHintsNormal, 15,5,0,0));
  PlotSpectrumDerivativeError_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  
  GraphicsFrame_VF->AddFrame(PlotAbsValueSpectrumDerivative_CB = new TGCheckButton(GraphicsFrame_VF, "Plot abs. value of spectrum derivative ", PlotAbsValueSpectrumDerivative_CB_ID),
			     new TGLayoutHints(kLHintsNormal, 15,5,0,0));
  PlotAbsValueSpectrumDerivative_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  
  GraphicsFrame_VF->AddFrame(AutoYAxisRange_CB = new TGCheckButton(GraphicsFrame_VF, "Auto. Y Axis Range (waveform only)", -1),
			       new TGLayoutHints(kLHintsLeft, 15,5,0,5));

  TGHorizontalFrame *ResetAxesLimits_HF = new TGHorizontalFrame(GraphicsFrame_VF);
  GraphicsFrame_VF->AddFrame(ResetAxesLimits_HF, new TGLayoutHints(kLHintsLeft,15,5,5,5));
  
  ResetAxesLimits_HF->AddFrame(ResetXAxisLimits_TB = new TGTextButton(ResetAxesLimits_HF, "Reset X Axis", ResetXAxisLimits_TB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  ResetXAxisLimits_TB->Resize(175, 40);
  ResetXAxisLimits_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");
  
  ResetAxesLimits_HF->AddFrame(ResetYAxisLimits_TB = new TGTextButton(ResetAxesLimits_HF, "Reset Y Axis", ResetYAxisLimits_TB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,0,5));
  ResetYAxisLimits_TB->Resize(175, 40);
  ResetYAxisLimits_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");
  
  GraphicsFrame_VF->AddFrame(ReplotWaveform_TB = new TGTextButton(GraphicsFrame_VF, "Replot waveform", ReplotWaveform_TB_ID),
			       new TGLayoutHints(kLHintsCenterX, 15,5,5,5));
  ReplotWaveform_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  ReplotWaveform_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  ReplotWaveform_TB->Resize(200,30);
  ReplotWaveform_TB->ChangeOptions(ReplotWaveform_TB->GetOptions() | kFixedSize);
  ReplotWaveform_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");

  GraphicsFrame_VF->AddFrame(ReplotSpectrum_TB = new TGTextButton(GraphicsFrame_VF, "Replot spectrum", ReplotSpectrum_TB_ID),
			     new TGLayoutHints(kLHintsCenterX, 15,5,5,5));
  ReplotSpectrum_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  ReplotSpectrum_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  ReplotSpectrum_TB->Resize(200,30);
  ReplotSpectrum_TB->ChangeOptions(ReplotSpectrum_TB->GetOptions() | kFixedSize);
  ReplotSpectrum_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");
  
  GraphicsFrame_VF->AddFrame(ReplotSpectrumDerivative_TB = new TGTextButton(GraphicsFrame_VF, "Replot spectrum derivative", ReplotSpectrumDerivative_TB_ID),
			     new TGLayoutHints(kLHintsCenterX, 15,5,5,5));
  ReplotSpectrumDerivative_TB->Resize(200, 30);
  ReplotSpectrumDerivative_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  ReplotSpectrumDerivative_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  ReplotSpectrumDerivative_TB->ChangeOptions(ReplotSpectrumDerivative_TB->GetOptions() | kFixedSize);
  ReplotSpectrumDerivative_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");

  GraphicsFrame_VF->AddFrame(ReplotPSDHistogram_TB = new TGTextButton(GraphicsFrame_VF, "Replot PSD histogram", ReplotPSDHistogram_TB_ID),
			       new TGLayoutHints(kLHintsCenterX, 15,5,5,5));
  ReplotPSDHistogram_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  ReplotPSDHistogram_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  ReplotPSDHistogram_TB->Resize(200,30);
  ReplotPSDHistogram_TB->ChangeOptions(ReplotPSDHistogram_TB->GetOptions() | kFixedSize);
  ReplotPSDHistogram_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");

  // Override default plot options
  GraphicsFrame_VF->AddFrame(OverrideTitles_CB = new TGCheckButton(GraphicsFrame_VF, "Override default graphical options", OverrideTitles_CB_ID),
			       new TGLayoutHints(kLHintsNormal, 5,5,5,5));
  OverrideTitles_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");

  // Plot title text entry
  GraphicsFrame_VF->AddFrame(Title_TEL = new ADAQTextEntryWithLabel(GraphicsFrame_VF, "Plot title", Title_TEL_ID),
			       new TGLayoutHints(kLHintsNormal, 5,5,5,5));

  // X-axis title options

  TGGroupFrame *XAxis_GF = new TGGroupFrame(GraphicsFrame_VF, "X Axis", kVerticalFrame);
  GraphicsFrame_VF->AddFrame(XAxis_GF, new TGLayoutHints(kLHintsNormal, 5,5,5,5));

  XAxis_GF->AddFrame(XAxisTitle_TEL = new ADAQTextEntryWithLabel(XAxis_GF, "Title", XAxisTitle_TEL_ID),
		     new TGLayoutHints(kLHintsNormal, 0,5,5,0));

  TGHorizontalFrame *XAxisTitleOptions_HF = new TGHorizontalFrame(XAxis_GF);
  XAxis_GF->AddFrame(XAxisTitleOptions_HF, new TGLayoutHints(kLHintsNormal, 0,5,0,5));
  
  XAxisTitleOptions_HF->AddFrame(XAxisSize_NEL = new ADAQNumberEntryWithLabel(XAxisTitleOptions_HF, "Size", XAxisSize_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  XAxisSize_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  XAxisSize_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  XAxisSize_NEL->GetEntry()->SetNumber(0.04);
  XAxisSize_NEL->GetEntry()->Resize(50, 20);
  XAxisSize_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");
  
  XAxisTitleOptions_HF->AddFrame(XAxisOffset_NEL = new ADAQNumberEntryWithLabel(XAxisTitleOptions_HF, "Offset", XAxisOffset_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 5,2,0,0));
  XAxisOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  XAxisOffset_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  XAxisOffset_NEL->GetEntry()->SetNumber(1.5);
  XAxisOffset_NEL->GetEntry()->Resize(50, 20);
  XAxisOffset_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  XAxis_GF->AddFrame(XAxisDivs_NEL = new ADAQNumberEntryWithLabel(XAxis_GF, "Divisions", XAxisDivs_NEL_ID),		
		     new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  XAxisDivs_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  XAxisDivs_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  XAxisDivs_NEL->GetEntry()->SetNumber(510);
  XAxisDivs_NEL->GetEntry()->Resize(50, 20);
  XAxisDivs_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  // Y-axis title options

  TGGroupFrame *YAxis_GF = new TGGroupFrame(GraphicsFrame_VF, "Y Axis", kVerticalFrame);
  GraphicsFrame_VF->AddFrame(YAxis_GF, new TGLayoutHints(kLHintsNormal, 5,5,5,5));
  
  YAxis_GF->AddFrame(YAxisTitle_TEL = new ADAQTextEntryWithLabel(YAxis_GF, "Title", YAxisTitle_TEL_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  
  TGHorizontalFrame *YAxisTitleOptions_HF = new TGHorizontalFrame(YAxis_GF);
  YAxis_GF->AddFrame(YAxisTitleOptions_HF, new TGLayoutHints(kLHintsNormal, 0,5,0,5));

  YAxisTitleOptions_HF->AddFrame(YAxisSize_NEL = new ADAQNumberEntryWithLabel(YAxisTitleOptions_HF, "Size", YAxisSize_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  YAxisSize_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  YAxisSize_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  YAxisSize_NEL->GetEntry()->SetNumber(0.04);
  YAxisSize_NEL->GetEntry()->Resize(50, 20);
  YAxisSize_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");
  
  YAxisTitleOptions_HF->AddFrame(YAxisOffset_NEL = new ADAQNumberEntryWithLabel(YAxisTitleOptions_HF, "Offset", YAxisOffset_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 5,2,0,0));
  YAxisOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  YAxisOffset_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  YAxisOffset_NEL->GetEntry()->SetNumber(1.5);
  YAxisOffset_NEL->GetEntry()->Resize(50, 20);
  YAxisOffset_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  YAxis_GF->AddFrame(YAxisDivs_NEL = new ADAQNumberEntryWithLabel(YAxis_GF, "Divisions", YAxisDivs_NEL_ID),		
		     new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  YAxisDivs_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  YAxisDivs_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  YAxisDivs_NEL->GetEntry()->SetNumber(510);
  YAxisDivs_NEL->GetEntry()->Resize(50, 20);
  YAxisDivs_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  // Z-axis options

  TGGroupFrame *ZAxis_GF = new TGGroupFrame(GraphicsFrame_VF, "Z Axis", kVerticalFrame);
  GraphicsFrame_VF->AddFrame(ZAxis_GF, new TGLayoutHints(kLHintsNormal, 5,5,5,5));

  ZAxis_GF->AddFrame(ZAxisTitle_TEL = new ADAQTextEntryWithLabel(ZAxis_GF, "Title", ZAxisTitle_TEL_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,5,0));

  TGHorizontalFrame *ZAxisTitleOptions_HF = new TGHorizontalFrame(ZAxis_GF);
  ZAxis_GF->AddFrame(ZAxisTitleOptions_HF, new TGLayoutHints(kLHintsNormal, 0,5,0,5));

  ZAxisTitleOptions_HF->AddFrame(ZAxisSize_NEL = new ADAQNumberEntryWithLabel(ZAxisTitleOptions_HF, "Size", ZAxisSize_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  ZAxisSize_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  ZAxisSize_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  ZAxisSize_NEL->GetEntry()->SetNumber(0.04);
  ZAxisSize_NEL->GetEntry()->Resize(50, 20);
  ZAxisSize_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");
  
  ZAxisTitleOptions_HF->AddFrame(ZAxisOffset_NEL = new ADAQNumberEntryWithLabel(ZAxisTitleOptions_HF, "Offset", ZAxisOffset_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 5,2,0,0));
  ZAxisOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  ZAxisOffset_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  ZAxisOffset_NEL->GetEntry()->SetNumber(1.5);
  ZAxisOffset_NEL->GetEntry()->Resize(50, 20);
  ZAxisOffset_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  ZAxis_GF->AddFrame(ZAxisDivs_NEL = new ADAQNumberEntryWithLabel(ZAxis_GF, "Divisions", ZAxisDivs_NEL_ID),		
		     new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  ZAxisDivs_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  ZAxisDivs_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  ZAxisDivs_NEL->GetEntry()->SetNumber(510);
  ZAxisDivs_NEL->GetEntry()->Resize(50, 20);
  ZAxisDivs_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  // Color palette options

  TGGroupFrame *PaletteAxis_GF = new TGGroupFrame(GraphicsFrame_VF, "Palette Axis", kVerticalFrame);
  GraphicsFrame_VF->AddFrame(PaletteAxis_GF, new TGLayoutHints(kLHintsNormal, 5,5,5,5));

  PaletteAxis_GF->AddFrame(PaletteAxisTitle_TEL = new ADAQTextEntryWithLabel(PaletteAxis_GF, "Title", PaletteAxisTitle_TEL_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  
  // A horizontal frame for the Z-axis title/label size and offset
  TGHorizontalFrame *PaletteAxisTitleOptions_HF = new TGHorizontalFrame(PaletteAxis_GF);
  PaletteAxis_GF->AddFrame(PaletteAxisTitleOptions_HF, new TGLayoutHints(kLHintsNormal, 0,5,0,5));

  PaletteAxisTitleOptions_HF->AddFrame(PaletteAxisSize_NEL = new ADAQNumberEntryWithLabel(PaletteAxisTitleOptions_HF, "Size", PaletteAxisSize_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PaletteAxisSize_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PaletteAxisSize_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PaletteAxisSize_NEL->GetEntry()->SetNumber(0.04);
  PaletteAxisSize_NEL->GetEntry()->Resize(50, 20);
  PaletteAxisSize_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");
  
  PaletteAxisTitleOptions_HF->AddFrame(PaletteAxisOffset_NEL = new ADAQNumberEntryWithLabel(PaletteAxisTitleOptions_HF, "Offset", PaletteAxisOffset_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 5,2,0,0));
  PaletteAxisOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PaletteAxisOffset_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PaletteAxisOffset_NEL->GetEntry()->SetNumber(1.1);
  PaletteAxisOffset_NEL->GetEntry()->Resize(50, 20);
  PaletteAxisOffset_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");

  TGHorizontalFrame *PaletteAxisPositions_HF1 = new TGHorizontalFrame(PaletteAxis_GF);
  PaletteAxis_GF->AddFrame(PaletteAxisPositions_HF1, new TGLayoutHints(kLHintsNormal, 0,5,0,5));

  PaletteAxisPositions_HF1->AddFrame(PaletteX1_NEL = new ADAQNumberEntryWithLabel(PaletteAxisPositions_HF1, "X1", -1),
				    new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PaletteX1_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PaletteX1_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PaletteX1_NEL->GetEntry()->SetNumber(0.85);
  PaletteX1_NEL->GetEntry()->Resize(50, 20);

  PaletteAxisPositions_HF1->AddFrame(PaletteX2_NEL = new ADAQNumberEntryWithLabel(PaletteAxisPositions_HF1, "X2", -1),
				    new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PaletteX2_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PaletteX2_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PaletteX2_NEL->GetEntry()->SetNumber(0.90);
  PaletteX2_NEL->GetEntry()->Resize(50, 20);

  TGHorizontalFrame *PaletteAxisPositions_HF2 = new TGHorizontalFrame(PaletteAxis_GF);
  PaletteAxis_GF->AddFrame(PaletteAxisPositions_HF2, new TGLayoutHints(kLHintsNormal, 0,5,0,5));

  PaletteAxisPositions_HF2->AddFrame(PaletteY1_NEL = new ADAQNumberEntryWithLabel(PaletteAxisPositions_HF2, "Y1", -1),
				    new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PaletteY1_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PaletteY1_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PaletteY1_NEL->GetEntry()->SetNumber(0.05);
  PaletteY1_NEL->GetEntry()->Resize(50, 20);

  PaletteAxisPositions_HF2->AddFrame(PaletteY2_NEL = new ADAQNumberEntryWithLabel(PaletteAxisPositions_HF2, "Y2", -1),
				    new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PaletteY2_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PaletteY2_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PaletteY2_NEL->GetEntry()->SetNumber(0.65);
  PaletteY2_NEL->GetEntry()->Resize(50, 20);
}


void AAInterface::FillProcessingFrame()
{
  ///////////////////////////////////
  // Fill the processing widget frame
  
  TGCanvas *ProcessingFrame_C = new TGCanvas(ProcessingOptions_CF, 320, 705, kDoubleBorder);
  ProcessingOptions_CF->AddFrame(ProcessingFrame_C, new TGLayoutHints(kLHintsCenterX, 0,0,10,10));
  
  TGVerticalFrame *ProcessingFrame_VF = new TGVerticalFrame(ProcessingFrame_C->GetViewPort(), 320, 705);
  ProcessingFrame_C->SetContainer(ProcessingFrame_VF);
  
  // Processing type (sequential / parallel)

  ProcessingFrame_VF->AddFrame(ProcessingType_BG = new TGButtonGroup(ProcessingFrame_VF, "Waveform processing type", kHorizontalFrame),
			       new TGLayoutHints(kLHintsNormal, 15,5,5,5));
  ProcessingSeq_RB = new TGRadioButton(ProcessingType_BG, "Sequential    ", ProcessingSeq_RB_ID);
  ProcessingSeq_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  ProcessingSeq_RB->SetState(kButtonDown);
  
  ProcessingPar_RB = new TGRadioButton(ProcessingType_BG, "Parallel", ProcessingPar_RB_ID);
  ProcessingPar_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  //ProcessingPar_RB->SetState(kButtonDown);
  
  // Processing processing options
  
  TGGroupFrame *ProcessingOptions_GF = new TGGroupFrame(ProcessingFrame_VF, "Processing options", kVerticalFrame);
  ProcessingFrame_VF->AddFrame(ProcessingOptions_GF, new TGLayoutHints(kLHintsNormal, 15,5,5,5));
  
  ProcessingOptions_GF->AddFrame(NumProcessors_NEL = new ADAQNumberEntryWithLabel(ProcessingOptions_GF, "Number of Processors", -1),
				 new TGLayoutHints(kLHintsNormal, 0,0,5,0));
  NumProcessors_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  NumProcessors_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  NumProcessors_NEL->GetEntry()->SetLimitValues(1,NumProcessors);
  NumProcessors_NEL->GetEntry()->SetNumber(NumProcessors);
  
  ProcessingOptions_GF->AddFrame(UpdateFreq_NEL = new ADAQNumberEntryWithLabel(ProcessingOptions_GF, "Update freq (% done)", -1),
				 new TGLayoutHints(kLHintsNormal, 0,0,5,0));
  UpdateFreq_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  UpdateFreq_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  UpdateFreq_NEL->GetEntry()->SetLimitValues(1,100);
  UpdateFreq_NEL->GetEntry()->SetNumber(2);
  
  TGGroupFrame *PearsonAnalysis_GF = new TGGroupFrame(ProcessingFrame_VF, "Pearson (RFQ current) integration", kVerticalFrame);
  ProcessingFrame_VF->AddFrame(PearsonAnalysis_GF, new TGLayoutHints(kLHintsNormal, 15,5,0,0));
  
  PearsonAnalysis_GF->AddFrame(IntegratePearson_CB = new TGCheckButton(PearsonAnalysis_GF, "Integrate signal", IntegratePearson_CB_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  IntegratePearson_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");
  
  PearsonAnalysis_GF->AddFrame(PearsonChannel_CBL = new ADAQComboBoxWithLabel(PearsonAnalysis_GF, "Contains signal", -1),
			       new TGLayoutHints(kLHintsNormal, 0,5,5,0));

  stringstream ss;
  string entry;
  for(int ch=0; ch<NumDataChannels; ch++){
    ss << ch;
    entry.assign("Channel " + ss.str());
    PearsonChannel_CBL->GetComboBox()->AddEntry(entry.c_str(),ch);
    ss.str("");
  }
  PearsonChannel_CBL->GetComboBox()->Select(3);
  PearsonChannel_CBL->GetComboBox()->SetEnabled(false);

  TGHorizontalFrame *PearsonPolarity_HF = new TGHorizontalFrame(PearsonAnalysis_GF);
  PearsonAnalysis_GF->AddFrame(PearsonPolarity_HF, new TGLayoutHints(kLHintsNormal));

  PearsonPolarity_HF->AddFrame(PearsonPolarityPositive_RB = new TGRadioButton(PearsonPolarity_HF, "Positive", PearsonPolarityPositive_RB_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PearsonPolarityPositive_RB->SetState(kButtonDown);
  PearsonPolarityPositive_RB->SetState(kButtonDisabled);
  PearsonPolarityPositive_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");

  PearsonPolarity_HF->AddFrame(PearsonPolarityNegative_RB = new TGRadioButton(PearsonPolarity_HF, "Negative", PearsonPolarityNegative_RB_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PearsonPolarityNegative_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");

  TGHorizontalFrame *PearsonIntegrationType_HF = new TGHorizontalFrame(PearsonAnalysis_GF);
  PearsonAnalysis_GF->AddFrame(PearsonIntegrationType_HF, new TGLayoutHints(kLHintsNormal));

  PearsonIntegrationType_HF->AddFrame(IntegrateRawPearson_RB = new TGRadioButton(PearsonIntegrationType_HF, "Integrate raw", IntegrateRawPearson_RB_ID),
				      new TGLayoutHints(kLHintsLeft, 0,5,5,0));
  IntegrateRawPearson_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  IntegrateRawPearson_RB->SetState(kButtonDown);
  IntegrateRawPearson_RB->SetState(kButtonDisabled);

  PearsonIntegrationType_HF->AddFrame(IntegrateFitToPearson_RB = new TGRadioButton(PearsonIntegrationType_HF, "Integrate fit", IntegrateFitToPearson_RB_ID),
				      new TGLayoutHints(kLHintsLeft, 10,5,5,0));
  IntegrateFitToPearson_RB->Connect("Clicked()", "AAInterface", this, "HandleRadioButtons()");
  IntegrateFitToPearson_RB->SetState(kButtonDisabled);
  
  PearsonAnalysis_GF->AddFrame(PearsonLowerLimit_NEL = new ADAQNumberEntryWithLabel(PearsonAnalysis_GF, "Lower limit (sample)", PearsonLowerLimit_NEL_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PearsonLowerLimit_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PearsonLowerLimit_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  PearsonLowerLimit_NEL->GetEntry()->SetLimitValues(0,1); // Updated when ADAQ ROOT file loaded
  PearsonLowerLimit_NEL->GetEntry()->SetNumber(0); // Updated when ADAQ ROOT file loaded
  PearsonLowerLimit_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");
  PearsonLowerLimit_NEL->GetEntry()->SetState(false);
  
  PearsonAnalysis_GF->AddFrame(PearsonMiddleLimit_NEL = new ADAQNumberEntryWithLabel(PearsonAnalysis_GF, "Middle limit (sample)", PearsonMiddleLimit_NEL_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PearsonMiddleLimit_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PearsonMiddleLimit_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  PearsonMiddleLimit_NEL->GetEntry()->SetLimitValues(0,1); // Updated when ADAQ ROOT file loaded
  PearsonMiddleLimit_NEL->GetEntry()->SetNumber(0); // Updated when ADAQ ROOT file loaded
  PearsonMiddleLimit_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");
  PearsonMiddleLimit_NEL->GetEntry()->SetState(false);
  
  PearsonAnalysis_GF->AddFrame(PearsonUpperLimit_NEL = new ADAQNumberEntryWithLabel(PearsonAnalysis_GF, "Upper limit (sample))", PearsonUpperLimit_NEL_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PearsonUpperLimit_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PearsonUpperLimit_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  PearsonUpperLimit_NEL->GetEntry()->SetLimitValues(0,1); // Updated when ADAQ ROOT file loaded
  PearsonUpperLimit_NEL->GetEntry()->SetNumber(0); // Updated when ADAQ ROOT file loaded
  PearsonUpperLimit_NEL->GetEntry()->Connect("ValueSet(long)", "AAInterface", this, "HandleNumberEntries()");
  PearsonUpperLimit_NEL->GetEntry()->SetState(false);
  
  PearsonAnalysis_GF->AddFrame(PlotPearsonIntegration_CB = new TGCheckButton(PearsonAnalysis_GF, "Plot integration", PlotPearsonIntegration_CB_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  PlotPearsonIntegration_CB->SetState(kButtonDisabled);
  PlotPearsonIntegration_CB->Connect("Clicked()", "AAInterface", this, "HandleCheckButtons()");

  PearsonAnalysis_GF->AddFrame(DeuteronsInWaveform_NEFL = new ADAQNumberEntryFieldWithLabel(PearsonAnalysis_GF, "D+ in waveform", -1),
			       new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  DeuteronsInWaveform_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal, TGNumberFormat::kNEANonNegative);
  DeuteronsInWaveform_NEFL->GetEntry()->Resize(100,20);
  DeuteronsInWaveform_NEFL->GetEntry()->SetState(false);
  
  PearsonAnalysis_GF->AddFrame(DeuteronsInTotal_NEFL = new ADAQNumberEntryFieldWithLabel(PearsonAnalysis_GF, "D+ in total", -1),
			       new TGLayoutHints(kLHintsNormal, 0,5,0,5));
  DeuteronsInTotal_NEFL->GetEntry()->SetFormat(TGNumberFormat::kNESReal, TGNumberFormat::kNEANonNegative);
  DeuteronsInTotal_NEFL->GetEntry()->Resize(100,20);
  DeuteronsInTotal_NEFL->GetEntry()->SetState(false);
  
  // Despliced file creation options
  
  TGGroupFrame *WaveformDesplicer_GF = new TGGroupFrame(ProcessingFrame_VF, "Despliced file creation", kVerticalFrame);
  ProcessingFrame_VF->AddFrame(WaveformDesplicer_GF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  
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
  DesplicedFileSelection_TB->SetBackgroundColor(ColorMgr->Number2Pixel(18));
  DesplicedFileSelection_TB->ChangeOptions(DesplicedFileSelection_TB->GetOptions() | kFixedSize);
  DesplicedFileSelection_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");
  
  WaveformDesplicerName_HF->AddFrame(DesplicedFileName_TE = new TGTextEntry(WaveformDesplicerName_HF, "<No file currently selected>", -1),
				     new TGLayoutHints(kLHintsLeft, 5,0,5,5));
  DesplicedFileName_TE->Resize(180,25);
  DesplicedFileName_TE->SetAlignment(kTextRight);
  DesplicedFileName_TE->SetBackgroundColor(ColorMgr->Number2Pixel(18));
  DesplicedFileName_TE->ChangeOptions(DesplicedFileName_TE->GetOptions() | kFixedSize);
  
  WaveformDesplicer_GF->AddFrame(DesplicedFileCreation_TB = new TGTextButton(WaveformDesplicer_GF, "Create despliced file", DesplicedFileCreation_TB_ID),
				 new TGLayoutHints(kLHintsLeft, 0,0,5,5));
  DesplicedFileCreation_TB->Resize(200, 30);
  DesplicedFileCreation_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  DesplicedFileCreation_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  DesplicedFileCreation_TB->ChangeOptions(DesplicedFileCreation_TB->GetOptions() | kFixedSize);
  DesplicedFileCreation_TB->Connect("Clicked()", "AAInterface", this, "HandleTextButtons()");
}


void AAInterface::FillCanvasFrame()
{
  /////////////////////////////////////
  // Canvas and associated widget frame

  TGVerticalFrame *Canvas_VF = new TGVerticalFrame(OptionsAndCanvas_HF);
  Canvas_VF->SetBackgroundColor(ColorMgr->Number2Pixel(16));
  OptionsAndCanvas_HF->AddFrame(Canvas_VF, new TGLayoutHints(kLHintsNormal, 5,5,5,5));

  // ADAQ ROOT file name text entry
  Canvas_VF->AddFrame(FileName_TE = new TGTextEntry(Canvas_VF, "<No file currently selected>", -1),
		      new TGLayoutHints(kLHintsTop, 30,5,0,5));
  FileName_TE->Resize(700,20);
  FileName_TE->SetState(false);
  FileName_TE->SetAlignment(kTextCenterX);
  FileName_TE->SetBackgroundColor(ColorMgr->Number2Pixel(18));
  FileName_TE->ChangeOptions(FileName_TE->GetOptions() | kFixedSize);

  // File statistics horizontal frame

  TGHorizontalFrame *FileStats_HF = new TGHorizontalFrame(Canvas_VF,700,30);
  FileStats_HF->SetBackgroundColor(ColorMgr->Number2Pixel(16));
  Canvas_VF->AddFrame(FileStats_HF, new TGLayoutHints(kLHintsTop | kLHintsCenterX, 5,5,0,5));

  // Waveforms

  TGHorizontalFrame *WaveformNELAndLabel_HF = new TGHorizontalFrame(FileStats_HF);
  WaveformNELAndLabel_HF->SetBackgroundColor(ColorMgr->Number2Pixel(16));
  FileStats_HF->AddFrame(WaveformNELAndLabel_HF, new TGLayoutHints(kLHintsLeft, 5,35,0,0));

  WaveformNELAndLabel_HF->AddFrame(Waveforms_NEL = new TGNumberEntryField(WaveformNELAndLabel_HF,-1),
				  new TGLayoutHints(kLHintsLeft,5,5,0,0));
  Waveforms_NEL->SetBackgroundColor(ColorMgr->Number2Pixel(18));
  Waveforms_NEL->SetAlignment(kTextCenterX);
  Waveforms_NEL->SetState(false);
  
  WaveformNELAndLabel_HF->AddFrame(WaveformLabel_L = new TGLabel(WaveformNELAndLabel_HF, " Waveforms "),
				   new TGLayoutHints(kLHintsLeft,0,5,3,0));
  WaveformLabel_L->SetBackgroundColor(ColorMgr->Number2Pixel(18));

  // Record length

  TGHorizontalFrame *RecordLengthNELAndLabel_HF = new TGHorizontalFrame(FileStats_HF);
  RecordLengthNELAndLabel_HF->SetBackgroundColor(ColorMgr->Number2Pixel(16));
  FileStats_HF->AddFrame(RecordLengthNELAndLabel_HF, new TGLayoutHints(kLHintsLeft, 5,5,0,0));

  RecordLengthNELAndLabel_HF->AddFrame(RecordLength_NEL = new TGNumberEntryField(RecordLengthNELAndLabel_HF,-1),
				      new TGLayoutHints(kLHintsLeft,5,5,0,0));
  RecordLength_NEL->SetBackgroundColor(ColorMgr->Number2Pixel(18));
  RecordLength_NEL->SetAlignment(kTextCenterX);
  RecordLength_NEL->SetState(false);

  RecordLengthNELAndLabel_HF->AddFrame(RecordLengthLabel_L = new TGLabel(RecordLengthNELAndLabel_HF, " Record length "),
				       new TGLayoutHints(kLHintsLeft,0,5,3,0));
  RecordLengthLabel_L->SetBackgroundColor(ColorMgr->Number2Pixel(18));

  // The embedded canvas

  TGHorizontalFrame *Canvas_HF = new TGHorizontalFrame(Canvas_VF);
  Canvas_VF->AddFrame(Canvas_HF, new TGLayoutHints(kLHintsCenterX));

  Canvas_HF->AddFrame(YAxisLimits_DVS = new TGDoubleVSlider(Canvas_HF, 500, kDoubleScaleBoth, YAxisLimits_DVS_ID),
		      new TGLayoutHints(kLHintsTop));
  YAxisLimits_DVS->SetRange(0,1);
  YAxisLimits_DVS->SetPosition(0,1);
  YAxisLimits_DVS->Connect("PositionChanged()", "AAInterface", this, "HandleDoubleSliders()");
  
  Canvas_HF->AddFrame(Canvas_EC = new TRootEmbeddedCanvas("TheCanvas_EC", Canvas_HF, 700, 500),
		      new TGLayoutHints(kLHintsCenterX, 5,5,5,5));
  Canvas_EC->GetCanvas()->SetFillColor(0);
  Canvas_EC->GetCanvas()->SetFrameFillColor(19);
  Canvas_EC->GetCanvas()->SetGrid();
  Canvas_EC->GetCanvas()->SetBorderMode(0);
  Canvas_EC->GetCanvas()->SetLeftMargin(0.13);
  Canvas_EC->GetCanvas()->SetBottomMargin(0.12);
  Canvas_EC->GetCanvas()->SetRightMargin(0.05);
  Canvas_EC->GetCanvas()->Connect("ProcessedEvent(int, int, int, TObject *)", "AAInterface", this, "HandleCanvas(int, int, int, TObject *)");

  Canvas_VF->AddFrame(XAxisLimits_THS = new TGTripleHSlider(Canvas_VF, 700, kDoubleScaleBoth, XAxisLimits_THS_ID),
		      new TGLayoutHints(kLHintsRight));
  XAxisLimits_THS->SetRange(0,1);
  XAxisLimits_THS->SetPosition(0,1);
  XAxisLimits_THS->SetPointerPosition(0.5);
  XAxisLimits_THS->Connect("PositionChanged()", "AAInterface", this, "HandleDoubleSliders()");
  XAxisLimits_THS->Connect("PointerPositionChanged()", "AAInterface", this, "HandleTripleSliderPointer()");
    
  Canvas_VF->AddFrame(new TGLabel(Canvas_VF, "  Waveform selector  "),
		      new TGLayoutHints(kLHintsCenterX, 5,5,15,0));

  Canvas_VF->AddFrame(WaveformSelector_HS = new TGHSlider(Canvas_VF, 700, kSlider1),
		      new TGLayoutHints(kLHintsRight));
  WaveformSelector_HS->SetRange(1,100);
  WaveformSelector_HS->SetPosition(1);
  WaveformSelector_HS->Connect("PositionChanged(int)", "AAInterface", this, "HandleSliders(int)");

  Canvas_VF->AddFrame(new TGLabel(Canvas_VF, "  Spectrum integration limits  "),
		      new TGLayoutHints(kLHintsCenterX, 5,5,15,0));

  Canvas_VF->AddFrame(SpectrumIntegrationLimits_DHS = new TGDoubleHSlider(Canvas_VF, 700, kDoubleScaleBoth, SpectrumIntegrationLimits_DHS_ID),
		      new TGLayoutHints(kLHintsRight));
  SpectrumIntegrationLimits_DHS->SetRange(0,1);
  SpectrumIntegrationLimits_DHS->SetPosition(0,1);
  SpectrumIntegrationLimits_DHS->Connect("PositionChanged()", "AAInterface", this, "HandleDoubleSliders()");
		      
  TGHorizontalFrame *SubCanvas_HF = new TGHorizontalFrame(Canvas_VF);
  SubCanvas_HF->SetBackgroundColor(ColorMgr->Number2Pixel(16));
  Canvas_VF->AddFrame(SubCanvas_HF, new TGLayoutHints(kLHintsRight | kLHintsBottom, 5,5,20,5));
  
  SubCanvas_HF->AddFrame(ProcessingProgress_PB = new TGHProgressBar(SubCanvas_HF, TGProgressBar::kFancy, 400),
			 new TGLayoutHints(kLHintsLeft, 5,55,7,5));
  ProcessingProgress_PB->SetBarColor(ColorMgr->Number2Pixel(41));
  ProcessingProgress_PB->ShowPosition(kTRUE, kFALSE, "%0.f% waveforms processed");
  
  SubCanvas_HF->AddFrame(Quit_TB = new TGTextButton(SubCanvas_HF, "Access standby", Quit_TB_ID),
			 new TGLayoutHints(kLHintsRight, 5,5,0,5));
  Quit_TB->Resize(200, 40);
  Quit_TB->SetBackgroundColor(ColorMgr->Number2Pixel(36));
  Quit_TB->SetForegroundColor(ColorMgr->Number2Pixel(0));
  Quit_TB->ChangeOptions(Quit_TB->GetOptions() | kFixedSize);
  Quit_TB->Connect("Clicked()", "AAInterface", this, "HandleTerminate()");
}


void AAInterface::HandleMenu(int MenuID)
{
  switch(MenuID){
    
    // Action that enables the user to select a ROOT file with
    // waveforms using the nicely prebuilt ROOT interface for file
    // selection. Only ROOT files are displayed in the dialog.
  case MenuFileOpen_ID:{
    const char *FileTypes[] = {"ADAQ-formatted ROOT file", "*.root",
			       "All files",                 "*",
			       0,                           0};
    
    // Use the ROOT prebuilt file dialog machinery
    TGFileInfo FileInformation;
    FileInformation.fFileTypes = FileTypes;
    FileInformation.fIniDir = StrDup(DataDirectory.c_str());
    new TGFileDialog(gClient->GetRoot(), this, kFDOpen, &FileInformation);
    
    // If the selected file is not found...
    if(FileInformation.fFilename==NULL)
      CreateMessageBox("No valid ADAQ ROOT file was selected so there's nothing to load!\nPlease select a valid file!","Stop");
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
	DataDirectory  = FileName.substr(0,pos);
      
      // Load the ROOT file and set the bool depending on outcome
      ADAQFileLoaded = ComputationMgr->LoadADAQRootFile(FileName);
      ADAQFileName = FileName;
      
      if(ADAQFileLoaded)
	UpdateForNewFile(FileName);
      else
	CreateMessageBox("The ADAQ ROOT file that you specified fail to load for some reason!\n","Stop");
    }
    break;
  }
    
  case MenuFileSaveSpectrum_ID:
  case MenuFileSaveSpectrumDerivative_ID:
  case MenuFileSavePSDHistogramSlice_ID:{

    // Create character arrays that enable file type selection (.dat
    // files have data columns separated by spaces and .csv have data
    // columns separated by commas)
    const char *FileTypes[] = {"ASCII file",  "*.dat",
			       "CSV file",    "*.csv",
			       "ROOT file",   "*.root",
			       0,             0};
  
    TGFileInfo FileInformation;
    FileInformation.fFileTypes = FileTypes;
    FileInformation.fIniDir = StrDup(HistogramDirectory.c_str());
    
    new TGFileDialog(gClient->GetRoot(), this, kFDSave, &FileInformation);
    
    if(FileInformation.fFilename==NULL)
      CreateMessageBox("No file was selected! Nothing will be saved to file!","Stop");
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
	HistogramDirectory  = FileName.substr(0,pos);

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
      
      if(MenuID == MenuFileSaveSpectrum_ID){
	if(!ComputationMgr->GetSpectrumExists()){
	  CreateMessageBox("No spectra have been created yet and, therefore, there is nothing to save!","Stop");
	  break;
	}
	else
	  Success = ComputationMgr->SaveHistogramData("Spectrum", FileName, FileExtension);
      }
      
      else if(MenuID == MenuFileSaveSpectrumDerivative_ID){
	if(!ComputationMgr->GetSpectrumDerivativeExists()){
	  CreateMessageBox("No spectrum derivatives have been created yet and, therefore, there is nothing to save!","Stop");
	  break;
	}
	else
	  Success = ComputationMgr->SaveHistogramData("SpectrumDerivative", FileName, FileExtension);
      }
      
      else if (MenuID == MenuFileSavePSDHistogramSlice_ID){
	CreateMessageBox("Saving the PSD histogram slice is not yet implemented!","Stop");
	Success = false;	
      }

      if(Success){
	if(FileExtension == ".dat")
	  CreateMessageBox("The histogram was successfully saved to the .dat file","Asterisk");
	else if(FileExtension == ".csv")
	  CreateMessageBox("The histogram was successfully saved to the .csv file","Asterisk");
	else if(FileExtension == ".root")
	  CreateMessageBox("The histogram name 'Spectrum' was successfully saved to the .root file","Asterisk");
      }
      else
	CreateMessageBox("The histogram failed to save to the file for unknown reasons!","Stop");
    }
    break;
  }
    
    // Acition that enables the user to print the currently displayed
    // canvas to a file of the user's choice. But really, it's not a
    // choice. Vector-based graphics are the only way to go. Do
    // everyone a favor, use .eps or .ps or .pdf if you want to be a
    // serious scientist.
  case MenuFilePrint_ID:{
    
    // List the available graphical file options
    const char *FileTypes[] = {"EPS file",          "*.eps",
			       "PS file",           "*.ps",
			       "PDF file",          "*.pdf",
			       "PNG file",          "*.png",
			       "JPG file",          "*.jpeg",
			       0,                  0 };

    // Use the ROOT prebuilt file dialog machinery
    TGFileInfo FileInformation;
    FileInformation.fFileTypes = FileTypes;
    FileInformation.fIniDir = StrDup(PrintDirectory.c_str());

    new TGFileDialog(gClient->GetRoot(), this, kFDSave, &FileInformation);
    
    if(FileInformation.fFilename==NULL)
      CreateMessageBox("No file was selected to the canvas graphics will not be saved!\nSelect a valid file to save the canvas graphis!","Stop");
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
	PrintDirectory  = GraphicFileName.substr(0,Found);
      
      Found = GraphicFileName.find_last_of(".");
      if(Found != string::npos)
	GraphicFileName = GraphicFileName.substr(0, Found);
      
      GraphicFileExtension = FileInformation.fFileTypes[FileInformation.fFileTypeIdx+1];
      Found = GraphicFileExtension.find_last_of("*");
      if(Found != string::npos)
	GraphicFileExtension = GraphicFileExtension.substr(Found+1, GraphicFileExtension.size());
      
      string GraphicFile = GraphicFileName+GraphicFileExtension;
      
      Canvas_EC->GetCanvas()->Update();
      Canvas_EC->GetCanvas()->Print(GraphicFile.c_str(), GraphicFileExtension.c_str());
      
      string SuccessMessage = "The canvas graphics have been successfully saved to the following file:\n" + GraphicFileName + GraphicFileExtension;
      CreateMessageBox(SuccessMessage,"Asterisk");
    }
    break;
  }
    
    // Action that enables the Quit_TB and File->Exit selections to
    // quit the ROOT application
  case MenuFileExit_ID:
    HandleTerminate();
    
  default:
    break;
  }
}


void AAInterface::HandleTextButtons()
{
  if(!ADAQFileLoaded)
    return;
  
  // Get the currently active widget and its ID, i.e. the widget from
  // which the user has just sent a signal...
  TGTextButton *ActiveTextButton = (TGTextButton *) gTQSender;
  int ActiveTextButtonID  = ActiveTextButton->WidgetId();

  SaveSettings();

  switch(ActiveTextButtonID){

  case Quit_TB_ID:
    HandleTerminate();
    break;

  case ResetXAxisLimits_TB_ID:

    // Reset the XAxis double slider and member data
    //    XAxisLimits_THS->SetPosition(0, RecordLength);
    //    XAxisMin = 0;
    //    XAxisMax = RecordLength;
    
    GraphicsMgr->PlotWaveform();
    break;

  case ResetYAxisLimits_TB_ID:    
    // Reset the YAxis double slider and member data
    //    YAxisLimits_DVS->SetPosition(0, V1720MaximumBit);
    //    YAxisMin = 0;
    //    YAxisMax = V1720MaximumBit;

    GraphicsMgr->PlotWaveform();
    break;


    //////////////////////////////////////////
    // Actions for setting a calibration point

  case SpectrumCalibrationSetPoint_TB_ID:{

    // Get the calibration point to be set
    uint SetPoint = SpectrumCalibrationPoint_CBL->GetComboBox()->GetSelected();

    // Get the energy of the present calibration point
    double Energy = SpectrumCalibrationEnergy_NEL->GetEntry()->GetNumber();
   
    // Get the pulse unit value of the present calibration point
    double PulseUnit = SpectrumCalibrationPulseUnit_NEL->GetEntry()->GetNumber();
    
    // Get the current channel being histogrammed in DGScope
    int Channel = ChannelSelector_CBL->GetComboBox()->GetSelected();

    bool NewPoint = ComputationMgr->SetCalibrationPoint(Channel, SetPoint, Energy, PulseUnit);
    
    if(NewPoint){
      // Add a new point to the number of calibration points in case
      // the user wants to add subsequent points to the calibration
      stringstream ss;
      ss << (SetPoint+1);
      string NewPointLabel = "Point " + ss.str();
      SpectrumCalibrationPoint_CBL->GetComboBox()->AddEntry(NewPointLabel.c_str(),SetPoint+1);
      
      // Set the combo box to display the new calibration point...
      SpectrumCalibrationPoint_CBL->GetComboBox()->Select(SetPoint+1);
      
      // ...and set the calibration energy and pulse unit ROOT number
      // entry widgets to their default "0.0" and "1.0" respectively,
      SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(0.0);
      SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(1.0);
    }
    break;
  }

    //////////////////////////////////////
    // Actions for setting the calibration
    
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
    int Channel = ChannelSelector_CBL->GetComboBox()->GetSelected();

    bool Success = ComputationMgr->SetCalibration(Channel);

    if(!Success)
      CreateMessageBox("The calibration could not be set!","Stop");

    break;
  }

    /////////////////////////////////////////////////////
    // Actions for plotting the CalibrationManager TGraph
    
  case SpectrumCalibrationPlot_TB_ID:{
    
    int Channel = ChannelSelector_CBL->GetComboBox()->GetSelected();
    
    GraphicsMgr->PlotCalibration(Channel);

    break;
  }

    ///////////////////////////////////////////
    // Actions for resetting CalibrationManager
    
  case SpectrumCalibrationReset_TB_ID:{
    
    // Get the current channel being histogrammed in 
    int Channel = ChannelSelector_CBL->GetComboBox()->GetSelected();
    
    bool Success = ComputationMgr->ClearCalibration(Channel);

    // Reset the calibration widgets
    if(Success){
      SpectrumCalibrationPoint_CBL->GetComboBox()->RemoveAll();
      SpectrumCalibrationPoint_CBL->GetComboBox()->AddEntry("Point 0", 0);
      SpectrumCalibrationPoint_CBL->GetComboBox()->Select(0);
      SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(0.0);
      SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(1.0);
    }
    break;
  }

    /////////////////////////////////
    // Actions to replot the spectrum

  case ReplotWaveform_TB_ID:
    GraphicsMgr->PlotWaveform();
    break;

    /////////////////////////////////
    // Actions to replot the spectrum
    
  case ReplotSpectrum_TB_ID:
    
    if(ComputationMgr->GetSpectrumExists())
      GraphicsMgr->PlotSpectrum();
    else
      CreateMessageBox("A valid spectrum does not yet exist; therefore, it is difficult to replot it!","Stop");
    break;
    
    ////////////////////////////////////////////
    // Actions to replot the spectrum derivative
    
  case ReplotSpectrumDerivative_TB_ID:
    
    if(ComputationMgr->GetSpectrumExists()){
      SpectrumOverplotDerivative_CB->SetState(kButtonUp);
      GraphicsMgr->PlotSpectrumDerivative();
    }
    else
      CreateMessageBox("A valid spectrum does not yet exist; therefore, the spectrum derivative cannot be plotted!","Stop");
    break;
      
    //////////////////////////////////////
    // Actions to replot the PSD histogram
    
  case ReplotPSDHistogram_TB_ID:
    
    if(ComputationMgr->GetPSDHistogramExists())
      GraphicsMgr->PlotPSDHistogram();
    else
      CreateMessageBox("A valid PSD histogram does not yet exist; therefore, replotting cannot be achieved!","Stop");
    break;
    
  case CreateSpectrum_TB_ID:

    // Alert the user the filtering particles by PSD into the spectra
    // requires integration type peak finder to be used
    //if(UsePSDFilterManager[PSDChannel] and IntegrationTypeWholeWaveform)
    //      CreateMessageBox("Warning! Use of the PSD filter with spectra creation requires peak finding integration","Asterisk");
    
    // Create spectrum with sequential processing
    if(ProcessingSeq_RB->IsDown())
      ComputationMgr->CreateSpectrum();
    
    // Create spectrum with parallel processing
    else{
      SaveSettings(true);
      ComputationMgr->ProcessWaveformsInParallel("histogramming");
    }

    GraphicsMgr->PlotSpectrum();
    break;
    
  case CountRate_TB_ID:
    //CalculateCountRate();
    break;
    
  case PSDCalculate_TB_ID:
    if(ProcessingSeq_RB->IsDown())
      ComputationMgr->CreatePSDHistogram();
    else{
      SaveSettings(true);
      ComputationMgr->ProcessWaveformsInParallel("discriminating");
    }
    GraphicsMgr->PlotPSDHistogram();
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
    size_t Pos = ADAQFileName.find_last_of("/");
    if(Pos != string::npos){
      string RawFileName = ADAQFileName.substr(Pos+1, ADAQFileName.size());

      Pos = RawFileName.find_last_of(".");
      if(Pos != string::npos)
	InitialFileName = RawFileName.substr(0,Pos) + ".ds.root";
    }

    TGFileInfo FileInformation;
    FileInformation.fFileTypes = FileTypes;
    FileInformation.fFilename = StrDup(InitialFileName.c_str());
    FileInformation.fIniDir = StrDup(DesplicedDirectory.c_str());
    new TGFileDialog(gClient->GetRoot(), this, kFDOpen, &FileInformation);
    
    if(FileInformation.fFilename==NULL)
      CreateMessageBox("A file was not selected so the data will not be saved!\nSelect a valid file to save the despliced waveforms","Stop");
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
	DesplicedDirectory = DesplicedFileName.substr(0, Found);
      
      Found = DesplicedFileName.find_last_of(".");
      if(Found != string::npos)
	DesplicedFileName = DesplicedFileName.substr(0, Found);
     
      DesplicedFileExtension = FileInformation.fFileTypes[FileInformation.fFileTypeIdx+1];
      Found = DesplicedFileExtension.find_last_of("*");
      if(Found != string::npos)
	DesplicedFileExtension = DesplicedFileExtension.substr(Found+1, DesplicedFileExtension.size());

      string DesplicedFile = DesplicedFileName + DesplicedFileExtension;

      DesplicedFileName_TE->SetText(DesplicedFile.c_str());
    }
    break;
  }
    
  case DesplicedFileCreation_TB_ID:
    
    
    // Alert the user the filtering particles by PSD into the spectra
    // requires integration type peak finder to be used
    //if(UsePSDFilterManager[PSDChannel] and IntegrationTypeWholeWaveform)
      CreateMessageBox("Warning! Use of the PSD filter with spectra creation requires peak finding integration","Asterisk");
    
    // Desplice waveforms sequentially
    if(ProcessingSeq_RB->IsDown())
      {}//CreateDesplicedFile();

    // Desplice waveforms in parallel
    else
      {}//ProcessWaveformsInParallel("desplicing");

    break;

    
  case PSDClearFilter_TB_ID:
    ComputationMgr->ClearPSDFilter(ChannelSelector_CBL->GetComboBox()->GetSelected());
    PSDEnableFilter_CB->SetState(kButtonUp);

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

    break;
  }
}


void AAInterface::HandleCheckButtons()
{
  // Get the currently active widget and its ID, i.e. the widget from
  // which the user has just sent a signal...
  TGCheckButton *ActiveCheckButton = (TGCheckButton *) gTQSender;
  int CheckButtonID = ActiveCheckButton->WidgetId();
  
  if(!ADAQFileLoaded)
    return;

  SaveSettings();
  
  switch(CheckButtonID){
    
  case FindPeaks_CB_ID:
    GraphicsMgr->PlotWaveform();
    break;

  case PlotZeroSuppressionCeiling_CB_ID:
  case PlotFloor_CB_ID:
  case PlotCrossings_CB_ID:
  case PlotPeakIntegratingRegion_CB_ID:
  case PlotTrigger_CB_ID:
  case PlotBaseline_CB_ID:
  case UsePileupRejection_CB_ID:
  case PlotPearsonIntegration_CB_ID:
    GraphicsMgr->PlotWaveform();
    break;

  case OverrideTitles_CB_ID:
  case PlotVerticalAxisInLog_CB_ID:
  case SetStatsOff_CB_ID:
    
    switch(GraphicsMgr->GetCanvasContentType()){
    case zWaveform:
      GraphicsMgr->PlotWaveform();
      break;
      
    case zSpectrum:{
      GraphicsMgr->PlotSpectrum();
      if(SpectrumOverplotDerivative_CB->IsDown())
	GraphicsMgr->PlotSpectrumDerivative();
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
    
  case SpectrumCalibration_CB_ID:{
    
    if(SpectrumCalibration_CB->IsDown()){
      SpectrumCalibrationManual_RB->SetState(kButtonUp);
      SpectrumCalibrationFixedEP_RB->SetState(kButtonUp);

      SetCalibrationWidgetState(true, kButtonUp);
      
      HandleTripleSliderPointer();
    }
    else{
      SpectrumCalibrationManual_RB->SetState(kButtonDisabled);
      SpectrumCalibrationFixedEP_RB->SetState(kButtonDisabled);

      SetCalibrationWidgetState(false, kButtonDisabled);
    }
    break;
  }
    
  case SpectrumFindPeaks_CB_ID:
    if(!ComputationMgr->GetSpectrumExists())
      break;
    
    if(SpectrumFindPeaks_CB->IsDown()){
      SpectrumNumPeaks_NEL->GetEntry()->SetState(true);
      SpectrumSigma_NEL->GetEntry()->SetState(true);
      SpectrumResolution_NEL->GetEntry()->SetState(true);

      //FindSpectrumPeaks();
    }
    else{
      SpectrumNumPeaks_NEL->GetEntry()->SetState(false);
      SpectrumSigma_NEL->GetEntry()->SetState(false);
      SpectrumResolution_NEL->GetEntry()->SetState(false);

      GraphicsMgr->PlotSpectrum();
    }
    break;
    
  case SpectrumFindBackground_CB_ID:
    if(!ComputationMgr->GetSpectrumExists())
      break;
    
    if(SpectrumFindBackground_CB->IsDown()){
      SpectrumRangeMin_NEL->GetEntry()->SetState(true);
      SpectrumRangeMax_NEL->GetEntry()->SetState(true);
      SpectrumWithBackground_RB->SetState(kButtonUp);
      SpectrumLessBackground_RB->SetState(kButtonUp);
      
      ComputationMgr->CalculateSpectrumBackground();
      GraphicsMgr->PlotSpectrum();
    }
    else{
      SpectrumRangeMin_NEL->GetEntry()->SetState(false);
      SpectrumRangeMax_NEL->GetEntry()->SetState(false);
      SpectrumWithBackground_RB->SetState(kButtonDisabled);
      SpectrumLessBackground_RB->SetState(kButtonDisabled);
      
      GraphicsMgr->PlotSpectrum();
    }
    break;

  case SpectrumFindIntegral_CB_ID:
  case SpectrumIntegralInCounts_CB_ID:
  case SpectrumUseGaussianFit_CB_ID:
  case SpectrumNormalizePeakToCurrent_CB_ID:
    
    if(SpectrumFindIntegral_CB->IsDown()){
      ComputationMgr->IntegrateSpectrum();
      GraphicsMgr->PlotSpectrum();
      
      SpectrumIntegral_NEFL->GetEntry()->SetNumber( ComputationMgr->GetSpectrumIntegralValue() );
      SpectrumIntegralError_NEFL->GetEntry()->SetNumber (ComputationMgr->GetSpectrumIntegralError() );
    }
    else
      GraphicsMgr->PlotSpectrum();
    break;

  case IntegratePearson_CB_ID:{
    EButtonState ButtonState = kButtonDisabled;
    bool WidgetState = false;
    
    if(IntegratePearson_CB->IsDown()){
      ButtonState = kButtonUp;
      WidgetState = true;
    }
    
    PlotPearsonIntegration_CB->SetState(ButtonState);
    PearsonChannel_CBL->GetComboBox()->SetEnabled(WidgetState);
    PearsonPolarityPositive_RB->SetState(ButtonState);
    PearsonPolarityNegative_RB->SetState(ButtonState);
    IntegrateRawPearson_RB->SetState(ButtonState);
    IntegrateFitToPearson_RB->SetState(ButtonState);
    PearsonLowerLimit_NEL->GetEntry()->SetState(WidgetState);
    if(WidgetState==true and IntegrateFitToPearson_RB->IsDown())
      PearsonMiddleLimit_NEL->GetEntry()->SetState(WidgetState);
    PearsonUpperLimit_NEL->GetEntry()->SetState(WidgetState);
    
    if(!IntegratePearson_CB->IsDown() or
       (IntegratePearson_CB->IsDown() and PlotPearsonIntegration_CB->IsDown()))
      GraphicsMgr->PlotWaveform();
    
    break;
  }
    
  case PSDEnable_CB_ID:{
    EButtonState ButtonState = kButtonDisabled;
    bool WidgetState = false;
    
    if(PSDEnable_CB->IsDown()){
      ButtonState = kButtonUp;
      WidgetState = true;
    }

    // Be sure to turn off PSD filtering if the user does not want to
    // discriminate by pulse shape
    else{
      PSDEnableFilter_CB->SetState(kButtonUp);
      if(GraphicsMgr->GetCanvasContentType() == zWaveform)
	GraphicsMgr->PlotWaveform();
    }

    PSDChannel_CBL->GetComboBox()->SetEnabled(WidgetState);
    PSDWaveforms_NEL->GetEntry()->SetState(WidgetState);
    PSDNumTailBins_NEL->GetEntry()->SetState(WidgetState);
    PSDMinTailBin_NEL->GetEntry()->SetState(WidgetState);
    PSDMaxTailBin_NEL->GetEntry()->SetState(WidgetState);
    PSDNumTotalBins_NEL->GetEntry()->SetState(WidgetState);
    PSDMinTotalBin_NEL->GetEntry()->SetState(WidgetState);
    PSDMaxTotalBin_NEL->GetEntry()->SetState(WidgetState);
    PSDThreshold_NEL->GetEntry()->SetState(WidgetState);
    PSDPeakOffset_NEL->GetEntry()->SetState(WidgetState);
    PSDTailOffset_NEL->GetEntry()->SetState(WidgetState);
    PSDPlotType_CBL->GetComboBox()->SetEnabled(WidgetState);
    PSDPlotTailIntegration_CB->SetState(ButtonState);
    PSDEnableFilterCreation_CB->SetState(ButtonState);
    PSDEnableFilter_CB->SetState(ButtonState);
    PSDPositiveFilter_RB->SetState(ButtonState);
    PSDNegativeFilter_RB->SetState(ButtonState);
    PSDClearFilter_TB->SetState(ButtonState);
    PSDCalculate_TB->SetState(ButtonState);
    break;
  }

  case PSDEnableFilterCreation_CB_ID:{

    if(PSDEnableFilterCreation_CB->IsDown() and GraphicsMgr->GetCanvasContentType() != zPSDHistogram){
      CreateMessageBox("The canvas does not presently contain a PSD histogram! PSD filter creation is not possible!","Stop");
      PSDEnableFilterCreation_CB->SetState(kButtonUp);
      break;
    }
    break;
  }

  case PSDEnableFilter_CB_ID:{
    if(PSDEnableFilter_CB->IsDown()){
      //UsePSDFilterManager[PSDChannel] = true;
      FindPeaks_CB->SetState(kButtonDown);
    }
    else
      {}//UsePSDFilterManager[PSDChannel] = false;
    break;
  }
    
  case PSDPlotTailIntegration_CB_ID:{
    if(!FindPeaks_CB->IsDown())
      {}
    //FindPeaks_CB->SetState(kButtonDown);
    GraphicsMgr->PlotWaveform();
    break;
  }

  case PSDEnableHistogramSlicing_CB_ID:{
    if(PSDEnableHistogramSlicing_CB->IsDown()){
      PSDHistogramSliceX_RB->SetState(kButtonUp);
      PSDHistogramSliceY_RB->SetState(kButtonUp);
      
      // Temporary hack ZSH 12 Feb 13
      PSDHistogramSliceX_RB->SetState(kButtonDown);
    }
    else{
      // Disable histogram buttons
      PSDHistogramSliceX_RB->SetState(kButtonDisabled);
      PSDHistogramSliceY_RB->SetState(kButtonDisabled);

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

  case PlotSpectrumDerivativeError_CB_ID:
  case PlotAbsValueSpectrumDerivative_CB_ID:
  case SpectrumOverplotDerivative_CB_ID:{
    if(!ComputationMgr->GetSpectrumExists()){
      CreateMessageBox("A valid spectrum does not yet exists! The calculation of a spectrum derivative is, therefore, moot!", "Stop");
      break;
    }
    else{
      if(SpectrumOverplotDerivative_CB->IsDown()){
	GraphicsMgr->PlotSpectrum();
	GraphicsMgr->PlotSpectrumDerivative();
      }
      else if(PlotSpectrumDerivativeError_CB->IsDown()){
	GraphicsMgr->PlotSpectrumDerivative();
      }
      else{
	GraphicsMgr->PlotSpectrum();
      }
      break;
    }
  }

  default:
    break;
  }
}


void AAInterface::HandleSliders(int SliderPosition)
{
  if(!ADAQFileLoaded)
    return;

  SaveSettings();
  
  WaveformSelector_NEL->GetEntry()->SetNumber(SliderPosition);
  
  GraphicsMgr->PlotWaveform();
}


void AAInterface::HandleDoubleSliders()
{
  if(!ADAQFileLoaded)
    return;
  
  SaveSettings();
  
  // Get the currently active widget and its ID, i.e. the widget from
  // which the user has just sent a signal...
  TGDoubleSlider *ActiveDoubleSlider = (TGDoubleSlider *) gTQSender;
  int SliderID = ActiveDoubleSlider->WidgetId();

  switch(SliderID){
    
  case XAxisLimits_THS_ID:
  case YAxisLimits_DVS_ID:
    if(GraphicsMgr->GetCanvasContentType() == zWaveform)
      GraphicsMgr->PlotWaveform();

    else if(GraphicsMgr->GetCanvasContentType() == zSpectrum and ComputationMgr->GetSpectrumExists()){
      GraphicsMgr->PlotSpectrum();
      if(SpectrumOverplotDerivative_CB->IsDown())
	GraphicsMgr->PlotSpectrumDerivative();
    }
    
    else if(GraphicsMgr->GetCanvasContentType() == zSpectrumDerivative and ComputationMgr->GetSpectrumExists())
      GraphicsMgr->PlotSpectrumDerivative();
    
    else if(GraphicsMgr->GetCanvasContentType() == zPSDHistogram and ComputationMgr->GetSpectrumExists())
      GraphicsMgr->PlotPSDHistogram();
    
    break;

  case SpectrumIntegrationLimits_DHS_ID:
    ComputationMgr->IntegrateSpectrum();
    GraphicsMgr->PlotSpectrum();
    
    SpectrumIntegral_NEFL->GetEntry()->SetNumber( ComputationMgr->GetSpectrumIntegralValue() );
    SpectrumIntegralError_NEFL->GetEntry()->SetNumber (ComputationMgr->GetSpectrumIntegralError () );
    
    break;
  }
}


void AAInterface::HandleTripleSliderPointer()
{
  // To enable easy spectra calibration, the user has the options of
  // dragging the pointer of the X-axis horizontal slider just below
  // the canvas, which results in a "calibration line" drawn over the
  // plotted pulse spectrum. As the user drags the line, the pulse
  // unit number entry widget in the calibration group frame is
  // update, allowing the user to easily set the value of pulse units
  // (in ADC) of a feature that appears in the spectrum with a known
  // calibration energy, entered in the number entry widget above.

  // If the pulse spectrum object (Spectrum_H) exists and the user has
  // selected calibration mode via the appropriate check button ...
  if(ComputationMgr->GetSpectrumExists() and SpectrumCalibration_CB->IsDown() and GraphicsMgr->GetCanvasContentType() == zSpectrum){

    TH1F *Spectrum_H = ComputationMgr->GetSpectrum();
        
    // Calculate the position along the X-axis of the pulse spectrum
    // (the "area" or "height" in ADC units) based on the current
    // X-axis maximum and the triple slider's pointer position
    double XPos = XAxisLimits_THS->GetPointerPosition() * Spectrum_H->GetXaxis()->GetXmax();
    
    // Calculate min. and max. on the Y-axis for plotting
    float Min, Max;
    YAxisLimits_DVS->GetPosition(Min, Max);
    double YMin = Spectrum_H->GetBinContent(Spectrum_H->GetMaximumBin()) * (1-Max);
    double YMax = Spectrum_H->GetBinContent(Spectrum_H->GetMaximumBin()) * (1-Min);

    // Redraw the pulse spectrum
    GraphicsMgr->PlotSpectrum();
    GraphicsMgr->PlotCalibrationLine(XPos, YMin, YMax);

    // Update the canvas with the new spectrum and calibration line
    Canvas_EC->GetCanvas()->Update();
    
    // Set the calibration pulse units for the current calibration
    // point based on the X position of the calibration line
    SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(XPos);
  }
}


void AAInterface::HandleNumberEntries()
{ 
  // Get the "active" widget object/ID from which a signal has been sent
  TGNumberEntry *ActiveNumberEntry = (TGNumberEntry *) gTQSender;
  int NumberEntryID = ActiveNumberEntry->WidgetId();
  
  SaveSettings();
  
  if(!ADAQFileLoaded)
    return;

  switch(NumberEntryID){

    // The number entry allows precise selection of waveforms to
    // plot. It also works with the slider to make sure to update the
    // position of slider if the number entry value changes
  case WaveformSelector_NEL_ID:
    WaveformSelector_HS->SetPosition(WaveformSelector_NEL->GetEntry()->GetIntNumber());
    GraphicsMgr->PlotWaveform();
    break;
    
  case MaxPeaks_NEL_ID:
    ComputationMgr->CreateNewPeakFinder(ADAQSettings->MaxPeaks);
    GraphicsMgr->PlotWaveform();
    break;

  case ZeroSuppressionCeiling_NEL_ID:
  case Sigma_NEL_ID:
  case Resolution_NEL_ID:
  case Floor_NEL_ID:
  case BaselineCalcMin_NEL_ID:
  case BaselineCalcMax_NEL_ID:
  case PearsonLowerLimit_NEL_ID:
  case PearsonMiddleLimit_NEL_ID:
  case PearsonUpperLimit_NEL_ID:
  case PSDPeakOffset_NEL_ID:
  case PSDTailOffset_NEL_ID:
    GraphicsMgr->PlotWaveform();
    break;
    
  case SpectrumNumPeaks_NEL_ID:
  case SpectrumSigma_NEL_ID:
  case SpectrumResolution_NEL_ID:
    //FindSpectrumPeaks();
    break;

  case SpectrumRangeMin_NEL_ID:
  case SpectrumRangeMax_NEL_ID:
    ComputationMgr->CalculateSpectrumBackground();
    GraphicsMgr->PlotSpectrum();
    break;
    
  case XAxisSize_NEL_ID:
  case XAxisOffset_NEL_ID:
  case XAxisDivs_NEL_ID:
  case YAxisSize_NEL_ID:
  case YAxisOffset_NEL_ID:
  case YAxisDivs_NEL_ID:
  case ZAxisSize_NEL_ID:
  case ZAxisOffset_NEL_ID:
  case ZAxisDivs_NEL_ID:

    switch(GraphicsMgr->GetCanvasContentType()){
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


void AAInterface::HandleRadioButtons()
{
  TGRadioButton *ActiveRadioButton = (TGRadioButton *) gTQSender;
  int RadioButtonID = ActiveRadioButton->WidgetId();
  
  SaveSettings();
  
  if(!ADAQFileLoaded)
    return;
  
  switch(RadioButtonID){
    
  case RawWaveform_RB_ID:
    FindPeaks_CB->SetState(kButtonDisabled);
    GraphicsMgr->PlotWaveform();
    break;
    
  case BaselineSubtractedWaveform_RB_ID:
    if(FindPeaks_CB->IsDown())
      FindPeaks_CB->SetState(kButtonDown);
    else
      FindPeaks_CB->SetState(kButtonUp);
    GraphicsMgr->PlotWaveform();
    break;
    
  case ZeroSuppressionWaveform_RB_ID:
    if(FindPeaks_CB->IsDown())
      FindPeaks_CB->SetState(kButtonDown);
    else
      FindPeaks_CB->SetState(kButtonUp);
    GraphicsMgr->PlotWaveform();
    break;

  case PositiveWaveform_RB_ID:
  case NegativeWaveform_RB_ID:
    GraphicsMgr->PlotWaveform();

  case SpectrumCalibrationManual_RB_ID:{
    
    if(SpectrumCalibrationManual_RB->IsDown()){
      SpectrumCalibrationFixedEP_RB->SetState(kButtonUp);
      
      SetCalibrationWidgetState(true, kButtonUp);
      
      SpectrumCalibrationReset_TB->Clicked();
    }

    break;
  }    
    

  case SpectrumCalibrationFixedEP_RB_ID:{

    if(SpectrumCalibrationFixedEP_RB->IsDown()){
      SpectrumCalibrationManual_RB->SetState(kButtonUp);

      SetCalibrationWidgetState(false, kButtonDisabled);
    }
    

    //    int CurrentChannel = ChannelSelector_CBL->GetComboBox()->GetSelected();
    
    //      if(UseCalibrationManager[CurrentChannel])
    //	delete CalibrationManager[CurrentChannel];
    
    //    CalibrationManager[CurrentChannel] = new TGraph(NumCalibrationPoints_FixedEP,
    //						    PulseUnitPoints_FixedEP,
    //						    EnergyPoints_FixedEP);
    
    //    UseCalibrationManager[CurrentChannel] = true;
    
    break;
  }
  
  case SpectrumWithBackground_RB_ID:
    SpectrumLessBackground_RB->SetState(kButtonUp);
    SaveSettings();
    ComputationMgr->CalculateSpectrumBackground();
    GraphicsMgr->PlotSpectrum();
    break;

  case SpectrumLessBackground_RB_ID:
    SpectrumWithBackground_RB->SetState(kButtonUp);
    SaveSettings();
    ComputationMgr->CalculateSpectrumBackground();
    GraphicsMgr->PlotSpectrum();
    break;
    
  case IntegrateRawPearson_RB_ID:
    IntegrateFitToPearson_RB->SetState(kButtonUp);
    PearsonMiddleLimit_NEL->GetEntry()->SetState(false);
    //PlotWaveform();
    break;

  case IntegrateFitToPearson_RB_ID:
    IntegrateRawPearson_RB->SetState(kButtonUp);
    PearsonMiddleLimit_NEL->GetEntry()->SetState(true);
    //    PlotWaveform();
    break;

  case PearsonPolarityPositive_RB_ID:
    PearsonPolarityNegative_RB->SetState(kButtonUp);
    //PlotWaveform();
    break;

  case PearsonPolarityNegative_RB_ID:
    PearsonPolarityPositive_RB->SetState(kButtonUp);
    //PlotWaveform();
    break;

  case ProcessingSeq_RB_ID:
    NumProcessors_NEL->GetEntry()->SetState(false);
    NumProcessors_NEL->GetEntry()->SetNumber(1);
    break;
    
  case ProcessingPar_RB_ID:
    NumProcessors_NEL->GetEntry()->SetState(true);
    NumProcessors_NEL->GetEntry()->SetNumber(NumProcessors);
    break;

  case PSDPositiveFilter_RB_ID:
    if(PSDPositiveFilter_RB->IsDown()){
      PSDNegativeFilter_RB->SetState(kButtonUp);
    }
    break;

  case PSDNegativeFilter_RB_ID:
    if(PSDNegativeFilter_RB->IsDown()){
      PSDPositiveFilter_RB->SetState(kButtonUp);
    }
    break;

  case PSDHistogramSliceX_RB_ID:
    if(PSDHistogramSliceX_RB->IsDown())
      PSDHistogramSliceY_RB->SetState(kButtonUp);
    
    if(GraphicsMgr->GetCanvasContentType() == zPSDHistogram)
      {}//PlotPSDHistogram();
    break;
    
  case PSDHistogramSliceY_RB_ID:
    if(PSDHistogramSliceY_RB->IsDown())
      PSDHistogramSliceX_RB->SetState(kButtonUp);
    if(GraphicsMgr->GetCanvasContentType() == zPSDHistogram)
      {}//PlotPSDHistogram();
    break;
    
  case DrawWaveformWithCurve_RB_ID:
  case DrawWaveformWithMarkers_RB_ID:
  case DrawWaveformWithBoth_RB_ID:
    GraphicsMgr->PlotWaveform();
    break;

  case DrawSpectrumWithBars_RB_ID:
  case DrawSpectrumWithCurve_RB_ID:
  case DrawSpectrumWithMarkers_RB_ID:
    GraphicsMgr->PlotSpectrum();
    break;
  }
}


void AAInterface::HandleComboBox(int ComboBoxID, int SelectedID)
{
  switch(ComboBoxID){
    
  case PSDPlotType_CBL_ID:
    if(ComputationMgr->GetPSDHistogramExists())
      {}//PlotPSDHistogram();
    break;
  }
}


void AAInterface::HandleCanvas(int EventID, int XPixel, int YPixel, TObject *Selected)
{
  // If the user has enabled the creation of a PSD filter and the
  // canvas event is equal to "1" (which represents a down-click
  // somewhere on the canvas pad) then send the pixel coordinates of
  // the down-click to the PSD filter creation function
  if(PSDEnableFilterCreation_CB->IsDown() and EventID == 1){
    ComputationMgr->CreatePSDFilter(XPixel, YPixel);
    GraphicsMgr->PlotPSDFilter();
  }
  
  if(PSDEnableHistogramSlicing_CB->IsDown()){
    
    // The user may click the canvas to "freeze" the PSD histogram
    // slice position, which ensures the PSD histogram slice at that
    // point remains plotted in the standalone canvas
    if(EventID == 1){
      PSDEnableHistogramSlicing_CB->SetState(kButtonUp);
      return;
    }
    else{
      ComputationMgr->CreatePSDHistogramSlice(XPixel, YPixel);
      GraphicsMgr->PlotPSDHistogramSlice(XPixel, YPixel);
    }
  }
}


void AAInterface::HandleTerminate()
{ gApplication->Terminate(); }


void AAInterface::SaveSettings(bool SaveToFile)
{
  if(ADAQSettings)
    delete ADAQSettings;
  
  ADAQSettings = new ADAQAnalysisSettings;
  
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
  
  if(PositiveWaveform_RB->IsDown())
    ADAQSettings->WaveformPolarity = 1.0;
  else
    ADAQSettings->WaveformPolarity = -1.0;
  
  ADAQSettings->FindPeaks = FindPeaks_CB->IsDown();
  ADAQSettings->MaxPeaks = MaxPeaks_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->Sigma = Sigma_NEL->GetEntry()->GetNumber();
  ADAQSettings->Resolution = Resolution_NEL->GetEntry()->GetNumber();
  ADAQSettings->Floor = Floor_NEL->GetEntry()->GetIntNumber();

  ADAQSettings->PlotFloor = PlotFloor_CB->IsDown();
  ADAQSettings->PlotCrossings = PlotCrossings_CB->IsDown();
  ADAQSettings->PlotPeakIntegrationRegion = PlotPeakIntegratingRegion_CB->IsDown();

  ADAQSettings->UsePileupRejection = UsePileupRejection_CB->IsDown();

  ADAQSettings->PlotBaselineCalcRegion = PlotBaseline_CB->IsDown();
  ADAQSettings->BaselineCalcMin = BaselineCalcMin_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->BaselineCalcMax = BaselineCalcMax_NEL->GetEntry()->GetIntNumber();

  ADAQSettings->PlotTrigger = PlotTrigger_CB->IsDown();

  //////////////////////////////////////
  // Values from "Spectrum" tabbed frame

  ADAQSettings->WaveformsToHistogram = WaveformsToHistogram_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->SpectrumNumBins = SpectrumNumBins_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->SpectrumMinBin = SpectrumMinBin_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->SpectrumMaxBin = SpectrumMaxBin_NEL->GetEntry()->GetIntNumber();

  ADAQSettings->SpectrumTypePAS = SpectrumTypePAS_RB->IsDown();
  ADAQSettings->SpectrumTypePHS = SpectrumTypePHS_RB->IsDown();

  ADAQSettings->IntegrationTypeWholeWaveform = IntegrationTypeWholeWaveform_RB->IsDown();
  ADAQSettings->IntegrationTypePeakFinder = IntegrationTypePeakFinder_RB->IsDown();
  
  ADAQSettings->FindBackground = SpectrumFindBackground_CB->IsDown();
  ADAQSettings->BackgroundMinBin = SpectrumRangeMin_NEL->GetEntry()->GetNumber();
  ADAQSettings->BackgroundMaxBin = SpectrumRangeMax_NEL->GetEntry()->GetNumber();
  ADAQSettings->PlotWithBackground = SpectrumWithBackground_RB->IsDown();
  ADAQSettings->PlotLessBackground = SpectrumLessBackground_RB->IsDown();
  
  ADAQSettings->SpectrumFindIntegral = SpectrumFindIntegral_CB->IsDown();
  ADAQSettings->SpectrumIntegralInCounts = SpectrumIntegralInCounts_CB->IsDown();
  ADAQSettings->SpectrumUseGaussianFit = SpectrumUseGaussianFit_CB->IsDown();
  ADAQSettings->SpectrumNormalizeToCurrent = SpectrumNormalizePeakToCurrent_CB->IsDown();
  ADAQSettings->SpectrumOverplotDerivative = SpectrumOverplotDerivative_CB->IsDown();

  
  //////////////////////////////////////////
  // Values from the "Analysis" tabbed frame 

  ADAQSettings->PSDEnable = PSDEnable_CB->IsDown();
  ADAQSettings->PSDChannel = PSDChannel_CBL->GetComboBox()->GetSelected();
  ADAQSettings->PSDWaveformsToDiscriminate = PSDWaveforms_NEL->GetEntry()->GetIntNumber();
  
  ADAQSettings->PSDNumTailBins = PSDNumTailBins_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PSDMinTailBin = PSDMinTailBin_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PSDMaxTailBin = PSDMaxTailBin_NEL->GetEntry()->GetIntNumber();

  ADAQSettings->PSDNumTotalBins = PSDNumTotalBins_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PSDMinTotalBin = PSDMinTotalBin_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PSDMaxTotalBin = PSDMaxTotalBin_NEL->GetEntry()->GetIntNumber();

  ADAQSettings->PSDThreshold = PSDThreshold_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PSDPeakOffset = PSDPeakOffset_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PSDTailOffset = PSDTailOffset_NEL->GetEntry()->GetIntNumber();

  ADAQSettings->PSDPlotType = PSDPlotType_CBL->GetComboBox()->GetSelected();
  ADAQSettings->PSDPlotTailIntegrationRegion = PSDPlotTailIntegration_CB->IsDown();

  if(PSDPositiveFilter_RB->IsDown())
    ADAQSettings->PSDFilterPolarity = 1.0;
  else
    ADAQSettings->PSDFilterPolarity = -1.0;

  ADAQSettings->PSDXSlice = PSDHistogramSliceX_RB->IsDown();
  ADAQSettings->PSDYSlice = PSDHistogramSliceY_RB->IsDown();

  ADAQSettings->RFQPulseWidth = RFQPulseWidth_NEL->GetEntry()->GetNumber();
  ADAQSettings->RFQRepRate = RFQRepRate_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->RFQCountRateWaveforms = CountRateWaveforms_NEL->GetEntry()->GetIntNumber();


  //////////////////////////////////////////
  // Values from the "Graphics" tabbed frame

  ADAQSettings->WaveformCurve = DrawWaveformWithCurve_RB->IsDown();
  ADAQSettings->WaveformMarkers = DrawWaveformWithMarkers_RB->IsDown();
  ADAQSettings->WaveformBoth = DrawWaveformWithBoth_RB->IsDown();
  
  ADAQSettings->SpectrumCurve = DrawSpectrumWithCurve_RB->IsDown();
  ADAQSettings->SpectrumMarkers = DrawSpectrumWithMarkers_RB->IsDown();
  ADAQSettings->SpectrumBars = DrawSpectrumWithBars_RB->IsDown();

  ADAQSettings->StatsOff = SetStatsOff_CB->IsDown();
  ADAQSettings->PlotVerticalAxisInLog = PlotVerticalAxisInLog_CB->IsDown();
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

  ADAQSettings->NumProcessors = NumProcessors_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->UpdateFreq = UpdateFreq_NEL->GetEntry()->GetIntNumber();

  ADAQSettings->IntegratePearson = IntegratePearson_CB->IsDown();
  ADAQSettings->PearsonChannel = PearsonChannel_CBL->GetComboBox()->GetSelected();  

  if(PearsonPolarityPositive_RB->IsDown())
    ADAQSettings->PearsonPolarity = 1.0;
  else
    ADAQSettings->PearsonPolarity = -1.0;
  
  ADAQSettings->IntegrateRawPearson = IntegrateRawPearson_RB->IsDown();
  ADAQSettings->IntegrateFitToPearson = IntegrateFitToPearson_RB->IsDown();

  ADAQSettings->PearsonLowerLimit = PearsonLowerLimit_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PearsonMiddleLimit = PearsonMiddleLimit_NEL->GetEntry()->GetIntNumber();
  ADAQSettings->PearsonUpperLimit = PearsonUpperLimit_NEL->GetEntry()->GetIntNumber();

  ADAQSettings->PlotPearsonIntegration = PlotPearsonIntegration_CB->IsDown();  

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
  ADAQSettings->SpectraCalibrations = ComputationMgr->GetSpectraCalibrations();
  
  // PSD filter objects
  ADAQSettings->UsePSDFilters = ComputationMgr->GetUsePSDFilters();
  ADAQSettings->PSDFilters = ComputationMgr->GetPSDFilters();

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
  
  const int NumTitles = 6;

  string BoxTitlesAsterisk[] = {"ADAQAnalysis says 'good job!", 
				"Oh, so you are competent!",
				"This is a triumph of science!",
				"Excellent work. You're practically a PhD now.",
				"For you ARE the Kwisatz Haderach!",
				"There will be a parade in your honor."};
  
  string BoxTitlesStop[] = {"ADAQAnalysis is disappointed in you...", 
			    "Seriously? I'd like another operator, please.",
			    "Unacceptable. Just totally unacceptable.",
			    "That was about as successful as the Hindenburg...",
			    "You blew it!",
			    "Abominable! Off with your head!" };

  // Surprise the user!
  TRandom *RNG = new TRandom;
  int RndmInt = RNG->Integer(NumTitles);
  
  string Title = BoxTitlesStop[RndmInt];
  if(IconType==kMBIconAsterisk)
    Title = BoxTitlesAsterisk[RndmInt];

  
  // Create the message box with the desired message and icon
  new TGMsgBox(gClient->GetRoot(), this, Title.c_str(), Message.c_str(), IconType, kMBOk);
  
  delete RNG;
}


void AAInterface::UpdateForNewFile(string FileName)
{
  int WaveformsInFile = ComputationMgr->GetADAQNumberOfWaveforms();
  ADAQRootMeasParams *AMP = ComputationMgr->GetADAQMeasurementParameters();
  int RecordLength = AMP->RecordLength;
  
  WaveformSelector_HS->SetRange(1, WaveformsInFile);

  WaveformsToHistogram_NEL->GetEntry()->SetLimitValues(1, WaveformsInFile);
  WaveformsToHistogram_NEL->GetEntry()->SetNumber(20000);//WaveformsInFile); ZSN

  FileName_TE->SetText(FileName.c_str());
  Waveforms_NEL->SetNumber(WaveformsInFile);
  RecordLength_NEL->SetNumber(RecordLength);
  
  BaselineCalcMin_NEL->GetEntry()->SetLimitValues(0,RecordLength-1);
  BaselineCalcMin_NEL->GetEntry()->SetNumber(0);
  
  BaselineCalcMax_NEL->GetEntry()->SetLimitValues(1,RecordLength);
  
  int BaselineCalcMax = 0;
  (RecordLength > 1500) ? BaselineCalcMax = 750 : BaselineCalcMax = 100;
  BaselineCalcMax_NEL->GetEntry()->SetNumber(BaselineCalcMax);
  
  PearsonLowerLimit_NEL->GetEntry()->SetLimitValues(0, RecordLength-1);
  PearsonLowerLimit_NEL->GetEntry()->SetNumber(0);

  PearsonMiddleLimit_NEL->GetEntry()->SetLimitValues(1, RecordLength-1);
  PearsonMiddleLimit_NEL->GetEntry()->SetNumber(RecordLength/2);

  PearsonUpperLimit_NEL->GetEntry()->SetLimitValues(1, RecordLength);
  PearsonUpperLimit_NEL->GetEntry()->SetNumber(RecordLength);
  
  DesplicedWaveformNumber_NEL->GetEntry()->SetLimitValues(1, WaveformsInFile);
  DesplicedWaveformNumber_NEL->GetEntry()->SetNumber(WaveformsInFile);
  
  PSDWaveforms_NEL->GetEntry()->SetLimitValues(1, WaveformsInFile);
  PSDWaveforms_NEL->GetEntry()->SetNumber(WaveformsInFile);
}


void AAInterface::SetCalibrationWidgetState(bool WidgetState, EButtonState ButtonState)
{
  // Set the widget states based on above variable set results
  SpectrumCalibrationPoint_CBL->GetComboBox()->SetEnabled(WidgetState);
  SpectrumCalibrationEnergy_NEL->GetEntry()->SetState(WidgetState);
  SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetState(WidgetState);
  SpectrumCalibrationSetPoint_TB->SetState(ButtonState);
  SpectrumCalibrationCalibrate_TB->SetState(ButtonState);
  SpectrumCalibrationPlot_TB->SetState(ButtonState);
  SpectrumCalibrationReset_TB->SetState(ButtonState);
}


