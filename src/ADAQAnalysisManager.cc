#include <TSystem.h>
#include <TError.h>

#ifdef MPI_ENABLED
#include <mpi.h>
#endif

#include <boost/array.hpp>
#include <boost/thread.hpp>

#include "ADAQAnalysisManager.hh"
#include "ADAQAnalysisConstants.hh"
#include "ADAQAnalysisVersion.hh"

#include <iostream>
using namespace std;


ADAQAnalysisManager *ADAQAnalysisManager::TheAnalysisManager = 0;


ADAQAnalysisManager *ADAQAnalysisManager::GetInstance()
{ return TheAnalysisManager; }


ADAQAnalysisManager::ADAQAnalysisManager(bool PA)
  : ADAQMeasParams(0), ADAQRootFile(0), ADAQRootFileName(""), ADAQRootFileLoaded(false), ADAQWaveformTree(0), 
    ADAQParResults(NULL), ADAQParResultsLoaded(false),
    DataDirectory(getenv("PWD")), SaveSpectrumDirectory(getenv("HOME")), SaveHistogramDirectory(getenv("HOME")),
    PrintCanvasDirectory(getenv("HOME")), DesplicedDirectory(getenv("HOME")),
    Time(0), RawVoltage(0), RecordLength(0),
    GraphicFileName(""), GraphicFileExtension(""),
    PeakFinder(new TSpectrum), NumPeaks(0), PeakInfoVec(0), PeakIntegral_LowerLimit(0), PeakIntegral_UpperLimit(0), PeakLimits(0),
    MPI_Size(1), MPI_Rank(0), IsMaster(true), IsSlave(false), ParallelArchitecture(PA), SequentialArchitecture(!PA),
    Verbose(false), ParallelVerbose(true),
    XAxisMin(0), XAxisMax(1.0), YAxisMin(0), YAxisMax(4095), 
    Title(""), XAxisTitle(""), YAxisTitle(""), ZAxisTitle(""), PaletteAxisTitle(""),
    WaveformPolarity(-1.0), PearsonPolarity(1.0), Baseline(0.), PSDFilterPolarity(1.),
    V1720MaximumBit(4095), NumDataChannels(8),
    SpectrumExists(false), SpectrumDerivativeExists(false), PSDHistogramExists(false), PSDHistogramSliceExists(false),
    SpectrumMaxPeaks(0), TotalPeaks(0), TotalDeuterons(0),
    GaussianBinWidth(1.), CountsBinWidth(1.)
{
  if(TheAnalysisManager)
    cout << "\nERROR! TheAnalysisManager was constructed twice!\n" << endl;
  TheAnalysisManager = this;

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
  
  // Assign the location of the ADAQAnalysisInterface parallel binary
  // (depends on whether the presently executed binary is a develpment
  // or production version of the code)
  ADAQHOME = getenv("ADAQHOME");
  USER = getenv("USER");
  
  if(VersionString == "Development")
    ParallelBinaryName = ADAQHOME + "/analysis/ADAQAnalysisGUI/trunk/bin/ADAQAnalysis_MPI";
  else
    ParallelBinaryName = ADAQHOME + "/analysis/ADAQAnalysisGUI/versions/" + VersionString + "/bin/ADAQAnalysis_MPI";

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
  /*  
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
      cout << "\nError! Unspecified command line argument '" << CmdLineArg << "' passed to ADAQAnalysis_MPI!\n"
	   <<   "       At present, only the args 'histogramming' and 'desplicing' are allowed\n"
	   <<   "       Parallel binaries will exit ...\n"
	   << endl;
      exit(-42);
    }
  }
  */
}


ADAQAnalysisManager::~ADAQAnalysisManager()
{;}


bool ADAQAnalysisManager::LoadADAQRootFile(string FileName)
{
  //////////////////////////////////
  // Open the specified ROOT file //
  //////////////////////////////////
      
  // Open the specified ROOT file 
  TFile *ADAQRootFile = new TFile(FileName.c_str(), "read");
      
  // If a valid ADAQ ROOT file was NOT opened...
  if(!ADAQRootFile->IsOpen()){
    ADAQRootFileLoaded = false;
  }
  else{

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
      //if(SequentialArchitecture)
      //DeuteronsInTotal_NEFL->GetEntry()->SetNumber(TotalDeuterons);
    
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
    // Update the bool that determines if a valid ROOT file is loaded
    ADAQRootFileLoaded = true;
  }
  return ADAQRootFileLoaded;
}

    
bool ADAQAnalysisManager::LoadACRONYMRootFile(string)
{return false;}


TH1F *ADAQAnalysisManager::CalculateRawWaveform(int Channel, int Waveform)
{
  ADAQWaveformTree->GetEntry(Waveform);

  vector<int> RawVoltage = *WaveformVecPtrs[Channel];

  Baseline = CalculateBaseline(&RawVoltage);

  if(Waveform_H[Channel])
    delete Waveform_H[Channel];
  
  Waveform_H[Channel] = new TH1F("Waveform_H", "Raw Waveform", RecordLength-1, 0, RecordLength);
  
  vector<int>::iterator iter;
  for(iter=RawVoltage.begin(); iter!=RawVoltage.end(); iter++)
    Waveform_H[Channel]->SetBinContent((iter-RawVoltage.begin()), *iter);
  
  return Waveform_H[Channel];
}


// Method to extract the digitized data on the specified data channel
// and store it into a TH1F object after calculating and subtracting
// the waveform baseline. Note that the function argument bool is used
// to determine which polarity (a standard (detector) waveform or a
// Pearson (RFQ current) waveform) should be used in processing
TH1F* ADAQAnalysisManager::CalculateBSWaveform(int Channel, int Waveform, bool CurrentWaveform)
{
  ADAQWaveformTree->GetEntry(Waveform);
  
  double Polarity;
  (CurrentWaveform) ? Polarity = ADAQSettings->PearsonPolarity : Polarity = ADAQSettings->WaveformPolarity;
  
  vector<int> RawVoltage = *WaveformVecPtrs[Channel];
  
  Baseline = CalculateBaseline(&RawVoltage);
  
  if(Waveform_H[Channel])
    delete Waveform_H[Channel];
  
  Waveform_H[Channel] = new TH1F("Waveform_H","Baseline-subtracted Waveform", RecordLength-1, 0, RecordLength);
  
  vector<int>::iterator it;  
  for(it=RawVoltage.begin(); it!=RawVoltage.end(); it++)
    Waveform_H[Channel]->SetBinContent((it-RawVoltage.begin()),
				       Polarity*(*it-Baseline));

  return Waveform_H[Channel];
}


// Method to extract the digitized data on the specified data channel
// and store it into a TH1F object after computing the zero-suppresion
// (ZS) waveform. The baseline is calculated and stored into the class
// member for later use.  Note that the function argument bool is used
// to determine which polarity (a standard (detector) waveform or a
// Pearson (RFQ current) waveform) should be used in processing
TH1F *ADAQAnalysisManager::CalculateZSWaveform(int Channel, int Waveform, bool CurrentWaveform)
{
  ADAQWaveformTree->GetEntry(Waveform);
  
  double Polarity;
  (CurrentWaveform) ? Polarity = ADAQSettings->PearsonPolarity : Polarity = ADAQSettings->WaveformPolarity;
  
  vector<int> RawVoltage = *WaveformVecPtrs[Channel];

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
  
  if(Waveform_H[Channel])
    delete Waveform_H[Channel];
  
  Waveform_H[Channel] = new TH1F("Waveform_H","Zero Suppression Waveform", ZSWaveformSize-1, 0, ZSWaveformSize);
  
  vector<double>::iterator iter;
  for(iter=ZSVoltage.begin(); iter!=ZSVoltage.end(); iter++)
    Waveform_H[Channel]->SetBinContent((iter-ZSVoltage.begin()), *iter);
  
  return Waveform_H[Channel];
}


// Method to calculate the baseline of a waveform. The "baseline" is
// simply the average of the waveform voltage taken over the specified
// range in time (in units of samples)
double ADAQAnalysisManager::CalculateBaseline(vector<int> *Waveform)
{
  int BaselineCalcLength = ADAQSettings->BaselineCalcMax - ADAQSettings->BaselineCalcMin;
  double Baseline = 0.;
  for(int sample=ADAQSettings->BaselineCalcMin; sample<ADAQSettings->BaselineCalcMax; sample++)
    Baseline += ((*Waveform)[sample]*1.0/BaselineCalcLength);
  
  return Baseline;
}


double ADAQAnalysisManager::CalculateBaseline(TH1F *Waveform)
{
  int BaselineCalcLength = ADAQSettings->BaselineCalcMax - ADAQSettings->BaselineCalcMin;
  double Baseline = 0.;
  for(int sample = 0; sample < ADAQSettings->BaselineCalcMax; sample++)
    Baseline += (Waveform->GetBinContent(sample)*1.0/BaselineCalcLength);
      
  return Baseline;
}


void ADAQAnalysisManager::CreateSpectrum()
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
  Spectrum_H = new TH1F("Spectrum_H", "Pulse spectrum", 
			ADAQSettings->SpectrumNumBins, 
			ADAQSettings->SpectrumMinBin,
			ADAQSettings->SpectrumMaxBin);
  
  
  // Variables for calculating pulse height and area
  double SampleHeight, PulseHeight, PulseArea;
  SampleHeight = PulseHeight = PulseArea = 0.;
  
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
  PeakFinder = new TSpectrum(ADAQSettings->MaxPeaks);

  // Assign the range of waveforms that will be analyzed to create a
  // histogram. Note that in sequential architecture if N waveforms
  // are to be histogrammed, waveforms from waveform_ID == 0 to
  // waveform_ID == (WaveformsToHistogram-1) will be included in the
  // final spectra 
  WaveformStart = 0; // Start (include this waveform in final histogram)
  WaveformEnd = ADAQSettings->WaveformsToHistogram; // End (Up to but NOT including this waveform)
  /*
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
    cout << "\nADAQAnalysis_MPI Node[0]: Waveforms allocated to slaves (node != 0) = " << SlaveEvents << "\n"
	 <<   "                             Waveforms alloced to master (node == 0) =  " << MasterEvents
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
  */
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
    if(ADAQSettings->RawWaveform or ADAQSettings->BSWaveform)
      CalculateBSWaveform(Channel, waveform);
    else if(ADAQSettings->ZSWaveform)
      CalculateZSWaveform(Channel, waveform);
    
    // Calculate the total RFQ current for this waveform.
    if(ADAQSettings->IntegratePearson)
      {}//IntegratePearsonWaveform(false);
    
    
    ///////////////////////////////
    // Whole waveform processing //
    ///////////////////////////////

    // If the entire waveform is to be used to calculate a pulse spectrum ...
    if(ADAQSettings->IntegrationTypeWholeWaveform){
      
      // If a pulse height spectrum (PHS) is to be created ...
      if(ADAQSettings->SpectrumTypePHS){
	// Get the pulse height by find the maximum bin value stored
	// in the Waveform_H TH1F via class methods. Note that spectra
	// are always created with positive polarity waveforms
	PulseHeight = Waveform_H[Channel]->GetBinContent(Waveform_H[Channel]->GetMaximumBin());

	// If the calibration manager is to be used to convert the
	// value from pulse units [ADC] to energy units [keV, MeV,
	// ...] then do so
	//if(UseCalibrationManager[Channel])
	//	  PulseHeight = CalibrationManager[Channel]->Eval(PulseHeight);

	// Add the pulse height to the spectrum histogram	
	Spectrum_H->Fill(PulseHeight);
      }

      // ... or if a pulse area spectrum (PAS) is to be created ...
      else if(ADAQSettings->SpectrumTypePAS){

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
	//	if(UseCalibrationManager[Channel])
	//	  PulseArea = CalibrationManager[Channel]->Eval(PulseArea);

	// Add the pulse area to the spectrum histogram
	Spectrum_H->Fill(PulseArea);
      }
      
      // Note that we must add a +1 to the waveform number in order to
      // get the modulo to land on the correct intervals
      if(IsMaster)
      	if((waveform+1) % int(WaveformEnd*ADAQSettings->UpdateFreq*1.0/100) == 0)
      	  UpdateProcessingProgress(waveform);
    }
    
    /////////////////////////////////////
    // Peak-finder waveform processing //
    /////////////////////////////////////
    
    // If the peak-finding/limit-finding algorithm is to be used to
    // create the pulse spectrum ...
    else if(ADAQSettings->IntegrationTypePeakFinder){
      
      // ...pass the Waveform_H[Channel] TH1F object to the peak-finding
      // algorithm, passing 'false' as the second argument to turn off
      // plotting of the waveforms. ADAQAnalysisInterface::FindPeaks() will
      // fill up a vector<PeakInfoStruct> that will be used to either
      // integrate the valid peaks to create a PAS or find the peak
      // heights to create a PHS, returning true. If zero peaks are
      // found in the waveform then FindPeaks() returns false
      //PeaksFound = FindPeaks(Waveform_H[Channel], false);

      // If no peaks are present in the current waveform then continue
      // on to the next waveform for analysis
      //      if(!PeaksFound)
      //      	continue;
      
      // Calculate the PSD integrals and determine if they pass
      // the pulse-shape filterthrough the pulse-shape filter
      //      if(UsePSDFilterManager[PSDChannel])
      //	CalculatePSDIntegrals(false);
      
      // If a PAS is to be created ...
      if(ADAQSettings->SpectrumTypePAS)
	IntegratePeaks();
      
      // or if a PHS is to be created ...
      else if(ADAQSettings->SpectrumTypePHS)
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
    ProcessingProgressBar->Increment(100);
    ProcessingProgressBar->SetBarColor(ColorManager->Number2Pixel(32));
    ProcessingProgressBar->SetForegroundColor(ColorManager->Number2Pixel(0));
    
    // Update the number entry field in the "Analysis" tab to display
    // the total number of deuterons
    //    if(IntegratePearson)
    //      DeuteronsInTotal_NEFL->GetEntry()->SetNumber(TotalDeuterons);
  }


  /*

  
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
  const int ArraySize = SpectrumNumBins + 2;
  
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
      cout << "\nADAQAnalysis_MPI Node[0] : Writing master TH1F histogram to disk!\n"
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
*/  
  SpectrumExists = true;
}




void ADAQAnalysisManager::IntegratePeaks()
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
    //    if(UsePSDFilterManager[PSDChannel] and (*it).PSDFilterFlag==true)
    //      continue;

    // ...and use the lower and upper peak limits to calculate the
    // integral under each waveform peak that has passed all criterion
    double PeakIntegral = Waveform_H[Channel]->Integral((*it).PeakLimit_Lower,
							(*it).PeakLimit_Upper);
    
    // If the user has calibrated the spectrum, then transform the
    // peak integral in pulse units [ADC] to energy units
    //    if(UseCalibrationManager[Channel])
    //      PeakIntegral = CalibrationManager[Channel]->Eval(PeakIntegral);
    
    // Add the peak integral to the PAS 
    Spectrum_H->Fill(PeakIntegral);

  }
}


void ADAQAnalysisManager::FindPeakHeights()
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


void ADAQAnalysisManager::UpdateProcessingProgress(int Waveform)
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
