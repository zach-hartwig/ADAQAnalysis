#ifndef __ADAQAnalysisSettings_hh__
#define __ADAQAnalysisSettings_hh__ 1

#include <TObject.h>
#include <TGraph.h>

#include <vector>
#include <string>
using namespace std;


class ADAQAnalysisSettings : public TObject
{
public:

  // Waveform frame
  
  int WaveformChannel, WaveformToPlot;
  bool RawWaveform, BSWaveform, ZSWaveform;
  int WaveformPolarity;
  
  bool PlotZeroSuppressionCeiling;
  int ZeroSuppressionCeiling;

  bool FindPeaks;
  int MaxPeaks, Sigma, Floor;
  double Resolution;
  
  bool PlotFloor, PlotCrossings, PlotPeakIntegrationRegion;
  
  bool UsePileupRejection;
  
  bool PlotBaselineCalcRegion;
  int BaselineCalcMin, BaselineCalcMax;
  
  bool PlotTrigger;

  
  // Spectrum frame

  int WaveformsToHistogram;
  int SpectrumNumBins, SpectrumMinBin, SpectrumMaxBin;
  bool SpectrumTypePAS, SpectrumTypePHS;
  bool IntegrationTypeWholeWaveform, IntegrationTypePeakFinder;

  bool UseManualCalibration, UseEPCalibration;
  vector<TGraph *> CalibrationManager;
  vector<bool> UseCalibrationManager;

  bool FindBackground;
  int BackgroundMinBin, BackgroundMaxBin;
  bool PlotWithBackground, PlotLessBackground;
  
  bool SpectrumFindIntegral, SpectrumIntegralInCounts;
  bool SpectrumUseGaussianFit, SpectrumNormalizeToCurrent;
  bool SpectrumOverplotDerivative;
  

  // Analysis

  bool PSDEnable;
  int PSDChannel, PSDWaveformsToDiscriminate;
  int PSDNumTotalBins, PSDMinTotalBin, PSDMaxTotalBin;
  int PSDNumTailBins, PSDMinTailBin, PSDMaxTailBin;
  int PSDThreshold, PSDTailOffset, PSDPeakOffset;

  int PSDPlotType;
  bool PSDPlotTailIntegrationRegion, EnableHistogramSlicing, PSDXSlice, PSDYSlice;

  bool PSDEnableFilterCreation, PSDEnableFilterUse;
  int PSDFilterPolarity;

  vector<TGraph *> PSDFilterManager;
  vector<bool> UsePSDFilterManager;

  double RFQPulseWidth, RFQRepRate;
  int RFQCountRateWaveforms;


  // Graphics
  
  bool WaveformCurve, WaveformMarkers, WaveformBoth;
  bool SpectrumCurve, SpectrumMarkers, SpectrumBars;
  bool StatsOff, PlotVerticalAxisInLog;
  bool PlotSpectrumDerivativeError, PlotAbsValueSpectrumDerivative, PlotYAxisWithAutoRange;
  bool OverrideGraphicalDefault;

  string PlotTitle, XAxisTitle, YAxisTitle, ZAxisTitle, PaletteTitle;
  double XSize, YSize, ZSize, PaletteSize;
  double XOffset, YOffset, ZOffset, PaletteOffset;
  int XDivs, YDivs, ZDivs;
  double PaletteX1, PaletteX2, PaletteY1, PaletteY2;


  // Processing

  bool SequentialProcessing, ParallelProcessing;
  int NumProcessors, UpdateFreq;

  bool IntegratePearson;
  int PearsonChannel, PearsonPolarity;
  bool IntegrateRawPearson, IntegrateFitToPearson;
  int PearsonLowerLimit, PearsonMiddleLimit, PearsonUpperLimit;
  bool PlotPearsonIntegration;

  int TotalDeuterons;
  
  int WaveformsToDesplice, DesplicedWaveformBuffer, DesplicedWaveformLength;
  string DesplicedFileName;


  // Canvas

  double XAxisMin, XAxisMax, XAxisPtr;
  double YAxisMin, YAxisMax;

  int WaveformSelector;
  
  double SpectrumIntegrationMin, SpectrumIntegrationMax;
  
  
  // General
  
  string ADAQFileName;
  string ACRONYMFileName;
  
  ClassDef(ADAQAnalysisSettings, 1);
};

#endif
