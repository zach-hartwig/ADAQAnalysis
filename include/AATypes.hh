/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                           Copyright (C) 2012-2015                           //
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
// name: AATypes.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: AATypes is a header file that contains global type
//       definitions that are used in ADAQAnalysis. These include very
//       useful structs and enumerators, and, in particular, the very
//       large widget ID enumerator that is used to connect signals
//       and slots for widgets on the GUI.
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef __AATypes_hh__
#define __AATypes_hh__ 1

#include <TGraph.h>

#include <vector>
#include <string>
using namespace std;


///////////////
// Stuctures //
///////////////

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


// Structure that contains information on a single calibration point
// for a single channel. For each calibration point, a structure is
// filled with the relevant information and pushed back into a vector
// for later processing
struct ADAQChannelCalibrationData{
  vector<int> PointID; // Unique integer ID of the calibration point
  vector<double> Energy; // Energy of calibration peak set by user [desired energy unit]
  vector<double> PulseUnit; // Pulse unit of calibration peak set by user [ADC]
};


/////////////////
// Enumerators //
/////////////////

// An enumerator that specifies what type of plot is presently
// contained in the main embedded canvas
enum CanvasContentTypes{zEmpty, zWaveform, zSpectrum, zSpectrumDerivative, zPSDHistogram};

enum PeakFindingAlgorithm{zPeakFinder, zWholeWaveform};

// The following enumerator is used to create unique integers that
// will be assigned as the "widget ID" to the ROOT widgets that make
// up the ADAQ analysis graphical interface. The widget IDs are used
// to connect the various signals -- issued by each widget when acted
// upon by the user -- to the appropriate slots -- the "action" that
// should be associated with each widget. The order of the enumerators
// is arbitrary from a code perspective but is organized here by tab
// and then as widgets appear in each tab for convenience.
enum{

  ////////////////////////////
  // Values for the menu frame
  
  MenuFileOpenADAQ_ID,
  MenuFileOpenASIM_ID,
  MenuFileLoadSpectrum_ID,
  MenuFileLoadPSDHistogram_ID,
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

  FindPeaks_CB_ID,
  UseMarkovSmoothing_CB_ID,
  MaxPeaks_NEL_ID,
  Sigma_NEL_ID,
  Resolution_NEL_ID,
  Floor_NEL_ID,
  PlotFloor_CB_ID,
  PlotCrossings_CB_ID,
  PlotPeakIntegratingRegion_CB_ID,

  PlotAnalysisRegion_CB_ID,
  AnalysisRegionMin_NEL_ID,
  AnalysisRegionMax_NEL_ID,
  
  PlotBaselineRegion_CB_ID,
  BaselineRegionMin_NEL_ID,
  BaselineRegionMax_NEL_ID,

  PlotZeroSuppressionCeiling_CB_ID,
  ZeroSuppressionCeiling_NEL_ID,
  ZeroSuppressionBuffer_NEL_ID,

  PlotTrigger_CB_ID,
  UsePileupRejection_CB_ID,
  AutoYAxisRange_CB_ID,
  WaveformAnalysis_CB_ID,

  
  /////////////////////////////////////////
  // Values for the "Spectrum" tabbed frame

  WaveformsToHistogram_NEL_ID,
  SpectrumNumBins_NEL_ID,
  SpectrumMinBin_NEL_ID,
  SpectrumMaxBin_NEL_ID,
  
  ADAQSpectrumTypePAS_RB_ID,
  ADAQSpectrumTypePHS_RB_ID,
  ADAQSpectrumAlgorithmSMS_RB_ID,
  ADAQSpectrumAlgorithmPF_RB_ID,
  ADAQSpectrumAlgorithmWD_RB_ID,

  ASIMSpectrumTypeEnergy_RB_ID,
  ASIMSpectrumTypePhotonsCreated_RB_ID,
  ASIMSpectrumTypePhotonsDetected_RB_ID,
  ASIMEventTree_CB_ID,

  SpectrumCalibration_CB_ID,
  SpectrumCalibrationStandard_RB_ID,
  SpectrumCalibrationEdgeFinder_RB_ID,
  SpectrumCalibrationType_CBL_ID,
  SpectrumCalibrationMin_NEL_ID,
  SpectrumCalibrationMax_NEL_ID,
  SpectrumCalibrationPoint_CBL_ID,
  SpectrumCalibrationEnergy_NEL_ID,
  SpectrumCalibrationPulseUnit_NEL_ID,
  SpectrumCalibrationSetPoint_TB_ID,
  SpectrumCalibrationCalibrate_TB_ID,
  SpectrumCalibrationReset_TB_ID,
  SpectrumCalibrationPlot_TB_ID,
  SpectrumCalibrationLoad_TB_ID,
  
  ProcessSpectrum_TB_ID,
  CreateSpectrum_TB_ID,
  
  
  /////////////////////////////////////////
  // Values for the "Analysis" tabbed frame

  SpectrumFindBackground_CB_ID,
  SpectrumBackgroundIterations_NEL_ID,
  SpectrumBackgroundCompton_CB_ID,
  SpectrumBackgroundSmoothing_CB_ID,
  SpectrumRangeMin_NEL_ID,
  SpectrumRangeMax_NEL_ID,
  SpectrumBackgroundDirection_CBL_ID,
  SpectrumBackgroundFilterOrder_CBL_ID,
  SpectrumBackgroundSmoothingWidth_CBL_ID,
  SpectrumNoBackground_RB_ID,
  SpectrumWithBackground_RB_ID,
  SpectrumLessBackground_RB_ID,

  SpectrumFindIntegral_CB_ID,
  SpectrumIntegralInCounts_CB_ID,

  SpectrumUseGaussianFit_CB_ID,
  SpectrumNormalizePeakToCurrent_CB_ID,

  SpectrumAnalysisLowerLimit_NEL_ID,
  SpectrumAnalysisUpperLimit_NEL_ID,

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

  DrawWaveformWithLine_RB_ID,
  DrawWaveformWithCurve_RB_ID,
  DrawWaveformWithMarkers_RB_ID,
  DrawWaveformWithBoth_RB_ID,
  WaveformColor_TB_ID,
  WaveformLineWidth_NEL_ID,
  WaveformMarkerSize_NEL_ID,

  DrawSpectrumWithLine_RB_ID,
  DrawSpectrumWithCurve_RB_ID,
  DrawSpectrumWithError_RB_ID,
  DrawSpectrumWithBars_RB_ID,
  SpectrumLineColor_TB_ID,
  SpectrumFillColor_TB_ID,
  SpectrumLineWidth_NEL_ID,
  SpectrumFillStyle_NEL_ID,

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
  
  
  ///////////////////////////////////////////
  // Values for the "Processing" tabbed frame

  ProcessingSeq_RB_ID,
  ProcessingPar_RB_ID,

  PSDEnable_CB_ID,
  
  PSDAlgorithmPF_RB_ID,
  PSDAlgorithmSMS_RB_ID,
  PSDAlgorithmWD_RB_ID,

  PSDTotalStart_NEL_ID,
  PSDTotalStop_NEL_ID,
  PSDTailStart_NEL_ID,
  PSDTailStop_NEL_ID,
  PSDPlotType_CBL_ID,
  PSDPlotPalette_CBL_ID,
  PSDPlotTailIntegration_CB_ID,
  PSDYAxisTail_RB_ID,
  PSDYAxisTailTotal_RB_ID,

  PSDEnableHistogramSlicing_CB_ID,
  PSDHistogramSliceX_RB_ID,
  PSDHistogramSliceY_RB_ID,

  PSDEnableRegionCreation_CB_ID,
  PSDEnableRegion_CB_ID,
  PSDInsideRegion_RB_ID,
  PSDOutsideRegion_RB_ID,
  PSDCreateRegion_TB_ID,
  PSDClearRegion_TB_ID,
  
  ProcessPSDHistogram_TB_ID,
  CreatePSDHistogram_TB_ID,

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
  WaveformSelector_HS_ID,
  SpectrumIntegrationLimits_DHS_ID,

  Quit_TB_ID
};

#endif

