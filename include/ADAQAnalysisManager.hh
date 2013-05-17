//////////////////////////////////////////////////////////////////////////////////////////
//
// name: ADAQAnalysisInterface.hh
// date: 17 May 13
// auth: Zach Hartwig
//
// desc: 
//
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __ADAQAnalysisManager_hh__
#define __ADAQAnalysisManager_hh__ 1

#include <TObject.h>
#include <TRootEmbeddedCanvas.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TBox.h>
#include <TSpectrum.h>
#include <TLine.h>
#include <TFile.h>
#include <TTree.h>
#include <TRandom.h>
#include <TColor.h>

#include <string>
#include <vector>
using namespace std;

#include "ADAQRootGUIClasses.hh"
#include "ADAQAnalysisTypes.hh"

#ifndef __CINT__
#include <boost/array.hpp>
#endif



class ADAQAnalysisManager : public TObject
{
public:
  ADAQAnalysisManager(bool);
  ~ADAQAnalysisManager();

  static ADAQAnalysisManager *GetInstance();
  
  void SetEmbeddedCanvasPointer(TRootEmbeddedCanvas *EC) {TheEmbeddedCanvas_EC = EC;}
  TRootEmbeddedCanvas *GetEmbeddedCanvasPointer() {return TheEmbeddedCanvas_EC;}
  
  void SetFileOpen(bool FO) {FileOpen = FO;}
  bool GetFileOpen() {return FileOpen;}

  void SetSpectrumExists(bool SE) {SpectrumExists = SE;}
  bool GetSpectrumExists() {return SpectrumExists;}

  void SetSpectrumDerivativeExists(bool SDE) {SpectrumDerivativeExists = SDE;}
  bool GetSpectrumDerivativeExists() {return SpectrumDerivativeExists;}

  void SetPSDHistogramExists(bool PHE) {PSDHistogramExists = PHE;}
  bool GetPSDHistogramExists() {return PSDHistogramExists;}

  void TestDraw(){

    TH1F *H = new TH1F("H","H",200,-4,4);
    H->FillRandom("gaus",1000000);
    H->SetFillColor(2);
    TheEmbeddedCanvas_EC->GetCanvas()->cd(0);
    H->Draw("B");
    TheEmbeddedCanvas_EC->GetCanvas()->Update();
  }


  bool LoadADAQRootFile(string);
  bool LoadACRONYMRootFile(string);


  
private:
  bool FileOpen;


  
  


  /*
  void CalculateRawWaveform(int);
  void CalculateBSWaveform(int, bool CurrentWaveform=false);
  void CalculateZSWaveform(int, bool CurrentWaveform=false);

  bool FindPeaks(TH1F *, bool PlotPeaksAndGraphics=true);
  void FindPeakLimits(TH1F *, bool PlotPeaksAndGraphics=true);
  void IntegratePeaks();
  void RejectPileup(TH1F *);
  void FindPeakHeights();
  void FindSpectrumPeaks();
  void IntegrateSpectrum();
  void CreateSpectrum();
  void CreateDesplicedFile();
  void CreatePSDHistogram();
  double CalculateBaseline(vector<int> *);  
  void CalculateSpectrumBackground(TH1F *);
  
  // Method to save a generic histogram to a data file
  void SaveHistogramData(TH1 *);
  
  // Methods to handle the processing of waveforms in parallel
  void ProcessWaveformsInParallel(string);
  void SaveParallelProcessingData();
  void LoadParallelProcessingData();
  double* SumDoubleArrayToMaster(double*, size_t);
  double SumDoublesToMaster(double);

  // Methods to individual waveforms and spectra
  void PlotWaveform();
  void PlotSpectrum();
  void PlotSpectrumDerivative();
  void PlotPSDHistogram();
  void PlotPSDHistogramSlice(int, int);

  // Methods for general waveform analysis
  void CalculateCountRate();
  void CalculatePSDIntegrals(bool);
  bool ApplyPSDFilter(double,double);
  void IntegratePearsonWaveform(bool PlotPearsonIntegration=true);

  void UpdateProcessingProgress(int);
  void CreatePSDFilter(int, int);
  void PlotPSDFilter();
  void SetCalibrationWidgetState(bool, EButtonState);
  */

private:
  
  static ADAQAnalysisManager *TheAnalysisManager;

  TRootEmbeddedCanvas *TheEmbeddedCanvas_EC;

  // Objects for opening ADAQ ROOT files and accessing them
  ADAQRootMeasParams *ADAQMeasParams;
  TFile *ADAQRootFile;
  string ADAQRootFileName;
  bool ADAQRootFileLoaded;
  TTree *ADAQWaveformTree;
  ADAQAnalysisParallelResults *ADAQParResults;
  bool ADAQParResultsLoaded;
  
  // Strings used to store the current directory for various files
  string DataDirectory, SaveSpectrumDirectory, SaveHistogramDirectory;;
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
  TLine *DerivativeReference_L;
  TBox *Baseline_B, *PSDTailIntegral_B;
  
  // ROOT TH1F histograms for storing the waveform and pulse spectrum
  TH1F *Waveform_H[8], *Spectrum_H, *SpectrumBackground_H, *SpectrumDeconvolved_H;

  TH1F *Spectrum2Plot_H;

  TH1F *SpectrumDerivative_H;
  TH1D *PSDHistogramSlice_H;
  
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
  string ADAQHOME, USER;
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

  // Bool to determine what graphical objects exist
  bool SpectrumExists, SpectrumDerivativeExists;
  bool PSDHistogramExists, PSDHistogramSliceExists;

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

  enum {zWaveform, zSpectrum, zSpectrumDerivative, zPSDHistogram};
  int CanvasContents;

  // Define the class to ROOT
  ClassDef(ADAQAnalysisManager, 1)
};

#endif
