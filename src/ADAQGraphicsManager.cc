#include "ADAQGraphicsManager.hh"

#include <iostream>
using namespace std;


ADAQGraphicsManager *ADAQGraphicsManager::TheGraphicsManager = 0;


ADAQGraphicsManager *ADAQGraphicsManager::GetInstance()
{ return TheGraphicsManager; }


ADAQGraphicsManager::ADAQGraphicsManager()
  : Trigger_L(new TLine), Floor_L(new TLine), Baseline_B(new TBox), ZSCeiling_L(new TLine),
    LPeakDelimiter_L(new TLine), RPeakDelimiter_L(new TLine), Calibration_L(new TLine),
    PearsonLowerLimit_L(new TLine), PearsonMiddleLimit_L(new TLine), PearsonUpperLimit_L(new TLine),
    PSDPeakOffset_L(new TLine), PSDTailOffset_L(new TLine), PSDTailIntegral_B(new TBox),
    LowerLimit_L(new TLine), UpperLimit_L(new TLine), DerivativeReference_L(new TLine)
{
  if(TheGraphicsManager)
    cout << "\nERROR! TheGraphicsManager was constructed twice\n" << endl;
  TheGraphicsManager = this;

  Trigger_L->SetLineStyle(7);
  Trigger_L->SetLineColor(2);
  Trigger_L->SetLineWidth(2);

  Floor_L->SetLineStyle(7);
  Floor_L->SetLineColor(2);
  Floor_L->SetLineWidth(2);

  Baseline_B->SetFillStyle(3002);
  Baseline_B->SetFillColor(8);

  ZSCeiling_L->SetLineStyle(7);
  ZSCeiling_L->SetLineColor(7);
  ZSCeiling_L->SetLineWidth(2);

  LPeakDelimiter_L->SetLineStyle(7);
  LPeakDelimiter_L->SetLineWidth(2);
  LPeakDelimiter_L->SetLineColor(6);
  
  RPeakDelimiter_L->SetLineStyle(7);
  RPeakDelimiter_L->SetLineWidth(2);
  RPeakDelimiter_L->SetLineColor(7);
  
  Calibration_L->SetLineStyle(7);
  Calibration_L->SetLineColor(2);
  Calibration_L->SetLineWidth(2);

  PearsonLowerLimit_L->SetLineStyle(7);
  PearsonLowerLimit_L->SetLineColor(4);
  PearsonLowerLimit_L->SetLineWidth(2);

  PearsonMiddleLimit_L->SetLineStyle(7);
  PearsonMiddleLimit_L->SetLineColor(8);
  PearsonMiddleLimit_L->SetLineWidth(2);
  
  PearsonUpperLimit_L->SetLineStyle(7);
  PearsonUpperLimit_L->SetLineColor(4);
  PearsonUpperLimit_L->SetLineWidth(2);

  PSDPeakOffset_L->SetLineStyle(7);
  PSDPeakOffset_L->SetLineColor(8);
  PSDPeakOffset_L->SetLineWidth(2);

  PSDTailOffset_L->SetLineStyle(7);
  PSDTailOffset_L->SetLineColor(8);
  PSDTailOffset_L->SetLineWidth(2);

  PSDTailIntegral_B->SetFillStyle(3002);
  PSDTailIntegral_B->SetFillColor(38);

  LowerLimit_L->SetLineStyle(7);
  LowerLimit_L->SetLineColor(2);
  LowerLimit_L->SetLineWidth(2);

  UpperLimit_L->SetLineStyle(7);
  UpperLimit_L->SetLineColor(2);
  UpperLimit_L->SetLineWidth(2);

  DerivativeReference_L->SetLineStyle(7);
  DerivativeReference_L->SetLineColor(2);
  DerivativeReference_L->SetLineWidth(2);

  AnalysisMgr = ADAQAnalysisManager::GetInstance();
}


ADAQGraphicsManager::~ADAQGraphicsManager()
{
  delete TheGraphicsManager;
}


void ADAQGraphicsManager::PlotWaveform(ADAQAnalysisSettings *ADAQSettings)
{
  TH1F *Waveform_H = 0;
  
  int Channel = ADAQSettings->Channel;
  int Waveform = ADAQSettings->WaveformToPlot;
  
  if(ADAQSettings->RawWaveform)
    Waveform_H = AnalysisMgr->CalculateRawWaveform(Channel, Waveform);

  else if(ADAQSettings->BSWaveform)
    Waveform_H = AnalysisMgr->CalculateBSWaveform(Channel, Waveform);

  else if(ADAQSettings->ZSWaveform)
    Waveform_H = AnalysisMgr->CalculateZSWaveform(Channel, Waveform);


  float Min, Max;
  int XAxisSize, YAxisSize;

  // Determine the X-axis size and min/max values
  XAxisSize = Waveform_H->GetNbinsX();
  
  /*
  XAxisLimits_THS->GetPosition(Min, Max);
  XAxisMin = XAxisSize * Min;
  XAxisMax = XAxisSize * Max;

  // Determine the Y-axis size (dependent on whether or not the user
  // has opted to allow the Y-axis range to be automatically
  // determined by the size of the waveform or fix the Y-Axis range
  // with the vertical double slider positions) and min/max values

  YAxisLimits_DVS->GetPosition(Min, Max);

  if(PlotVerticalAxisInLog_CB->IsDown()){
    YAxisMin = V1720MaximumBit * (1 - Max);
    YAxisMax = V1720MaximumBit * (1 - Min);
    
    if(YAxisMin == 0)
      YAxisMin = 0.05;
  }
  else if(AutoYAxisRange_CB->IsDown()){
    YAxisMin = Waveform_H[Channel]->GetBinContent(Waveform_H[Channel]->GetMinimumBin()) - 10;
    YAxisMax = Waveform_H[Channel]->GetBinContent(Waveform_H[Channel]->GetMaximumBin()) + 10;
  }
  else{
    YAxisLimits_DVS->GetPosition(Min, Max);
    YAxisMin = (V1720MaximumBit * (1 - Max))-30;
    YAxisMax = V1720MaximumBit * (1 - Min);
  }
  YAxisSize = YAxisMax - YAxisMin;

  if(OverrideTitles_CB->IsDown()){
    Title = Title_TEL->GetEntry()->GetText();
    XAxisTitle = XAxisTitle_TEL->GetEntry()->GetText();
    YAxisTitle = YAxisTitle_TEL->GetEntry()->GetText();
  }
  else{
    Title = "Digitized Waveform";
    XAxisTitle = "Time [4 ns samples]";
    YAxisTitle = "Voltage [ADC]";
  }

  int XAxisDivs = XAxisDivs_NEL->GetEntry()->GetIntNumber();
  int YAxisDivs = YAxisDivs_NEL->GetEntry()->GetIntNumber();
  */

  TheCanvas->SetLeftMargin(0.13);
  TheCanvas->SetBottomMargin(0.12);
  TheCanvas->SetRightMargin(0.05);
  
  if(ADAQSettings->PlotVerticalAxisInLog)
    gPad->SetLogy(true);
  else
    gPad->SetLogy(false);
  /*
  Waveform_H->SetTitle(Title.c_str());

  Waveform_H->GetXaxis()->SetTitle(XAxisTitle.c_str());
  Waveform_H->GetXaxis()->SetTitleSize(XAxisSize_NEL->GetEntry()->GetNumber());
  Waveform_H->GetXaxis()->SetLabelSize(XAxisSize_NEL->GetEntry()->GetNumber());
  Waveform_H->GetXaxis()->SetTitleOffset(XAxisOffset_NEL->GetEntry()->GetNumber());
  Waveform_H->GetXaxis()->CenterTitle();
  Waveform_H->GetXaxis()->SetRangeUser(XAxisMin, XAxisMax);
  Waveform_H->GetXaxis()->SetNdivisions(XAxisDivs, true);

  Waveform_H->GetYaxis()->SetTitle(YAxisTitle.c_str());
  Waveform_H->GetYaxis()->SetTitleSize(YAxisSize_NEL->GetEntry()->GetNumber());
  Waveform_H->GetYaxis()->SetLabelSize(YAxisSize_NEL->GetEntry()->GetNumber());
  Waveform_H->GetYaxis()->SetTitleOffset(YAxisOffset_NEL->GetEntry()->GetNumber());
  Waveform_H->GetYaxis()->CenterTitle();
  Waveform_H->GetYaxis()->SetNdivisions(YAxisDivs, true);
  Waveform_H->SetMinimum(YAxisMin);
  Waveform_H->SetMaximum(YAxisMax);
  */
  Waveform_H->SetLineColor(kBlue);
  Waveform_H->SetStats(false);  
  
  Waveform_H->SetMarkerStyle(24);
  Waveform_H->SetMarkerSize(1.0);
  Waveform_H->SetMarkerColor(4);
  
  string DrawString;
  if(ADAQSettings->DrawWaveformWithCurve)
    DrawString = "C";
  else if(ADAQSettings->DrawWaveformWithMarkers)
    DrawString = "P";
  else if(ADAQSettings->DrawWaveformWithBoth)
    DrawString = "CP";
  
  Waveform_H->Draw(DrawString.c_str());
    

  // The user may choose to plot a number of graphical objects
  // representing various waveform parameters (zero suppression
  // ceiling, noise ceiling, trigger, baseline calculation, ... ) to
  // aid him/her optimizing the parameter values

  if(ADAQSettings->PlotZeroSuppressionCeiling)
    {}/*
    ZSCeiling_L->DrawLine(XAxisMin,
			  ZeroSuppressionCeiling_NEL->GetEntry()->GetIntNumber(),
			  XAxisMax,
			  ZeroSuppressionCeiling_NEL->GetEntry()->GetIntNumber());
      */


  //  if(PlotNoiseCeiling_CB->IsDown())
  //    NCeiling_L->DrawLine(XAxisMin,
  //			 NoiseCeiling_NEL->GetEntry()->GetIntNumber(),
  //			 XAxisMax,
  //			 NoiseCeiling_NEL->GetEntry()->GetIntNumber());


  if(ADAQSettings->PlotTrigger)
  {}/*Trigger_L->DrawLine(XAxisMin,
		      ADAQMeasParams->TriggerThreshold[Channel], 
		      XAxisMax,
		      ADAQMeasParams->TriggerThreshold[Channel]);
    */

  // The user may choose to overplot a transparent box on the
  // waveform. The lower and upper X limits of the box represent the
  // min/max baseline calculation bounds; the Y position of the box
  // center represents the calculated value of the baseline. Note that
  // plotting the baseline is disabled for ZS waveforms since plotting
  // on a heavily modified X-axis range does not visually represent
  // the actual region of the waveform used for baseline calculation.
  if(ADAQSettings->PlotBaseline and !ADAQSettings->ZSWaveform){
    
    /*
    double BaselinePlotMinValue = Baseline - (0.04 * YAxisSize);
    double BaselinePlotMaxValue = Baseline + (0.04 * YAxisSize);
    if(ADAQSettings->BaselineSubtractedWaveform){
      BaselinePlotMinValue = -0.04 * YAxisSize;
      BaselinePlotMaxValue = 0.04 * YAxisSize;
    }
    */
    /*
    Baseline_B->DrawBox(BaselineCalcMin_NEL->GetEntry()->GetIntNumber(),
			BaselinePlotMinValue,
			BaselineCalcMax_NEL->GetEntry()->GetIntNumber(),
			BaselinePlotMaxValue);
    */
  }
  /* 
  if(ADAQSettings->PlotPearsonIntegration){
    PearsonLowerLimit_L->DrawLine(PearsonLowerLimit_NEL->GetEntry()->GetIntNumber(),
				  YAxisMin,
				  PearsonLowerLimit_NEL->GetEntry()->GetIntNumber(),
				  YAxisMax);

    if(ADAQSettings->IntegrateFitToPearson)
      PearsonMiddleLimit_L->DrawLine(PearsonMiddleLimit_NEL->GetEntry()->GetIntNumber(),
				     YAxisMin,
				     PearsonMiddleLimit_NEL->GetEntry()->GetIntNumber(),
				     YAxisMax);
    
    PearsonUpperLimit_L->DrawLine(PearsonUpperLimit_NEL->GetEntry()->GetIntNumber(),
				  YAxisMin,
				  PearsonUpperLimit_NEL->GetEntry()->GetIntNumber(),
				  YAxisMax);
  }
  */
  TheCanvas->Update();
  
  //if(FindPeaks_CB->IsDown())
  //    {}//FindPeaks(Waveform_H);
  
  // Set the class member int that tells the code what is currently
  // plotted on the embedded canvas. This is important for influencing
  // the behavior of the vertical and horizontal double sliders used
  // to "zoom" on the canvas contents
  //CanvasContents = zWaveform;
  
  //  if(ADAQSettings->PlotPearsonIntegration)
  //    IntegratePearsonWaveform(true);


}
