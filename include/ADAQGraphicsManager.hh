//////////////////////////////////////////////////////////////////////////////////////////
//
// name: ADAQAnalysisInterface.hh
// date: 17 May 13
// auth: Zach Hartwig
//
// desc: 
//
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __ADAQGraphicsManager_hh__
#define __ADAQGraphicsManager_hh__ 1

#include <TObject.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TBox.h>
#include <TLine.h>

#include "ADAQAnalysisManager.hh"
#include "ADAQAnalysisTypes.hh"

class ADAQGraphicsManager : public TObject
{
public:
  ADAQGraphicsManager();
  ~ADAQGraphicsManager();

  static ADAQGraphicsManager *GetInstance();

  void SetCanvasPointer(TCanvas *C) { TheCanvas = C; }

  void PlotWaveform(ADAQAnalysisSettings *);
  void PlotSpectrum(ADAQAnalysisSettings *){;}
  void PlotSpectrumDerivative(ADAQAnalysisSettings *){;}
  void PlotPSDHistogram(ADAQAnalysisSettings *){;}
  
private:
  
  static ADAQGraphicsManager *TheGraphicsManager;

  TCanvas *TheCanvas;

  TLine *Trigger_L, *Floor_L, *Calibration_L, *ZSCeiling_L;
  TLine *LPeakDelimiter_L, *RPeakDelimiter_L;
  TLine *PearsonLowerLimit_L, *PearsonMiddleLimit_L, *PearsonUpperLimit_L;
  TLine *PSDPeakOffset_L, *PSDTailOffset_L;
  TLine *LowerLimit_L, *UpperLimit_L;
  TLine *DerivativeReference_L;
  TBox *Baseline_B, *PSDTailIntegral_B;

  ADAQAnalysisManager *AnalysisMgr;

  ClassDef(ADAQGraphicsManager, 1)
};

#endif
