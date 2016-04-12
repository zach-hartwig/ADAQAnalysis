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
// name: AAGraphics.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAGraphics class is responsible for the plotting of all
//       graphical objects, including waveforms, spectra, PSD
//       histograms, and associated fits, lines, fill regions, etc.
//       It receives a pointer to the main embedded ROOT canvas from
//       the AAInterface class so that it may populate it with all
//       required graphical objects. The AASettings object, which
//       contains all values of the widgets from the GUI, is made
//       available to the class so that the necessary values may be
//       used in plotting the various objects.
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef __AAGraphics_hh__
#define __AAGraphics_hh__ 1


// ROOT
#include <TObject.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TBox.h>
#include <TLine.h>
#include <TColorWheel.h>


// AA
#include "AAComputation.hh"
#include "AASettings.hh"


class AAGraphics : public TObject
{
public:
  AAGraphics();
  ~AAGraphics();

  // Access omethod to the Meyer's singleton pointer
  static AAGraphics *GetInstance();

  //////////////////
  // Set/Get methods

  void SetCanvasPointer(TCanvas *C) { TheCanvas = C; }

  void SetADAQSettings(AASettings *AAS) { ADAQSettings = AAS; }

  CanvasContentTypes GetCanvasContentType() {return CanvasContentType;}

  void SetWaveformColor();
  int GetWaveformColor() {return WaveformColor;}

  void SetSpectrumLineColor();
  int GetSpectrumLineColor() {return SpectrumLineColor;}

  void SetSpectrumFillColor();
  int GetSpectrumFillColor() {return SpectrumFillColor;}

  void CreateColorWheel();
  void PickColor(int, int, int, TObject *);

  
  ////////////////////////////
  // Waveform plotting methods
  
  void PlotWaveform(int Color=kBlue);
  void PlotPearsonIntegration();


  ///////////////////////////
  // Spectra plotting methods
  
  void PlotSpectrum();
  void PlotSpectrumDerivative();

  
  //////////////////////////////////////////////
  // Pulse shape discrimination plotting methods

  void PlotPSDHistogram();
  void PlotPSDHistogramSlice(int, int);
  void PlotPSDRegionProgress();
  void PlotPSDRegion();
 
  Double_t GetPSDFigureOfMerit() {return PSDFigureOfMerit;}


  ///////////////////////////////////////////
  // Analysis graphic object plotting methods
  
  void PlotCalibration(int);
  void PlotHCalibrationLine(double, bool Refresh = true);
  void PlotVCalibrationLine(double, bool Refresh = true);
  void PlotCalibrationCross(double, double);
  void PlotEdgeBoundingBox(double, double, double, double);
  
  void PlotEALine(double, double, bool ErrorBox = false, bool EscapePeaks = false);
  
private:
  
  static AAGraphics *TheGraphicsManager;
  
  AASettings *ADAQSettings;

  TCanvas *TheCanvas;

  TH1F *Spectrum_H;

  // Objects for waveform analysis

  TLine *Trigger_L, *Floor_L, *ZSCeiling_L;
  TBox *Analysis_B, *Baseline_B;
  TLine *LPeakDelimiter_L, *RPeakDelimiter_L;
  TBox *IntegrationRegion_B;
  TBox *PSDTotal_B;
  TLine *PSDPeak_L, *PSDTail_L0, *PSDTail_L1;
  TLine *PearsonLowerLimit_L, *PearsonMiddleLimit_L, *PearsonUpperLimit_L;

  // Objects for spectral analysis

  TLine *HCalibration_L, *VCalibration_L;
  TBox *EdgeBoundingBox_B;
  TLine *EALine_L;
  TBox *EABox_B;
  TLine *DerivativeReference_L;

  // Objects for PSD analysis
  TGraph *PSDRegionProgress_G;
  
  AAComputation *ComputationMgr;
  
  CanvasContentTypes CanvasContentType;

  TColorWheel *ColorWheel;
  int ColorToSet;
  int WaveformColor, WaveformLineWidth;
  double WaveformMarkerSize;
  int SpectrumLineColor, SpectrumLineWidth;
  int SpectrumFillColor, SpectrumFillStyle;

  enum{zWaveformColor, zSpectrumLineColor, zSpectrumFillColor};

  Double_t PSDFigureOfMerit;

  ClassDef(AAGraphics, 1)
};

#endif
