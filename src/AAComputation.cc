#include <TSystem.h>
#include <TError.h>
#include <TF1.h>
#include <TVector.h>
#include <TChain.h>

#ifdef MPI_ENABLED
#include <mpi.h>
#endif

#include <boost/array.hpp>
#include <boost/thread.hpp>

#include "AAComputation.hh"
#include "AAParallel.hh"
#include "AAConstants.hh"

#include <iostream>
#include <fstream>
using namespace std;


AAComputation *AAComputation::TheComputationManager = 0;


AAComputation *AAComputation::GetInstance()
{ return TheComputationManager; }


AAComputation::AAComputation(string CmdLineArg, bool PA)
  : SequentialArchitecture(!PA), ParallelArchitecture(PA),
    ADAQFileLoaded(false), ACRONYMFileLoaded(false), 
    ADAQParResults(NULL), ADAQParResultsLoaded(false),
    Time(0), RawVoltage(0), RecordLength(0),
    PeakFinder(new TSpectrum), NumPeaks(0), PeakInfoVec(0), PearsonIntegralValue(0),
    PeakIntegral_LowerLimit(0), PeakIntegral_UpperLimit(0), PeakLimits(0),
    WaveformStart(0), WaveformEnd(0),
    Spectrum_H(new TH1F), SpectrumBackground_H(new TH1F), SpectrumDeconvolved_H(new TH1F),
    MPI_Size(1), MPI_Rank(0), IsMaster(true), IsSlave(false), 
    Verbose(false), ParallelVerbose(true),
    Baseline(0.), PSDFilterPolarity(1.),
    V1720MaximumBit(4095), NumDataChannels(8),
    SpectrumExists(false), SpectrumBackgroundExists(false), SpectrumDerivativeExists(false), 
    PSDHistogramExists(false), PSDHistogramSliceExists(false),
    TotalPeaks(0), DeuteronsInWaveform(0.), DeuteronsInTotal(0.)
{
  if(TheComputationManager){
    cout << "\nERROR! TheComputationManager was constructed twice!\n" << endl;
    exit(-42);
  }
  TheComputationManager = this;

  // Initialize the objects used in the calibration and pulse shape
  // discrimination (PSD) scheme. A set of 8 calibration and 8 PSD
  // filter "managers" -- really, TGraph *'s used for interpolating --
  // along with the corresponding booleans that determine whether or
  // not that channel's "manager" should be used is initialized. 
  for(int ch=0; ch<NumDataChannels; ch++){
    // All 8 channel's managers are set "off" by default
    UseSpectraCalibrations.push_back(false);
    UsePSDFilters.push_back(false);
    
    // Store empty TGraph pointers in std::vectors to hold space and
    // prevent seg. faults later when we test/delete unused objects
    SpectraCalibrations.push_back(new TGraph);
    PSDFilters.push_back(new TGraph);
    
    // Assign a blank initial structure for channel calibration data
    ADAQChannelCalibrationData Init;
    CalibrationData.push_back(Init);
    
    Waveform_H[ch] = new TH1F;
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
    else{
      // Load the specified ADAQ ROOT file
      LoadADAQRootFile(ADAQSettings->ADAQFileName);
    }
    
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


bool AAComputation::LoadADAQRootFile(string FileName)
{
  //////////////////////////////////
  // Open the specified ROOT file //
  //////////////////////////////////

  ADAQFileName = FileName;
  
  // Open the specified ROOT file 
  ADAQRootFile = new TFile(FileName.c_str(), "read");
  
  // If a valid ADAQ ROOT file was NOT opened...
  if(!ADAQRootFile->IsOpen()){
    ADAQFileLoaded = false;
  }
  else{
    
    /////////////////////////////////////
    // Extract data from the ROOT file //
    /////////////////////////////////////

    // Get the ADAQRootMeasParams objects stored in the ROOT file
    ADAQMeasParams = (ADAQRootMeasParams *)ADAQRootFile->Get("MeasParams");

    // Get the TTree with waveforms stored in the ROOT file
    ADAQWaveformTree = (TTree *)ADAQRootFile->Get("WaveformTree");

    // Seg fault protection against valid ROOT files that are missing
    // the essential ADAQ objects.
    if(ADAQMeasParams == NULL || ADAQWaveformTree == NULL){
      ADAQFileLoaded = false;
      return false;
    }

    // Attempt to get the class containing results that were calculated
    // in a parallel run of ADAQAnalysisGUI and stored persistently
    // along with data in the ROOT file. At present, this is only the
    // despliced waveform files. For standard ADAQ ROOT files, the
    // ADAQParResults class member will be a NULL pointer
    ADAQParResults = dynamic_cast<AAParallelResults *>(ADAQRootFile->Get("ParResults"));

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
      DeuteronsInTotal = ADAQParResults->DeuteronsInTotal;
      
      if(Verbose)
	cout << "Total RFQ current from despliced file: " << ADAQParResults->DeuteronsInTotal << endl;
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
    // Update the bool that determines if a valid ROOT file is loaded
    ADAQFileLoaded = true;
  }
  return ADAQFileLoaded;
}

    
bool AAComputation::LoadACRONYMRootFile(string FileName)
{
  ACRONYMFileName = FileName;

  TFile *ACRONYMRootFile = new TFile(FileName.c_str(), "read");
  
  if(!ACRONYMRootFile->IsOpen()){
    ACRONYMFileLoaded = false;
  }
  else{

    //////////////
    // LS Detector 
    
    if(LSDetectorTree) delete LSDetectorTree;
    LSDetectorTree = dynamic_cast<TTree *>(ACRONYMRootFile->Get("LSDetectorTree"));

    if(!LSDetectorTree){
      cout << "\nERROR! Could not find the LSDetectorTree in the ACRONYM ROOT file!\n"
	   << endl;
    }
    else{
      //      if(LSDetectorEvent) delete LSDetectorEvent;
      LSDetectorEvent = new acroEvent;
      LSDetectorTree->SetBranchAddress("LSDetectorEvents", &LSDetectorEvent);
    }
    
    //////////////
    // ES Detector

    if(ESDetectorTree) delete ESDetectorTree;
    ESDetectorTree = dynamic_cast<TTree *>(ACRONYMRootFile->Get("ESDetectorTree"));
    
    if(!ESDetectorTree){
      cout << "\nERROR! Could not find the ESDetectorTree in the ACRONYM ROOT file!\n"
	   << endl;
    }
    else{
      //      if(ESDetectorEvent) delete ESDetectorEvent;
      ESDetectorEvent = new acroEvent;
      ESDetectorTree->SetBranchAddress("ESDetectorEvents", &ESDetectorEvent);
    }

    //////////////
    // Run Summary
    
    if(RunSummary) delete RunSummary;
    RunSummary = dynamic_cast<acroRun *>(ACRONYMRootFile->Get("RunSummary"));



    ACRONYMFileLoaded = true;
  }

  return ACRONYMFileLoaded;
}


TH1F *AAComputation::CalculateRawWaveform(int Channel, int Waveform)
{
  if(Waveform_H[Channel])
    delete Waveform_H[Channel];
  Waveform_H[Channel] = new TH1F("Waveform_H", "Raw Waveform", RecordLength-1, 0, RecordLength);
  
  ADAQWaveformTree->GetEntry(Waveform);

  vector<int> RawVoltage = *WaveformVecPtrs[Channel];
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
// the waveform baseline. Note that the function argument bool is used
// to determine which polarity (a standard (detector) waveform or a
// Pearson (RFQ current) waveform) should be used in processing
TH1F* AAComputation::CalculateBSWaveform(int Channel, int Waveform, bool CurrentWaveform)
{
  if(Waveform_H[Channel])
    delete Waveform_H[Channel];
  Waveform_H[Channel] = new TH1F("Waveform_H","Baseline-subtracted Waveform", RecordLength-1, 0, RecordLength);

  ADAQWaveformTree->GetEntry(Waveform);
  
  double Polarity;
  (CurrentWaveform) ? Polarity = ADAQSettings->PearsonPolarity : Polarity = ADAQSettings->WaveformPolarity;
  
  vector<int> RawVoltage = *WaveformVecPtrs[Channel];

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
// member for later use.  Note that the function argument bool is used
// to determine which polarity (a standard (detector) waveform or a
// Pearson (RFQ current) waveform) should be used in processing
TH1F *AAComputation::CalculateZSWaveform(int Channel, int Waveform, bool CurrentWaveform)
{
  ADAQWaveformTree->GetEntry(Waveform);
  
  double Polarity;
  (CurrentWaveform) ? Polarity = ADAQSettings->PearsonPolarity : Polarity = ADAQSettings->WaveformPolarity;
  
  vector<int> RawVoltage = *WaveformVecPtrs[Channel];
  
  if(Waveform_H[Channel])
    delete Waveform_H[Channel];

  
  if(RawVoltage.empty()){
    Waveform_H[Channel] = new TH1F("Waveform_H","Zero Suppression Waveform", RecordLength-1, 0, RecordLength);
  }
  else{
    Baseline = CalculateBaseline(&RawVoltage);
    
    vector<double> ZSVoltage;
    vector<int> ZSPosition;
    
    vector<int>::iterator it;
    for(it=RawVoltage.begin(); it!=RawVoltage.end(); it++){
      
      double VoltageMinusBaseline = Polarity*(*it-Baseline);
      
      if(VoltageMinusBaseline >= ADAQSettings->ZeroSuppressionCeiling){
	ZSVoltage.push_back(VoltageMinusBaseline);
	ZSPosition.push_back(it - RawVoltage.begin());
      }
    }
  
    int ZSWaveformSize = ZSVoltage.size();
    Waveform_H[Channel] = new TH1F("Waveform_H","Zero Suppression Waveform", ZSWaveformSize-1, 0, ZSWaveformSize);
  
    vector<double>::iterator iter;
    for(iter=ZSVoltage.begin(); iter!=ZSVoltage.end(); iter++)
      Waveform_H[Channel]->SetBinContent((iter-ZSVoltage.begin()), *iter);
  }
  return Waveform_H[Channel];
}


// Method to calculate the baseline of a waveform. The "baseline" is
// simply the average of the waveform voltage taken over the specified
// range in time (in units of samples)
double AAComputation::CalculateBaseline(vector<int> *Waveform)
{
  int BaselineCalcLength = ADAQSettings->BaselineCalcMax - ADAQSettings->BaselineCalcMin;
  double Baseline = 0.;
  for(int sample=ADAQSettings->BaselineCalcMin; sample<ADAQSettings->BaselineCalcMax; sample++)
    Baseline += ((*Waveform)[sample]*1.0/BaselineCalcLength);
  
  return Baseline;
}


double AAComputation::CalculateBaseline(TH1F *Waveform)
{
  int BaselineCalcLength = ADAQSettings->BaselineCalcMax - ADAQSettings->BaselineCalcMin;
  double Baseline = 0.;
  for(int sample = 0; sample < ADAQSettings->BaselineCalcMax; sample++)
    Baseline += (Waveform->GetBinContent(sample)*1.0/BaselineCalcLength);
      
  return Baseline;
}


// Method used to find peaks in any TH1F object.  
bool AAComputation::FindPeaks(TH1F *Histogram_H)
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

  string PeakFinderOptions = "goff nodraw";
  if(!ADAQSettings->UseMarkovSmoothing)
    PeakFinderOptions += " noMarkov";
  
  int NumPotentialPeaks = PeakFinder->Search(Histogram_H,				    
					     ADAQSettings->Sigma,
					     PeakFinderOptions.c_str(),
					     ADAQSettings->Resolution);

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


void AAComputation::CreateSpectrum()
{
  // Delete the previous Spectrum_H TH1F object if it exists to
  // prevent memory leaks
  if(Spectrum_H){
    delete Spectrum_H;
    SpectrumExists = false;
  }
  
  // Reset the waveform progress bar
  if(SequentialArchitecture){
    ProcessingProgressBar->Reset();
    ProcessingProgressBar->SetBarColor(ColorManager->Number2Pixel(41));
    ProcessingProgressBar->SetForegroundColor(ColorManager->Number2Pixel(1));
  }
  
  // Create the TH1F histogram object for spectra creation
  Spectrum_H = new TH1F("Spectrum_H", "ADAQ spectrum", 
			ADAQSettings->SpectrumNumBins, 
			ADAQSettings->SpectrumMinBin,
			ADAQSettings->SpectrumMaxBin);

  int Channel = ADAQSettings->WaveformChannel;
  
  
  // Variables for calculating pulse height and area
  double SampleHeight, PulseHeight, PulseArea;
  SampleHeight = PulseHeight = PulseArea = 0.;
  
  // Reset the variable holding accumulated RFQ current if the value
  // was not loaded directly from an ADAQ ROOT file created during
  // parallel processing (i.e. a "despliced" file) and will be
  // calculated from scratch here
  if(!ADAQParResultsLoaded)
    DeuteronsInTotal = 0.;
  
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
    cout << "\nADAQAnalysis_MPI Node[0]: Waveforms allocated to slaves (node != 0) = " << SlaveEvents << "\n"
	 <<   "                          Waveforms alloced to master (node == 0) =  " << MasterEvents
	 << endl;
  
  // Assign each master/slave a range of the total waveforms to
  // process based on the MPI rank. Note that WaveformEnd holds the
  // waveform_ID that the loop runs up to but does NOT include
  WaveformStart = (MPI_Rank * MasterEvents); // Start (include this waveform in the final histogram)
  WaveformEnd =  (MPI_Rank * MasterEvents) + SlaveEvents; // End (Up to but NOT including this waveform)
  
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

    int Channel = ADAQSettings->WaveformChannel;
    
    // Assign the raw waveform voltage to a class member vector<int>
    RawVoltage = *WaveformVecPtrs[Channel];
    
    // Calculate the selected waveform that will be analyzed into the
    // spectrum histogram. Note that "raw" waveforms may not be
    // analyzed (simply due to how the code is presently setup) and
    // will default to analyzing the baseline subtracted
    if(ADAQSettings->RawWaveform or ADAQSettings->BSWaveform)
      CalculateBSWaveform(Channel, waveform);
    else if(ADAQSettings->ZSWaveform)
      CalculateZSWaveform(Channel, waveform);
    
    // Calculate the total RFQ current for this waveform.
    if(ADAQSettings->IntegratePearson)
      IntegratePearsonWaveform(waveform);

    
    ///////////////////////////////
    // Whole waveform processing //
    ///////////////////////////////

    // If the entire waveform is to be used to calculate a pulse spectrum ...
    if(ADAQSettings->ADAQSpectrumIntTypeWW){
      
      // If a pulse height spectrum (PHS) is to be created ...
      if(ADAQSettings->ADAQSpectrumTypePHS){
	// Get the pulse height by find the maximum bin value stored
	// in the Waveform_H TH1F via class methods. Note that spectra
	// are always created with positive polarity waveforms
	PulseHeight = Waveform_H[Channel]->GetBinContent(Waveform_H[Channel]->GetMaximumBin());

	// If the calibration manager is to be used to convert the
	// value from pulse units [ADC] to energy units [keV, MeV,
	// ...] then do so
	if(ADAQSettings->UseSpectraCalibrations[Channel])
	  PulseHeight = ADAQSettings->SpectraCalibrations[Channel]->Eval(PulseHeight);
	
	// Add the pulse height to the spectrum histogram	
	Spectrum_H->Fill(PulseHeight);
      }

      // ... or if a pulse area spectrum (PAS) is to be created ...
      else if(ADAQSettings->ADAQSpectrumTypePAS){

	// Reset the pulse area "integral" to zero
	PulseArea = 0.;
	
	// ...iterate through the bins in the Waveform_H histogram and
	// add each bin value to the pulse area integral.
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
	  PulseArea += SampleHeight;
	}
	
	// If the calibration manager is to be used to convert the
	// value from pulse units [ADC] to energy units [keV, MeV,
	// ...] then do so
	if(ADAQSettings->UseSpectraCalibrations[Channel])
	  PulseArea = ADAQSettings->SpectraCalibrations[Channel]->Eval(PulseArea);
	
	// Add the pulse area to the spectrum histogram
	Spectrum_H->Fill(PulseArea);
      }
      
      // Note that we must add a +1 to the waveform number in order to
      // get the modulo to land on the correct intervals
      if(IsMaster)
	// Check to ensure no floating point exception for low number
	if(WaveformEnd >= 50)
	  if((waveform+1) % int(WaveformEnd*ADAQSettings->UpdateFreq*1.0/100) == 0)
	    UpdateProcessingProgress(waveform);
    }
    
    /////////////////////////////////////
    // Peak-finder waveform processing //
    /////////////////////////////////////
    
    // If the peak-finding/limit-finding algorithm is to be used to
    // create the pulse spectrum ...
    else if(ADAQSettings->ADAQSpectrumIntTypePF){
      
      // ...pass the Waveform_H[Channel] TH1F object to the peak-finding
      // algorithm, passing 'false' as the second argument to turn off
      // plotting of the waveforms. ADAQAnalysisInterface::FindPeaks() will
      // fill up a vector<PeakInfoStruct> that will be used to either
      // integrate the valid peaks to create a PAS or find the peak
      // heights to create a PHS, returning true. If zero peaks are
      // found in the waveform then FindPeaks() returns false
      PeaksFound = FindPeaks(Waveform_H[Channel]);
      
      // If no peaks are present in the current waveform then continue
      // on to the next waveform for analysis
      if(!PeaksFound)
	continue;
      
      // Calculate the PSD integrals and determine if they pass
      // the pulse-shape filterthrough the pulse-shape filter
      if(ADAQSettings->UsePSDFilters[ADAQSettings->PSDChannel])
      	CalculatePSDIntegrals(false);
      
      // If a PAS is to be created ...
      if(ADAQSettings->ADAQSpectrumTypePAS)
	IntegratePeaks();
      
      // or if a PHS is to be created ...
      else if(ADAQSettings->ADAQSpectrumTypePHS)
	FindPeakHeights();

      // Note that we must add a +1 to the waveform number in order to
      // get the modulo to land on the correct intervals
      if(IsMaster)
	if(WaveformEnd >= 50)
	  if((waveform+1) % int(WaveformEnd*ADAQSettings->UpdateFreq*1.0/100) == 0)
	    UpdateProcessingProgress(waveform);
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
  // Parallel waveform processing is complete at this point and it is
  // time to aggregate all the results from the nodes to the master
  // (node == 0) for output to a text file (for now). In the near
  // future, we will most likely persistently store a TH1F master
  // histogram object in a ROOT file (along with other variables) for
  // later retrieval
  
  // Ensure that all nodes reach this point before beginning the
  // aggregation by using an MPI barrier
  if(ParallelVerbose)
    cout << "\nADAQAnalysis_MPI Node[" << MPI_Rank << "] : Reached the end-of-processing MPI barrier!"
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
  const int ArraySize = ADAQSettings->SpectrumNumBins + 2;
  
  // Create the array for Spectrum_H readout
  double SpectrumArray[ArraySize];

  // Set array values to the bin content the node's Spectrum_H object
  for(int i=0; i<ArraySize; i++)
    SpectrumArray[i] = Spectrum_H->GetBinContent(i);
  
  // Get the total number of entries in the nod'es Spectrum_H object
  double Entries = Spectrum_H->GetEntries();
  
  if(ParallelVerbose)
    cout << "\nADAQAnalysis_MPI Node[" << MPI_Rank << "] : Aggregating results to Node[0]!" << endl;
  
  // Use MPI::Reduce function to aggregate the arrays on each node
  // (which representing the Spectrum_H histogram) to a single array
  // on the master node.
  double *ReturnArray = AAParallel::GetInstance()->SumDoubleArrayToMaster(SpectrumArray, ArraySize);

  // Use the MPI::Reduce function to aggregate the total number of
  // entries on each node to a single double on the master
  double ReturnDouble = AAParallel::GetInstance()->SumDoublesToMaster(Entries);

  // Aggregate the total calculated RFQ current (if enabled) from all
  // nodes to the master node
  DeuteronsInTotal = AAParallel::GetInstance()->SumDoublesToMaster(DeuteronsInTotal);
  
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
    
    // Open the ROOT file that stores all the parallel processing data
    // in "update" mode such that we can append the master histogram
    ParallelFile = new TFile(AAParallel::GetInstance()->GetParallelFileName().c_str(), "update");
    
    // Write the master histogram object to the ROOT file ...
    MasterHistogram_H->Write();
    
    // Create/write the aggregated current vector to a file
    TVectorD AggregatedDeuterons(1);
    AggregatedDeuterons[0] = DeuteronsInTotal;
    AggregatedDeuterons.Write("AggregatedDeuterons");
    
    // ... and write the ROOT file to disk
    ParallelFile->Write();
  }
#endif

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
    if(ADAQSettings->UsePSDFilters[ADAQSettings->PSDChannel] and (*it).PSDFilterFlag==true)
      continue;
    
    // ...and use the lower and upper peak limits to calculate the
    // integral under each waveform peak that has passed all criterion
    double PeakIntegral = Waveform_H[ADAQSettings->WaveformChannel]->Integral((*it).PeakLimit_Lower,
									      (*it).PeakLimit_Upper);

    int Channel = ADAQSettings->WaveformChannel;
    
    // If the user has calibrated the spectrum, then transform the
    // peak integral in pulse units [ADC] to energy units
    if(ADAQSettings->UseSpectraCalibrations[Channel])
      PeakIntegral = ADAQSettings->SpectraCalibrations[Channel]->Eval(PeakIntegral);
    
    // Add the peak integral to the PAS 
    Spectrum_H->Fill(PeakIntegral);
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
    if(ADAQSettings->UsePSDFilters[ADAQSettings->PSDChannel] and (*it).PSDFilterFlag==true)
      continue;
    
    // Initialize the peak height for each peak region
    double PeakHeight = 0.;

    // Get the waveform processing channel number
    int Channel = ADAQSettings->WaveformChannel;

    // Iterate over the samples between lower and upper integration
    // limits to determine the maximum peak height
    for(int sample=(*it).PeakLimit_Lower; sample<(*it).PeakLimit_Upper; sample++){
      if(Waveform_H[Channel]->GetBinContent(sample) > PeakHeight)
	PeakHeight = Waveform_H[Channel]->GetBinContent(sample);
    }

    // If the user has calibrated the spectrum then transform the peak
    // heights in pulse units [ADC] to energy
    if(ADAQSettings->UseSpectraCalibrations[Channel])
      PeakHeight = ADAQSettings->SpectraCalibrations[Channel]->Eval(PeakHeight);
    
    // Add the detector pulse peak height to the spectrum histogram
    Spectrum_H->Fill(PeakHeight);
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
  double XAxisMin = Spectrum_H->GetXaxis()->GetXmax() * ADAQSettings->XAxisMin;
  double XAxisMax = Spectrum_H->GetXaxis()->GetXmax() * ADAQSettings->XAxisMax;

  double LowerIntLimit = (ADAQSettings->SpectrumIntegrationMin * XAxisMax) + XAxisMin;
  double UpperIntLimit = ADAQSettings->SpectrumIntegrationMax * XAxisMax;
  
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
  double Int = 0.;
  
  // Variable to hold the result of the error in the spectrum integral
  // calculation. Integration error is simply the square root of the
  // sum of the squares of the integration spectrum bin contents. The
  // error is computed with a ROOT TH1 method for robustness.
  double Err = 0.;
  
  string IntegralArg = "width";
  if(ADAQSettings->SpectrumIntegralInCounts)
    IntegralArg.assign("");
  
  ///////////////////////////////
  // Gaussian fitting/integration

  if(ADAQSettings->SpectrumUseGaussianFit){
    if(SpectrumFit_F)
      delete SpectrumFit_F;

    // Create a gaussian fit between the lower/upper limits; fit the
    // gaussian to the histogram representing the integral region
    // new TH1F object for analysis
    SpectrumFit_F = new TF1("PeakFit", "gaus", LowerIntLimit, UpperIntLimit);
    SpectrumIntegral_H->Fit("PeakFit","R N");

    // Project the gaussian fit into a histogram with identical
    // binning to the original spectrum to make analysis easier
    TH1F *SpectrumFit_H = (TH1F *)SpectrumIntegral_H->Clone("SpectrumFit_H");
    SpectrumFit_H->Eval(SpectrumFit_F);
    
    // Compute the integral and error between the lower/upper limits
    // of the histogram that resulted from the gaussian fit
    Int = SpectrumFit_H->IntegralAndError(SpectrumFit_H->FindBin(LowerIntLimit),
					  SpectrumFit_H->FindBin(UpperIntLimit),
					  Err,
					  IntegralArg.c_str());
    
    // Draw the gaussian peak fit
    SpectrumFit_F->SetLineColor(kGreen+2);
    SpectrumFit_F->SetLineWidth(3);
  }
  
  ////////////////////////
  // Histogram integration

  else{
    // Set the low and upper bin for the integration
    int StartBin = SpectrumIntegral_H->FindBin(LowerIntLimit);
    int StopBin = SpectrumIntegral_H->FindBin(UpperIntLimit);
    
    // Compute the integral and error
    Int = SpectrumIntegral_H->IntegralAndError(StartBin,
						 StopBin,
						 Err,
						 IntegralArg.c_str());
  }
  
  // The spectrum integral and error may be normalized to the total
  // computed RFQ current if desired by the user
  if(ADAQSettings->SpectrumNormalizeToCurrent){
    if(DeuteronsInTotal == 0)
      DeuteronsInTotal = 1.0;
    else{
      Int /= DeuteronsInTotal;
      Err /= DeuteronsInTotal;
    }
  }
  
  SpectrumIntegralValue = Int;
  SpectrumIntegralError = Err;
}


// Method used to output a generic TH1 object to a data text file in
// the format column1 == bin center, column2 == bin content. Note that
// the function accepts class types TH1 such that any derived class
// (TH1F, TH1D ...) can be saved with this function
bool AAComputation::SaveHistogramData(string Type, string FileName, string FileExtension)
{
  TH1F *HistogramToSave_H1;
  TH2F *HistogramToSave_H2;
  
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


TH2F *AAComputation::CreatePSDHistogram()
{
  if(PSDHistogram_H){
    delete PSDHistogram_H;
    PSDHistogramExists = false;
  }
  
  if(SequentialArchitecture){
    ProcessingProgressBar->Reset();
    ProcessingProgressBar->SetBarColor(ColorManager->Number2Pixel(41));
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

  int PSDChannel = ADAQSettings->PSDChannel;
  
  // Reboot the PeakFinder with up-to-date max peaks
  if(PeakFinder) delete PeakFinder;
  PeakFinder = new TSpectrum(ADAQSettings->MaxPeaks);
  
  // See documention in either ::CreateSpectrum() or
  // ::CreateDesplicedFile for parallel processing assignemnts
  
  WaveformStart = 0;
  WaveformEnd = ADAQSettings->PSDWaveformsToDiscriminate;
  
#ifdef MPI_ENABLED
  
  int SlaveEvents = int(ADAQSettings->PSDWaveformsToDiscriminate/MPI_Size);
  
  int MasterEvents = int(ADAQSettings->PSDWaveformsToDiscriminate-SlaveEvents*(MPI_Size-1));
  
  if(ParallelVerbose and IsMaster)
    cout << "\nADAQAnalysis_MPI Node[0]: Waveforms allocated to slaves (node != 0) = " << SlaveEvents << "\n"
	 <<   "                          Waveforms alloced to master (node == 0) =  " << MasterEvents
	 << endl;
  
  // Assign each master/slave a range of the total waveforms to
  // process based on the MPI rank. Note that WaveformEnd holds the
  // waveform_ID that the loop runs up to but does NOT include
  WaveformStart = (MPI_Rank * MasterEvents); // Start (include this waveform in the final histogram)
  WaveformEnd =  (MPI_Rank * MasterEvents) + SlaveEvents; // End (Up to but NOT including this waveform)
  
  if(ParallelVerbose)
    cout << "\nADAQAnalysis_MPI Node[" << MPI_Rank << "] : Handling waveforms " << WaveformStart << " to " << (WaveformEnd-1)
	 << endl;
#endif

  bool PeaksFound = false;
  
  for(int waveform=WaveformStart; waveform<WaveformEnd; waveform++){
    if(SequentialArchitecture)
      gSystem->ProcessEvents();
    
    ADAQWaveformTree->GetEntry(waveform);

    RawVoltage = *WaveformVecPtrs[PSDChannel];
    
    if(ADAQSettings->RawWaveform or ADAQSettings->BSWaveform)
      CalculateBSWaveform(PSDChannel, waveform);
    else if(ADAQSettings->ZSWaveform)
      CalculateZSWaveform(PSDChannel, waveform);
    
    // Find the peaks and peak limits in the current waveform
    PeaksFound = FindPeaks(Waveform_H[PSDChannel]);
    
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
      if((waveform+1) % int(WaveformEnd*ADAQSettings->UpdateFreq*1.0/100) == 0)
	UpdateProcessingProgress(waveform);
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
    double *ReturnArray = AAParallel::GetInstance()->SumDoubleArrayToMaster(&ColumnVector[0], ArraySizeY);
    
    // Assign the array to the DoubleArray that represents the
    // "master" or total PSDHistogram_H object
    for(int j=0; j<ArraySizeY; j++)
      DoubleArray[i][j] = ReturnArray[j];
  }

  // Aggregated the histogram entries from all nodes to the master
  double Entries = PSDHistogram_H->GetEntries();
  double ReturnDouble = AAParallel::GetInstance()->SumDoublesToMaster(Entries);
  
  // Aggregated the total deuterons from all nodes to the master
  DeuteronsInTotal = AAParallel::GetInstance()->SumDoublesToMaster(DeuteronsInTotal);

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
        
    // Assign the content from the aggregated 2-D array to the new
    // master histogram
    for(int i=0; i<ArraySizeX; i++)
      for(int j=0; j<ArraySizeY; j++)
	MasterPSDHistogram_H->SetBinContent(i, j, DoubleArray[i][j]);
    MasterPSDHistogram_H->SetEntries(ReturnDouble);
    
    // Open the TFile  and write all the necessary object to it
    ParallelFile = new TFile(AAParallel::GetInstance()->GetParallelFileName().c_str(), "update");
    
    MasterPSDHistogram_H->Write("MasterPSDHistogram");

    TVectorD AggregatedDeuterons(1);
    AggregatedDeuterons[0] = DeuteronsInTotal;
    AggregatedDeuterons.Write("AggregatedDeuterons");

    ParallelFile->Write();
  }
#endif
 
  // Update the bool to alert the code that a valid PSDHistogram_H object exists.
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
void AAComputation::CalculatePSDIntegrals(bool FillPSDHistogram)
{
  // Iterate over each peak stored in the vector of PeakInfoStructs...
  vector<PeakInfoStruct>::iterator it;
  for(it=PeakInfoVec.begin(); it!=PeakInfoVec.end(); it++){
    
    // Assign the integration limits for the tail and total
    // integrals. Note that peak limit and upper limit have the
    // user-specified included to provide flexible integration
    double LowerLimit = (*it).PeakLimit_Lower; // Left-most side of peak
    double Peak = (*it).PeakPosX + ADAQSettings->PSDPeakOffset; // The peak location + offset
    double UpperLimit = (*it).PeakLimit_Upper + ADAQSettings->PSDTailOffset; // Right-most side of peak + offset
    
    // Compute the total integral (lower to upper limit)
    double TotalIntegral = Waveform_H[ADAQSettings->PSDChannel]->Integral(LowerLimit, UpperLimit);
    
    // Compute the tail integral (peak to upper limit)
    double TailIntegral = Waveform_H[ADAQSettings->PSDChannel]->Integral(Peak, UpperLimit);
    
    if(Verbose)
      cout << "PSD tail integral = " << TailIntegral << " (ADC)\n"
	   << "PSD total integral = " << TotalIntegral << " (ADC)\n"
	   << endl;
    
    // If the user has enabled a PSD filter ...
    if(ADAQSettings->UsePSDFilters[ADAQSettings->PSDChannel])
      
      // ... then apply the PSD filter to the waveform. If the
      // waveform does not pass the filter, mark the flag indicating
      // that it should be filtered out due to its pulse shap
      if(ApplyPSDFilter(TailIntegral, TotalIntegral))
	(*it).PSDFilterFlag = true;
    
    // The total integral of the waveform must exceed the PSDThreshold
    // in order to be histogrammed. This allows the user flexibility
    // in eliminating the large numbers of small waveform events.
    if((TotalIntegral > ADAQSettings->PSDThreshold) and FillPSDHistogram)
      if((*it).PSDFilterFlag == false)
	PSDHistogram_H->Fill(TotalIntegral, TailIntegral);
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
bool AAComputation::ApplyPSDFilter(double TailIntegral, double TotalIntegral)
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
  if(ADAQSettings->PSDFilterPolarity > 0 and 
     TailIntegral >= ADAQSettings->PSDFilters[ADAQSettings->PSDChannel]->Eval(TotalIntegral))
    return false;

  // Waveform passed the criterion for a negative PSD filter (point in
  // tail/total PSD integral space fell "below" the TGraph; therefore,
  // it should not be filtered so return false
  else if(ADAQSettings->PSDFilterPolarity < 0 and 
	  TailIntegral <= ADAQSettings->PSDFilters[ADAQSettings->PSDChannel]->Eval(TotalIntegral))
    return false;

  // Waveform did not pass the PSD filter tests; therefore it should
  // be filtered so return true
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

  // If the user has selected to plot the derivative on top of the
  // spectrum, assign a vertical offset that will be used to plot the
  // derivative vertically centered in the canvas window. Also, reduce
  // the vertical scale of the total derivative such that it will
  // appropriately fit within the spectrum canvas.
  double VerticalOffset = 0;
  double ScaleFactor = 1.;
  if(ADAQSettings->SpectrumOverplotDerivative){
    VerticalOffset = Spectrum_H->GetMaximum() / 2;
    ScaleFactor = 1.3;
  }
  
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
      Differences[bin] = VerticalOffset;
      continue;
    }
    
    double Previous = Spectrum_H->GetBinContent(bin-1);
    double Current = Spectrum_H->GetBinContent(bin);

    // Compute the "derivative", i.e. the difference between the
    // current and previous bin contents
    Differences[bin] = (ScaleFactor*(Current - Previous)) + VerticalOffset;
    
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
    SpectrumDerivative_H->SetBinContent(bin, y-VerticalOffset);
  }
  
  SpectrumDerivativeExists = true;

  return SpectrumDerivative_G;
}



// Method to integrate the Pearson waveform (which measures the RFQ
// beam current) in order to calculate the number of deuterons
// delivered during a single waveform. The deuterons/waveform are
// aggregated into a class member that stores total deuterons for all
// waveforms processed.
void AAComputation::IntegratePearsonWaveform(int Waveform)
{
  // The total deutorons delivered for all waveforms in the presently
  // loaed ADAQ-formatted ROOT file may be stored in the ROOT file
  // itself if that ROOT file was created during parallel processing.
  // If this is the case then return from this function since we
  // already have the number of primary interest.
  if(ADAQParResultsLoaded)
    return;

  // Get the V1720/data channel containing the output of the Pearson
  int Channel = ADAQSettings->PearsonChannel;
  
  // Compute the baseline subtracted Pearson waveform
  CalculateBSWaveform(Channel, Waveform, true);
  
  // There are two integration options for the Pearson waveform:
  // simply integrating the waveform from a lower limit to an upper
  // limit (in sample time), "raw integration"; fitting two 1st order
  // functions to the current trace and integrating (and summing)
  // their area, "fit integration". The firsbt option is the most CPU
  // efficient but requires a very clean RFQ current traces: the
  // second options is more CPU expensive but necessary for very noise
  // Pearson waveforms;
  
  if(ADAQSettings->IntegrateRawPearson){
    
    // Integrate the current waveform between the lower/upper limits. 
    double PearsonIntegralValue = Waveform_H[Channel]->Integral(Waveform_H[Channel]->FindBin(ADAQSettings->PearsonLowerLimit),
								Waveform_H[Channel]->FindBin(ADAQSettings->PearsonUpperLimit));
    
    // Compute the total number of deuterons in this waveform by
    // converting the result of the integral from units of V / ADC
    // with the appropriate factors.
    DeuteronsInWaveform = PearsonIntegralValue * adc2volts_V1720 * sample2seconds_V1720;
    DeuteronsInWaveform *= (volts2amps_Pearson / amplification_Pearson / electron_charge);
    
    if(DeuteronsInWaveform > 0)
      DeuteronsInTotal += DeuteronsInWaveform;
    
    if(Verbose)
      cout << "Total number of deuterons: " << "\t" << DeuteronsInTotal << endl;
    
    PearsonRawIntegration_H = (TH1F *)Waveform_H[Channel]->Clone("PearsonRawIntegration_H");
  }
  else if(ADAQSettings->IntegrateFitToPearson){
    // Create a TF1 object for the initial rise region of the current
    // trace; place the result into a TH1F for potential plotting
    TF1 *RiseFit = new TF1("RiseFit", "pol1", 
			   ADAQSettings->PearsonLowerLimit, 
			   ADAQSettings->PearsonMiddleLimit);
    Waveform_H[Channel]->Fit("RiseFit","R N Q C");
    PearsonRiseFit_H = (TH1F *)RiseFit->GetHistogram()->Clone("PearsonRiseFit_H");
    
    // Create a TF1 object for long "flat top" region of the current
    // trace a TH1F for potential plotting
    TF1 *PlateauFit = new TF1("PlateauFit", "pol1", 
			      ADAQSettings->PearsonMiddleLimit, 
			      ADAQSettings->PearsonUpperLimit);
    Waveform_H[Channel]->Fit("PlateauFit","R N Q C");
    PearsonPlateauFit_H = (TH1F *)PlateauFit->GetHistogram()->Clone("PearsonPlateauFit_H");
    
    // Compute the integrals. Note the "width" argument is passed to
    // the integration to specify that the histogram results should be
    // multiplied by the bin width.
    double PearsonIntegralValue = PearsonRiseFit_H->Integral(PearsonRiseFit_H->FindBin(ADAQSettings->PearsonLowerLimit), 
							     PearsonRiseFit_H->FindBin(ADAQSettings->PearsonUpperLimit),
							     "width");
    
    PearsonIntegralValue += PearsonPlateauFit_H->Integral(PearsonPlateauFit_H->FindBin(ADAQSettings->PearsonMiddleLimit),
							  PearsonPlateauFit_H->FindBin(ADAQSettings->PearsonUpperLimit),
							  "width");
    
    // Compute the total number of deuterons in this waveform by
    // converting the result of the integrals from units of V / ADC
    // with the appropriate factors.
    DeuteronsInWaveform = PearsonIntegralValue * adc2volts_V1720 * sample2seconds_V1720;
    DeuteronsInWaveform *= (volts2amps_Pearson / amplification_Pearson / electron_charge);
    
    if(DeuteronsInWaveform > 0)
      DeuteronsInTotal += DeuteronsInWaveform;
    
    if(Verbose)
      cout << "Total number of deuterons: " << "\t" << DeuteronsInTotal << endl;
    
    // Delete the TF1 objects to prevent bleeding memory
    delete RiseFit;
    delete PlateauFit;
  }
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
      // Obtain the "master" histogram, which is a TH1F object
      // contains the result of MPI reducing all the TH1F objects
      // calculated by the nodes in parallel. Set the master histogram
      // to the Spectrum_H class data member and plot it.
      Spectrum_H = (TH1F *)ParallelFile->Get("MasterHistogram");
      SpectrumExists = true;

      // Obtain the total number of deuterons integrated during
      // histogram creation and update the ROOT widget
      TVectorD *AggregatedDeuterons = (TVectorD *)ParallelFile->Get("AggregatedDeuterons");
      DeuteronsInTotal = (*AggregatedDeuterons)[0];
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
      
      TVectorD *AggregatedDeuterons = (TVectorD *)ParallelFile->Get("AggregatedDeuterons");
      DeuteronsInTotal = (*AggregatedDeuterons)[0];
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
  
  // Return to the ROOT TFile containing the waveforms
  // ZSH : Causing segfaults...necessary?
  //ADAQRootFile->cd();
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


bool AAComputation::SetCalibrationPoint(int Channel, int SetPoint,
					double Energy, double PulseUnit)
{
  // Add a new point to the calibration
  if(SetPoint == CalibrationData[Channel].PointID.size()){
    CalibrationData[Channel].PointID.push_back(SetPoint);
    CalibrationData[Channel].Energy.push_back(Energy);
    CalibrationData[Channel].PulseUnit.push_back(PulseUnit);
    
    return true;
  }
  
  // Overwrite a previous calibration point
  else{
    CalibrationData[Channel].Energy[SetPoint] = Energy;
    CalibrationData[Channel].PulseUnit[SetPoint] = PulseUnit;

    return false;
  }
}


bool AAComputation::SetCalibration(int Channel)
{
  if(CalibrationData[Channel].PointID.size() >= 2){
    // Determine the total number of calibration points in the
    // current channel's calibration data set
    int NumCalibrationPoints = CalibrationData[Channel].PointID.size();
    
    // Create a new "SpectraCalibrations" TGraph object.
    SpectraCalibrations[Channel] = new TGraph(NumCalibrationPoints,
						    &CalibrationData[Channel].PulseUnit[0],
						    &CalibrationData[Channel].Energy[0]);
    
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


bool AAComputation::SetFixedEPCalibration()
{
  SetCalibrationPoint(ADAQSettings->WaveformChannel, 
		      0, 
		      EnergyPoints_FixedEP[0],
		      PulseUnitPoints_FixedEP[0]);
  
  SetCalibrationPoint(ADAQSettings->WaveformChannel, 
		      1, 
		      EnergyPoints_FixedEP[1],
		      PulseUnitPoints_FixedEP[1]);
  
  SetCalibration(ADAQSettings->WaveformChannel);
}



bool AAComputation::ClearCalibration(int Channel)
{
  // Clear the channel calibration vectors for the current channel
  CalibrationData[Channel].PointID.clear();
  CalibrationData[Channel].Energy.clear();
  CalibrationData[Channel].PulseUnit.clear();
  
  // Delete the current channel's depracated calibration manager
  // TGraph object to prevent memory leaks but reallocat the TGraph
  // object to prevent seg. faults when writing the
  // SpectraCalibrations to the ROOT parallel processing file
  if(UseSpectraCalibrations[Channel]){
    delete SpectraCalibrations[Channel];
    SpectraCalibrations[Channel] = new TGraph;
  }
  
  // Set the current channel's calibration boolean to false,
  // indicating that the calibration manager will NOT be used within
  // the acquisition loop
  UseSpectraCalibrations[Channel] = false;

  return true;
}



// Method to create a pulse shape discrimination (PSD) filter, which
// is a TGraph composed of points defined by the user clicking on the
// active canvas to define a line used to delinate regions. This
// function is called by ::HandleCanvas() each time that the user
// clicks on the active pad, passing the x- and y-pixel click
// location to the function
void AAComputation::CreatePSDFilter(int XPixel, int YPixel)
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
  if(PSDFilters[ADAQSettings->PSDChannel]) delete PSDFilters[ADAQSettings->PSDChannel];
  PSDFilters[ADAQSettings->PSDChannel] = new TGraph(PSDNumFilterPoints, &PSDFilterXPoints[0], &PSDFilterYPoints[0]);
  
  UsePSDFilters[ADAQSettings->PSDChannel] = true;
}


void AAComputation::ClearPSDFilter(int Channel)
{
  PSDNumFilterPoints = 0;
  PSDFilterXPoints.clear();
  PSDFilterYPoints.clear();
  
  if(PSDFilters[ADAQSettings->PSDChannel]) delete PSDFilters[ADAQSettings->PSDChannel];
  PSDFilters[ADAQSettings->PSDChannel] = new TGraph;
  
  UsePSDFilters[ADAQSettings->PSDChannel] = false;
}


void AAComputation::CreateDesplicedFile()
{
  cout << ADAQSettings->WaveformsToDesplice << endl;

  



  ////////////////////////////
  // Prepare for processing //
  ////////////////////////////
  
  // Reset the progres bar if binary is sequential architecture
  
  if(SequentialArchitecture){
    ProcessingProgressBar->Reset();
    ProcessingProgressBar->SetBarColor(ColorManager->Number2Pixel(41));
    ProcessingProgressBar->SetForegroundColor(ColorManager->Number2Pixel(1));
  }
  
  
  if(PeakFinder) delete PeakFinder;
  PeakFinder = new TSpectrum(ADAQSettings->MaxPeaks);
  
  // Variable to aggregate the integrated RFQ current. Presently this
  // feature is NOT implemented. 
  DeuteronsInTotal = 0.;
  
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
    cout << "\nADAQAnalysis_MPI Node[0] : Waveforms allocated to slaves (node != 0) = " << SlaveEvents << "\n"
	 <<   "                           Waveforms alloced to master (node == 0) =  " << MasterEvents
	 << endl;
  
  // Assign each master/slave a range of the total waveforms to be
  // process based on each nodes MPI rank
  WaveformStart = (MPI_Rank * MasterEvents); // Start (and include this waveform in despliced file)
  WaveformEnd =  (MPI_Rank * MasterEvents) + SlaveEvents; // End (up to but NOT including this waveform)
  
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
  ADAQRootFile->cd();


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
    RawVoltage = *WaveformVecPtrs[Channel];

    // Select the type of Waveform_H object to create
    if(ADAQSettings->RawWaveform or ADAQSettings->BSWaveform)
      CalculateBSWaveform(Channel, waveform);
    else if(ADAQSettings->ZSWaveform)
      CalculateZSWaveform(Channel, waveform);
    
    // If specified by the user then calculate the RFQ current
    // integral for each waveform and aggregate the total (into the
    // 'DeuteronsInTotal' class data member) for persistent storage in the
    // despliced TFile. The 'false' argument specifies not to plot
    // integration results on the canvas.
    if(ADAQSettings->IntegratePearson)
      IntegratePearsonWaveform(false);


    ///////////////////////////
    // Find peaks and peak data
    
    // Find the peaks (and peak data) in the current waveform.
    // Function returns "true" ("false) if peaks are found (not found).
    PeaksFound = FindPeaks(Waveform_H[Channel]);
    
    // If no peaks found, continue to next waveform to save CPU $
    if(!PeaksFound)
      continue;
    
    // Calculate the PSD integrals and determine if they pass through
    // the pulse-shape filter 
    if(UsePSDFilters[ADAQSettings->PSDChannel])
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

  // Store the values calculated in parallel into the class for
  // persistent storage in the despliced ROOT file
  
  PR->DeuteronsInTotal = DeuteronsInTotal;
  
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
  double AggregatedCurrent = AAParallel::GetInstance()->SumDoublesToMaster(DeuteronsInTotal);

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
    
    // Store the aggregated RFQ current value on the master into the
    // persistent parallel results class for writing to the ROOT file
    PR->DeuteronsInTotal = AggregatedCurrent;

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


// Method to compute the instantaneous (within the RFQ deuteron pulse)
// and time-averaged (in real time) detector count rate. Note that the
// count rate can only be calculated using sequential
// architecture. Because only a few thousand waveforms needs to be
// processed to compute the count rates, it is not presently foreseen
// that this feature will be implemented in parallel. Note also that
// count rates can only be calculated for RFQ triggered acquisition
// windows with a known pulse width and rep. rate.
void AAComputation::CalculateCountRate()
{
  // If the TSPectrum PeakFinder object exists then delete it and
  // recreate it to account for changes to the MaxPeaks variable
  if(PeakFinder) delete PeakFinder;
  PeakFinder = new TSpectrum(ADAQSettings->MaxPeaks);
  
  // Reset the total number of identified peaks to zero
  TotalPeaks = 0;
  
  // Get the number of waveforms to process in the calculation
  int NumWaveforms = ADAQSettings->RFQCountRateWaveforms;

  int Channel = ADAQSettings->WaveformChannel;

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
    if(ADAQSettings->RawWaveform or ADAQSettings->BSWaveform)
      CalculateBSWaveform(Channel, waveform);
    else if(ADAQSettings->ZSWaveform)
      CalculateZSWaveform(Channel, waveform);
  
    // Find the peaks in the waveform. Note that this function is
    // responsible for incrementing the 'TotalPeaks' member data
    FindPeaks(Waveform_H[Channel]);

    // Update the waveform processing progress bar
    if((waveform+1) % int(WaveformEnd*ADAQSettings->UpdateFreq*1.0/100) == 0)
      UpdateProcessingProgress(waveform);
  }
  
  // At this point, the 'TotalPeaks' variable contains the total
  // number of identified peaks after processing the number of
  // waveforms specified by the 'NumWaveforms' variable

  // Compute the time (in seconds) that the RFQ beam was 'on'
  double TotalTime = ADAQSettings->RFQPulseWidth * us2s * NumWaveforms;

  // Compute the instantaneous count rate, i.e. the detector count
  // rate only when the RFQ beam is on
  double InstCountRate = TotalPeaks * 1.0 / TotalTime;
  //InstCountRate_NEFL->GetEntry()->SetNumber(InstCountRate);
  
  // Compute the average count rate, i.e. the detector count rate in
  // real time, which requires a knowledge of the RFQ rep rate.
  double AvgCountRate = InstCountRate * (ADAQSettings->RFQPulseWidth * us2s * ADAQSettings->RFQRepRate);
  //  AvgCountRate_NEFL->GetEntry()->SetNumber(AvgCountRate);
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
  if(PSDHistogramSlice_H) delete PSDHistogramSlice_H;
  PSDHistogramSlice_H = (TH1D *)PSDSlice_H->Clone("PSDHistogramSlice_H");
  PSDHistogramSliceExists = true;

}


void AAComputation::CreateACRONYMSpectrum()
{
  if(!Spectrum_H){
    delete Spectrum_H;
    SpectrumExists = false;
  }
  
  Spectrum_H = new TH1F("Spectrum_H", "ACRONYM Spectrum",
			ADAQSettings->SpectrumNumBins,
			ADAQSettings->SpectrumMinBin,
			ADAQSettings->SpectrumMaxBin);
  
  TTree *Tree = 0;
  acroEvent *Event = 0;

  int Entries = 0;
  if(ADAQSettings->ACROSpectrumLS){
    Entries = LSDetectorTree->GetEntries();
    Tree = LSDetectorTree;
    Event = LSDetectorEvent;
  }
  else if(ADAQSettings->ACROSpectrumES){
    Entries = ESDetectorTree->GetEntries();
    Tree = ESDetectorTree;
    Event = ESDetectorEvent;
  }

  // Number to histogram is automatically updated in AAInterface when
  // the LaBr3/EJ301 radio buttons are clicked
  int MaxEntriesToPlot = ADAQSettings->WaveformsToHistogram;
  
  for(int entry=0; entry<MaxEntriesToPlot; entry++){
    
    Tree->GetEvent(entry);

    double Quantity = 0.;
    if(ADAQSettings->ACROSpectrumTypeEnergy)
      Quantity = Event->recoilEnergyDep;
    else if(ADAQSettings->ACROSpectrumTypeScintCreated)
      Quantity = Event->scintPhotonsCreated;
    else if(ADAQSettings->ACROSpectrumTypeScintCounted)
      Quantity = Event->scintPhotonsCounted;
    
    if(ADAQSettings->UseSpectraCalibrations[ADAQSettings->WaveformChannel])
      Quantity = SpectraCalibrations[ADAQSettings->WaveformChannel]->Eval(Quantity);
    
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
