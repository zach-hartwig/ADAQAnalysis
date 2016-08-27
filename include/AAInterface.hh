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
// name: AAInterface.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAInterface class is responsible for building the entire
//       graphical user interface, including the creation and
//       organization of all widgets and the connection of the widgets
//       to the "slot" methods that process the generated widget
//       "signals". The slot methods are organized into six associated
//       classes that are named "AA*Slots", where * refers to one of
//       the five GUI tabs or the remaining non-tab methods. The slot
//       handler classes are friend class data members of AAInterface
//       in order that they may manipulate other data members.
//
/////////////////////////////////////////////////////////////////////////////////

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
#include <TRandom3.h>
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
#include "ADAQRootClasses.hh"

// ADAQAnalysis
#include "AATypes.hh"
#include "AASettings.hh"
#include "AAComputation.hh"
#include "AAInterpolation.hh"

class AAWaveformSlots;
class AASpectrumSlots;
class AAAnalysisSlots;
class AAPSDSlots;
class AAGraphics;
class AAGraphicsSlots;
class AAProcessingSlots;
class AANontabSlots;


class AAInterface : public TGMainFrame
{
  friend class AAWaveformSlots;
  friend class AASpectrumSlots;
  friend class AAAnalysisSlots;
  friend class AAPSDSlots;
  friend class AAGraphicsSlots;
  friend class AAProcessingSlots;
  friend class AANontabSlots;

public:

  // Constructor/destructor
  AAInterface(string);
  ~AAInterface();

  // Create the ROOT graphical interface
  void CreateTheMainFrames();
  void FillWaveformFrame();
  void FillSpectrumFrame();
  void FillAnalysisFrame();
  void FillPSDFrame();
  void FillGraphicsFrame();
  void FillProcessingFrame();
  void FillCanvasFrame();

  // Method to save all widget values in a storage class
  void SaveSettings(bool SaveToFile=false);
  
  // Methods to update the interface after different actions
  void UpdateForADAQFile();
  void UpdateForASIMFile();
  void UpdateForSpectrumCreation();
  void UpdateForPSDHistogramCreation();
  void UpdateForPSDHistogramSlicingFinished();

  // Method to alert the user via a ROOT message box
  void CreateMessageBox(string, string);

  // Methods to enable/disable widgets after different actions
  void SetPeakFindingWidgetState(bool, EButtonState);
  void SetPSDWidgetState(bool, EButtonState);
  void SetCalibrationWidgetState(bool, EButtonState);
  void SetEAGammaWidgetState(bool, EButtonState);
  void SetEANeutronWidgetState(bool, EButtonState);
  void SetSpectrumBackgroundWidgetState(bool, EButtonState);
  

private:
  TGTab *OptionsTabs_T;

  TGCompositeFrame *WaveformOptionsTab_CF;
  TGCompositeFrame *WaveformOptions_CF;
  
  TGCompositeFrame *SpectrumOptionsTab_CF;
  TGCompositeFrame *SpectrumOptions_CF;

  TGCompositeFrame *AnalysisOptionsTab_CF;
  TGCompositeFrame *AnalysisOptions_CF;
  
  TGCompositeFrame *PSDOptionsTab_CF;
  TGCompositeFrame *PSDOptions_CF;

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

  TGCheckButton *PlotAnalysisRegion_CB;
  ADAQNumberEntryWithLabel *AnalysisRegionMin_NEL;
  ADAQNumberEntryWithLabel *AnalysisRegionMax_NEL;

  TGCheckButton *PlotTrigger_CB;
  ADAQNumberEntryFieldWithLabel *TriggerLevel_NEFL;
  
  // Widgets for controlling baseline calc. and plotting
  TGCheckButton *PlotBaselineRegion_CB;
  ADAQNumberEntryWithLabel *BaselineRegionMin_NEL;
  ADAQNumberEntryWithLabel *BaselineRegionMax_NEL;

  // Widgets for controlling peak finding and plotting
  TGCheckButton *PlotZeroSuppressionCeiling_CB;
  ADAQNumberEntryWithLabel *ZeroSuppressionCeiling_NEL;
  ADAQNumberEntryWithLabel *ZeroSuppressionBuffer_NEL;

  TGCheckButton *FindPeaks_CB, *UseMarkovSmoothing_CB;
  ADAQNumberEntryWithLabel *MaxPeaks_NEL;
  ADAQNumberEntryWithLabel *Sigma_NEL;
  ADAQNumberEntryWithLabel *Resolution_NEL;
  ADAQNumberEntryWithLabel *Floor_NEL;
  TGCheckButton *PlotFloor_CB;
  TGCheckButton *PlotCrossings_CB;
  TGCheckButton *PlotPeakIntegratingRegion_CB;

  // Widget to enable/disable pileup rejection algorithm
  TGCheckButton *UsePileupRejection_CB;
  
  TGCheckButton *AutoYAxisRange_CB;

  TGCheckButton *WaveformAnalysis_CB;
  ADAQNumberEntryWithLabel *WaveformIntegral_NEL, *WaveformHeight_NEL;


  ///////////////////////////////////////////
  // Widgets for the spectrum tabbed frame //
  ///////////////////////////////////////////

  ADAQNumberEntryWithLabel *WaveformsToHistogram_NEL;

  ADAQNumberEntryWithLabel *SpectrumNumBins_NEL;
  ADAQNumberEntryWithLabel *SpectrumMinBin_NEL;
  ADAQNumberEntryWithLabel *SpectrumMaxBin_NEL;
  ADAQNumberEntryWithLabel *SpectrumMinThresh_NEL;
  ADAQNumberEntryWithLabel *SpectrumMaxThresh_NEL;

  //TGButtonGroup *ADAQSpectrumType_BG;
  TGRadioButton *ADAQSpectrumTypePAS_RB, *ADAQSpectrumTypePHS_RB;

  //TGButtonGroup *ADAQSpectrumAlgorithm_BG;
  TGRadioButton *ADAQSpectrumAlgorithmSMS_RB;
  TGRadioButton *ADAQSpectrumAlgorithmPF_RB;
  TGRadioButton *ADAQSpectrumAlgorithmWD_RB;

  //TGButtonGroup *ASIMSpectrumType_BG;
  TGRadioButton *ASIMSpectrumTypeEnergy_RB;
  TGRadioButton *ASIMSpectrumTypePhotonsCreated_RB;
  TGRadioButton *ASIMSpectrumTypePhotonsDetected_RB;

  TGComboBox *ASIMEventTree_CB;
  
  TGCheckButton *SpectrumCalibration_CB;
  TGRadioButton *SpectrumCalibrationStandard_RB, *SpectrumCalibrationEdgeFinder_RB;
  ADAQComboBoxWithLabel *SpectrumCalibrationType_CBL;
  ADAQNumberEntryWithLabel *SpectrumCalibrationMin_NEL, *SpectrumCalibrationMax_NEL;

  ADAQComboBoxWithLabel *SpectrumCalibrationPoint_CBL;
  ADAQComboBoxWithLabel *SpectrumCalibrationUnit_CBL;
  ADAQNumberEntryWithLabel *SpectrumCalibrationEnergy_NEL;
  ADAQNumberEntryWithLabel *SpectrumCalibrationPulseUnit_NEL;

  TGTextButton *SpectrumCalibrationSetPoint_TB;
  TGTextButton *SpectrumCalibrationCalibrate_TB;
  TGTextButton *SpectrumCalibrationPlot_TB;
  TGTextButton *SpectrumCalibrationReset_TB;
  TGTextButton *SpectrumCalibrationLoad_TB;

  TGTextButton *ProcessSpectrum_TB, *CreateSpectrum_TB;


  ///////////////////////////////////////////
  // Widgets for the analysis tabbed frame //
  ///////////////////////////////////////////

  TGCheckButton *SpectrumFindPeaks_CB;
  ADAQNumberEntryWithLabel *SpectrumNumPeaks_NEL;
  ADAQNumberEntryWithLabel *SpectrumSigma_NEL;
  ADAQNumberEntryWithLabel *SpectrumResolution_NEL;

  TGCheckButton *SpectrumFindBackground_CB;
  ADAQNumberEntryWithLabel *SpectrumRangeMin_NEL;
  ADAQNumberEntryWithLabel *SpectrumRangeMax_NEL;
  ADAQNumberEntryWithLabel *SpectrumBackgroundIterations_NEL;
  ADAQComboBoxWithLabel *SpectrumBackgroundDirection_CBL;
  TGCheckButton *SpectrumBackgroundCompton_CB, *SpectrumBackgroundSmoothing_CB;
  ADAQComboBoxWithLabel *SpectrumBackgroundFilterOrder_CBL, *SpectrumBackgroundSmoothingWidth_CBL;

  TGRadioButton *SpectrumWithBackground_RB;
  TGRadioButton *SpectrumLessBackground_RB;

  ADAQNumberEntryFieldWithLabel *SpectrumFitHeight_NEFL, *SpectrumFitHeightErr_NEFL;
  ADAQNumberEntryFieldWithLabel *SpectrumFitMean_NEFL, *SpectrumFitMeanErr_NEFL;
  ADAQNumberEntryFieldWithLabel *SpectrumFitSigma_NEFL, *SpectrumFitSigmaErr_NEFL;
  ADAQNumberEntryFieldWithLabel *SpectrumFitRes_NEFL, *SpectrumFitResErr_NEFL;

  ADAQNumberEntryWithLabel *SpectrumAnalysisLowerLimit_NEL;
  ADAQNumberEntryWithLabel *SpectrumAnalysisUpperLimit_NEL;

  TGCheckButton *SpectrumFindIntegral_CB;
  TGCheckButton *SpectrumIntegralInCounts_CB;
  TGCheckButton *SpectrumUseGaussianFit_CB;
  TGCheckButton *SpectrumUseVerboseFit_CB;
  TGCheckButton *SpectrumNormalizePeakToCurrent_CB;
  ADAQNumberEntryFieldWithLabel *SpectrumIntegral_NEFL;
  ADAQNumberEntryFieldWithLabel *SpectrumIntegralError_NEFL;
  
  TGCheckButton *SpectrumOverplotDerivative_CB;
  
  TGCheckButton *EAEnable_CB;
  ADAQComboBoxWithLabel *EASpectrumType_CBL;

  ADAQNumberEntryWithLabel *EAGammaEDep_NEL;
  TGCheckButton *EAEscapePeaks_CB;

  TGRadioButton *EAEJ301_RB, *EAEJ309_RB;
  ADAQNumberEntryWithLabel *EALightConversionFactor_NEL;
  ADAQNumberEntryWithLabel *EAErrorWidth_NEL;
  ADAQNumberEntryWithLabel *EAElectronEnergy_NEL, *EAGammaEnergy_NEL;
  ADAQNumberEntryWithLabel *EAProtonEnergy_NEL, *EAAlphaEnergy_NEL, *EACarbonEnergy_NEL;


  //////////////////////////////////////
  // Widgets for the PSD tabbed frame //
  //////////////////////////////////////

  // PSD histogram creation
  ADAQNumberEntryWithLabel *PSDWaveforms_NEL, *PSDThreshold_NEL;
  ADAQNumberEntryWithLabel *PSDNumTotalBins_NEL, *PSDMinTotalBin_NEL, *PSDMaxTotalBin_NEL;
  TGRadioButton *PSDXAxisADC_RB, *PSDXAxisEnergy_RB;
  ADAQNumberEntryWithLabel *PSDNumTailBins_NEL, *PSDMinTailBin_NEL, *PSDMaxTailBin_NEL;
  TGRadioButton *PSDYAxisTail_RB, *PSDYAxisTailTotal_RB;
  ADAQNumberEntryWithLabel *PSDTotalStart_NEL, *PSDTotalStop_NEL;
  ADAQNumberEntryWithLabel *PSDTailStart_NEL, *PSDTailStop_NEL;
  TGCheckButton *PSDPlotIntegrationLimits_CB;
  TGRadioButton *PSDAlgorithmPF_RB, *PSDAlgorithmSMS_RB, *PSDAlgorithmWD_RB;
  ADAQComboBoxWithLabel *PSDPlotType_CBL, *PSDPlotPalette_CBL;
  TGTextButton *ProcessPSDHistogram_TB, *CreatePSDHistogram_TB;

  // PSD region creation
  TGCheckButton *PSDEnableRegionCreation_CB;
  TGCheckButton *PSDEnableRegion_CB;
  TGRadioButton *PSDInsideRegion_RB, *PSDOutsideRegion_RB;
  TGTextButton *PSDCreateRegion_TB, *PSDClearRegion_TB;

  // PSD histogram slicing and 1D analysis
  TGCheckButton *PSDEnableHistogramSlicing_CB;
  TGRadioButton *PSDHistogramSliceX_RB, *PSDHistogramSliceY_RB;
  
  TGCheckButton *PSDCalculateFOM_CB;
  ADAQNumberEntryWithLabel *PSDLowerFOMFitMin_NEL, *PSDLowerFOMFitMax_NEL;
  ADAQNumberEntryWithLabel *PSDUpperFOMFitMin_NEL, *PSDUpperFOMFitMax_NEL;
  ADAQNumberEntryFieldWithLabel *PSDFigureOfMerit_NEFL;
  
  
  ///////////////////////////////////////////
  // Widgets for the graphics tabbed frame //
  ///////////////////////////////////////////
  
  TGRadioButton *DrawWaveformWithLine_RB, *DrawWaveformWithCurve_RB;
  TGRadioButton *DrawWaveformWithMarkers_RB, *DrawWaveformWithBoth_RB;
  TGTextButton *WaveformColor_TB;
  ADAQNumberEntryWithLabel *WaveformLineWidth_NEL;
  ADAQNumberEntryWithLabel *WaveformMarkerSize_NEL;
  
  TGRadioButton *DrawSpectrumWithCurve_RB, *DrawSpectrumWithLine_RB;
  TGRadioButton *DrawSpectrumWithError_RB, *DrawSpectrumWithBars_RB;
  TGTextButton *SpectrumLineColor_TB, *SpectrumFillColor_TB;
  ADAQNumberEntryWithLabel *SpectrumLineWidth_NEL;
  ADAQNumberEntryWithLabel *SpectrumFillStyle_NEL;
  
  TGCheckButton *HistogramStats_CB, *CanvasGrid_CB;
  TGCheckButton *CanvasXAxisLog_CB, *CanvasYAxisLog_CB, *CanvasZAxisLog_CB;
  TGCheckButton *PlotSpectrumDerivativeError_CB;
  TGCheckButton *PlotAbsValueSpectrumDerivative_CB;
  
  TGTextButton *ReplotWaveform_TB;
  TGTextButton *ReplotSpectrum_TB;
  TGTextButton *ReplotSpectrumDerivative_TB;
  TGTextButton *ReplotPSDHistogram_TB;

  TGCheckButton *OverrideTitles_CB;
  ADAQTextEntryWithLabel *Title_TEL;
  ADAQTextEntryWithLabel *XAxisTitle_TEL;
  ADAQNumberEntryWithLabel *XAxisSize_NEL, *XAxisOffset_NEL, *XAxisDivs_NEL;

  ADAQTextEntryWithLabel *YAxisTitle_TEL;
  ADAQNumberEntryWithLabel *YAxisSize_NEL, *YAxisOffset_NEL, *YAxisDivs_NEL;

  ADAQTextEntryWithLabel *ZAxisTitle_TEL;
  ADAQNumberEntryWithLabel *ZAxisSize_NEL, *ZAxisOffset_NEL, *ZAxisDivs_NEL;

  ADAQTextEntryWithLabel *PaletteAxisTitle_TEL;
  ADAQNumberEntryWithLabel *PaletteAxisSize_NEL, *PaletteAxisOffset_NEL;
  ADAQNumberEntryWithLabel *PaletteX1_NEL, *PaletteX2_NEL, *PaletteY1_NEL, *PaletteY2_NEL;


  /////////////////////////////////////////////
  // Widgets for the processing tabbed frame //
  /////////////////////////////////////////////

  TGButtonGroup *ProcessingType_BG;
  TGRadioButton *ProcessingSeq_RB, *ProcessingPar_RB;
  ADAQNumberEntryWithLabel *NumProcessors_NEL;
  ADAQNumberEntryWithLabel *UpdateFreq_NEL;

  TGTextButton *DesplicedFileSelection_TB;
  TGTextEntry *DesplicedFileName_TE;
  ADAQNumberEntryWithLabel *DesplicedWaveformBuffer_NEL;
  ADAQNumberEntryWithLabel *DesplicedWaveformNumber_NEL;
  ADAQNumberEntryWithLabel *DesplicedWaveformLength_NEL;
  TGTextButton *DesplicedFileCreation_TB;


  ///////////////////////////////////////////
  // Widget objects for the "Canvas" frame //
  ///////////////////////////////////////////

  // Widget to display open ADAQ ROOT file statistic
  TGTextEntry *FileName_TE, *FirmwareType_TE;
  TGNumberEntryField *Waveforms_NEL, *RecordLength_NEL;
  TGLabel *FirmwareLabel_L, *WaveformLabel_L, *RecordLengthLabel_L;

  // The canvas to display all plots
  TRootEmbeddedCanvas *Canvas_EC;

  // Triple and double slider widgets for zooming in X and Y (the
  // pointer in the triple slider is used for spectrum calibration)
  TGTripleHSlider *XAxisLimits_THS;
  TGDoubleVSlider *YAxisLimits_DVS;

  // Slider widget for quickly scrolling through accessible waveforms
  TGHSlider *WaveformSelector_HS;

  // Slider to control spectrum integration and fit limits
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

  // Variables for size adjustment of widget layout
  int CanvasX, CanvasY, CanvasFrameWidth, SliderBuffer;
  int TotalX, TotalY;
  int TabFrameWidth, TabFrameLength;
  int NumEdgeBoundingPoints;
  double EdgeBoundX0, EdgeBoundY0;
  
  // Variables for interface colors
  long ThemeForegroundColor, ThemeBackgroundColor;

  // Variables relating to files (paths, bools)
  string DataDirectory, PrintDirectory, DesplicedDirectory, HistogramDirectory;
  bool ADAQFileLoaded, ASIMFileLoaded;
  string ADAQFileName, ASIMFileName;

  Bool_t EnableInterface;

  // The class which holds all ROOT widget settings
  AASettings *ADAQSettings;
  string ADAQSettingsFileName;

  // The managers 
  TColor *ColorMgr;
  TRandom3 *RndmMgr;
  AAComputation *ComputationMgr;
  AAGraphics *GraphicsMgr;
  AAInterpolation *InterpolationMgr;
  
  AAWaveformSlots *WaveformSlots;
  AASpectrumSlots *SpectrumSlots;
  AAAnalysisSlots *AnalysisSlots;
  AAPSDSlots *PSDSlots;
  AAGraphicsSlots *GraphicsSlots;
  AAProcessingSlots *ProcessingSlots;
  AANontabSlots *NontabSlots;
  
  // Define the class to ROOT
  ClassDef(AAInterface, 1);
};

#endif
