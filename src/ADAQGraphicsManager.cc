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


void ADAQGraphicsManager::PlotWaveform()
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


  // Determine the X-axis size and min/max values
  int XAxisSize = Waveform_H->GetNbinsX();

  double XMin = ADAQSettings->XAxisMin * XAxisSize;
  double XMax = ADAQSettings->XAxisMax * XAxisSize;

  // Determine the Y-axis size (dependent on whether or not the user
  // has opted to allow the Y-axis range to be automatically
  // determined by the size of the waveform or fix the Y-Axis range
  // with the vertical double slider positions) and min/max values

  double YMin, YMax, YAxisSize;
  
  if(ADAQSettings->PlotVerticalAxisInLog){
    YMin = 4095 * (1 - ADAQSettings->YAxisMax);
    YMax = 4095 * (1 - ADAQSettings->YAxisMin);
    
    if(YMin == 0)
      YMin = 0.05;
  }
  else if(ADAQSettings->YAxisAutoRange){
    YMin = Waveform_H->GetBinContent(Waveform_H->GetMinimumBin()) - 10;
    YMax = Waveform_H->GetBinContent(Waveform_H->GetMaximumBin()) + 10;
  }
  else{
    YMin = (4095 * (1 - ADAQSettings->YAxisMax))-30;
    YMax = 4095 * (1 - ADAQSettings->YAxisMin);
  }

  YAxisSize = YMax - YMin;


  string Title, XTitle, YTitle;
  
  if(ADAQSettings->OverrideGraphicalDefault){
    Title = ADAQSettings->PlotTitle;
    XTitle = ADAQSettings->XAxisTitle;
    YTitle = ADAQSettings->YAxisTitle;
  }
  else{
    Title = "Digitized Waveform";
    XTitle = "Time [4 ns samples]";
    YTitle = "Voltage [ADC]";
  }
  
  int XDivs = ADAQSettings->XDivs;
  int YDivs = ADAQSettings->YDivs;

  TheCanvas->SetLeftMargin(0.13);
  TheCanvas->SetBottomMargin(0.12);
  TheCanvas->SetRightMargin(0.05);
  
  if(ADAQSettings->PlotVerticalAxisInLog)
    gPad->SetLogy(true);
  else
    gPad->SetLogy(false);

  Waveform_H->SetTitle(Title.c_str());

  Waveform_H->GetXaxis()->SetTitle(XTitle.c_str());
  Waveform_H->GetXaxis()->SetTitleSize(ADAQSettings->XSize);
  Waveform_H->GetXaxis()->SetLabelSize(ADAQSettings->XSize);
  Waveform_H->GetXaxis()->SetTitleOffset(ADAQSettings->XOffset);
  Waveform_H->GetXaxis()->CenterTitle();
  Waveform_H->GetXaxis()->SetRangeUser(XMin, XMax);
  Waveform_H->GetXaxis()->SetNdivisions(XDivs, true);

  Waveform_H->GetYaxis()->SetTitle(YTitle.c_str());
  Waveform_H->GetYaxis()->SetTitleSize(ADAQSettings->YSize);
  Waveform_H->GetYaxis()->SetLabelSize(ADAQSettings->YSize);
  Waveform_H->GetYaxis()->SetTitleOffset(ADAQSettings->YOffset);
  Waveform_H->GetYaxis()->CenterTitle();
  Waveform_H->GetYaxis()->SetNdivisions(YDivs, true);

  Waveform_H->SetMinimum(YMin);
  Waveform_H->SetMaximum(YMax);

  Waveform_H->SetLineColor(kBlue);
  Waveform_H->SetStats(false);  
  
  Waveform_H->SetMarkerStyle(24);
  Waveform_H->SetMarkerSize(1.0);
  Waveform_H->SetMarkerColor(4);
  
  string DrawString;
  if(ADAQSettings->WaveformCurve)
    DrawString = "C";
  else if(ADAQSettings->WaveformMarkers)
    DrawString = "P";
  else if(ADAQSettings->WaveformBoth)
    DrawString = "CP";
  
  Waveform_H->Draw(DrawString.c_str());
    
  if(ADAQSettings->PlotZeroSuppressionCeiling)
    ZSCeiling_L->DrawLine(XMin,
			  ADAQSettings->ZeroSuppressionCeiling,
			  XMax,
			  ADAQSettings->ZeroSuppressionCeiling);

  if(ADAQSettings->PlotTrigger)
    Trigger_L->DrawLine(XMin,
			AnalysisMgr->GetADAQMeasurementParameters()->TriggerThreshold[Channel], 
			XMax,
			AnalysisMgr->GetADAQMeasurementParameters()->TriggerThreshold[Channel]);

  if(ADAQSettings->PlotBaseline and !ADAQSettings->ZSWaveform){
    double Baseline = AnalysisMgr->CalculateBaseline(Waveform_H);
    double BaselinePlotMinValue = Baseline - (0.04 * YAxisSize);
    double BaselinePlotMaxValue = Baseline + (0.04 * YAxisSize);
    
    if(ADAQSettings->BSWaveform){
      BaselinePlotMinValue = -0.04 * YAxisSize;
      BaselinePlotMaxValue = 0.04 * YAxisSize;
    }
    Baseline_B->DrawBox(ADAQSettings->BaselineCalcMin,
			BaselinePlotMinValue,
			ADAQSettings->BaselineCalcMax,
			BaselinePlotMaxValue);
  }
  
  
  if(ADAQSettings->PlotPearsonIntegration){
    PearsonLowerLimit_L->DrawLine(ADAQSettings->PearsonLowerLimit,
				  YMin,
				  ADAQSettings->PearsonLowerLimit,
				  YMax);
    
    if(ADAQSettings->IntegrateFitToPearson)
      PearsonMiddleLimit_L->DrawLine(ADAQSettings->PearsonMiddleLimit,
				     YMin,
				     ADAQSettings->PearsonMiddleLimit,
				     YMax);
    
    PearsonUpperLimit_L->DrawLine(ADAQSettings->PearsonUpperLimit,
				  YMin,
				  ADAQSettings->PearsonUpperLimit,
				  YMax);
  }

  TheCanvas->Update();
  
  //if(FindPeaks_CB->IsDown())
  //    {}//FindPeaks(Waveform_H);
  
  // Set the class member int that tells the code what is currently
  // plotted on the embedded canvas. This is important for influencing
  // the behavior of the vertical and horizontal double sliders used
  // to "zoom" on the canvas contents
  CanvasContentType = zWaveform;
  
  //  if(ADAQSettings->PlotPearsonIntegration)
  //    IntegratePearsonWaveform(true);
  
}


void ADAQGraphicsManager::PlotSpectrum()
{
  TH1F *Spectrum_H = AnalysisMgr->GetSpectrum();

  if(!Spectrum_H){
    //CreateMessageBox("A valid Spectrum_H object was not found! Spectrum plotting will abort!","Stop");
    return;
  }

  //////////////////////////////////
  // Determine main spectrum to plot

  //  TH1F *Spectrum2Plot_H = 0;
  
  //  if(SpectrumFindBackground_CB->IsDown() and SpectrumLessBackground_RB->IsDown())
  //    Spectrum2Plot_H = (TH1F *)SpectrumDeconvolved_H->Clone();
  //  else
  //    Spectrum2Plot_H = (TH1F *)Spectrum_H->Clone();
  

  double XMin = Spectrum_H->GetXaxis()->GetXmax() * ADAQSettings->XAxisMin;
  double XMax = Spectrum_H->GetXaxis()->GetXmax() * ADAQSettings->XAxisMax;

  Spectrum_H->GetXaxis()->SetRangeUser(XMin, XMax);

  double YMin, YMax;
  if(ADAQSettings->PlotVerticalAxisInLog and ADAQSettings->YAxisMax==1)
    YMin = 1.0;
  else if(ADAQSettings->FindBackground and ADAQSettings->PlotLessBackground)
    YMin = (Spectrum_H->GetMaximumBin() * (1-ADAQSettings->YAxisMax)) - (Spectrum_H->GetMaximumBin()*0.8);
  else
    YMin = Spectrum_H->GetBinContent(Spectrum_H->GetMaximumBin()) * (1-ADAQSettings->YAxisMax);
  
  YMax = Spectrum_H->GetBinContent(Spectrum_H->GetMaximumBin()) * (1-ADAQSettings->YAxisMin) * 1.05;
  
  Spectrum_H->SetMinimum(YMin);
  Spectrum_H->SetMaximum(YMax);

  
  /////////////////
  // Spectrum title

  string Title, XTitle, YTitle;

  if(ADAQSettings->OverrideGraphicalDefault){
    Title = ADAQSettings->PlotTitle;
    XTitle = ADAQSettings->XAxisTitle;
    YTitle = ADAQSettings->YAxisTitle;
  }
  else{
    Title = "Pulse Spectrum";
    XTitle = "Pulse units [ADC]";
    YTitle = "Counts";
  }

  // Get the desired axes divisions
  int XDivs = ADAQSettings->XDivs;
  int YDivs = ADAQSettings->YDivs;

  
  ////////////////////
  // Statistics legend

  if(ADAQSettings->StatsOff)
    Spectrum_H->SetStats(false);
  else
    Spectrum_H->SetStats(true);


  /////////////////////
  // Logarithmic Y axis
  
  if(ADAQSettings->PlotVerticalAxisInLog)
    gPad->SetLogy(true);
  else
    gPad->SetLogy(false);
  
  TheCanvas->SetLeftMargin(0.13);
  TheCanvas->SetBottomMargin(0.12);
  TheCanvas->SetRightMargin(0.05);

  ////////////////////////////////
  // Axis and graphical attributes

  Spectrum_H->SetTitle(Title.c_str());

  Spectrum_H->GetXaxis()->SetTitle(XTitle.c_str());
  Spectrum_H->GetXaxis()->SetTitleSize(ADAQSettings->XSize);
  Spectrum_H->GetXaxis()->SetLabelSize(ADAQSettings->XSize);
  Spectrum_H->GetXaxis()->SetTitleOffset(ADAQSettings->XOffset);
  Spectrum_H->GetXaxis()->CenterTitle();
  Spectrum_H->GetXaxis()->SetNdivisions(XDivs, true);

  Spectrum_H->GetYaxis()->SetTitle(YTitle.c_str());
  Spectrum_H->GetYaxis()->SetTitleSize(ADAQSettings->YSize);
  Spectrum_H->GetYaxis()->SetLabelSize(ADAQSettings->YSize);
  Spectrum_H->GetYaxis()->SetTitleOffset(ADAQSettings->YOffset);
  Spectrum_H->GetYaxis()->CenterTitle();
  Spectrum_H->GetYaxis()->SetNdivisions(YDivs, true);

  Spectrum_H->SetLineColor(4);
  Spectrum_H->SetLineWidth(2);

  /////////////////////////
  // Plot the main spectrum
  string DrawString;
  if(ADAQSettings->SpectrumBars){
    Spectrum_H->SetFillStyle(4100);
    Spectrum_H->SetFillColor(4);
    DrawString = "B";
  }
  else if(ADAQSettings->SpectrumCurve){
    Spectrum_H->SetFillStyle(4000);
    DrawString = "C";
  }
  else if(ADAQSettings->SpectrumMarkers){
    Spectrum_H->SetMarkerStyle(24);
    Spectrum_H->SetMarkerColor(4);
    Spectrum_H->SetMarkerSize(1.0);
    DrawString = "P";
  }
  
  Spectrum_H->Draw(DrawString.c_str());

  ////////////////////////////////////////////
  // Overlay the background spectra if desired
  //  if(ADAQSettings->FindBackground and ADAQSettings->PlotWithBackground)
  //    SpectrumBackground_H->Draw("C SAME");

  if(ADAQSettings->FindIntegral)
    {};//IntegrateSpectrum();
  
  // Update the canvas
  TheCanvas->Update();

  // Set the class member int that tells the code what is currently
  // plotted on the embedded canvas. This is important for influencing
  // the behavior of the vertical and horizontal double sliders used
  // to "zoom" on the canvas contents
  CanvasContentType = zSpectrum;
}
