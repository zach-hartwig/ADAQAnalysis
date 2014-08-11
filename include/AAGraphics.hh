//////////////////////////////////////////////////////////////////////////////////////////
//
// name: AAGraphics.hh
// date: 01 Apr 14
// auth: Zach Hartwig
//
// desc: 
//
//////////////////////////////////////////////////////////////////////////////////////////

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
  void PlotPSDFilter();


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
  TBox *Baseline_B;
  TLine *LPeakDelimiter_L, *RPeakDelimiter_L;
  TBox *IntegrationRegion_B;
  TLine *PSDPeakOffset_L, *PSDTailOffset_L;
  TBox *PSDTailIntegral_B;
  TLine *PearsonLowerLimit_L, *PearsonMiddleLimit_L, *PearsonUpperLimit_L;

  // Objects for spectral analysis

  TLine *HCalibration_L, *VCalibration_L;
  TBox *EdgeBoundingBox_B;
  TLine *EALine_L;
  TBox *EABox_B;
  TLine *DerivativeReference_L;
  
  AAComputation *ComputationMgr;
  
  CanvasContentTypes CanvasContentType;

  TColorWheel *ColorWheel;
  int ColorToSet;
  int WaveformColor, WaveformLineWidth;
  double WaveformMarkerSize;
  int SpectrumLineColor, SpectrumLineWidth;
  int SpectrumFillColor, SpectrumFillStyle;

  enum{zWaveformColor, zSpectrumLineColor, zSpectrumFillColor};

  ClassDef(AAGraphics, 1)
};

#endif
