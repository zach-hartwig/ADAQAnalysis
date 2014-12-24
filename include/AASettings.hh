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

#ifndef __AASettings_hh__
#define __AASettings_hh__ 1

#include <TObject.h>
#include <TGraph.h>

#include <vector>
#include <string>
using namespace std;


class AASettings : public TObject
{
public:

  /////////////////////
  // Waveform frame  //
  /////////////////////
  
  int WaveformChannel, WaveformToPlot;
  bool RawWaveform, BSWaveform, ZSWaveform;
  int WaveformPolarity;
  
  bool PlotZeroSuppressionCeiling;
  int ZeroSuppressionCeiling;
  int ZeroSuppressionBuffer;

  bool FindPeaks, UseMarkovSmoothing;
  int MaxPeaks, Sigma, Floor;
  double Resolution;
  
  bool PlotFloor, PlotCrossings, PlotPeakIntegrationRegion;
  
  bool UsePileupRejection;
  
  bool PlotBaselineCalcRegion;
  int BaselineCalcMin, BaselineCalcMax;
  
  bool PlotTrigger, WaveformAnalysis;

  
  ////////////////////
  // Spectrum frame //
  ////////////////////

  int WaveformsToHistogram;
  int SpectrumNumBins;
  double SpectrumMinBin, SpectrumMaxBin;
  
  bool ADAQSpectrumTypePAS, ADAQSpectrumTypePHS;
  bool ADAQSpectrumIntTypeWW, ADAQSpectrumIntTypePF;
  
  bool ASIMSpectrumTypeEnergy;
  bool ASIMSpectrumTypeScintCreated;
  bool ASIMSpectrumTypeScintCounted;
  bool ASIMSpectrumLS, ASIMSpectrumES;

  int EnergyUnit;
  vector<TGraph *> SpectraCalibrations;
  vector<bool> UseSpectraCalibrations;
  
  
  ////////////////////
  // Analysis frame //
  ////////////////////

  bool FindBackground;
  int BackgroundIterations;
  bool BackgroundCompton, BackgroundSmoothing;
  int BackgroundMinBin, BackgroundMaxBin;
  int BackgroundDirection;
  int BackgroundFilterOrder;
  int BackgroundSmoothingWidth;

  bool PlotWithBackground, PlotLessBackground;

  bool SpectrumFindIntegral, SpectrumIntegralInCounts;
  bool SpectrumUseGaussianFit, SpectrumNormalizeToCurrent;
  bool SpectrumOverplotDerivative;
  

  ////////////////////
  // Graphics frame //
  ////////////////////
  
  bool WaveformLine, WaveformCurve, WaveformMarkers, WaveformBoth;
  int WaveformLineWidth;
  double WaveformMarkerSize;
  bool SpectrumLine, SpectrumCurve, SpectrumError, SpectrumBars;
  int SpectrumLineWidth, SpectrumFillStyle;
  bool HistogramStats, CanvasGrid;
  bool CanvasXAxisLog, CanvasYAxisLog, CanvasZAxisLog;
  bool PlotSpectrumDerivativeError, PlotAbsValueSpectrumDerivative, PlotYAxisWithAutoRange;
  bool OverrideGraphicalDefault;

  string PlotTitle, XAxisTitle, YAxisTitle, ZAxisTitle, PaletteTitle;
  double XSize, YSize, ZSize, PaletteSize;
  double XOffset, YOffset, ZOffset, PaletteOffset;
  int XDivs, YDivs, ZDivs;
  double PaletteX1, PaletteX2, PaletteY1, PaletteY2;


  //////////////////////
  // Processing frame //
  //////////////////////

  bool SeqProcessing, ParProcessing;
  int NumProcessors, UpdateFreq;

  bool PSDEnable;
  int PSDChannel, PSDWaveformsToDiscriminate;
  int PSDNumTotalBins, PSDMinTotalBin, PSDMaxTotalBin;
  int PSDNumTailBins, PSDMinTailBin, PSDMaxTailBin;
  int PSDThreshold, PSDTailOffset, PSDPeakOffset;

  int PSDPlotType;
  bool PSDPlotTailIntegrationRegion, EnableHistogramSlicing, PSDXSlice, PSDYSlice;

  bool PSDEnableFilterCreation, PSDEnableFilterUse;
  int PSDFilterPolarity;

  vector<TGraph *> PSDFilters;
  vector<bool> UsePSDFilters;

  double RFQPulseWidth, RFQRepRate;
  int RFQCountRateWaveforms;

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
  
  ClassDef(AASettings, 1);
};

#endif
