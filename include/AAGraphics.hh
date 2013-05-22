//////////////////////////////////////////////////////////////////////////////////////////
//
// name: AAGraphics.hh
// date: 17 May 13
// auth: Zach Hartwig
//
// desc: 
//
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __AAGraphics_hh__
#define __AAGraphics_hh__ 1


#include <TObject.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TBox.h>
#include <TLine.h>


#include "AAComputation.hh"
#include "ADAQAnalysisSettings.hh"


class AAGraphics : public TObject
{
public:
  AAGraphics();
  ~AAGraphics();

  static AAGraphics *GetInstance();

  void SetCanvasPointer(TCanvas *C) { TheCanvas = C; }

  void SetADAQSettings(ADAQAnalysisSettings *AAS) { ADAQSettings = AAS; }

  CanvasContentTypes GetCanvasContentType() {return CanvasContentType;}
  
  void PlotWaveform();
  void PlotPearsonIntegration();
  
  void PlotSpectrum();
  void PlotSpectrumDerivative();

  void PlotPSDHistogram();
  void PlotPSDHistogramSlice(int, int);
  void PlotPSDFilter();

  void PlotCalibration(int);
  void PlotCalibrationLine(double, double, double);
  
private:
  
  static AAGraphics *TheGraphicsManager;

  ADAQAnalysisSettings *ADAQSettings;

  TCanvas *TheCanvas;

  TLine *Trigger_L, *Floor_L, *Calibration_L, *ZSCeiling_L;
  TLine *LPeakDelimiter_L, *RPeakDelimiter_L;
  TBox *IntegrationRegion_B;
  TLine *PearsonLowerLimit_L, *PearsonMiddleLimit_L, *PearsonUpperLimit_L;
  TLine *PSDPeakOffset_L, *PSDTailOffset_L;
  TLine *DerivativeReference_L;
  TBox *Baseline_B, *PSDTailIntegral_B;
  
  AAComputation *ComputationMgr;
  
  CanvasContentTypes CanvasContentType;

  ClassDef(AAGraphics, 1)
};

#endif
