//////////////////////////////////////////////////////////////////////////////////////////
//
// name: AAInterface.hh
// date: 23 May 13
// auth: Zach Hartwig
//
// desc: 
//
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __AAInterface_hh__
#define __AAInterface_hh__ 1

// ROOT
#include <TROOT.h>
#include <TGMenu.h>
#include <TGButton.h>
#include <TGSlider.h>
#include <TGNumberEntry.h>
#include <TRootEmbeddedCanvas.h>
#include <TGDoubleSlider.h>
#include <TGTripleSlider.h>
#include <TColor.h>
#include <TLine.h>
#include <TBox.h>
#include <TGProgressBar.h>
#include <TObject.h>
#include <TRandom.h>
#include <TGMsgBox.h>
#include <TGTab.h>

// C++
#include <string>
#include <vector>
#include <cmath>
#include <fstream>
using namespace std;

// Boost
#ifndef __CINT__
#include <boost/array.hpp>
#endif

// ADAQ
#include "ADAQRootGUIClasses.hh"
#include "AATypes.hh"
#include "AASettings.hh"
#include "AAComputation.hh"
#include "AAGraphics.hh"

class AAInterface : public TGMainFrame
{

public:

  // Constructor/destructor
  AAInterface(string);
  ~AAInterface();

  // Create the ROOT graphical interface
  void CreateTheMainFrames();
  void FillWaveformFrame();
  void FillSpectrumFrame();
  void FillAnalysisFrame();
  void FillGraphicsFrame();
  void FillProcessingFrame();
  void FillCanvasFrame();

  // "Slot" methods to recieve and act upon ROOT widget "signals"
  void HandleMenu(int);
  void HandleTextButtons();
  void HandleCheckButtons();
  void HandleSliders(int);
  void HandleDoubleSliders();
  void HandleTripleSliderPointer();
  void HandleNumberEntries();
  void HandleRadioButtons();
  void HandleComboBox(int, int);
  void HandleCanvas(int, int, int, TObject *);
  void HandleTerminate();
  
  // Method to save all widget values in a storage class
  void SaveSettings(bool SaveToFile=false);
  
  void UpdateForADAQFile();
  void UpdateForACRONYMFile();

  // Method to alert the user via a ROOT message box
  void CreateMessageBox(string, string);

  void SetPearsonWidgetState(bool, EButtonState);
  void SetCalibrationWidgetState(bool, EButtonState);
  

private:
  TGTab *OptionsTabs_T;

  TGCompositeFrame *WaveformOptionsTab_CF;
  TGCompositeFrame *WaveformOptions_CF;
  
  TGCompositeFrame *SpectrumOptionsTab_CF;
  TGCompositeFrame *SpectrumOptions_CF;

  TGCompositeFrame *AnalysisOptionsTab_CF;
  TGCompositeFrame *AnalysisOptions_CF;

  TGCompositeFrame *GraphicsOptionsTab_CF;
  TGCompositeFrame *GraphicsOptions_CF;

  TGCompositeFrame *ProcessingOptionsTab_CF;
  TGCompositeFrame *ProcessingOptions_CF;

  TGHorizontalFrame *OptionsAndCanvas_HF;

  /////////////////////////////////////////////////////////
  // Widget objects for the WaveformOptions tabbed frame //
  /////////////////////////////////////////////////////////

  // Widgets for selecting waveform number and digitizer channel
  ADAQNumberEntryWithLabel *WaveformSelector_NEL;
  ADAQComboBoxWithLabel *ChannelSelector_CBL;

  // Widgets for setting general plot options
  TGButtonGroup *WaveformType_BG;
  TGRadioButton *RawWaveform_RB, *BaselineSubtractedWaveform_RB, *ZeroSuppressionWaveform_RB;

  // Widgets for setting the waveform (signal) polarity
  TGButtonGroup *WaveformPolarity_BG;
  TGRadioButton *PositiveWaveform_RB, *NegativeWaveform_RB;

  TGCheckButton *PlotTrigger_CB;

  // Widgets for controlling baseline calc. and plotting
  TGCheckButton *PlotBaseline_CB;
  ADAQNumberEntryWithLabel *BaselineCalcMin_NEL;
  ADAQNumberEntryWithLabel *BaselineCalcMax_NEL;

  // Widgets for controlling peak finding and plotting
  TGCheckButton *PlotZeroSuppressionCeiling_CB;
  ADAQNumberEntryWithLabel *ZeroSuppressionCeiling_NEL;

  TGCheckButton *FindPeaks_CB;
  ADAQNumberEntryWithLabel *MaxPeaks_NEL;
  ADAQNumberEntryWithLabel *Sigma_NEL;
  ADAQNumberEntryWithLabel *Resolution_NEL;
  ADAQNumberEntryWithLabel *Floor_NEL;
  TGCheckButton *PlotFloor_CB;
  TGCheckButton *PlotCrossings_CB;
  TGCheckButton *PlotPeakIntegratingRegion_CB;

  // Widget to enable/disable pileup rejection algorithm
  TGCheckButton *UsePileupRejection_CB;


  ///////////////////////////////////////////
  // Widgets for the spectrum tabbed frame //
  ///////////////////////////////////////////

  ADAQNumberEntryWithLabel *WaveformsToHistogram_NEL;

  ADAQNumberEntryWithLabel *SpectrumNumBins_NEL;
  ADAQNumberEntryWithLabel *SpectrumMinBin_NEL;
  ADAQNumberEntryWithLabel *SpectrumMaxBin_NEL;

  TGButtonGroup *ADAQSpectrumType_BG;
  TGRadioButton *ADAQSpectrumTypePAS_RB, *ADAQSpectrumTypePHS_RB;

  TGButtonGroup *ADAQSpectrumIntType_BG;
  TGRadioButton *ADAQSpectrumIntTypePF_RB;
  TGRadioButton *ADAQSpectrumIntTypeWW_RB;

  TGButtonGroup *ACROSpectrumType_BG;
  TGRadioButton *ACROSpectrumTypeEnergy_RB;
  TGRadioButton *ACROSpectrumTypeScintCreated_RB;
  TGRadioButton *ACROSpectrumTypeScintCounted_RB;
  
  TGButtonGroup *ACROSpectrumDetector_BG;
  TGRadioButton *ACROSpectrumLS_RB;
  TGRadioButton *ACROSpectrumES_RB;

  TGCheckButton *SpectrumCalibration_CB;
  TGRadioButton *SpectrumCalibrationManual_RB, *SpectrumCalibrationFixedEP_RB;
  ADAQComboBoxWithLabel *SpectrumCalibrationPoint_CBL;
  ADAQNumberEntryWithLabel *SpectrumCalibrationEnergy_NEL;
  ADAQNumberEntryWithLabel *SpectrumCalibrationPulseUnit_NEL;
  TGTextButton *SpectrumCalibrationSetPoint_TB;
  TGTextButton *SpectrumCalibrationCalibrate_TB;
  TGTextButton *SpectrumCalibrationPlot_TB;
  TGTextButton *SpectrumCalibrationReset_TB;

  TGCheckButton *SpectrumFindPeaks_CB;
  ADAQNumberEntryWithLabel *SpectrumNumPeaks_NEL;
  ADAQNumberEntryWithLabel *SpectrumSigma_NEL;
  ADAQNumberEntryWithLabel *SpectrumResolution_NEL;

  TGCheckButton *SpectrumFindBackground_CB;
  ADAQNumberEntryWithLabel *SpectrumRangeMin_NEL;
  ADAQNumberEntryWithLabel *SpectrumRangeMax_NEL;
  TGRadioButton *SpectrumWithBackground_RB;
  TGRadioButton *SpectrumLessBackground_RB;

  TGCheckButton *SpectrumFindIntegral_CB;
  TGCheckButton *SpectrumIntegralInCounts_CB;
  TGCheckButton *SpectrumUseGaussianFit_CB;
  TGCheckButton *SpectrumNormalizePeakToCurrent_CB;
  ADAQNumberEntryFieldWithLabel *SpectrumIntegral_NEFL;
  ADAQNumberEntryFieldWithLabel *SpectrumIntegralError_NEFL;
  
  TGCheckButton *SpectrumOverplotDerivative_CB;

  TGTextButton *CreateSpectrum_TB;


  ///////////////////////////////////////////
  // Widgets for the analysis tabbed frame //
  ///////////////////////////////////////////
  
  TGCheckButton *PSDEnable_CB;
  ADAQComboBoxWithLabel *PSDChannel_CBL;
  ADAQNumberEntryWithLabel *PSDThreshold_NEL;
  ADAQNumberEntryWithLabel *PSDPeakOffset_NEL;
  ADAQNumberEntryWithLabel *PSDTailOffset_NEL;
  ADAQNumberEntryWithLabel *PSDWaveforms_NEL;

  ADAQNumberEntryWithLabel *PSDNumTailBins_NEL, *PSDMinTailBin_NEL, *PSDMaxTailBin_NEL;
  ADAQNumberEntryWithLabel *PSDNumTotalBins_NEL, *PSDMinTotalBin_NEL, *PSDMaxTotalBin_NEL;

  ADAQComboBoxWithLabel *PSDPlotType_CBL;

  TGCheckButton *PSDPlotTailIntegration_CB;
  
  TGCheckButton *PSDEnableHistogramSlicing_CB;
  TGRadioButton *PSDHistogramSliceX_RB, *PSDHistogramSliceY_RB;
  
  TGTextButton *PSDCalculate_TB;
  
  TGCheckButton *PSDEnableFilterCreation_CB;
  TGCheckButton *PSDEnableFilter_CB;
  TGRadioButton *PSDPositiveFilter_RB, *PSDNegativeFilter_RB;
  TGTextButton *PSDClearFilter_TB;
  
  ADAQNumberEntryWithLabel *RFQPulseWidth_NEL;
  ADAQNumberEntryWithLabel *RFQRepRate_NEL;
  ADAQNumberEntryWithLabel *CountRateWaveforms_NEL;
  TGTextButton *CalculateCountRate_TB;
  ADAQNumberEntryFieldWithLabel *InstCountRate_NEFL, *AvgCountRate_NEFL;


  ///////////////////////////////////////////
  // Widgets for the graphics tabbed frame //
  ///////////////////////////////////////////

  TGCheckButton *OverrideTitles_CB;

  ADAQTextEntryWithLabel *Title_TEL;
  ADAQTextEntryWithLabel *XAxisTitle_TEL;
  ADAQTextEntryWithLabel *YAxisTitle_TEL;
  ADAQTextEntryWithLabel *ZAxisTitle_TEL;
  ADAQTextEntryWithLabel *PaletteAxisTitle_TEL;

  ADAQNumberEntryWithLabel *XAxisSize_NEL, *XAxisOffset_NEL, *XAxisDivs_NEL;
  ADAQNumberEntryWithLabel *YAxisSize_NEL, *YAxisOffset_NEL, *YAxisDivs_NEL;
  ADAQNumberEntryWithLabel *ZAxisSize_NEL, *ZAxisOffset_NEL, *ZAxisDivs_NEL;
  ADAQNumberEntryWithLabel *PaletteAxisSize_NEL, *PaletteAxisOffset_NEL;
  ADAQNumberEntryWithLabel *PaletteX1_NEL, *PaletteX2_NEL, *PaletteY1_NEL, *PaletteY2_NEL;
  
  TGButtonGroup *WaveformDrawOptions_BG;
  TGRadioButton *DrawWaveformWithCurve_RB, *DrawWaveformWithMarkers_RB, *DrawWaveformWithBoth_RB;
  
  TGButtonGroup *SpectrumDrawOptions_BG;
  TGRadioButton *DrawSpectrumWithCurve_RB, *DrawSpectrumWithMarkers_RB, *DrawSpectrumWithBars_RB;
  
  TGCheckButton *SetStatsOff_CB;
  TGCheckButton *PlotVerticalAxisInLog_CB;

  TGCheckButton *PlotSpectrumDerivativeError_CB;
  TGCheckButton *PlotAbsValueSpectrumDerivative_CB;

  TGCheckButton *AutoYAxisRange_CB;
  
  TGTextButton *ReplotWaveform_TB;
  TGTextButton *ReplotSpectrum_TB;
  TGTextButton *ReplotSpectrumDerivative_TB;
  TGTextButton *ReplotPSDHistogram_TB;


  /////////////////////////////////////////////
  // Widgets for the processing tabbed frame //
  /////////////////////////////////////////////

  TGButtonGroup *ProcessingType_BG;
  TGRadioButton *ProcessingSeq_RB, *ProcessingPar_RB;
  ADAQNumberEntryWithLabel *NumProcessors_NEL;
  ADAQNumberEntryWithLabel *UpdateFreq_NEL;

  TGCheckButton *IntegratePearson_CB;
  ADAQComboBoxWithLabel *PearsonChannel_CBL;
  TGRadioButton *PearsonPolarityPositive_RB, *PearsonPolarityNegative_RB;
  TGCheckButton *PlotPearsonIntegration_CB;
  TGRadioButton *IntegrateRawPearson_RB, *IntegrateFitToPearson_RB;
  ADAQNumberEntryWithLabel *PearsonLowerLimit_NEL, *PearsonMiddleLimit_NEL, *PearsonUpperLimit_NEL;
  ADAQNumberEntryFieldWithLabel *DeuteronsInWaveform_NEFL;
  ADAQNumberEntryFieldWithLabel *DeuteronsInTotal_NEFL;

  TGTextButton *DesplicedFileSelection_TB;
  TGTextEntry *DesplicedFileName_TE;
  ADAQNumberEntryWithLabel *DesplicedWaveformBuffer_NEL;
  ADAQNumberEntryWithLabel *DesplicedWaveformNumber_NEL;
  ADAQNumberEntryWithLabel *DesplicedWaveformLength_NEL;
  TGTextButton *DesplicedFileCreation_TB;


  ///////////////////////////////////////////
  // Widget objects for the "Canvas" frame //
  ///////////////////////////////////////////

  // Widget to display open ADAQ ROOT file statistics
  TGTextEntry *FileName_TE;
  TGNumberEntryField *Waveforms_NEL, *RecordLength_NEL;
  TGLabel *WaveformLabel_L, *RecordLengthLabel_L;

  // The canvas to display all plots
  TRootEmbeddedCanvas *Canvas_EC;

  // Triple and double slider widgets for zooming in X and Y (the
  // pointer in the triple slider is used for spectrum calibration)
  TGTripleHSlider *XAxisLimits_THS;
  TGDoubleVSlider *YAxisLimits_DVS;

  // Slider widget for quickly scrolling through accessible waveforms
  TGHSlider *WaveformSelector_HS;

  TGDoubleHSlider *SpectrumIntegrationLimits_DHS;

  // A progress bar! Hot stuff.
  TGHProgressBar *ProcessingProgress_PB;

  // Widget for quiting the GUI
  TGTextButton *Quit_TB;


  //////////////////////////////////////
  // Member variables for general use //
  //////////////////////////////////////

  // Number of data channels in the ADAQ ROOT files
  const int NumDataChannels;

  // Number of processors/threads available on current system
  const int NumProcessors;

  // Variables relating to files (paths, bools)
  string DataDirectory, PrintDirectory, DesplicedDirectory, HistogramDirectory;
  bool ADAQFileLoaded, ACRONYMFileLoaded;
  string ADAQFileName, ACRONYMFileName;

  // The class which holds all ROOT widget settings
  AASettings *ADAQSettings;
  string ADAQSettingsFileName;

  // The managers 
  TColor *ColorMgr;
  AAComputation *ComputationMgr;
  AAGraphics *GraphicsMgr;
  
  // Define the class to ROOT
  ClassDef(AAInterface, 1);
};

#endif
