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
// name: AAComputation.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAComputation class is responsible for all waveform
//       processing, computation, and analysis actions that are
//       performed during use of ADAQAnalysis. It is constructed as a
//       Meyer's singleton class and made available throughout the
//       code via static methods to facilitate its use. The AASettings
//       object is made available to it so that necessary values from
//       the GUI widgets can be used in calculations. It is also
//       responsible for handling parallel processing of waveforms.
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef __AAComputation_hh__
#define __AAComputation_hh__ 1

// ROOT
#include <TObject.h>
#include <TRootEmbeddedCanvas.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TBox.h>
#include <TSpectrum.h>
#include <TLine.h>
#include <TFile.h>
#include <TTree.h>
#include <TRandom.h>
#include <TColor.h>
#include <TGProgressBar.h>
#include <TF1.h>
#include <TCutG.h>

// C++
#include <string>
#include <vector>
using namespace std;

// ADAQ
#include "ADAQRootClasses.hh"
#include "ADAQReadoutInformation.hh"
#include "ADAQWaveformData.hh"
#include "ASIMEvent.hh"
#include "ASIMRun.hh"

// ADAQAnalysis
#include "AASettings.hh"
#include "AAParallelResults.hh"
#include "AATypes.hh"

#ifndef __CINT__
#include <boost/array.hpp>
#endif

#define MAX_DG_CHANNELS 16

class AAComputation : public TObject
{
public:
  AAComputation(string, Bool_t);
  ~AAComputation();

  static AAComputation *GetInstance();

  // Pointer set methods
  void SetProgressBarPointer(TGHProgressBar *PB) { ProcessingProgressBar = PB; }
  void SetADAQSettings(AASettings *AAS) { ADAQSettings = AAS; }

  
  ///////////////////////////
  // Data computation methods
  
  // File I/O methods
  Bool_t LoadADAQFile(string);
  void LoadLegacyADAQFile();
  Bool_t LoadASIMFile(string);
  Bool_t SaveHistogramData(string, string, string);
  void CreateDesplicedFile();

  // Waveform creation
  TH1F *CalculateRawWaveform(Int_t, Int_t);
  TH1F *CalculateBSWaveform(Int_t, Int_t, Bool_t CurrentWaveform=false);
  TH1F *CalculateZSWaveform(Int_t, Int_t, Bool_t CurrentWaveform=false);
  Double_t CalculateBaseline(vector<Int_t> *);  
  Double_t CalculateBaseline(TH1F *);
  
  // Waveform processing 
  Bool_t FindPeaks(TH1F *, Int_t);
  void FindPeakLimits(TH1F *);
  void IntegratePeaks();
  void FindPeakHeights();
  void RejectPileup(TH1F *);
  void AnalyzeWaveform(TH1F *);
  
  // Spectrum creation
  void ProcessSpectrumWaveforms();
  void CreateSpectrum();
  void CreateASIMSpectrum();
  void CalculateSpectrumBackground();

  // Spectrum processing
  void FindSpectrumPeaks();
  void IntegrateSpectrum();
  void FitSpectrum();
  TGraph *CalculateSpectrumDerivative();
  Bool_t WriteSpectrumFitResultsFile(string);

  // Spectrum calibration
  Bool_t SetCalibrationPoint(Int_t, Int_t, Double_t, Double_t);
  Bool_t SetCalibration(Int_t);
  Bool_t ClearCalibration(Int_t);
  Bool_t WriteCalibrationFile(Int_t, string);

  void SetEdgeBound(Double_t,Double_t);
  void FindEdge();

  // Pulse shape discrimination processing
  TH2F *ProcessPSDHistogramWaveforms();
  TH2F *CreatePSDHistogram();

  void CalculatePSDIntegrals(Bool_t);
  Bool_t ApplyPSDRegion(Double_t, Double_t);

  void AddPSDRegionPoint(Int_t, Int_t);
  void CreatePSDRegion();
  void ClearPSDRegion();
  void CreatePSDHistogramSlice(Int_t, Int_t);
  
  // Processing methods
  void UpdateProcessingProgress(Int_t);
  void ProcessWaveformsInParallel(string);


  ////////////////////////////////////////
  // Public access methods for member data

  // Waveform analysis results
  Double_t GetWaveformAnalysisHeight() {return WaveformAnalysisHeight;}
  Double_t GetWaveformAnalysisArea() {return WaveformAnalysisArea;}

  // Waveform peak data
  vector<PeakInfoStruct> GetPeakInfoVec() {return PeakInfoVec;}
  
  // Spectra
  void SetSpectrum(TH1F *H) 
  {
    Spectrum_H = H;
    SpectrumExists = true;
  }
  
  TH1F *GetSpectrum() {return (TH1F *)Spectrum_H->Clone();}
  TH1F *GetSpectrumBackground() {return (TH1F *)SpectrumBackground_H->Clone();}
  TH1F *GetSpectrumWithoutBackground() {return (TH1F *)SpectrumDeconvolved_H->Clone();}
  
  // Spectra calibrations
  vector<TGraph *> GetSpectraCalibrationData() { return SpectraCalibrationData; }
  vector<TF1 *> GetSpectraCalibrations() { return SpectraCalibrations; }
  vector<Int_t> GetSpectraCalibrationType() {return SpectraCalibrationType; }
  vector<Bool_t> GetUseSpectraCalibrations() { return UseSpectraCalibrations; }

  Double_t GetEdgePosition() {return EdgePosition;}
  Double_t GetHalfHeight() {return HalfHeight;}
  Bool_t GetEdgePositionFound() {return EdgePositionFound;}
  
  // Spectra analysis
  TH1F *GetSpectrumIntegral() { return SpectrumIntegral_H; }
  TF1 *GetSpectrumFit() { return SpectrumFit_F; }
  Double_t GetSpectrumIntegralValue() { return SpectrumIntegralValue; }
  Double_t GetSpectrumIntegralError() { return SpectrumIntegralError; }

  // Pulse shape discrimination histograms
  TH2F *GetPSDHistogram() { return PSDHistogram_H; }
  TH1D *GetPSDHistogramSlice() { return PSDHistogramSlice_H; }
  
  // Pulse shape discrimination regions
  vector<TCutG *> GetPSDRegions() { return PSDRegions; }
  vector<Bool_t> GetUsePSDRegions() { return UsePSDRegions; }
  void SetUsePSDRegions(int Channel, Bool_t Use) {UsePSDRegions[Channel] = Use;}
  vector<Double_t> &GetPSDRegionXPoints() {return PSDRegionXPoints;}
  vector<Double_t> &GetPSDRegionYPoints() {return PSDRegionYPoints;}

  // ADAQ file data
  string GetADAQFileName() { return ADAQFileName; }
  Bool_t GetADAQLegacyFileLoaded() {return ADAQLegacyFileLoaded;}
  Int_t GetADAQNumberOfWaveforms() {return ADAQWaveformTree->GetEntries();}
  ADAQRootMeasParams *GetADAQMeasurementParameters() {return ADAQMeasParams;}
  ADAQReadoutInformation *GetADAQReadoutInformation() {return ARI;}
  
  // ASIM file data
  string GetASIMFileName() { return ASIMFileName; }
  TList *GetASIMEventTreeList() { return ASIMEventTreeList; }
  
  // Bool_Teans
  Bool_t GetADAQFileLoaded() { return ADAQFileLoaded; }
  Bool_t GetASIMFileLoaded() { return ASIMFileLoaded; }
  Bool_t GetSpectrumExists() { return SpectrumExists; }
  Bool_t GetSpectrumBackgroundExists() { return SpectrumBackgroundExists; }
  Bool_t GetSpectrumDerivativeExists() { return SpectrumDerivativeExists; }
  Bool_t GetPSDHistogramExists() { return PSDHistogramExists; }
  Bool_t GetPSDHistogramSliceExists() { return PSDHistogramSliceExists; }


  ////////////////
  // Miscellaneous

  void CreateNewPeakFinder(Int_t NumPeaks){
    if(PeakFinder) delete PeakFinder;
    PeakFinder = new TSpectrum(NumPeaks);
  }
  
  
private:

  static AAComputation *TheComputationManager;

  TGHProgressBar *ProcessingProgressBar;
  AASettings *ADAQSettings;

  // Bool_Ts to specify architecture type
  Bool_t SequentialArchitecture, ParallelArchitecture;


  ///////////
  // File I/O

  TFile *ADAQFile;
  string ADAQFileName;
  Bool_t ADAQFileLoaded, ADAQLegacyFileLoaded;
  
  TTree *ADAQWaveformTree;
  
  TString MachineName, MachineUser, FileDate, FileVersion;
  ADAQReadoutInformation *ARI;
  vector<Int_t> *Waveforms[MAX_DG_CHANNELS];
  ADAQWaveformData *WaveformData[MAX_DG_CHANNELS];
  
  ADAQRootMeasParams *ADAQMeasParams;

  TFile *ASIMFile;
  string ASIMFileName;
  Bool_t ASIMFileLoaded;

  TList *ASIMEventTreeList;
  ASIMEvent *ASIMEvt;

  AAParallelResults *ADAQParResults;
  Bool_t ADAQParResultsLoaded;


  //////////////////////
  // Waveforms variables

  vector<TH1F *> Waveform_H;
  
  vector<Int_t> Time, RawVoltage;
  Int_t RecordLength;
  Double_t Baseline;
  
  // Peak finding machinery

  TSpectrum *PeakFinder;
  Int_t NumPeaks;
  vector<PeakInfoStruct> PeakInfoVec;
  vector<Int_t> PeakIntegral_LowerLimit, PeakIntegral_UpperLimit;
#ifndef __CINT__
  vector< boost::array<int,2> > PeakLimits;
#endif

  // Waveform processing range
  Int_t WaveformStart, WaveformEnd;

  // Waveform analysis results
  Double_t WaveformAnalysisHeight, WaveformAnalysisArea;

  
  /////////////////////
  // Spectrum variables

  TH1F *Spectrum_H;
  TH1F *SpectrumDerivative_H;
  TGraph *SpectrumDerivative_G;

  TH1F *SpectrumBackground_H, *SpectrumDeconvolved_H;
  TH1F *SpectrumIntegral_H;
  TF1 *SpectrumFit_F;
  Double_t SpectrumIntegralValue, SpectrumIntegralError;

  // Vectors used to store processed waveform values
  vector<Double_t> SpectrumPHVec[MAX_DG_CHANNELS], SpectrumPAVec[MAX_DG_CHANNELS];

  // Objects that are used in energy calibration of the pulse spectra
  vector<TGraph *>SpectraCalibrationData;
  vector<TF1 *> SpectraCalibrations;
  vector<Bool_t> UseSpectraCalibrations;
  vector<ADAQChannelCalibrationData> CalibrationData;
  
  enum {zCalibrationFit, zCalibrationInterp};
  vector<Int_t> SpectraCalibrationType;
  

  ////////////////
  // PSD variables

  // Variables for PSD histograms and filter
  TH2F *PSDHistogram_H, *MasterPSDHistogram_H;
  TH1D *PSDHistogramSlice_H;
  
  vector<Double_t> PSDHistogramTotalVec[MAX_DG_CHANNELS], PSDHistogramTailVec[MAX_DG_CHANNELS];
  
  Double_t PSDRegionPolarity;
  vector<TCutG *> PSDRegions;
  vector<Bool_t> UsePSDRegions;
  vector<Double_t> PSDRegionXPoints, PSDRegionYPoints;


  ///////////
  // Bool_Teans
  Bool_t SpectrumExists, SpectrumBackgroundExists, SpectrumDerivativeExists;
  Bool_t PSDHistogramExists, PSDHistogramSliceExists;


  ///////////
  // Parallel

  Int_t MPI_Size, MPI_Rank;
  Bool_t IsMaster, IsSlave;

  Bool_t ParallelVerbose;
  TFile *ParallelFile;

  // Variables used to specify whether to print to stdout
  Bool_t Verbose;

  // ROOT TH1F to hold aggregated spectra from parallel processing
  TH1F *MasterHistogram_H;

  // Number of data channels in the ADAQ ROOT files
  Int_t NumDataChannels;

  // Aggregated total waveform peaks found during processing
  Int_t TotalPeaks;
  
  // Create a TColor ROOT object to handle pixel-2-color conversions
  TColor *ColorManager;

  // A ROOT random number generator (RNG)
  TRandom *RNG;

  vector<Double_t> EdgeHBound, EdgeVBound;
  Double_t HalfHeight, EdgePosition;
  Bool_t EdgePositionFound;

  // Define the class to ROOT
  ClassDef(AAComputation, 1)
};

#endif
