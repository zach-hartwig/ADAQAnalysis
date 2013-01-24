//////////////////////////////////////////////////////////////////////////////////////////
//
// name: ADAQAnalysisGUI.hh
// date: 23 Jan 13 (Last updated)
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

#ifndef __ADAQAnalysisGUI_hh__
#define __ADAQAnalysisGUI_hh__ 1

// ROOT
#include <TGraph.h>
#include <TROOT.h>
#include <TGMenu.h>
#include <TGButton.h>
#include <TGSlider.h>
#include <TGNumberEntry.h>
#include <TRootEmbeddedCanvas.h>
#include <TGDoubleSlider.h>
#include <TGTripleSlider.h>
#include <TColor.h>
#include <TFile.h>
#include <TTree.h>
#include <TLine.h>
#include <TBox.h>
#include <TSpectrum.h>
#include <TGraph.h>
#include <TGProgressBar.h>
#include <TObject.h>
#include <TRandom.h>
#include <TGMsgBox.h>
#include <TH1F.h>
#include <TH2F.h>

// C++
#include <string>
#include <vector>
#include <cmath>
#include <fstream>
using namespace std;

// Boost
#ifndef __CINT__
#include <boost/array.hpp>
#endif

// ADAQ
#include "ADAQRootGUIClasses.hh"
#include "ADAQAnalysisTypes.hh"


class ADAQAnalysisGUI : public TGMainFrame
{

public:

  // Constructor/destructor
  ADAQAnalysisGUI(bool, string);
  ~ADAQAnalysisGUI();
  
  // Create the ROOT graphical interface
  void CreateMainFrame();

  // Load an ADAQ ROOT file for processing
  void LoadADAQRootFile();

  // Member functions that are used to extract/produce waveforms
  void CalculateRawWaveform(int);
  void CalculateBSWaveform(int, bool CurrentWaveform=false);
  void CalculateZSWaveform(int, bool CurrentWaveform=false);

  // Methods to operate on waveforms and spectra
  bool FindPeaks(TH1F *, bool PlotPeaksAndGraphics=true);
  void FindPeakLimits(TH1F *, bool PlotPeaksAndGraphics=true);
  void IntegratePeaks();
  void RejectPileup(TH1F *);
  void FindPeakHeights();
  void FindSpectrumPeaks();
  void IntegrateSpectrum();
  void SaveSpectrumData(string, string);
  void CreateSpectrum();
  void CreateDesplicedFile();
  void CreatePSDHistogram();
  double CalculateBaseline(vector<int> *);  
  void CalculateSpectrumBackground(TH1F *);

  // Methods to handle the processing of waveforms in parallel
  void ProcessWaveformsInParallel(string);
  void SaveParallelProcessingData();
  void LoadParallelProcessingData();
  double* SumDoubleArrayToMaster(double*, size_t);
  double SumDoublesToMaster(double);

  // Methods to individual waveforms and spectra
  void PlotWaveform();
  void PlotSpectrum();
  void PlotPSDHistogram();

  // Methods for general waveform analysis
  void CalculateCountRate();
  void CalculatePSDIntegrals(bool);
  bool ApplyPSDFilter(double,double);
  void IntegratePearsonWaveform(bool PlotPearsonIntegration=true);
  
  // Method to readout ROOT widget values to class member data
  void ReadoutWidgetValues();

  // Method to alert the user via a ROOT message box
  void CreateMessageBox(string, string);

  void UpdateProcessingProgress(int);
  void CreatePSDFilter(int, int);
  void PlotPSDFilter();

  // Methods to recieve and act upon ROOT widget signals
  void HandleMenu(int);
  void HandleTextButtons();
  void HandleCheckButtons();
  void HandleSliders(int);
  void HandleDoubleSliders();
  void HandleTripleSliderPointer();
  void HandleNumberEntries();
  void HandleRadioButtons();
  void HandleComboBox(int, int);
  void HandleCanvas(int, int, int, TObject *);
  void HandleTerminate();

private:

  /////////////////////////////////////////////////////////
  // Widget objects for the WaveformOptions tabbed frame //
  /////////////////////////////////////////////////////////

  // Widgets for selecting waveform number and digitizer channel
  ADAQNumberEntryWithLabel *WaveformSelector_NEL;
  ADAQComboBoxWithLabel *ChannelSelector_CBL;

  // Widgets for setting general plot options
  TGButtonGroup *WaveformType_BG;
  TGRadioButton *RawWaveform_RB, *BaselineSubtractedWaveform_RB, *ZeroSuppressionWaveform_RB;

  // Widgets for setting the waveform (signal) polarity
  TGButtonGroup *WaveformPolarity_BG;
  TGRadioButton *PositiveWaveform_RB, *NegativeWaveform_RB;

  TGCheckButton *AutoYAxisRange_CB;
  TGCheckButton *PlotTrigger_CB;
  TGTextButton *ResetXAxisLimits_TB;
  TGTextButton *ResetYAxisLimits_TB;

  // Widgets for controlling baseline calc. and plotting
  TGCheckButton *PlotBaseline_CB;
  ADAQNumberEntryWithLabel *BaselineCalcMin_NEL;
  ADAQNumberEntryWithLabel *BaselineCalcMax_NEL;

  // Widgets for controlling peak finding and plotting
  TGCheckButton *PlotZeroSuppressionCeiling_CB;
  ADAQNumberEntryWithLabel *ZeroSuppressionCeiling_NEL;

  TGCheckButton *FindPeaks_CB;
  ADAQNumberEntryWithLabel *MaxPeaks_NEL;
  ADAQNumberEntryWithLabel *Sigma_NEL;
  ADAQNumberEntryWithLabel *Resolution_NEL;
  ADAQNumberEntryWithLabel *Floor_NEL;
  TGCheckButton *PlotFloor_CB;
  TGCheckButton *PlotCrossings_CB;
  TGCheckButton *PlotPeakIntegratingRegion_CB;

  // Widget to enable/disable pileup rejection algorithm
  TGCheckButton *UsePileupRejection_CB;


  ///////////////////////////////////////////
  // Widgets for the spectrum tabbed frame //
  ///////////////////////////////////////////

  ADAQNumberEntryWithLabel *WaveformsToHistogram_NEL;

  ADAQNumberEntryWithLabel *SpectrumNumBins_NEL;
  ADAQNumberEntryWithLabel *SpectrumMinBin_NEL;
  ADAQNumberEntryWithLabel *SpectrumMaxBin_NEL;

  TGButtonGroup *SpectrumType_BG;
  TGRadioButton *SpectrumTypePAS_RB, *SpectrumTypePHS_RB;

  TGButtonGroup *IntegrationType_BG;
  TGRadioButton *IntegrationTypePeakFinder_RB;
  TGRadioButton *IntegrationTypeWholeWaveform_RB;

  TGCheckButton *SpectrumCalibration_CB;
  ADAQComboBoxWithLabel *SpectrumCalibrationPoint_CBL;
  ADAQNumberEntryWithLabel *SpectrumCalibrationEnergy_NEL;
  ADAQNumberEntryWithLabel *SpectrumCalibrationPulseUnit_NEL;
  TGTextButton *SpectrumCalibrationSetPoint_TB;
  TGTextButton *SpectrumCalibrationCalibrate_TB;
  TGTextButton *SpectrumCalibrationPlot_TB;
  TGTextButton *SpectrumCalibrationReset_TB;

  TGCheckButton *SpectrumFindPeaks_CB;
  ADAQNumberEntryWithLabel *SpectrumNumPeaks_NEL;
  ADAQNumberEntryWithLabel *SpectrumSigma_NEL;
  ADAQNumberEntryWithLabel *SpectrumResolution_NEL;

  TGCheckButton *SpectrumFindBackground_CB;
  ADAQNumberEntryWithLabel *SpectrumRangeMin_NEL;
  ADAQNumberEntryWithLabel *SpectrumRangeMax_NEL;
  TGRadioButton *SpectrumWithBackground_RB;
  TGRadioButton *SpectrumLessBackground_RB;

  TGCheckButton *SpectrumFindIntegral_CB;
  TGCheckButton *SpectrumIntegralInCounts_CB;
  TGCheckButton *SpectrumUseGaussianFit_CB;
  TGCheckButton *SpectrumNormalizePeakToCurrent_CB;
  ADAQNumberEntryFieldWithLabel *SpectrumIntegral_NEFL;
  ADAQNumberEntryFieldWithLabel *SpectrumIntegralError_NEFL;

  TGTextButton *CreateSpectrum_TB;


  ///////////////////////////////////////////
  // Widgets for the analysis tabbed frame //
  ///////////////////////////////////////////
  
  TGCheckButton *PSDEnable_CB, *PSDPlotTailIntegration_CB;
  ADAQComboBoxWithLabel *PSDChannel_CBL;
  ADAQNumberEntryWithLabel *PSDThreshold_NEL;
  ADAQNumberEntryWithLabel *PSDPeakOffset_NEL;
  ADAQNumberEntryWithLabel *PSDTailOffset_NEL;
  ADAQNumberEntryWithLabel *PSDWaveforms_NEL;

  ADAQNumberEntryWithLabel *PSDNumTailBins_NEL, *PSDMinTailBin_NEL, *PSDMaxTailBin_NEL;
  ADAQNumberEntryWithLabel *PSDNumTotalBins_NEL, *PSDMinTotalBin_NEL, *PSDMaxTotalBin_NEL;

  ADAQComboBoxWithLabel *PSDPlotType_CBL;
  
  TGTextButton *PSDCalculate_TB;
  
  TGCheckButton *PSDEnableFilterCreation_CB;
  TGCheckButton *PSDEnableFilter_CB;
  TGRadioButton *PSDPositiveFilter_RB, *PSDNegativeFilter_RB;
  TGTextButton *PSDClearFilter_TB;
  
  ADAQNumberEntryWithLabel *RFQPulseWidth_NEL;
  ADAQNumberEntryWithLabel *RFQRepRate_NEL;
  ADAQNumberEntryWithLabel *CountRateWaveforms_NEL;
  TGTextButton *CalculateCountRate_TB;
  ADAQNumberEntryFieldWithLabel *InstCountRate_NEFL, *AvgCountRate_NEFL;


  ///////////////////////////////////////////
  // Widgets for the graphics tabbed frame //
  ///////////////////////////////////////////

  TGCheckButton *OverrideTitles_CB;

  ADAQTextEntryWithLabel *Title_TEL;
  ADAQTextEntryWithLabel *XAxisTitle_TEL;
  ADAQTextEntryWithLabel *YAxisTitle_TEL;
  ADAQTextEntryWithLabel *ZAxisTitle_TEL;
  ADAQTextEntryWithLabel *PaletteAxisTitle_TEL;

  ADAQNumberEntryWithLabel *XAxisSize_NEL, *XAxisOffset_NEL, *XAxisDivs_NEL;
  ADAQNumberEntryWithLabel *YAxisSize_NEL, *YAxisOffset_NEL, *YAxisDivs_NEL;
  ADAQNumberEntryWithLabel *ZAxisSize_NEL, *ZAxisOffset_NEL, *ZAxisDivs_NEL;
  ADAQNumberEntryWithLabel *PaletteAxisSize_NEL, *PaletteAxisOffset_NEL;
  ADAQNumberEntryWithLabel *PaletteX1_NEL, *PaletteX2_NEL, *PaletteY1_NEL, *PaletteY2_NEL;
  
  TGButtonGroup *WaveformDrawOptions_BG;
  TGRadioButton *DrawWaveformWithCurve_RB, *DrawWaveformWithMarkers_RB, *DrawWaveformWithBoth_RB;
  
  TGButtonGroup *SpectrumDrawOptions_BG;
  TGRadioButton *DrawSpectrumWithCurve_RB, *DrawSpectrumWithMarkers_RB, *DrawSpectrumWithBars_RB;
  
  TGCheckButton *SetStatsOff_CB;
  TGCheckButton *PlotVerticalAxisInLog_CB;
  
  TGTextButton *ReplotWaveform_TB;
  TGTextButton *ReplotSpectrum_TB;
  TGTextButton *ReplotPSDHistogram_TB;


  /////////////////////////////////////////////
  // Widgets for the processing tabbed frame //
  /////////////////////////////////////////////

  TGButtonGroup *ProcessingType_BG;
  TGRadioButton *ProcessingSeq_RB, *ProcessingPar_RB;
  ADAQNumberEntryWithLabel *NumProcessors_NEL;
  ADAQNumberEntryWithLabel *UpdateFreq_NEL;

  TGCheckButton *IntegratePearson_CB;
  ADAQComboBoxWithLabel *PearsonChannel_CBL;
  TGRadioButton *PearsonPolarityPositive_RB, *PearsonPolarityNegative_RB;
  TGCheckButton *PlotPearsonIntegration_CB;
  TGRadioButton *IntegrateRawPearson_RB, *IntegrateFitToPearson_RB;
  ADAQNumberEntryWithLabel *PearsonLowerLimit_NEL, *PearsonMiddleLimit_NEL, *PearsonUpperLimit_NEL;
  ADAQNumberEntryFieldWithLabel *DeuteronsInWaveform_NEFL;
  ADAQNumberEntryFieldWithLabel *DeuteronsInTotal_NEFL;

  TGTextButton *DesplicedFileSelection_TB;
  TGTextEntry *DesplicedFileName_TE;
  ADAQNumberEntryWithLabel *DesplicedWaveformBuffer_NEL;
  ADAQNumberEntryWithLabel *DesplicedWaveformNumber_NEL;
  ADAQNumberEntryWithLabel *DesplicedWaveformLength_NEL;
  TGTextButton *DesplicedFileCreation_TB;


  ///////////////////////////////////////////
  // Widget objects for the "Canvas" frame //
  ///////////////////////////////////////////

  // Widget to display open ADAQ ROOT file statistics
  TGTextEntry *FileName_TE;
  TGNumberEntryField *Waveforms_NEL, *RecordLength_NEL;
  TGLabel *WaveformLabel_L, *RecordLengthLabel_L;

  // The canvas to display all plots
  TRootEmbeddedCanvas *Canvas_EC;

  // Triple and double slider widgets for zooming in X and Y (the
  // pointer in the triple slider is used for spectrum calibration)
  TGTripleHSlider *XAxisLimits_THS;
  TGDoubleVSlider *YAxisLimits_DVS;

  // Slider widget for quickly scrolling through accessible waveforms
  TGHSlider *WaveformSelector_HS;

  TGDoubleHSlider *SpectrumIntegrationLimits_DHS;

  // A progress bar! Hot stuff.
  TGHProgressBar *ProcessingProgress_PB;

  // Widget for quiting the GUI
  TGTextButton *Quit_TB;


  //////////////////////////////////////
  // Member variables for general use //
  //////////////////////////////////////

  // Objects for opening ADAQ ROOT files and accessing them
  ADAQRootMeasParams *ADAQMeasParams;
  TFile *ADAQRootFile;
  string ADAQRootFileName;
  bool ADAQRootFileLoaded;
  TTree *ADAQWaveformTree;
  ADAQAnalysisParallelResults *ADAQParResults;
  bool ADAQParResultsLoaded;
  
  // Strings used to store the current directory for various files
  string DataDirectory, SaveSpectrumDirectory;
  string PrintCanvasDirectory, DesplicedDirectory;
  
  // Vectors and variables for extracting waveforms from the TTree in
  // the ADAQ ROOT file and storing the information in vector format
  vector<int> *WaveformVecPtrs[8];
  vector<int> Time, RawVoltage;
  int RecordLength;

  // String objects for storing the file name and extension of graphic
  // files that will receive the contents of the embedded canvas
  string GraphicFileName, GraphicFileExtension;  

  // ROOT graphical objects to represent various settings
  TLine *Trigger_L, *Floor_L, *Calibration_L, *ZSCeiling_L, *NCeiling_L;
  TLine *LPeakDelimiter_L, *RPeakDelimiter_L;
  TLine *PearsonLowerLimit_L, *PearsonMiddleLimit_L, *PearsonUpperLimit_L;
  TLine *PSDPeakOffset_L, *PSDTailOffset_L;
  TLine *LowerLimit_L, *UpperLimit_L;
  TBox *Baseline_B, *PSDTailIntegral_B;
  
  // ROOT TH1F histograms for storing the waveform and pulse spectrum
  TH1F *Waveform_H[8], *Spectrum_H, *SpectrumBackground_H, *SpectrumDeconvolved_H;
  
  // ROOT TSpectrum peak-finding object that operates on TH1F
  TSpectrum *PeakFinder;

  // Variables to specify the range of waveforms for processing
  int WaveformStart, WaveformEnd;

  // Variables and objects that hold peak information. Note that
  // anything that uses Boost must be protected from ROOT's C++
  // interpretor in order to successfully compile
  int NumPeaks;
  vector<PeakInfoStruct> PeakInfoVec;
  vector<int> PeakIntegral_LowerLimit, PeakIntegral_UpperLimit;
#ifndef __CINT__
  vector< boost::array<int,2> > PeakLimits;
#endif
  
  // Objects that are used in energy calibration of the pulse spectra
  vector<TGraph *> CalibrationManager;
  vector<bool> UseCalibrationManager;
  vector<ADAQChannelCalibrationData> CalibrationData;
  
  // Variables used in parallel architecture
  int MPI_Size, MPI_Rank;
  bool IsMaster, IsSlave;

  // Bools to specify architecture type
  bool ParallelArchitecture, SequentialArchitecture;

  // Variables used to specify whether to print to stdout
  bool Verbose, ParallelVerbose;
  
  // Member data for storing ROOT widget variables
  int WaveformToPlot, Channel;
  int SpectrumNumBins, SpectrumMinBin, SpectrumMaxBin;
  int WaveformsToHistogram, MaxPeaks, ZeroSuppressionCeiling;
  int BaselineCalcMin, BaselineCalcMax, Floor;
  int UpdateFreq;
  bool PlotFloor, PlotCrossings, PlotPeakIntegratingRegion, PlotPearsonIntegration, UsePileupRejection;
  bool IntegratePearson, IntegrateRawPearson, IntegrateFitToPearson;
  int PearsonLowerLimit, PearsonMiddleLimit, PearsonUpperLimit;
  bool RawWaveform, BaselineSubtractedWaveform, ZeroSuppressionWaveform;
  bool IntegrationTypeWholeWaveform, IntegrationTypePeakFinder, SpectrumTypePHS, SpectrumTypePAS;
  int PearsonChannel;
  double Sigma, Resolution;
  string DesplicedFileName;
  int DesplicedWaveformBuffer, WaveformsToDesplice, DesplicedWaveformLength;
  int PSDChannel, PSDThreshold, PSDPeakOffset, PSDTailOffset;
  int PSDWaveformsToDiscriminate;
  int PSDNumTailBins, PSDMinTailBin, PSDMaxTailBin;
  int PSDNumTotalBins, PSDMinTotalBin, PSDMaxTotalBin;

  // Strings for specifying binaries and ROOT files
  string ADAQHOME;
  string ParallelBinaryName;
  string ParallelProcessingFName;
  string ParallelProgressFName;
  
  // ROOT TFile to hold values need for parallel processing
  TFile *ParallelProcessingFile;

  // ROOT TH1F to hold aggregated spectra from parallel processing
  TH1F *MasterHistogram_H;

  // Number of processors/threads available on current system
  int NumProcessors;

  // Generally useful data members
  float XAxisMin, XAxisMax, YAxisMin, YAxisMax;
  string Title, XAxisTitle, YAxisTitle, ZAxisTitle, PaletteAxisTitle;

  // Variables to hold waveform processing values
  double WaveformPolarity, PearsonPolarity, Baseline;

  // Variables for PSD histograms and filter
  TH2F *PSDHistogram_H, *MasterPSDHistogram_H;
  double PSDFilterPolarity;
  vector<TGraph *> PSDFilterManager;
  vector<bool> UsePSDFilterManager;
  int PSDNumFilterPoints;
  vector<int> PSDFilterXPoints, PSDFilterYPoints;

  // Maximum bit value of CAEN X720 digitizer family (4095)
  int V1720MaximumBit;

  // Number of data channels in the ADAQ ROOT files
  int NumDataChannels;

  // Bool to determine what canvas presently contains
  bool CanvasContainsSpectrum, SpectrumExists;
  bool CanvasContainsPSDHistogram, PSDHistogramExists;

  // Max number of peaks expected for TSpectrum on spectra
  int SpectrumMaxPeaks;

  // Aggregated total waveform peaks found during processing
  int TotalPeaks;
  
  // Aggregated total number of deuterons from the RFQ
  double TotalDeuterons;

  // Bin widths for spectrum integration algorithms
  double GaussianBinWidth, CountsBinWidth;
  
  // Create a TColor ROOT object to handle pixel-2-color conversions
  TColor *ColorManager;

  // A ROOT random number generator (RNG)
  TRandom *RNG;
  
  // Define the ADAQAnalysisGUI to ROOT
  ClassDef(ADAQAnalysisGUI, 1);
};
  


#endif
