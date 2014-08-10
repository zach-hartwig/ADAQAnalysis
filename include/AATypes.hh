//////////////////////////////////////////////////////////////////////////////////////////
//
// name: AATypes.hh
// date: 22 May 13
// auth: Zach Hartwig
// 
// desc: 
//
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __AATypes_hh__
#define __AATypes_hh__ 1

#include <TGraph.h>

#include <vector>
#include <string>
using namespace std;

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
  bool PSDFilterFlag; // Flag to indicate whether current peak should be filtered out due to pulse shape
  
  // Initialization for the variables
  PeakInfoStruct() : PeakID(-1),
		     PeakPosX(-1),
		     PeakPosY(-1),
		     PeakLimit_Lower(-1),
		     PeakLimit_Upper(-1),
		     AnalyzeFlag(true),
		     PileupFlag(false), // Convention : true == pileup; false == no pile up
		     PSDFilterFlag(false) // Convention : true == filter out; false == do not filter out
  {}
};


enum CanvasContentTypes{zEmpty, zWaveform, zSpectrum, zSpectrumDerivative, zPSDHistogram};

// Structure that contains information on a single calibration point
// for a single channel. For each calibration point, a structure is
// filled with the relevant information and pushed back into a vector
// for later processing
struct ADAQChannelCalibrationData{
  vector<int> PointID; // Unique integer ID of the calibration point
  vector<double> Energy; // Energy of calibration peak set by user [desired energy unit]
  vector<double> PulseUnit; // Pulse unit of calibration peak set by user [ADC]
};


// The following enumerator is used to create unique integers that
// will be assigned as the "widget ID" to the ROOT widgets that make
// up the ADAQ analysis graphical interface. The widget IDs are used
// to connect the various signals -- issued by each widget when acted
// upon by the user -- to the appropriate slots -- the "action" that
// should be associated with each widget.
enum{

  ////////////////////////////
  // Values for the menu frame
  
  MenuFileOpenADAQ_ID,
  MenuFileOpenACRONYM_ID,
  MenuFileSaveWaveform_ID,
  MenuFileSaveSpectrum_ID,
  MenuFileSaveSpectrumBackground_ID,
  MenuFileSaveSpectrumDerivative_ID,
  MenuFileSaveSpectrumCalibration_ID,
  MenuFileSavePSDHistogram_ID,
  MenuFileSavePSDHistogramSlice_ID,
  MenuFilePrint_ID,
  MenuFileExit_ID,
  

  /////////////////////////////////////////
  // Values for the "Waveform" tabbed frame

  ChannelSelector_CBL_ID,
  WaveformSelector_NEL_ID,

  RawWaveform_RB_ID,
  BaselineSubtractedWaveform_RB_ID,
  ZeroSuppressionWaveform_RB_ID,

  PositiveWaveform_RB_ID,
  NegativeWaveform_RB_ID,

  PlotZeroSuppressionCeiling_CB_ID,
  ZeroSuppressionCeiling_NEL_ID,

  FindPeaks_CB_ID,
  UseMarkovSmoothing_CB_ID,
  MaxPeaks_NEL_ID,
  Sigma_NEL_ID,
  Resolution_NEL_ID,
  Floor_NEL_ID,

  PlotFloor_CB_ID,
  PlotCrossings_CB_ID,
  PlotPeakIntegratingRegion_CB_ID,

  UsePileupRejection_CB_ID,

  AutoYAxisRange_CB_ID,

  PlotBaseline_CB_ID,
  BaselineCalcMin_NEL_ID,
  BaselineCalcMax_NEL_ID,

  PlotTrigger_CB_ID,

  WaveformAnalysis_CB_ID,

  
  /////////////////////////////////////////
  // Values for the "Spectrum" tabbed frame

  WaveformsToHistogram_NEL_ID,

  SpectrumNumBins_NEL_ID,
  SpectrumMinBin_NEL_ID,
  SpectrumMaxBin_NEL_ID,


  ACROSpectrumLS_RB_ID,
  ACROSpectrumES_RB_ID,

  SpectrumCalibration_CB_ID,
  SpectrumCalibrationStandard_RB_ID,
  SpectrumCalibrationEdgeFinder_RB_ID,
  SpectrumCalibrationPoint_CBL_ID,
  SpectrumCalibrationEnergy_NEL_ID,
  SpectrumCalibrationPulseUnit_NEL_ID,
  SpectrumCalibrationSetPoint_TB_ID,
  SpectrumCalibrationCalibrate_TB_ID,
  SpectrumCalibrationReset_TB_ID,
  SpectrumCalibrationPlot_TB_ID,
  SpectrumCalibrationLoad_TB_ID,

  SpectrumNumPeaks_NEL_ID,
  SpectrumSigma_NEL_ID,
  SpectrumResolution_NEL_ID,

  SpectrumRangeMin_NEL_ID,
  SpectrumRangeMax_NEL_ID,
  SpectrumFindPeaks_CB_ID,

  SpectrumFindBackground_CB_ID,
  SpectrumBackgroundIterations_NEL_ID,
  SpectrumBackgroundCompton_CB_ID,
  SpectrumBackgroundSmoothing_CB_ID,
  SpectrumBackgroundDirection_CBL_ID,
  SpectrumBackgroundFilterOrder_CBL_ID,
  SpectrumBackgroundSmoothingWidth_CBL_ID,

  SpectrumNoBackground_RB_ID,
  SpectrumWithBackground_RB_ID,
  SpectrumLessBackground_RB_ID,
  SpectrumIntegrationLimits_DHS_ID,

  SpectrumFindIntegral_CB_ID,
  SpectrumIntegralInCounts_CB_ID,
  SpectrumUseGaussianFit_CB_ID,
  SpectrumNormalizePeakToCurrent_CB_ID,

  SpectrumOverplotDerivative_CB_ID,

  CreateSpectrum_TB_ID,


  /////////////////////////////////////////
  // Values for the "Analysis" tabbed frame


  PSDEnable_CB_ID,

  PSDPeakOffset_NEL_ID,
  PSDTailOffset_NEL_ID,

  PSDPlotType_CBL_ID,

  PSDPlotTailIntegration_CB_ID,

  PSDEnableHistogramSlicing_CB_ID,
  PSDHistogramSliceX_RB_ID,
  PSDHistogramSliceY_RB_ID,

  PSDEnableFilterCreation_CB_ID,
  PSDEnableFilter_CB_ID,
  PSDPositiveFilter_RB_ID,
  PSDNegativeFilter_RB_ID,

  PSDClearFilter_TB_ID,
  PSDCalculate_TB_ID,

  EAEnable_CB_ID,
  EASpectrumType_CBL_ID,

  EAGammaEDep_NEL_ID,
  EAEscapePeaks_CB_ID,

  EAEJ301_RB_ID,
  EAEJ309_RB_ID,
  EALightConversionFactor_NEL_ID,
  EAErrorWidth_NEL_ID,
  EAElectronEnergy_NEL_ID,
  EAGammaEnergy_NEL_ID,
  EAProtonEnergy_NEL_ID,
  EAAlphaEnergy_NEL_ID,
  EACarbonEnergy_NEL_ID,

  CountRate_TB_ID,


  /////////////////////////////////////////
  // Values for the "Graphics" tabbed frame

  OverrideTitles_CB_ID,

  Title_TEL_ID,

  XAxisTitle_TEL_ID,
  XAxisSize_NEL_ID,
  XAxisOffset_NEL_ID,
  XAxisDivs_NEL_ID,

  YAxisTitle_TEL_ID,
  YAxisSize_NEL_ID,
  YAxisOffset_NEL_ID,
  YAxisDivs_NEL_ID,

  ZAxisTitle_TEL_ID,
  ZAxisSize_NEL_ID,
  ZAxisOffset_NEL_ID,
  ZAxisDivs_NEL_ID,

  PaletteAxisTitle_TEL_ID,
  PaletteAxisSize_NEL_ID,
  PaletteAxisOffset_NEL_ID,
  PaletteAxisDivs_NEL_ID,
  PaletteX1_NEL_ID,
  PaletteX2_NEL_ID,
  PaletteY1_NEL_ID,
  PaletteY2_NEL_ID,
  
  DrawWaveformWithCurve_RB_ID,
  DrawWaveformWithMarkers_RB_ID,
  DrawWaveformWithBoth_RB_ID,
  
  DrawSpectrumWithCurve_RB_ID,
  DrawSpectrumWithMarkers_RB_ID,
  DrawSpectrumWithError_RB_ID,
  DrawSpectrumWithBars_RB_ID,

  HistogramStats_CB_ID,
  CanvasGrid_CB_ID,
  CanvasXAxisLog_CB_ID,
  CanvasYAxisLog_CB_ID,
  CanvasZAxisLog_CB_ID,

  PlotSpectrumDerivativeError_CB_ID,
  PlotAbsValueSpectrumDerivative_CB_ID,

  ReplotWaveform_TB_ID,
  ReplotSpectrum_TB_ID,
  ReplotSpectrumDerivative_TB_ID,
  ReplotPSDHistogram_TB_ID,

  
  ///////////////////////////////////////////
  // Values for the "Processing" tabbed frame

  ProcessingSeq_RB_ID,
  ProcessingPar_RB_ID,

  IntegratePearson_CB_ID,
  PlotPearsonIntegration_CB_ID,
  PearsonLowerLimit_NEL_ID,
  PearsonMiddleLimit_NEL_ID,
  PearsonUpperLimit_NEL_ID,
  
  PearsonPolarityPositive_RB_ID,
  PearsonPolarityNegative_RB_ID,

  IntegrateRawPearson_RB_ID,
  IntegrateFitToPearson_RB_ID,

  DesplicedFileSelection_TB_ID,
  DesplicedFileCreation_TB_ID,

  
  //////////////////////////////////////////
  // Values for the "Canvas + Sliders" frame

  XAxisLimits_THS_ID,
  YAxisLimits_DVS_ID,

  Quit_TB_ID
};

#endif

