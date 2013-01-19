//////////////////////////////////////////////////////////////////////////////////////////
// name: ADAQAnalysisTypes.hh
// date: 17 Jan 13 (Last updated)
// auth: Zach Hartwig
// 
// desc: This header file contains several structures and classes that
//       are used for various purposes in ADAQAnalysisGUI. Descriptions 
//       of each can be found below
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __ADAQAnalysisTypes_hh__
#define __ADAQAnalysisTypes_hh__ 1

// Structure that contains information on a single peak found by the
// TSpectrum PeakFinder during waveform processing. For each peak
// found, this structure is filled with the relevant information and
// pushed back into a vector for later processing
struct PeakInfoStruct{
  int PeakID; // Unique integer ID of the peak
  double PeakPosX; // Peak position along the X-axis [sample number]
  double PeakPosY; // Peak position along the Y-axis [ADC or calibrated energy units]
  double PeakLimit_Lower; // Position along X-axis where a low-2-high floor crossing occurs
  double PeakLimit_Upper; // Position along X-axis where a high-2-low floor crossing occurs
  bool AnalyzeFlag; // Flag to indicate whether the current peak should be analyzed into a spectrum
  bool PileupFlag; // Flag to indicate whether current peak is part of a pileup events

  // Initialization for the variables
  PeakInfoStruct() : PeakID(-1),
		     PeakPosX(-1),
		     PeakPosY(-1),
		     PeakLimit_Lower(-1),
		     PeakLimit_Upper(-1),
		     AnalyzeFlag(true),
		     PileupFlag(false)
  {}
};


// Structure that contains information on a single calibration point
// for a single channel. For each calibration point, a structure is
// filled with the relevant information and pushed back into a vector
// for later processing
struct ADAQChannelCalibrationData{
  vector<int> PointID; // Unique integer ID of the calibration point
  vector<double> Energy; // Energy of calibration peak set by user [desired energy unit]
  vector<double> PulseUnit; // Pulse unit of calibration peak set by user [ADC]
};


// Class derived from ROOT TObject that is used to store variables
// from the sequential binary for use in waveform processing by the
// parallel binaries. Class is derived from TObject class to provide
// easy, persistant storage in a ROOT TFile
class ADAQAnalysisParallelParameters : public TObject
{
public:
  vector<int> TestVector;

  string ADAQRootFileName;

  string DesplicedFileName;
  
  int Channel, WaveformsToHistogram;
  int SpectrumNumBins, SpectrumMinBin, SpectrumMaxBin;
  int MaxPeaks, ZeroSuppressionCeiling, Floor;
  int BaselineCalcMin, BaselineCalcMax;
  int UpdateFreq;
  bool PlotFloor, PlotPeakIntegratingRegion, PlotCrossings, UsePileupRejection;
  bool RawWaveform, BaselineSubtractedWaveform, ZeroSuppressionWaveform;
  bool IntegrationTypeWholeWaveform, IntegrationTypePeakFinder;
  bool SpectrumTypePHS, SpectrumTypePAS;
  double Sigma, Resolution;
  double WaveformPolarity, PearsonPolarity;

  int PSDChannel, PSDWaveformsToDiscriminate;
  int PSDThreshold, PSDTailOffset, PSDPeakOffset;
  int PSDNumTailBins, PSDMinTailBin, PSDMaxTailBin;
  int PSDNumTotalBins, PSDMinTotalBin, PSDMaxTotalBin;
  
  int PearsonChannel;
  bool PlotPearsonIntegration, IntegratePearson, IntegrateRawPearson, IntegrateFitToPearson;
  int PearsonLowerLimit, PearsonMiddleLimit, PearsonUpperLimit;

  int TotalDeuterons;
  
  int WaveformsToDesplice, DesplicedWaveformBuffer, DesplicedWaveformLength;
  
  vector<TGraph *> CalibrationManager;
  vector<bool> UseCalibrationManager;
  
  ClassDef(ADAQAnalysisParallelParameters,1);
};


class ADAQAnalysisParallelResults : public TObject
{
public:
  double TotalDeuterons;

  ClassDef(ADAQAnalysisParallelResults,1);
};


// The following enumerator is used to create unique integers that
// will be assigned as the "widget ID" to the ROOT widgets that make
// up the ADAQ analysis graphical interface. The widget IDs are used
// to connect the various signals -- issued by each widget when acted
// upon by the user -- to the appropriate slots -- the "action" that
// should be associated with each widget.
enum{
  MenuFileOpen_ID,
  MenuFileSaveSpectrumData_ID,
  MenuFilePrint_ID,
  MenuFileExit_ID,

  WaveformSelector_NEL_ID,

  PlotBaseline_CB_ID,
  BaselineCalcMin_NEL_ID,
  BaselineCalcMax_NEL_ID,

  XAxisLimits_THS_ID,
  YAxisLimits_DVS_ID,

  RawWaveform_RB_ID,
  BaselineSubtractedWaveform_RB_ID,
  ZeroSuppressionWaveform_RB_ID,

  PositiveWaveform_RB_ID,
  NegativeWaveform_RB_ID,

  MaxPeaks_NEL_ID,
  Sigma_NEL_ID,
  Resolution_NEL_ID,
  Floor_NEL_ID,

  UsePileupRejection_CB_ID,

  ResetXAxisLimits_TB_ID,
  ResetYAxisLimits_TB_ID,

  PlotTrigger_CB_ID,
  
  PlotNoiseCeiling_CB_ID,
  NoiseCeiling_NEL_ID,

  FindPeaks_CB_ID,
  PlotZeroSuppressionCeiling_CB_ID,
  ZeroSuppressionCeiling_NEL_ID,
  PlotFloor_CB_ID,
  PlotCrossings_CB_ID,
  PlotPeakIntegratingRegion_CB_ID,

  WaveformsToHistogram_NEL_ID,

  SpectrumNumBins_NEL_ID,
  SpectrumMinBin_NEL_ID,
  SpectrumMaxBin_NEL_ID,

  SpectrumCalibration_CB_ID,
  SpectrumCalibrationPoint_CBL_ID,
  SpectrumCalibrationEnergy_NEL_ID,
  SpectrumCalibrationPulseUnit_NEL_ID,
  SpectrumCalibrationSetPoint_TB_ID,
  SpectrumCalibrationCalibrate_TB_ID,
  SpectrumCalibrationReset_TB_ID,
  SpectrumCalibrationPlot_TB_ID,

  SpectrumNumPeaks_NEL_ID,
  SpectrumSigma_NEL_ID,
  SpectrumResolution_NEL_ID,
  SpectrumRangeMin_NEL_ID,
  SpectrumRangeMax_NEL_ID,
  SpectrumFindPeaks_CB_ID,
  SpectrumFindBackground_CB_ID,
  SpectrumIntegrationLimits_DHS_ID,
  SpectrumFindIntegral_CB_ID,
  SpectrumIntegralInCounts_CB_ID,
  SpectrumUseGaussianFit_CB_ID,

  OverrideTitles_CB_ID,
  Title_TEL_ID,
  XAxisTitle_TEL_ID,
  XAxisSize_NEL_ID,
  XAxisOffset_NEL_ID,
  YAxisTitle_TEL_ID,
  YAxisSize_NEL_ID,
  YAxisOffset_NEL_ID,
  
  ReplotWaveform_TB_ID,
  ReplotSpectrum_TB_ID,
  ReplotPSDHistogram_TB_ID,

  CreateSpectrum_TB_ID,

  SpectrumNoBackground_RB_ID,
  SpectrumWithBackground_RB_ID,
  SpectrumLessBackground_RB_ID,

  SpectrumNormalizePeakToCurrent_CB_ID,

  IntegratePearson_CB_ID,
  PlotPearsonIntegration_CB_ID,
  PearsonLowerLimit_NEL_ID,
  PearsonMiddleLimit_NEL_ID,
  PearsonUpperLimit_NEL_ID,
  
  PearsonPolarityPositive_RB_ID,
  PearsonPolarityNegative_RB_ID,

  IntegrateRawPearson_RB_ID,
  IntegrateFitToPearson_RB_ID,

  CountRate_TB_ID,

  PSDEnable_CB_ID,
  PSDPlotTailIntegration_CB_ID,
  PSDPeakOffset_NEL_ID,
  PSDTailOffset_NEL_ID,
  PSDCalculate_TB_ID,
  PSDPlot_TB_ID,
  
  DesplicedFileSelection_TB_ID,
  DesplicedFileCreation_TB_ID,

  ProcessingSeq_RB_ID,
  ProcessingPar_RB_ID,

  PSDPlotType_CBL_ID,

  PlotWaveformWithMarkers_CB_ID,

  Quit_TB_ID
};

#endif

