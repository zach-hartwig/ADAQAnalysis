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
// name: AAComputation.cc
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

// ROOT
#include <TSystem.h>
#include <TError.h>
#include <TF1.h>
#include <TVector.h>
#include <TChain.h>
#include <TDirectory.h>
#include <TKey.h>

// C++
#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
using namespace std;

// MPI
#ifdef MPI_ENABLED
#include <mpi.h>
#endif

// Boost
#include <boost/array.hpp>
#include <boost/thread.hpp>

// ADAQAnalysis
#include "AAComputation.hh"
#include "AAParallel.hh"


AAComputation *AAComputation::TheComputationManager = 0;


AAComputation *AAComputation::GetInstance()
{ return TheComputationManager; }


AAComputation::AAComputation(string CmdLineArg, bool PA)
  : SequentialArchitecture(!PA), ParallelArchitecture(PA),
    ADAQFile(new TFile), ADAQFileName(""), ADAQFileLoaded(false), ADAQLegacyFileLoaded(false),
    ADAQWaveformTree(new TTree),

    ASIMFile(new TFile), ASIMFileName(""), ASIMFileLoaded(false), 
    ASIMEventTreeList(new TList), ASIMEvt(new ASIMEvent),
    
    ADAQParResults(NULL), ADAQParResultsLoaded(false),
    Time(0), RawVoltage(0), RecordLength(0), Baseline(0.),
    PeakFinder(new TSpectrum), NumPeaks(0), PeakInfoVec(0), 
    PeakIntegral_LowerLimit(0), PeakIntegral_UpperLimit(0), PeakLimits(0),
    WaveformStart(0), WaveformEnd(0),
    WaveformAnalysisHeight(0.), WaveformAnalysisArea(0.), 
    Spectrum_H(new TH1F), SpectrumDerivative_H(new TH1F), SpectrumDerivative_G(new TGraph),
    SpectrumBackground_H(new TH1F), SpectrumDeconvolved_H(new TH1F), 
    SpectrumIntegral_H(new TH1F), SpectrumFit_F(new TF1),
    PSDHistogram_H(new TH2F), MasterPSDHistogram_H(new TH2F), PSDHistogramSlice_H(new TH1D),
    PSDRegionPolarity(1.),
   
    SpectrumExists(false), SpectrumBackgroundExists(false), SpectrumDerivativeExists(false), 
    PSDHistogramExists(false), PSDHistogramSliceExists(false),

    MPI_Size(1), MPI_Rank(0), IsMaster(true), IsSlave(false), ParallelVerbose(true),
    Verbose(false), NumDataChannels(16), TotalPeaks(0), 
    HalfHeight(0.), EdgePosition(0.), EdgePositionFound(false)
{
  if(TheComputationManager){
    cout << "\nADAQAnalysis error! TheComputationManager was constructed twice!\n" << endl;
    exit(-42);
  }
  else
    TheComputationManager = this;

  
  
  // Initialize the objects used in the calibration and pulse shape
  // discrimination (PSD) scheme. A set of 8 calibration and 8 PSD
  // filter "managers" -- really, TGraph *'s used for interpolating --
  // along with the corresponding booleans that determine whether or
  // not that channel's "manager" should be used is initialized. 

  for(Int_t ch=0; ch<NumDataChannels; ch++){
    // All 8 channel's managers are set "off" by default
    UseSpectraCalibrations.push_back(false);
    SpectraCalibrationType.push_back(zCalibrationFit);
      
    UsePSDRegions.push_back(false);
    
    // Store empty TCutG pointers in std::vectors to hold space and
    // prevent seg. faults later when we test/delete unused objects
    SpectraCalibrationData.push_back(new TGraph);
    SpectraCalibrations.push_back(new TF1);
    PSDRegions.push_back(new TCutG);
    
    // Assign a blank initial structure for channel calibration data
    ADAQChannelCalibrationData Init;
    CalibrationData.push_back(Init);
    
    Waveform_H.push_back(new TH1F);
  }

  // Set ROOT to print only break messages and above (to suppress the
  // annoying warning from TSpectrum that the peak buffer is full)
  gErrorIgnoreLevel = kBreak;

 
  ///////////////////////////
  // Parallel architecture //
  ///////////////////////////
  // The parallel binary of ADAQAnalysisInterface (which is called
  // "ADAQAnalysisInterface_MPI" when build via the parallel.sh/Makefile
  // files) is completely handled (initiation, processing, results
  // collection) by the sequential binary of ADAQAnalysisInterface. It
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
 
  if(ParallelArchitecture){

    AAParallel *ParallelMgr = AAParallel::GetInstance();

    MPI_Rank = ParallelMgr->GetRank();
    MPI_Size = ParallelMgr->GetSize();
    IsMaster = ParallelMgr->GetIsMaster();
    IsSlave = !IsMaster;

    // Load the parameters required for processing from the ROOT file
    // generated from the sequential binary's ROOT widget settings
    string USER = getenv("USER");
    string ADAQSettingsFile = "/tmp/ADAQSettings_" + USER + ".root";
    TFile *F = new TFile(ADAQSettingsFile.c_str(), "read");
    
    ADAQSettings = dynamic_cast<AASettings *>(F->Get("ADAQSettings"));
    if(!ADAQSettings){
      cout << "\nError! ADAQAnalysis_MPI could not find the ADAQSettings object!\n"
	   << endl;
      exit(-42);
    }
    else
      // Load the specified ADAQ ROOT file
      LoadADAQFile(ADAQSettings->ADAQFileName);
    
    // Initiate the desired parallel waveform processing algorithm

    // Histogram waveforms into a spectrum
    if(CmdLineArg == "histogramming")
      ProcessSpectrumWaveforms();
    
    // Desplice (or "uncouple") waveforms into a new ADAQ ROOT file
    else if(CmdLineArg == "desplicing")
      CreateDesplicedFile();
    
    else if(CmdLineArg == "discriminating")
      ProcessPSDHistogramWaveforms();
    
    // Notify the user of error in processing type specification
    else{
      cout << "\nError! Unspecified command line argument '" << CmdLineArg << "' passed to ADAQAnalysis_MPI!\n"
	   <<   "       At present, only the args 'histogramming' and 'desplicing' are allowed\n"
	   <<   "       Parallel binaries will exit ...\n"
	   << endl;
      exit(-42);
    }
  }
}


AAComputation::~AAComputation()
{;}


bool AAComputation::LoadADAQFile(string FileName)
{
  //////////////////////////////////
  // Open the specified ROOT file //
  //////////////////////////////////

  ADAQFileName = FileName;

  // Open the specified ROOT file 
  ADAQFile = new TFile(FileName.c_str(), "read");

  // If a valid ADAQ ROOT file was NOT opened...
  if(!ADAQFile->IsOpen())
    ADAQFileLoaded = false;
  else{
    
    // At present (13 Jan 15) there are two main types of ADAQ file:

    // The "legacy" file format that was the original file format used
    // between 2012 and 2015. These file types were produced for a
    // number of experiments (AIMS, DNDO/ARI, and other projects), and
    // thus, support must be enabled for backwards
    // compatibility. Typically, these files will have the ".root" or
    // ".adaq" extension.
    //
    // The new "production" ADAQ file format that is far more
    // comprehensive, flexible, and standardized in all senses. These
    // files are denoted with the extension ".adaq.root" by force if
    // created with ADAQAcquisition.

    // Determine whether the specified ADAQ file format is "legacy" or
    // "production" by the presence of the FileVersion TObjString

    TObjString *FileVersionOS = NULL;
    FileVersionOS = (TObjString *)ADAQFile->Get("FileVersion");
    
    // Handle loading the ADAQ file with format-appropriate methods
 
    // The "legacy" ADAQ file version
    if(FileVersionOS == NULL)
      LoadLegacyADAQFile();
    
    // The "production" ADAQ file version
    else{

      /////////////////////////
      // Get ADAQ file metadata

      TObjString *OS = (TObjString *)ADAQFile->Get("MachineName");
      MachineName = OS->GetString();
      
      OS = (TObjString *)ADAQFile->Get("MachineUser");
      MachineUser = OS->GetString();

      OS = (TObjString *)ADAQFile->Get("FileDate");
      FileDate = OS->GetString();

      FileVersion = FileVersionOS->GetString();
      

      /////////////////////////////////////////
      // Get the ADAQ readout information class
      
      ARI = (ADAQReadoutInformation *)ADAQFile->Get("ReadoutInformation");


      ///////////////////////////////////////////
      // Get the waveform and waveform data trees
      
      // Get the pointers to the waveform TTree stored in the TFile
      ADAQWaveformTree = (TTree *)ADAQFile->Get("WaveformTree");

      // For each digitizer channel that was enabled when creating the
      // ADAQ file, there are two associated branches named:
      // - "WaveformChX" stores the digitized waveform itself
      // - "WaveformDataChX" stores analyzed waveform data
      // where "X" is the digitizer channel number
      
      // Set branch addresses for the TTree
      int NumChannels = ARI->GetDGNumChannels();
      for(int ch=0; ch<NumChannels; ch++){
	
	stringstream SS;
	SS << "WaveformCh" << ch;
	TString BranchName = SS.str();

	// Initialize pointer to the digitized waveform, which is
	// stored as a vector<Int_t> * in the TTree branch
	Waveforms[ch] = 0;
	
	ADAQWaveformTree->SetBranchStatus(BranchName, 1);
	ADAQWaveformTree->SetBranchAddress(BranchName, &Waveforms[ch]);

	// Initialize pointer to the ADAQWaveformData class
	WaveformData[ch] = new ADAQWaveformData;

	SS.str("");
	SS << "WaveformDataCh" << ch;
	BranchName = SS.str();
	
	ADAQWaveformTree->SetBranchStatus(BranchName, 1);
	ADAQWaveformTree->SetBranchAddress(BranchName, &WaveformData[ch]);
      }

      ADAQLegacyFileLoaded = false;
    }
    
    // An ADAQ file should be successfull loaded at this point
    ADAQFileLoaded = true;
  }
  
  return ADAQFileLoaded;
}

void AAComputation::LoadLegacyADAQFile()
{
  /////////////////////////////////////
  // Extract data from the ROOT file //
  /////////////////////////////////////
  
  // Get the ADAQRootMeasParams objects stored in the ROOT file
  ADAQMeasParams = (ADAQRootMeasParams *)ADAQFile->Get("MeasParams");
  
  // Get the TTree with waveforms stored in the ROOT file
  ADAQWaveformTree = (TTree *)ADAQFile->Get("WaveformTree");
  
  // Seg fault protection against valid ROOT files that are missing
  // the essential ADAQ objects.
  if(ADAQMeasParams == NULL || ADAQWaveformTree == NULL){
    ADAQFileLoaded = false;
  }
  
  // Attempt to get the class containing results that were calculated
  // in a parallel run of ADAQAnalysisGUI and stored persistently
  // along with data in the ROOT file. At present, this is only the
  // despliced waveform files. For standard ADAQ ROOT files, the
  // ADAQParResults class member will be a NULL pointer
  ADAQParResults = dynamic_cast<AAParallelResults *>(ADAQFile->Get("ParResults"));
  
  // If a valid class with the parallel parameters was found
  // (despliced ADAQ ROOT files, etc) then set the bool to true; if no
  // class with parallel parameters was found (standard ADAQ ROOT
  // files) then set the bool to false. This bool will be used
  // appropriately throughout the code to import parallel results.
  (ADAQParResults) ? ADAQParResultsLoaded = true : ADAQParResultsLoaded = false;
  
  // If the ADAQParResults class was loaded then ...
  if(ADAQParResultsLoaded){
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
  // initializes the member object Waveforms to be vector of
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
    
    // Initialize the vector<int> pointers! No initialization worked
    // in ROOT v5.34.19 and below but created a very difficult to
    // track seg. fault for higher versions.
    Waveforms[ch] = 0;
    
    // Set the present channels' class object vector pointer to the
    // address of that chennel's vector<int> stored in the TTree
    ADAQWaveformTree->SetBranchAddress(BranchName.c_str(), &Waveforms[ch]);
    
    // Clear the string for the next channel.
    ss.str("");
  }

  // Update ADAQ file loaded booleans
  ADAQFileLoaded = true;
  ADAQLegacyFileLoaded = true;
}

    
bool AAComputation::LoadASIMFile(string FileName)
{
  // Set the ASIM File name
  ASIMFileName = FileName;
  
  // Open the ASIM ROOT file in read-only mode
  ASIMFile = new TFile(FileName.c_str(), "read");

  // Recreate the TList that contains TTrees with ADAQSimulationEvents
  if(ASIMEventTreeList) delete ASIMEventTreeList;
  ASIMEventTreeList = new TList;
  
  if(!ASIMFile->IsOpen()){
    ASIMFileLoaded = false;
  }
  else{
    // Iterate over the TFile using the TObject keys to search for
    // TTrees to add to the EventTreeList. The EventTreeList will be
    // access later for analysis. Note that the only TTrees that
    // should be present in ASIM files are those that contain
    // event-level information in branches with ADAQSimulationEvent
    // objects.

    TIter It(ASIMFile->GetListOfKeys());
    TKey *Key;
    while((Key = (TKey *)It.Next())){
      TString ClassType = ASIMFile->Get(Key->GetName())->ClassName();
      
      if(ClassType == "TTree"){
	TTree *Tree = (TTree *)ASIMFile->Get(Key->GetName());
	ASIMEventTreeList->Add(Tree);
      }
    }
    ASIMFileLoaded = true;
  }
  return ASIMFileLoaded;
}


TH1F *AAComputation::CalculateRawWaveform(int Channel, int Waveform)
{
  // Readout the desired waveform from the tree
  ADAQWaveformTree->GetEntry(Waveform);

  // Set individual channel waveform address
  vector<int> RawVoltage = *Waveforms[Channel];

  // Get waveform size. This accounts for the possibility of waveforms
  // that vary length from event-to-event, such as with ZLE algorithm
  int Size = RawVoltage.size();
  
  // Create a TH1F representing the waveform
  if(Waveform_H[Channel])
    delete Waveform_H[Channel];
  Waveform_H[Channel] = new TH1F("Waveform_H", "Raw Waveform", Size-1, 0, Size);
  
  if(!RawVoltage.empty()){
    Baseline = CalculateBaseline(&RawVoltage);
    
    vector<int>::iterator iter;
    for(iter=RawVoltage.begin(); iter!=RawVoltage.end(); iter++)
      Waveform_H[Channel]->SetBinContent((iter-RawVoltage.begin()), *iter);
  }
  return Waveform_H[Channel];
}


// Method to extract the digitized data on the specified data channel
// and store it into a TH1F object after calculating and subtracting
// the waveform baseline. Note that the function argument bool is
// depracated code but left in place for potential future use
TH1F* AAComputation::CalculateBSWaveform(int Channel, int Waveform, bool CurrentWaveform)
{
  ADAQWaveformTree->GetEntry(Waveform);

  vector<Int_t> RawVoltage = *Waveforms[Channel];
  
  Int_t Size = RawVoltage.size();

  if(Waveform_H[Channel])
    delete Waveform_H[Channel];
  Waveform_H[Channel] = new TH1F("Waveform_H","Baseline-subtracted Waveform", Size-1, 0, Size);
  
  Double_t Polarity = ADAQSettings->WaveformPolarity;
  
  if(!RawVoltage.empty()){
    Baseline = CalculateBaseline(&RawVoltage);
    
    vector<int>::iterator it;  
    for(it=RawVoltage.begin(); it!=RawVoltage.end(); it++)
      Waveform_H[Channel]->SetBinContent((it-RawVoltage.begin()),
					 Polarity*(*it-Baseline));
  }
  return Waveform_H[Channel];
}


// Method to extract the digitized data on the specified data channel
// and store it into a TH1F object after computing the zero-suppresion
// (ZS) waveform. The baseline is calculated and stored into the class
// member for later use. Note that the function argument bool is
// depracated but left in place for potential future use.
TH1F *AAComputation::CalculateZSWaveform(int Channel, int Waveform, bool CurrentWaveform)
{
  ADAQWaveformTree->GetEntry(Waveform);
  
  Double_t Polarity = ADAQSettings->WaveformPolarity;
  
  vector<Int_t> RawVoltage = *Waveforms[Channel];
  
  if(Waveform_H[Channel])
    delete Waveform_H[Channel];
  
  if(RawVoltage.empty()){
    Waveform_H[Channel] = new TH1F("Waveform_H","Zero Suppression Waveform", RecordLength-1, 0, RecordLength);
  }
  else{
    Baseline = CalculateBaseline(&RawVoltage);
    
    vector<Double_t> ZSVoltage(ADAQSettings->ZeroSuppressionBuffer,0);
    
    vector<int>::iterator it;
    for(it=RawVoltage.begin(); it!=RawVoltage.end(); it++){

      Double_t VoltageMinusBaseline = Polarity*(*it-Baseline);
      
      if(VoltageMinusBaseline >= ADAQSettings->ZeroSuppressionCeiling)
	ZSVoltage.push_back(VoltageMinusBaseline);
    }
    
    for(Int_t sample=0; sample<ADAQSettings->ZeroSuppressionBuffer; sample++)
      ZSVoltage.push_back(0);
  
    Int_t ZSWaveformSize = ZSVoltage.size();
    Waveform_H[Channel] = new TH1F("Waveform_H","Zero Suppression Waveform", ZSWaveformSize-1, 0, ZSWaveformSize);
  
    vector<double>::iterator iter;
    for(iter=ZSVoltage.begin(); iter!=ZSVoltage.end(); iter++)
      Waveform_H[Channel]->SetBinContent((iter-ZSVoltage.begin()), *iter);
  }
  return Waveform_H[Channel];
}


// The following methods compute the baseline of a waveform (as a
// vector<int> or as a TH1F *). The baseline is the average of the
// waveform voltage taken over the specified range in time. The units
// of the baseline are in [samples]
double AAComputation::CalculateBaseline(vector<int> *Waveform)
{
  int BaselineRegionLength = ADAQSettings->BaselineRegionMax - ADAQSettings->BaselineRegionMin;
  double Baseline = 0.;
  for(int sample=ADAQSettings->BaselineRegionMin; sample<ADAQSettings->BaselineRegionMax; sample++)
    Baseline += ((*Waveform)[sample]*1.0/BaselineRegionLength);

  return Baseline;
}

double AAComputation::CalculateBaseline(TH1F *Waveform)
{
  int BaselineRegionLength = ADAQSettings->BaselineRegionMax - ADAQSettings->BaselineRegionMin;
  double Baseline = 0.;
  for(int sample=ADAQSettings->BaselineRegionMin; sample<ADAQSettings->BaselineRegionMax; sample++)
    Baseline += (Waveform->GetBinContent(sample)*1.0/BaselineRegionLength);

  return Baseline;
}


// Method used to find peaks in any TH1F object.  
bool AAComputation::FindPeaks(TH1F *Histogram_H, int PeakFindingAlgorithm)
{
  // Initialize the counter of successful peaks to zero and clear the
  // peak info vector in preparation for the next iteration
  NumPeaks = 0;
  PeakInfoVec.clear();
  
  //////////////////////////////
  // Use TSpectrum peak finding

  // This method of peak finding uses the TSpectrum class, which
  // provides a vastly more powerful and accurate peak finding method
  // but is much more computationally intensive. In addition to
  // accuracy, programmability, and robustness, this algorithm can
  // find many peaks within a single record length

  if(PeakFindingAlgorithm == zPeakFinder){
    
    // Use the PeakFinder to determine the number of potential peaks
    // in the waveform and return the total number of peaks that the
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

    string PeakFinderOptions = "goff nodraw";
    if(!ADAQSettings->UseMarkovSmoothing)
      PeakFinderOptions += " noMarkov";
  
    int NumPotentialPeaks = PeakFinder->Search(Histogram_H,
					       ADAQSettings->Sigma,
					       PeakFinderOptions.c_str(),
					       ADAQSettings->Resolution);

    // Since the PeakFinder actually found potential peaks then get the
    // X and Y positions of the potential peaks from the PeakFinder
    Double_t *PotentialPeakPosX = PeakFinder->GetPositionX();
    Double_t *PotentialPeakPosY = PeakFinder->GetPositionY();
  
    // For each of the potential peaks found by the PeakFinder...
    for(int peak=0; peak<NumPotentialPeaks; peak++){
    
      // Determine if the peaks Y value (voltage) is above the "floor"
      //if(PotentialPeakPosY[peak] > Floor_NEL->GetEntry()->GetIntNumber()){
      if(PotentialPeakPosY[peak] > ADAQSettings->Floor){
      
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
      }
    }

    // Call the member functions that will find the lower (leftwards on
    // the time axis) and upper (rightwards on the time axis)
    // integration limits for each successful peak in the waveform
    FindPeakLimits(Histogram_H);
  }
  

  /////////////////////////////////////////////////////
  // Use simple "whole waveform" peak finding algorithm

  // This method uses TH1 methods to perform an extremely simple peak
  // finding algorithm. While only a single peak can be found since
  // the algorithm uses absolute height within a record length to find
  // the peak position, the algorithm is extremely fast.

  else if(PeakFindingAlgorithm == zWholeWaveform){
    
    // Create a new peak info struct, fill it, and push it back into
    // the storage vector; note only one peak will be found
    PeakInfoStruct PeakInfo;
    PeakInfo.PeakID = 0;
    PeakInfo.PeakPosX = Histogram_H->GetMaximumBin();
    PeakInfo.PeakPosY = Histogram_H->GetBinContent(Histogram_H->GetMaximumBin());
    PeakInfoVec.push_back(PeakInfo);

    NumPeaks++;
  }

  
  // Function returns 'false' if zero peaks are found; algorithms can
  // use this flag to exit from analysis for this acquisition window
  // to save on CPU time
  if(NumPeaks == 0)
    return false;
  else
    return true;

}


// Method to find the lower/upper peak limits in any histogram.
void AAComputation::FindPeakLimits(TH1F *Histogram_H)
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
    if(PreStepValue<ADAQSettings->Floor and PostStepValue>=ADAQSettings->Floor)
      FloorCrossing_Low2High.push_back(sample-1);
    
    // If a high-to-low floor crossing occurred ...
    if(PreStepValue>=ADAQSettings->Floor and PostStepValue<ADAQSettings->Floor)
      FloorCrossing_High2Low.push_back(sample);
    
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
  if(ADAQSettings->UsePileupRejection)
    RejectPileup(Histogram_H);
}


void AAComputation::ProcessSpectrumWaveforms()
{
  // Get the current digitizer channel to analyze
  Int_t Channel = ADAQSettings->WaveformChannel;
  
  // Delete the previous Spectrum_H TH1F object if it exists to
  // prevent memory leaks
  
  if(Spectrum_H){
    delete Spectrum_H;
    SpectrumExists = false;
  }
  
  // Clear the spectrum pulse value vectors to zero elements. I have
  // chosen *not* to preallocate memory intentionally since (a)
  // vector::push_back() is easily sufficiently fast compared to
  // preallocation for our purposes and (b) the PF algorithm, which
  // can find multiple values per pulse and therefore does not have a
  // fixed vector length, makes preallocation difficult.

  SpectrumPHVec[Channel].clear();
  SpectrumPAVec[Channel].clear();
  
  // Reset the waveform progress bar
  if(SequentialArchitecture){
    ProcessingProgressBar->Reset();
    ProcessingProgressBar->SetBarColor(ColorManager->Number2Pixel(33));
    ProcessingProgressBar->SetForegroundColor(ColorManager->Number2Pixel(1));
  }
  
  // Create a new TH1F histogram object for spectra creation
  Spectrum_H = new TH1F("Spectrum_H", "ADAQ spectrum", 
			ADAQSettings->SpectrumNumBins, 
			ADAQSettings->SpectrumMinBin,
			ADAQSettings->SpectrumMaxBin);
  
  // Variables for calculating pulse height and area

  Double_t SampleHeight = 0.;
  Double_t PulseHeight = 0., PrevPulseHeight = 0.;
  Double_t PulseArea = 0., PrevPulseArea = 0.;
  
  
  ///////////////////////////////////////////////////
  // Create the spectrum from stored waveform data //
  ///////////////////////////////////////////////////

  if(ADAQSettings->ADAQSpectrumAlgorithmWD){

    ////////////////////////////////////////////
    // Check to ensure that waveform data exists

    // Using waveform data is not available for legacy ADAQ files
    if(ADAQLegacyFileLoaded){
      SpectrumExists = false;
      return;
    }
    
    if(!ARI->GetStoreEnergyData()){
      SpectrumExists = false;
      return;
    }
    
    //////////////////////////////////////////////
    // Readout the waveform data into the spectrum
    
    // Set the ADAQWaveformData branch name depending on channel number
    stringstream SS;
    SS << "WaveformDataCh" << Channel;
    string WDName = SS.str();

    // Readout appropriate waveform data into the spectrum
    for(Int_t entry=0; entry<ADAQSettings->WaveformsToHistogram; entry++){
      
      // Ensure only the specified number of events are processed
      if(entry == ADAQSettings->WaveformsToHistogram)
	break;
      
      ADAQWaveformTree->GetEntry(entry);
      
      // Get the pulse height and area...
      
      PulseHeight = WaveformData[Channel]->GetPulseHeight();
      PulseArea = WaveformData[Channel]->GetPulseArea();

      // The following is a temporary hack. As of ADAQ libraries
      // version 1.6.0, all digitizer channel data is stored in a
      // single tree with channel-specific branches. When a
      // TTree::Fill() is called, all branches are saved. When
      // operating with multiple detectors, this causes non-triggered
      // branched to write whatever data is present from previously
      // triggered events. The fix below prevents adding this
      // redundant data to the spectra. In the future, each channel is
      // likely to get its own TTree, resolving this issue

      if(ADAQSettings->ADAQSpectrumTypePHS){
	if(PulseHeight == PrevPulseHeight)
	  continue;
	else
	  PrevPulseHeight = PulseHeight;
      }
      else if(ADAQSettings->ADAQSpectrumTypePAS){
	if(PulseArea == PrevPulseArea)
	  continue;
	else
	  PrevPulseArea = PulseArea;
      }
      
      // If specified, assess the waveform's pulse shape and exclude
      // it from the pulse spectra if it fails the test

      Bool_t PSDReject = false;
      
      if(ADAQSettings->UsePSDRegions[Channel]){
	
	// Get the stored PSD total and tail integrals
	Double_t Total = WaveformData[Channel]->GetPSDTotalIntegral();
	Double_t Tail = WaveformData[Channel]->GetPSDTailIntegral();
	
	// Flag the waveform if it is not accepted
	if(ApplyPSDRegion(Total, Tail))
	  PSDReject = true;
      }
      
      if(PSDReject)
	continue;

      // Add uncalibrated waveform data to the storage vectors for
      // potential later use
      SpectrumPHVec[Channel].push_back(PulseHeight);
      SpectrumPAVec[Channel].push_back(PulseArea);

      // Determine pulse height/area value for analysis
      
      Double_t Quantity = 0.;
      if(ADAQSettings->ADAQSpectrumTypePHS)
	Quantity = PulseHeight;
      else if(ADAQSettings->ADAQSpectrumTypePAS)
	Quantity = PulseArea;
      
      // Convert the quantity if calibration has been activated

      if(ADAQSettings->UseSpectraCalibrations[Channel]){
	if(SpectraCalibrationType[Channel] == zCalibrationFit)      	  
	  Quantity = ADAQSettings->SpectraCalibrations[Channel]->Eval(Quantity);
	else if(SpectraCalibrationType[Channel] == zCalibrationInterp)
	  Quantity = ADAQSettings->SpectraCalibrationData[Channel]->Eval(Quantity);
      }
      
      // Fill the spectrum is quantity is within thresholds
      
      if(Quantity > ADAQSettings->SpectrumMinThresh and
	 Quantity < ADAQSettings->SpectrumMaxThresh)
	Spectrum_H->Fill(Quantity);
    }
    SpectrumExists = true;
  }
  
  
  /////////////////////////////////////////////////
  // Create the spectrum by processing waveforms //
  /////////////////////////////////////////////////
  
  else{

    if(!ARI->GetStoreRawWaveforms()){
      SpectrumExists = false;
      return;
    }
    
    // Reboot the PeakFinder with up-to-date max peaks
    if(PeakFinder) delete PeakFinder;
    PeakFinder = new TSpectrum(ADAQSettings->MaxPeaks);

    // Assign the range of waveforms that will be analyzed to create a
    // histogram. Note that in sequential architecture if N waveforms
    // are to be histogrammed, waveforms from waveform_ID == 0 to
    // waveform_ID == (WaveformsToHistogram-1) will be included in the
    // final spectra 
    WaveformStart = 0; // Start (include this waveform in final histogram)
    WaveformEnd = ADAQSettings->WaveformsToHistogram; // End (Up to but NOT including this waveform)

#ifdef MPI_ENABLED

    // If the waveform processing is to be done in parallel then
    // distribute the events as evenly as possible between the master
    // (rank == 0) and the slaves (rank != 0) to maximize computational
    // efficienct. Note that the master will carry the small leftover
    // waveforms from the even division of labor to the slaves.
  
    // Assign the number of waveforms processed by each slave
    int SlaveEvents = int(ADAQSettings->WaveformsToHistogram/MPI_Size);

    // Assign the number of waveforms processed by the master
    int MasterEvents = int(ADAQSettings->WaveformsToHistogram-SlaveEvents*(MPI_Size-1));

    if(ParallelVerbose and IsMaster)
      cout << "\nADAQAnalysis_MPI Node[0] : Number waveforms allocated to master (node == 0) : " << MasterEvents << "\n"
	   <<   "                           Number waveforms allocated to slaves (node != 0) : " << SlaveEvents
	   << endl;
  
    // Divide up the total number of waveforms to be processed amongst
    // the master and slaves as evenly as possible. Note that the
    // 'WaveformStart' value is *included* whereas the 'WaveformEnd'
    // value _excluded_ from the for loop range 
    WaveformStart = (MPI_Rank * SlaveEvents) + (ADAQSettings->WaveformsToHistogram % MPI_Size);
    WaveformEnd = (MPI_Rank * SlaveEvents) + MasterEvents;
  
    // The master _always_ starts on waveform zero. This is required
    // with the waveform allocation algorithm abov.
    if(IsMaster)
      WaveformStart = 0;
  
    if(ParallelVerbose)
      cout << "\nADAQAnalysis_MPI Node[" << MPI_Rank << "] : Handling waveforms " << WaveformStart << " to " << (WaveformEnd-1)
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
      RawVoltage = *Waveforms[Channel];
    
      // Calculate the selected waveform that will be analyzed into the
      // spectrum histogram. Note that "raw" waveforms may not be
      // analyzed (simply due to how the code is presently setup) and
      // will default to analyzing the baseline subtracted waveform
      if(ADAQSettings->RawWaveform or ADAQSettings->BSWaveform)
	CalculateBSWaveform(Channel, waveform);
      else if(ADAQSettings->ZSWaveform)
	CalculateZSWaveform(Channel, waveform);
    
      ///////////////////////////////////////////
      // Simple max/sum (SMS) waveform processing

      if(ADAQSettings->ADAQSpectrumAlgorithmSMS){
	
	// If specified, calculate the PSD integrals for the waveform
	// and determine if they meet the acceptance criterion defined
	// by the current channel's PSD region. If not, continue the
	// processing loop to prevent adding the waveform height/area to
	// the pulse spectrum

	if(ADAQSettings->UsePSDRegions[Channel]){
	
	  FindPeaks(Waveform_H[Channel], zWholeWaveform);
	  CalculatePSDIntegrals(false);
	
	  if(PeakInfoVec[0].PSDFilterFlag == true)
	    continue;
	}

	////////////////////////////////////////////////
	// Calculation of waveform pulse height and area
	
	Int_t AnalysisMin = ADAQSettings->AnalysisRegionMin;
	Int_t AnalysisMax = ADAQSettings->AnalysisRegionMax;

	// Pulse height

	// Get the pulse height by finding the maximum bin value
	// within the waveform analysis region from the Waveform_H
	// TH1F via class methods. Note that spectra are always
	// created with positive polarity waveforms
	Waveform_H[Channel]->GetXaxis()->SetRange(AnalysisMin, AnalysisMax);
	PulseHeight = Waveform_H[Channel]->
	  GetBinContent(Waveform_H[Channel]->GetMaximumBin());
	
	
	// Store the uncalibrated pulse height in the designated vector
	SpectrumPHVec[Channel].push_back(PulseHeight);
	
	// If the calibration manager is to be used to convert the
	// value from pulse units [ADC] to energy units [keV, MeV,
	// ...] then do so
	if(ADAQSettings->UseSpectraCalibrations[Channel]){
	  if(SpectraCalibrationType[Channel] == zCalibrationFit)
	    PulseHeight = ADAQSettings->SpectraCalibrations[Channel]->Eval(PulseHeight);
	  else if(SpectraCalibrationType[Channel] == zCalibrationInterp)
	    PulseHeight = ADAQSettings->SpectraCalibrationData[Channel]->Eval(PulseHeight);
	}
	// Pulse area
	
	// Reset the pulse area "integral" to zero
	PulseArea = 0.;
	
	// ...iterate through the bins in the Waveform_H histogram
	// and add each bin value to the pulse area integral. Note
	// the integral is over the entire waveform.
	for(Int_t sample=AnalysisMin; sample<=AnalysisMax; sample++){
	  SampleHeight = Waveform_H[Channel]->GetBinContent(sample);
	  PulseArea += SampleHeight;
	}
	
	// Store the uncalibrated pulse height in the designated vector
	SpectrumPAVec[Channel].push_back(PulseArea);

	// If the calibration manager is to be used to convert the
	// value from pulse units [ADC] to energy units [keV, MeV,
	// ...] then do so
	if(ADAQSettings->UseSpectraCalibrations[Channel]){
	  if(SpectraCalibrationType[Channel] == zCalibrationFit)
	    PulseArea = ADAQSettings->SpectraCalibrations[Channel]->Eval(PulseArea);
	  else if(SpectraCalibrationType[Channel] == zCalibrationInterp)
	    PulseArea = ADAQSettings->SpectraCalibrationData[Channel]->Eval(PulseArea);
	}

	// Initial spectra creation
	
	// Add the pulse value to the spectrum object depending on
	// type of spectrum that is to be created initially

	if(ADAQSettings->ADAQSpectrumTypePHS){
	  if(PulseHeight > ADAQSettings->SpectrumMinThresh and
	     PulseHeight < ADAQSettings->SpectrumMaxThresh){
	    Spectrum_H->Fill(PulseHeight);
	  }
	}
	
	else if(ADAQSettings->ADAQSpectrumTypePAS){
	  if(PulseArea > ADAQSettings->SpectrumMinThresh and
	     PulseArea < ADAQSettings->SpectrumMaxThresh){
	    Spectrum_H->Fill(PulseArea);
	  }
	}
	
	// Note that we must add a +1 to the waveform number in order to
	// get the modulo to land on the correct intervals
	if(IsMaster)
	  // Check to ensure no floating point exception for low number
	  if(WaveformEnd >= 50)
	    if((waveform+1) % int(WaveformEnd*ADAQSettings->UpdateFreq*1.0/100) == 0)
	      UpdateProcessingProgress(waveform);
      }
      
      
      //////////////////////////////////
      // Peak-finder waveform processing
    
      // If the peak-finding/limit-finding algorithm is to be used to
      // create the pulse spectrum ...
      else if(ADAQSettings->ADAQSpectrumAlgorithmPF){
	
	// ...pass the Waveform_H[Channel] TH1F object to the peak-finding
	// algorithm, passing 'false' as the second argument to turn off
	// plotting of the waveforms. ADAQAnalysisInterface::FindPeaks() will
	// fill up a vector<PeakInfoStruct> that will be used to either
	// integrate the valid peaks to create a PAS or find the peak
	// heights to create a PHS, returning true. If zero peaks are
	// found in the waveform then FindPeaks() returns false
	PeaksFound = FindPeaks(Waveform_H[Channel], zPeakFinder);

	// Because the peak finding algorithm skips analysis of
	// waveforms for which it cannot find peaks, we need to update
	// the progress bar here to ensure that bar ends at 100% when
	// actual processing ends. It would be nice to do this more
	// accurately in the future (i.e. update the bar when processing
	// actually finished!). Note that we must add a +1 to the
	// waveform number in order to get the modulo to land on the
	// correct intervals
	if(IsMaster)
	  if(WaveformEnd >= 50)
	    if((waveform+1) % int(WaveformEnd*ADAQSettings->UpdateFreq*1.0/100) == 0)
	      UpdateProcessingProgress(waveform);

	// If no peaks are present in the current waveform then continue
	// on to the next waveform for analysis
	if(!PeaksFound)
	  continue;

	// Calculate the PSD integrals and determine if they pass
	// the pulse-shape filterthrough the pulse-shape filter
	if(UsePSDRegions[ADAQSettings->WaveformChannel])
	  CalculatePSDIntegrals(false);
	
	// Find both pulse area and peak heights during processing so
	// that the values can be added to the spectrum vectors
	IntegratePeaks();
	FindPeakHeights();
      }
    }
  
    // Make final updates to the progress bar, ensuring that it reaches
    // 100% and changes color to acknoqledge that processing is complete

    if(SequentialArchitecture){
      ProcessingProgressBar->Increment(100);
      ProcessingProgressBar->SetBarColor(ColorManager->Number2Pixel(32));
      ProcessingProgressBar->SetForegroundColor(ColorManager->Number2Pixel(0));
    }
  
#ifdef MPI_ENABLED

    // Parallel waveform processing is complete at this point, and it
    // is time to aggregate all the processed results from all nodes
    // to the master, which is responsible for saving the aggregated
    // results to disk in a ROOT TFile. From disk, the running
    // sequential ADAQAnalysis process will open the TFile and extract
    // the aggregated results for plotting, analysis, etc (For
    // details, see AAComputaiton::ProcessWaveformsInParallel).

    /////////////////////
    // Impose MPI barrier

    // Ensure that all nodes have reached the end-of-processing!

    if(ParallelVerbose)
      cout << "\nADAQAnalysis_MPI Node[" << MPI_Rank << "] : Reached the end-of-processing MPI barrier!"
	   << endl;

    MPI::COMM_WORLD.Barrier();

    
    /////////////////////////////////
    // Aggregate TH1F spectra objects

    // In order to aggregate the Spectrum_H objects on each node to a
    // single Spectrum_H object on the master node, the following is
    // performed:
    //
    // 0. Create a double array on each node 
    // 1. Store each node's Spectrum_H bin contents in the array
    // 2. Use MPI::Reduce on the array ptr to quickly aggregate the
    //    array values to the master's array (master == node 0)
    //
    // The size of the array must be (nbins+2) since ROOT TH1 objects
    // that are created with nbins have a underflow (bin = 0) and
    // overflow (bin = nbin+1) bin added onto the bins within range.
  
    const Int_t ArraySize = ADAQSettings->SpectrumNumBins + 2;
    Double_t SpectrumArray[ArraySize];
    for(Int_t i=0; i<ArraySize; i++)
      SpectrumArray[i] = Spectrum_H->GetBinContent(i);
    
    // Get the total number of spectrum entries on the node 
    Double_t Entries = Spectrum_H->GetEntries();
    
    if(ParallelVerbose)
      cout << "\nADAQAnalysis_MPI Node[" << MPI_Rank << "] : Aggregating results to Node[0]!" << endl;

    AAParallel *ParallelMgr = AAParallel::GetInstance();
    
    // Use AAParallel's implementation of MPI::Reduce() to aggregate
    // the spectrum arrays and entries on each node into a single
    // array and total entries on the master node

    Double_t *ReturnArray = ParallelMgr->SumDoubleArrayToMaster(SpectrumArray, ArraySize);
    Double_t ReturnDouble = ParallelMgr->SumDoublesToMaster(Entries);


    ///////////////////////////////////
    // Aggregate spectrum value vectors

    // In order to aggregate the vectors that hold the calculated
    // pulse area and heights from the nodes to the master, the
    // following is performed (for each node unless otherwise noted):
    //
    // 0. The vectors are converted into persistent TVectorT<double>
    //    classes, which as TObject-derived, can be stored in TFiles
    // 1. The TVectorT's are written to temporary TFiles in /tmp
    //
    // 2. The master then reads each node's vector's from each TFile
    //    and aggregates them into master TVectorT's
    //
    // 3. The master TVectorT's are written to the parallel results
    //    TFile, which is accessed later by the sequential binary
    //
    // It's not overwhelmingly elegant, but it's reasonable efficiency
    // and works!
   
    string USER = getenv("USER");
    stringstream SS;
    SS << "/tmp/ParallelProcessing_" << USER << MPI_Rank << ".root";
    TString FName = SS.str();
    
    TFile *VectorWrite = new TFile(FName, "recreate");
    TVectorD PH(SpectrumPHVec[Channel].size(), &SpectrumPHVec[Channel][0]);
    TVectorD PA(SpectrumPAVec[Channel].size(), &SpectrumPAVec[Channel][0]);
    PH.Write("PH");

    PA.Write("PA");
    VectorWrite->Close();

    // The master should output the array to a text file, which will be
    // read in by the running sequential binary of ADAQAnalysisGUI
    if(IsMaster){
      if(ParallelVerbose)
	cout << "\nADAQAnalysis_MPI Node[0] : Writing master TH1F histogram to disk!\n"
	     << endl;
      
      // Create the master TH1F histogram object. Note that the member
      // data for spectrum creation are used to ensure the correct
      // number of bins and bin aranges
      MasterHistogram_H = new TH1F("MasterHistogram","MasterHistogram", 
				   ADAQSettings->SpectrumNumBins,
				   ADAQSettings->SpectrumMinBin, 
				   ADAQSettings->SpectrumMaxBin);
    
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

      // Aggregate each nodes' TVectorT's objects, which stored
      // calculated pulse height/areas for each waveform they
      // processed, aggregate to master TVectorT's, and then write the
      // master TVectorT's to the parallel results file

      vector<Double_t> SpectrumPHVec_Master, SpectrumPAVec_Master;
      
      for(int file=0; file<MPI_Size; file++){
	// Open the node TFile sequentially
	SS.str("");
	SS << "/tmp/ParallelProcessing_" << USER << file << ".root";
	TString FName = SS.str();
	TFile *VectorRead = new TFile(FName, "read");
	
	// Get the VectorT object and readout to class member vectors

	TVectorD *PH = (TVectorD *)VectorRead->Get("PH");
	for(int i=0; i<PH->GetNoElements(); i++)
	  SpectrumPHVec_Master.push_back( (*PH)[i] );
	
	TVectorD *PA = (TVectorD *)VectorRead->Get("PA");
	for(int i=0; i<PA->GetNoElements(); i++)
	  SpectrumPAVec_Master.push_back( (*PA)[i]);
	
	// Close and delete the now-depracated node TFiles
	VectorRead->Close();
	string RemoveFilesCommand;
	RemoveFilesCommand = "rm " + FName + " -f";
	system(RemoveFilesCommand.c_str()); 
      }

      TVectorD MasterPHVec(SpectrumPHVec_Master.size(), &SpectrumPHVec_Master[0]);
      TVectorD MasterPAVec(SpectrumPAVec_Master.size(), &SpectrumPAVec_Master[0]);

      // Open the ROOT file that stores all the parallel processing data
      // in "update" mode such that we can append the master histogram
      ParallelFile = new TFile(ParallelMgr->GetParallelFileName().c_str(), "update");
      
      // Write the master histogram object to the ROOT file ...
      MasterHistogram_H->Write();
      MasterPHVec.Write("MasterPHVec");
      MasterPAVec.Write("MasterPAVec");
      
      // ... and write the ROOT file to disk
      ParallelFile->Write();
      
      // Delete the now-depracated parallel TFiles containing the
      // nodes' SpectrumPH/PAVec object
      string RemoveFilesCommand;
      RemoveFilesCommand = "rm " + FName + " -f";
      system(RemoveFilesCommand.c_str()); 
    }
#endif
    SpectrumExists = true;
  }
}


void AAComputation::CreateSpectrum()
{
  // Delete the previous Spectrum_H TH1F object if it exists to
  // prevent memory leaks
  if(Spectrum_H){
    delete Spectrum_H;
    SpectrumExists = false;
  }

  // Create the TH1F histogram object for spectra creation
  Spectrum_H = new TH1F("Spectrum_H", "ADAQ spectrum", 
			ADAQSettings->SpectrumNumBins, 
			ADAQSettings->SpectrumMinBin,
			ADAQSettings->SpectrumMaxBin);

  // Get the current digitizer channel to analyze
  Int_t Channel = ADAQSettings->WaveformChannel;

  vector<Double_t>::iterator It, ItB, ItE;
  if(ADAQSettings->ADAQSpectrumTypePAS){
    It = SpectrumPAVec[Channel].begin();
    ItB = SpectrumPAVec[Channel].begin();
    ItE = SpectrumPAVec[Channel].end();
  }
  else if(ADAQSettings->ADAQSpectrumTypePHS){
    It = SpectrumPHVec[Channel].begin();
    ItB = SpectrumPHVec[Channel].begin();
    ItE = SpectrumPHVec[Channel].end();
  }
  
  for(; It!=ItE; It++){

    // If using SMS or WD algorithms, histogram only the number of
    // waveforms specified by user; note that if using PF algorithm,
    // all pulse heights/areas will be histogrammed regardless of user
    // specifications to account for case of multiple values per
    // waveforms, in which case values>waveforms-specified. This
    // ensures that ALL values found during waveform processing in PF
    // are used in the spectrum histogram.

    if(!ADAQSettings->ADAQSpectrumAlgorithmPF)
      if((It-ItB) > ADAQSettings->WaveformsToHistogram)
	break;

    Double_t Quantity = (*It);

    // Convert the quantity if calibration has been activated
    if(ADAQSettings->UseSpectraCalibrations[Channel]){
      if(SpectraCalibrationType[Channel] == zCalibrationFit)      	  
	Quantity = ADAQSettings->SpectraCalibrations[Channel]->Eval(Quantity);
      else if(SpectraCalibrationType[Channel] == zCalibrationInterp)
	Quantity = ADAQSettings->SpectraCalibrationData[Channel]->Eval(Quantity);
    }
    
    if(Quantity > ADAQSettings->SpectrumMinThresh and
       Quantity < ADAQSettings->SpectrumMaxThresh)
      Spectrum_H->Fill(Quantity);
  }
  SpectrumExists = true;
}


void AAComputation::IntegratePeaks()
{
  // Iterate over each peak stored in the vector of PeakInfoStructs...
  vector<PeakInfoStruct>::iterator it;
  for(it=PeakInfoVec.begin(); it!=PeakInfoVec.end(); it++){
    
    // If pileup rejection is begin used, examine the pileup flag
    // stored in each PeakInfoStruct to determine whether or not this
    // peak is part of a pileup events. If so, skip it...
    //if(UsePileupRejection_CB->IsDown() and (*it).PileupFlag==true)
    if(ADAQSettings->UsePileupRejection and (*it).PileupFlag==true)
      continue;
    
    // If the PSD filter is desired, examine the PSD filter flag
    // stored in each PeakInfoStruct to determine whether or not this
    // peak should be filtered out of the spectrum.
    if(UsePSDRegions[ADAQSettings->WaveformChannel] and (*it).PSDFilterFlag==true)
      continue;

    // If the peak falls outside the user-specific waveform analysis
    // region then filter this peak out of the spectrum
    if((*it).PeakPosX < ADAQSettings->AnalysisRegionMin or
       (*it).PeakPosX > ADAQSettings->AnalysisRegionMax)
      continue;
    
    // ...and use the lower and upper peak limits to calculate the
    // integral under each waveform peak that has passed all criterion
    Double_t PeakIntegral = Waveform_H[ADAQSettings->WaveformChannel]->Integral((*it).PeakLimit_Lower,
										(*it).PeakLimit_Upper);
    
    Int_t Channel = ADAQSettings->WaveformChannel;
    
    // Add the uncalibrated integral to the spectrum vector
    SpectrumPAVec[Channel].push_back(PeakIntegral);
    
    // If the user has calibrated the spectrum, then transform the
    // peak integral in pulse units [ADC] to energy units
    if(ADAQSettings->UseSpectraCalibrations[Channel]){
      if(SpectraCalibrationType[Channel] == zCalibrationFit)
	PeakIntegral = ADAQSettings->SpectraCalibrations[Channel]->Eval(PeakIntegral);
      else if(SpectraCalibrationType[Channel] == zCalibrationInterp)
	PeakIntegral = ADAQSettings->SpectraCalibrationData[Channel]->Eval(PeakIntegral);
    }
    
    // Add the integral to the spectrum if a pulse area spectrum is
    // desired to create the initial post-processing histogram
    if(ADAQSettings->ADAQSpectrumTypePAS){
      if(PeakIntegral > ADAQSettings->SpectrumMinThresh and
	 PeakIntegral < ADAQSettings->SpectrumMaxThresh){
	Spectrum_H->Fill(PeakIntegral);
      }
    }
  }
}


void AAComputation::FindPeakHeights()
{
  // Iterate over each peak stored in the vector of PeakInfoStructs...
  vector<PeakInfoStruct>::iterator it;
  for(it=PeakInfoVec.begin(); it!=PeakInfoVec.end(); it++){

    // If pileup rejection is begin used, examine the pileup flag
    // stored in each PeakInfoStruct to determine whether or not this
    // peak is part of a pileup events. If so, skip it...
    if(ADAQSettings->UsePileupRejection and (*it).PileupFlag==true)
      continue;

    // If the PSD filter is desired, examine the PSD filter flag
    // stored in each PeakInfoStruct to determine whether or not this
    // peak should be filtered out of the spectrum.
    if(UsePSDRegions[ADAQSettings->WaveformChannel] and (*it).PSDFilterFlag==true)
      continue;

    // If the peak falls outside the user-specific waveform analysis
    // region then filter this peak out of the spectrum
    if((*it).PeakPosX < ADAQSettings->AnalysisRegionMin or
       (*it).PeakPosX > ADAQSettings->AnalysisRegionMax)
      continue;
    
    // Initialize the peak height for each peak region
    Double_t PeakHeight = 0.;

    // Get the waveform processing channel number
    Int_t Channel = ADAQSettings->WaveformChannel;

    // Iterate over the samples between lower and upper integration
    // limits to determine the maximum peak height
    for(Int_t sample=(*it).PeakLimit_Lower; sample<(*it).PeakLimit_Upper; sample++){
      if(Waveform_H[Channel]->GetBinContent(sample) > PeakHeight)
	PeakHeight = Waveform_H[Channel]->GetBinContent(sample);
    }

    // Add the uncalibrated peak height to the spectrum vector
    SpectrumPHVec[Channel].push_back(PeakHeight);
    
    // If the user has calibrated the spectrum then transform the peak
    // heights in pulse units [ADC] to energy
    if(ADAQSettings->UseSpectraCalibrations[Channel]){
      if(SpectraCalibrationType[Channel] == zCalibrationFit)
	PeakHeight = ADAQSettings->SpectraCalibrations[Channel]->Eval(PeakHeight);
      else if(SpectraCalibrationType[Channel] == zCalibrationInterp)
	PeakHeight = ADAQSettings->SpectraCalibrationData[Channel]->Eval(PeakHeight);
    }
    
    // Add the integral to the spectrum if a pulse area spectrum is
    // desired to create the initial post-processing histogram
    if(ADAQSettings->ADAQSpectrumTypePHS){
      if(PeakHeight > ADAQSettings->SpectrumMinThresh and
	 PeakHeight < ADAQSettings->SpectrumMaxThresh){
	Spectrum_H->Fill(PeakHeight);
      }
    }
  }
}


void AAComputation::UpdateProcessingProgress(int Waveform)
{
#ifndef MPI_ENABLED
  if(Waveform > 0)
    ProcessingProgressBar->Increment(ADAQSettings->UpdateFreq);
#else
  if(Waveform == 0)
    cout << "\n\n"; 
  else
    cout << "\rADAQAnalysis_MPI Node[0] : Estimated progress = " 
	 << setprecision(2)
	 << (Waveform*100./WaveformEnd) << "%"
	 << "       "
	 << flush;
#endif
}


// Method to compute a background of a TH1F object representing a
// detector pulse height / energy spectrum.
void AAComputation::CalculateSpectrumBackground()//TH1F *Spectrum_H)
{
  // raw = original spectrum for which background is calculated 
  // background = background component of raw spectrum
  // deconvolved = raw spectrum less background spectrum


  // Clone the Spectrum_H object and give the clone a unique name
  TH1F *SpectrumClone_H = (TH1F *)Spectrum_H->Clone("SpectrumClone_H");
  
  // Set the range of the Spectrum_H clone to correspond to the range
  // that the user has specified to calculate the background over
  SpectrumClone_H->GetXaxis()->SetRangeUser(ADAQSettings->BackgroundMinBin,
					    ADAQSettings->BackgroundMaxBin);
  
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
  if(SpectrumBackground_H){
    delete SpectrumBackground_H;
    SpectrumBackgroundExists = false;
  }

  // Use the TSpectrum::Background() method to compute the spectrum
  // background; Set the resulting TH1F objects graphic attributes

  int Iterations = ADAQSettings->BackgroundIterations;

  string Compton = "";
  if(ADAQSettings->BackgroundCompton)
    Compton = " Compton ";

  string Smoothing = " nosmoothing ";
  if(ADAQSettings->BackgroundSmoothing)
    Smoothing = "";

  string Direction;
  if(ADAQSettings->BackgroundDirection == 0)
    Direction = " BackIncreasingWindow ";
  else if(ADAQSettings->BackgroundDirection == 1)
    Direction = " BackDecreasingWindow ";

  stringstream ss;

  string FilterOrder;
  ss << " BackOrder" << ADAQSettings->BackgroundFilterOrder << " ";
  FilterOrder = ss.str();

  ss.str("");

  string SmoothingWidth;
  ss << " BackSmoothing" << ADAQSettings->BackgroundSmoothingWidth << " ";
  SmoothingWidth = ss.str();

  string BackgroundOptionsString = Compton + Smoothing + Direction + FilterOrder + SmoothingWidth;

  SpectrumBackground_H = (TH1F *)PeakFinder->Background(SpectrumClone_H, 
							Iterations,
							BackgroundOptionsString.c_str());
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
  SpectrumBackground_H->SetEntries(SpectrumBackground_H->Integral(0, ADAQSettings->SpectrumNumBins+1));
  
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
				   ADAQSettings->SpectrumNumBins, 
				   ADAQSettings->SpectrumMinBin, 
				   ADAQSettings->SpectrumMaxBin);
  SpectrumDeconvolved_H->SetLineColor(4);
  SpectrumDeconvolved_H->SetLineWidth(2);

  // Create an object to hold the sum of the squares of the bin
  // weights is created and calculated (i.e. error will be propogated
  // during the background subtraction into the deconvolved spectrum)
  SpectrumDeconvolved_H->Sumw2();
  
  SpectrumDeconvolved_H->SetLineColor(kBlue);
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
  SpectrumDeconvolved_H->SetEntries(SpectrumDeconvolved_H->Integral(ADAQSettings->SpectrumMinBin,
								    ADAQSettings->SpectrumMaxBin+1));
  
  // Set the bool that flags whether or not the SpectrumBackground_H
  // object is available
  SpectrumBackgroundExists = true;
}


// Method used to integrate a pulse / energy spectrum
void AAComputation::IntegrateSpectrum()
{
  // Get the spectrum binning min/max/total range
  double Min = ADAQSettings->SpectrumMinBin;
  double Max = ADAQSettings->SpectrumMaxBin;
  double Range = Max-Min;

  // Get the spectrum integration slider fractional position 
  double LowerFraction = ADAQSettings->SpectrumIntegrationMin;
  double UpperFraction = ADAQSettings->SpectrumIntegrationMax;

  // Compute the integration range, ensuring to account for case where
  // the lowest spectrum bin is non-zero.
  double LowerIntLimit = LowerFraction * Range + Min;
  double UpperIntLimit = UpperFraction * Range + Min;

  /*
  double XAxisMin = Spectrum_H->GetXaxis()->GetXmax() * ADAQSettings->XAxisMin;
  double XAxisMax = Spectrum_H->GetXaxis()->GetXmax() * ADAQSettings->XAxisMax;

  double LowerIntLimit = (ADAQSettings->SpectrumIntegrationMin * XAxisMax) + XAxisMin;
  double UpperIntLimit = ADAQSettings->SpectrumIntegrationMax * XAxisMax;
  */

  // Check to ensure that the lower limit line is always LESS than the
  // upper limit line
  if(UpperIntLimit < LowerIntLimit)
    UpperIntLimit = LowerIntLimit+1;

  // Clone the appropriate spectrum object depending on user's
  // selection into a new TH1F object for integration

  if(SpectrumIntegral_H)
    delete SpectrumIntegral_H;

  if(ADAQSettings->PlotLessBackground)
    SpectrumIntegral_H = (TH1F *)SpectrumDeconvolved_H->Clone("SpectrumToIntegrate_H");
  else
    SpectrumIntegral_H = (TH1F *)Spectrum_H->Clone("SpectrumToIntegrate_H");
  
  // Set the integration TH1F object attributes and draw it
  SpectrumIntegral_H->SetLineColor(4);
  SpectrumIntegral_H->SetLineWidth(2);
  SpectrumIntegral_H->SetFillColor(2);
  SpectrumIntegral_H->SetFillStyle(3001);
  SpectrumIntegral_H->GetXaxis()->SetRangeUser(LowerIntLimit, UpperIntLimit);
  
  // Variable to hold the result of the spectrum integral
  //  double Int = 0.;
  
  // Variable to hold the result of the error in the spectrum integral
  // calculation. Integration error is simply the square root of the
  // sum of the squares of the integration spectrum bin contents. The
  // error is computed with a ROOT TH1 method for robustness.
  //double Err = 0.;
  
  string IntegralArg = "width";
  if(ADAQSettings->SpectrumIntegralInCounts)
    IntegralArg.assign("");

  
  ///////////////////////////////
  // Gaussian fitting/integration

  if(ADAQSettings->SpectrumUseGaussianFit)
    FitSpectrum();

  
  ////////////////////////
  // Histogram integration
  
  else{
    // Set the low and upper bin for the integration
    int StartBin = SpectrumIntegral_H->FindBin(LowerIntLimit);
    int StopBin = SpectrumIntegral_H->FindBin(UpperIntLimit);

    // Ensure that the underflow bin (== 0) is included if the lowest
    // valid bin (==1) is within the integration range
    if(StartBin == 1)
      StartBin = 0;
    
    // Compute the integral and error
    SpectrumIntegralValue = SpectrumIntegral_H->IntegralAndError(StartBin,
								 StopBin,
								 SpectrumIntegralError,
								 IntegralArg.c_str());
  }
}


void AAComputation::FitSpectrum()
{
  /*
  double XAxisMin = Spectrum_H->GetXaxis()->GetXmax() * ADAQSettings->XAxisMin;
  double XAxisMax = Spectrum_H->GetXaxis()->GetXmax() * ADAQSettings->XAxisMax;

  double LowerIntLimit = (ADAQSettings->SpectrumIntegrationMin * XAxisMax) + XAxisMin;
  double UpperIntLimit = ADAQSettings->SpectrumIntegrationMax * XAxisMax;
  */

  // Get the spectrum binning min/max/total range
  double Min = ADAQSettings->SpectrumMinBin;
  double Max = ADAQSettings->SpectrumMaxBin;
  double Range = Max-Min;

  // Get the spectrum integration slider fractional position 
  double LowerFraction = ADAQSettings->SpectrumIntegrationMin;
  double UpperFraction = ADAQSettings->SpectrumIntegrationMax;

  // Compute the integration range, ensuring to account for case where
  // the lowest spectrum bin is non-zero.
  double LowerIntLimit = LowerFraction * Range + Min;
  double UpperIntLimit = UpperFraction * Range + Min;

  
  // Check to ensure that the lower limit line is always LESS than the
  // upper limit line
  if(UpperIntLimit < LowerIntLimit)
    UpperIntLimit = LowerIntLimit+1;

  string IntegralArg = "width";
  if(ADAQSettings->SpectrumIntegralInCounts)
    IntegralArg.assign("");
  
  if(SpectrumFit_F)
    delete SpectrumFit_F;
  
  // Create a gaussian fit between the lower/upper limits; fit the
  // gaussian to the histogram representing the integral region
  // new TH1F object for analysis
  SpectrumFit_F = new TF1("PeakFit", "gaus", LowerIntLimit, UpperIntLimit);
  SpectrumIntegral_H->Fit("PeakFit","R N Q");
  
  // Project the gaussian fit into a histogram with identical
  // binning to the original spectrum to make analysis easier
  TH1F *SpectrumFit_H = (TH1F *)SpectrumIntegral_H->Clone("SpectrumFit_H");
  SpectrumFit_H->Eval(SpectrumFit_F);
  
  // Compute the integral and error between the lower/upper limits
  // of the histogram that resulted from the gaussian fit
  SpectrumIntegralValue = SpectrumFit_H->IntegralAndError(SpectrumFit_H->FindBin(LowerIntLimit),
							  SpectrumFit_H->FindBin(UpperIntLimit),
							  SpectrumIntegralError,
							  IntegralArg.c_str());

  TF1 *F_u = new TF1("F_u", "gaus", LowerIntLimit, UpperIntLimit);
  F_u->SetParameter(0, SpectrumFit_F->GetParameter(0) + SpectrumFit_F->GetParError(0));
  F_u->SetParameter(1, SpectrumFit_F->GetParameter(1) + SpectrumFit_F->GetParError(1));
  F_u->SetParameter(2, SpectrumFit_F->GetParameter(2) + SpectrumFit_F->GetParError(2));
  Double_t UpperErrorIntegral = F_u->Integral(LowerIntLimit, UpperIntLimit);

  TF1 *F_l = new TF1("F_l", "gaus", LowerIntLimit, UpperIntLimit);
  F_l->SetParameter(0, SpectrumFit_F->GetParameter(0) - SpectrumFit_F->GetParError(0));
  F_l->SetParameter(1, SpectrumFit_F->GetParameter(1) - SpectrumFit_F->GetParError(1));
  F_l->SetParameter(2, SpectrumFit_F->GetParameter(2) - SpectrumFit_F->GetParError(2));
  Double_t LowerErrorIntegral = F_l->Integral(LowerIntLimit, UpperIntLimit);
  
  // Take the average of the upper and lower errors
  Double_t AverageError = (UpperErrorIntegral + LowerErrorIntegral) / 2;

  // Get the histogram bin width
  Double_t BinWidth = Range / ADAQSettings->SpectrumNumBins;

  // Set the new error
  // SpectrumIntegralError = fabs(SpectrumIntegralValue - AverageError/BinWidth);
  
  // Draw the gaussian peak fit
  SpectrumFit_F->SetLineColor(kGreen+2);
  SpectrumFit_F->SetLineWidth(3);
}


Bool_t AAComputation::WriteSpectrumFitResultsFile(string FName)
{
  // Get the spectrum fit parameters
  
  Double_t Const = SpectrumFit_F->GetParameter(0);
  Double_t ConstErr = SpectrumFit_F->GetParError(0);

  Double_t Mean = SpectrumFit_F->GetParameter(1);
  Double_t MeanErr = SpectrumFit_F->GetParError(1);
  
  Double_t Sigma = SpectrumFit_F->GetParameter(2);
  Double_t SigmaErr = SpectrumFit_F->GetParError(2);

  Double_t Res = 2.35 * Sigma / Mean * 100;
  Double_t ResErr = Res * sqrt(pow(SigmaErr/Sigma,2) + pow(MeanErr/Mean,2));

  // Get the present time/date

  time_t Time = chrono::system_clock::to_time_t(chrono::system_clock::now());

  // Output to file
    
  ofstream Out(FName.c_str(), ofstream::trunc);

  Out << "# File name : " << FName << "\n"
      << "# File date : " << ctime(&Time)
      << "# File desc : " << "Gaussian fit parameters and energy resolution with absolute error\n"
      << "# ADAQ file : " << ADAQFileName << "\n"
      << "\n"
      << setw(10) << "Constant:" << setw(10) << Const << setw(10) << ConstErr << "\n"
      << setw(10) << "Mean:" << setw(10) << Mean << setw(10) << MeanErr << "\n"
      << setw(10) << "Sigma:" << setw(10) << Sigma << setw(10) << SigmaErr << "\n"
      << setw(10) << "Res:" << setw(10) << Res << setw(10) << ResErr << "\n"
      << endl;

  return true;
}


// Method used to output a generic TH1 object to a data text file in
// the format column1 == bin center, column2 == bin content. Note that
// the function accepts class types TH1 such that any derived class
// (TH1F, TH1D ...) can be saved with this function
bool AAComputation::SaveHistogramData(string Type, string FileName, string FileExtension)
{
  TH1F *HistogramToSave_H1 = NULL;
  TH2F *HistogramToSave_H2 = NULL;
  
  if(Type == "Waveform")
    HistogramToSave_H1 = Waveform_H[ADAQSettings->WaveformChannel];
  else if(Type == "Spectrum")
    HistogramToSave_H1 = Spectrum_H;
  else if(Type == "SpectrumBackground")
    HistogramToSave_H1 = SpectrumBackground_H;
  else if(Type == "SpectrumDerivative")
    HistogramToSave_H1 = SpectrumDerivative_H;
  else if(Type == "PSDHistogram")
    HistogramToSave_H2 = PSDHistogram_H;
  else if(Type == "PSDHistogramSlice")
    HistogramToSave_H1 = (TH1F *)PSDHistogramSlice_H;
  
  if(FileExtension == ".dat" or FileExtension == ".csv"){
    
    string FullFileName = FileName + FileExtension;
    
    ofstream HistogramOutput(FullFileName.c_str(), ofstream::trunc);
    
    // Assign the data separator based on file extension
    string separator;
    if(FileExtension == ".dat")
      separator = "\t";
    else if(FileExtension == ".csv")
      separator = ",";

    int NumBins = HistogramToSave_H1->GetNbinsX();
    
    for(int bin=0; bin<=NumBins; bin++)
      HistogramOutput << HistogramToSave_H1->GetBinCenter(bin) << separator
		      << HistogramToSave_H1->GetBinContent(bin)
		      << endl;
    
    HistogramOutput.close();

    return true;
  }
  else if(FileExtension == ".root"){
    
    string FullFileName = FileName + FileExtension;
    
    TFile *HistogramOutput = new TFile(FullFileName.c_str(), "recreate");
    
    if(Type == "Waveform")
      HistogramToSave_H1->Write("Waveform");
    else if(Type == "PSDHistogram")
      HistogramToSave_H2->Write("PSDHistogram");
    else
      HistogramToSave_H1->Write("Spectrum");
    
    HistogramOutput->Close();
    
    return true;
  }
  else{
    return false;
  }
}


TH2F *AAComputation::ProcessPSDHistogramWaveforms()
{
  if(PSDHistogramExists){
    delete PSDHistogram_H;
    PSDHistogramExists = false;
  }
  
  if(SequentialArchitecture){
    ProcessingProgressBar->Reset();
    ProcessingProgressBar->SetBarColor(ColorManager->Number2Pixel(33));
    ProcessingProgressBar->SetForegroundColor(ColorManager->Number2Pixel(1));
  }
  
  // Create a 2-dimensional histogram to store the PSD integrals
  PSDHistogram_H = new TH2F("PSDHistogram_H","PSDHistogram_H", 
			    ADAQSettings->PSDNumTotalBins, 
			    ADAQSettings->PSDMinTotalBin,
			    ADAQSettings->PSDMaxTotalBin,
			    ADAQSettings->PSDNumTailBins,
			    ADAQSettings->PSDMinTailBin,
			    ADAQSettings->PSDMaxTailBin);
  
  Int_t Channel = ADAQSettings->WaveformChannel;

  Double_t TotalIntegral = 0.;
  Double_t TailIntegral = 0.;

  // Clear the PSD integrals vectors to zero elements. I have chosen
  // *not* to preallocate memory intentionally since (a)
  // vector::push_back() is easily sufficiently fast compared to
  // preallocation for our purposes and (b) the PF algorithm, which
  // can find multiple values per pulse and therefore does not have a
  // fixed vector length, makes preallocation difficult.
  PSDHistogramTotalVec[Channel].clear();
  PSDHistogramTailVec[Channel].clear();


  ////////////////////////////////////////////////////////
  // Create the PSD histogram from stored waveform data //
  ////////////////////////////////////////////////////////
  
  if(ADAQSettings->PSDAlgorithmWD){

    ////////////////////////////////////////////
    // Check to ensure that waveform data exists

    // Using waveform data is not available for legacy ADAQ files
    if(ADAQLegacyFileLoaded){
      PSDHistogramExists = false;
      return PSDHistogram_H;
    }
    
    // Ensure the user chose to save PSD waveform data
    if(!ARI->GetStorePSDData()){
      PSDHistogramExists = false;
      return PSDHistogram_H;
    }

    ///////////////////////////////////////////////////
    // Readout the waveform data into the PSD histogram
    
    // Set the ADAQWaveformData branch name depending on channel number
    stringstream SS;
    SS << "WaveformDataCh" << Channel;
    string WDName = SS.str();

    // Create and set address of ADAQWavefomData object in the waveform tree
    ADAQWaveformData *WD = new ADAQWaveformData;

    // Clone the ADAQWaveformTree for use to prevent TTree memory
    // modifications that cause seg fault when PSD mode is switched
    // from waveform data back to other PSD modes
    TTree *CloneTree = (TTree *)ADAQWaveformTree->Clone("CloneTree");
    CloneTree->SetBranchAddress(WDName.c_str(), &WD);
    
    // Readout appropriate waveform data into the spectrum
    for(Int_t entry=0; entry<CloneTree->GetEntries(); entry++){
      
      // Ensure only the specified number of events are processed
      if(entry == ADAQSettings->PSDWaveformsToDiscriminate)
	break;

      // Get the entry
      CloneTree->GetEntry(entry);
      
      // Get the stored total and tail PSD integrals
      TotalIntegral = WD->GetPSDTotalIntegral();
      TailIntegral = WD->GetPSDTailIntegral();

      // ...and also storem them in vectors for later use
      PSDHistogramTotalVec[Channel].push_back(TotalIntegral);
      PSDHistogramTailVec[Channel].push_back(TailIntegral);

      // If the user wants to plot (Tail integral / Total integral) on
      // the y-axis of the PSD histogram then modify the TailIntegral:
      if(ADAQSettings->PSDYAxisTailTotal)
	TailIntegral /= TotalIntegral;

      // If the user wants to plot the X-axis (PSD total integral) in
      // energy [MeVee] then use the spectra calibrations
      if(ADAQSettings->PSDXAxisEnergy and UseSpectraCalibrations[Channel]){
	if(SpectraCalibrationType[Channel] == zCalibrationFit)
	  TotalIntegral = ADAQSettings->SpectraCalibrations[Channel]->Eval(TotalIntegral);
	else if(SpectraCalibrationType[Channel] == zCalibrationInterp)
	  TotalIntegral = ADAQSettings->SpectraCalibrationData[Channel]->Eval(TotalIntegral);
      }
      
      // Determine if waveform exceeds the PSD threshold
      if(TotalIntegral > ADAQSettings->PSDThreshold){
	
	// If the user has enabled a PSD filter ...
	if(UsePSDRegions[ADAQSettings->WaveformChannel]){
	  
	  // Determine whether to accept/exclude the event
	  if(!ApplyPSDRegion(TotalIntegral, TailIntegral))
	    PSDHistogram_H->Fill(TotalIntegral, TailIntegral);
	}
	
	// ... otherwise straight PSD histogramming
	else
	  PSDHistogram_H->Fill(TotalIntegral, TailIntegral);
      }
    }

    // Memory clean up
    delete CloneTree;
    delete WD;
    
    PSDHistogramExists = true;
  }
  
  ////////////////////////////////////////////////////////
  // Create the PSD histogram from processing waveforms //
  ////////////////////////////////////////////////////////
  
  else{
    
    // Reboot the PeakFinder with up-to-date max peaks
    if(PeakFinder) delete PeakFinder;
    PeakFinder = new TSpectrum(ADAQSettings->MaxPeaks);
    
    // See documention in either ::ProcessSpectrumWaveforms() or
    // ::CreateDesplicedFile for parallel processing assignemnts
    
    WaveformStart = 0;
    WaveformEnd = ADAQSettings->PSDWaveformsToDiscriminate;
    
#ifdef MPI_ENABLED
    
    Int_t SlaveEvents = Int_t(ADAQSettings->PSDWaveformsToDiscriminate/MPI_Size);
    Int_t MasterEvents = Int_t(ADAQSettings->PSDWaveformsToDiscriminate-SlaveEvents*(MPI_Size-1));
    
    if(ParallelVerbose and IsMaster)
      cout << "\nADAQAnalysis_MPI Node[0] : Number waveforms allocated to master (node == 0) : " << MasterEvents << "\n"
	   <<   "                           Number waveforms allocated to slaves (node != 0) : " << SlaveEvents
	   << endl;
  
    // Divide up the total number of waveforms to be processed amongst
    // the master and slaves as evenly as possible. Note that the
    // 'WaveformStart' value is *included* whereas the 'WaveformEnd'
    // value _excluded_ from the for loop range 
    WaveformStart = (MPI_Rank * SlaveEvents) + (ADAQSettings->WaveformsToHistogram % MPI_Size);
    WaveformEnd =  (MPI_Rank * SlaveEvents) + MasterEvents;

    // The master _always_ starts on waveform zero. This is required
    // with the waveform allocation algorithm abov.
    if(IsMaster)
      WaveformStart = 0;
  
    if(ParallelVerbose)
      cout << "\nADAQAnalysis_MPI Node[" << MPI_Rank << "] : Handling waveforms " << WaveformStart << " to " << (WaveformEnd-1)
	   << endl;
#endif
    
    Bool_t PeaksFound = false;
    
    for(Int_t waveform=WaveformStart; waveform<WaveformEnd; waveform++){
      if(SequentialArchitecture)
	gSystem->ProcessEvents();

      ADAQWaveformTree->GetEntry(waveform);

      RawVoltage = *Waveforms[Channel];
    
      if(ADAQSettings->RawWaveform or ADAQSettings->BSWaveform)
	CalculateBSWaveform(Channel, waveform);
      else if(ADAQSettings->ZSWaveform)
	CalculateZSWaveform(Channel, waveform);
    
      // Find the peaks and peak limits in the current waveform. The
      // second argument ('true') indicates the find peaks calculation
      // is being performed for PSD and thus the radio button settings
      // for PSD 'peak finder' or 'whole waveform' should be used to
      // decide which peak finding algorithm to use
      if(ADAQSettings->PSDAlgorithmPF)
	PeaksFound = FindPeaks(Waveform_H[Channel], zPeakFinder);
      else if(ADAQSettings->PSDAlgorithmSMS)
	PeaksFound = FindPeaks(Waveform_H[Channel], zWholeWaveform);

      // Update the user with progress here because the peak finding
      // algorithm can skip waveform for which it doesn't find a peak,
      // which can throw off the progress bar progress.
      if(IsMaster)
	if((waveform+1) % int(WaveformEnd*ADAQSettings->UpdateFreq*1.0/100) == 0)
	  UpdateProcessingProgress(waveform);
    
      // If not peaks are present in the current waveform then continue
      // onto the next waveform to optimize CPU $.
      if(!PeaksFound)
	continue;
    
      // Calculate the "total" and "tail" integrals of each
      // peak. Because we want to create a PSD histogram, pass "true" to
      // the function to indicate the results should be histogrammed
      CalculatePSDIntegrals(true);
    }
  
    if(SequentialArchitecture){
      ProcessingProgressBar->Increment(100);
      ProcessingProgressBar->SetBarColor(ColorManager->Number2Pixel(32));
      ProcessingProgressBar->SetForegroundColor(ColorManager->Number2Pixel(0));
    }

#ifdef MPI_ENABLED

    if(ParallelVerbose)
      cout << "\nADAQAnalysis_MPI Node[" << MPI_Rank << "] : Reached the end-of-processing MPI barrier!"
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
    const Int_t ArraySizeX = PSDHistogram_H->GetNbinsX() + 2;
    const Int_t ArraySizeY = PSDHistogram_H->GetNbinsY() + 2;
    Double_t DoubleArray[ArraySizeX][ArraySizeY];

    // Iterate over the PSDHistogram_H columns...
    for(Int_t i=0; i<ArraySizeX; i++){
      
      // A container for the PSDHistogram_H's present column
      vector<Double_t> ColumnVector(ArraySizeY, 0.);

      // Assign the PSDHistogram_H's column to the vector
      for(Int_t j=0; j<ArraySizeY; j++)
	ColumnVector[j] = PSDHistogram_H->GetBinContent(i,j);
      
      // Reduce the array representing the column
      Double_t *ReturnArray = AAParallel::GetInstance()->SumDoubleArrayToMaster(&ColumnVector[0], ArraySizeY);
      
      // Assign the array to the DoubleArray that represents the
      // "master" or total PSDHistogram_H object
      for(Int_t j=0; j<ArraySizeY; j++)
	DoubleArray[i][j] = ReturnArray[j];
    }
    
    // Aggregated the histogram entries from all nodes to the master
    double Entries = PSDHistogram_H->GetEntries();
    double ReturnDouble = AAParallel::GetInstance()->SumDoublesToMaster(Entries);
    
    ///////////////////////////////////////////
    // Aggregate PSD histogram integral vectors

    // The following procedure is identical to that performed for
    // aggregated the spectrum pulse value vectors. See description in
    // AAComputation::ProcessSpectrumWaveforms()

    string USER = getenv("USER");
    stringstream SS;
    SS << "/tmp/ParallelProcessing_" << USER << MPI_Rank << ".root";
    TString FName = SS.str();
    
    TFile *VectorWrite = new TFile(FName, "recreate");
    TVectorD PT(PSDHistogramTotalVec[Channel].size(),
		&PSDHistogramTotalVec[Channel][0]);
    
    TVectorD PP(PSDHistogramTailVec[Channel].size(),
		&PSDHistogramTailVec[Channel][0]);
    
    PT.Write("PT");
    PP.Write("PP");

    VectorWrite->Close();
  
    if(IsMaster){
    
      if(ParallelVerbose)
	cout << "\nADAQAnalysis_MPI Node[0] : Writing master PSD TH2F histogram to disk!\n"
	     << endl;

      // Create the master PSDHistogram_H object, i.e. the sum of all
      // the PSDHistogram_H object values from the nodes
      MasterPSDHistogram_H = new TH2F("MasterPSDHistogram_H","MasterPSDHistogram_H",
				      ADAQSettings->PSDNumTotalBins, 
				      ADAQSettings->PSDMinTotalBin,
				      ADAQSettings->PSDMaxTotalBin,
				      ADAQSettings->PSDNumTailBins,
				      ADAQSettings->PSDMinTailBin,
				      ADAQSettings->PSDMaxTailBin);

      // Assign each bin content in the master PSD histogram from the
      // double array containing the aggregated slave values
      for(Int_t i=0; i<ArraySizeX; i++)
	for(Int_t j=0; j<ArraySizeY; j++)
	  MasterPSDHistogram_H->SetBinContent(i, j, DoubleArray[i][j]);
      
      // Assign the total number of entries in the master PSD histogram
      MasterPSDHistogram_H->SetEntries(ReturnDouble);

      vector<Double_t> PSDHistogramTotalVec_Master, PSDHistogramTailVec_Master;
      
      for(int file=0; file<MPI_Size; file++){
	// Open the node TFile sequentially
	SS.str("");
	SS << "/tmp/ParallelProcessing_" << USER << file << ".root";
	TString FName = SS.str();
	TFile *VectorRead = new TFile(FName, "read");
	
	// Get the VectorT object and readout to class member vectors

	TVectorD *PT = (TVectorD *)VectorRead->Get("PT");
	for(int i=0; i<PT->GetNoElements(); i++)
	  PSDHistogramTotalVec_Master.push_back( (*PT)[i] );
	
	TVectorD *PP = (TVectorD *)VectorRead->Get("PP");
	for(int i=0; i<PP->GetNoElements(); i++)
	  PSDHistogramTailVec_Master.push_back( (*PP)[i] );
	
	// Close and delete the now-depracated node TFiles
	VectorRead->Close();
	string RemoveFilesCommand;
	RemoveFilesCommand = "rm " + FName + " -f";
	system(RemoveFilesCommand.c_str()); 
      }
      
      TVectorD MasterPSDTotalVec(PSDHistogramTotalVec_Master.size(),
				 &PSDHistogramTotalVec_Master[0]);
      
      TVectorD MasterPSDTailVec(PSDHistogramTailVec_Master.size(),
				&PSDHistogramTailVec_Master[0]);
      
      // Open the TFile  and write all the necessary object to it
      ParallelFile = new TFile(AAParallel::GetInstance()->GetParallelFileName().c_str(), "update");
      
      MasterPSDHistogram_H->Write("MasterPSDHistogram");
      MasterPSDTotalVec.Write("MasterPSDTotalVec");
      MasterPSDTailVec.Write("MasterPSDTailVec");
      
      ParallelFile->Write();
    }
#endif
    
    // Update the bool to alert the code that a valid PSDHistogram_H object exists.
    PSDHistogramExists = true;
  }
  return PSDHistogram_H;
}


TH2F *AAComputation::CreatePSDHistogram()
{
  if(PSDHistogram_H){
    delete PSDHistogram_H;
    PSDHistogramExists = false;
  }
  
  PSDHistogram_H = new TH2F("PSDHistogram_H","PSDHistogram_H", 
			    ADAQSettings->PSDNumTotalBins, 
			    ADAQSettings->PSDMinTotalBin,
			    ADAQSettings->PSDMaxTotalBin,
			    ADAQSettings->PSDNumTailBins,
			    ADAQSettings->PSDMinTailBin,
			    ADAQSettings->PSDMaxTailBin);

  Int_t Channel = ADAQSettings->WaveformChannel;

  for(Int_t p=0; p<(Int_t)PSDHistogramTotalVec[Channel].size(); p++){
    
    // If using SMS or WD algorithms, discriminate only the number of
    // waveforms specified by user; note that if using PF algorithm,
    // all pulse heights/areas will be discriminated regardless of user
    // specifications to account for case of multiple values per
    // waveforms, in which case values>waveforms-specified. This
    // ensures that ALL values found during waveform processing in PF
    // are used in the spectrum histogram.

    if(!ADAQSettings->PSDAlgorithmPF)
      if(p >= ADAQSettings->PSDWaveformsToDiscriminate)
	break;
    
    Double_t PSDTotal = PSDHistogramTotalVec[Channel][p];
    Double_t PSDParameter = PSDHistogramTailVec[Channel][p];

    if(ADAQSettings->PSDYAxisTailTotal)
      PSDParameter /= PSDTotal;

    if(ADAQSettings->PSDXAxisEnergy and UseSpectraCalibrations[Channel]){
      if(SpectraCalibrationType[Channel] == zCalibrationFit)
	PSDTotal = ADAQSettings->SpectraCalibrations[Channel]->Eval(PSDTotal);
      else if(SpectraCalibrationType[Channel] == zCalibrationInterp)
	PSDTotal = ADAQSettings->SpectraCalibrationData[Channel]->Eval(PSDTotal);
    }

    // If the PSD total integral exceeds the threshold
    if(PSDTotal > ADAQSettings->PSDThreshold){

      // If the user has selected to use PSD regions
      if(UsePSDRegions[ADAQSettings->WaveformChannel]){
	
	// Determine whether to accept/exclude the event 
	if(!ApplyPSDRegion(PSDTotal, PSDParameter))
	  PSDHistogram_H->Fill(PSDTotal, PSDParameter);
      }
      
      // else just straight fill the PSD histogram
      else
	PSDHistogram_H->Fill(PSDTotal, PSDParameter);
    }
  }
  
  PSDHistogramExists = true;
  
  return PSDHistogram_H;
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
void AAComputation::CalculatePSDIntegrals(Bool_t FillPSDHistogram)
{
  // Get the present channel for analysis
  Int_t Channel = ADAQSettings->WaveformChannel;
  
  // Iterate over each peak stored in the vector of PeakInfoStructs...
  vector<PeakInfoStruct>::iterator it;
  for(it=PeakInfoVec.begin(); it!=PeakInfoVec.end(); it++){
    
    // If the peak falls outside the user-specific waveform analysis
    // region then filter this peak out of the spectrum
    if((*it).PeakPosX < ADAQSettings->AnalysisRegionMin or
       (*it).PeakPosX > ADAQSettings->AnalysisRegionMax)
      continue;

    // Get the peak position in time (x axis)
    Double_t Peak = (*it).PeakPosX;
    
    // Compute the total and tail integral regions
    Double_t TotalStart = Peak + ADAQSettings->PSDTotalStart;
    Double_t TotalStop = Peak + ADAQSettings->PSDTotalStop;
    Double_t TailStart = Peak + ADAQSettings->PSDTailStart;
    Double_t TailStop = Peak + ADAQSettings->PSDTailStop;
    
    // Compute the total integral
    Double_t TotalIntegral = Waveform_H[Channel]->Integral(TotalStart,
							   TotalStop);
    
    // Compute the tail integral
    Double_t TailIntegral = Waveform_H[Channel]->Integral(TailStart,
							  TailStop);
    
    // Store the values in the member data vectors
    PSDHistogramTotalVec[Channel].push_back(TotalIntegral);
    PSDHistogramTailVec[Channel].push_back(TailIntegral);
    
      // If the user wants to plot (Tail integral / Total integral) on
    // the y-axis of the PSD histogram then modify the TailIntegral:
    if(ADAQSettings->PSDYAxisTailTotal)
      TailIntegral /= TotalIntegral;
    
    // If the user wants to plot the X-axis (PSD total integral) in
    // energy [MeVee] then use the spectra calibrations
    
    if(ADAQSettings->PSDXAxisEnergy and ADAQSettings->UseSpectraCalibrations[Channel]){
      if(SpectraCalibrationType[Channel] == zCalibrationFit)
	TotalIntegral = ADAQSettings->SpectraCalibrations[Channel]->Eval(TotalIntegral);
      else if(SpectraCalibrationType[Channel] == zCalibrationInterp)
	TotalIntegral = ADAQSettings->SpectraCalibrationData[Channel]->Eval(TotalIntegral);
    }
    
    // If the user has enabled a PSD filter ...
    if(ADAQSettings->UsePSDRegions[Channel]){
      
      // ... then apply the PSD filter to the waveform. If the
      // waveform does not pass the filter, mark the flag indicating
      // that it should be filtered out due to its pulse shap
      if(ApplyPSDRegion(TotalIntegral, TailIntegral))
	(*it).PSDFilterFlag = true;
    }
    
    // The total integral of the waveform must exceed the PSDThreshold
    // in order to be histogrammed. This allows the user flexibility
    // in eliminating the large numbers of small waveform events.
    if((TotalIntegral > ADAQSettings->PSDThreshold) and FillPSDHistogram){
      if((*it).PSDFilterFlag == false)
	PSDHistogram_H->Fill(TotalIntegral, TailIntegral);
    }
  }
}


// Use the PSD integrals and the user-specified PSD region to
// determine whether waveform should be excluded from the PSD
// histogram. The user can chose to exclude points that are inside or
// outside hte pSD region. The method determines uses the
// TCutG::IsInside() method on the point (PSDTotal, PSDParameter) to
// determine whether to exclude the waveform or not. The return
// convention is:
//   return true  : exclude waveform (fails criterion)
//   return false : accept waveform (passes criterion)
Bool_t AAComputation::ApplyPSDRegion(Double_t PSDTotal,
				     Double_t PSDParameter)
{
  Int_t Channel = ADAQSettings->WaveformChannel;
  
  if(ADAQSettings->PSDInsideRegion and
     ADAQSettings->PSDRegions[Channel]->IsInside(PSDTotal,
						    PSDParameter))
    return false;
  
  else if(ADAQSettings->PSDOutsideRegion and 
	  !ADAQSettings->PSDRegions[Channel]->IsInside(PSDTotal,
							  PSDParameter))
    return false;
  
  else
    return true;
}


// Method to compute the derivative of the pulse spectrum
TGraph *AAComputation::CalculateSpectrumDerivative()
{
  // The discrete spectrum derivative is simply defined as the
  // difference between bin N_i and N_i-1 and then scaled down. The
  // main purpose is more quantitatively examine spectra for peaks,
  // edges, and other features.

  const int NumBins = Spectrum_H->GetNbinsX();
  
  double BinCenters[NumBins], Differences[NumBins];  

  double BinCenterErrors[NumBins], DifferenceErrors[NumBins];

  // Iterate over the bins to assign the bin center and the difference
  // between bins N_i and N_i-1 to the designed arrays
  for(int bin = 0; bin<NumBins; bin++){
    
    // Set the bin center right in between the two bin centers used to
    // calculate the spectrum derivative
    BinCenters[bin] = (Spectrum_H->GetBinCenter(bin) + Spectrum_H->GetBinCenter(bin-1)) / 2;
    
    // Recall that TH1F bin 0 is the underfill bin; therfore, set the
    // difference between bins 0/-1 and 0/1 to zero to account for
    // bins that do not contain relevant content for the derivative
    if(bin < 2){
      Differences[bin] = 0;
      continue;
    }
    
    double Previous = Spectrum_H->GetBinContent(bin-1);
    double Current = Spectrum_H->GetBinContent(bin);

    // Compute the "derivative", i.e. the difference between the
    // current and previous bin contents
    Differences[bin] = (Current - Previous);
    
    if(ADAQSettings->PlotAbsValueSpectrumDerivative)
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
  
  if(SpectrumDerivative_G){
    delete SpectrumDerivative_G;
    SpectrumDerivativeExists = false;
  }
  
  if(ADAQSettings->PlotSpectrumDerivativeError)
    SpectrumDerivative_G = new TGraphErrors(NumBins, BinCenters, Differences, BinCenterErrors, DifferenceErrors);
  else
    SpectrumDerivative_G = new TGraph(NumBins, BinCenters, Differences);

  // Iterate over the TGraph derivative points and assign them to the
  // TH1F object. The main purpose for converting this to a TH1F is so
  // that the generic/modular function SaveHistogramData() can be used
  // to output the spectrum derivative data to a file. We also
  // subtract off the vertical offset used for plotting to ensure the
  // derivative is vertically centered at zero.
  
  if(SpectrumDerivative_H) delete SpectrumDerivative_H;
  SpectrumDerivative_H = new TH1F("SpectrumDerivative_H","SpectrumDerivative_H", 
				  ADAQSettings->SpectrumNumBins, 
				  ADAQSettings->SpectrumMinBin, 
				  ADAQSettings->SpectrumMaxBin);
    
  double x,y;
  for(int bin=0; bin<ADAQSettings->SpectrumNumBins; bin++){
    SpectrumDerivative_G->GetPoint(bin,x,y);
    SpectrumDerivative_H->SetBinContent(bin, y);
  }
  
  SpectrumDerivativeExists = true;

  return SpectrumDerivative_G;
}


void AAComputation::ProcessWaveformsInParallel(string ProcessingType)
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
  // is also used to "transfer" results created in parallel back to
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
  
  string Binary = AAParallel::GetInstance()->GetParallelBinaryName();
  
  // Create a shell command to launch the parallel binary of
  // ADAQAnalysisGUI with the desired number of nodes
  stringstream ss;
  ss << "mpirun -np " << ADAQSettings->NumProcessors << " " << Binary << " " << ProcessingType;
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

  string FileName = AAParallel::GetInstance()->GetParallelFileName();
  
  // Open the parallel processing ROOT file to retrieve the results
  // produced by the parallel processing session
  ParallelFile = new TFile(FileName.c_str(), "read");  
  
  if(ParallelFile->IsOpen()){
    
    ////////////////
    // Histogramming

    if(ProcessingType == "histogramming"){
      
      // Retrieve the master TH1F histogram, which is a sum of all
      // TH1F histograms computed by all MPI nodes 

      Spectrum_H = (TH1F *)ParallelFile->Get("MasterHistogram");
      SpectrumExists = true;

      // Retrieve the master TVectorT<double> objects that contain the
      // pulse height and areas computed by all MPI nodes and use them
      // to fill the class member vectors for later use

      Int_t Channel = ADAQSettings->WaveformChannel;

      TVectorD *MasterPHVec = (TVectorD *)ParallelFile->Get("MasterPHVec");
      SpectrumPHVec[Channel].clear();
      for(int i=0; i<MasterPHVec->GetNoElements(); i++)
	SpectrumPHVec[Channel].push_back( (*MasterPHVec)[i]);

      TVectorD *MasterPAVec = (TVectorD *)ParallelFile->Get("MasterPAVec");
      SpectrumPAVec[Channel].clear();
      for(int i=0; i<MasterPAVec->GetNoElements(); i++)
	SpectrumPAVec[Channel].push_back( (*MasterPAVec)[i]);
    }
    
    
    /////////////
    // Desplicing

    else if(ProcessingType == "desplicing"){
    }	  
    

    /////////////////////////////
    // Pulse shape discriminating 
    
    else if(ProcessingType == "discriminating"){

      PSDHistogram_H = (TH2F *)ParallelFile->Get("MasterPSDHistogram");
      PSDHistogramExists = true;

      // Retrieve the master TVectorD<Double_t> objects that contain
      // the PSD values computed by all MPI nodes and use them to fill
      // the class member vectors for later use

      Int_t Channel = ADAQSettings->WaveformChannel;
      
      TVectorD *MasterPSDTotalVec = (TVectorD *)ParallelFile->Get("MasterPSDTotalVec");
      PSDHistogramTotalVec[Channel].clear();
      for(int i=0; i<MasterPSDTotalVec->GetNoElements(); i++)
	PSDHistogramTotalVec[Channel].push_back( (*MasterPSDTotalVec)[i]);
      
      TVectorD *MasterPSDTailVec = (TVectorD *)ParallelFile->Get("MasterPSDTailVec");
      PSDHistogramTailVec[Channel].clear();
      for(int i=0; i<MasterPSDTailVec->GetNoElements(); i++)
	PSDHistogramTailVec[Channel].push_back( (*MasterPSDTailVec)[i]);
    }
  }
  else
    {/* ErrorMessage */}
  
  
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
  RemoveFilesCommand = "rm " + FileName + " -f";
  system(RemoveFilesCommand.c_str()); 
}


void AAComputation::RejectPileup(TH1F *Histogram_H)
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


Bool_t AAComputation::SetCalibrationPoint(Int_t Channel, Int_t SetPoint,
					  Double_t Energy, Double_t PulseUnit)
{
  // Add a new point to the calibration
  if(SetPoint == (Int_t)CalibrationData[Channel].PointID.size()){

    // Push data into the channel's calibration vectors
    CalibrationData[Channel].PointID.push_back(SetPoint);
    CalibrationData[Channel].Energy.push_back(Energy);
    CalibrationData[Channel].PulseUnit.push_back(PulseUnit);

    // Automatically sort the vectors from lowest-to-highest
    sort(CalibrationData[Channel].Energy.begin(),
	 CalibrationData[Channel].Energy.end());
    
    sort(CalibrationData[Channel].PulseUnit.begin(),
	 CalibrationData[Channel].PulseUnit.end());
    
    return true;
  }
  
  // Overwrite a previous calibration point
  else{
    CalibrationData[Channel].Energy[SetPoint] = Energy;
    CalibrationData[Channel].PulseUnit[SetPoint] = PulseUnit;

    return false;
  }
}


Bool_t AAComputation::SetCalibration(Int_t Channel)
{
  Int_t NumCalibrationPoints = CalibrationData[Channel].PointID.size();

  if(NumCalibrationPoints >= 2){
    
    SpectraCalibrationData[Channel] = new TGraph(NumCalibrationPoints,
						 &CalibrationData[Channel].PulseUnit[0],
						 &CalibrationData[Channel].Energy[0]);
    
    TString FitType;
    if(ADAQSettings->CalibrationType == "Linear fit")
      FitType = "pol1";
    else if(ADAQSettings->CalibrationType == "Quadratic fit")
      FitType = "pol2";
    else if(ADAQSettings->CalibrationType == "Lin. interpolation")
      FitType = "None";

    if(FitType == "pol1" or FitType == "pol2"){
      Double_t FitRangeMin = ADAQSettings->CalibrationMin;
      Double_t FitRangeMax = ADAQSettings->CalibrationMax;
    
      delete SpectraCalibrations[Channel];
      
      SpectraCalibrations[Channel] = new TF1("SpectrumCalibration", 
					     FitType, 
					     FitRangeMin,
					     FitRangeMax);
      
      SpectraCalibrationData[Channel]->Fit("SpectrumCalibration", "RN");

      SpectraCalibrationType[Channel] = zCalibrationFit;
    }
    else
      SpectraCalibrationType[Channel] = zCalibrationInterp;
     
    
    // Set the current channel's calibration boolean to true,
    // indicating that the current channel will convert pulse units
    // to energy within the acquisition loop before histogramming
    // the result into the channel's spectrum
    UseSpectraCalibrations[Channel] = true;
    
    return true;
  }
  else
    return false;
}


bool AAComputation::ClearCalibration(int Channel)
{
  // Clear the channel calibration vectors for the current channel
  CalibrationData[Channel].PointID.clear();
  CalibrationData[Channel].Energy.clear();
  CalibrationData[Channel].PulseUnit.clear();
  
  // Delete the current channel's depracated calibration data TGraph
  // and fit TF1 objects to prevent memory leaks but reallocate the
  // objects to prevent seg. faults when writing the
  // SpectraCalibrations to the ROOT parallel processing file
  if(UseSpectraCalibrations[Channel]){
    delete SpectraCalibrationData[Channel];
    SpectraCalibrationData[Channel] = new TGraph;

    delete SpectraCalibrations[Channel];
    SpectraCalibrations[Channel] = new TF1;
  }
  
  // Set the current channel's calibration boolean to false,
  // indicating that the calibration manager will NOT be used within
  // the acquisition loop
  UseSpectraCalibrations[Channel] = false;

  return true;
}


Bool_t AAComputation::WriteCalibrationFile(Int_t Channel,
					   string FName)
{
  if(!UseSpectraCalibrations[Channel])
    return false;
  else{
    ofstream Out(FName.c_str(), ofstream::trunc);
    for(UInt_t ch=0; ch<CalibrationData[Channel].PointID.size(); ch++)
      Out << setw(10) << CalibrationData[Channel].Energy[ch]
	  << setw(10) << CalibrationData[Channel].PulseUnit[ch]
	  << endl;
    Out.close();
    
    return true;
  }
}


void AAComputation::SetEdgeBound(double X, double Y)
{
  if(gPad->GetLogy())
    Y = pow(10,Y);

  if(EdgeHBound.size() == 0){
    EdgePositionFound = false;

    EdgeHBound.push_back(Y);
    EdgeVBound.push_back(X);
  }
  else if (EdgeHBound.size() == 1){
    EdgeHBound.push_back(Y);
    EdgeVBound.push_back(X);
    
    HalfHeight = (EdgeHBound[0]+EdgeHBound[1])/2;
    
    FindEdge();
    
    EdgeHBound.clear();
    EdgeVBound.clear();
  }
}


void AAComputation::FindEdge()
{
  double MinADC = 0.;
  double MaxADC = 0.;
  if(EdgeVBound[0] < EdgeVBound[1]){
    MinADC = EdgeVBound[0];
    MaxADC = EdgeVBound[1];
  }
  else{
    MinADC = EdgeVBound[1];
    MaxADC = EdgeVBound[0];
  }

  double increment = 1;
  for(double value=MinADC; value<=MaxADC; value+=increment){
    double X0 = value;
    double X1 = value + increment;

    double Y0 = Spectrum_H->Interpolate(X0);
    double Y1 = Spectrum_H->Interpolate(X1);
    
    if((Y0 > HalfHeight) and (Y1 < HalfHeight)){
      double m = (Y1 - Y0) / (X1 - X0);
      double Y = abs(Y0+Y1)/2;

      EdgePosition = (1./m)*(Y-Y0)+X0;
      
      EdgePositionFound = true;

      break;
    }
  }
}


void AAComputation::AddPSDRegionPoint(Int_t XPixel, Int_t YPixel)
{
  // Convert the x and y pixel values into absolute x and y
  // coordinates and then push them into the appropriate vectors. Note
  // that for the PSD region, the absolute x values correspond to the
  // PSD total integral and the absolute y values correspond to the
  // PSD tail integral values. Both are in units of ADC.
  PSDRegionXPoints.push_back(gPad->AbsPixeltoX(XPixel));
  PSDRegionYPoints.push_back(gPad->AbsPixeltoY(YPixel));
}


void AAComputation::CreatePSDRegion()
{
  if(PSDRegionXPoints.size() < 3)
    return;

  Int_t Channel = ADAQSettings->WaveformChannel;
  
  // Ensure that the last point in the vectors is identical to the
  // first point such that the TCutG will be a closed region
  PSDRegionXPoints.push_back(PSDRegionXPoints.at(0));
  PSDRegionYPoints.push_back(PSDRegionYPoints.at(0));
  
  // Delete the previous TCutG object to prevent memory leaks
  if(PSDRegions[Channel] != NULL) 
    delete PSDRegions[Channel];
  
  // Create a new TCutG object representing a PSD region
  PSDRegions[Channel] = new TCutG("PSDRegion", PSDRegionXPoints.size(), &PSDRegionXPoints[0], &PSDRegionYPoints[0]);
}


void AAComputation::ClearPSDRegion()
{
  PSDRegionXPoints.clear();
  PSDRegionYPoints.clear();
  
  Int_t Channel = ADAQSettings->WaveformChannel;
  
  if(PSDRegions[Channel] != NULL)
    delete PSDRegions[Channel];
  
  PSDRegions[Channel] = new TCutG;
  UsePSDRegions[ADAQSettings->WaveformChannel] = false;
}


void AAComputation::CreateDesplicedFile()
{
  ////////////////////////////
  // Prepare for processing //
  ////////////////////////////
  
  // Reset the progres bar if binary is sequential architecture
  
  if(SequentialArchitecture){
    ProcessingProgressBar->Reset();
    ProcessingProgressBar->SetBarColor(ColorManager->Number2Pixel(33));
    ProcessingProgressBar->SetForegroundColor(ColorManager->Number2Pixel(1));
  }
  
  
  if(PeakFinder) delete PeakFinder;
  PeakFinder = new TSpectrum(ADAQSettings->MaxPeaks);
  
  ////////////////////////////////////
  // Assign waveform processing ranges
  
  // Set the range of waveforms that will be processed. Note that in
  // sequential architecture if N waveforms are to be histogrammed,
  // waveforms from waveform_ID == 0 to waveform_ID ==
  // (WaveformsToDesplice-1) will be included in the despliced file
  WaveformStart = 0; // Start (and include this waveform in despliced file)
  WaveformEnd = ADAQSettings->WaveformsToDesplice; // End (Up to but NOT including this waveform)
  
#ifdef MPI_ENABLED

  // If the waveform processing is to be done in parallel then
  // distribute the events as evenly as possible between the master
  // (rank == 0) and the slaves (rank != 0) to maximize computational
  // efficiency. Note that the master will carry the small leftover
  // waveforms from the even division of labor to the slaves.
  
  // Assign the number of waveforms processed by each slave
  int SlaveEvents = int(ADAQSettings->WaveformsToDesplice/MPI_Size);
  
  // Assign the number of waveforms processed by the master
  int MasterEvents = int(ADAQSettings->WaveformsToDesplice-SlaveEvents*(MPI_Size-1));
  
  if(ParallelVerbose and IsMaster)
    cout << "\nADAQAnalysis_MPI Node[0] : Number waveforms allocated to master (node == 0) : " << MasterEvents << "\n"
         <<   "                           Number waveforms allocated to slaves (node != 0) : " << SlaveEvents
	 << endl;

  // Divide up the total number of waveforms to be processed amongst
  // the master and slaves as evenly as possible. Note that the
  // 'WaveformStart' value is *included* whereas the 'WaveformEnd'
  // value _excluded_ from the for loop range 
  WaveformStart = (MPI_Rank * SlaveEvents) + (ADAQSettings->WaveformsToHistogram % MPI_Size);
  WaveformEnd = (MPI_Rank * SlaveEvents) + MasterEvents;
  
  // The master _always_ starts on waveform zero. This is required
  // with the waveform allocation algorithm abov.
  if(IsMaster)
    WaveformStart = 0;
  
  if(ParallelVerbose)
    cout << "\nADAQAnalysis_MPI Node[" << MPI_Rank << "] : Handling waveforms " << WaveformStart << " to " << (WaveformEnd-1)
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
    F = new TFile(ADAQSettings->DesplicedFileName.c_str(), "recreate");

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
  MP->RecordLength = ADAQSettings->DesplicedWaveformLength;
  
  // Create a new TObjString object representing the measurement
  // commend. This feature is currently unimplemented.
  TObjString *MC = new TObjString("");

  // Create a new class that will store the results calculated in
  // parallel persistently in the new despliced ROOT file
  AAParallelResults *PR = new AAParallelResults;


  // When the new despliced TFile object(s) that will RECEIVE the
  // despliced waveforms was(were) created above, that(those) TFile(s)
  // became the current TFile (or "directory") for ROOT. In order to
  // desplice the original waveforms, we must move back to the TFile
  // (or "directory) that contains the original waveforms UPON WHICH
  // we want to operate. Recall that TFiles should be though of as
  // unix-like directories that we can move to/from...it's a bit
  // confusing but that's ROOT.
  ADAQFile->cd();


  ///////////////////////
  // Process waveforms //
  ///////////////////////

  bool PeaksFound = false;

  int Channel = ADAQSettings->WaveformChannel;
  
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
    RawVoltage = *Waveforms[Channel];

    // Select the type of Waveform_H object to create
    if(ADAQSettings->RawWaveform or ADAQSettings->BSWaveform)
      CalculateBSWaveform(Channel, waveform);
    else if(ADAQSettings->ZSWaveform)
      CalculateZSWaveform(Channel, waveform);
    

    ///////////////////////////
    // Find peaks and peak data
    
    // Find the peaks (and peak data) in the current waveform.
    // Function returns "true" ("false) if peaks are found (not found).
    PeaksFound = FindPeaks(Waveform_H[Channel], zPeakFinder);
    
    // If no peaks found, continue to next waveform to save CPU $
    if(!PeaksFound)
      continue;
    
    // Calculate the PSD integrals and determine if they pass through
    // the pulse-shape filter 
    if(UsePSDRegions[ADAQSettings->WaveformChannel])
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

      // A crude filter to prevent imposter peaks (e.g. TSpectrum
      // finds a peak in the noise or in a badly distorted or
      // saturated waveform)
      int WaveformWidth = (*peak_iter).PeakLimit_Upper - (*peak_iter).PeakLimit_Lower;
      if(WaveformWidth < 10)
	continue;

      for(int sample=(*peak_iter).PeakLimit_Lower; sample<(*peak_iter).PeakLimit_Upper; sample++){
	VoltageInADC_AllChannels[0].push_back(Waveform_H[Channel]->GetBinContent(sample));;
	index++;
      }

      // Add a buffer (of length DesplicedWaveformBuffer in [samples]) of
      // zeros to the beginning and end of the despliced waveform to
      // aid future waveform processing
      
      VoltageInADC_AllChannels[0].insert(VoltageInADC_AllChannels[0].begin(),
					 ADAQSettings->DesplicedWaveformBuffer,
					 0);
      
      VoltageInADC_AllChannels[0].insert(VoltageInADC_AllChannels[0].end(),
					 ADAQSettings->DesplicedWaveformBuffer,
					 0);
      
      // Fill the TTree with this despliced waveform provided that the
      // current peak is not flagged as a piled up pulse nor flagged
      // as a pulse to be filtered out by pulse shape
      if((*peak_iter).PileupFlag == false and (*peak_iter).PSDFilterFlag == false)
	T->Fill();
    }
    
    // Update the user with progress
    if(IsMaster)
      if((waveform+1) % int(WaveformEnd*ADAQSettings->UpdateFreq*1.0/100) == 0)
	UpdateProcessingProgress(waveform);
  }
  
  // Make final updates to the progress bar, ensuring that it reaches
  // 100% and changes color to acknoqledge that processing is complete
  if(SequentialArchitecture){
    ProcessingProgressBar->Increment(100);
    ProcessingProgressBar->SetBarColor(ColorManager->Number2Pixel(32));
    ProcessingProgressBar->SetForegroundColor(ColorManager->Number2Pixel(0));
  }
  
#ifdef MPI_ENABLED
  
  if(ParallelVerbose)
    cout << "\nADAQAnalysis_MPI Node[" << MPI_Rank << "] : Reached the end-of-processing MPI barrier!"
	 << endl;

  MPI::COMM_WORLD.Barrier();
  
  if(IsMaster)
    cout << "\nADAQAnalysis_MPI Node[0] : Waveform processing complete!\n"
	 << endl;
  
  #endif
  
  ///////////////////////////////////////////////////////
  // Finish processing and creation of the despliced file

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

  // Switch back to the ADAQFile TFile directory
  ADAQFile->cd();
  
  // Note: if in parallel architecture then at this point a series of
  // despliced TFiles now exists in the /tmp directory with each TFile
  // containing despliced waveforms for the range of original
  // waveforms allocated to each node.

#ifdef MPI_ENABLED

  /////////////////////////////////////////////////
  // Aggregrate parallel TFiles into a single TFile

  // Ensure all nodes are at the same point before aggregating files
  MPI::COMM_WORLD.Barrier();

  // If the present node is the master then use a TChain object to
  // merge the despliced waveform TTrees contained in each of the
  // TFiles created by each node 
  if(IsMaster){

  if(IsMaster and ParallelVerbose)
    cout << "\nADAQAnalysis_MPI Node[" << MPI_Rank << "] : Beginning aggregation of ROOT TFiles ..."
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
    WaveformTreeChain->Merge(ADAQSettings->DesplicedFileName.c_str());
    
    // Open the final despliced TFile, write the measurement
    // parameters and comment objects to it, and close the TFile.
    TFile *F_Final = new TFile(ADAQSettings->DesplicedFileName.c_str(), "update");
    MP->Write("MeasParams");
    MC->Write("MeasComment");
    PR->Write("ParResults");
    F_Final->Close();
    
    cout << "\nADAQAnalysis_MPI Node[" << MPI_Rank << "] : Finished aggregation of ROOT TFiles!"
	 << endl;  
    
    cout << "\nADAQAnalysis_MPI Node[" << MPI_Rank << "] : Finished production of despliced file!\n"
	 << endl;  
  }
#endif
}


// Method used to find peaks in a pulse / energy spectrum
void AAComputation::FindSpectrumPeaks()
{
  /*
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
  */
}


void AAComputation::CreatePSDHistogramSlice(int XPixel, int YPixel)
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

  if(ADAQSettings->PSDXSlice){
    int XPixelOld = gPad->GetUniqueID();
    
    gVirtualX->DrawLine(XPixelOld, PixelYMin, XPixelOld, PixelYMax);
    gVirtualX->DrawLine(XPixel, PixelYMin, XPixel, PixelYMax);
    
    gPad->SetUniqueID(XPixel);
  }
  else if(ADAQSettings->PSDYSlice){

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


  // An integer to determine which bin in the TH2F PSDHistogram_H
  // object should be sliced
  int PSDSliceBin = -1;

  // The 1D histogram slice object
  TH1D *PSDSlice_H = 0;

  string HistogramTitle, XAxisTitle;

  // Create a slice at a specific "X" (total pulse integral) value,
  // i.e. create a 1D histogram of the "Y" (tail pulse integrals)
  // values at a specific value of "X" (total pulse integral).
  if(ADAQSettings->PSDXSlice){
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

  // Save the histogram  to a class member TH1F object

  if(PSDHistogramSliceExists)
    delete PSDHistogramSlice_H;
  
  PSDHistogramSlice_H = (TH1D *)PSDSlice_H->Clone("PSDHistogramSlice_H");
  PSDHistogramSliceExists = true;
}


void AAComputation::CreateASIMSpectrum()
{
  if(SpectrumExists){
    delete Spectrum_H;
    SpectrumExists = false;
  }
  
  Spectrum_H = new TH1F("Spectrum_H", "ADAQ Simulation (ASIM) Spectrum",
			ADAQSettings->SpectrumNumBins,
			ADAQSettings->SpectrumMinBin,
			ADAQSettings->SpectrumMaxBin);
  
  // Get the name of the ASIM event tree to be analyzed as specified
  // by the associated combo box setting.
  TString ASIMEventTreeName = ADAQSettings->ASIMEventTreeName;
  TTree *ASIMEventTree = (TTree *)ASIMEventTreeList->FindObject(ASIMEventTreeName);
  
  // Bail out if the TTree cannot be found!
  if(ASIMEventTree == NULL){
    cout << "Warning: The TTree named '" << ASIMEventTreeName << "' cannot be found!\n"
	 << endl;
    return;
  }
  
  // Set the branch address of the ASIM event
  ASIMEventTree->SetBranchAddress("ASIMEventBranch", &ASIMEvt);
  
  // When the user selected an ASIM EventTree via the combo box, the
  // ADAQSettings::WaveformsToHistogram NEL is updated to reflect the
  // total number of events contained within the TTree. This enables
  // the user to select a smaller number than the total entries via
  // this value without exceeding the maximum
  int MaxEntriesToPlot = ADAQSettings->WaveformsToHistogram;

  for(int entry=0; entry<MaxEntriesToPlot; entry++){
    
    ASIMEventTree->GetEvent(entry);

    double Quantity = 0.;
    if(ADAQSettings->ASIMSpectrumTypeEnergy)
      Quantity = ASIMEvt->GetEnergyDep();
    else if(ADAQSettings->ASIMSpectrumTypePhotonsCreated)
      Quantity = ASIMEvt->GetPhotonsCreated();
    else if(ADAQSettings->ASIMSpectrumTypePhotonsDetected)
      Quantity = ASIMEvt->GetPhotonsDetected();

    if(ADAQSettings->UseSpectraCalibrations[ADAQSettings->WaveformChannel]){
      if(SpectraCalibrationType[ADAQSettings->WaveformChannel] == zCalibrationFit)
	Quantity = SpectraCalibrations[ADAQSettings->WaveformChannel]->Eval(Quantity);
      else if(SpectraCalibrationType[ADAQSettings->WaveformChannel] == zCalibrationInterp)
	Quantity = SpectraCalibrationData[ADAQSettings->WaveformChannel]->Eval(Quantity);
    }

    if(Quantity > ADAQSettings->SpectrumMinThresh and
       Quantity < ADAQSettings->SpectrumMaxThresh)
      Spectrum_H->Fill(Quantity);
  }
  SpectrumExists = true;
}


void AAComputation::AnalyzeWaveform(TH1F *Histogram_H)
{
  WaveformAnalysisHeight = Histogram_H->GetBinContent(Histogram_H->GetMaximumBin());
  
  WaveformAnalysisArea = 0.;
  for(int sample=0; sample<Histogram_H->GetEntries(); sample++){
    double PulseHeight = Histogram_H->GetBinContent(sample);
    WaveformAnalysisArea += PulseHeight;
  }
}
