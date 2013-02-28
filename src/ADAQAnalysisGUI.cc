//////////////////////////////////////////////////////////////////////////////////////////
//
// name: ADAQAnalysisGUI.cc
// date: 27 Feb 13 (Last updated)
// auth: Zach Hartwig
//
// desc: ADAQAnalysisGUI.cc is the main implementation file for the
//       ADAQAnalysisGUI program. ADAQAnalysisGUI is exactly that: an
//       incredibly powerful graphical user interface (GUI) that can
//       be used to view, analyze, and operate on ADAQ-formatted ROOT
//       data files. The graphical interface, as well as the bulk of
//       the underlying analysis tooks, are provided by the ROOT
//       toolkit. 
//
//////////////////////////////////////////////////////////////////////////////////////////

// ROOT
#include <TFile.h>
#include <TTree.h>
#include <TBrowser.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TAxis.h>
#include <TCanvas.h>
#include <TApplication.h>
#include <TLine.h>
#include <TBox.h>
#include <TGClient.h>
#include <TGFileDialog.h>
#include <TSpectrum.h>
#include <TMarker.h>
#include <TH1F.h>
#include <TF1.h>
#include <TGTab.h>
#include <TGButtonGroup.h>
#include <TSystem.h>
#include <TError.h>
#include <TPaveLabel.h>
#include <TGMsgBox.h>
#include <TVectorD.h>
#include <TChain.h>
#include <TPaletteAxis.h>
#include <TStyle.h>

// C++
#include <string>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>
#include <iomanip>
#include <ctime>
using namespace std;

// MPI
#ifdef MPI_ENABLED
#include <mpi.h>
#endif

// Boost
#include <boost/array.hpp>
#include <boost/thread.hpp>

// ADAQ
#include "ADAQAnalysisGUI.hh"
#include "ADAQAnalysisConstants.hh"
#include "ADAQAnalysisVersion.hh"

ADAQAnalysisGUI::ADAQAnalysisGUI(bool PA, string CmdLineArg)
  : TGMainFrame(gClient->GetRoot()),
    ADAQMeasParams(0), ADAQRootFile(0), ADAQRootFileName(""), ADAQRootFileLoaded(false), ADAQWaveformTree(0), 
    ADAQParResults(NULL), ADAQParResultsLoaded(false),
    DataDirectory(getenv("PWD")), SaveSpectrumDirectory(getenv("HOME")), SaveHistogramDirectory(getenv("HOME")),
    PrintCanvasDirectory(getenv("HOME")), DesplicedDirectory(getenv("HOME")),
    Time(0), RawVoltage(0), RecordLength(0),
    GraphicFileName(""), GraphicFileExtension(""),
    Trigger_L(new TLine), Floor_L(new TLine), Calibration_L(new TLine), ZSCeiling_L(new TLine), NCeiling_L(new TLine),
    LPeakDelimiter_L(new TLine), RPeakDelimiter_L(new TLine),
    PearsonLowerLimit_L(new TLine), PearsonMiddleLimit_L(new TLine), PearsonUpperLimit_L(new TLine),
    PSDPeakOffset_L(new TLine), PSDTailOffset_L(new TLine), 
    LowerLimit_L(new TLine), UpperLimit_L(new TLine), DerivativeReference_L(new TLine),
    Baseline_B(new TBox), PSDTailIntegral_B(new TBox),
    Spectrum_H(new TH1F), SpectrumBackground_H(new TH1F), SpectrumDeconvolved_H(new TH1F), 
    PeakFinder(new TSpectrum), NumPeaks(0), PeakInfoVec(0), PeakIntegral_LowerLimit(0), PeakIntegral_UpperLimit(0), PeakLimits(0),
    MPI_Size(1), MPI_Rank(0), IsMaster(true), IsSlave(false), ParallelArchitecture(PA), SequentialArchitecture(!PA),
    Verbose(false), ParallelVerbose(true),
    XAxisMin(0), XAxisMax(1.0), YAxisMin(0), YAxisMax(4095), 
    Title(""), XAxisTitle(""), YAxisTitle(""), ZAxisTitle(""), PaletteAxisTitle(""),
    WaveformPolarity(-1.0), PearsonPolarity(1.0), Baseline(0.), PSDFilterPolarity(1.),
    V1720MaximumBit(4095), NumDataChannels(8),
    SpectrumExists(false), SpectrumDerivativeExists(false), PSDHistogramExists(false), PSDHistogramSliceExists(false),
    SpectrumMaxPeaks(0), TotalPeaks(0), TotalDeuterons(0),
    GaussianBinWidth(1.), CountsBinWidth(1.),
    ColorManager(new TColor), RNG(new TRandom(time(NULL)))
{
  SetCleanup(kDeepCleanup);
  
  // Assign attributes to the class member objects that are used to
  // represent graphical objects. The purpose in making them class
  // member objects is to allocate memory only once via the "new"
  // operator above in the initialization list to avoid memory leaks.

  Trigger_L->SetLineStyle(7);
  Trigger_L->SetLineColor(2);
  Trigger_L->SetLineWidth(2);

  Floor_L->SetLineStyle(7);
  Floor_L->SetLineColor(2);
  Floor_L->SetLineWidth(2);

  Baseline_B->SetFillStyle(3002);
  Baseline_B->SetFillColor(8);

  ZSCeiling_L->SetLineStyle(7);
  ZSCeiling_L->SetLineColor(7);
  ZSCeiling_L->SetLineWidth(2);
  
  NCeiling_L->SetLineStyle(7);
  NCeiling_L->SetLineColor(6);
  NCeiling_L->SetLineWidth(2);

  LPeakDelimiter_L->SetLineStyle(7);
  LPeakDelimiter_L->SetLineWidth(2);
  LPeakDelimiter_L->SetLineColor(6);
  
  RPeakDelimiter_L->SetLineStyle(7);
  RPeakDelimiter_L->SetLineWidth(2);
  RPeakDelimiter_L->SetLineColor(7);
  
  Calibration_L->SetLineStyle(7);
  Calibration_L->SetLineColor(2);
  Calibration_L->SetLineWidth(2);

  PearsonLowerLimit_L->SetLineStyle(7);
  PearsonLowerLimit_L->SetLineColor(4);
  PearsonLowerLimit_L->SetLineWidth(2);

  PearsonMiddleLimit_L->SetLineStyle(7);
  PearsonMiddleLimit_L->SetLineColor(8);
  PearsonMiddleLimit_L->SetLineWidth(2);
  
  PearsonUpperLimit_L->SetLineStyle(7);
  PearsonUpperLimit_L->SetLineColor(4);
  PearsonUpperLimit_L->SetLineWidth(2);

  PSDPeakOffset_L->SetLineStyle(7);
  PSDPeakOffset_L->SetLineColor(8);
  PSDPeakOffset_L->SetLineWidth(2);

  PSDTailOffset_L->SetLineStyle(7);
  PSDTailOffset_L->SetLineColor(8);
  PSDTailOffset_L->SetLineWidth(2);

  PSDTailIntegral_B->SetFillStyle(3002);
  PSDTailIntegral_B->SetFillColor(38);

  LowerLimit_L->SetLineStyle(7);
  LowerLimit_L->SetLineColor(2);
  LowerLimit_L->SetLineWidth(2);

  UpperLimit_L->SetLineStyle(7);
  UpperLimit_L->SetLineColor(2);
  UpperLimit_L->SetLineWidth(2);

  DerivativeReference_L->SetLineStyle(7);
  DerivativeReference_L->SetLineColor(2);
  DerivativeReference_L->SetLineWidth(2);


  // Initialize the objects used in the calibration and pulse shape
  // discrimination (PSD) scheme. A set of 8 calibration and 8 PSD
  // filter "managers" -- really, TGraph *'s used for interpolating --
  // along with the corresponding booleans that determine whether or
  // not that channel's "manager" should be used is initialized. 
  for(int ch=0; ch<NumDataChannels; ch++){
    // All 8 channel's managers are set "off" by default
    UseCalibrationManager.push_back(false);
    UsePSDFilterManager.push_back(false);
    
    // Store empty TGraph pointers in std::vectors to hold space and
    // prevent seg. faults later when we test/delete unused objects
    CalibrationManager.push_back(new TGraph);
    PSDFilterManager.push_back(new TGraph);
    
    // Assign a blank initial structure for channel calibration data
    ADAQChannelCalibrationData Init;
    CalibrationData.push_back(Init);
  }
  
  // Assign the location of the ADAQAnalysisGUI parallel binary
  // (depends on whether the presently executed binary is a develpment
  // or production version of the code)
  ADAQHOME = getenv("ADAQHOME");
  USER = getenv("USER");
  
  if(VersionString == "Development")
    ParallelBinaryName = ADAQHOME + "/analysis/ADAQAnalysisGUI/trunk/bin/ADAQAnalysisGUI_MPI";
  else{
    ParallelBinaryName = ADAQHOME + "/analysis/ADAQAnalysisGUI/versions/" + VersionString + "/bin/ADAQAnalysisGUI_MPI";
  }

  // Assign the locatino of the temporary parallel processing ROOT
  // file, which is used to transmit data computed in parallel
  // architecture back to sequential architecture
  ParallelProcessingFName = "/tmp/ParallelProcessing_" + USER + ".root";

  // Set ROOT to print only break messages and above (to suppress the
  // annoying warning from TSpectrum that the peak buffer is full)
  gErrorIgnoreLevel = kBreak;
  
  // Use Boost's thread library to set the number of available
  // processors/threads on the current machine
  NumProcessors = boost::thread::hardware_concurrency();

  
  /////////////////////////////
  // Sequential architecture //
  /////////////////////////////
  // The sequential binary of ADAQAnalysisGUI provides the user with a
  // full graphical user interface for ADAQ waveform analysis. Note
  // that parallel processing of waveforms (using the parallel version
  // of ADAQAnalysisGUI) is completely handled from the sequential
  // binary graphical interface.

  if(SequentialArchitecture){
    // Build the graphical user interface
    CreateMainFrame();
    
    // If the first cmd line arg is not "unspecified" then assume the
    // user has specified an ADAQ ROOT file and automatically load it
    if(CmdLineArg != "unspecified"){

      // In order to ensure that parallel binaries can locate the
      // specified data file (in the case that their current linux
      // directory has been changed by the user in selecting save file
      // paths), we must specify the full path to the ADAQ-formatted
      // ROOT file that is to be processsed

      // If the cmd line specified file name starts with a forward
      // slash then assume it is an absolute path that fully specifies
      // the location of the file
      if(CmdLineArg[0] == '/')
	ADAQRootFileName = CmdLineArg;
      
      // Else assume that the path is a relative path that lies
      // "below" (in the directory hierarchy) the binary's execution
      // direction. In this case, specifying the absolute path
      // requires tacking on the relative path to the execution path
      else{
	// Get the execution directory of the sequential binary
	string ExecDir = getenv("PWD");
	
	// Create the full path to the data file by combining the
	// execution directory with the cmd line specified (relative)
	// path to the file name
	ADAQRootFileName = ExecDir + "/" + CmdLineArg;
      }

      LoadADAQRootFile();
    }
  }

  
  ///////////////////////////
  // Parallel architecture //
  ///////////////////////////
  // The parallel binary of ADAQAnalysisGUI (which is called
  // "ADAQAnalysisGUI_MPI" when build via the parallel.sh/Makefile
  // files) is completely handled (initiation, processing, results
  // collection) by the sequential binary of ADAQAnalysisGUI. It
  // simply acts as a brute force processor for a specific waveform
  // processing task; all options are set via the sequential binary
  // before parallel processing is initiated. Therefore, the parallel
  // binary does not build the GUI nor many of the class objects
  // associated with the GUI. The parameters that are necessary for
  // parallel processing (which are set via the ROOT widgets in the
  // sequential binary) are loaded from a ROOT file that is written by
  // the sequential binary for the purpose of transmitting widget
  // settings to the parallel binary. Then, the specified ADAQ ROOT
  // file is loaded and the specified parallel waveform processing is
  // initiated. Upon completeion, results generated in parallel are
  // aggregated into a single object (histogram, file, etc...) that
  // the sequential binary can import for further manipulation. The
  // key point is that the sequential binary is running before,
  // during, and after the waveforms are being processing in parallel.
  
  else if(ParallelArchitecture){
    
#ifdef MPI_ENABLED
    // Get the total number of processors 
    MPI_Size = MPI::COMM_WORLD.Get_size();

    // Get the node ID of the present node
    MPI_Rank = MPI::COMM_WORLD.Get_rank();
    
    // Set present node status (master (node 0) or a slave (!node 0)
    IsMaster = (MPI_Rank == 0);
    IsSlave = (MPI_Rank != 0);
#endif

    // Load the parameters required for processing from the ROOT file
    // generated from the sequential binary's ROOT widget settings
    LoadParallelProcessingData();

    // Load the specified ADAQ ROOT file
    LoadADAQRootFile();

    // Initiate the desired parallel waveform processing algorithm

    // Histogram waveforms into a spectrum
    if(CmdLineArg == "histogramming")
      CreateSpectrum();

    // Desplice (or "uncouple") waveforms into a new ADAQ ROOT file
    else if(CmdLineArg == "desplicing")
      CreateDesplicedFile();

    else if(CmdLineArg == "discriminating")
      CreatePSDHistogram();

    // Notify the user of error in processing type specification
    else{
      cout << "\nError! Unspecified command line argument '" << CmdLineArg << "' passed to ADAQAnalysisGUI_MPI!\n"
	   <<   "       At present, only the args 'histogramming' and 'desplicing' are allowed\n"
	   <<   "       Parallel binaries will exit ...\n"
	   << endl;
      exit(-42);
    }
  }
}


ADAQAnalysisGUI::~ADAQAnalysisGUI()
{
  delete Trigger_L;
  delete Floor_L;
  delete Calibration_L;
  delete ZSCeiling_L;
  delete NCeiling_L;
  delete LPeakDelimiter_L;
  delete RPeakDelimiter_L;
  delete PearsonLowerLimit_L;
  delete PearsonMiddleLimit_L;
  delete PearsonUpperLimit_L;
  delete PSDPeakOffset_L;
  delete PSDTailOffset_L;
  delete Baseline_B;
  delete Spectrum_H;
  delete SpectrumBackground_H;
  delete SpectrumDeconvolved_H;
  delete PeakFinder;
  delete ColorManager;
  delete RNG;

  for(int ch=0; ch<NumDataChannels; ch++)
    delete CalibrationManager[ch];
}


void ADAQAnalysisGUI::CreateMainFrame()
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
  MenuFile->Connect("Activated(int)", "ADAQAnalysisGUI", this, "HandleMenu(int)");

  TGMenuBar *MenuBar = new TGMenuBar(MenuFrame, 100, 20, kHorizontalFrame);
  MenuBar->AddPopup("&File", MenuFile, new TGLayoutHints(kLHintsTop | kLHintsLeft, 0,0,0,0));
  MenuFrame->AddFrame(MenuBar, new TGLayoutHints(kLHintsTop | kLHintsLeft, 0,0,0,0));
  AddFrame(MenuFrame, new TGLayoutHints(kLHintsTop | kLHintsExpandX));


  //////////////////////////////////////////////////////////
  // Create the main frame to hold the options and canvas //
  //////////////////////////////////////////////////////////

  TGHorizontalFrame *OptionsAndCanvas_HF = new TGHorizontalFrame(this);
  OptionsAndCanvas_HF->SetBackgroundColor(ColorManager->Number2Pixel(16));
  AddFrame(OptionsAndCanvas_HF, new TGLayoutHints(kLHintsTop, 5, 5,5,5));

  
  //////////////////////////////////////////////////////////////
  // Create the left-side vertical options frame with tab holder 

  TGVerticalFrame *Options_VF = new TGVerticalFrame(OptionsAndCanvas_HF);
  Options_VF->SetBackgroundColor(ColorManager->Number2Pixel(16));
  OptionsAndCanvas_HF->AddFrame(Options_VF);
  
  TGTab *OptionsTabs_T = new TGTab(Options_VF);
  OptionsTabs_T->SetBackgroundColor(ColorManager->Number2Pixel(16));
  Options_VF->AddFrame(OptionsTabs_T, new TGLayoutHints(kLHintsLeft, 15,15,5,5));


  //////////////////////////////////////
  // Create the individual tabs + frames

  // The tabbed frame to hold waveform widgets
  TGCompositeFrame *WaveformOptionsTab_CF = OptionsTabs_T->AddTab("Waveform");
  TGCompositeFrame *WaveformOptions_CF = new TGCompositeFrame(WaveformOptionsTab_CF, 1, 1, kVerticalFrame);
  WaveformOptions_CF->Resize(320,735);
  WaveformOptions_CF->ChangeOptions(WaveformOptions_CF->GetOptions() | kFixedSize);
  WaveformOptionsTab_CF->AddFrame(WaveformOptions_CF);

  // The tabbed frame to hold spectrum widgets
  TGCompositeFrame *SpectrumOptionsTab_CF = OptionsTabs_T->AddTab("Spectrum");
  TGCompositeFrame *SpectrumOptions_CF = new TGCompositeFrame(SpectrumOptionsTab_CF, 1, 1, kVerticalFrame);
  SpectrumOptions_CF->Resize(320,735);
  SpectrumOptions_CF->ChangeOptions(SpectrumOptions_CF->GetOptions() | kFixedSize);
  SpectrumOptionsTab_CF->AddFrame(SpectrumOptions_CF);

  // The tabbed frame to hold analysis widgets
  TGCompositeFrame *AnalysisOptionsTab_CF = OptionsTabs_T->AddTab("Analysis");
  TGCompositeFrame *AnalysisOptions_CF = new TGCompositeFrame(AnalysisOptionsTab_CF, 1, 1, kVerticalFrame);
  AnalysisOptions_CF->Resize(320,735);
  AnalysisOptions_CF->ChangeOptions(AnalysisOptions_CF->GetOptions() | kFixedSize);
  AnalysisOptionsTab_CF->AddFrame(AnalysisOptions_CF);

  // The tabbed frame to hold graphical widgets
  TGCompositeFrame *GraphicsOptionsTab_CF = OptionsTabs_T->AddTab("Graphics");
  TGCompositeFrame *GraphicsOptions_CF = new TGCompositeFrame(GraphicsOptionsTab_CF, 1, 1, kVerticalFrame);
  GraphicsOptions_CF->Resize(320,735);
  GraphicsOptions_CF->ChangeOptions(GraphicsOptions_CF->GetOptions() | kFixedSize);
  GraphicsOptionsTab_CF->AddFrame(GraphicsOptions_CF);

  // The tabbed frame to hold parallel processing widgets
  TGCompositeFrame *ProcessingOptionsTab_CF = OptionsTabs_T->AddTab("Processing");
  TGCompositeFrame *ProcessingOptions_CF = new TGCompositeFrame(ProcessingOptionsTab_CF, 1, 1, kVerticalFrame);
  ProcessingOptions_CF->Resize(320,735);
  ProcessingOptions_CF->ChangeOptions(ProcessingOptions_CF->GetOptions() | kFixedSize);
  ProcessingOptionsTab_CF->AddFrame(ProcessingOptions_CF);


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
  WaveformSelector_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
  
  // Waveform specification (type and polarity)

  TGHorizontalFrame *WaveformSpecification_HF = new TGHorizontalFrame(WaveformOptions_CF);
  WaveformOptions_CF->AddFrame(WaveformSpecification_HF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  
  WaveformSpecification_HF->AddFrame(WaveformType_BG = new TGButtonGroup(WaveformSpecification_HF, "Type", kVerticalFrame),
				     new TGLayoutHints(kLHintsLeft, 0,5,0,5));
  RawWaveform_RB = new TGRadioButton(WaveformType_BG, "Raw voltage", RawWaveform_RB_ID);
  RawWaveform_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  RawWaveform_RB->SetState(kButtonDown);
  
  BaselineSubtractedWaveform_RB = new TGRadioButton(WaveformType_BG, "Baseline-subtracted", BaselineSubtractedWaveform_RB_ID);
  BaselineSubtractedWaveform_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");

  ZeroSuppressionWaveform_RB = new TGRadioButton(WaveformType_BG, "Zero suppression", ZeroSuppressionWaveform_RB_ID);
  ZeroSuppressionWaveform_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");

  WaveformSpecification_HF->AddFrame(WaveformPolarity_BG = new TGButtonGroup(WaveformSpecification_HF, "Polarity", kVerticalFrame),
				     new TGLayoutHints(kLHintsLeft, 5,5,0,5));
  
  PositiveWaveform_RB = new TGRadioButton(WaveformPolarity_BG, "Positive", PositiveWaveform_RB_ID);
  PositiveWaveform_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  PositiveWaveform_RB->SetState(kButtonDown);
  
  NegativeWaveform_RB = new TGRadioButton(WaveformPolarity_BG, "Negative", NegativeWaveform_RB_ID);
  NegativeWaveform_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");

  // Zero suppression options

  WaveformOptions_CF->AddFrame(PlotZeroSuppressionCeiling_CB = new TGCheckButton(WaveformOptions_CF, "Plot zero suppression ceiling", PlotZeroSuppressionCeiling_CB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  PlotZeroSuppressionCeiling_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  
  WaveformOptions_CF->AddFrame(ZeroSuppressionCeiling_NEL = new ADAQNumberEntryWithLabel(WaveformOptions_CF, "Zero suppression ceiling", ZeroSuppressionCeiling_NEL_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,0,5));
  ZeroSuppressionCeiling_NEL->GetEntry()->SetNumber(15);
  ZeroSuppressionCeiling_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

  // Peak finding (ROOT TSpectrum) options

  WaveformOptions_CF->AddFrame(FindPeaks_CB = new TGCheckButton(WaveformOptions_CF, "Find peaks", FindPeaks_CB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  FindPeaks_CB->SetState(kButtonDisabled);
  FindPeaks_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");

  WaveformOptions_CF->AddFrame(MaxPeaks_NEL = new ADAQNumberEntryWithLabel(WaveformOptions_CF, "Maximum peaks", MaxPeaks_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  MaxPeaks_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  MaxPeaks_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  MaxPeaks_NEL->GetEntry()->SetNumber(15);
  MaxPeaks_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

  WaveformOptions_CF->AddFrame(Sigma_NEL = new ADAQNumberEntryWithLabel(WaveformOptions_CF, "Sigma", Sigma_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  Sigma_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  Sigma_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  Sigma_NEL->GetEntry()->SetNumber(15);
  Sigma_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

  WaveformOptions_CF->AddFrame(Resolution_NEL = new ADAQNumberEntryWithLabel(WaveformOptions_CF, "Resolution", Resolution_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  Resolution_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  Resolution_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  Resolution_NEL->GetEntry()->SetNumber(0.005);
  Resolution_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

  WaveformOptions_CF->AddFrame(Floor_NEL = new ADAQNumberEntryWithLabel(WaveformOptions_CF, "Floor", Floor_NEL_ID),
		       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  Floor_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  Floor_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  Floor_NEL->GetEntry()->SetNumber(50);
  Floor_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
  
  TGGroupFrame *PeakPlottingOptions_GF = new TGGroupFrame(WaveformOptions_CF, "Peak finding plotting options", kHorizontalFrame);
  WaveformOptions_CF->AddFrame(PeakPlottingOptions_GF, new TGLayoutHints(kLHintsLeft, 15,5,0,5));

  PeakPlottingOptions_GF->AddFrame(PlotFloor_CB = new TGCheckButton(PeakPlottingOptions_GF, "Floor", PlotFloor_CB_ID),
				   new TGLayoutHints(kLHintsLeft, 0,0,5,0));
  PlotFloor_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  
  PeakPlottingOptions_GF->AddFrame(PlotCrossings_CB = new TGCheckButton(PeakPlottingOptions_GF, "Crossings", PlotCrossings_CB_ID),
				   new TGLayoutHints(kLHintsLeft, 5,0,5,0));
  PlotCrossings_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  
  PeakPlottingOptions_GF->AddFrame(PlotPeakIntegratingRegion_CB = new TGCheckButton(PeakPlottingOptions_GF, "Int. region", PlotPeakIntegratingRegion_CB_ID),
		       new TGLayoutHints(kLHintsLeft, 5,0,5,0));
  PlotPeakIntegratingRegion_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");

  // Pileup options
  
  WaveformOptions_CF->AddFrame(UsePileupRejection_CB = new TGCheckButton(WaveformOptions_CF, "Use pileup rejection", UsePileupRejection_CB_ID),
			       new TGLayoutHints(kLHintsNormal, 15,5,5,5));
  UsePileupRejection_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  UsePileupRejection_CB->SetState(kButtonDown);
			       
  // Baseline calculation options
  
  WaveformOptions_CF->AddFrame(PlotBaseline_CB = new TGCheckButton(WaveformOptions_CF, "Plot baseline calc. region.", PlotBaseline_CB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  PlotBaseline_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  
  TGHorizontalFrame *BaselineRegion_HF = new TGHorizontalFrame(WaveformOptions_CF);
  WaveformOptions_CF->AddFrame(BaselineRegion_HF, new TGLayoutHints(kLHintsLeft, 15,5,5,5));
  
  BaselineRegion_HF->AddFrame(BaselineCalcMin_NEL = new ADAQNumberEntryWithLabel(BaselineRegion_HF, "Min.", BaselineCalcMin_NEL_ID),
			      new TGLayoutHints(kLHintsLeft, 0,5,0,0));
  BaselineCalcMin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  BaselineCalcMin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  BaselineCalcMin_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  BaselineCalcMin_NEL->GetEntry()->SetLimitValues(0,1); // Set when ADAQRootFile loaded
  BaselineCalcMin_NEL->GetEntry()->SetNumber(0); // Set when ADAQRootFile loaded
  BaselineCalcMin_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
  
  BaselineRegion_HF->AddFrame(BaselineCalcMax_NEL = new ADAQNumberEntryWithLabel(BaselineRegion_HF, "Max.", BaselineCalcMax_NEL_ID),
			      new TGLayoutHints(kLHintsLeft, 0,5,0,5)); 
  BaselineCalcMax_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  BaselineCalcMax_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEANonNegative);
  BaselineCalcMax_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  BaselineCalcMax_NEL->GetEntry()->SetLimitValues(1,2); // Set When ADAQRootFile loaded
  BaselineCalcMax_NEL->GetEntry()->SetNumber(1); // Set when ADAQRootFile loaded
  BaselineCalcMax_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

  // Trigger

  WaveformOptions_CF->AddFrame(PlotTrigger_CB = new TGCheckButton(WaveformOptions_CF, "Plot trigger", PlotTrigger_CB_ID),
		       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  PlotTrigger_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");


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
  SpectrumMaxBin_NEL->GetEntry()->SetNumber(2000);

  TGHorizontalFrame *SpectrumOptions_HF = new TGHorizontalFrame(SpectrumFrame_VF);
  SpectrumFrame_VF->AddFrame(SpectrumOptions_HF, new TGLayoutHints(kLHintsNormal, 15,0,0,0));

  // Spectrum type radio buttons
  SpectrumOptions_HF->AddFrame(SpectrumType_BG = new TGButtonGroup(SpectrumOptions_HF, "Spectrum type", kVerticalFrame),
			       new TGLayoutHints(kLHintsLeft, 0,5,0,0));
  SpectrumTypePAS_RB = new TGRadioButton(SpectrumType_BG, "Pulse area", -1);
  
  SpectrumTypePHS_RB = new TGRadioButton(SpectrumType_BG, "Pulse height", -1);
  SpectrumTypePHS_RB->SetState(kButtonDown);

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
  SpectrumCalibration_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  SpectrumCalibration_CB->SetState(kButtonUp);

  TGHorizontalFrame *SpectrumCalibrationType_HF = new TGHorizontalFrame(SpectrumCalibration_GF);
  SpectrumCalibration_GF->AddFrame(SpectrumCalibrationType_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));

  SpectrumCalibrationType_HF->AddFrame(SpectrumCalibrationManual_RB = new TGRadioButton(SpectrumCalibrationType_HF, "Manual entry", SpectrumCalibrationManual_RB_ID),
				       new TGLayoutHints(kLHintsNormal, 0,15,0,5));
  SpectrumCalibrationManual_RB->SetState(kButtonDown);
  SpectrumCalibrationManual_RB->SetState(kButtonDisabled);
  SpectrumCalibrationManual_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  
  SpectrumCalibrationType_HF->AddFrame(SpectrumCalibrationFixedEP_RB = new TGRadioButton(SpectrumCalibrationType_HF, "EP detector", SpectrumCalibrationFixedEP_RB_ID),
				       new TGLayoutHints(kLHintsNormal, 0,0,0,5));
  SpectrumCalibrationFixedEP_RB->SetState(kButtonDisabled);
  SpectrumCalibrationFixedEP_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  
  
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
  SpectrumCalibrationSetPoint_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");
  SpectrumCalibrationSetPoint_TB->Resize(100,25);
  SpectrumCalibrationSetPoint_TB->ChangeOptions(SpectrumCalibrationSetPoint_TB->GetOptions() | kFixedSize);
  SpectrumCalibrationSetPoint_TB->SetState(kButtonDisabled);

  // Calibrate text button
  SpectrumCalibration_HF1->AddFrame(SpectrumCalibrationCalibrate_TB = new TGTextButton(SpectrumCalibration_HF1, "Calibrate", SpectrumCalibrationCalibrate_TB_ID),
				    new TGLayoutHints(kLHintsNormal, 0,0,5,5));
  SpectrumCalibrationCalibrate_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");
  SpectrumCalibrationCalibrate_TB->Resize(100,25);
  SpectrumCalibrationCalibrate_TB->ChangeOptions(SpectrumCalibrationCalibrate_TB->GetOptions() | kFixedSize);
  SpectrumCalibrationCalibrate_TB->SetState(kButtonDisabled);

  TGHorizontalFrame *SpectrumCalibration_HF2 = new TGHorizontalFrame(SpectrumCalibration_GF);
  SpectrumCalibration_GF->AddFrame(SpectrumCalibration_HF2);
  
  // Plot text button
  SpectrumCalibration_HF2->AddFrame(SpectrumCalibrationPlot_TB = new TGTextButton(SpectrumCalibration_HF2, "Plot", SpectrumCalibrationPlot_TB_ID),
				    new TGLayoutHints(kLHintsNormal, 5,0,5,5));
  SpectrumCalibrationPlot_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");
  SpectrumCalibrationPlot_TB->Resize(100,25);
  SpectrumCalibrationPlot_TB->ChangeOptions(SpectrumCalibrationPlot_TB->GetOptions() | kFixedSize);
  SpectrumCalibrationPlot_TB->SetState(kButtonDisabled);

  // Reset text button
  SpectrumCalibration_HF2->AddFrame(SpectrumCalibrationReset_TB = new TGTextButton(SpectrumCalibration_HF2, "Reset", SpectrumCalibrationReset_TB_ID),
					  new TGLayoutHints(kLHintsNormal, 0,0,5,5));
  SpectrumCalibrationReset_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");
  SpectrumCalibrationReset_TB->Resize(100,25);
  SpectrumCalibrationReset_TB->ChangeOptions(SpectrumCalibrationReset_TB->GetOptions() | kFixedSize);
  SpectrumCalibrationReset_TB->SetState(kButtonDisabled);

  // Spectrum analysis
  
  TGGroupFrame *SpectrumAnalysis_GF = new TGGroupFrame(SpectrumFrame_VF, "Analysis", kVerticalFrame);
  SpectrumFrame_VF->AddFrame(SpectrumAnalysis_GF, new TGLayoutHints(kLHintsNormal, 15,5,5,5));

  /*
  SpectrumAnalysis_GF->AddFrame(SpectrumFindPeaks_CB = new TGCheckButton(SpectrumAnalysis_GF, "Find peaks", SpectrumFindPeaks_CB_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  SpectrumFindPeaks_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");

  SpectrumAnalysis_GF->AddFrame(SpectrumNumPeaks_NEL = new ADAQNumberEntryWithLabel(SpectrumAnalysis_GF, "Num. peaks", SpectrumNumPeaks_NEL_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumNumPeaks_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  SpectrumNumPeaks_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumNumPeaks_NEL->GetEntry()->SetNumber(1);
  SpectrumNumPeaks_NEL->GetEntry()->Resize(75,20);
  SpectrumNumPeaks_NEL->GetEntry()->SetState(false);
  SpectrumNumPeaks_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
    
  SpectrumAnalysis_GF->AddFrame(SpectrumSigma_NEL = new ADAQNumberEntryWithLabel(SpectrumAnalysis_GF, "Sigma", SpectrumSigma_NEL_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumSigma_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  SpectrumSigma_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumSigma_NEL->GetEntry()->SetNumber(2);
  SpectrumSigma_NEL->GetEntry()->Resize(75,20);
  SpectrumSigma_NEL->GetEntry()->SetState(false);
  SpectrumSigma_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
  
  SpectrumAnalysis_GF->AddFrame(SpectrumResolution_NEL = new ADAQNumberEntryWithLabel(SpectrumAnalysis_GF, "Resolution", SpectrumResolution_NEL_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,5));
  SpectrumResolution_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  SpectrumResolution_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumResolution_NEL->GetEntry()->SetNumber(0.05);
  SpectrumResolution_NEL->GetEntry()->Resize(75,20);
  SpectrumResolution_NEL->GetEntry()->SetState(false);
  SpectrumResolution_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
  */

  SpectrumAnalysis_GF->AddFrame(SpectrumFindBackground_CB = new TGCheckButton(SpectrumAnalysis_GF, "Find background", SpectrumFindBackground_CB_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  SpectrumFindBackground_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");

  TGHorizontalFrame *SpectrumBackgroundRange_HF = new TGHorizontalFrame(SpectrumAnalysis_GF);
  SpectrumAnalysis_GF->AddFrame(SpectrumBackgroundRange_HF);
  
  SpectrumBackgroundRange_HF->AddFrame(SpectrumRangeMin_NEL = new ADAQNumberEntryWithLabel(SpectrumBackgroundRange_HF, "Min.", SpectrumRangeMin_NEL_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumRangeMin_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  SpectrumRangeMin_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumRangeMin_NEL->GetEntry()->SetNumber(0);
  SpectrumRangeMin_NEL->GetEntry()->Resize(75,20);
  SpectrumRangeMin_NEL->GetEntry()->SetState(false);
  SpectrumRangeMin_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

  SpectrumBackgroundRange_HF->AddFrame(SpectrumRangeMax_NEL = new ADAQNumberEntryWithLabel(SpectrumBackgroundRange_HF, "Max.", SpectrumRangeMax_NEL_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumRangeMax_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  SpectrumRangeMax_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  SpectrumRangeMax_NEL->GetEntry()->SetNumber(2000);
  SpectrumRangeMax_NEL->GetEntry()->Resize(75,20);
  SpectrumRangeMax_NEL->GetEntry()->SetState(false);
  SpectrumRangeMax_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

  TGHorizontalFrame *BackgroundPlotting_HF = new TGHorizontalFrame(SpectrumAnalysis_GF);
  SpectrumAnalysis_GF->AddFrame(BackgroundPlotting_HF, new TGLayoutHints(kLHintsNormal, 0,0,0,0));

  BackgroundPlotting_HF->AddFrame(SpectrumWithBackground_RB = new TGRadioButton(BackgroundPlotting_HF, "Plot with bckgnd", SpectrumWithBackground_RB_ID),
				  new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumWithBackground_RB->SetState(kButtonDown);
  SpectrumWithBackground_RB->SetState(kButtonDisabled);
  SpectrumWithBackground_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  
  BackgroundPlotting_HF->AddFrame(SpectrumLessBackground_RB = new TGRadioButton(BackgroundPlotting_HF, "Plot less bckgnd", SpectrumLessBackground_RB_ID),
				  new TGLayoutHints(kLHintsNormal, 5,5,0,5));
  SpectrumLessBackground_RB->SetState(kButtonDisabled);
  SpectrumLessBackground_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");

  SpectrumAnalysis_GF->AddFrame(SpectrumFindIntegral_CB = new TGCheckButton(SpectrumAnalysis_GF, "Find integral", SpectrumFindIntegral_CB_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  SpectrumFindIntegral_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  
  SpectrumAnalysis_GF->AddFrame(SpectrumIntegralInCounts_CB = new TGCheckButton(SpectrumAnalysis_GF, "Integral in counts", SpectrumIntegralInCounts_CB_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumIntegralInCounts_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  
  SpectrumAnalysis_GF->AddFrame(SpectrumUseGaussianFit_CB = new TGCheckButton(SpectrumAnalysis_GF, "Use gaussian fit", SpectrumUseGaussianFit_CB_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumUseGaussianFit_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");

  SpectrumAnalysis_GF->AddFrame(SpectrumNormalizePeakToCurrent_CB = new TGCheckButton(SpectrumAnalysis_GF, "Normalize peak to current", SpectrumNormalizePeakToCurrent_CB_ID),
				new TGLayoutHints(kLHintsNormal, 5,5,0,0));
  SpectrumNormalizePeakToCurrent_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");

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
  SpectrumOverplotDerivative_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");

  SpectrumFrame_VF->AddFrame(CreateSpectrum_TB = new TGTextButton(SpectrumFrame_VF, "Create spectrum", CreateSpectrum_TB_ID),
		       new TGLayoutHints(kLHintsCenterX | kLHintsTop, 5,5,0,0));
  CreateSpectrum_TB->Resize(200, 30);
  CreateSpectrum_TB->SetBackgroundColor(ColorManager->Number2Pixel(36));
  CreateSpectrum_TB->SetForegroundColor(ColorManager->Number2Pixel(0));
  CreateSpectrum_TB->ChangeOptions(CreateSpectrum_TB->GetOptions() | kFixedSize);
  CreateSpectrum_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");
  

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
  PSDEnable_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");

  PSDAnalysis_GF->AddFrame(PSDChannel_CBL = new ADAQComboBoxWithLabel(PSDAnalysis_GF, "", -1),
                           new TGLayoutHints(kLHintsNormal, 0,5,0,0));

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
  PSDPeakOffset_NEL->GetEntry()->Connect("ValueSet(long", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

  PSDAnalysis_GF->AddFrame(PSDTailOffset_NEL = new ADAQNumberEntryWithLabel(PSDAnalysis_GF, "Tail offset (sample)", PSDTailOffset_NEL_ID),
                           new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PSDTailOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PSDTailOffset_NEL->GetEntry()->SetNumber(29);
  PSDTailOffset_NEL->GetEntry()->SetState(false);
  PSDTailOffset_NEL->GetEntry()->Connect("ValueSet(long", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
  
  PSDAnalysis_GF->AddFrame(PSDPlotType_CBL = new ADAQComboBoxWithLabel(PSDAnalysis_GF, "Plot type", PSDPlotType_CBL_ID),
			   new TGLayoutHints(kLHintsNormal, 0,5,5,5));
  PSDPlotType_CBL->GetComboBox()->AddEntry("COL",0);
  PSDPlotType_CBL->GetComboBox()->AddEntry("LEGO2",1);
  PSDPlotType_CBL->GetComboBox()->AddEntry("SURF3",2);
  PSDPlotType_CBL->GetComboBox()->AddEntry("SURF4",3);
  PSDPlotType_CBL->GetComboBox()->AddEntry("CONT",4);
  PSDPlotType_CBL->GetComboBox()->Select(0);
  PSDPlotType_CBL->GetComboBox()->Connect("Selected(int,int)", "ADAQAnalysisGUI", this, "HandleComboBox(int,int)");
  PSDPlotType_CBL->GetComboBox()->SetEnabled(false);

  PSDAnalysis_GF->AddFrame(PSDPlotTailIntegration_CB = new TGCheckButton(PSDAnalysis_GF, "Plot tail integration region", PSDPlotTailIntegration_CB_ID),
                           new TGLayoutHints(kLHintsNormal, 0,5,0,5));
  PSDPlotTailIntegration_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  PSDPlotTailIntegration_CB->SetState(kButtonDisabled);
  
  PSDAnalysis_GF->AddFrame(PSDEnableHistogramSlicing_CB = new TGCheckButton(PSDAnalysis_GF, "Enable histogram slicing", PSDEnableHistogramSlicing_CB_ID),
			   new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  PSDEnableHistogramSlicing_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");

  TGHorizontalFrame *PSDHistogramSlicing_HF = new TGHorizontalFrame(PSDAnalysis_GF);
  PSDAnalysis_GF->AddFrame(PSDHistogramSlicing_HF, new TGLayoutHints(kLHintsCenterX, 0,5,0,5));
  
  PSDHistogramSlicing_HF->AddFrame(PSDHistogramSliceX_RB = new TGRadioButton(PSDHistogramSlicing_HF, "X slice", PSDHistogramSliceX_RB_ID),
				   new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  PSDHistogramSliceX_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  PSDHistogramSliceX_RB->SetState(kButtonDown);
  PSDHistogramSliceX_RB->SetState(kButtonDisabled);

  PSDHistogramSlicing_HF->AddFrame(PSDHistogramSliceY_RB = new TGRadioButton(PSDHistogramSlicing_HF, "Y slice", PSDHistogramSliceY_RB_ID),
				   new TGLayoutHints(kLHintsNormal, 20,0,0,0));
  PSDHistogramSliceY_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  PSDHistogramSliceY_RB->SetState(kButtonDisabled);
  
  PSDAnalysis_GF->AddFrame(PSDEnableFilterCreation_CB = new TGCheckButton(PSDAnalysis_GF, "Enable filter creation", PSDEnableFilterCreation_CB_ID),
			   new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  PSDEnableFilterCreation_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  PSDEnableFilterCreation_CB->SetState(kButtonDisabled);

  
  PSDAnalysis_GF->AddFrame(PSDEnableFilter_CB = new TGCheckButton(PSDAnalysis_GF, "Enable filter use", PSDEnableFilter_CB_ID),
			   new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PSDEnableFilter_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  PSDEnableFilter_CB->SetState(kButtonDisabled);

  TGHorizontalFrame *PSDFilterPolarity_HF = new TGHorizontalFrame(PSDAnalysis_GF);
  PSDAnalysis_GF->AddFrame(PSDFilterPolarity_HF);

  PSDFilterPolarity_HF->AddFrame(PSDPositiveFilter_RB = new TGRadioButton(PSDFilterPolarity_HF, "Positive  ", PSDPositiveFilter_RB_ID),
				 new TGLayoutHints(kLHintsNormal, 5,5,0,5));
  PSDPositiveFilter_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  PSDPositiveFilter_RB->SetState(kButtonDown);
  PSDPositiveFilter_RB->SetState(kButtonDisabled);

  PSDFilterPolarity_HF->AddFrame(PSDNegativeFilter_RB = new TGRadioButton(PSDFilterPolarity_HF, "Negative", PSDNegativeFilter_RB_ID),
				 new TGLayoutHints(kLHintsNormal, 5,5,0,5));
  PSDNegativeFilter_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  PSDNegativeFilter_RB->SetState(kButtonDisabled);

  
  PSDAnalysis_GF->AddFrame(PSDClearFilter_TB = new TGTextButton(PSDAnalysis_GF, "Clear filter", PSDClearFilter_TB_ID),
			   new TGLayoutHints(kLHintsNormal, 0,5,0,5));
  PSDClearFilter_TB->Resize(200,30);
  PSDClearFilter_TB->ChangeOptions(PSDClearFilter_TB->GetOptions() | kFixedSize);
  PSDClearFilter_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");
  PSDClearFilter_TB->SetState(kButtonDisabled);
  
  PSDAnalysis_GF->AddFrame(PSDCalculate_TB = new TGTextButton(PSDAnalysis_GF, "Create PSD histogram", PSDCalculate_TB_ID),
                           new TGLayoutHints(kLHintsNormal, 0,5,5,5));
  PSDCalculate_TB->Resize(200,30);
  PSDCalculate_TB->SetBackgroundColor(ColorManager->Number2Pixel(36));
  PSDCalculate_TB->SetForegroundColor(ColorManager->Number2Pixel(0));
  PSDCalculate_TB->ChangeOptions(PSDCalculate_TB->GetOptions() | kFixedSize);
  PSDCalculate_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");
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
  CalculateCountRate_TB->SetBackgroundColor(ColorManager->Number2Pixel(36));
  CalculateCountRate_TB->SetForegroundColor(ColorManager->Number2Pixel(0));
  CalculateCountRate_TB->Resize(200,30);
  CalculateCountRate_TB->ChangeOptions(CalculateCountRate_TB->GetOptions() | kFixedSize);
  CalculateCountRate_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");

  CountRate_GF->AddFrame(InstCountRate_NEFL = new ADAQNumberEntryFieldWithLabel(CountRate_GF, "Inst. count rate [1/s]", -1),
                         new TGLayoutHints(kLHintsNormal, 5,5,5,0));
  InstCountRate_NEFL->GetEntry()->Resize(100,20);
  InstCountRate_NEFL->GetEntry()->SetState(kButtonDisabled);
  InstCountRate_NEFL->GetEntry()->SetAlignment(kTextCenterX);
  InstCountRate_NEFL->GetEntry()->SetBackgroundColor(ColorManager->Number2Pixel(19));
  InstCountRate_NEFL->GetEntry()->ChangeOptions(InstCountRate_NEFL->GetEntry()->GetOptions() | kFixedSize);

  CountRate_GF->AddFrame(AvgCountRate_NEFL = new ADAQNumberEntryFieldWithLabel(CountRate_GF, "Avg. count rate [1/s]", -1),
                         new TGLayoutHints(kLHintsNormal, 5,5,0,5));
  AvgCountRate_NEFL->GetEntry()->Resize(100,20);
  AvgCountRate_NEFL->GetEntry()->SetState(kButtonDisabled);
  AvgCountRate_NEFL->GetEntry()->SetAlignment(kTextCenterX);
  AvgCountRate_NEFL->GetEntry()->SetBackgroundColor(ColorManager->Number2Pixel(19));
  AvgCountRate_NEFL->GetEntry()->ChangeOptions(AvgCountRate_NEFL->GetEntry()->GetOptions() | kFixedSize);


  /////////////////////////////////////////
  // Fill the graphics options tabbed frame

  TGCanvas *GraphicsFrame_C = new TGCanvas(GraphicsOptions_CF, 320, 705, kDoubleBorder);
  GraphicsOptions_CF->AddFrame(GraphicsFrame_C, new TGLayoutHints(kLHintsCenterX, 0,0,10,10));
  
  TGVerticalFrame *GraphicsFrame_VF = new TGVerticalFrame(GraphicsFrame_C->GetViewPort(), 320, 705);
  GraphicsFrame_C->SetContainer(GraphicsFrame_VF);
  
  GraphicsFrame_VF->AddFrame(WaveformDrawOptions_BG = new TGButtonGroup(GraphicsFrame_VF, "Waveform draw options", kHorizontalFrame),
			      new TGLayoutHints(kLHintsNormal, 5,5,5,5));
  
  DrawWaveformWithCurve_RB = new TGRadioButton(WaveformDrawOptions_BG, "Smooth curve   ", DrawWaveformWithCurve_RB_ID);
  DrawWaveformWithCurve_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  DrawWaveformWithCurve_RB->SetState(kButtonDown);
  
  DrawWaveformWithMarkers_RB = new TGRadioButton(WaveformDrawOptions_BG, "Markers   ", DrawWaveformWithMarkers_RB_ID);
  DrawWaveformWithMarkers_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  
  DrawWaveformWithBoth_RB = new TGRadioButton(WaveformDrawOptions_BG, "Both", DrawWaveformWithBoth_RB_ID);
  DrawWaveformWithBoth_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  
  GraphicsFrame_VF->AddFrame(SpectrumDrawOptions_BG = new TGButtonGroup(GraphicsFrame_VF, "Spectrum draw options", kHorizontalFrame),
			      new TGLayoutHints(kLHintsNormal, 5,5,5,5));
  
  DrawSpectrumWithCurve_RB = new TGRadioButton(SpectrumDrawOptions_BG, "Smooth curve   ", DrawSpectrumWithCurve_RB_ID);
  DrawSpectrumWithCurve_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  DrawSpectrumWithCurve_RB->SetState(kButtonDown);
  
  DrawSpectrumWithMarkers_RB = new TGRadioButton(SpectrumDrawOptions_BG, "Markers   ", DrawSpectrumWithMarkers_RB_ID);
  DrawSpectrumWithMarkers_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");

  DrawSpectrumWithBars_RB = new TGRadioButton(SpectrumDrawOptions_BG, "Bars", DrawSpectrumWithBars_RB_ID);
  DrawSpectrumWithBars_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");

  GraphicsFrame_VF->AddFrame(SetStatsOff_CB = new TGCheckButton(GraphicsFrame_VF, "Set statistics off", SetStatsOff_CB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,5,0));
  SetStatsOff_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  
  GraphicsFrame_VF->AddFrame(PlotVerticalAxisInLog_CB = new TGCheckButton(GraphicsFrame_VF, "Vertical axis in log.", PlotVerticalAxisInLog_CB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  PlotVerticalAxisInLog_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");

  GraphicsFrame_VF->AddFrame(PlotSpectrumDerivativeError_CB = new TGCheckButton(GraphicsFrame_VF, "Plot spectrum derivative error", PlotSpectrumDerivativeError_CB_ID),
			     new TGLayoutHints(kLHintsNormal, 15,5,0,0));
  PlotSpectrumDerivativeError_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  
  GraphicsFrame_VF->AddFrame(PlotAbsValueSpectrumDerivative_CB = new TGCheckButton(GraphicsFrame_VF, "Plot abs. value of spectrum derivative ", PlotAbsValueSpectrumDerivative_CB_ID),
			     new TGLayoutHints(kLHintsNormal, 15,5,0,0));
  PlotAbsValueSpectrumDerivative_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  
  GraphicsFrame_VF->AddFrame(AutoYAxisRange_CB = new TGCheckButton(GraphicsFrame_VF, "Auto. Y Axis Range (waveform only)", -1),
			       new TGLayoutHints(kLHintsLeft, 15,5,0,5));

  TGHorizontalFrame *ResetAxesLimits_HF = new TGHorizontalFrame(GraphicsFrame_VF);
  GraphicsFrame_VF->AddFrame(ResetAxesLimits_HF, new TGLayoutHints(kLHintsLeft,15,5,5,5));
  
  ResetAxesLimits_HF->AddFrame(ResetXAxisLimits_TB = new TGTextButton(ResetAxesLimits_HF, "Reset X Axis", ResetXAxisLimits_TB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,0,0));
  ResetXAxisLimits_TB->Resize(175, 40);
  ResetXAxisLimits_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");
  
  ResetAxesLimits_HF->AddFrame(ResetYAxisLimits_TB = new TGTextButton(ResetAxesLimits_HF, "Reset Y Axis", ResetYAxisLimits_TB_ID),
			       new TGLayoutHints(kLHintsLeft, 15,5,0,5));
  ResetYAxisLimits_TB->Resize(175, 40);
  ResetYAxisLimits_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");
  
  GraphicsFrame_VF->AddFrame(ReplotWaveform_TB = new TGTextButton(GraphicsFrame_VF, "Replot waveform", ReplotWaveform_TB_ID),
			       new TGLayoutHints(kLHintsCenterX, 15,5,5,5));
  ReplotWaveform_TB->SetBackgroundColor(ColorManager->Number2Pixel(36));
  ReplotWaveform_TB->SetForegroundColor(ColorManager->Number2Pixel(0));
  ReplotWaveform_TB->Resize(200,30);
  ReplotWaveform_TB->ChangeOptions(ReplotWaveform_TB->GetOptions() | kFixedSize);
  ReplotWaveform_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");

  GraphicsFrame_VF->AddFrame(ReplotSpectrum_TB = new TGTextButton(GraphicsFrame_VF, "Replot spectrum", ReplotSpectrum_TB_ID),
			     new TGLayoutHints(kLHintsCenterX, 15,5,5,5));
  ReplotSpectrum_TB->SetBackgroundColor(ColorManager->Number2Pixel(36));
  ReplotSpectrum_TB->SetForegroundColor(ColorManager->Number2Pixel(0));
  ReplotSpectrum_TB->Resize(200,30);
  ReplotSpectrum_TB->ChangeOptions(ReplotSpectrum_TB->GetOptions() | kFixedSize);
  ReplotSpectrum_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");
  
  GraphicsFrame_VF->AddFrame(ReplotSpectrumDerivative_TB = new TGTextButton(GraphicsFrame_VF, "Replot spectrum derivative", ReplotSpectrumDerivative_TB_ID),
			     new TGLayoutHints(kLHintsCenterX, 15,5,5,5));
  ReplotSpectrumDerivative_TB->Resize(200, 30);
  ReplotSpectrumDerivative_TB->SetBackgroundColor(ColorManager->Number2Pixel(36));
  ReplotSpectrumDerivative_TB->SetForegroundColor(ColorManager->Number2Pixel(0));
  ReplotSpectrumDerivative_TB->ChangeOptions(ReplotSpectrumDerivative_TB->GetOptions() | kFixedSize);
  ReplotSpectrumDerivative_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");

  GraphicsFrame_VF->AddFrame(ReplotPSDHistogram_TB = new TGTextButton(GraphicsFrame_VF, "Replot PSD histogram", ReplotPSDHistogram_TB_ID),
			       new TGLayoutHints(kLHintsCenterX, 15,5,5,5));
  ReplotPSDHistogram_TB->SetBackgroundColor(ColorManager->Number2Pixel(36));
  ReplotPSDHistogram_TB->SetForegroundColor(ColorManager->Number2Pixel(0));
  ReplotPSDHistogram_TB->Resize(200,30);
  ReplotPSDHistogram_TB->ChangeOptions(ReplotPSDHistogram_TB->GetOptions() | kFixedSize);
  ReplotPSDHistogram_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");

  // Override default plot options
  GraphicsFrame_VF->AddFrame(OverrideTitles_CB = new TGCheckButton(GraphicsFrame_VF, "Override default graphical options", OverrideTitles_CB_ID),
			       new TGLayoutHints(kLHintsNormal, 5,5,5,5));
  OverrideTitles_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");

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
  XAxisSize_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
  
  XAxisTitleOptions_HF->AddFrame(XAxisOffset_NEL = new ADAQNumberEntryWithLabel(XAxisTitleOptions_HF, "Offset", XAxisOffset_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 5,2,0,0));
  XAxisOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  XAxisOffset_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  XAxisOffset_NEL->GetEntry()->SetNumber(1.5);
  XAxisOffset_NEL->GetEntry()->Resize(50, 20);
  XAxisOffset_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

  XAxis_GF->AddFrame(XAxisDivs_NEL = new ADAQNumberEntryWithLabel(XAxis_GF, "Divisions", XAxisDivs_NEL_ID),		
		     new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  XAxisDivs_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  XAxisDivs_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  XAxisDivs_NEL->GetEntry()->SetNumber(510);
  XAxisDivs_NEL->GetEntry()->Resize(50, 20);
  XAxisDivs_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

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
  YAxisSize_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
  
  YAxisTitleOptions_HF->AddFrame(YAxisOffset_NEL = new ADAQNumberEntryWithLabel(YAxisTitleOptions_HF, "Offset", YAxisOffset_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 5,2,0,0));
  YAxisOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  YAxisOffset_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  YAxisOffset_NEL->GetEntry()->SetNumber(1.5);
  YAxisOffset_NEL->GetEntry()->Resize(50, 20);
  YAxisOffset_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

  YAxis_GF->AddFrame(YAxisDivs_NEL = new ADAQNumberEntryWithLabel(YAxis_GF, "Divisions", YAxisDivs_NEL_ID),		
		     new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  YAxisDivs_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  YAxisDivs_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  YAxisDivs_NEL->GetEntry()->SetNumber(510);
  YAxisDivs_NEL->GetEntry()->Resize(50, 20);
  YAxisDivs_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

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
  ZAxisSize_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
  
  ZAxisTitleOptions_HF->AddFrame(ZAxisOffset_NEL = new ADAQNumberEntryWithLabel(ZAxisTitleOptions_HF, "Offset", ZAxisOffset_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 5,2,0,0));
  ZAxisOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  ZAxisOffset_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  ZAxisOffset_NEL->GetEntry()->SetNumber(1.5);
  ZAxisOffset_NEL->GetEntry()->Resize(50, 20);
  ZAxisOffset_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

  ZAxis_GF->AddFrame(ZAxisDivs_NEL = new ADAQNumberEntryWithLabel(ZAxis_GF, "Divisions", ZAxisDivs_NEL_ID),		
		     new TGLayoutHints(kLHintsNormal, 0,0,0,0));
  ZAxisDivs_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  ZAxisDivs_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  ZAxisDivs_NEL->GetEntry()->SetNumber(510);
  ZAxisDivs_NEL->GetEntry()->Resize(50, 20);
  ZAxisDivs_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

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
  PaletteAxisSize_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
  
  PaletteAxisTitleOptions_HF->AddFrame(PaletteAxisOffset_NEL = new ADAQNumberEntryWithLabel(PaletteAxisTitleOptions_HF, "Offset", PaletteAxisOffset_NEL_ID),		
				 new TGLayoutHints(kLHintsNormal, 5,2,0,0));
  PaletteAxisOffset_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESReal);
  PaletteAxisOffset_NEL->GetEntry()->SetNumAttr(TGNumberFormat::kNEAPositive);
  PaletteAxisOffset_NEL->GetEntry()->SetNumber(1.1);
  PaletteAxisOffset_NEL->GetEntry()->Resize(50, 20);
  PaletteAxisOffset_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");

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
  ProcessingSeq_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  //ProcessingSeq_RB->SetState(kButtonDown);
  
  ProcessingPar_RB = new TGRadioButton(ProcessingType_BG, "Parallel", ProcessingPar_RB_ID);
  ProcessingPar_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  ProcessingPar_RB->SetState(kButtonDown);
  
  // Processing processing options
  
  TGGroupFrame *ProcessingOptions_GF = new TGGroupFrame(ProcessingFrame_VF, "Processing options", kVerticalFrame);
  ProcessingFrame_VF->AddFrame(ProcessingOptions_GF, new TGLayoutHints(kLHintsNormal, 15,5,5,5));
  
  ProcessingOptions_GF->AddFrame(NumProcessors_NEL = new ADAQNumberEntryWithLabel(ProcessingOptions_GF, "Number of Processors", -1),
				 new TGLayoutHints(kLHintsNormal, 0,0,5,0));
  NumProcessors_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  NumProcessors_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  NumProcessors_NEL->GetEntry()->SetLimitValues(2,8);
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
  IntegratePearson_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");
  
  PearsonAnalysis_GF->AddFrame(PearsonChannel_CBL = new ADAQComboBoxWithLabel(PearsonAnalysis_GF, "Contains signal", -1),
			       new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  
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
  PearsonPolarityPositive_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");

  PearsonPolarity_HF->AddFrame(PearsonPolarityNegative_RB = new TGRadioButton(PearsonPolarity_HF, "Negative", PearsonPolarityNegative_RB_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PearsonPolarityNegative_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");

  TGHorizontalFrame *PearsonIntegrationType_HF = new TGHorizontalFrame(PearsonAnalysis_GF);
  PearsonAnalysis_GF->AddFrame(PearsonIntegrationType_HF, new TGLayoutHints(kLHintsNormal));

  PearsonIntegrationType_HF->AddFrame(IntegrateRawPearson_RB = new TGRadioButton(PearsonIntegrationType_HF, "Integrate raw", IntegrateRawPearson_RB_ID),
				      new TGLayoutHints(kLHintsLeft, 0,5,5,0));
  IntegrateRawPearson_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  IntegrateRawPearson_RB->SetState(kButtonDown);
  IntegrateRawPearson_RB->SetState(kButtonDisabled);

  PearsonIntegrationType_HF->AddFrame(IntegrateFitToPearson_RB = new TGRadioButton(PearsonIntegrationType_HF, "Integrate fit", IntegrateFitToPearson_RB_ID),
				      new TGLayoutHints(kLHintsLeft, 10,5,5,0));
  IntegrateFitToPearson_RB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleRadioButtons()");
  IntegrateFitToPearson_RB->SetState(kButtonDisabled);
  
  PearsonAnalysis_GF->AddFrame(PearsonLowerLimit_NEL = new ADAQNumberEntryWithLabel(PearsonAnalysis_GF, "Lower limit (sample)", PearsonLowerLimit_NEL_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PearsonLowerLimit_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PearsonLowerLimit_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  PearsonLowerLimit_NEL->GetEntry()->SetLimitValues(0,1); // Updated when ADAQ ROOT file loaded
  PearsonLowerLimit_NEL->GetEntry()->SetNumber(0); // Updated when ADAQ ROOT file loaded
  PearsonLowerLimit_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
  PearsonLowerLimit_NEL->GetEntry()->SetState(false);
  
  PearsonAnalysis_GF->AddFrame(PearsonMiddleLimit_NEL = new ADAQNumberEntryWithLabel(PearsonAnalysis_GF, "Middle limit (sample)", PearsonMiddleLimit_NEL_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PearsonMiddleLimit_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PearsonMiddleLimit_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  PearsonMiddleLimit_NEL->GetEntry()->SetLimitValues(0,1); // Updated when ADAQ ROOT file loaded
  PearsonMiddleLimit_NEL->GetEntry()->SetNumber(0); // Updated when ADAQ ROOT file loaded
  PearsonMiddleLimit_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
  PearsonMiddleLimit_NEL->GetEntry()->SetState(false);
  
  PearsonAnalysis_GF->AddFrame(PearsonUpperLimit_NEL = new ADAQNumberEntryWithLabel(PearsonAnalysis_GF, "Upper limit (sample))", PearsonUpperLimit_NEL_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,0,0));
  PearsonUpperLimit_NEL->GetEntry()->SetNumStyle(TGNumberFormat::kNESInteger);
  PearsonUpperLimit_NEL->GetEntry()->SetNumLimits(TGNumberFormat::kNELLimitMinMax);
  PearsonUpperLimit_NEL->GetEntry()->SetLimitValues(0,1); // Updated when ADAQ ROOT file loaded
  PearsonUpperLimit_NEL->GetEntry()->SetNumber(0); // Updated when ADAQ ROOT file loaded
  PearsonUpperLimit_NEL->GetEntry()->Connect("ValueSet(long)", "ADAQAnalysisGUI", this, "HandleNumberEntries()");
  PearsonUpperLimit_NEL->GetEntry()->SetState(false);
  
  PearsonAnalysis_GF->AddFrame(PlotPearsonIntegration_CB = new TGCheckButton(PearsonAnalysis_GF, "Plot integration", PlotPearsonIntegration_CB_ID),
			       new TGLayoutHints(kLHintsNormal, 0,5,5,0));
  PlotPearsonIntegration_CB->SetState(kButtonDisabled);
  PlotPearsonIntegration_CB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleCheckButtons()");

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
  DesplicedFileSelection_TB->SetBackgroundColor(ColorManager->Number2Pixel(18));
  DesplicedFileSelection_TB->ChangeOptions(DesplicedFileSelection_TB->GetOptions() | kFixedSize);
  DesplicedFileSelection_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");
  
  WaveformDesplicerName_HF->AddFrame(DesplicedFileName_TE = new TGTextEntry(WaveformDesplicerName_HF, "<No file currently selected>", -1),
				     new TGLayoutHints(kLHintsLeft, 5,0,5,5));
  DesplicedFileName_TE->Resize(180,25);
  DesplicedFileName_TE->SetAlignment(kTextRight);
  DesplicedFileName_TE->SetBackgroundColor(ColorManager->Number2Pixel(18));
  DesplicedFileName_TE->ChangeOptions(DesplicedFileName_TE->GetOptions() | kFixedSize);
  
  WaveformDesplicer_GF->AddFrame(DesplicedFileCreation_TB = new TGTextButton(WaveformDesplicer_GF, "Create despliced file", DesplicedFileCreation_TB_ID),
				 new TGLayoutHints(kLHintsLeft, 0,0,5,5));
  DesplicedFileCreation_TB->Resize(200, 30);
  DesplicedFileCreation_TB->SetBackgroundColor(ColorManager->Number2Pixel(36));
  DesplicedFileCreation_TB->SetForegroundColor(ColorManager->Number2Pixel(0));
  DesplicedFileCreation_TB->ChangeOptions(DesplicedFileCreation_TB->GetOptions() | kFixedSize);
  DesplicedFileCreation_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTextButtons()");


  /////////////////////////////////////
  // Canvas and associated widget frame

  TGVerticalFrame *Canvas_VF = new TGVerticalFrame(OptionsAndCanvas_HF);
  Canvas_VF->SetBackgroundColor(ColorManager->Number2Pixel(16));
  OptionsAndCanvas_HF->AddFrame(Canvas_VF, new TGLayoutHints(kLHintsNormal, 5,5,5,5));

  // ADAQ ROOT file name text entry
  Canvas_VF->AddFrame(FileName_TE = new TGTextEntry(Canvas_VF, "<No file currently selected>", -1),
		      new TGLayoutHints(kLHintsTop, 30,5,0,5));
  FileName_TE->Resize(700,20);
  FileName_TE->SetState(false);
  FileName_TE->SetAlignment(kTextCenterX);
  FileName_TE->SetBackgroundColor(ColorManager->Number2Pixel(18));
  FileName_TE->ChangeOptions(FileName_TE->GetOptions() | kFixedSize);

  // File statistics horizontal frame

  TGHorizontalFrame *FileStats_HF = new TGHorizontalFrame(Canvas_VF,700,30);
  FileStats_HF->SetBackgroundColor(ColorManager->Number2Pixel(16));
  Canvas_VF->AddFrame(FileStats_HF, new TGLayoutHints(kLHintsTop | kLHintsCenterX, 5,5,0,5));

  // Waveforms

  TGHorizontalFrame *WaveformNELAndLabel_HF = new TGHorizontalFrame(FileStats_HF);
  WaveformNELAndLabel_HF->SetBackgroundColor(ColorManager->Number2Pixel(16));
  FileStats_HF->AddFrame(WaveformNELAndLabel_HF, new TGLayoutHints(kLHintsLeft, 5,35,0,0));

  WaveformNELAndLabel_HF->AddFrame(Waveforms_NEL = new TGNumberEntryField(WaveformNELAndLabel_HF,-1),
				  new TGLayoutHints(kLHintsLeft,5,5,0,0));
  Waveforms_NEL->SetBackgroundColor(ColorManager->Number2Pixel(18));
  Waveforms_NEL->SetAlignment(kTextCenterX);
  Waveforms_NEL->SetState(false);
  
  WaveformNELAndLabel_HF->AddFrame(WaveformLabel_L = new TGLabel(WaveformNELAndLabel_HF, " Waveforms "),
				   new TGLayoutHints(kLHintsLeft,0,5,3,0));
  WaveformLabel_L->SetBackgroundColor(ColorManager->Number2Pixel(18));

  // Record length

  TGHorizontalFrame *RecordLengthNELAndLabel_HF = new TGHorizontalFrame(FileStats_HF);
  RecordLengthNELAndLabel_HF->SetBackgroundColor(ColorManager->Number2Pixel(16));
  FileStats_HF->AddFrame(RecordLengthNELAndLabel_HF, new TGLayoutHints(kLHintsLeft, 5,5,0,0));

  RecordLengthNELAndLabel_HF->AddFrame(RecordLength_NEL = new TGNumberEntryField(RecordLengthNELAndLabel_HF,-1),
				      new TGLayoutHints(kLHintsLeft,5,5,0,0));
  RecordLength_NEL->SetBackgroundColor(ColorManager->Number2Pixel(18));
  RecordLength_NEL->SetAlignment(kTextCenterX);
  RecordLength_NEL->SetState(false);

  RecordLengthNELAndLabel_HF->AddFrame(RecordLengthLabel_L = new TGLabel(RecordLengthNELAndLabel_HF, " Record length "),
				       new TGLayoutHints(kLHintsLeft,0,5,3,0));
  RecordLengthLabel_L->SetBackgroundColor(ColorManager->Number2Pixel(18));

  // The embedded canvas

  TGHorizontalFrame *Canvas_HF = new TGHorizontalFrame(Canvas_VF);
  Canvas_VF->AddFrame(Canvas_HF, new TGLayoutHints(kLHintsCenterX));

  Canvas_HF->AddFrame(YAxisLimits_DVS = new TGDoubleVSlider(Canvas_HF, 500, kDoubleScaleBoth, YAxisLimits_DVS_ID),
		      new TGLayoutHints(kLHintsTop));
  YAxisLimits_DVS->SetRange(0,1);
  YAxisLimits_DVS->SetPosition(0,1);
  YAxisLimits_DVS->Connect("PositionChanged()", "ADAQAnalysisGUI", this, "HandleDoubleSliders()");
  
  Canvas_HF->AddFrame(Canvas_EC = new TRootEmbeddedCanvas("TheCanvas_EC", Canvas_HF, 700, 500),
		      new TGLayoutHints(kLHintsCenterX, 5,5,5,5));
  Canvas_EC->GetCanvas()->SetFillColor(0);
  Canvas_EC->GetCanvas()->SetFrameFillColor(19);
  Canvas_EC->GetCanvas()->SetGrid();
  Canvas_EC->GetCanvas()->SetBorderMode(0);
  Canvas_EC->GetCanvas()->SetLeftMargin(0.13);
  Canvas_EC->GetCanvas()->SetBottomMargin(0.12);
  Canvas_EC->GetCanvas()->SetRightMargin(0.05);
  Canvas_EC->GetCanvas()->Connect("ProcessedEvent(int, int, int, TObject *)", "ADAQAnalysisGUI", this, "HandleCanvas(int, int, int, TObject *)");

  Canvas_VF->AddFrame(XAxisLimits_THS = new TGTripleHSlider(Canvas_VF, 700, kDoubleScaleBoth, XAxisLimits_THS_ID),
		      new TGLayoutHints(kLHintsRight));
  XAxisLimits_THS->SetRange(0,1);
  XAxisLimits_THS->SetPosition(0,1);
  XAxisLimits_THS->SetPointerPosition(0.5);
  XAxisLimits_THS->Connect("PositionChanged()", "ADAQAnalysisGUI", this, "HandleDoubleSliders()");
  XAxisLimits_THS->Connect("PointerPositionChanged()", "ADAQAnalysisGUI", this, "HandleTripleSliderPointer()");
    
  Canvas_VF->AddFrame(new TGLabel(Canvas_VF, "  Waveform selector  "),
		      new TGLayoutHints(kLHintsCenterX, 5,5,15,0));

  Canvas_VF->AddFrame(WaveformSelector_HS = new TGHSlider(Canvas_VF, 700, kSlider1),
		      new TGLayoutHints(kLHintsRight));
  WaveformSelector_HS->SetRange(1,100);
  WaveformSelector_HS->SetPosition(1);
  WaveformSelector_HS->Connect("PositionChanged(int)", "ADAQAnalysisGUI", this, "HandleSliders(int)");

  Canvas_VF->AddFrame(new TGLabel(Canvas_VF, "  Spectrum integration limits  "),
		      new TGLayoutHints(kLHintsCenterX, 5,5,15,0));

  Canvas_VF->AddFrame(SpectrumIntegrationLimits_DHS = new TGDoubleHSlider(Canvas_VF, 700, kDoubleScaleBoth, SpectrumIntegrationLimits_DHS_ID),
		      new TGLayoutHints(kLHintsRight));
  SpectrumIntegrationLimits_DHS->SetRange(0,1);
  SpectrumIntegrationLimits_DHS->SetPosition(0,1);
  SpectrumIntegrationLimits_DHS->Connect("PositionChanged()", "ADAQAnalysisGUI", this, "HandleDoubleSliders()");
		      
  TGHorizontalFrame *SubCanvas_HF = new TGHorizontalFrame(Canvas_VF);
  SubCanvas_HF->SetBackgroundColor(ColorManager->Number2Pixel(16));
  Canvas_VF->AddFrame(SubCanvas_HF, new TGLayoutHints(kLHintsRight | kLHintsBottom, 5,5,20,5));
  
  SubCanvas_HF->AddFrame(ProcessingProgress_PB = new TGHProgressBar(SubCanvas_HF, TGProgressBar::kFancy, 400),
			 new TGLayoutHints(kLHintsLeft, 5,55,7,5));
  ProcessingProgress_PB->SetBarColor(ColorManager->Number2Pixel(41));
  ProcessingProgress_PB->ShowPosition(kTRUE, kFALSE, "%0.f% waveforms processed");
  
  SubCanvas_HF->AddFrame(Quit_TB = new TGTextButton(SubCanvas_HF, "Access standby", Quit_TB_ID),
			 new TGLayoutHints(kLHintsRight, 5,5,0,5));
  Quit_TB->Resize(200, 40);
  Quit_TB->SetBackgroundColor(ColorManager->Number2Pixel(36));
  Quit_TB->SetForegroundColor(ColorManager->Number2Pixel(0));
  Quit_TB->ChangeOptions(Quit_TB->GetOptions() | kFixedSize);
  Quit_TB->Connect("Clicked()", "ADAQAnalysisGUI", this, "HandleTerminate()");

  
  //////////////////////////////////////
  // Finalize options and map windows //
  //////////////////////////////////////
  
  SetBackgroundColor(ColorManager->Number2Pixel(16));
  
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

void ADAQAnalysisGUI::HandleMenu(int MenuID)
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
      // Set the file name to a class member object. Note that the
      // FileInformation.fFilename variable storeds the absolute path
      // to the data file selected by the user
      ADAQRootFileName = FileInformation.fFilename;

      // Strip the data file name off the absolute file path and set
      // the path to the DataDirectory variable. Thus, the "current"
      // directory from which a file was selected will become the new
      // default directory that automically openso
      size_t pos = ADAQRootFileName.find_last_of("/");
      if(pos != string::npos)
	DataDirectory  = ADAQRootFileName.substr(0,pos);
      
      // Load the ROOT file
      LoadADAQRootFile();
    }
    break;
  }

  case MenuFileSaveSpectrum_ID:{
    
    if(!SpectrumExists){
      CreateMessageBox("No spectra have been created yet and, therefore, there is nothing to save!","Stop");
      break;
    }
    
    SaveHistogramData(Spectrum_H);
    break;
  }

  case MenuFileSaveSpectrumDerivative_ID:{

    if(!SpectrumDerivativeExists){
      CreateMessageBox("No spectrum derivatives have been created yet and, therefore, there is nothing to save!","Stop");
      break;
    }

    SaveHistogramData(SpectrumDerivative_H);
    break;
  }
    
  case MenuFileSavePSDHistogramSlice_ID:{
    
    if(!PSDHistogramSliceExists){
      CreateMessageBox("No PSD histogram slices have been created yet and, therefore, there is nothing to save!","Stop");
      break;
    }

    SaveHistogramData(PSDHistogramSlice_H);
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
    FileInformation.fIniDir = StrDup(PrintCanvasDirectory.c_str());

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
	PrintCanvasDirectory  = ADAQRootFileName.substr(0,Found);
      
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


void ADAQAnalysisGUI::HandleTextButtons()
{
  if(!ADAQRootFileLoaded)
    return;
  
  // Get the currently active widget and its ID, i.e. the widget from
  // which the user has just sent a signal...
  TGTextButton *ActiveTextButton = (TGTextButton *) gTQSender;
  int ActiveTextButtonID  = ActiveTextButton->WidgetId();

  switch(ActiveTextButtonID){

  case Quit_TB_ID:
    HandleTerminate();
    break;

  case ResetXAxisLimits_TB_ID:
    if(!ADAQRootFileLoaded)
      return;

    // Reset the XAxis double slider and member data
    XAxisLimits_THS->SetPosition(0, RecordLength);
    XAxisMin = 0;
    XAxisMax = RecordLength;
    
    PlotWaveform();
    break;

  case ResetYAxisLimits_TB_ID:
    if(!ADAQRootFileLoaded)
      return;
    
    // Reset the YAxis double slider and member data
    YAxisLimits_DVS->SetPosition(0, V1720MaximumBit);
    YAxisMin = 0;
    YAxisMax = V1720MaximumBit;

    PlotWaveform();
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
    int CurrentChannel = ChannelSelector_CBL->GetComboBox()->GetSelected();

    if(SetPoint == CalibrationData[CurrentChannel].PointID.size()){
      
      CalibrationData[CurrentChannel].PointID.push_back(SetPoint);
      CalibrationData[CurrentChannel].Energy.push_back(Energy);
      CalibrationData[CurrentChannel].PulseUnit.push_back(PulseUnit);
      
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
    
    // ...or if the user is re-setting previously set points then
    // simply overwrite the preexisting values in the vectors
    else{
      CalibrationData[CurrentChannel].Energy[SetPoint] = Energy;
      CalibrationData[CurrentChannel].PulseUnit[SetPoint] = PulseUnit;
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
    int CurrentChannel = ChannelSelector_CBL->GetComboBox()->GetSelected();
    
    if(CalibrationData[CurrentChannel].PointID.size() >= 2){

      // Determine the total number of calibration points in the
      // current channel's calibration data set
      const int NumCalibrationPoints = CalibrationData[CurrentChannel].PointID.size();

      // Create a new "CalibrationManager" TGraph object.
      CalibrationManager[CurrentChannel] = new TGraph(NumCalibrationPoints,
						      &CalibrationData[CurrentChannel].PulseUnit[0],
						      &CalibrationData[CurrentChannel].Energy[0]);
      
      // Set the current channel's calibration boolean to true,
      // indicating that the current channel will convert pulse units
      // to energy within the acquisition loop before histogramming
      // the result into the channel's spectrum
      UseCalibrationManager[CurrentChannel] = true;
    }
    break;
  }

    /////////////////////////////////////////////////////
    // Actions for plotting the CalibrationManager TGraph
    
  case SpectrumCalibrationPlot_TB_ID:{
    
    int CurrentChannel = ChannelSelector_CBL->GetComboBox()->GetSelected();
    
    if(UseCalibrationManager[CurrentChannel]){
      stringstream ss;
      ss << "CalibrationManager TGraph for Channel[" << CurrentChannel << "]";
      string Title = ss.str();

      CalibrationManager[CurrentChannel]->SetTitle(Title.c_str());

      CalibrationManager[CurrentChannel]->GetXaxis()->SetTitle("Pulse unit [ADC]");
      CalibrationManager[CurrentChannel]->GetXaxis()->SetTitleSize(0.05);
      CalibrationManager[CurrentChannel]->GetXaxis()->SetTitleOffset(1.2);
      CalibrationManager[CurrentChannel]->GetXaxis()->CenterTitle();
      CalibrationManager[CurrentChannel]->GetXaxis()->SetLabelSize(0.05);

      CalibrationManager[CurrentChannel]->GetYaxis()->SetTitle("Energy");
      CalibrationManager[CurrentChannel]->GetYaxis()->SetTitleSize(0.05);
      CalibrationManager[CurrentChannel]->GetYaxis()->SetTitleOffset(1.2);
      CalibrationManager[CurrentChannel]->GetYaxis()->CenterTitle();
      CalibrationManager[CurrentChannel]->GetYaxis()->SetLabelSize(0.05);

      CalibrationManager[CurrentChannel]->SetMarkerSize(2);
      CalibrationManager[CurrentChannel]->SetMarkerStyle(23);
      CalibrationManager[CurrentChannel]->SetMarkerColor(1);
      CalibrationManager[CurrentChannel]->SetLineWidth(2);
      CalibrationManager[CurrentChannel]->SetLineColor(1);
      CalibrationManager[CurrentChannel]->Draw("ALP");

      Canvas_EC->GetCanvas()->Update();
    }
    break;
  }

    ///////////////////////////////////////////
    // Actions for resetting CalibrationManager
    
  case SpectrumCalibrationReset_TB_ID:{

    // Get the current channel being histogrammed in 
    int CurrentChannel = ChannelSelector_CBL->GetComboBox()->GetSelected();

    // Clear the channel calibration vectors for the current channel
    CalibrationData[CurrentChannel].PointID.clear();
    CalibrationData[CurrentChannel].Energy.clear();
    CalibrationData[CurrentChannel].PulseUnit.clear();

    // Delete the current channel's depracated calibration manager
    // TGraph object to prevent memory leaks but reallocat the TGraph
    // object to prevent seg. faults when writing the
    // CalibrationManager to the ROOT parallel processing file
    if(UseCalibrationManager[CurrentChannel]){
      delete CalibrationManager[CurrentChannel];
      CalibrationManager[CurrentChannel] = new TGraph;
    }
    
    // Set the current channel's calibration boolean to false,
    // indicating that the calibration manager will NOT be used within
    // the acquisition loop
    UseCalibrationManager[CurrentChannel] = false;
    
    // Reset the calibration widgets
    SpectrumCalibrationPoint_CBL->GetComboBox()->RemoveAll();
    SpectrumCalibrationPoint_CBL->GetComboBox()->AddEntry("Point 0", 0);
    SpectrumCalibrationPoint_CBL->GetComboBox()->Select(0);
    SpectrumCalibrationEnergy_NEL->GetEntry()->SetNumber(0.0);
    SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(1.0);
    break;
  }

    /////////////////////////////////
    // Actions to replot the spectrum

  case ReplotWaveform_TB_ID:
    PlotWaveform();
    break;

    /////////////////////////////////
    // Actions to replot the spectrum
    
  case ReplotSpectrum_TB_ID:
    
    if(SpectrumExists)
      PlotSpectrum();
    else
      CreateMessageBox("A valid spectrum does not yet exist; therefore, it is difficult to replot it!","Stop");
    break;

    ////////////////////////////////////////////
    // Actions to replot the spectrum derivative
    
  case ReplotSpectrumDerivative_TB_ID:
    
    if(SpectrumExists){
      SpectrumOverplotDerivative_CB->SetState(kButtonUp);
      PlotSpectrumDerivative();
    }
    else
      CreateMessageBox("A valid spectrum does not yet exist; therefore, the spectrum derivative cannot be plotted!","Stop");
    break;
      
    //////////////////////////////////////
    // Actions to replot the PSD histogram
    
  case ReplotPSDHistogram_TB_ID:
    
    if(PSDHistogram_H)
      PlotPSDHistogram();
    else
      CreateMessageBox("A valid PSD histogram does not yet exist; therefore, replotting cannot be achieved!","Stop");
    break;
    
  case CreateSpectrum_TB_ID:
    if(!ADAQRootFileLoaded)
      return;

    // Alert the user the filtering particles by PSD into the spectra
    // requires integration type peak finder to be used
    //    if(UsePSDFilterManager[PSDChannel] and IntegrationTypeWholeWaveform)
    //      CreateMessageBox("Warning! Use of the PSD filter with spectra creation requires peak finding integration","Asterisk");
    
    // Create spectrum with sequential processing
    if(ProcessingSeq_RB->IsDown()){
      CreateSpectrum();
      PlotSpectrum();
      
      if(SpectrumOverplotDerivative_CB->IsDown())
	PlotSpectrumDerivative();
    }
    
    // Create spectrum with parallel processing
    else
      ProcessWaveformsInParallel("histogramming");
    break;

  case CountRate_TB_ID:
    CalculateCountRate();
    break;
    
  case PSDCalculate_TB_ID:
    if(!ADAQRootFileLoaded)
      return;
    
    if(ProcessingSeq_RB->IsDown()){
      CreatePSDHistogram();
      PlotPSDHistogram();
    }
    else
      ProcessWaveformsInParallel("discriminating");
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
    size_t Pos = ADAQRootFileName.find_last_of("/");
    if(Pos != string::npos){
      string RawFileName = ADAQRootFileName.substr(Pos+1, ADAQRootFileName.size());

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
    
    if(!ADAQRootFileLoaded)
      return;

    // Alert the user the filtering particles by PSD into the spectra
    // requires integration type peak finder to be used
    //if(UsePSDFilterManager[PSDChannel] and IntegrationTypeWholeWaveform)
    //      CreateMessageBox("Warning! Use of the PSD filter with spectra creation requires peak finding integration","Asterisk");
    
    // Desplice waveforms sequentially
    if(ProcessingSeq_RB->IsDown())
      CreateDesplicedFile();

    // Desplice waveforms in parallel
    else
      ProcessWaveformsInParallel("desplicing");

    break;


  case PSDClearFilter_TB_ID:
    PSDNumFilterPoints = 0;
    PSDFilterXPoints.clear();
    PSDFilterYPoints.clear();

    if(PSDFilterManager[PSDChannel]) delete PSDFilterManager[PSDChannel];
    PSDFilterManager[Channel] = new TGraph();
    
    PSDEnableFilter_CB->SetState(kButtonUp);

    switch(CanvasContents){
    case zWaveform:
      PlotWaveform();
      break;
      
    case zSpectrum:
      PlotSpectrum();
      break;

    case zPSDHistogram:
      PlotPSDHistogram();
      break;
    }
    break;
  }


}


void ADAQAnalysisGUI::HandleCheckButtons()
{
  if(!ADAQRootFileLoaded)
    return;
  
  // Get the currently active widget and its ID, i.e. the widget from
  // which the user has just sent a signal...
  TGCheckButton *ActiveCheckButton = (TGCheckButton *) gTQSender;
  int CheckButtonID = ActiveCheckButton->WidgetId();
  
  switch(CheckButtonID){
    
  case FindPeaks_CB_ID:

    if(!ADAQRootFile)
      return;

    // Delete the previoue TSpectrum PeakFinder object if it exists to
    // prevent memory leaks
    if(PeakFinder)
      delete PeakFinder;
    
    // Create a TSpectrum PeakFinder using the appropriate widget to set 
    // the maximum number of peaks that can be found
    PeakFinder = new TSpectrum(MaxPeaks_NEL->GetEntry()->GetIntNumber());
    
    PlotWaveform();
    break;

  case PlotZeroSuppressionCeiling_CB_ID:
  case PlotFloor_CB_ID:
  case PlotCrossings_CB_ID:
  case PlotPeakIntegratingRegion_CB_ID:
  case PlotTrigger_CB_ID:
  case PlotBaseline_CB_ID:
  case UsePileupRejection_CB_ID:
  case PlotPearsonIntegration_CB_ID:
    PlotWaveform();
    break;

  case OverrideTitles_CB_ID:
  case PlotVerticalAxisInLog_CB_ID:
  case SetStatsOff_CB_ID:

    switch(CanvasContents){
    case zWaveform:
      PlotWaveform();
      break;
      
    case zSpectrum:{
      PlotSpectrum();
      if(SpectrumOverplotDerivative_CB->IsDown())
	PlotSpectrumDerivative();
      break;
    }
      
    case zSpectrumDerivative:
      PlotSpectrumDerivative();
      break;
      
    case zPSDHistogram:
      PlotPSDHistogram();
      break;
    }
    break;
    
  case SpectrumCalibration_CB_ID:{
    
    if(SpectrumCalibration_CB->IsDown()){
      SpectrumCalibrationManual_RB->SetState(kButtonUp);
      SpectrumCalibrationFixedEP_RB->SetState(kButtonUp);

      SetCalibrationWidgetState(true, kButtonUp);
    }
    else{
      SpectrumCalibrationManual_RB->SetState(kButtonDisabled);
      SpectrumCalibrationFixedEP_RB->SetState(kButtonDisabled);

      SetCalibrationWidgetState(false, kButtonDisabled);
    }
    break;
  }
    
  case SpectrumFindPeaks_CB_ID:
    if(!Spectrum_H)
      break;

    if(SpectrumFindPeaks_CB->IsDown()){
      SpectrumNumPeaks_NEL->GetEntry()->SetState(true);
      SpectrumSigma_NEL->GetEntry()->SetState(true);
      SpectrumResolution_NEL->GetEntry()->SetState(true);

      FindSpectrumPeaks();
    }
    else{
      SpectrumNumPeaks_NEL->GetEntry()->SetState(false);
      SpectrumSigma_NEL->GetEntry()->SetState(false);
      SpectrumResolution_NEL->GetEntry()->SetState(false);

      PlotSpectrum();
    }
    break;
    
  case SpectrumFindBackground_CB_ID:
    if(!Spectrum_H)
      break;

    if(SpectrumFindBackground_CB->IsDown()){
      SpectrumRangeMin_NEL->GetEntry()->SetState(true);
      SpectrumRangeMax_NEL->GetEntry()->SetState(true);
      SpectrumWithBackground_RB->SetState(kButtonUp);
      SpectrumLessBackground_RB->SetState(kButtonUp);

      CalculateSpectrumBackground(Spectrum_H);
      PlotSpectrum();
    }
    else{
      SpectrumRangeMin_NEL->GetEntry()->SetState(false);
      SpectrumRangeMax_NEL->GetEntry()->SetState(false);
      SpectrumWithBackground_RB->SetState(kButtonDisabled);
      SpectrumLessBackground_RB->SetState(kButtonDisabled);

      PlotSpectrum();
    }
    break;

  case SpectrumFindIntegral_CB_ID:
  case SpectrumIntegralInCounts_CB_ID:
  case SpectrumUseGaussianFit_CB_ID:
    if(!Spectrum_H)
      break;

    if(SpectrumFindIntegral_CB->IsDown())
      IntegrateSpectrum();
    else
      PlotSpectrum();
    break;
    
  case SpectrumNormalizePeakToCurrent_CB_ID:
    if(!Spectrum_H)
      break;
    
    IntegrateSpectrum();
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
      PlotWaveform();
    
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
      if(CanvasContents == zWaveform)
	PlotWaveform();
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

    if(PSDEnableFilterCreation_CB->IsDown() and CanvasContents != zPSDHistogram){
      CreateMessageBox("The canvas does not presently contain a PSD histogram! PSD filter creation is not possible!","Stop");
      PSDEnableFilterCreation_CB->SetState(kButtonUp);
      break;
    }
    break;
  }

  case PSDEnableFilter_CB_ID:{
    if(PSDEnableFilter_CB->IsDown()){
      UsePSDFilterManager[PSDChannel] = true;
      FindPeaks_CB->SetState(kButtonDown);
    }
    else
      UsePSDFilterManager[PSDChannel] = false;
    break;
  }
    
  case PSDPlotTailIntegration_CB_ID:{
    if(!FindPeaks_CB->IsDown())
      FindPeaks_CB->SetState(kButtonDown);
    PlotWaveform();
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
      PlotPSDHistogram();
      
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
    if(!SpectrumExists){
      CreateMessageBox("A valid spectrum does not yet exists! The calculation of a spectrum derivative is, therefore, moot!", "Stop");
      break;
    }
    else{
      if(SpectrumOverplotDerivative_CB->IsDown()){
	PlotSpectrum();
	PlotSpectrumDerivative();
      }
      else if(PlotSpectrumDerivativeError_CB->IsDown()){
	PlotSpectrumDerivative();
      }
      else{
	PlotSpectrum();
      }
      break;
    }
  }

  default:
    break;
  }
}


void ADAQAnalysisGUI::HandleSliders(int SliderPosition)
{
  // The slider is used to enable easy selection of waveforms to
  // display. As it slides, do the following...

  // Update the number entry widget that also enables selection of
  // waveform to plot...
  WaveformSelector_NEL->GetEntry()->SetNumber(SliderPosition);

  // If an ADAQ ROOT file has been successfully loaded, enable
  // plotting to occure...
  if(ADAQRootFileLoaded)
    PlotWaveform();
}


void ADAQAnalysisGUI::HandleDoubleSliders()
{
  if(!ADAQRootFileLoaded)
    return;
  
  // Get the currently active widget and its ID, i.e. the widget from
  // which the user has just sent a signal...
  TGDoubleSlider *ActiveDoubleSlider = (TGDoubleSlider *) gTQSender;
  int SliderID = ActiveDoubleSlider->WidgetId();

  switch(SliderID){
    
  case XAxisLimits_THS_ID:
  case YAxisLimits_DVS_ID:
  case SpectrumIntegrationLimits_DHS_ID:
    
    if(CanvasContents == zWaveform)
      PlotWaveform();

    else if(CanvasContents == zSpectrum and SpectrumExists){
      PlotSpectrum();
      if(SpectrumOverplotDerivative_CB->IsDown())
	PlotSpectrumDerivative();
    }
    
    else if(CanvasContents == zSpectrumDerivative and SpectrumExists)
      PlotSpectrumDerivative();
    
    else if(CanvasContents == zPSDHistogram and PSDHistogramExists)
      PlotPSDHistogram();

    break;
  }
}


void ADAQAnalysisGUI::HandleTripleSliderPointer()
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
  if(Spectrum_H and SpectrumCalibration_CB->IsDown() and CanvasContents == zSpectrum){
    
    // Calculate the position along the X-axis of the pulse spectrum
    // (the "area" or "height" in ADC units) based on the current
    // X-axis maximum and the triple slider's pointer position
    double CalibrationLineXPos = XAxisLimits_THS->GetPointerPosition() * Spectrum_H->GetXaxis()->GetXmax();

    // Calculate min. and max. on the Y-axis for plotting
    float Min, Max;
    YAxisLimits_DVS->GetPosition(Min, Max);
    double CalibrationLineYMin = Spectrum_H->GetBinContent(Spectrum_H->GetMaximumBin()) * (1-Max);
    double CalibrationLineYMax = Spectrum_H->GetBinContent(Spectrum_H->GetMaximumBin()) * (1-Min);

    // Redraw the pulse spectrum
    PlotSpectrum();

    // Draw the new calibration line position
    Calibration_L->DrawLine(CalibrationLineXPos,
			    CalibrationLineYMin,
			    CalibrationLineXPos,
			    CalibrationLineYMax);

    // Update the canvas with the new spectrum and calibration line
    Canvas_EC->GetCanvas()->Update();

    // Set the calibration pulse units for the current calibration
    // point based on the X position of the calibration line
    SpectrumCalibrationPulseUnit_NEL->GetEntry()->SetNumber(CalibrationLineXPos);
  }
}


void ADAQAnalysisGUI::HandleNumberEntries()
{ 
  // Get the currently active widget and its ID, i.e. the widget from
  // which the user has just sent a signal...
  TGNumberEntry *ActiveNumberEntry = (TGNumberEntry *) gTQSender;
  int NumberEntryID = ActiveNumberEntry->WidgetId();

  switch(NumberEntryID){

    // The number entry allows precise selection of waveforms to
    // plot. It also works with the slider to make sure to update the
    // position of slider if the number entry value changes
  case WaveformSelector_NEL_ID:
    WaveformSelector_HS->SetPosition(WaveformSelector_NEL->GetEntry()->GetIntNumber());
    if(!ADAQRootFile)
      return;
    PlotWaveform();
    break;

  case MaxPeaks_NEL_ID:
    if(!ADAQRootFile)
      return;

    // Delete the previoue TSpectrum PeakFinder object if it exists to
    // prevent memory leaks
    if(PeakFinder)
      delete PeakFinder;
    
    // Create a TSpectrum PeakFinder using the appropriate widget to set
    // the maximum number of peaks that can be found
    PeakFinder = new TSpectrum(MaxPeaks_NEL->GetEntry()->GetIntNumber());

    // Update to work with Waveform and Spectrum
    PlotWaveform();
    break;

  case BaselineCalcMin_NEL_ID:
  case BaselineCalcMax_NEL_ID:
  case Sigma_NEL_ID:
  case Resolution_NEL_ID:
  case Floor_NEL_ID:
  case ZeroSuppressionCeiling_NEL_ID:
  case PearsonLowerLimit_NEL_ID:
  case PearsonMiddleLimit_NEL_ID:
  case PearsonUpperLimit_NEL_ID:
  case PSDPeakOffset_NEL_ID:
  case PSDTailOffset_NEL_ID:
    if(!ADAQRootFile)
      return;
    else
      PlotWaveform();
    break;

  case SpectrumNumPeaks_NEL_ID:
  case SpectrumSigma_NEL_ID:
  case SpectrumResolution_NEL_ID:
    FindSpectrumPeaks();
    break;

  case SpectrumRangeMin_NEL_ID:
  case SpectrumRangeMax_NEL_ID:
    CalculateSpectrumBackground(Spectrum_H);
    PlotSpectrum();
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

    switch(CanvasContents){
    case zWaveform:
      PlotWaveform();
      break;
      
    case zSpectrum:
      PlotSpectrum();
      break;

    case zSpectrumDerivative:
      PlotSpectrumDerivative();
      break;
      
    case zPSDHistogram:
      PlotPSDHistogram();
      break;
    }
    break;

  default:
    break;
  }
}


void ADAQAnalysisGUI::HandleRadioButtons()
{
  TGRadioButton *ActiveRadioButton = (TGRadioButton *) gTQSender;
  int RadioButtonID = ActiveRadioButton->WidgetId();
  
  switch(RadioButtonID){
    
  case RawWaveform_RB_ID:
    FindPeaks_CB->SetState(kButtonDisabled);
    PlotWaveform();
    break;
    
  case BaselineSubtractedWaveform_RB_ID:
    if(FindPeaks_CB->IsDown())
      FindPeaks_CB->SetState(kButtonDown);
    else
      FindPeaks_CB->SetState(kButtonUp);
    PlotWaveform();
    break;
    
  case ZeroSuppressionWaveform_RB_ID:
    if(FindPeaks_CB->IsDown())
      FindPeaks_CB->SetState(kButtonDown);
    else
      FindPeaks_CB->SetState(kButtonUp);
    PlotWaveform();
    break;

  case PositiveWaveform_RB_ID:
    WaveformPolarity = 1.0;
    PlotWaveform();
    break;
    
  case NegativeWaveform_RB_ID:
    WaveformPolarity = -1.0;
    PlotWaveform();
    break;

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
    
    int CurrentChannel = ChannelSelector_CBL->GetComboBox()->GetSelected();
    
    //      if(UseCalibrationManager[CurrentChannel])
    //	delete CalibrationManager[CurrentChannel];
    
    CalibrationManager[CurrentChannel] = new TGraph(NumCalibrationPoints_FixedEP,
						    PulseUnitPoints_FixedEP,
						    EnergyPoints_FixedEP);
    
    UseCalibrationManager[CurrentChannel] = true;
    
    break;
  }
  
  case SpectrumWithBackground_RB_ID:
    SpectrumLessBackground_RB->SetState(kButtonUp);
    CalculateSpectrumBackground(Spectrum_H);
    PlotSpectrum();
    break;

  case SpectrumLessBackground_RB_ID:
    SpectrumWithBackground_RB->SetState(kButtonUp);
    CalculateSpectrumBackground(Spectrum_H);
    PlotSpectrum();
    break;
    
  case IntegrateRawPearson_RB_ID:
    IntegrateFitToPearson_RB->SetState(kButtonUp);
    PearsonMiddleLimit_NEL->GetEntry()->SetState(false);
    PlotWaveform();
    break;

  case IntegrateFitToPearson_RB_ID:
    IntegrateRawPearson_RB->SetState(kButtonUp);
    PearsonMiddleLimit_NEL->GetEntry()->SetState(true);
    PlotWaveform();
    break;

  case PearsonPolarityPositive_RB_ID:
    PearsonPolarityNegative_RB->SetState(kButtonUp);
    PearsonPolarity = 1.0;
    PlotWaveform();
    break;

  case PearsonPolarityNegative_RB_ID:
    PearsonPolarityPositive_RB->SetState(kButtonUp);
    PearsonPolarity = -1.0;
    PlotWaveform();
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
      PSDFilterPolarity = 1.;
    }
    break;

  case PSDNegativeFilter_RB_ID:
    if(PSDNegativeFilter_RB->IsDown()){
      PSDPositiveFilter_RB->SetState(kButtonUp);
      PSDFilterPolarity = -1.;
    }
    break;

  case PSDHistogramSliceX_RB_ID:
    if(PSDHistogramSliceX_RB->IsDown())
      PSDHistogramSliceY_RB->SetState(kButtonUp);
    
    if(CanvasContents == zPSDHistogram)
      PlotPSDHistogram();
    break;
    
  case PSDHistogramSliceY_RB_ID:
    if(PSDHistogramSliceY_RB->IsDown())
      PSDHistogramSliceX_RB->SetState(kButtonUp);
    if(CanvasContents == zPSDHistogram)
      PlotPSDHistogram();
    break;
    
  case DrawWaveformWithCurve_RB_ID:
  case DrawWaveformWithMarkers_RB_ID:
  case DrawWaveformWithBoth_RB_ID:
    PlotWaveform();
    break;

  case DrawSpectrumWithBars_RB_ID:
  case DrawSpectrumWithCurve_RB_ID:
  case DrawSpectrumWithMarkers_RB_ID:
    
    PlotSpectrum();
    break;
  }
}


void ADAQAnalysisGUI::HandleComboBox(int ComboBoxID, int SelectedID)
{
  switch(ComboBoxID){

  case PSDPlotType_CBL_ID:
    if(PSDHistogram_H)
      PlotPSDHistogram();
    break;
  }
}


void ADAQAnalysisGUI::HandleCanvas(int EventID, int XPixel, int YPixel, TObject *Selected)
{
  // If the user has enabled the creation of a PSD filter and the
  // canvas event is equal to "1" (which represents a down-click
  // somewhere on the canvas pad) then send the pixel coordinates of
  // the down-click to the PSD filter creation function
  if(PSDEnableFilterCreation_CB->IsDown() and EventID == 1)
    CreatePSDFilter(XPixel, YPixel);
  
  if(PSDEnableHistogramSlicing_CB->IsDown()){
    
    // The user may click the canvas to "freeze" the PSD histogram
    // slice position, which ensures the PSD histogram slice at that
    // point remains plotted in the standalone canvas
    if(EventID == 1){
      PSDEnableHistogramSlicing_CB->SetState(kButtonUp);
      return;
    }
    else
      PlotPSDHistogramSlice(XPixel, YPixel);
  }
}


void ADAQAnalysisGUI::HandleTerminate()
{ gApplication->Terminate(); }


void ADAQAnalysisGUI::LoadADAQRootFile()
{
  //////////////////////////////////
  // Open the specified ROOT file //
  //////////////////////////////////

  // Open the specified ROOT file 
  ADAQRootFile = new TFile(ADAQRootFileName.c_str(),"read");

  // If a valid ADAQ ROOT file was NOT opened...
  if(!ADAQRootFile->IsOpen()){
    
    // Alert the user...
    string Message = "Could not find a valid AGNOSTIC ROOT file to open! Looking for file:\n" + ADAQRootFileName;
    CreateMessageBox(Message,"Stop");
    
    // Set the file loaded boolean appropriately
    ADAQRootFileLoaded = false;

    return;
  }


  /////////////////////////////////////
  // Extract data from the ROOT file //
  /////////////////////////////////////

  // Get the ADAQRootMeasParams objects stored in the ROOT file
  ADAQMeasParams = (ADAQRootMeasParams *)ADAQRootFile->Get("MeasParams");
  
  // Get the TTree with waveforms stored in the ROOT file
  ADAQWaveformTree = (TTree *)ADAQRootFile->Get("WaveformTree");

  // Attempt to get the class containing results that were calculated
  // in a parallel run of ADAQAnalysisGUI and stored persistently
  // along with data in the ROOT file. At present, this is only the
  // despliced waveform files. For standard ADAQ ROOT files, the
  // ADAQParResults class member will be a NULL pointer
  ADAQParResults = (ADAQAnalysisParallelResults *)ADAQRootFile->Get("ParResults");

  // If a valid class with the parallel parameters was found
  // (despliced ADAQ ROOT files, etc) then set the bool to true; if no
  // class with parallel parameters was found (standard ADAQ ROOT
  // files) then set the bool to false. This bool will be used
  // appropriately throughout the code to import parallel results.
  (ADAQParResults) ? ADAQParResultsLoaded = true : ADAQParResultsLoaded = false;

  // If the ADAQParResults class was loaded then ...
  if(ADAQParResultsLoaded){
    // Load the total integrated RFQ current and update the number
    // entry field widget in the "Analysis" tab frame accordingly
    TotalDeuterons = ADAQParResults->TotalDeuterons;
    if(SequentialArchitecture)
      DeuteronsInTotal_NEFL->GetEntry()->SetNumber(TotalDeuterons);
    
    if(Verbose)
      cout << "Total RFQ current from despliced file: " << ADAQParResults->TotalDeuterons << endl;
  }

  // Get the record length (acquisition window)
  RecordLength = ADAQMeasParams->RecordLength;
  
  // Create the Time vector<int> with size equal to the acquisition
  // RecordLength. The Time vector, which represents the X-axis of a
  // plotted waveform, is used for plotting 
  for(int sample=0; sample<RecordLength; sample++)
    Time.push_back(sample);
  
  // The ADAQ Waveform TTree stores digitized waveforms into
  // vector<int>s of length RecordLength. Recall that the "record
  // length" is the width of the acquisition window in time in units
  // of 4 ns samples. There are 8 vectors<int>s corresponding to the 8
  // channels on the V1720 digitizer board. The following code
  // initializes the member object WaveformVecPtrs to be vector of
  // vectors (outer vector size 8 = 8 V1720 channels; inner vector of
  // size RecordLength = RecordLength samples). Each of the outer
  // vectors is assigned to point to the address of the vector<int> in
  // the ADAQ TTree representing the appopriate V1720 channel data.

  stringstream ss;
  for(int ch=0; ch<NumDataChannels; ch++){
    // Create the correct ADAQ TTree branch name
    ss << "VoltageInADC_Ch" << ch;
    string BranchName = ss.str();

    // Activate the branch in the ADAQ TTree
    ADAQWaveformTree->SetBranchStatus(BranchName.c_str(), 1);

    // Set the present channels' class object vector pointer to the
    // address of that chennel's vector<int> stored in the TTree
    ADAQWaveformTree->SetBranchAddress(BranchName.c_str(), &WaveformVecPtrs[ch]);

    // Clear the string for the next channel.
    ss.str("");
  }
   

  //////////////////////////////////////////////
  // Output summary of measurement parameters //
  //////////////////////////////////////////////

  if(Verbose){
    
    cout << "\n\nMEASUREMENT SUMMARY:\n";
    
    cout  << "Record Length = " << ADAQMeasParams->RecordLength << "\n"
	  << "Number of waveforms = " << ADAQWaveformTree->GetEntries() << "\n"
	  << endl;
    
    cout << "\nHIGH VOLTAGE CHANNEL SUMMARY: \n";
    
    for(int ch=0; ch<6; ch++){
      cout << "Detector HV[" << ch << "] = " << ADAQMeasParams->DetectorVoltage[ch] << " V\n"
	   << "Detector I[" << ch << "] = " << ADAQMeasParams->DetectorCurrent[ch] << " uA\n\n";
    }
    
    cout << "\nDIGITIZER CHANNEL SUMMARY: \n";
    
    for(int ch=0; ch<NumDataChannels; ch++)
      cout << "DC Offset[" << ch << "] = 0x" << ADAQMeasParams->DCOffset[ch] << " ADC\n"
	   << "Trigger Threshold[" << ch << "] = " << ADAQMeasParams->TriggerThreshold[ch] << " ADC\n"
	   << "Baseline Calc. Min.[" << ch << "] = " << ADAQMeasParams->BaselineCalcMin[ch] << " ADC\n"
	   << "Baseline Calc. Max.[" << ch << "] = " << ADAQMeasParams->BaselineCalcMax[ch] << " ADC\n"
	   << endl;
  }


  ////////////////////////////////////////////
  // Update various ADAQAnalysisGUI widgets //
  ////////////////////////////////////////////

  // Deny parallel binaries access to the ROOT widgets to prevent
  // seg. faults since parallel architecture do not construct widgets
#ifndef MPI_ENABLED

  int WaveformsInFile = ADAQWaveformTree->GetEntries();




  // Set the range of the slider used to plot waveforms such that it
  // can access all the waveforms in the ADAQ ROOT file.
  WaveformSelector_HS->SetRange(1, WaveformsInFile);

  // Set the range of the number entry to the maximum number of
  // waveforms available to histogram
  WaveformsToHistogram_NEL->GetEntry()->SetLimitValues(1, WaveformsInFile);
  WaveformsToHistogram_NEL->GetEntry()->SetNumber(WaveformsInFile);

  // Update the ROOT widgets above the embedded canvas to display
  // the file name, total waveofmrs, and record length
  FileName_TE->SetText(ADAQRootFileName.c_str());
  Waveforms_NEL->SetNumber(WaveformsInFile);
  RecordLength_NEL->SetNumber(ADAQMeasParams->RecordLength);
  
  BaselineCalcMin_NEL->GetEntry()->SetLimitValues(0,RecordLength-1);
  BaselineCalcMin_NEL->GetEntry()->SetNumber(0);
  
  BaselineCalcMax_NEL->GetEntry()->SetLimitValues(1,RecordLength);
  
  // Inelegant implementation to automatically set the baseline calc
  // length to account for real ADAQ data with long acquisition
  // windows (where we want a lot of samples for an accurate baseline)
  // or calibration/other data with short acquisition windows (where
  // we can't take as many samples for baseline calculation)
  double BaselineCalcMax = 0;
  (RecordLength > 1500) ? BaselineCalcMax = 750 : BaselineCalcMax = 100;;
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
  
  // Update the XAxis minimum and maximum values for correct plotting
  XAxisMin = 0;
  XAxisMax = RecordLength;

#endif

  // Update the bool that determines if a valid ROOT file is loaded
  ADAQRootFileLoaded = true;
}


// Method to simply readout out the values of the relevant ROOT
// widgets and saves the values to the appropriate class member
// variables.  Ultimately, this is done to ensure that the waveform
// processing algorithms may be run in parallel architecture (in which
// the ROOT widgets are not built) with the necessary variables
// values, which are set by the ROOT widgets. The ROOT widget values
// are set to member function variables here for access during
// waveform processing in either sequential (not necessary) or
// parallal (necessary) architecture
void ADAQAnalysisGUI::ReadoutWidgetValues()
{
  // Order of the following class data member setting processes ro
  // rughly mimicks the order that their corresponding widgets appears
  // graphically in the GUI
  
  //////////////////////////////////////
  // Values from "Waveform" tabbed frame

  Channel = ChannelSelector_CBL->GetComboBox()->GetSelected();
  WaveformToPlot = WaveformSelector_NEL->GetEntry()->GetIntNumber();

  RawWaveform = RawWaveform_RB->IsDown();
  BaselineSubtractedWaveform = BaselineSubtractedWaveform_RB->IsDown();
  ZeroSuppressionWaveform = ZeroSuppressionWaveform_RB->IsDown();
  
  (PositiveWaveform_RB->IsDown()) ? WaveformPolarity = 1.0 : WaveformPolarity = -1.0;

  ZeroSuppressionCeiling = ZeroSuppressionCeiling_NEL->GetEntry()->GetIntNumber();

  MaxPeaks = MaxPeaks_NEL->GetEntry()->GetIntNumber();
  Sigma = Sigma_NEL->GetEntry()->GetNumber();
  Resolution = Resolution_NEL->GetEntry()->GetNumber();
  Floor = Floor_NEL->GetEntry()->GetIntNumber();

  PlotFloor = PlotFloor_CB->IsDown();
  PlotCrossings = PlotCrossings_CB->IsDown();
  PlotPeakIntegratingRegion = PlotPeakIntegratingRegion_CB->IsDown();

  UsePileupRejection = UsePileupRejection_CB->IsDown();

  BaselineCalcMin = BaselineCalcMin_NEL->GetEntry()->GetIntNumber();
  BaselineCalcMax = BaselineCalcMax_NEL->GetEntry()->GetIntNumber();


  //////////////////////////////////////
  // Values from "Spectrum" tabbed frame

  WaveformsToHistogram = WaveformsToHistogram_NEL->GetEntry()->GetIntNumber();
  SpectrumNumBins = SpectrumNumBins_NEL->GetEntry()->GetIntNumber();
  SpectrumMinBin = SpectrumMinBin_NEL->GetEntry()->GetIntNumber();
  SpectrumMaxBin = SpectrumMaxBin_NEL->GetEntry()->GetIntNumber();

  SpectrumTypePAS = SpectrumTypePAS_RB->IsDown();
  SpectrumTypePHS = SpectrumTypePHS_RB->IsDown();

  IntegrationTypeWholeWaveform = IntegrationTypeWholeWaveform_RB->IsDown();
  IntegrationTypePeakFinder = IntegrationTypePeakFinder_RB->IsDown();


  //////////////////////////////////////////
  // Values from the "Analysis" tabbed frame

  PSDWaveformsToDiscriminate = PSDWaveforms_NEL->GetEntry()->GetIntNumber();
  PSDNumTailBins = PSDNumTailBins_NEL->GetEntry()->GetIntNumber();
  PSDMinTailBin = PSDMinTailBin_NEL->GetEntry()->GetIntNumber();
  PSDMaxTailBin = PSDMaxTailBin_NEL->GetEntry()->GetIntNumber();
  
  PSDChannel = PSDChannel_CBL->GetComboBox()->GetSelected();
  PSDThreshold = PSDThreshold_NEL->GetEntry()->GetIntNumber();
  PSDPeakOffset = PSDPeakOffset_NEL->GetEntry()->GetIntNumber();
  PSDTailOffset = PSDTailOffset_NEL->GetEntry()->GetIntNumber();
    
  PSDNumTotalBins = PSDNumTotalBins_NEL->GetEntry()->GetIntNumber();
  PSDMinTotalBin = PSDMinTotalBin_NEL->GetEntry()->GetIntNumber();
  PSDMaxTotalBin = PSDMaxTotalBin_NEL->GetEntry()->GetIntNumber();
  
  WaveformsToDesplice = DesplicedWaveformNumber_NEL->GetEntry()->GetIntNumber();
  DesplicedWaveformBuffer = DesplicedWaveformBuffer_NEL->GetEntry()->GetIntNumber();
  DesplicedWaveformLength = DesplicedWaveformLength_NEL->GetEntry()->GetIntNumber();
  DesplicedFileName = DesplicedFileName_TE->GetText();


  ////////////////////////////////////////
  // Values from "Processing" tabbed frame

  UpdateFreq = UpdateFreq_NEL->GetEntry()->GetIntNumber();

  IntegratePearson = IntegratePearson_CB->IsDown();
  PearsonChannel = PearsonChannel_CBL->GetComboBox()->GetSelected();  

  (PearsonPolarityPositive_RB->IsDown()) ? PearsonPolarity = 1.0 : PearsonPolarity = -1.0;

  IntegrateRawPearson = IntegrateRawPearson_RB->IsDown();
  IntegrateFitToPearson = IntegrateFitToPearson_RB->IsDown();

  PearsonLowerLimit = PearsonLowerLimit_NEL->GetEntry()->GetIntNumber();
  PearsonMiddleLimit = PearsonMiddleLimit_NEL->GetEntry()->GetIntNumber();
  PearsonUpperLimit = PearsonUpperLimit_NEL->GetEntry()->GetIntNumber();

  PlotPearsonIntegration = PlotPearsonIntegration_CB->IsDown();  

}


// Method to store the variables necessary for parallel processing in
// a ROOT file that will be accessed by the ADAQAnalysisGUI parallel
// nodes before processing. This method will be called by the
// sequential ADAQAnalysisGUI binary to save the current state of the
// ROOT widgets and other variables to a ROOT file for access by the
// parallel binaries
void ADAQAnalysisGUI::SaveParallelProcessingData()
{
  // Set the current sequential GUI widget values to member variables
  ReadoutWidgetValues();

  // Open a ROOT TFile to store the variables
  ParallelProcessingFile = new TFile(ParallelProcessingFName.c_str(), "recreate");

  // Create a custom object to store the parameters
  ADAQAnalysisParallelParameters *ADAQParParams = new ADAQAnalysisParallelParameters;

  //////////////////////////////////////////
  // Values from the "Waveform" tabbed frame 

  ADAQParParams->Channel = Channel;

  ADAQParParams->RawWaveform = RawWaveform;
  ADAQParParams->BaselineSubtractedWaveform = BaselineSubtractedWaveform;
  ADAQParParams->ZeroSuppressionWaveform = ZeroSuppressionWaveform;

  ADAQParParams->WaveformPolarity = WaveformPolarity;

  ADAQParParams->ZeroSuppressionCeiling = ZeroSuppressionCeiling;

  ADAQParParams->MaxPeaks = MaxPeaks;
  ADAQParParams->Sigma = Sigma;
  ADAQParParams->Resolution = Resolution;
  ADAQParParams->Floor = Floor;

  ADAQParParams->PlotFloor = PlotFloor;
  ADAQParParams->PlotCrossings = PlotCrossings;
  ADAQParParams->PlotPeakIntegratingRegion = PlotPeakIntegratingRegion;

  ADAQParParams->UsePileupRejection = UsePileupRejection;

  ADAQParParams->BaselineCalcMin = BaselineCalcMin;
  ADAQParParams->BaselineCalcMax = BaselineCalcMax;


  //////////////////////////////////////
  // Values from "Spectrum" tabbed frame

  ADAQParParams->WaveformsToHistogram = WaveformsToHistogram;
  ADAQParParams->SpectrumNumBins = SpectrumNumBins;
  ADAQParParams->SpectrumMinBin = SpectrumMinBin;
  ADAQParParams->SpectrumMaxBin = SpectrumMaxBin;

  ADAQParParams->SpectrumTypePAS = SpectrumTypePAS;
  ADAQParParams->SpectrumTypePHS = SpectrumTypePHS;

  ADAQParParams->IntegrationTypeWholeWaveform = IntegrationTypeWholeWaveform;
  ADAQParParams->IntegrationTypePeakFinder = IntegrationTypePeakFinder;

  
  //////////////////////////////////////////
  // Values from the "Analysis" tabbed frame 

  ADAQParParams->PSDChannel = PSDChannel;
  ADAQParParams->PSDWaveformsToDiscriminate = PSDWaveformsToDiscriminate;
  ADAQParParams->PSDThreshold = PSDThreshold;
  ADAQParParams->PSDPeakOffset = PSDPeakOffset;
  ADAQParParams->PSDTailOffset = PSDTailOffset;

  ADAQParParams->PSDNumTailBins = PSDNumTailBins;
  ADAQParParams->PSDMinTailBin = PSDMinTailBin;
  ADAQParParams->PSDMaxTailBin = PSDMaxTailBin;

  ADAQParParams->PSDNumTotalBins = PSDNumTotalBins;
  ADAQParParams->PSDMinTotalBin = PSDMinTotalBin;
  ADAQParParams->PSDMaxTotalBin = PSDMaxTotalBin;

  ADAQParParams->PSDFilterPolarity = PSDFilterPolarity;

  ADAQParParams->WaveformsToDesplice = WaveformsToDesplice;
  ADAQParParams->DesplicedWaveformBuffer = DesplicedWaveformBuffer;
  ADAQParParams->DesplicedWaveformLength = DesplicedWaveformLength;
  ADAQParParams->DesplicedFileName = DesplicedFileName;


  ////////////////////////////////////////
  // Values from "Processing" tabbed frame

  ADAQParParams->UpdateFreq = UpdateFreq;

  ADAQParParams->IntegratePearson = IntegratePearson;
  ADAQParParams->PearsonChannel = PearsonChannel;

  ADAQParParams->PearsonPolarity = PearsonPolarity;

  ADAQParParams->IntegrateRawPearson = IntegrateRawPearson;
  ADAQParParams->IntegrateFitToPearson = IntegrateFitToPearson;

  ADAQParParams->PearsonLowerLimit = PearsonLowerLimit;
  ADAQParParams->PearsonMiddleLimit = PearsonMiddleLimit;
  ADAQParParams->PearsonUpperLimit = PearsonUpperLimit;

  ADAQParParams->PlotPearsonIntegration = PlotPearsonIntegration;


  ////////////////////////////////////////////////////////
  // Miscellaneous values required for parallel processing
  
  // The ADAQ-formatted ROOT file name
  ADAQParParams->ADAQRootFileName = ADAQRootFileName;

  // The calibration maanager bool and array of TGraph *'s
  ADAQParParams->UseCalibrationManager = UseCalibrationManager;
  ADAQParParams->CalibrationManager = CalibrationManager;
  
  // The PSD filter
  ADAQParParams->UsePSDFilterManager = UsePSDFilterManager;
  ADAQParParams->PSDFilterManager = PSDFilterManager;

  
  //////////////////////////////////////////////////
  // Write the transient parallel value storage file

  // Write the customn storage class to the ROOT file
  ADAQParParams->Write("ADAQParParams");

  // Write the ROOT file (with its contents) to disk and close
  ParallelProcessingFile->Write();
  ParallelProcessingFile->Close();
}


// Method to load the variables necessary for parallel processing from
// the ROOT file. This method will be called by the ADAQAnalysisGUI
// parallel binaries immediately before beginning to process waveforms.
void ADAQAnalysisGUI::LoadParallelProcessingData()
{
  // Load the ROOT file
  ParallelProcessingFile = new TFile(ParallelProcessingFName.c_str(), "read");

  // Alert the user if the ROOT file cannont be opened
  if(!ParallelProcessingFile->IsOpen()){
    cout << "\nADAQAnalysisGUI_MPI Node[" << MPI_Rank << " : Error! Could not load the parallel processing file! Abort!\n"
	 << endl;
    exit(-42);
  }
  
  // Load the necessary processing variables
  else{
    // Get the custom storage class object from the ROOT file
    ADAQAnalysisParallelParameters *ADAQParParams = (ADAQAnalysisParallelParameters *)ParallelProcessingFile->Get("ADAQParParams");
    
    // Set the ROOT widget-derived variables to class member variables

    //////////////////////////////////////////
    // Values from the "Waveform" tabbed frame
    Channel = ADAQParParams->Channel;

    RawWaveform = ADAQParParams->RawWaveform;
    BaselineSubtractedWaveform = ADAQParParams->BaselineSubtractedWaveform;
    ZeroSuppressionWaveform = ADAQParParams->ZeroSuppressionWaveform;

    WaveformPolarity = ADAQParParams->WaveformPolarity;

    ZeroSuppressionCeiling = ADAQParParams->ZeroSuppressionCeiling;

    MaxPeaks = ADAQParParams->MaxPeaks;
    Sigma = ADAQParParams->Sigma;
    Resolution = ADAQParParams->Resolution;
    Floor = ADAQParParams->Floor;

    PlotFloor = ADAQParParams->PlotFloor;
    PlotCrossings = ADAQParParams->PlotCrossings;
    PlotPeakIntegratingRegion = ADAQParParams->PlotPeakIntegratingRegion;

    UsePileupRejection = ADAQParParams->UsePileupRejection;

    BaselineCalcMin = ADAQParParams->BaselineCalcMin;
    BaselineCalcMax = ADAQParParams->BaselineCalcMax;


    //////////////////////////////////////////
    // Values from the "Spectrum" tabbed frame

    WaveformsToHistogram = ADAQParParams->WaveformsToHistogram;
    SpectrumNumBins = ADAQParParams->SpectrumNumBins;
    SpectrumMinBin = ADAQParParams->SpectrumMinBin;
    SpectrumMaxBin = ADAQParParams->SpectrumMaxBin;

    SpectrumTypePAS = ADAQParParams->SpectrumTypePAS;
    SpectrumTypePHS = ADAQParParams->SpectrumTypePHS;

    IntegrationTypeWholeWaveform = ADAQParParams->IntegrationTypeWholeWaveform;
    IntegrationTypePeakFinder = ADAQParParams->IntegrationTypePeakFinder;

    
    //////////////////////////////////////////
    // Values from the "Analysis" tabbed frame

    PSDChannel = ADAQParParams->PSDChannel; 
    PSDWaveformsToDiscriminate = ADAQParParams->PSDWaveformsToDiscriminate;
    PSDThreshold = ADAQParParams->PSDThreshold; 
    PSDPeakOffset = ADAQParParams->PSDPeakOffset; 
    PSDTailOffset = ADAQParParams->PSDTailOffset; 

    PSDNumTailBins = ADAQParParams->PSDNumTailBins; 
    PSDMinTailBin = ADAQParParams->PSDMinTailBin; 
    PSDMaxTailBin = ADAQParParams->PSDMaxTailBin; 

    PSDNumTotalBins = ADAQParParams->PSDNumTotalBins;
    PSDMinTotalBin = ADAQParParams->PSDMinTotalBin; 
    PSDMaxTotalBin = ADAQParParams->PSDMaxTotalBin; 

    PSDFilterPolarity = ADAQParParams->PSDFilterPolarity;

    WaveformsToDesplice = ADAQParParams->WaveformsToDesplice;
    DesplicedWaveformBuffer = ADAQParParams->DesplicedWaveformBuffer;
    DesplicedWaveformLength = ADAQParParams->DesplicedWaveformLength;
    DesplicedFileName = ADAQParParams->DesplicedFileName;


    ////////////////////////////////////////////
    // Values from the "Processing" tabbed frame

    UpdateFreq = ADAQParParams->UpdateFreq;

    IntegratePearson = ADAQParParams->IntegratePearson;
    PearsonChannel = ADAQParParams->PearsonChannel;

    PearsonPolarity = ADAQParParams->PearsonPolarity;

    IntegrateRawPearson = ADAQParParams->IntegrateRawPearson;
    IntegrateFitToPearson = ADAQParParams->IntegrateFitToPearson;

    PearsonLowerLimit = ADAQParParams->PearsonLowerLimit;
    PearsonMiddleLimit = ADAQParParams->PearsonMiddleLimit;
    PearsonUpperLimit = ADAQParParams->PearsonUpperLimit;

    PlotPearsonIntegration = ADAQParParams->PlotPearsonIntegration;


    ////////////////////////////////////////////////////////
    // Miscellaneous values required for parallel processing

    ADAQRootFileName = ADAQParParams->ADAQRootFileName;

    UseCalibrationManager = ADAQParParams->UseCalibrationManager;
    CalibrationManager = ADAQParParams->CalibrationManager;
    
    UsePSDFilterManager = ADAQParParams->UsePSDFilterManager;
    PSDFilterManager = ADAQParParams->PSDFilterManager;

    
    // Close the ROOT File
    ParallelProcessingFile->Close();
  }
}


void ADAQAnalysisGUI::PlotWaveform()
{
  if(!ADAQRootFileLoaded)
    return;
  
  // Readout the ROOT widget values into member data for access in the
  // subsequent code. Although it's seemingly unnecessary here
  // (especially in sequential architecture), the main purpose in
  // standardize access to widget value to enable waveform processing
  // with parallel architecture
  ReadoutWidgetValues();

  // Get the waveform from the ADAQ TTree
  ADAQWaveformTree->GetEntry(WaveformToPlot);

  // The user can choose to find waveform peaks in the acquisition
  // window via the appropriate check button widget ...

  if(RawWaveform_RB->IsDown())
    CalculateRawWaveform(Channel);
  else if(BaselineSubtractedWaveform_RB->IsDown())
    CalculateBSWaveform(Channel);
  else if(ZeroSuppressionWaveform_RB->IsDown())
    CalculateZSWaveform(Channel);

  float Min, Max;
  int XAxisSize, YAxisSize;

  // Determine the X-axis size and min/max values
  XAxisSize = Waveform_H[Channel]->GetNbinsX();
  XAxisLimits_THS->GetPosition(Min, Max);
  XAxisMin = XAxisSize * Min;
  XAxisMax = XAxisSize * Max;

  // Determine the Y-axis size (dependent on whether or not the user
  // has opted to allow the Y-axis range to be automatically
  // determined by the size of the waveform or fix the Y-Axis range
  // with the vertical double slider positions) and min/max values

  YAxisLimits_DVS->GetPosition(Min, Max);

  if(PlotVerticalAxisInLog_CB->IsDown()){
    YAxisMin = V1720MaximumBit * (1 - Max);
    YAxisMax = V1720MaximumBit * (1 - Min);
    
    if(YAxisMin == 0)
      YAxisMin = 0.05;
  }
  else if(AutoYAxisRange_CB->IsDown()){
    YAxisMin = Waveform_H[Channel]->GetBinContent(Waveform_H[Channel]->GetMinimumBin()) - 10;
    YAxisMax = Waveform_H[Channel]->GetBinContent(Waveform_H[Channel]->GetMaximumBin()) + 10;
  }
  else{
    YAxisLimits_DVS->GetPosition(Min, Max);
    YAxisMin = (V1720MaximumBit * (1 - Max))-30;
    YAxisMax = V1720MaximumBit * (1 - Min);
  }
  YAxisSize = YAxisMax - YAxisMin;

  if(OverrideTitles_CB->IsDown()){
    Title = Title_TEL->GetEntry()->GetText();
    XAxisTitle = XAxisTitle_TEL->GetEntry()->GetText();
    YAxisTitle = YAxisTitle_TEL->GetEntry()->GetText();
  }
  else{
    Title = "Digitized Waveform";
    XAxisTitle = "Time [4 ns samples]";
    YAxisTitle = "Voltage [ADC]";
  }

  int XAxisDivs = XAxisDivs_NEL->GetEntry()->GetIntNumber();
  int YAxisDivs = YAxisDivs_NEL->GetEntry()->GetIntNumber();
  
  Canvas_EC->GetCanvas()->SetLeftMargin(0.13);
  Canvas_EC->GetCanvas()->SetBottomMargin(0.12);
  Canvas_EC->GetCanvas()->SetRightMargin(0.05);
  
  if(PlotVerticalAxisInLog_CB->IsDown())
    gPad->SetLogy(true);
  else
    gPad->SetLogy(false);

  Waveform_H[Channel]->SetTitle(Title.c_str());

  Waveform_H[Channel]->GetXaxis()->SetTitle(XAxisTitle.c_str());
  Waveform_H[Channel]->GetXaxis()->SetTitleSize(XAxisSize_NEL->GetEntry()->GetNumber());
  Waveform_H[Channel]->GetXaxis()->SetLabelSize(XAxisSize_NEL->GetEntry()->GetNumber());
  Waveform_H[Channel]->GetXaxis()->SetTitleOffset(XAxisOffset_NEL->GetEntry()->GetNumber());
  Waveform_H[Channel]->GetXaxis()->CenterTitle();
  Waveform_H[Channel]->GetXaxis()->SetRangeUser(XAxisMin, XAxisMax);
  Waveform_H[Channel]->GetXaxis()->SetNdivisions(XAxisDivs, true);

  Waveform_H[Channel]->GetYaxis()->SetTitle(YAxisTitle.c_str());
  Waveform_H[Channel]->GetYaxis()->SetTitleSize(YAxisSize_NEL->GetEntry()->GetNumber());
  Waveform_H[Channel]->GetYaxis()->SetLabelSize(YAxisSize_NEL->GetEntry()->GetNumber());
  Waveform_H[Channel]->GetYaxis()->SetTitleOffset(YAxisOffset_NEL->GetEntry()->GetNumber());
  Waveform_H[Channel]->GetYaxis()->CenterTitle();
  Waveform_H[Channel]->GetYaxis()->SetNdivisions(YAxisDivs, true);
  Waveform_H[Channel]->SetMinimum(YAxisMin);
  Waveform_H[Channel]->SetMaximum(YAxisMax);

  
  Waveform_H[Channel]->SetLineColor(4);
  Waveform_H[Channel]->SetStats(false);  
  
  Waveform_H[Channel]->SetMarkerStyle(24);
  Waveform_H[Channel]->SetMarkerSize(1.0);
  Waveform_H[Channel]->SetMarkerColor(4);
  
  string DrawString;
  if(DrawWaveformWithCurve_RB->IsDown())
    DrawString = "C";
  else if(DrawWaveformWithMarkers_RB->IsDown())
    DrawString = "P";
  else if(DrawWaveformWithBoth_RB->IsDown())
    DrawString = "CP";
  
  Waveform_H[Channel]->Draw(DrawString.c_str());
    

  // The user may choose to plot a number of graphical objects
  // representing various waveform parameters (zero suppression
  // ceiling, noise ceiling, trigger, baseline calculation, ... ) to
  // aid him/her optimizing the parameter values

  if(PlotZeroSuppressionCeiling_CB->IsDown())
    ZSCeiling_L->DrawLine(XAxisMin,
			  ZeroSuppressionCeiling_NEL->GetEntry()->GetIntNumber(),
			  XAxisMax,
			  ZeroSuppressionCeiling_NEL->GetEntry()->GetIntNumber());

  /*
  if(PlotNoiseCeiling_CB->IsDown())
    NCeiling_L->DrawLine(XAxisMin,
			 NoiseCeiling_NEL->GetEntry()->GetIntNumber(),
			 XAxisMax,
			 NoiseCeiling_NEL->GetEntry()->GetIntNumber());
  */

  if(PlotTrigger_CB->IsDown())
    Trigger_L->DrawLine(XAxisMin,
			ADAQMeasParams->TriggerThreshold[Channel], 
			XAxisMax,
			ADAQMeasParams->TriggerThreshold[Channel]);

  // The user may choose to overplot a transparent box on the
  // waveform. The lower and upper X limits of the box represent the
  // min/max baseline calculation bounds; the Y position of the box
  // center represents the calculated value of the baseline. Note that
  // plotting the baseline is disabled for ZS waveforms since plotting
  // on a heavily modified X-axis range does not visually represent
  // the actual region of the waveform used for baseline calculation.
  if(PlotBaseline_CB->IsDown() and !ZeroSuppressionWaveform_RB->IsDown()){
    
    double BaselinePlotMinValue = Baseline - (0.04 * YAxisSize);
    double BaselinePlotMaxValue = Baseline + (0.04 * YAxisSize);
    if(BaselineSubtractedWaveform_RB->IsDown()){
      BaselinePlotMinValue = -0.04 * YAxisSize;
      BaselinePlotMaxValue = 0.04 * YAxisSize;
    }
    
    Baseline_B->DrawBox(BaselineCalcMin_NEL->GetEntry()->GetIntNumber(),
			BaselinePlotMinValue,
			BaselineCalcMax_NEL->GetEntry()->GetIntNumber(),
			BaselinePlotMaxValue);
  }
    
  if(PlotPearsonIntegration_CB->IsDown()){
    PearsonLowerLimit_L->DrawLine(PearsonLowerLimit_NEL->GetEntry()->GetIntNumber(),
				  YAxisMin,
				  PearsonLowerLimit_NEL->GetEntry()->GetIntNumber(),
				  YAxisMax);

    if(IntegrateFitToPearson_RB->IsDown())
      PearsonMiddleLimit_L->DrawLine(PearsonMiddleLimit_NEL->GetEntry()->GetIntNumber(),
				     YAxisMin,
				     PearsonMiddleLimit_NEL->GetEntry()->GetIntNumber(),
				     YAxisMax);
    
    PearsonUpperLimit_L->DrawLine(PearsonUpperLimit_NEL->GetEntry()->GetIntNumber(),
				  YAxisMin,
				  PearsonUpperLimit_NEL->GetEntry()->GetIntNumber(),
				  YAxisMax);
  }
  
  Canvas_EC->GetCanvas()->Update();
  
  if(FindPeaks_CB->IsDown())
    FindPeaks(Waveform_H[Channel]);
  
  // Set the class member int that tells the code what is currently
  // plotted on the embedded canvas. This is important for influencing
  // the behavior of the vertical and horizontal double sliders used
  // to "zoom" on the canvas contents
  CanvasContents = zWaveform;
  
  if(PlotPearsonIntegration_CB->IsDown())
    IntegratePearsonWaveform(true);
}


void ADAQAnalysisGUI::CreateSpectrum()
{
  // Delete the previous Spectrum_H TH1F object if it exists to
  // prevent memory leaks
  if(Spectrum_H){
    delete Spectrum_H;
    SpectrumExists = false;
  }
  
  // Reset the waveform progress bar
  if(SequentialArchitecture){
    ProcessingProgress_PB->Reset();
    ProcessingProgress_PB->SetBarColor(ColorManager->Number2Pixel(41));
    ProcessingProgress_PB->SetForegroundColor(ColorManager->Number2Pixel(1));

    ReadoutWidgetValues();
  }

  //; Create the TH1F histogram object for spectra creation
  Spectrum_H = new TH1F("Spectrum_H", "Pulse spectrum", SpectrumNumBins, SpectrumMinBin, SpectrumMaxBin);
  
  // Variables for calculating pulse height and area
  double SampleHeight, PulseHeight, PulseArea;
  SampleHeight = PulseHeight= PulseArea = 0.;

  // Reset the variable holding accumulated RFQ current if the value
  // was not loaded directly from an ADAQ ROOT file created during
  // parallel processing (i.e. a "despliced" file) and will be
  // calculated from scratch here
  if(!ADAQParResultsLoaded)
    TotalDeuterons = 0.;
  
  // Delete the previoue TSpectrum PeakFinder object if it exists to
  // prevent memory leaks
  if(PeakFinder)
    delete PeakFinder;
  
  // Create a TSpectrum PeakFinder using the appropriate widget to set
  // the maximum number of peaks that can be found
  PeakFinder = new TSpectrum(MaxPeaks);
    
  // Assign the range of waveforms that will be analyzed to create a
  // histogram. Note that in sequential architecture if N waveforms
  // are to be histogrammed, waveforms from waveform_ID == 0 to
  // waveform_ID == (WaveformsToHistogram-1) will be included in the
  // final spectra 
  WaveformStart = 0; // Start (include this waveform in final histogram)
  WaveformEnd = WaveformsToHistogram; // End (Up to but NOT including this waveform)

#ifdef MPI_ENABLED

  // If the waveform processing is to be done in parallel then
  // distribute the events as evenly as possible between the master
  // (rank == 0) and the slaves (rank != 0) to maximize computational
  // efficienct. Note that the master will carry the small leftover
  // waveforms from the even division of labor to the slaves.
  
  // Assign the number of waveforms processed by each slave
  int SlaveEvents = int(WaveformsToHistogram/MPI_Size);

  // Assign the number of waveforms processed by the master
  int MasterEvents = int(WaveformsToHistogram-SlaveEvents*(MPI_Size-1));

  if(ParallelVerbose and IsMaster)
    cout << "\nADAQAnalysisGUI_MPI Node[0]: Waveforms allocated to slaves (node != 0) = " << SlaveEvents << "\n"
	 <<   "                             Waveforms alloced to master (node == 0) =  " << MasterEvents
	 << endl;
  
  // Assign each master/slave a range of the total waveforms to
  // process based on the MPI rank. Note that WaveformEnd holds the
  // waveform_ID that the loop runs up to but does NOT include
  WaveformStart = (MPI_Rank * MasterEvents); // Start (include this waveform in the final histogram)
  WaveformEnd =  (MPI_Rank * MasterEvents) + SlaveEvents; // End (Up to but NOT including this waveform)
  
  if(ParallelVerbose)
    cout << "\nADAQAnalysisGUI_MPI Node[" << MPI_Rank << "] : Handling waveforms " << WaveformStart << " to " << (WaveformEnd-1)
	 << endl;

#endif
  
  bool PeaksFound = false;

  // Process the waveforms. 
  for(int waveform=WaveformStart; waveform<WaveformEnd; waveform++){
    // Run processing in a separate thread to enable use of the GUI by
    // the user while the spectrum is being created
    if(SequentialArchitecture)
      gSystem->ProcessEvents();
    
    // Get the data from the ADAQ TTree for the current waveform
    ADAQWaveformTree->GetEntry(waveform);
    
    // Assign the raw waveform voltage to a class member vector<int>
    RawVoltage = *WaveformVecPtrs[Channel];

    // Calculate the selected waveform that will be analyzed into the
    // spectrum histogram. Note that "raw" waveforms may not be
    // analyzed (simply due to how the code is presently setup) and
    // will default to analyzing the baseline subtracted
    if(RawWaveform or BaselineSubtractedWaveform)
      CalculateBSWaveform(Channel);
    else if(ZeroSuppressionWaveform)
      CalculateZSWaveform(Channel);
    
    // Calculate the total RFQ current for this waveform.
    if(IntegratePearson)
      IntegratePearsonWaveform(false);


    ///////////////////////////////
    // Whole waveform processing //
    ///////////////////////////////

    // If the entire waveform is to be used to calculate a pulse spectrum ...
    if(IntegrationTypeWholeWaveform){
      
      // If a pulse height spectrum (PHS) is to be created ...
      if(SpectrumTypePHS){
	// Get the pulse height by find the maximum bin value stored
	// in the Waveform_H TH1F via class methods. Note that spectra
	// are always created with positive polarity waveforms
	PulseHeight = Waveform_H[Channel]->GetBinContent(Waveform_H[Channel]->GetMaximumBin());

	// If the calibration manager is to be used to convert the
	// value from pulse units [ADC] to energy units [keV, MeV,
	// ...] then do so
	if(UseCalibrationManager[Channel])
	  PulseHeight = CalibrationManager[Channel]->Eval(PulseHeight);

	// Add the pulse height to the spectrum histogram	
	Spectrum_H->Fill(PulseHeight);
      }

      // ... or if a pulse area spectrum (PAS) is to be created ...
      else if(SpectrumTypePAS){

	// Reset the pulse area "integral" to zero
	PulseArea = 0.;
	
	// ...iterate through the bins in the Waveform_H histogram. If
	// the current bin value (the waveform voltage) is above the
	// floor, then add that bin to the pulse area integral
	for(int sample=0; sample<Waveform_H[Channel]->GetEntries(); sample++){
	  SampleHeight = Waveform_H[Channel]->GetBinContent(sample);

	  // ZSH: At present, the whole-waveform spectra creation
	  // simply integrates throught the entire waveform. Ideally,
	  // I would like to eliminate the signal noise by eliminating
	  // it from the integral with a threshold. But to avoid
	  // confusing the user and to ensure consistency, I will
	  // simply trust cancellation of + and - noise to avoid
	  // broadening the photo peaks. May want to implement a
	  // "minimum waveform height to be histogrammed" at some
	  // point in the future.
	  // if(SampleHeight >= 20)
	  PulseArea += SampleHeight;
	}
	
	// If the calibration manager is to be used to convert the
	// value from pulse units [ADC] to energy units [keV, MeV,
	// ...] then do so
	if(UseCalibrationManager[Channel])
	  PulseArea = CalibrationManager[Channel]->Eval(PulseArea);
	
	// Add the pulse area to the spectrum histogram
	Spectrum_H->Fill(PulseArea);
      }
      
      // Note that we must add a +1 to the waveform number in order to
      // get the modulo to land on the correct intervals
      if(IsMaster)
	if((waveform+1) % int(WaveformEnd*UpdateFreq*1.0/100) == 0)
	  UpdateProcessingProgress(waveform);
    }
    
    /////////////////////////////////////
    // Peak-finder waveform processing //
    /////////////////////////////////////
    
    // If the peak-finding/limit-finding algorithm is to be used to
    // create the pulse spectrum ...
    else if(IntegrationTypePeakFinder){
      
      // ...pass the Waveform_H[Channel] TH1F object to the peak-finding
      // algorithm, passing 'false' as the second argument to turn off
      // plotting of the waveforms. ADAQAnalysisGUI::FindPeaks() will
      // fill up a vector<PeakInfoStruct> that will be used to either
      // integrate the valid peaks to create a PAS or find the peak
      // heights to create a PHS, returning true. If zero peaks are
      // found in the waveform then FindPeaks() returns false
      PeaksFound = FindPeaks(Waveform_H[Channel], false);

      // If no peaks are present in the current waveform then continue
      // on to the next waveform for analysis
      if(!PeaksFound)
      	continue;
      
      // Calculate the PSD integrals and determine if they pass
      // the pulse-shape filterthrough the pulse-shape filter
      if(UsePSDFilterManager[PSDChannel])
	CalculatePSDIntegrals(false);
      
      // If a PAS is to be created ...
      if(SpectrumTypePAS)
	IntegratePeaks();
      
      // or if a PHS is to be created ...
      else if(SpectrumTypePHS)
	FindPeakHeights();

      // Note that we must add a +1 to the waveform number in order to
      // get the modulo to land on the correct intervals
      if(IsMaster)
	if((waveform+1) % int(WaveformEnd*UpdateFreq*1.0/100) == 0)
	  UpdateProcessingProgress(waveform);
    }
  }
  
  // Make final updates to the progress bar, ensuring that it reaches
  // 100% and changes color to acknoqledge that processing is complete
  if(SequentialArchitecture){
    ProcessingProgress_PB->Increment(100);
    ProcessingProgress_PB->SetBarColor(ColorManager->Number2Pixel(32));
    ProcessingProgress_PB->SetForegroundColor(ColorManager->Number2Pixel(0));
    
    // Update the number entry field in the "Analysis" tab to display
    // the total number of deuterons
    if(IntegratePearson)
      DeuteronsInTotal_NEFL->GetEntry()->SetNumber(TotalDeuterons);
  }
  
#ifdef MPI_ENABLED
  // Parallel waveform processing is complete at this point and it is
  // time to aggregate all the results from the nodes to the master
  // (node == 0) for output to a text file (for now). In the near
  // future, we will most likely persistently store a TH1F master
  // histogram object in a ROOT file (along with other variables) for
  // later retrieval
  
  // Ensure that all nodes reach this point before beginning the
  // aggregation by using an MPI barrier
  if(ParallelVerbose)
    cout << "\nADAQAnalysisGUI_MPI Node[" << MPI_Rank << "] : Reached the end-of-processing MPI barrier!"
	 << endl;

  // Ensure that all of the nodes have finished processing and
  // check-in at the barrier before proceeding  
  MPI::COMM_WORLD.Barrier();
  
  // In order to aggregate the Spectrum_H objects on each node to a
  // single Spectrum_H object on the master node, we perform the
  // following procedure:
  //
  // 0. Create a double array on each node 
  // 1. Store each node's Spectrum_H bin contents in the array
  // 2. Use MPI::Reduce on the array ptr to quickly aggregate the
  //    array values to the master's array (master == node 0)
  //
  // The size of the array must be (nbins+2) since ROOT TH1 objects
  // that are created with nbins have a underflow (bin = 0) and
  // overflow (bin = nbin+1) bin added onto the bins within range. For
  // example if a TH1 is created with 200 bins, the actual number of
  // bins in the TH1 object is 202 (content + under/overflow bins).
  
  // Set the size of the array for Spectrum_H readout
  const int ArraySize = SpectrumNumBins + 2;
  
  // Create the array for Spectrum_H readout
  double SpectrumArray[ArraySize];

  // Set array values to the bin content the node's Spectrum_H object
  for(int i=0; i<ArraySize; i++)
    SpectrumArray[i] = Spectrum_H->GetBinContent(i);
  
  // Get the total number of entries in the nod'es Spectrum_H object
  double Entries = Spectrum_H->GetEntries();
  
  if(ParallelVerbose)
    cout << "\nADAQAnalysisGUI_MPI Node[" << MPI_Rank << "] : Aggregating results to Node[0]!" << endl;
  
  // Use MPI::Reduce function to aggregate the arrays on each node
  // (which representing the Spectrum_H histogram) to a single array
  // on the master node.
  double *ReturnArray = SumDoubleArrayToMaster(SpectrumArray, ArraySize);

  // Use the MPI::Reduce function to aggregate the total number of
  // entries on each node to a single double on the master
  double ReturnDouble = SumDoublesToMaster(Entries);

  // Aggregate the total calculated RFQ current (if enabled) from all
  // nodes to the master node
  TotalDeuterons = SumDoublesToMaster(TotalDeuterons);
  
  // The master should output the array to a text file, which will be
  // read in by the running sequential binary of ADAQAnalysisGUI
  if(IsMaster){
    if(ParallelVerbose)
      cout << "\nADAQAnalysisGUI_MPI Node[0] : Writing master TH1F histogram to disk!\n"
	   << endl;
    
    // Create the master TH1F histogram object. Note that the member
    // data for spectrum creation are used to ensure the correct
    // number of bins and bin aranges
    MasterHistogram_H = new TH1F("MasterHistogram","MasterHistogram", SpectrumNumBins, SpectrumMinBin, SpectrumMaxBin);
    
    // Assign the bin content to the appropriate bins. Note that the
    // 'for loop' must include the TH1 overflow bin that exists at
    // (nbin+1) in order to capture all possible entries
    for(int i=0; i<ArraySize; i++)
      MasterHistogram_H->SetBinContent(i, ReturnArray[i]);
    
    // Set the total number of entries in the histogram. Because we
    // are setting the content of each bin with a single "entry"
    // weighted to the desired bin content value for each bin, the
    // number of entries will equal to the total number of
    // bins. However, we'd like to preserve the number of counts in
    // the histogram for statistics purposes;
    MasterHistogram_H->SetEntries(ReturnDouble);
    
    // Open the ROOT file that stores all the parallel processing data
    // in "update" mode such that we can append the master histogram
    ParallelProcessingFile = new TFile(ParallelProcessingFName.c_str(), "update");
    
    // Write the master histogram object to the ROOT file ...
    MasterHistogram_H->Write();
    
    // Create/write the aggregated current vector to a file
    TVectorD AggregatedDeuterons(1);
    AggregatedDeuterons[0] = TotalDeuterons;
    AggregatedDeuterons.Write("AggregatedDeuterons");
    
    // ... and write the ROOT file to disk
    ParallelProcessingFile->Write();
  }
#endif
  
  SpectrumExists = true;
}


void ADAQAnalysisGUI::PlotSpectrum()
{
  ////////////////////////////////////
  // Determine graphical attributes //
  ////////////////////////////////////

  if(!Spectrum_H){
    CreateMessageBox("A valid Spectrum_H object was not found! Spectrum plotting will abort!","Stop");
    return;
  }


  //////////////////////////////////
  // Determine main spectrum to plot

  //  TH1F *Spectrum2Plot_H = 0;
  
  if(SpectrumFindBackground_CB->IsDown() and SpectrumLessBackground_RB->IsDown())
    Spectrum2Plot_H = (TH1F *)SpectrumDeconvolved_H->Clone();
  else
    Spectrum2Plot_H = (TH1F *)Spectrum_H->Clone();
  

  //////////////////////
  // X and Y axis ranges

  float Min,Max;

  XAxisLimits_THS->GetPosition(Min,Max);
  XAxisMin = Spectrum2Plot_H->GetXaxis()->GetXmax() * Min;
  XAxisMax = Spectrum2Plot_H->GetXaxis()->GetXmax() * Max;

  Spectrum2Plot_H->GetXaxis()->SetRangeUser(XAxisMin, XAxisMax);

  YAxisLimits_DVS->GetPosition(Min,Max);
  if(PlotVerticalAxisInLog_CB->IsDown() and Max==1)
    YAxisMin = 1.0;
  else if(SpectrumFindBackground_CB->IsDown() and SpectrumLessBackground_RB->IsDown())
    YAxisMin = (Spectrum2Plot_H->GetMaximumBin() * (1-Max)) - (Spectrum2Plot_H->GetMaximumBin()*0.8);
  else
    YAxisMin = Spectrum2Plot_H->GetBinContent(Spectrum2Plot_H->GetMaximumBin()) * (1-Max);

  YAxisMax = Spectrum2Plot_H->GetBinContent(Spectrum2Plot_H->GetMaximumBin()) * (1-Min) * 1.05;
  
  Spectrum2Plot_H->SetMinimum(YAxisMin);
  Spectrum2Plot_H->SetMaximum(YAxisMax);

  
  /////////////////
  // Spectrum title

  if(OverrideTitles_CB->IsDown()){
    // Assign the titles to strings
    Title = Title_TEL->GetEntry()->GetText();
    XAxisTitle = XAxisTitle_TEL->GetEntry()->GetText();
    YAxisTitle = YAxisTitle_TEL->GetEntry()->GetText();
  }
  // ... otherwise use the default titles
  else{
    // Assign the default titles
    Title = "Pulse Spectrum";
    XAxisTitle = "Pulse units [ADC]";
    YAxisTitle = "Counts";
  }

  // Get the desired axes divisions
  int XAxisDivs = XAxisDivs_NEL->GetEntry()->GetIntNumber();
  int YAxisDivs = YAxisDivs_NEL->GetEntry()->GetIntNumber();

  
  ////////////////////
  // Statistics legend

  if(SetStatsOff_CB->IsDown())
    Spectrum2Plot_H->SetStats(false);
  else
    Spectrum2Plot_H->SetStats(true);


  /////////////////////
  // Logarithmic Y axis
  
  if(PlotVerticalAxisInLog_CB->IsDown())
    gPad->SetLogy(true);
  else
    gPad->SetLogy(false);

  Canvas_EC->GetCanvas()->SetLeftMargin(0.13);
  Canvas_EC->GetCanvas()->SetBottomMargin(0.12);
  Canvas_EC->GetCanvas()->SetRightMargin(0.05);

  ////////////////////////////////
  // Axis and graphical attributes

  Spectrum2Plot_H->SetTitle(Title.c_str());

  Spectrum2Plot_H->GetXaxis()->SetTitle(XAxisTitle.c_str());
  Spectrum2Plot_H->GetXaxis()->SetTitleSize(XAxisSize_NEL->GetEntry()->GetNumber());
  Spectrum2Plot_H->GetXaxis()->SetLabelSize(XAxisSize_NEL->GetEntry()->GetNumber());
  Spectrum2Plot_H->GetXaxis()->SetTitleOffset(XAxisOffset_NEL->GetEntry()->GetNumber());
  Spectrum2Plot_H->GetXaxis()->CenterTitle();
  Spectrum2Plot_H->GetXaxis()->SetNdivisions(XAxisDivs, true);

  Spectrum2Plot_H->GetYaxis()->SetTitle(YAxisTitle.c_str());
  Spectrum2Plot_H->GetYaxis()->SetTitleSize(YAxisSize_NEL->GetEntry()->GetNumber());
  Spectrum2Plot_H->GetYaxis()->SetLabelSize(YAxisSize_NEL->GetEntry()->GetNumber());
  Spectrum2Plot_H->GetYaxis()->SetTitleOffset(YAxisOffset_NEL->GetEntry()->GetNumber());
  Spectrum2Plot_H->GetYaxis()->CenterTitle();
  Spectrum2Plot_H->GetYaxis()->SetNdivisions(YAxisDivs, true);

  Spectrum2Plot_H->SetLineColor(4);
  Spectrum2Plot_H->SetLineWidth(2);

  /////////////////////////
  // Plot the main spectrum
  string DrawString;
  if(DrawSpectrumWithBars_RB->IsDown()){
    Spectrum2Plot_H->SetFillColor(4);
    DrawString = "B";
  }
  else if(DrawSpectrumWithCurve_RB->IsDown())
    DrawString = "C";
  else if(DrawSpectrumWithMarkers_RB->IsDown()){
    Spectrum2Plot_H->SetMarkerStyle(24);
    Spectrum2Plot_H->SetMarkerColor(4);
    Spectrum2Plot_H->SetMarkerSize(1.0);
    DrawString = "P";
  }
  
  Spectrum2Plot_H->Draw(DrawString.c_str());

  ////////////////////////////////////////////
  // Overlay the background spectra if desired
  if(SpectrumFindBackground_CB->IsDown() and SpectrumWithBackground_RB->IsDown())
    SpectrumBackground_H->Draw("C SAME");


  if(SpectrumFindIntegral_CB->IsDown())
    IntegrateSpectrum();
  
  // Update the canvas
  Canvas_EC->GetCanvas()->Update();

  // Set the class member int that tells the code what is currently
  // plotted on the embedded canvas. This is important for influencing
  // the behavior of the vertical and horizontal double sliders used
  // to "zoom" on the canvas contents
  CanvasContents = zSpectrum;
}

// Method to extract the digitized data on the specified data channel
// and store it into a TH1F object as a "raw" waveform. Note that the
// baseline must be calculated in this method even though it is not
// used to operate on the waveform
void ADAQAnalysisGUI::PlotPSDHistogram()
{
  // Check to ensure that the PSD histogram object exists!
  if(!PSDHistogram_H){
    CreateMessageBox("A valid PSDHistogram object was not found! PSD histogram plotting will abort!","Stop");
    return;
  }

  // If the PSD histogram has zero events then alert the user and
  // abort plotting. This can happen, for example, if the PSD channel
  // was incorrectly set to an "empty" data channel
  if(PSDHistogram_H->GetEntries() == 0){
    CreateMessageBox("The PSD histogram had zero entries! Recheck PSD settings and take another whirl around the merry-go-round!","Stop");
    return;
  }
  
  //////////////////////
  // X and Y axis ranges
  
  float Min,Max;

  XAxisLimits_THS->GetPosition(Min,Max);
  XAxisMin = PSDHistogram_H->GetXaxis()->GetXmax() * Min;
  XAxisMax = PSDHistogram_H->GetXaxis()->GetXmax() * Max;
  PSDHistogram_H->GetXaxis()->SetRangeUser(XAxisMin, XAxisMax);

  YAxisLimits_DVS->GetPosition(Min,Max);
  YAxisMin = PSDHistogram_H->GetYaxis()->GetXmax() * (1-Max);
  YAxisMax = PSDHistogram_H->GetYaxis()->GetXmax() * (1-Min);
  PSDHistogram_H->GetYaxis()->SetRangeUser(YAxisMin, YAxisMax);

  if(OverrideTitles_CB->IsDown()){
    Title = Title_TEL->GetEntry()->GetText();
    XAxisTitle = XAxisTitle_TEL->GetEntry()->GetText();
    YAxisTitle = YAxisTitle_TEL->GetEntry()->GetText();
    ZAxisTitle = ZAxisTitle_TEL->GetEntry()->GetText();
    PaletteAxisTitle = PaletteAxisTitle_TEL->GetEntry()->GetText();
  }
  else{
    Title = "Pulse Shape Discrimination Integrals";
    XAxisTitle = "Waveform total integral [ADC]";
    YAxisTitle = "Waveform tail integral [ADC]";
    ZAxisTitle = "Number of waveforms";
    PaletteAxisTitle ="";
  }

  // Get the desired axes divisions
  int XAxisDivs = XAxisDivs_NEL->GetEntry()->GetIntNumber();
  int YAxisDivs = YAxisDivs_NEL->GetEntry()->GetIntNumber();
  int ZAxisDivs = ZAxisDivs_NEL->GetEntry()->GetIntNumber();

  // Hack to make the X axis of the 2D PSD histogram actually look
  // decent given the large numbers involved
  if(!OverrideTitles_CB->IsDown())
    XAxisDivs = 505;
  
  if(SetStatsOff_CB->IsDown())
    PSDHistogram_H->SetStats(false);
  else
    PSDHistogram_H->SetStats(true);

  Canvas_EC->GetCanvas()->SetLeftMargin(0.13);
  Canvas_EC->GetCanvas()->SetBottomMargin(0.12);
  Canvas_EC->GetCanvas()->SetRightMargin(0.2);

  ////////////////////////////////
  // Axis and graphical attributes

  if(PlotVerticalAxisInLog_CB->IsDown())
    gPad->SetLogz(true);
  else
    gPad->SetLogz(false);

  // The X and Y axes should never be plotted in log
  gPad->SetLogx(false);
  gPad->SetLogy(false);

  PSDHistogram_H->SetTitle(Title.c_str());
  
  PSDHistogram_H->GetXaxis()->SetTitle(XAxisTitle.c_str());
  PSDHistogram_H->GetXaxis()->SetTitleSize(XAxisSize_NEL->GetEntry()->GetNumber());
  PSDHistogram_H->GetXaxis()->SetLabelSize(XAxisSize_NEL->GetEntry()->GetNumber());
  PSDHistogram_H->GetXaxis()->SetTitleOffset(XAxisOffset_NEL->GetEntry()->GetNumber());
  PSDHistogram_H->GetXaxis()->CenterTitle();
  PSDHistogram_H->GetXaxis()->SetNdivisions(XAxisDivs, true);

  PSDHistogram_H->GetYaxis()->SetTitle(YAxisTitle.c_str());
  PSDHistogram_H->GetYaxis()->SetTitleSize(YAxisSize_NEL->GetEntry()->GetNumber());
  PSDHistogram_H->GetYaxis()->SetLabelSize(YAxisSize_NEL->GetEntry()->GetNumber());
  PSDHistogram_H->GetYaxis()->SetTitleOffset(YAxisOffset_NEL->GetEntry()->GetNumber());
  PSDHistogram_H->GetYaxis()->CenterTitle();
  PSDHistogram_H->GetYaxis()->SetNdivisions(YAxisDivs, true);

  PSDHistogram_H->GetZaxis()->SetTitle(ZAxisTitle.c_str());
  PSDHistogram_H->GetZaxis()->SetTitleSize(ZAxisSize_NEL->GetEntry()->GetNumber());
  PSDHistogram_H->GetZaxis()->SetTitleOffset(ZAxisOffset_NEL->GetEntry()->GetNumber());
  PSDHistogram_H->GetZaxis()->CenterTitle();
  PSDHistogram_H->GetZaxis()->SetLabelSize(ZAxisSize_NEL->GetEntry()->GetNumber());
  PSDHistogram_H->GetZaxis()->SetNdivisions(ZAxisDivs, true);

  int PlotTypeID = PSDPlotType_CBL->GetComboBox()->GetSelected();
  switch(PlotTypeID){
  case 0:
    PSDHistogram_H->Draw("COLZ");
    break;

  case 1:
    PSDHistogram_H->Draw("LEGO2 0Z");
    break;

  case 2:
    PSDHistogram_H->Draw("SURF3 Z");
    break;
    
  case 3: 
    PSDHistogram_H->SetFillColor(kOrange);
    PSDHistogram_H->Draw("SURF4");
    break;
    
  case 4:
    PSDHistogram_H->Draw("CONT Z");
    break;
  }

  // Modify the histogram color palette only for the histograms that
  // actually create a palette
  if(PlotTypeID != 3){

    // The canvas must be updated before the TPaletteAxis is accessed
    Canvas_EC->GetCanvas()->Update();

    TPaletteAxis *ColorPalette = (TPaletteAxis *)PSDHistogram_H->GetListOfFunctions()->FindObject("palette");
    ColorPalette->GetAxis()->SetTitle(PaletteAxisTitle.c_str());
    ColorPalette->GetAxis()->SetTitleSize(PaletteAxisSize_NEL->GetEntry()->GetNumber());
    ColorPalette->GetAxis()->SetTitleOffset(PaletteAxisOffset_NEL->GetEntry()->GetNumber());
    ColorPalette->GetAxis()->CenterTitle();
    ColorPalette->GetAxis()->SetLabelSize(PaletteAxisSize_NEL->GetEntry()->GetNumber());
    
    ColorPalette->SetX1NDC(PaletteX1_NEL->GetEntry()->GetNumber());
    ColorPalette->SetX2NDC(PaletteX2_NEL->GetEntry()->GetNumber());
    ColorPalette->SetY1NDC(PaletteY1_NEL->GetEntry()->GetNumber());
    ColorPalette->SetY2NDC(PaletteY2_NEL->GetEntry()->GetNumber());
    
    ColorPalette->Draw("SAME");
  }
  
  if(PSDEnableFilterCreation_CB->IsDown())
    PlotPSDFilter();

  CanvasContents = zPSDHistogram;
  
  Canvas_EC->GetCanvas()->Update();
}


void ADAQAnalysisGUI::PlotPSDHistogramSlice(int XPixel, int YPixel)
{
  // pixel coordinates: refers to an (X,Y) position on the canvas
  //                    using X and Y pixel IDs. The (0,0) pixel is in
  //                    the upper-left hand corner of the canvas
  //
  // user coordinates: refers to a position on the canvas using the X
  //                   and Y axes of the plotted object to assign a
  //                   X and Y value to a particular pixel
  
  // Get the cursor position in user coordinates
  double XPos = gPad->AbsPixeltoX(XPixel);
  double YPos = gPad->AbsPixeltoY(YPixel);

  // Get the min/max x values in user coordinates
  double XMin = gPad->GetUxmin();
  double XMax = gPad->GetUxmax();

  // Get the min/max y values in user coordinates
  double YMin = gPad->GetUymin();
  double YMax = gPad->GetUymax();

  // Compute the min/max x values in pixel coordinates
  double PixelXMin = gPad->XtoAbsPixel(XMin);
  double PixelXMax = gPad->XtoAbsPixel(XMax);

  // Compute the min/max y values in pixel coodinates
  double PixelYMin = gPad->YtoAbsPixel(YMin);
  double PixelYMax = gPad->YtoAbsPixel(YMax);
  
  // Enable the canvas feedback mode to help in smoothly plotting a
  // line representing the user's desired value
  gPad->GetCanvas()->FeedbackMode(kTRUE);
  
  ////////////////////////////////////////
  // Draw a line representing the slice //
  ////////////////////////////////////////
  // A line will be drawn along the X or Y slice depending on the
  // user's selection and current position on the canvas object. Note
  // that the unique pad ID is used to prevent having to redraw the
  // TH2F PSDHistogram_H object each time the user moves the cursor

  if(PSDHistogramSliceX_RB->IsDown()){
    int XPixelOld = gPad->GetUniqueID();
    
    gVirtualX->DrawLine(XPixelOld, PixelYMin, XPixelOld, PixelYMax);
    gVirtualX->DrawLine(XPixel, PixelYMin, XPixel, PixelYMax);

    gPad->SetUniqueID(XPixel);
  }
  else if(PSDHistogramSliceY_RB->IsDown()){

    int YPixelOld = gPad->GetUniqueID();
  
    gVirtualX->DrawLine(PixelXMin, YPixelOld, PixelXMax, YPixelOld);

    gVirtualX->DrawLine(PixelXMin, YPixel, PixelXMax, YPixel);
    
    gPad->SetUniqueID(YPixel);
  }

  
  ////////////////////////////////////////
  // Draw the TH1D PSDHistogram_H slice //
  ////////////////////////////////////////
  // A 1D histogram representing the slice along X (or Y) at the value
  // specified by the cursor position on the canvas will be drawn in a
  // separate canvas to enable continual selection of slices. The PSD
  // histogram slice canvas is kept open until the user turns off
  // histogram slicing to enable smooth plotting

  ////////////////////
  // Canvas assignment

  string SliceCanvasName = "PSDSlice_C";
  string SliceHistogramName = "PSDSlice_H";

  // Get the list of current canvas objects
  TCanvas *PSDSlice_C = (TCanvas *)gROOT->GetListOfCanvases()->FindObject(SliceCanvasName.c_str());

  // If the canvas then delete the contents to prevent memory leaks
  if(PSDSlice_C)
    delete PSDSlice_C->GetPrimitive(SliceHistogramName.c_str());
  
  // ... otherwise, create a new canvas
  else{
    PSDSlice_C = new TCanvas(SliceCanvasName.c_str(), "PSD Histogram Slice", 700, 500, 600, 400);
    PSDSlice_C->SetGrid(true);
    PSDSlice_C->SetLeftMargin(0.13);
    PSDSlice_C->SetBottomMargin(0.13);
  }
  
  // Ensure the PSD slice canvas is the active canvas
  PSDSlice_C->cd();


  /////////////////////////////////
  // Create the PSD slice histogram

  // An integer to determine which bin in the TH2F PSDHistogram_H
  // object should be sliced
  int PSDSliceBin = -1;

  // The 1D histogram slice object
  TH1D *PSDSlice_H = 0;

  string HistogramTitle, XAxisTitle;

  // Create a slice at a specific "X" (total pulse integral) value,
  // i.e. create a 1D histogram of the "Y" (tail pulse integrals)
  // values at a specific value of "X" (total pulse integral).
  if(PSDHistogramSliceX_RB->IsDown()){
    PSDSliceBin = PSDHistogram_H->GetXaxis()->FindBin(gPad->PadtoX(XPos));
    PSDSlice_H = PSDHistogram_H->ProjectionY("",PSDSliceBin,PSDSliceBin);

    stringstream ss;
    ss << "PSDHistogram X slice at " << XPos << " ADC";
    HistogramTitle = ss.str();

    XAxisTitle = PSDHistogram_H->GetYaxis()->GetTitle();
  }
  
  // Create a slice at a specific "Y" (tail pulse integral) value,
  // i.e. create a 1D histogram of the "X" (total pulse integrals)
  // values at a specific value of "Y" (tail pulse integral).
  else{
    PSDSliceBin = PSDHistogram_H->GetYaxis()->FindBin(gPad->PadtoY(YPos));
    PSDSlice_H = PSDHistogram_H->ProjectionX("",PSDSliceBin,PSDSliceBin);

    stringstream ss;
    ss << "PSDHistogram Y slice at " << YPos << " ADC";
    HistogramTitle = ss.str();
    
    XAxisTitle = PSDHistogram_H->GetXaxis()->GetTitle();
  }
  
  /////////////////////////////////
  // Set slice histogram attributes
  
  PSDSlice_H->SetName(SliceHistogramName.c_str());
  PSDSlice_H->SetTitle(HistogramTitle.c_str());

  PSDSlice_H->GetXaxis()->SetTitle(XAxisTitle.c_str());
  PSDSlice_H->GetXaxis()->SetTitleSize(0.05);
  PSDSlice_H->GetXaxis()->SetTitleOffset(1.1);
  PSDSlice_H->GetXaxis()->SetLabelSize(0.05);
  PSDSlice_H->GetXaxis()->CenterTitle();
  
  PSDSlice_H->GetYaxis()->SetTitle("Counts");
  PSDSlice_H->GetYaxis()->SetTitleSize(0.05);
  PSDSlice_H->GetYaxis()->SetTitleOffset(1.1);
  PSDSlice_H->GetYaxis()->SetLabelSize(0.05);
  PSDSlice_H->GetYaxis()->CenterTitle();
  
  PSDSlice_H->SetFillColor(4);
  PSDSlice_H->Draw("B");

  // Save the histogram  to a class member TH1F object
  if(PSDHistogramSlice_H) delete PSDHistogramSlice_H;
  PSDHistogramSlice_H = (TH1D *)PSDSlice_H->Clone("PSDHistogramSlice_H");
  PSDHistogramSliceExists = true;
  
  // Update the standalone canvas
  PSDSlice_C->Update();

  gPad->GetCanvas()->FeedbackMode(kFALSE);
  
  // Reset the main embedded canvas to active
  Canvas_EC->GetCanvas()->cd();
}


void ADAQAnalysisGUI::CalculateRawWaveform(int Channel)
{
  vector<int> RawVoltage = *WaveformVecPtrs[Channel];

  Baseline = CalculateBaseline(&RawVoltage);
  
  if(Waveform_H[Channel])
    delete Waveform_H[Channel];
  
  Waveform_H[Channel] = new TH1F("Waveform_H", "Raw Waveform", RecordLength-1, 0, RecordLength);
  
  vector<int>::iterator iter;
  for(iter=RawVoltage.begin(); iter!=RawVoltage.end(); iter++)
    Waveform_H[Channel]->SetBinContent((iter-RawVoltage.begin()), *iter);
}


// Method to extract the digitized data on the specified data channel
// and store it into a TH1F object after calculating and subtracting
// the waveform baseline. Note that the function argument bool is used
// to determine which polarity (a standard (detector) waveform or a
// Pearson (RFQ current) waveform) should be used in processing
void ADAQAnalysisGUI::CalculateBSWaveform(int Channel, bool CurrentWaveform)
{
  double Polarity;
  (CurrentWaveform) ? Polarity = PearsonPolarity : Polarity = WaveformPolarity;
  
  vector<int> RawVoltage = *WaveformVecPtrs[Channel];
  
  Baseline = CalculateBaseline(&RawVoltage);
  
  if(Waveform_H[Channel])
    delete Waveform_H[Channel];
  
  Waveform_H[Channel] = new TH1F("Waveform_H","Baseline-subtracted Waveform", RecordLength-1, 0, RecordLength);
  
  vector<int>::iterator it;  
  for(it=RawVoltage.begin(); it!=RawVoltage.end(); it++)
    Waveform_H[Channel]->SetBinContent((it-RawVoltage.begin()),
				       Polarity*(*it-Baseline));
}


// Method to extract the digitized data on the specified data channel
// and store it into a TH1F object after computing the zero-suppresion
// (ZS) waveform. The baseline is calculated and stored into the class
// member for later use.  Note that the function argument bool is used
// to determine which polarity (a standard (detector) waveform or a
// Pearson (RFQ current) waveform) should be used in processing
void ADAQAnalysisGUI::CalculateZSWaveform(int Channel, bool CurrentWaveform)
{
  double Polarity;
  (CurrentWaveform) ? Polarity = PearsonPolarity : Polarity = WaveformPolarity;

  vector<int> RawVoltage = *WaveformVecPtrs[Channel];

  Baseline = CalculateBaseline(&RawVoltage);
  
  vector<double> ZSVoltage;
  vector<int> ZSPosition;
  
  vector<int>::iterator it;
  for(it=RawVoltage.begin(); it!=RawVoltage.end(); it++){
    
    double VoltageMinusBaseline = Polarity*(*it-Baseline);
    
    if(VoltageMinusBaseline >= ZeroSuppressionCeiling){
      ZSVoltage.push_back(VoltageMinusBaseline);
      ZSPosition.push_back(it - RawVoltage.begin());
    }
  }
  
  int ZSWaveformSize = ZSVoltage.size();
  
  if(Waveform_H[Channel])
    delete Waveform_H[Channel];
  
  Waveform_H[Channel] = new TH1F("Waveform_H","Zero Suppression Waveform", ZSWaveformSize-1, 0, ZSWaveformSize);
  
  vector<double>::iterator iter;
  for(iter=ZSVoltage.begin(); iter!=ZSVoltage.end(); iter++)
    Waveform_H[Channel]->SetBinContent((iter-ZSVoltage.begin()), *iter);
}


// Method to calculate the baseline of a waveform. The "baseline" is
// simply the average of the waveform voltage taken over the specified
// range in time (in units of samples)
double ADAQAnalysisGUI::CalculateBaseline(vector<int> *Waveform)
{
  int BaselineCalcLength = BaselineCalcMax - BaselineCalcMin;
  double Baseline = 0.;
  for(int sample=BaselineCalcMin; sample<BaselineCalcMax; sample++)
    Baseline += ((*Waveform)[sample]*1.0/BaselineCalcLength);
  
  return Baseline;
}


// Method used to find peaks in any TH1F object.  
bool ADAQAnalysisGUI::FindPeaks(TH1F *Histogram_H, bool PlotPeaksAndGraphics)
{
  // Use the PeakFinder to determine the number of potential peaks in
  // the waveform and return the total number of peaks that the
  // algorithm has found. "Potential" peaks are those that meet the
  // criterion of the TSpectrum::Search algorithm with the
  // user-specified (via the appropriate widgets) tuning
  // parameters. They are "potential" because I want to impose a
  // minimum threshold criterion (called the "floor") that potential
  // peaks must meet before being declared real peaks. Note that the
  // plotting of markers on TSpectrum's peaks have been disabled so
  // that I can plot my "successful" peaks once they are determined
  //
  // sigma = distance between allowable peak finds
  // resolution = fraction of max peak above which peaks are valid
  
  int NumPotentialPeaks = PeakFinder->Search(Histogram_H,				    
					     Sigma,
					     "goff", 
					     Resolution);
  
  // Since the PeakFinder actually found potential peaks then get the
  // X and Y positions of the potential peaks from the PeakFinder
  float *PotentialPeakPosX = PeakFinder->GetPositionX();
  float *PotentialPeakPosY = PeakFinder->GetPositionY();
  
  // Initialize the counter of successful peaks to zero and clear the
  // peak info vector in preparation for the next set of peaks
  NumPeaks = 0;
  PeakInfoVec.clear();
  
  // For each of the potential peaks found by the PeakFinder...
  for(int peak=0; peak<NumPotentialPeaks; peak++){
    
    // Determine if the peaks Y value (voltage) is above the "floor"
    //if(PotentialPeakPosY[peak] > Floor_NEL->GetEntry()->GetIntNumber()){
    if(PotentialPeakPosY[peak] > Floor){
      
      // Success! This peak was a winner. Let's give it a prize.
      
      // Increment the counter for successful peaks in the present waveform
      NumPeaks++;

      // Increment the counter for total peaks in all processed waveforms
      TotalPeaks++;
      
      // Create a PeakInfoStruct to hold the current successful peaks'
      // information. Store the X and Y position in the structure and
      // push it back into the dedicated vector for later use
      PeakInfoStruct PeakInfo;
      PeakInfo.PeakID = NumPeaks;
      PeakInfo.PeakPosX = PotentialPeakPosX[peak];
      PeakInfo.PeakPosY = PotentialPeakPosY[peak];
      PeakInfoVec.push_back(PeakInfo);
      
      if(PlotPeaksAndGraphics){
	// Create a marker that will be plotted at the peaks position;
	// this marker substitutes for the default TSpectrum markers
	// that have been turned off above
	TMarker *PeakMarker = new TMarker(PeakInfo.PeakPosX, 
					  PeakInfo.PeakPosY + (PeakInfo.PeakPosY*0.15), 
					  peak);
	PeakMarker->SetMarkerColor(2);
	PeakMarker->SetMarkerStyle(23);
	PeakMarker->SetMarkerSize(2.0);
	PeakMarker->Draw();
      }
    }
  }
  
  // The user has the option to plot the floor used to determine
  // whether potential peaks are above the threshold
  if(PlotFloor and PlotPeaksAndGraphics)
    Floor_L->DrawLine(XAxisMin, Floor, XAxisMax, Floor);
  
  // Call the member functions that will find the lower (leftwards on
  // the time axis) and upper (rightwards on the time axis)
  // integration limits for each successful peak in the waveform
  FindPeakLimits(Histogram_H, PlotPeaksAndGraphics);

  // Update the embedded canvas to reflect the vast cornucopia of
  // potential graphical options that may have been added if the
  // plotting boolean is set
  if(PlotPeaksAndGraphics)
    Canvas_EC->GetCanvas()->Update();  
  
  // Function returns 'false' if zero peaks are found; algorithms can
  // use this flag to exit from analysis for this acquisition window
  // to save on CPU time
  if(NumPeaks == 0)
    return false;
  else
    return true;
}


// Method to find the lower/upper peak limits in any histogram.
void ADAQAnalysisGUI::FindPeakLimits(TH1F *Histogram_H, bool PlotPeaksAndGraphics)
{
  // Vector that will hold the sample number after which the floor was
  // crossed from the low (below the floor) to the high (above the
  // floor) side. The vector will eventually hold all the candidates for
  // a detector waveform rising edge
  vector<int> FloorCrossing_Low2High;

  // Vector that will hold the sample number before which the floor
  // was crossed from the high (above the floor) to the low (above the
  // floor) side. The vector will eventually hold all the candidates
  // for a detector waveform decay tail back to the baseline
  vector<int> FloorCrossing_High2Low;

  // Clear the vector that stores the lower and upper peak limits for
  // all "successful" peaks in the present histogram. We are done with
  // the old ones and have moved on to a new histogram so we need to
  // start with a clean slate
  PeakLimits.clear();

  // Get the number of bins in the current histogram
  int NumBins = Histogram_H->GetNbinsX();

  double PreStepValue, PostStepValue;

  // Iterate through the waveform to look for floor crossings ...
  for(int sample=1; sample<NumBins; sample++){

    PreStepValue = Histogram_H->GetBinContent(sample-1);
    PostStepValue = Histogram_H->GetBinContent(sample);
    
    // If a low-to-high floor crossing occurred ...
    if(PreStepValue<Floor and PostStepValue>=Floor){
      
      // Push the PreStep sample index (sample below the floor) into the
      // appropriate vector for later use.
      FloorCrossing_Low2High.push_back(sample-1);
      
      // The user has the options to plot the low-2-high crossings
      //      if(PlotCrossings_CB->IsDown() and PlotPeaksAndGraphics){
      if(PlotCrossings and PlotPeaksAndGraphics){
	TMarker *CrossingMarker = new TMarker(sample, PreStepValue,0);
	CrossingMarker->SetMarkerColor(6);
	CrossingMarker->SetMarkerStyle(29);
	CrossingMarker->SetMarkerSize(1.5);
	CrossingMarker->Draw();
      }
    }

    // If a high-to-low floor crossing occurred ...
    if(PreStepValue>=Floor and PostStepValue<Floor){
            
      // Push the PostStep sample index (sample below floor) into the
      // appropriate vector later use
      FloorCrossing_High2Low.push_back(sample);

      // The user has the options to plot the high-2-low crossings
      if(PlotCrossings and PlotPeaksAndGraphics){
	TMarker *CrossingMarker = new TMarker(sample, PostStepValue, 0);
	CrossingMarker->SetMarkerColor(7);
	CrossingMarker->SetMarkerStyle(29);
	CrossingMarker->SetMarkerSize(1.5);
	CrossingMarker->Draw();
      }
    }
  }

  // For each peak located by the TSpectrum PeakFinder and determined
  // to be above the floor, the following (probably inefficient)
  // determines the closest sample on either side of each peak that
  // crosses the floor. To the left of the peak (in time), the
  // crossing is a "low-2-high" crossing; to the right of the peak (in
  // time), the crossing is a "high-2-low" crossing. 

  vector<PeakInfoStruct>::iterator peak_iter;
  for(peak_iter=PeakInfoVec.begin(); peak_iter!=PeakInfoVec.end(); peak_iter++){
    
    // Counters which will be incremented conditionally such that the
    // correct samples for low-2-high and high-2-low crossings can be
    // accessed after the conditional testing for crossings
    int FloorCrossing_Low2High_index = -1;
    int FloorCrossing_High2Low_index = -1;
    
    // Iterator to loop over the FloorCrossing_* vectors, which as
    // you'll recall hold all the low-2-high and high-2-low crossings,
    // respectively, that were found for the digitized waveform
    vector<int>::iterator it;

    // Iterate over the vector containing all the low-2-high crossings
    // in the waveform...
    
    if(FloorCrossing_Low2High.size() == 1)
      FloorCrossing_Low2High_index = 0;
    else{
      for(it=FloorCrossing_Low2High.begin(); it!=FloorCrossing_Low2High.end(); it++){
	
	// The present algorithm determines the lower integration
	// limit by considering the closest low-2-high floor crossing
	// on the left side of the peak, i.e. the algorithm identifies
	// the rising edge of a detector pulse closest to the
	// peak. Here, we test to determine if the present low-2-high
	// crossing causes the peak position minues the low-2-high to
	// go negative. If so, we break without incrementing the
	// index; otherwise, we increment the index and continue
	// looping.

	if((*peak_iter).PeakPosX - *it < 0)
	  break;
	
	FloorCrossing_Low2High_index++;
      }
    }

    // Iterate over the vector containing all the high-2-low crossings
    // in the waveform...
    
    if(FloorCrossing_High2Low.size() == 1)
      FloorCrossing_High2Low_index = 0;
    else{
      for(it=FloorCrossing_High2Low.begin(); it!=FloorCrossing_High2Low.end(); it++){
      	
	// The present algorithm determines the upper integration
	// limit by considering the closest high-2-low floor crossing
	// on the right side of the peak, i.e. the algorithm
	// identifies the falling edge of a detector pulse closest to
	// the peak. Here, the FloorCrossing_High2Low_index is
	// incremented until the first high-2-low crossing is found on
	// the right side of the peak, i.e. the peak position less the
	// low-2-high crossing becomes negative. The index variable
	// can then be used later to extract the correct lower limit

	FloorCrossing_High2Low_index++;
	
	if((*peak_iter).PeakPosX - *it < 0)
	  break;
	
      }
    }
  	
    // Very rare events (more often when triggering of the RFQ timing
    // pulse) can cause waveforms that have detector pulses whose
    // voltages exceed what is the obvious the baseline during the
    // start (sample=0) or end (sample=RecordLength) of the waveform,
    // The present algorithm will not be able to determine the correct
    // lower and upper integration limits and, hence, the
    // FloorCrossing_*_index will remain at its -1 initialization
    // value. The -1 will be used later to exclude this peak from
    // analysis

    if((int)FloorCrossing_Low2High.size() < FloorCrossing_Low2High_index or
       (int)FloorCrossing_High2Low.size() < FloorCrossing_High2Low_index){
      
      cout << "FloorCrossing_Low2High.size() = " << FloorCrossing_Low2High.size()  << "\n"
	   << "FloorCrossing_Low2High_index = " << FloorCrossing_Low2High_index << "\n"
	   << endl;
      
      cout << "FloorCrossing_High2Low.size() = " << FloorCrossing_High2Low.size()  << "\n"
	   << "FloorCrossing_High2Low_index = " << FloorCrossing_High2Low_index << "\n"
	   << endl;
    }

    // If the algorithm has successfully determined both low and high
    // floor crossings (which now become the lower and upper
    // integration limits for the present peak)...
    if(FloorCrossing_Low2High_index > -1 and FloorCrossing_High2Low_index > -1){
      (*peak_iter).PeakLimit_Lower = FloorCrossing_Low2High[FloorCrossing_Low2High_index];
      (*peak_iter).PeakLimit_Upper = FloorCrossing_High2Low[FloorCrossing_High2Low_index];
    }
  }

  // The following call examines each peak in the now-full PeakInfoVec
  // to determine whether or not the peak is part of a "piled-up"
  // peak. If so, the PeakInfoVec.PileupFlag is marked true to flag
  // the peak to any later analysis methods
  if(UsePileupRejection)
    RejectPileup(Histogram_H);

  
  // Set the Y-axis limits for drawing TLines and a TBox
  // representing the integration region
  for(peak_iter=PeakInfoVec.begin(); peak_iter!=PeakInfoVec.end(); peak_iter++){
    
    if(PlotPeakIntegratingRegion and PlotPeaksAndGraphics){
      
      // The user has the option to plot vertical TLines representing
      // the closest low-2-high and high-2-low floow crossing points for
      // each peak. The option also plots a TBox around the peak
      // representing the integration region. Note the need to increment
      // by one extra when accessing the correct high-2-low floor
      // crossing since the above loop (at present) determines the
      // adjacent high-2-low floor crossing
      
      LPeakDelimiter_L->DrawLine((*peak_iter).PeakLimit_Lower,
				 YAxisMin,
				 (*peak_iter).PeakLimit_Lower,
				 YAxisMax);

      RPeakDelimiter_L->DrawLine((*peak_iter).PeakLimit_Upper,
				 YAxisMin,
				 (*peak_iter).PeakLimit_Upper,
				 YAxisMax);    

      TBox *PeakBox_B = new TBox((*peak_iter).PeakLimit_Lower,
				 YAxisMin,
				 (*peak_iter).PeakLimit_Upper,
				 YAxisMax);

        
      int Color = 8;
      if((*peak_iter).PileupFlag == true)
	Color = 2;
      
      PeakBox_B->SetFillColor(Color);
      PeakBox_B->SetFillStyle(3003);
      PeakBox_B->Draw();
    }

#ifndef MPI_ENABLED    
    // If PSD and PSD plotting are enabled then plot two vertical
    // lines representing the peak location and the tail location
    // (including potential offsets) as well as a lightly shaded box
    // representing the tail integration region
    if(PSDEnable_CB->IsDown() and PSDPlotTailIntegration_CB->IsDown() and PlotPeaksAndGraphics){
      
      if(PSDChannel_CBL->GetComboBox()->GetSelected() ==
	 ChannelSelector_CBL->GetComboBox()->GetSelected()){
	
	int PeakOffset = PSDPeakOffset_NEL->GetEntry()->GetNumber();
	int LowerPos = (*peak_iter).PeakPosX + PeakOffset;

	int TailOffset = PSDTailOffset_NEL->GetEntry()->GetNumber();
	int UpperPos = (*peak_iter).PeakLimit_Upper + TailOffset;
	
	PSDPeakOffset_L->DrawLine(LowerPos, YAxisMin, LowerPos, YAxisMax);
	PSDTailIntegral_B->DrawBox(LowerPos, YAxisMin, UpperPos, YAxisMax);
	PSDTailOffset_L->DrawLine(UpperPos, YAxisMin, UpperPos, YAxisMax);
      }
    }
#endif

  }
  
  // Update the embedded canvas
  if(PlotPeaksAndGraphics)
    Canvas_EC->GetCanvas()->Update(); 
}


void ADAQAnalysisGUI::RejectPileup(TH1F *Histogram_H)
{
  vector<PeakInfoStruct>::iterator it1, it2;

  for(it1=PeakInfoVec.begin(); it1!=PeakInfoVec.end(); it1++){
    
    int PileupCounter = 0;
    vector<bool> PileupRejection(false, PeakLimits.size());
    
    for(it2=PeakInfoVec.begin(); it2!=PeakInfoVec.end(); it2++){
      
      if((*it1).PeakLimit_Lower == (*it2).PeakLimit_Lower){
	PileupRejection[it2 - PeakInfoVec.begin()] = true;
	PileupCounter++;
      }
      else
	PileupRejection[it2 - PeakInfoVec.begin()] = false;
    }
    
    if(PileupCounter != 1)
      (*it1).PileupFlag = true;

  }
}


void ADAQAnalysisGUI::IntegratePeaks()
{
  // Iterate over each peak stored in the vector of PeakInfoStructs...
  vector<PeakInfoStruct>::iterator it;
  for(it=PeakInfoVec.begin(); it!=PeakInfoVec.end(); it++){

    // If pileup rejection is begin used, examine the pileup flag
    // stored in each PeakInfoStruct to determine whether or not this
    // peak is part of a pileup events. If so, skip it...
    //if(UsePileupRejection_CB->IsDown() and (*it).PileupFlag==true)
    if(UsePileupRejection and (*it).PileupFlag==true)
      continue;

    // If the PSD filter is desired, examine the PSD filter flag
    // stored in each PeakInfoStruct to determine whether or not this
    // peak should be filtered out of the spectrum.
    if(UsePSDFilterManager[PSDChannel] and (*it).PSDFilterFlag==true)
      continue;

    // ...and use the lower and upper peak limits to calculate the
    // integral under each waveform peak that has passed all criterion
    double PeakIntegral = Waveform_H[Channel]->Integral((*it).PeakLimit_Lower,
							(*it).PeakLimit_Upper);
    
    // If the user has calibrated the spectrum, then transform the
    // peak integral in pulse units [ADC] to energy units
    if(UseCalibrationManager[Channel])
      PeakIntegral = CalibrationManager[Channel]->Eval(PeakIntegral);
    
    // Add the peak integral to the PAS 
    Spectrum_H->Fill(PeakIntegral);

  }
}


void ADAQAnalysisGUI::FindPeakHeights()
{
  // Iterate over each peak stored in the vector of PeakInfoStructs...
  vector<PeakInfoStruct>::iterator it;
  for(it=PeakInfoVec.begin(); it!=PeakInfoVec.end(); it++){

    // If pileup rejection is begin used, examine the pileup flag
    // stored in each PeakInfoStruct to determine whether or not this
    // peak is part of a pileup events. If so, skip it...
    if(UsePileupRejection and (*it).PileupFlag==true)
      continue;

    // If the PSD filter is desired, examine the PSD filter flag
    // stored in each PeakInfoStruct to determine whether or not this
    // peak should be filtered out of the spectrum.
    if(UsePSDFilterManager[PSDChannel] and (*it).PSDFilterFlag==true)
      continue;
    
    // Initialize the peak height for each peak region
    double PeakHeight = 0.;
    
    // Iterate over the samples between lower and upper integration
    // limits to determine the maximum peak height
    for(int sample=(*it).PeakLimit_Lower; sample<(*it).PeakLimit_Upper; sample++){
      if(Waveform_H[Channel]->GetBinContent(sample) > PeakHeight)
	PeakHeight = Waveform_H[Channel]->GetBinContent(sample);
    }

    // If the user has calibrated the spectrum then transform the peak
    // heights in pulse units [ADC] to energy
    if(UseCalibrationManager[Channel])
      PeakHeight = CalibrationManager[Channel]->Eval(PeakHeight);
    
    // Add the detector pulse peak height to the spectrum histogram
    Spectrum_H->Fill(PeakHeight);
  }
}


void ADAQAnalysisGUI::CreatePSDHistogram()
{
  // Delete a prior existing histogram to eliminate memory leaks
  if(PSDHistogram_H){
    delete PSDHistogram_H;
    PSDHistogramExists = false;
  }
  
  if(SequentialArchitecture){
    // Reset the waveform procesing progress bar
    ProcessingProgress_PB->Reset();
    ProcessingProgress_PB->SetBarColor(ColorManager->Number2Pixel(41));
    ProcessingProgress_PB->SetForegroundColor(ColorManager->Number2Pixel(1));

    // Save ROOT widget values into the class member variables
    ReadoutWidgetValues();
  }

  // Create a 2-dimensional histogram to store the PSD integrals
  PSDHistogram_H = new TH2F("PSDHistogram_H","PSDHistogram_H", 
			    PSDNumTotalBins, PSDMinTotalBin, PSDMaxTotalBin,
			    PSDNumTailBins, PSDMinTailBin, PSDMaxTailBin);
  
  // Delete a prior existing TSpectrum object to account for the user
  // wanted a different number of peaks in the algorithm
  if(PeakFinder) delete PeakFinder;
  PeakFinder = new TSpectrum(MaxPeaks);
  
  // See documention in either ::CreateSpectrum() or
  // ::CreateDesplicedFile for parallel processing assignemnts

  WaveformStart = 0;
  WaveformEnd = PSDWaveformsToDiscriminate;
  
#ifdef MPI_ENABLED

  int SlaveEvents = int(PSDWaveformsToDiscriminate/MPI_Size);
  
  int MasterEvents = int(PSDWaveformsToDiscriminate-SlaveEvents*(MPI_Size-1));
  
  if(ParallelVerbose and IsMaster)
    cout << "\nADAQAnalysisGUI_MPI Node[0]: Waveforms allocated to slaves (node != 0) = " << SlaveEvents << "\n"
	 <<   "                             Waveforms alloced to master (node == 0) =  " << MasterEvents
	 << endl;
  
  // Assign each master/slave a range of the total waveforms to
  // process based on the MPI rank. Note that WaveformEnd holds the
  // waveform_ID that the loop runs up to but does NOT include
  WaveformStart = (MPI_Rank * MasterEvents); // Start (include this waveform in the final histogram)
  WaveformEnd =  (MPI_Rank * MasterEvents) + SlaveEvents; // End (Up to but NOT including this waveform)
  
  if(ParallelVerbose)
    cout << "\nADAQAnalysisGUI_MPI Node[" << MPI_Rank << "] : Handling waveforms " << WaveformStart << " to " << (WaveformEnd-1)
	 << endl;
#endif

  bool PeaksFound = false;
  
  for(int waveform=WaveformStart; waveform<WaveformEnd; waveform++){
    if(SequentialArchitecture)
      gSystem->ProcessEvents();
    
    ADAQWaveformTree->GetEntry(waveform);

    RawVoltage = *WaveformVecPtrs[PSDChannel];
    
    if(RawWaveform or BaselineSubtractedWaveform)
      CalculateBSWaveform(PSDChannel);
    else if(ZeroSuppressionWaveform)
      CalculateZSWaveform(PSDChannel);
    
    if(IntegratePearson)
      IntegratePearsonWaveform(false);
  
    // Find the peaks and peak limits in the current waveform
    PeaksFound = FindPeaks(Waveform_H[PSDChannel], false);
    
    // If not peaks are present in the current waveform then continue
    // onto the next waveform to optimize CPU $.
    if(!PeaksFound)
      continue;
    
    // Calculate the "total" and "tail" integrals of each
    // peak. Because we want to create a PSD histogram, pass "true" to
    // the function to indicate the results should be histogrammed
    CalculatePSDIntegrals(true);
    
    // Update the user with progress
    if(IsMaster)
      if((waveform+1) % int(WaveformEnd*UpdateFreq*1.0/100) == 0)
	UpdateProcessingProgress(waveform);
  }
  
  if(SequentialArchitecture){
    ProcessingProgress_PB->Increment(100);
    ProcessingProgress_PB->SetBarColor(ColorManager->Number2Pixel(32));
    ProcessingProgress_PB->SetForegroundColor(ColorManager->Number2Pixel(0));
    
    if(IntegratePearson)
      DeuteronsInTotal_NEFL->GetEntry()->SetNumber(TotalDeuterons);
  }

#ifdef MPI_ENABLED

  if(ParallelVerbose)
    cout << "\nADAQAnalysisGUI_MPI Node[" << MPI_Rank << "] : Reached the end-of-processing MPI barrier!"
	 << endl;

  MPI::COMM_WORLD.Barrier();

  // The PSDHistogram_H is a 2-dimensional array containing the total
  // (on the "X-axis") and the tail (on the "Y-axis") integrals of
  // each detector pulse. In order to create a single master TH2F
  // object from all the TH2F objects on the nodes, the following
  // procedure is followed:
  //
  // 0. Create a 2-D array of doubles using the X and Y sizes of the
  //    PSDHistogram_H TH2F object
  // 
  // 1. Readout the first column of the PSDHistogram_H object into a
  //    1-D vector (array)
  //
  // 2. Use the SumDoubleArrayToMaster() to sum the nodes's array to a
  //    single array on the master node
  //
  // 3. Fill up the first column of the 2-D array with the reduced
  //    values
  //
  // 3. Repeat #1 for the second column, Repeat...to the Nth column
  //
  // 4. Finally, create a new MasterPSDHistogram_H object on the
  //    master and assign it the values from the 2-D reduced array
  //    (which is essentially a "sum" of all the TH2F objects on the
  //    nodes, just in 2-D array form) and write it to the parallel
  //    processing file
  //
  // Note the total entries in all the nodes PSDHistogram_H objects
  // are aggregated and assigned to the master object, and the
  // deuterons are integrated as well.


  // Create a 2-D array to represent the PSDHistogram_H in array form
  const int ArraySizeX = PSDHistogram_H->GetNbinsX() + 2;
  const int ArraySizeY = PSDHistogram_H->GetNbinsY() + 2;
  double DoubleArray[ArraySizeX][ArraySizeY];

  // Iterate over the PSDHistogram_H columns...
  for(int i=0; i<ArraySizeX; i++){
   
    // A container for the PSDHistogram_H's present column
    vector<double> ColumnVector(ArraySizeY, 0.);

    // Assign the PSDHistogram_H's column to the vector
    for(int j=0; j<ArraySizeY; j++)
      ColumnVector[j] = PSDHistogram_H->GetBinContent(i,j);

    // Reduce the array representing the column
    double *ReturnArray = SumDoubleArrayToMaster(&ColumnVector[0], ArraySizeY);
    
    // Assign the array to the DoubleArray that represents the
    // "master" or total PSDHistogram_H object
    for(int j=0; j<ArraySizeY; j++)
      DoubleArray[i][j] = ReturnArray[j];
  }

  // Aggregated the histogram entries from all nodes to the master
  double Entries = PSDHistogram_H->GetEntries();
  double ReturnDouble = SumDoublesToMaster(Entries);
  
  // Aggregated the total deuterons from all nodes to the master
  TotalDeuterons = SumDoublesToMaster(TotalDeuterons);

  if(IsMaster){
    
    if(ParallelVerbose)
      cout << "\nADAQAnalysisGUI_MPI Node[0] : Writing master PSD TH2F histogram to disk!\n"
	   << endl;

    // Create the master PSDHistogram_H object, i.e. the sum of all
    // the PSDHistogram_H object values from the nodes
    MasterPSDHistogram_H = new TH2F("MasterPSDHistogram_H","MasterPSDHistogram_H",
				    PSDNumTotalBins, PSDMinTotalBin, PSDMaxTotalBin,
				    PSDNumTailBins, PSDMinTailBin, PSDMaxTailBin);
    
    
    // Assign the content from the aggregated 2-D array to the new
    // master histogram
    for(int i=0; i<ArraySizeX; i++)
      for(int j=0; j<ArraySizeY; j++)
	MasterPSDHistogram_H->SetBinContent(i, j, DoubleArray[i][j]);
    MasterPSDHistogram_H->SetEntries(ReturnDouble);
    
    // Open the TFile  and write all the necessary object to it
    ParallelProcessingFile = new TFile(ParallelProcessingFName.c_str(), "update");
    
    MasterPSDHistogram_H->Write("MasterPSDHistogram");

    TVectorD AggregatedDeuterons(1);
    AggregatedDeuterons[0] = TotalDeuterons;
    AggregatedDeuterons.Write("AggregatedDeuterons");

    ParallelProcessingFile->Write();
  }
#endif
 
  // Update the bool to alert the code that a valid PSDHistogram_H object exists.
  PSDHistogramExists = true;
}


// Method to calculate the "tail" and "total" integrals of each of the
// peaks located by the peak finding algorithm and stored in the class
// member vector of PeakInfoStruct. Because calculating these
// integrals are necessary for pulse shape discrimiation, this method
// also (if specified by the user) applies a PSD filter to the
// waveforms. Thus, this function is called anytime PSD filtering is
// required (spectra creation, desplicing, etc), which may or may not
// require histogramming the result of the PSD integrals. Hence, the
// function argment boolean to provide flexibility
void ADAQAnalysisGUI::CalculatePSDIntegrals(bool FillPSDHistogram)
{
  // Iterate over each peak stored in the vector of PeakInfoStructs...
  vector<PeakInfoStruct>::iterator it;
  for(it=PeakInfoVec.begin(); it!=PeakInfoVec.end(); it++){
    
    // Assign the integration limits for the tail and total
    // integrals. Note that peak limit and upper limit have the
    // user-specified included to provide flexible integration
    double LowerLimit = (*it).PeakLimit_Lower; // Left-most side of peak
    double Peak = (*it).PeakPosX + PSDPeakOffset; // The peak location + offset
    double UpperLimit = (*it).PeakLimit_Upper + PSDTailOffset; // Right-most side of peak + offset
    
    // Compute the total integral (lower to upper limit)
    double TotalIntegral = Waveform_H[PSDChannel]->Integral(LowerLimit, UpperLimit);
    
    // Compute the tail integral (peak to upper limit)
    double TailIntegral = Waveform_H[PSDChannel]->Integral(Peak, UpperLimit);
    
    if(Verbose)
      cout << "PSD tail integral = " << TailIntegral << " (ADC)\n"
	   << "PSD total integral = " << TotalIntegral << " (ADC)\n"
	   << endl;
    
    // If the user has enabled a PSD filter ...
    if(UsePSDFilterManager[PSDChannel])
      
      // ... then apply the PSD filter to the waveform. If the
      // waveform does not pass the filter, mark the flag indicating
      // that it should be filtered out due to its pulse shap
      if(ApplyPSDFilter(TailIntegral, TotalIntegral))
	(*it).PSDFilterFlag = true;
    
    // The total integral of the waveform must exceed the PSDThreshold
    // in order to be histogrammed. This allows the user flexibility
    // in eliminating the large numbers of small waveform events.
    if((TotalIntegral > PSDThreshold) and FillPSDHistogram){
      
      if((*it).PSDFilterFlag == false)
	PSDHistogram_H->Fill(TotalIntegral, TailIntegral);
    }
  }
}


// Analyze the tail and total PSD integrals to determine whether the
// waveform should be filtered out depending on its pulse shape. The
// comparison is made by determining if the value of tail integral
// falls above/below the interpolated value of the total integral via
// a TGraph created by the user (depending on "positive" or "negative"
// filter polarity as selected by the user.). The following convention
// - uniform throughout the code - is used for the return value:true =
// waveform should be filtered; false = waveform should not be
// filtered
bool ADAQAnalysisGUI::ApplyPSDFilter(double TailIntegral, double TotalIntegral)
{
  // The PSDFilter uses a TGraph created by the user in order to
  // filter events out of the PSDHistogram_H. The events to be
  // filtered must fall either above ("positive filter") or below
  // ("negative filter) the line defined by the TGraph object. The
  // tail integral of the pulse is compared to an interpolated
  // value (using the total integral) to decide whether to filter.


  // Waveform passed the criterion for a positive PSD filter (point in
  // tail/total PSD integral space fell "above" the TGraph; therefore,
  // it should not be filtered so return false
  if(PSDFilterPolarity > 0 and TailIntegral >= PSDFilterManager[PSDChannel]->Eval(TotalIntegral))
    return false;

  // Waveform passed the criterion for a negative PSD filter (point in
  // tail/total PSD integral space fell "below" the TGraph; therefore,
  // it should not be filtered so return false
  else if(PSDFilterPolarity < 0 and TailIntegral <= PSDFilterManager[PSDChannel]->Eval(TotalIntegral))
    return false;

  // Waveform did not pass the PSD filter tests; therefore it should
  // be filtered so return true
  else
    return true;
}


// Method used to aggregate arrays of doubles on each node to a single
// array on the MPI master node (master == node 0). A pointer to the
// array on each node (*SlaveArray) as well as the size of the array
// (ArraySize) must be passed as function arguments from each node.
double *ADAQAnalysisGUI::SumDoubleArrayToMaster(double *SlaveArray, size_t ArraySize)
{
  double *MasterSum = new double[ArraySize];
#ifdef MPI_ENABLED
  MPI::COMM_WORLD.Reduce(SlaveArray, MasterSum, ArraySize, MPI::DOUBLE, MPI::SUM, 0);
#endif
  return MasterSum;
}


// Method used to aggregate doubles on each node to a single double on
// the MPI master node (master == node 0).
double ADAQAnalysisGUI::SumDoublesToMaster(double SlaveDouble)
{
  double MasterSum = 0;
#ifdef MPI_ENABLED
  MPI::COMM_WORLD.Reduce(&SlaveDouble, &MasterSum, 1, MPI::DOUBLE, MPI::SUM, 0);
#endif
  return MasterSum;
}


// Method to compute the instantaneous (within the RFQ deuteron pulse)
// and time-averaged (in real time) detector count rate. Note that the
// count rate can only be calculated using sequential
// architecture. Because only a few thousand waveforms needs to be
// processed to compute the count rates, it is not presently foreseen
// that this feature will be implemented in parallel. Note also that
// count rates can only be calculated for RFQ triggered acquisition
// windows with a known pulse width and rep. rate.
void ADAQAnalysisGUI::CalculateCountRate()
{
  if(!ADAQRootFileLoaded)
    return;
  
  // Readout the ROOT widget values to class member data
  if(!ParallelArchitecture)
    ReadoutWidgetValues();
  
  // If the TSPectrum PeakFinder object exists then delete it and
  // recreate it to account for changes to the MaxPeaks variable
  if(PeakFinder) delete PeakFinder;
  PeakFinder = new TSpectrum(MaxPeaks);
  
  // Reset the total number of identified peaks to zero
  TotalPeaks = 0;
  
  // Get the number of waveforms to process in the calculation
  int NumWaveforms = CountRateWaveforms_NEL->GetEntry()->GetIntNumber();

  for(int waveform = 0; waveform<NumWaveforms; waveform++){
    // Process in a separate thread to allow the user to retain
    // control of the ROOT widgets
    gSystem->ProcessEvents();

    // Get a waveform
    ADAQWaveformTree->GetEntry(waveform);

    // Get the raw waveform voltage
    RawVoltage = *WaveformVecPtrs[Channel];

    // Calculate either a baseline subtracted waveform or a zero
    // suppression waveform
    if(RawWaveform or BaselineSubtractedWaveform)
      CalculateBSWaveform(Channel);
    else if(ZeroSuppressionWaveform)
      CalculateZSWaveform(Channel);
  
    // Find the peaks in the waveform. Note that this function is
    // responsible for incrementing the 'TotalPeaks' member data
    FindPeaks(Waveform_H[Channel], false);

    // Update the waveform processing progress bar
    if((waveform+1) % int(WaveformEnd*UpdateFreq*1.0/100) == 0)
      UpdateProcessingProgress(waveform);
  }
  
  // At this point, the 'TotalPeaks' variable contains the total
  // number of identified peaks after processing the number of
  // waveforms specified by the 'NumWaveforms' variable

  // Compute the time (in seconds) that the RFQ beam was 'on'
  double TotalTime = RFQPulseWidth_NEL->GetEntry()->GetNumber() * us2s * NumWaveforms;

  // Compute the instantaneous count rate, i.e. the detector count
  // rate only when the RFQ beam is on
  double InstCountRate = TotalPeaks * 1.0 / TotalTime;
  InstCountRate_NEFL->GetEntry()->SetNumber(InstCountRate);
  
  // Compute the average count rate, i.e. the detector count rate in
  // real time, which requires a knowledge of the RFQ rep rate.
  double AvgCountRate = InstCountRate * (RFQPulseWidth_NEL->GetEntry()->GetNumber() * us2s * RFQRepRate_NEL->GetEntry()->GetNumber());
  AvgCountRate_NEFL->GetEntry()->SetNumber(AvgCountRate);
}


// Method to compute a background of a TH1F object representing a
// detector pulse height / energy spectrum.
void ADAQAnalysisGUI::CalculateSpectrumBackground(TH1F *Spectrum_H)
{
  // raw = original spectrum for which background is calculated 
  // background = background component of raw spectrum
  // deconvolved = raw spectrum less background spectrum

  // Clone the Spectrum_H object and give the clone a unique name
  TH1F *SpectrumClone_H = (TH1F *)Spectrum_H->Clone("SpectrumClone_H");
  
  // Set the range of the Spectrum_H clone to correspond to the range
  // that the user has specified to calculate the background over
  SpectrumClone_H->GetXaxis()->SetRangeUser(SpectrumRangeMin_NEL->GetEntry()->GetIntNumber(),
					    SpectrumRangeMax_NEL->GetEntry()->GetIntNumber());

  // Delete and recreate the TSpectrum peak finder object
  if(PeakFinder) delete PeakFinder;
  PeakFinder = new TSpectrum(5);
  
  // ZSH: This widget has been disabled for lack of use. Presently, I
  //       have hardcoded a rather unimportant (Ha! This will come
  //       back to bite you in the ass, I'm sure of it) "5" above in
  //       place since it doesn't affect the backgroud calculation.
  //
  // PeakFinder = new TSpectrum(SpectrumNumPeaks_NEL->GetEntry()->GetIntNumber());

  // Delete the TH1F object that holds a previous background histogram
  if(SpectrumBackground_H) delete SpectrumBackground_H;

  // Use the TSpectrum::Background() method to compute the spectrum
  // background; Set the resulting TH1F objects graphic attributes
  SpectrumBackground_H = (TH1F *)PeakFinder->Background(SpectrumClone_H, 15, "goff");
  SpectrumBackground_H->SetLineColor(2);
  SpectrumBackground_H->SetLineWidth(2);

  // ZSH: The TSpectrum::Background() method sets the TH1F::Entries
  // class member of the returned TH1F background object to the number
  // of bins of the TH1F object for which the background is calculated
  // NOT the actual integral of the resulting background histogram
  // (including under/over flow bins). See TSpectrum.cxx Line 241 in
  // the ROOT source code. Unclear if this is a bug or if this is done
  // for statistical validity reasons. May post in ROOTTalk forum.
  //
  // Set the TH1F::Entries variable to equal the full integral of the
  // background TH1F object
  SpectrumBackground_H->SetEntries(SpectrumBackground_H->Integral(0, SpectrumNumBins+1));

  // Delete the TH1F object that holds a previous deconvolved TH1F
  if(SpectrumDeconvolved_H)
    delete SpectrumDeconvolved_H;

  // Create a new deconvolved spectrum. Note that the bin number and
  // range match those of the raw spectrum, whereas the background
  // spectrum range is set by the user. This enables a background
  // spectrum to be computed for whatever range the user desires and
  // for resulting deconvolved spectrum to have the entire original
  // raw spectrum with the background subtracted out.
  SpectrumDeconvolved_H = new TH1F("Deconvolved spectrum", "Deconvolved spectrum", 
				   SpectrumNumBins, SpectrumMinBin, SpectrumMaxBin);
  SpectrumDeconvolved_H->SetLineColor(4);
  SpectrumDeconvolved_H->SetLineWidth(2);

  // Subtract the background spectrum from the raw spectrum
  SpectrumDeconvolved_H->Add(SpectrumClone_H, SpectrumBackground_H, 1.0, -1.0);

  // ZSH: The final TH1 statistics from two combined TH1's depend on
  // the the operation: "adding" two histograms preserves statistics,
  // but "subtracting" two histograms cannot preserve statistics in
  // order to prevent violating statistical rules (See TH1.cxx, Lines
  // 1062-1081) such as negative variances. Therefore, the
  // TH1::Entries data member is NOT equal to the full integral of the
  // TH1 object (including under/over flow bins); however, it is
  // highly convenient for the TH1::Entries variable to **actually
  // reflect the total content of the histogram**, either for viewing
  // on the TH1 stats box or comparing to the calculated integral. 
  //
  // Set the TH1::Entries to equal the full integral of the TH1 object
  SpectrumDeconvolved_H->SetEntries(SpectrumDeconvolved_H->Integral(SpectrumMinBin,SpectrumMaxBin+1));
}


// Method used to find peaks in a pulse / energy spectrum
void ADAQAnalysisGUI::FindSpectrumPeaks()
{
  PlotSpectrum();
  
  int SpectrumNumPeaks = SpectrumNumPeaks_NEL->GetEntry()->GetIntNumber();
  int SpectrumSigma = SpectrumSigma_NEL->GetEntry()->GetIntNumber();
  int SpectrumResolution = SpectrumResolution_NEL->GetEntry()->GetNumber();
  
  TSpectrum *SpectrumPeakFinder = new TSpectrum(SpectrumNumPeaks);

  int SpectrumPeaks = SpectrumPeakFinder->Search(Spectrum_H, SpectrumSigma, "goff", SpectrumResolution);
  float *PeakPosX = SpectrumPeakFinder->GetPositionX();
  float *PeakPosY = SpectrumPeakFinder->GetPositionY();
  
  for(int peak=0; peak<SpectrumPeaks; peak++){
    TMarker *PeakMarker = new TMarker(PeakPosX[peak],
				      PeakPosY[peak]+50,
				      peak);
    PeakMarker->SetMarkerColor(2);
    PeakMarker->SetMarkerStyle(23);
    PeakMarker->SetMarkerSize(2.0);
    PeakMarker->Draw();
  }
  Canvas_EC->GetCanvas()->Update();
}


// Method used to integrate a pulse / energy spectrum
void ADAQAnalysisGUI::IntegrateSpectrum()
{
  // Get the min/max values from the double slider widget
  float Min, Max;
  SpectrumIntegrationLimits_DHS->GetPosition(Min, Max);
  
  // Set the X axis position of vertical lines representing the
  // lower/upper integration limits
  double LimitLineXMin = (Min * XAxisMax) + XAxisMin;
  double LimitLineXMax = Max * XAxisMax;
  
  // Check to ensure that the lower limit line is always LESS than the
  // upper limit line
  if(LimitLineXMax < LimitLineXMin)
    LimitLineXMax = LimitLineXMin+1;
  
  // Draw the lines reprenting lower/upper integration limits
  LowerLimit_L->DrawLine(LimitLineXMin, YAxisMin, LimitLineXMin, YAxisMax);
  UpperLimit_L->DrawLine(LimitLineXMax, YAxisMin, LimitLineXMax, YAxisMax);

  // Clone the appropriate spectrum object depending on user's
  // selection into a new TH1F object for integration
  TH1F *Spectrum2Integrate_H = 0;
  if(SpectrumLessBackground_RB->IsDown())
    Spectrum2Integrate_H = (TH1F *)SpectrumDeconvolved_H->Clone("SpectrumToIntegrate_H");
  else
    Spectrum2Integrate_H = (TH1F *)Spectrum_H->Clone("SpectrumToIntegrate_H");

  // Set the integration TH1F object attributes and draw it
  Spectrum2Integrate_H->SetLineColor(4);
  Spectrum2Integrate_H->SetLineWidth(2);
  Spectrum2Integrate_H->SetFillColor(2);
  Spectrum2Integrate_H->SetFillStyle(3001);
  Spectrum2Integrate_H->GetXaxis()->SetRangeUser(LimitLineXMin, LimitLineXMax);
  Spectrum2Integrate_H->Draw("B SAME");
  Spectrum2Integrate_H->Draw("C SAME");

  // Variable to hold the result of the spectrum integral
  double Integral = 0.;
  
  // Variable and pointer used to hold the result of the error in the
  // spectrum integral calculation. Note that the integration error is
  // simply the square root of the sum of the squares of the
  // integration spectrum bin contents. The error is computed with a
  // ROOT TH1 method for robustness.
  double Error = 0.;
  double *ErrorPTR = &Error;

  string IntegralArg = "width";
  if(SpectrumIntegralInCounts_CB->IsDown())
    IntegralArg.assign("");

  ///////////////////////////////
  // Gaussian fitting/integration

  if(SpectrumUseGaussianFit_CB->IsDown()){
    // Create a gaussian fit between the lower/upper limits; fit the
    // gaussian to the histogram and store the result of the fit in a
    // new TH1F object for analysis
    TF1 *PeakFit = new TF1("PeakFit", "gaus", LimitLineXMin, LimitLineXMax);
    Spectrum2Integrate_H->Fit("PeakFit","R N");
    TH1F *PeakFit_H = (TH1F *)PeakFit->GetHistogram();
    
    // Get the bin width. Note that bin width is constant for these
    // histograms so getting the zeroth bin is acceptable
    GaussianBinWidth = PeakFit_H->GetBinWidth(0);
    
    // Compute the integral and error between the lower/upper limits
    // of the histogram that resulted from the gaussian fit
    Integral = PeakFit_H->IntegralAndError(PeakFit_H->FindBin(LimitLineXMin),
					   PeakFit_H->FindBin(LimitLineXMax),
					   *ErrorPTR,
					   IntegralArg.c_str());

    if(SpectrumIntegralInCounts_CB->IsDown()){
      Integral *= (GaussianBinWidth/CountsBinWidth);
      Error *= (GaussianBinWidth/CountsBinWidth);
    }
    
    // Draw the gaussian peak fit
    PeakFit->SetLineColor(30);
    PeakFit->SetLineWidth(3);
    PeakFit->Draw("SAME");

  }

  ////////////////////////
  // Histogram integration

  else{
    // Set the low and upper bin for the integration
    int StartBin = Spectrum2Integrate_H->FindBin(LimitLineXMin);
    int StopBin = Spectrum2Integrate_H->FindBin(LimitLineXMax);

    // Compute the integral and error
    Integral = Spectrum2Integrate_H->IntegralAndError(StartBin,
						      StopBin,
						      *ErrorPTR,
						      IntegralArg.c_str());

    // Get the bin width of the histogram
    CountsBinWidth = Spectrum2Integrate_H->GetBinWidth(0);
  }
  
  // The spectrum integral and error may be normalized to the total
  // computed RFQ current if desired by the user
  if(SpectrumNormalizePeakToCurrent_CB->IsDown()){
    if(TotalDeuterons == 0){
      CreateMessageBox("Warning! The total RFQ current has not been computed! TotalDeuterons set to 1.0!\n","Asterisk");
      TotalDeuterons = 1.0;
    }
    else{
      Integral /= TotalDeuterons;
      Error /= TotalDeuterons;
    }
  }
  
  // Update the number entry widgets with the integration results
  SpectrumIntegral_NEFL->GetEntry()->SetNumber(Integral);
  SpectrumIntegralError_NEFL->GetEntry()->SetNumber(Error);
  
  Canvas_EC->GetCanvas()->Update();
}


// Creates a separate pop-up box with a message for the user. Function
// is modular to allow flexibility in use.
void ADAQAnalysisGUI::CreateMessageBox(string Message, string IconName)
{
  // Choose either a "stop" icon or an "asterisk" icon
  EMsgBoxIcon IconType = kMBIconStop;
  if(IconName == "Asterisk")
    IconType = kMBIconAsterisk;
  
  const int NumTitles = 6;

  string BoxTitlesAsterisk[] = {"ADAQAnalysisGUI says 'good job!", 
				"Oh, so you are competent!",
				"This is a triumph of science!",
				"Excellent work. You're practically a PhD now.",
				"For you ARE the Kwisatz Haderach!",
				"There will be a parade in your honor."};
  
  string BoxTitlesStop[] = {"ADAQAnalysisGUI is disappointed in you...", 
			    "Seriously? I'd like another operator, please.",
			    "Unacceptable. Just totally unacceptable.",
			    "That was about as successful as the Hindenburg...",
			    "You blew it!",
			    "Abominable! Off with you head!" };

  // Surprise the user!
  int RndmInt = RNG->Integer(NumTitles);
  string Title = BoxTitlesStop[RndmInt];
  if(IconType==kMBIconAsterisk)
    Title = BoxTitlesAsterisk[RndmInt];

  // Create the message box with the desired message and icon
  new TGMsgBox(gClient->GetRoot(), this, Title.c_str(), Message.c_str(), IconType, kMBOk);
}


// Method to integrate the Pearson waveform (which measures the RFQ
// beam current) in order to calculate the number of deuterons
// delivered during a single waveform. The deuterons/waveform are
// aggregated into a class member that stores total deuterons for all
// waveforms processed.
void ADAQAnalysisGUI::IntegratePearsonWaveform(bool PlotPearsonIntegration)
{
  // The total deutorons delivered for all waveforms in the presently
  // loaed ADAQ-formatted ROOT file may be stored in the ROOT file
  // itself if that ROOT file was created during parallel processing.
  // If this is the case then return from this function since we
  // already have the number of primary interest.
  if(ADAQParResultsLoaded)
    return;

  // Get the V1720/data channel containing the output of the Pearson
  int Channel = PearsonChannel;
  
  // Compute the baseline subtracted Pearson waveform
  CalculateBSWaveform(PearsonChannel, true);
  
  // There are two integration options for the Pearson waveform:
  // simply integrating the waveform from a lower limit to an upper
  // limit (in sample time), "raw integration"; fitting two 1st order
  // functions to the current trace and integrating (and summing)
  // their area, "fit integration". The firsbt option is the most CPU
  // efficient but requires a very clean RFQ current traces: the
  // second options is more CPU expensive but necessary for very noise
  // Pearson waveforms;
  
  if(IntegrateRawPearson){
    
    // Integrate the current waveform between the lower/upper limits. 
    double Integral = Waveform_H[Channel]->Integral(Waveform_H[Channel]->FindBin(PearsonLowerLimit),
						    Waveform_H[Channel]->FindBin(PearsonUpperLimit));
    
    // Compute the total number of deuterons in this waveform by
    // converting the result of the integral from units of V / ADC
    // with the appropriate factors.
    double Deuterons = Integral * adc2volts_V1720 * sample2seconds_V1720;
    Deuterons *= (volts2amps_Pearson / amplification_Pearson / electron_charge);
    
    if(Deuterons > 0)
      TotalDeuterons += Deuterons;
	
    if(Verbose)
      cout << "Total number of deuterons: " << "\t" << TotalDeuterons << endl;
    
    // Options to plot the results of the integration on the canvas
    // for the user's inspection purposess
    if(PlotPearsonIntegration){

      DeuteronsInWaveform_NEFL->GetEntry()->SetNumber(Deuterons);
      DeuteronsInTotal_NEFL->GetEntry()->SetNumber(TotalDeuterons);
      
      TH1F *Pearson2Integrate_H = (TH1F *)Waveform_H[Channel]->Clone("Pearson2Integrate_H");

      // Plot the integration region of the histogram waveform
      Pearson2Integrate_H->SetFillColor(4);
      Pearson2Integrate_H->SetLineColor(2);
      Pearson2Integrate_H->SetFillStyle(3001);
      Pearson2Integrate_H->GetXaxis()->SetRangeUser(PearsonLowerLimit, PearsonUpperLimit);
      Pearson2Integrate_H->Draw("B SAME");

      // Overplot the line representing raw RFQ current waveform
      Waveform_H[Channel]->SetLineColor(2);
      Waveform_H[Channel]->SetLineWidth(2);
      Waveform_H[Channel]->Draw("C SAME");      
      
      Canvas_EC->GetCanvas()->Update();
    }
  }
  else if(IntegrateFitToPearson){
    // Create a TF1 object for the initial rise region of the current
    // trace; place the result into a TH1F for potential plotting
    TF1 *RiseFit = new TF1("RiseFit", "pol1", PearsonLowerLimit, PearsonMiddleLimit);
    Waveform_H[Channel]->Fit("RiseFit","R N Q C");
    TH1F *RiseFit_H = (TH1F *)RiseFit->GetHistogram();

    // Create a TF1 object for long "flat top" region of the current
    // trace a TH1F for potential plotting
    TF1 *PlateauFit = new TF1("PlateauFit", "pol1", PearsonMiddleLimit, PearsonUpperLimit);
    Waveform_H[Channel]->Fit("PlateauFit","R N Q C");
    TH1F *PlateauFit_H = (TH1F *)PlateauFit->GetHistogram();

    // Compute the integrals. Note the "width" argument is passed to
    // the integration to specify that the histogram results should be
    // multiplied by the bin width.
    double Integral = RiseFit_H->Integral(RiseFit_H->FindBin(PearsonLowerLimit), 
					  RiseFit_H->FindBin(PearsonUpperLimit),
					  "width");
    
    Integral += PlateauFit_H->Integral(PlateauFit_H->FindBin(PearsonMiddleLimit),
				       PlateauFit_H->FindBin(PearsonUpperLimit),
				       "width");

        
    // Compute the total number of deuterons in this waveform by
    // converting the result of the integrals from units of V / ADC
    // with the appropriate factors.
    double Deuterons = Integral * adc2volts_V1720 * sample2seconds_V1720;
    Deuterons *= (volts2amps_Pearson / amplification_Pearson / electron_charge);

    if(Deuterons > 0)
      TotalDeuterons += Deuterons;

    if(Verbose)
      cout << "Total number of deuterons: " << "\t" << TotalDeuterons << endl;
    
    if(PlotPearsonIntegration){
      
      DeuteronsInWaveform_NEFL->GetEntry()->SetNumber(Deuterons);
      DeuteronsInTotal_NEFL->GetEntry()->SetNumber(TotalDeuterons);
	
      Waveform_H[Channel]->SetLineColor(2);
      Waveform_H[Channel]->SetLineWidth(2);
      Waveform_H[Channel]->Draw("C SAME");
      
      RiseFit_H->SetLineColor(8);
      RiseFit_H->SetFillColor(8);
      RiseFit_H->SetFillStyle(3001);
      RiseFit_H->Draw("SAME");

      PlateauFit_H->SetLineColor(4);
      PlateauFit_H->SetFillColor(4);
      PlateauFit_H->SetFillStyle(3001);
      PlateauFit_H->Draw("SAME");
      
      Canvas_EC->GetCanvas()->Update();
    }

    // Delete the TF1 objects to prevent bleeding memory
    delete RiseFit;
    delete PlateauFit;
  }
}


void ADAQAnalysisGUI::ProcessWaveformsInParallel(string ProcessingType)
{
  /////////////////////////////////////
  // Prepare for parallel processing //
  /////////////////////////////////////

  // The following command (with the exception of the single command
  // that launches the MPI parallel processing session) are all
  // executed by the the sequential binary. In order to "transfer" the
  // values required for processing (ROOT widget settings, calibration
  // manager, etc) from the sequential binary to the parallel binaries
  // (or nodes), a transient ROOT TFile is created in /tmp that acts
  // as an exchange point between seq. and par. binaries. This TFile
  // is also used to "tranfser" results created in parallel back to
  // the sequential binary for further viewing, analysis, etc.
  
  if(ParallelVerbose)
    cout << "\n\n"
	 << "/////////////////////////////////////////////////////\n"
	 << "//   Beginning parallel processing of waveforms!   //\n"
	 << "//"
	 << std::setw(15) << "     --> Mode: "
	 << std::setw(34) << std::left << ProcessingType
	 << "//\n"
	 << "/////////////////////////////////////////////////////\n"
	 << endl;
  

  /////////////////////////////
  // Assign necessary variables

  // Save the values necessary for parallel processing to the ROOT
  // TFile from the sequential binary. These values include the
  // sequential binary's ROOT widget values as well as file and
  // calibration variables. The values will be read into each node at
  // the beginning of each parallel processing session.
  SaveParallelProcessingData();
  
  // Set the number of processors to use in parallel
  int NumProcessors = NumProcessors_NEL->GetEntry()->GetIntNumber();
  
  // Create a shell command to launch the parallel binary of
  // ADAQAnalysisGUI with the desired number of nodes
  stringstream ss;
  ss << "mpirun -np " << NumProcessors << " " << ParallelBinaryName << " " << ProcessingType;
  string ParallelCommand = ss.str();
  
  if(Verbose)
    cout << "Initializing MPI slaves for processing!\n" 
	 << endl;


  //////////////////////////////////////
  // Processing waveforms in parallel //
  //////////////////////////////////////
  system(ParallelCommand.c_str());
  
  if(Verbose)
    cout << "Parallel processing has concluded successfully!\n" 
	 << endl;

  
  ///////////////////////////////////////////
  // Absorb results into sequential binary //
  ///////////////////////////////////////////
  
  // Open the parallel processing ROOT file to retrieve the results
  // produced by the parallel processing session
  ParallelProcessingFile = new TFile(ParallelProcessingFName.c_str(), "read");  
  
  if(ParallelProcessingFile->IsOpen()){
    
    ////////////////
    // Histogramming

    if(ProcessingType == "histogramming"){
      // Obtain the "master" histogram, which is a TH1F object
      // contains the result of MPI reducing all the TH1F objects
      // calculated by the nodes in parallel. Set the master histogram
      // to the Spectrum_H class data member and plot it.
      Spectrum_H = (TH1F *)ParallelProcessingFile->Get("MasterHistogram");
      SpectrumExists = true;
      PlotSpectrum();
      
      if(SpectrumOverplotDerivative_CB->IsDown())
	PlotSpectrumDerivative();

      // Obtain the total number of deuterons integrated during
      // histogram creation and update the ROOT widget
      TVectorD *AggregatedDeuterons = (TVectorD *)ParallelProcessingFile->Get("AggregatedDeuterons");
      TotalDeuterons = (*AggregatedDeuterons)[0];
      DeuteronsInTotal_NEFL->GetEntry()->SetNumber(TotalDeuterons);
    }
    

    /////////////
    // Desplicing

    else if(ProcessingType == "desplicing"){
    }	  
    

    /////////////////////////////
    // Pulse shape discriminating 
    
    else if(ProcessingType == "discriminating"){
      PSDHistogram_H = (TH2F *)ParallelProcessingFile->Get("MasterPSDHistogram");
      PSDHistogramExists = true;
      PlotPSDHistogram();
      
      TVectorD *AggregatedDeuterons = (TVectorD *)ParallelProcessingFile->Get("AggregatedDeuterons");
      TotalDeuterons = (*AggregatedDeuterons)[0];
      DeuteronsInTotal_NEFL->GetEntry()->SetNumber(TotalDeuterons);
    }
  }
  else
    CreateMessageBox("Failed to open the parallel processing ROOT file! Aborting spectra retrieval and plotting","Stop");
  
  
  /////////////
  // Cleanup //
  /////////////

  if(Verbose)
    cout << "Removing depracated parallel processing files!\n"
	 << endl;
  
  // Remove the parallel processing ROOT file since it is no longer
  // needed now that we have extracted all the results produced by the
  // parallel processing session and stored them in the seq. binary
  
  string RemoveFilesCommand;
  RemoveFilesCommand = "rm " + ParallelProcessingFName + " -f";
  system(RemoveFilesCommand.c_str()); 
  
  // Return to the ROOT TFile containing the waveforms
  ADAQRootFile->cd();
}


void ADAQAnalysisGUI::CreateDesplicedFile()
{
  ////////////////////////////
  // Prepare for processing //
  ////////////////////////////
  
  // Reset the progres bar if binary is sequential architecture

  if(SequentialArchitecture){
    ProcessingProgress_PB->Reset();
    ProcessingProgress_PB->SetBarColor(ColorManager->Number2Pixel(41));
    ProcessingProgress_PB->SetForegroundColor(ColorManager->Number2Pixel(1));
  }
  
  // If in sequential arch then readout the necessary processing
  // parameters from the ROOT widgets into class data members
  if(SequentialArchitecture)
    ReadoutWidgetValues();
  
  // Delete the previoue TSpectrum PeakFinder object if it exists to
  // prevent memory leaks
  if(PeakFinder)
    delete PeakFinder;
  
  // Create a TSpectrum PeakFinder using the appropriate widget to set
  // the maximum number of peaks that can be found
  PeakFinder = new TSpectrum(MaxPeaks);
  
  // Variable to aggregate the integrated RFQ current. Presently this
  // feature is NOT implemented. 
  TotalDeuterons = 0.;

  ////////////////////////////////////
  // Assign waveform processing ranges
  
  // Set the range of waveforms that will be processed. Note that in
  // sequential architecture if N waveforms are to be histogrammed,
  // waveforms from waveform_ID == 0 to waveform_ID ==
  // (WaveformsToDesplice-1) will be included in the despliced file
  WaveformStart = 0; // Start (and include this waveform in despliced file)
  WaveformEnd = WaveformsToDesplice; // End (Up to but NOT including this waveform)
  
#ifdef MPI_ENABLED
  
  // If the waveform processing is to be done in parallel then
  // distribute the events as evenly as possible between the master
  // (rank == 0) and the slaves (rank != 0) to maximize computational
  // efficiency. Note that the master will carry the small leftover
  // waveforms from the even division of labor to the slaves.
  
  // Assign the number of waveforms processed by each slave
  int SlaveEvents = int(WaveformsToDesplice/MPI_Size);
  
  // Assign the number of waveforms processed by the master
  int MasterEvents = int(WaveformsToDesplice-SlaveEvents*(MPI_Size-1));
  
  if(ParallelVerbose and IsMaster)
    cout << "\nADAQAnalysisGUI_MPI Node[0] : Waveforms allocated to slaves (node != 0) = " << SlaveEvents << "\n"
	 <<   "                              Waveforms alloced to master (node == 0) =  " << MasterEvents
	 << endl;
  
  // Assign each master/slave a range of the total waveforms to be
  // process based on each nodes MPI rank
  WaveformStart = (MPI_Rank * MasterEvents); // Start (and include this waveform in despliced file)
  WaveformEnd =  (MPI_Rank * MasterEvents) + SlaveEvents; // End (up to but NOT including this waveform)
  
  if(ParallelVerbose)
    cout << "\nADAQAnalysisGUI_MPI Node[" << MPI_Rank << "] : Handling waveforms " << WaveformStart << " to " << (WaveformEnd-1)
	 << endl;
#endif


  /////////////////////////////////////////////////
  // Create required objects for new despliced file

  // Create a new TFile for the despliced waveforms and associated
  // objects. In sequential arch, the TFile is created directly with
  // the name and location specified by the user in the desplicing
  // widgets; in parallel arch, a series of TFiles (one for each node)
  // is first created in the /tmp directory with the suffix
  // ".node<MPI_Rank>". After the desplicing algorithm is complete,
  // the master node is responsible for aggregating the series of
  // paralle TFiles into a single TFile with the name and location
  // specified by the user in the desplicing widgets.

  // The ROOT TFile
  TFile *F;

  // Sequential architecture
  if(SequentialArchitecture)
    F = new TFile(DesplicedFileName.c_str(), "recreate");

  // Parallel architecture
  else{
    stringstream ss;
    ss << "/tmp/DesplicedWaveforms.root" << ".node" << MPI_Rank;
    string DesplicedFileNameTMP = ss.str();
    F = new TFile(DesplicedFileNameTMP.c_str(), "recreate");
  }

  // Create a new TTree to hold the despliced waveforms. It is
  // important that the TTree is named "WaveformTree" (as in the
  // original ADAQ ROOT file) such that ADAQAnalysisGUI can correctly
  // import the TTree from the TFile for analysis
  TTree *T = new TTree("WaveformTree", "A TTree to store despliced waveforms");

  // Create the vector object that holds the data channel voltages
  // and will be assigned to the TTree branches
  vector< vector<int> > VoltageInADC_AllChannels;
  VoltageInADC_AllChannels.resize(NumDataChannels);

  // Assign the data channel voltage vector object to the TTree
  // branches with the correct branch names
  stringstream ss1;
  for(int channel=0; channel<NumDataChannels; channel++){
    ss1 << "VoltageInADC_Ch" << channel;
    string BranchName = ss1.str();
    ss1.str("");
    
    T->Branch(BranchName.c_str(),
	      &VoltageInADC_AllChannels.at(channel));
  }

  // Create a new ADAQRootMeasParams object based on the original. The
  // RecordLength data member is modified to accomodate the new size
  // of the despliced waveforms (much much shorter than the original
  // waveforms in most cases...). Note that the "dynamic" TTree stores
  // despliced waveforms of arbitrary and different lengths (i.e. the
  // despliced waveform sample length is NOT fixed as it is in data
  // acquisition with ADAQRootGUI); therefore, the RecordLength
  // associated with the despliced waveform TFile must be sufficiently
  // long to accomodate the longest despliced waveform. This new
  // RecordLength value is mainly updated to allow viewing of the
  // waveform by ADAQAnalysisGUI during future analysis sessions.
  ADAQRootMeasParams *MP = ADAQMeasParams;
  MP->RecordLength = DesplicedWaveformLength;
  
  // Create a new TObjString object representing the measurement
  // commend. This feature is currently unimplemented.
  TObjString *MC = new TObjString("");

  // Create a new class that will store the results calculated in
  // parallel persistently in the new despliced ROOT file
  ADAQAnalysisParallelResults *PR = new ADAQAnalysisParallelResults;
  
  // When the new despliced TFile object(s) that will RECEIVE the
  // despliced waveforms was(were) created above, that(those) TFile(s)
  // became the current TFile (or "directory") for ROOT. In order to
  // desplice the original waveforms, we must move back to the TFile
  // (or "directory) that contains the original waveforms UPON WHICH
  // we want to operate. Recall that TFiles should be though of as
  // unix-like directories that we can move to/from...it's a bit
  // confusing but that's ROOT.
  ADAQRootFile->cd();


  ///////////////////////
  // Process waveforms //
  ///////////////////////

  bool PeaksFound = false;
  
  for(int waveform=WaveformStart; waveform<WaveformEnd; waveform++){

    // Run sequential desplicing in a separate thread to allow full
    // control of the ADAQAnalysisGUI while processing
    if(SequentialArchitecture)
      gSystem->ProcessEvents();
    

    /////////////////////////////////////////
    // Calculate the Waveform_H member object
    
    // Get a set of data channal voltages from the TTree
    ADAQWaveformTree->GetEntry(waveform);

    // Assign the current data channel's voltage to RawVoltage
    RawVoltage = *WaveformVecPtrs[Channel];

    // Select the type of Waveform_H object to create
    if(RawWaveform or BaselineSubtractedWaveform)
      CalculateBSWaveform(Channel);
    else if(ZeroSuppressionWaveform)
      CalculateZSWaveform(Channel);

    // If specified by the user then calculate the RFQ current
    // integral for each waveform and aggregate the total (into the
    // 'TotalDeuterons' class data member) for persistent storage in the
    // despliced TFile. The 'false' argument specifies not to plot
    // integration results on the canvas.
    if(IntegratePearson)
      IntegratePearsonWaveform(false);


    ///////////////////////////
    // Find peaks and peak data
    
    // Find the peaks (and peak data) in the current waveform.
    // Function returns "true" ("false) if peaks are found (not found).
    PeaksFound = FindPeaks(Waveform_H[Channel], false);
    
    // If no peaks found, continue to next waveform to save CPU $
    if(!PeaksFound)
      continue;
    
    // Calculate the PSD integrals and determine if they pass through
    // the pulse-shape filter 
    if(UsePSDFilterManager[PSDChannel])
      CalculatePSDIntegrals(false);
    

    ////////////////////////////////////////
    // Iterate over the peak info structures

    // Operate on each of the peaks found in the Waveform_H object
    vector<PeakInfoStruct>::iterator peak_iter;
    for(peak_iter=PeakInfoVec.begin(); peak_iter!=PeakInfoVec.end(); peak_iter++){

      // Clear the TTree voltage variable for each new peak
      VoltageInADC_AllChannels[0].clear();
      

      //////////////////////////////////
      // Perform the waveform desplicing
      
      // Desplice each peak from the full waveform by taking the
      // samples (in time) corresponding to the lower and upper peak
      // limits and using them to assign the bounded voltage values to
      // the TTree variable from the Waveform_H object
      int index = 0;

      for(int sample=(*peak_iter).PeakLimit_Lower; sample<(*peak_iter).PeakLimit_Upper; sample++){
	VoltageInADC_AllChannels[0].push_back(Waveform_H[Channel]->GetBinContent(sample));;
	index++;
      }
      
      // Add a buffer (of length DesplicedWaveformBuffer in [samples]) of
      // zeros to the beginning and end of the despliced waveform to
      // aid future waveform processing

      VoltageInADC_AllChannels[0].insert(VoltageInADC_AllChannels[0].begin(),
					 DesplicedWaveformBuffer,
					 0);
      
      VoltageInADC_AllChannels[0].insert(VoltageInADC_AllChannels[0].end(),
					 DesplicedWaveformBuffer,
					 0);
      
      // Fill the TTree with this despliced waveform provided that the
      // current peak is not flagged as a piled up pulse nor flagged
      // as a pulse to be filtered out by pulse shape
      if((*peak_iter).PileupFlag == false and (*peak_iter).PSDFilterFlag == false)
	T->Fill();
    }
    
    // Update the user with progress
    if(IsMaster)
      if((waveform+1) % int(WaveformEnd*UpdateFreq*1.0/100) == 0)
	UpdateProcessingProgress(waveform);
  }
  
  // Make final updates to the progress bar, ensuring that it reaches
  // 100% and changes color to acknoqledge that processing is complete
  if(SequentialArchitecture){
    ProcessingProgress_PB->Increment(100);
    ProcessingProgress_PB->SetBarColor(ColorManager->Number2Pixel(32));
    ProcessingProgress_PB->SetForegroundColor(ColorManager->Number2Pixel(0));
  }
  
#ifdef MPI_ENABLED
  
  if(ParallelVerbose)
    cout << "\nADAQAnalysisGUI_MPI Node[" << MPI_Rank << "] : Reached the end-of-processing MPI barrier!"
	 << endl;

  MPI::COMM_WORLD.Barrier();
  
#endif
  
  if(IsMaster)
    cout << "\nADAQAnalysisGUI_MPI Node[0] : Waveform processing complete!\n"
	 << endl;
  
  ///////////////////////////////////////////////////////
  // Finish processing and creation of the despliced file

  // Store the values calculated in parallel into the class for
  // persistent storage in the despliced ROOT file
  
  PR->TotalDeuterons = TotalDeuterons;
  
  // Now that we are finished processing waveforms from the original
  // ADAQ ROOT file we want to switch the current TFile (or
  // "directory") to the despliced TFile such that we can store the
  // appropriate despliced objects in it and write it to disk
  F->cd();

  // Write the objects to the ROOT file. In parallel architecture, a
  // number of temporary, despliced ROOT files will be created in the
  // /tmp directory for later aggregation; in sequential architecture,
  // the final despliced ROOT will be created with the specified fle
  // name and file path
  T->Write();
  MP->Write("MeasParams");
  MC->Write("MeasComment");
  PR->Write("ParResults");
  F->Close();

  // Switch back to the ADAQRootFile TFile directory
  ADAQRootFile->cd();
  
  // Note: if in parallel architecture then at this point a series of
  // despliced TFiles now exists in the /tmp directory with each TFile
  // containing despliced waveforms for the range of original
  // waveforms allocated to each node.

#ifdef MPI_ENABLED

  /////////////////////////////////////////////////
  // Aggregrate parallel TFiles into a single TFile

  // Aggregate the total integrated RFQ current (if enabled) on each
  // of the parallal nodes to a single double value on the master
  double AggregatedCurrent = SumDoublesToMaster(TotalDeuterons);

  // Ensure all nodes are at the same point before aggregating files
  MPI::COMM_WORLD.Barrier();

  // If the present node is the master then use a TChain object to
  // merge the despliced waveform TTrees contained in each of the
  // TFiles created by each node 
  if(IsMaster){

  if(IsMaster and ParallelVerbose)
    cout << "\nADAQAnalysisGUI_MPI Node[" << MPI_Rank << "] : Beginning aggregation of ROOT TFiles ..."
	 << endl;
    
    // Create the TChain
    TChain *WaveformTreeChain = new TChain("WaveformTree");

    // Create each node's TFile name and add the TFile to the TChain
    for(int node=0; node<MPI_Size; node++){
      // Create the name
      stringstream ss;
      ss << "/tmp/DesplicedWaveforms.root.node" << node;
      string FName = ss.str();

      // Add TFile to TChain
      WaveformTreeChain->Add(FName.c_str());
    }

    // Using the TChain to merge all the TTrees in all the TFiles into
    // a single TTree contained in a TFile of the provided name. Note
    // that the name and location of this TFile correspond to the
    // values set by the user in the despliced widgets.
    WaveformTreeChain->Merge(DesplicedFileName.c_str());
    
    // Store the aggregated RFQ current value on the master into the
    // persistent parallel results class for writing to the ROOT file
    PR->TotalDeuterons = AggregatedCurrent;

    // Open the final despliced TFile, write the measurement
    // parameters and comment objects to it, and close the TFile.
    TFile *F_Final = new TFile(DesplicedFileName.c_str(), "update");
    MP->Write("MeasParams");
    MC->Write("MeasComment");
    PR->Write("ParResults");
    F_Final->Close();
    
    cout << "\nADAQAnalysisGUI_MPI Node[" << MPI_Rank << "] : Finished aggregation of ROOT TFiles!"
	 << endl;  
    
    cout << "\nADAQAnalysisGUI_MPI Node[" << MPI_Rank << "] : Finished production of despliced file!\n"
	 << endl;  
  }
#endif
}


void ADAQAnalysisGUI::UpdateProcessingProgress(int Waveform)
{
#ifndef MPI_ENABLED
  if(Waveform > 0)
    ProcessingProgress_PB->Increment(UpdateFreq);
#else
  if(Waveform == 0)
    cout << "\n\n"; 
  else
    cout << "\rADAQAnalysisGUI_MPI Node[0] : Estimated progress = " 
	 << setprecision(2)
	 << (Waveform*100./WaveformEnd) << "%"
	 << "       "
	 << flush;
#endif
}


// Method to create a pulse shape discrimination (PSD) filter, which
// is a TGraph composed of points defined by the user clicking on the
// active canvas to define a line used to delinate regions. This
// function is called by ::HandleCanvas() each time that the user
// clicks on the active pad, passing the x- and y-pixel click
// location to the function
void ADAQAnalysisGUI::CreatePSDFilter(int XPixel, int YPixel)
{
  // Pixel coordinates: (x,y) = (0,0) at the top left of the canvas
  // User coordinates: (x,y) at any point on the canvas corresponds to
  //                   that point's location on the plotted TGraph or
  //                   TH1/TH2

  // For some reason, it is necessary to correct the pixel-to-user Y
  // coordinate returned by ROOT; the X value seems slightly off as
  // well but only by a very small amount so it will be ignored.

  // Get the start and end positions of the TCanvas object in user
  // coordinates (the very bottom and top y extent values)
  double CanvasStart_YPos = gPad->GetY1();
  double CanvasEnd_YPos = gPad->GetY2();
    
  // Convert the clicked position in the pad pixel coordinates
  // (point on the pad in screen pixels starting in the top-left
  // corner) to data coordinates (point on the pad in coordinates
  // corresponding to the X and Y axis ranges of whatever is
  // currently plotted/histogrammed in the pad . Note that the Y
  // conversion requires obtaining the start and end vertical
  // positions of canvas and using them to get the exact number if
  // data coordinates. 
  double XPos = gPad->PixeltoX(XPixel);
  double YPos = gPad->PixeltoY(YPixel) + abs(CanvasStart_YPos) + abs(CanvasEnd_YPos);

  // Increment the number of points to be used with the TGraph
  PSDNumFilterPoints++;
    
  // Add the X and Y position in data coordinates to the vectors to
  // be used with the TGraph
  PSDFilterXPoints.push_back(XPos);
  PSDFilterYPoints.push_back(YPos);
 
  // Recreate the TGraph representing the "PSDFilter" 
  if(PSDFilterManager[PSDChannel]) delete PSDFilterManager[PSDChannel];
  PSDFilterManager[PSDChannel] = new TGraph(PSDNumFilterPoints, &PSDFilterXPoints[0], &PSDFilterYPoints[0]);
  
  PlotPSDFilter();
}


// Method to overplot the PSD filter TGraph on the existing canvas
void ADAQAnalysisGUI::PlotPSDFilter()
{
  PSDFilterManager[PSDChannel]->SetLineColor(2);
  PSDFilterManager[PSDChannel]->SetLineWidth(2);
  PSDFilterManager[PSDChannel]->SetMarkerStyle(20);
  PSDFilterManager[PSDChannel]->SetMarkerColor(2);
  PSDFilterManager[PSDChannel]->Draw("LP SAME");
  
  Canvas_EC->GetCanvas()->Update();
}


// Method to compute and plot the derivative of the pulse spectrum
void ADAQAnalysisGUI::PlotSpectrumDerivative()
{
  if(!Spectrum_H){
    CreateMessageBox("A valid Spectrum_H object was not found! Spectrum derivative plotting will abort!","Stop");
    return;
  }

  ///////////////////////////////////////
  // Calculate the spectrum derivative //
  ///////////////////////////////////////
  // The discrete spectrum derivative is simply defined as the
  // difference between bin N_i and N_i-1 and then scaled down. The
  // main purpose is more quantitatively examine spectra for peaks,
  // edges, and other features.

  // If the user has selected to plot the derivative on top of the
  // spectrum, assign a vertical offset that will be used to plot the
  // derivative vertically centered in the canvas window. Also, reduce
  // the vertical scale of the total derivative such that it will
  // appropriately fit within the spectrum canvas.
  double VerticalOffset = 0;
  double ScaleFactor = 1.;
  if(SpectrumOverplotDerivative_CB->IsDown()){
    VerticalOffset = Spectrum2Plot_H->GetMaximum() / 2;
    ScaleFactor = 1.3;
  }

  const int NumBins = Spectrum2Plot_H->GetNbinsX();

  double BinCenters[NumBins], Differences[NumBins];  

  double BinCenterErrors[NumBins], DifferenceErrors[NumBins];

  // Iterate over the bins to assign the bin center and the difference
  // between bins N_i and N_i-1 to the designed arrays
  for(int bin = 0; bin<NumBins; bin++){
    
    // Set the bin center right in between the two bin centers used to
    // calculate the spectrumderivative
    BinCenters[bin] = (Spectrum2Plot_H->GetBinCenter(bin) + Spectrum2Plot_H->GetBinCenter(bin-1)) / 2;
    
    // Recall that TH1F bin 0 is the underfill bin; therfore, set the
    // difference between bins 0/-1 and 0/1 to zero to account for
    // bins that do not contain relevant content for the derivative
    if(bin < 2){
      Differences[bin] = VerticalOffset;
      continue;
    }
    
    double Previous = Spectrum2Plot_H->GetBinContent(bin-1);
    double Current = Spectrum2Plot_H->GetBinContent(bin);

    // Compute the "derivative", i.e. the difference between the
    // current and previous bin contents
    Differences[bin] = (ScaleFactor*(Current - Previous)) + VerticalOffset;
    
    if(PlotAbsValueSpectrumDerivative_CB->IsDown())
      Differences[bin] = abs(Differences[bin]);
    
    // Assume that the error in the bin centers is zero
    BinCenterErrors[bin] = 0.;
    
    // Compute the error in the "derivative" by adding the bin
    // content in quadrature since bin content is uncorrelated. This
    // is a more *accurate* measure of error
    DifferenceErrors[bin] = sqrt(pow(sqrt(Current),2) + pow(sqrt(Previous),2));
    
    // Compute the error in the "derivative" by simply adding the
    // error in the current and previous bins. This is a very
    // *conservative* measure of error
    // DifferenceErrors[bin] = sqrt(Current) + sqrt(Previous);
  }
  

  //////////////////////////////
  // Set graphical attributes //
  //////////////////////////////

  ///////////////
  // Graph titles

  if(OverrideTitles_CB->IsDown()){
    // Assign the titles to strings
    Title = Title_TEL->GetEntry()->GetText();
    XAxisTitle = XAxisTitle_TEL->GetEntry()->GetText();
    YAxisTitle = YAxisTitle_TEL->GetEntry()->GetText();
  }
  // ... otherwise use the default titles
  else{
    // Assign the default titles
    Title = "Spectrum Derivative";
    XAxisTitle = "Pulse units [ADC]";
    YAxisTitle = "Counts";
  }

  // Get the desired axes divisions
  int XAxisDivs = XAxisDivs_NEL->GetEntry()->GetIntNumber();
  int YAxisDivs = YAxisDivs_NEL->GetEntry()->GetIntNumber();


  /////////////////////
  // Logarithmic Y axis
  
  if(PlotVerticalAxisInLog_CB->IsDown())
    gPad->SetLogy(true);
  else
    gPad->SetLogy(false);

  Canvas_EC->GetCanvas()->SetLeftMargin(0.13);
  Canvas_EC->GetCanvas()->SetBottomMargin(0.12);
  Canvas_EC->GetCanvas()->SetRightMargin(0.05);


  ///////////////////
  // Create the graph

  gStyle->SetEndErrorSize(0);
  
  TGraph *SpectrumDerivative_G;
  if(PlotSpectrumDerivativeError_CB->IsDown())
    SpectrumDerivative_G = new TGraphErrors(NumBins, BinCenters, Differences, BinCenterErrors, DifferenceErrors);
  else
    SpectrumDerivative_G = new TGraph(NumBins, BinCenters, Differences);

  SpectrumDerivative_G->SetTitle(Title.c_str());

  SpectrumDerivative_G->GetXaxis()->SetTitle(XAxisTitle.c_str());
  SpectrumDerivative_G->GetXaxis()->SetTitleSize(XAxisSize_NEL->GetEntry()->GetNumber());
  SpectrumDerivative_G->GetXaxis()->SetLabelSize(XAxisSize_NEL->GetEntry()->GetNumber());
  SpectrumDerivative_G->GetXaxis()->SetTitleOffset(XAxisOffset_NEL->GetEntry()->GetNumber());
  SpectrumDerivative_G->GetXaxis()->CenterTitle();
  SpectrumDerivative_G->GetXaxis()->SetNdivisions(XAxisDivs, true);

  SpectrumDerivative_G->GetYaxis()->SetTitle(YAxisTitle.c_str());
  SpectrumDerivative_G->GetYaxis()->SetTitleSize(YAxisSize_NEL->GetEntry()->GetNumber());
  SpectrumDerivative_G->GetYaxis()->SetLabelSize(YAxisSize_NEL->GetEntry()->GetNumber());
  SpectrumDerivative_G->GetYaxis()->SetTitleOffset(YAxisOffset_NEL->GetEntry()->GetNumber());
  SpectrumDerivative_G->GetYaxis()->CenterTitle();
  SpectrumDerivative_G->GetYaxis()->SetNdivisions(YAxisDivs, true);

  SpectrumDerivative_G->SetLineColor(2);
  SpectrumDerivative_G->SetLineWidth(2);
  SpectrumDerivative_G->SetMarkerStyle(20);
  SpectrumDerivative_G->SetMarkerSize(1.0);
  SpectrumDerivative_G->SetMarkerColor(2);

 
  //////////////////////
  // X and Y axis limits

  float Min, Max;

  // The X axis range should be identical to the spectrum
  XAxisLimits_THS->GetPosition(Min,Max);
  XAxisMin = Spectrum2Plot_H->GetXaxis()->GetXmax() * Min;
  XAxisMax = Spectrum2Plot_H->GetXaxis()->GetXmax() * Max;
  SpectrumDerivative_G->GetXaxis()->SetRangeUser(XAxisMin, XAxisMax);

  YAxisLimits_DVS->GetPosition(Min,Max);

  // The derivative can eithe be overplotted on the spectrum or
  // plotted on it's own, which determines how the Y-axis canvas
  // sliders should be used to set the Y axis ranges

  if(SpectrumOverplotDerivative_CB->IsDown()){

    if(PlotVerticalAxisInLog_CB->IsDown() and Max==1)
      YAxisMin = 1.0;
    else if(SpectrumFindBackground_CB->IsDown() and SpectrumLessBackground_RB->IsDown())
      YAxisMin = (Spectrum2Plot_H->GetMaximumBin() * (1-Max)) - (Spectrum2Plot_H->GetMaximumBin()*0.8);
    else
      YAxisMin = Spectrum2Plot_H->GetBinContent(Spectrum2Plot_H->GetMaximumBin()) * (1-Max);
    
    YAxisMax = Spectrum2Plot_H->GetBinContent(Spectrum2Plot_H->GetMaximumBin()) * (1-Min) * 1.05;
  
    SpectrumDerivative_G->GetYaxis()->SetRangeUser(YAxisMin, YAxisMax);
    
    if(PlotSpectrumDerivativeError_CB->IsDown()){
      SpectrumDerivative_G->SetLineColor(1);
      SpectrumDerivative_G->Draw("P SAME");
    }
    else
      SpectrumDerivative_G->Draw("LP SAME");
    
    // Plot a "zero" reference line (which is shifted by the vertical
    // offset just like the derivative) to show where the derivative
    // switches sign
    DerivativeReference_L->DrawLine(XAxisMin, VerticalOffset, XAxisMax, VerticalOffset);
  }
  
  else{

    // Calculates values that will allow the sliders to span the full
    // range of the derivative that, unlike all the other plotted
    // objects, typically has a substantial negative range
    double MinValue = SpectrumDerivative_G->GetHistogram()->GetMinimum();
    double MaxValue = SpectrumDerivative_G->GetHistogram()->GetMaximum();
    double AbsValue = abs(MinValue) + abs(MaxValue);

    YAxisMin = MinValue + (AbsValue * (1-Max));
    YAxisMax = MaxValue - (AbsValue * Min);

    SpectrumDerivative_G->GetYaxis()->SetRangeUser(YAxisMin, YAxisMax);
    
    if(PlotSpectrumDerivativeError_CB->IsDown()){
      SpectrumDerivative_G->SetLineColor(1);
      SpectrumDerivative_G->Draw("AP");
    }
    else
      SpectrumDerivative_G->Draw("ALP");
    
    // Set the class member int denoting that the canvas now contains
    // only a spectrum derivative
    CanvasContents = zSpectrumDerivative;
  }
  
  // If the TH1F object that stores the spectrum derivative exists
  // then delete/recreate it
  if(SpectrumDerivative_H) delete SpectrumDerivative_H;
  SpectrumDerivative_H = new TH1F("SpectrumDerivative_H","SpectrumDerivative_H", SpectrumNumBins, SpectrumMinBin, SpectrumMaxBin);
  
  // Iterate over the TGraph derivative points and assign them to the
  // TH1F object. The main purpose for converting this to a TH1F is so
  // that the generic/modular function SaveHistogramData() can be used
  // to output the spectrum derivative data to a file. We also
  // subtract off the vertical offset used for plotting to ensure the
  // derivative is vertically centered at zero.

  double x,y;
  for(int bin=0; bin<SpectrumNumBins; bin++){
    SpectrumDerivative_G->GetPoint(bin,x,y);
    SpectrumDerivative_H->SetBinContent(bin, y-VerticalOffset);
  }
  
  SpectrumDerivativeExists = true;
  
  Canvas_EC->GetCanvas()->Update();
}


// Method used to output a generic TH1 object to a data text file in
// the format column1 == bin center, column2 == bin content. Note that
// the function accepts class types TH1 such that any derived class
// (TH1F, TH1D ...) can be saved with this function
void ADAQAnalysisGUI::SaveHistogramData(TH1 *HistogramToSave_H)
{
  // Create character arrays that enable file type selection (.dat
  // files have data columns separated by spaces and .csv have data
  // columns separated by commas)
  const char *FileTypes[] = {"ASCII file",  "*.dat",
			     "CSV file",    "*.csv",
			     "ROOT file",   "*.root",
			     0,             0};
  
  TGFileInfo FileInformation;
  FileInformation.fFileTypes = FileTypes;
  FileInformation.fIniDir = StrDup(SaveHistogramDirectory.c_str());
  
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
      SaveHistogramDirectory  = ADAQRootFileName.substr(0,pos);

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

    if(FileExtension == ".dat" or FileExtension == ".csv"){

      string FullFileName = FileName + FileExtension;

      ofstream HistogramOutput(FullFileName.c_str(), ofstream::trunc);

      // Assign the data separator based on file extension
      string separator;
      if(FileExtension == ".dat")
	separator = "\t";
      else if(FileExtension == ".csv")
	separator = ",";
      
      int NumBins = HistogramToSave_H->GetNbinsX();
      
      for(int bin=0; bin<=NumBins; bin++)
	HistogramOutput << HistogramToSave_H->GetBinCenter(bin) << separator
			<< HistogramToSave_H->GetBinContent(bin)
			<< endl;
      
      HistogramOutput.close();
      
      string SuccessMessage = "The histogram data has been successfully saved to the following file:\n" + FullFileName;
      CreateMessageBox(SuccessMessage,"Asterisk");
    }
    else if(FileExtension == ".root"){

      string FullFileName = FileName + FileExtension;

      TFile *HistogramOutput = new TFile(FullFileName.c_str(), "recreate");
      
      HistogramToSave_H->Write("Spectrum");
      HistogramOutput->Close();
      
      string SuccessMessage = "The TH1 * histogram named 'Spectrum' has been successfully saved to the following file:\n" + FullFileName;
      CreateMessageBox(SuccessMessage,"Asterisk");
    }
    else{
      CreateMessageBox("Unacceptable file extension for the spectrum data file! Valid extensions are '.dat', '.csv', and '.root'!","Stop");
      return;
    }
  }
}


void ADAQAnalysisGUI::SetCalibrationWidgetState(bool WidgetState, EButtonState ButtonState)
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


///////////////////////////
// The C++ main() method //
///////////////////////////

int main(int argc, char *argv[])
{
  // Initialize the MPI session
#ifdef MPI_ENABLED
  MPI::Init_thread(argc, argv, MPI::THREAD_SERIALIZED);
#endif
  
  // Assign the binary architecture (sequential or parallel) to a bool
  // that will be used frequently in the code to determine arch. type
  bool ParallelArchitecture = false;
#ifdef MPI_ENABLED
  ParallelArchitecture = true;
#endif

  // A word on the use of the first cmd line arg: the first cmd line
  // arg is used in different ways depending on the binary
  // architecture. For sequential arch, the user may specify a valid
  // ADAQ ROOT file as the first cmd line arg to open automatically
  // upon launching the ADAQAnalysisGUI binary. For parallel arch, the
  // automatic call to the parallel binaries made by ADAQAnalysisGUI
  // contains the type of processing to be performed in parallel as
  // the first cmd line arg, which, at present, is "histogramming" for
  // spectra creation and "desplicing" for despliced file creation.

  // Get the zeroth command line arg (the binary name)
  string CmdLineBin = argv[0];
  
  // Get the first command line argument 
  string CmdLineArg;

  // If no first cmd line arg was specified then we will start the
  // analysis with no ADAQ ROOT file loaded via "unspecified" string
  if(argc==1)
    CmdLineArg = "unspecified";
  
  // If only 1 cmd line arg was specified then assign it to
  // corresponding string that will be passed to the ADAQAnalysis
  // class constructor for use
  else if(argc==2)
    CmdLineArg = argv[1];

  // Disallow any other combination of cmd line args and notify the
  // user about his/her mistake appropriately (arch. specific)
  else{
    if(CmdLineBin == "ADAQAnalysisGUI"){
      cout << "\nError! Unspecified command line arguments to ADAQAnalysisGUI!\n"
	   <<   "       Usage: ADAQAnalysisGUI </path/to/filename>\n"
	   << endl;
      exit(-42);
    }
    else{
      cout << "\nError! Unspecified command line arguments to ADAQAnalysisGUI_MPI!\n"
	   <<   "       Usage: ADAQAnalysisGUI <WaveformAnalysisType: {histogramming, desplicing, discriminating}\n"
	   << endl;
      exit(-42);
    }
  }    

  // Run ROOT in standalone mode
  TApplication *TheApplication = new TApplication("ADAQAnalysisGUI", &argc, argv);
  
  // Create the main analysis class
  ADAQAnalysisGUI *ADAQAnalysis = new ADAQAnalysisGUI(ParallelArchitecture, CmdLineArg);
  
  // For sequential binaries only ...
  if(!ParallelArchitecture){
    
    // Connect the main frame for the GUI
    ADAQAnalysis->Connect("CloseWindow()", "ADAQAnalysisGUI", ADAQAnalysis, "HandleTerminate()");

    // Run the application
    TheApplication->Run();
  }

  // Garbage collection
  delete ADAQAnalysis;
  delete TheApplication;

  // Finalize the MPI session
#ifdef MPI_ENABLED
  MPI::Finalize();
#endif
  
  return 0;
}
