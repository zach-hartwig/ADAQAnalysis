#include <TROOT.h>
#include <TStyle.h>
#include <TMarker.h>
#include <TPaletteAxis.h>


#include <iostream>
#include <cmath>
#include <sstream>
using namespace std;

#include "AAGraphics.hh"


AAGraphics *AAGraphics::TheGraphicsManager = 0;


AAGraphics *AAGraphics::GetInstance()
{ return TheGraphicsManager; }


AAGraphics::AAGraphics()
  : Trigger_L(new TLine), Floor_L(new TLine), Baseline_B(new TBox), ZSCeiling_L(new TLine),
    LPeakDelimiter_L(new TLine), RPeakDelimiter_L(new TLine), IntegrationRegion_B(new TBox), Calibration_L(new TLine),
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
  ZSCeiling_L->SetLineColor(kViolet+1);
  ZSCeiling_L->SetLineWidth(2);

  LPeakDelimiter_L->SetLineStyle(7);
  LPeakDelimiter_L->SetLineWidth(2);
  LPeakDelimiter_L->SetLineColor(kGreen+3);
  
  RPeakDelimiter_L->SetLineStyle(7);
  RPeakDelimiter_L->SetLineWidth(2);
  RPeakDelimiter_L->SetLineColor(kGreen+1);

  IntegrationRegion_B->SetFillColor(kGreen+2);
  IntegrationRegion_B->SetFillStyle(3003);
  
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
  PSDPeakOffset_L->SetLineColor(kMagenta+3);
  PSDPeakOffset_L->SetLineWidth(2);

  PSDTailOffset_L->SetLineStyle(7);
  PSDTailOffset_L->SetLineColor(kMagenta+1);
  PSDTailOffset_L->SetLineWidth(2);

  PSDTailIntegral_B->SetFillStyle(3003);
  PSDTailIntegral_B->SetFillColor(kMagenta+2);

  LowerLimit_L->SetLineStyle(7);
  LowerLimit_L->SetLineColor(2);
  LowerLimit_L->SetLineWidth(2);

  UpperLimit_L->SetLineStyle(7);
  UpperLimit_L->SetLineColor(2);
  UpperLimit_L->SetLineWidth(2);

  DerivativeReference_L->SetLineStyle(7);
  DerivativeReference_L->SetLineColor(2);
  DerivativeReference_L->SetLineWidth(2);

  ComputationMgr = AAComputation::GetInstance();
}


AAGraphics::~AAGraphics()
{
  delete TheGraphicsManager;
}


void AAGraphics::PlotWaveform()
{
  TH1F *Waveform_H = 0;
  
  int Channel = ADAQSettings->WaveformChannel;
  int Waveform = ADAQSettings->WaveformToPlot;
  
  if(ADAQSettings->RawWaveform)
    Waveform_H = ComputationMgr->CalculateRawWaveform(Channel, Waveform);

  else if(ADAQSettings->BSWaveform)
    Waveform_H = ComputationMgr->CalculateBSWaveform(Channel, Waveform);

  else if(ADAQSettings->ZSWaveform)
    Waveform_H = ComputationMgr->CalculateZSWaveform(Channel, Waveform);


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
  else if(ADAQSettings->PlotYAxisWithAutoRange){
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
			ComputationMgr->GetADAQMeasurementParameters()->TriggerThreshold[Channel], 
			XMax,
			ComputationMgr->GetADAQMeasurementParameters()->TriggerThreshold[Channel]);

  if(ADAQSettings->PlotBaselineCalcRegion and !ADAQSettings->ZSWaveform){
    double Baseline = ComputationMgr->CalculateBaseline(Waveform_H);
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
  
  if(ADAQSettings->PlotFloor)
    Floor_L->DrawLine(XMin, ADAQSettings->Floor, XMax, ADAQSettings->Floor);

  if(ADAQSettings->FindPeaks){
    ComputationMgr->FindPeaks(Waveform_H);
    
    vector<PeakInfoStruct> PeakInfoVec = ComputationMgr->GetPeakInfoVec();
    
    vector<PeakInfoStruct>::iterator it;
    for(it = PeakInfoVec.begin(); it != PeakInfoVec.end(); it++){

      TMarker *PeakMarker = new TMarker((*it).PeakPosX,
					(*it).PeakPosY*1.15,
					it-PeakInfoVec.begin());
      
      PeakMarker->SetMarkerColor(kRed);
      PeakMarker->SetMarkerStyle(23);
      PeakMarker->SetMarkerSize(2.0);
      PeakMarker->Draw();


      if(ADAQSettings->PlotCrossings){
	TMarker *Low2HighMarker = new TMarker((*it).PeakLimit_Lower,
					      ADAQSettings->Floor,
					      it-PeakInfoVec.begin());
	Low2HighMarker->SetMarkerColor(kGreen+3);
	Low2HighMarker->SetMarkerStyle(29);
	Low2HighMarker->SetMarkerSize(2.0);
	Low2HighMarker->Draw();
	
	TMarker *High2LowMarker = new TMarker((*it).PeakLimit_Upper, 
					      ADAQSettings->Floor,
					      it-PeakInfoVec.begin());
	High2LowMarker->SetMarkerColor(kGreen+1);
	High2LowMarker->SetMarkerStyle(29);
	High2LowMarker->SetMarkerSize(2.0);
	High2LowMarker->Draw();
      }


      if(ADAQSettings->PlotPeakIntegrationRegion){

	LPeakDelimiter_L->DrawLine((*it).PeakLimit_Lower,
				   YMin,
				   (*it).PeakLimit_Lower,
				   YMax);
	
	RPeakDelimiter_L->DrawLine((*it).PeakLimit_Upper,
				   YMin,
				   (*it).PeakLimit_Upper,
				   YMax);    
	
	if((*it).PileupFlag == true)
	  IntegrationRegion_B->SetFillColor(kRed);
	else
	  IntegrationRegion_B->SetFillColor(kGreen+2);
	
	IntegrationRegion_B->DrawBox((*it).PeakLimit_Lower,
				     YMin,
				     (*it).PeakLimit_Upper,
				     YMax);
      }
      
      
      if(ADAQSettings->PSDEnable and ADAQSettings->PSDPlotTailIntegrationRegion)
	if(ADAQSettings->PSDChannel == ADAQSettings->WaveformChannel){
	  
	  int PeakOffset = ADAQSettings->PSDPeakOffset;
	  int LowerPos = (*it).PeakPosX + PeakOffset;
	  
	  int TailOffset = ADAQSettings->PSDTailOffset;
	  int UpperPos = (*it).PeakLimit_Upper + TailOffset;
	  
	  PSDPeakOffset_L->DrawLine(LowerPos, YMin, LowerPos, YMax);
	  PSDTailIntegral_B->DrawBox(LowerPos, YMin, UpperPos, YMax);
	  PSDTailOffset_L->DrawLine(UpperPos, YMin, UpperPos, YMax);
	}
    }
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

  


  
  // Set the class member int that tells the code what is currently
  // plotted on the embedded canvas. This is important for influencing
  // the behavior of the vertical and horizontal double sliders used
  // to "zoom" on the canvas contents
  CanvasContentType = zWaveform;
  
  //  if(ADAQSettings->PlotPearsonIntegration)
  //    IntegratePearsonWaveform(true);
  
  TheCanvas->Update();
}


void AAGraphics::PlotSpectrum()
{
  //////////////////////////////////
  // Determine main spectrum to plot

  TH1F *Spectrum_H = 0;
  
  if(ADAQSettings->FindBackground and ADAQSettings->PlotLessBackground)
    Spectrum_H = ComputationMgr->GetSpectrumWithoutBackground();
  else
    Spectrum_H = ComputationMgr->GetSpectrum();
  
  if(!Spectrum_H)
    return;

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
  if(ADAQSettings->FindBackground and ADAQSettings->PlotWithBackground){
    TH1F *SpectrumBackground_H = ComputationMgr->GetSpectrumBackground();
    SpectrumBackground_H->Draw("C SAME");
  }

  if(ADAQSettings->SpectrumFindIntegral){
    TH1F *SpectrumIntegral_H = ComputationMgr->GetSpectrumIntegral();
    SpectrumIntegral_H->Draw("B SAME");
    SpectrumIntegral_H->Draw("C SAME");
    
    if(ADAQSettings->SpectrumUseGaussianFit){
      TF1 *SpectrumFit_F = ComputationMgr->GetSpectrumFit();
      SpectrumFit_F->Draw("SAME");
    }
  }
  
  // Update the canvas
  TheCanvas->Update();
  
  // Set the class member int that tells the code what is currently
  // plotted on the embedded canvas. This is important for influencing
  // the behavior of the vertical and horizontal double sliders used
  // to "zoom" on the canvas contents
  CanvasContentType = zSpectrum;
}


// Method to extract the digitized data on the specified data channel
// and store it into a TH1F object as a "raw" waveform. Note that the
// baseline must be calculated in this method even though it is not
// used to operate on the waveform
void AAGraphics::PlotPSDHistogram()
{
  TH2F *PSDHistogram_H = ComputationMgr->GetPSDHistogram();
  
  // Check to ensure that the PSD histogram object exists!
  if(!PSDHistogram_H){
    //    CreateMessageBox("A valid PSDHistogram object was not found! PSD histogram plotting will abort!","Stop");
    return;
  }

  // If the PSD histogram has zero events then alert the user and
  // abort plotting. This can happen, for example, if the PSD channel
  // was incorrectly set to an "empty" data channel
  if(PSDHistogram_H->GetEntries() == 0){
    //    CreateMessageBox("The PSD histogram had zero entries! Recheck PSD settings and take another whirl around the merry-go-round!","Stop");
    return;
  }
  
  //////////////////////
  // X and Y axis ranges
  
  double XMin = PSDHistogram_H->GetXaxis()->GetXmax() * ADAQSettings->XAxisMin;
  double XMax = PSDHistogram_H->GetXaxis()->GetXmax() * ADAQSettings->XAxisMax;
  PSDHistogram_H->GetXaxis()->SetRangeUser(XMin, XMax);
  
  double YMin = PSDHistogram_H->GetYaxis()->GetXmax() * (1-ADAQSettings->YAxisMax);
  double YMax = PSDHistogram_H->GetYaxis()->GetXmax() * (1-ADAQSettings->YAxisMin);
  PSDHistogram_H->GetYaxis()->SetRangeUser(YMin, YMax);
  
  string Title, XTitle, YTitle, ZTitle, PaletteTitle;

  if(ADAQSettings->OverrideGraphicalDefault){
    Title = ADAQSettings->PlotTitle;
    XTitle = ADAQSettings->XAxisTitle;
    YTitle = ADAQSettings->YAxisTitle;
    ZTitle = ADAQSettings->ZAxisTitle;
    PaletteTitle = ADAQSettings->PaletteTitle;
  }
  else{
    Title = "Pulse Shape Discrimination Integrals";
    XTitle = "Waveform total integral [ADC]";
    YTitle = "Waveform tail integral [ADC]";
    ZTitle = "Number of waveforms";
    PaletteTitle ="";
  }

  // Get the desired axes divisions
  int XDivs = ADAQSettings->XDivs;
  int YDivs = ADAQSettings->YDivs;
  int ZDivs = ADAQSettings->ZDivs;

  // Hack to make the X axis of the 2D PSD histogram actually look
  // decent given the large numbers involved
  if(ADAQSettings->OverrideGraphicalDefault)
    XDivs = 505;
  
  if(ADAQSettings->StatsOff)
    PSDHistogram_H->SetStats(false);
  else
    PSDHistogram_H->SetStats(true);
  
  TheCanvas->SetLeftMargin(0.13);
  TheCanvas->SetBottomMargin(0.12);
  TheCanvas->SetRightMargin(0.2);

  ////////////////////////////////
  // Axis and graphical attributes
  
  if(ADAQSettings->PlotVerticalAxisInLog)
    gPad->SetLogz(true);
  else
    gPad->SetLogz(false);
  
  // The X and Y axes should never be plotted in log
  gPad->SetLogx(false);
  gPad->SetLogy(false);

  PSDHistogram_H->SetTitle(Title.c_str());
  
  PSDHistogram_H->GetXaxis()->SetTitle(XTitle.c_str());
  PSDHistogram_H->GetXaxis()->SetTitleSize(ADAQSettings->XSize);
  PSDHistogram_H->GetXaxis()->SetLabelSize(ADAQSettings->XSize);
  PSDHistogram_H->GetXaxis()->SetTitleOffset(ADAQSettings->XOffset);
  PSDHistogram_H->GetXaxis()->CenterTitle();
  PSDHistogram_H->GetXaxis()->SetNdivisions(XDivs, true);

  PSDHistogram_H->GetYaxis()->SetTitle(YTitle.c_str());
  PSDHistogram_H->GetYaxis()->SetTitleSize(ADAQSettings->YSize);
  PSDHistogram_H->GetYaxis()->SetLabelSize(ADAQSettings->YSize);
  PSDHistogram_H->GetYaxis()->SetTitleOffset(ADAQSettings->YOffset);
  PSDHistogram_H->GetYaxis()->CenterTitle();
  PSDHistogram_H->GetYaxis()->SetNdivisions(YDivs, true);

  PSDHistogram_H->GetZaxis()->SetTitle(ZTitle.c_str());
  PSDHistogram_H->GetZaxis()->SetTitleSize(ADAQSettings->ZSize);
  PSDHistogram_H->GetZaxis()->SetLabelSize(ADAQSettings->ZSize);
  PSDHistogram_H->GetZaxis()->SetTitleOffset(ADAQSettings->ZOffset);
  PSDHistogram_H->GetZaxis()->CenterTitle();
  PSDHistogram_H->GetZaxis()->SetNdivisions(ZDivs, true);

  switch(ADAQSettings->PSDPlotType){

  case 0:
    PSDHistogram_H->Draw("COL Z");
    break;

  case 1:
    PSDHistogram_H->Draw("LEGO2 0Z");
    break;

  case 2:
    PSDHistogram_H->Draw("SURF3 Z");
    break;

  case 3:
    PSDHistogram_H->SetFillColor(kOrange);
    PSDHistogram_H->Draw("SURF4");
    break;

  case 4:
    PSDHistogram_H->Draw("CONT Z");
    break;
  }


  // Modify the histogram color palette only for the histograms that
  // actually create a palette
  if(ADAQSettings->PSDPlotType != 3){
    
    // The canvas must be updated before the TPaletteAxis is accessed
    TheCanvas->Update();
    
    TPaletteAxis *ColorPalette = (TPaletteAxis *)PSDHistogram_H->GetListOfFunctions()->FindObject("palette");
    ColorPalette->GetAxis()->SetTitle(PaletteTitle.c_str());
    ColorPalette->GetAxis()->SetTitleSize(ADAQSettings->PaletteSize);
    ColorPalette->GetAxis()->SetTitleOffset(ADAQSettings->PaletteOffset);
    ColorPalette->GetAxis()->CenterTitle();
    ColorPalette->GetAxis()->SetLabelSize(ADAQSettings->PaletteSize);
    
    ColorPalette->SetX1NDC(ADAQSettings->PaletteX1);
    ColorPalette->SetX2NDC(ADAQSettings->PaletteX2);
    ColorPalette->SetY1NDC(ADAQSettings->PaletteY1);
    ColorPalette->SetY2NDC(ADAQSettings->PaletteY2);
    
    ColorPalette->Draw("SAME");
  }
  
  if(ADAQSettings->PSDEnableFilterCreation)
    {}//PlotPSDFilter();

  CanvasContentType = zPSDHistogram;
  
  TheCanvas->Update();
}


void AAGraphics::PlotSpectrumDerivative()
{
  TGraph *SpectrumDerivative_G = ComputationMgr->CalculateSpectrumDerivative();

  string Title, XTitle, YTitle;

  if(ADAQSettings->OverrideGraphicalDefault){
    // Assign the titles to strings
    Title = ADAQSettings->PlotTitle;
    XTitle = ADAQSettings->XAxisTitle;
    YTitle = ADAQSettings->YAxisTitle;
  }
  // ... otherwise use the default titles
  else{
    // Assign the default titles
    Title = "Spectrum Derivative";
    XTitle = "Pulse units [ADC]";
    YTitle = "Counts";
  }

  // Get the desired axes divisions
  int XDivs = ADAQSettings->XDivs;
  int YDivs = ADAQSettings->YDivs;


  /////////////////////
  // Logarithmic Y axis
  
  if(ADAQSettings->PlotVerticalAxisInLog)
    gPad->SetLogy(true);
  else
    gPad->SetLogy(false);

  TheCanvas->SetLeftMargin(0.13);
  TheCanvas->SetBottomMargin(0.12);
  TheCanvas->SetRightMargin(0.05);


  ///////////////////
  // Create the graph

  gStyle->SetEndErrorSize(0);



  SpectrumDerivative_G->SetTitle(Title.c_str());

  SpectrumDerivative_G->GetXaxis()->SetTitle(XTitle.c_str());
  SpectrumDerivative_G->GetXaxis()->SetTitleSize(ADAQSettings->XSize);
  SpectrumDerivative_G->GetXaxis()->SetLabelSize(ADAQSettings->XSize);
  SpectrumDerivative_G->GetXaxis()->SetTitleOffset(ADAQSettings->XOffset);
  SpectrumDerivative_G->GetXaxis()->CenterTitle();
  SpectrumDerivative_G->GetXaxis()->SetNdivisions(XDivs, true);

  SpectrumDerivative_G->GetYaxis()->SetTitle(YTitle.c_str());
  SpectrumDerivative_G->GetYaxis()->SetTitleSize(ADAQSettings->YSize);
  SpectrumDerivative_G->GetYaxis()->SetLabelSize(ADAQSettings->YSize);
  SpectrumDerivative_G->GetYaxis()->SetTitleOffset(ADAQSettings->YOffset);
  SpectrumDerivative_G->GetYaxis()->CenterTitle();
  SpectrumDerivative_G->GetYaxis()->SetNdivisions(YDivs, true);
  
  SpectrumDerivative_G->SetLineColor(2);
  SpectrumDerivative_G->SetLineWidth(2);
  SpectrumDerivative_G->SetMarkerStyle(20);
  SpectrumDerivative_G->SetMarkerSize(1.0);
  SpectrumDerivative_G->SetMarkerColor(2);

 
  //////////////////////
  // X and Y axis limits
  
  double XMin = SpectrumDerivative_G->GetXaxis()->GetXmax() * ADAQSettings->XAxisMin;
  double XMax = SpectrumDerivative_G->GetXaxis()->GetXmax() * ADAQSettings->XAxisMax;
  SpectrumDerivative_G->GetXaxis()->SetRangeUser(XMin,XMax);
  
  // Calculates values that will allow the sliders to span the full
  // range of the derivative that, unlike all the other plotted
  // objects, typically has a substantial negative range
  double MinValue = SpectrumDerivative_G->GetHistogram()->GetMinimum();
  double MaxValue = SpectrumDerivative_G->GetHistogram()->GetMaximum();
  double AbsValue = abs(MinValue) + abs(MaxValue);
  
  double YMin = MinValue + (AbsValue * (1-ADAQSettings->YAxisMax));
  double YMax = MaxValue - (AbsValue * ADAQSettings->YAxisMin);
  
  SpectrumDerivative_G->GetYaxis()->SetRangeUser(YMin, YMax);
    
  if(ADAQSettings->PlotSpectrumDerivativeError){
    SpectrumDerivative_G->SetLineColor(1);
    SpectrumDerivative_G->Draw("AP");
  }
  else
    SpectrumDerivative_G->Draw("ALP");
  
  // Set the class member int denoting that the canvas now contains
  // only a spectrum derivative
  CanvasContentType = zSpectrumDerivative;

  TheCanvas->Update();
}


void AAGraphics::PlotCalibration(int Channel)
{
  vector<TGraph *> CalibrationManager = ComputationMgr->GetCalibrationManager();
  
  stringstream ss;
  ss << "CalibrationManager TGraph for Channel[" << Channel << "]";
  string Title = ss.str();
  
  CalibrationManager[Channel]->SetTitle(Title.c_str());
  
  CalibrationManager[Channel]->GetXaxis()->SetTitle("Pulse unit [ADC]");
  CalibrationManager[Channel]->GetXaxis()->SetTitleSize(0.05);
  CalibrationManager[Channel]->GetXaxis()->SetTitleOffset(1.2);
  CalibrationManager[Channel]->GetXaxis()->CenterTitle();
  CalibrationManager[Channel]->GetXaxis()->SetLabelSize(0.05);
  
  CalibrationManager[Channel]->GetYaxis()->SetTitle("Energy");
  CalibrationManager[Channel]->GetYaxis()->SetTitleSize(0.05);
  CalibrationManager[Channel]->GetYaxis()->SetTitleOffset(1.2);
  CalibrationManager[Channel]->GetYaxis()->CenterTitle();
  CalibrationManager[Channel]->GetYaxis()->SetLabelSize(0.05);
  
  CalibrationManager[Channel]->SetMarkerSize(2);
  CalibrationManager[Channel]->SetMarkerStyle(23);
  CalibrationManager[Channel]->SetMarkerColor(1);
  CalibrationManager[Channel]->SetLineWidth(2);
  CalibrationManager[Channel]->SetLineColor(1);
  CalibrationManager[Channel]->Draw("ALP");
  
  TheCanvas->Update();

}


void AAGraphics::PlotCalibrationLine(double XPos, double YMin, double YMax)
{
  Calibration_L->DrawLine(XPos, YMin, XPos, YMax);
}


void AAGraphics::PlotPSDFilter()
{
  vector<TGraph *> PSDFilter = ComputationMgr->GetPSDFilterManager();

  int PSDChannel = ADAQSettings->PSDChannel;

  PSDFilter[PSDChannel]->SetLineColor(2);
  PSDFilter[PSDChannel]->SetLineWidth(2);
  PSDFilter[PSDChannel]->SetMarkerStyle(20);
  PSDFilter[PSDChannel]->SetMarkerColor(2);
  PSDFilter[PSDChannel]->Draw("LP SAME");
  
  TheCanvas->Update();
}


void AAGraphics::PlotPSDHistogramSlice(int XPixel, int YPixel)
{
  TH1D *PSDHistogramSlice_H = ComputationMgr->GetPSDHistogramSlice();

  ////////////////////
  // Canvas assignment

  string SliceCanvasName = "PSDSlice_C";
  string SliceHistogramName = "PSDSlice_H";

  // Get the list of current canvas objects
  TCanvas *PSDSlice_C = (TCanvas *)gROOT->GetListOfCanvases()->FindObject(SliceCanvasName.c_str());

  // If the canvas then delete the contents to prevent memory leaks
  if(PSDSlice_C)
    delete PSDSlice_C->GetPrimitive(SliceHistogramName.c_str());
  
  // ... otherwise, create a new canvas
  else{
    PSDSlice_C = new TCanvas(SliceCanvasName.c_str(), "PSD Histogram Slice", 700, 500, 600, 400);
    PSDSlice_C->SetGrid(true);
    PSDSlice_C->SetLeftMargin(0.13);
    PSDSlice_C->SetBottomMargin(0.13);
  }
  
  // Ensure the PSD slice canvas is the active canvas
  PSDSlice_C->cd();

  string Title, XTitle;
  stringstream ss;
    
  if(ADAQSettings->PSDXSlice){
    ss << "PSDHistogram X slice at " << gPad->AbsPixeltoX(XPixel) << " ADC";
    Title = ss.str();
    XTitle = "PSD Tail Integral [ADC]";
  }
  else if(ADAQSettings->PSDYSlice){
    ss << "PSDHistogram Y slice at " << gPad->AbsPixeltoY(YPixel) << " ADC";
    Title == ss.str();
    XTitle = "PSD Total Integral [ADC]";
  }

  
  /////////////////////////////////
  // Set slice histogram attributes
  
  PSDHistogramSlice_H->SetName(SliceHistogramName.c_str());
  PSDHistogramSlice_H->SetTitle(Title.c_str());

  PSDHistogramSlice_H->GetXaxis()->SetTitle(XTitle.c_str());
  PSDHistogramSlice_H->GetXaxis()->SetTitleSize(0.05);
  PSDHistogramSlice_H->GetXaxis()->SetTitleOffset(1.1);
  PSDHistogramSlice_H->GetXaxis()->SetLabelSize(0.05);
  PSDHistogramSlice_H->GetXaxis()->CenterTitle();
  
  PSDHistogramSlice_H->GetYaxis()->SetTitle("Counts");
  PSDHistogramSlice_H->GetYaxis()->SetTitleSize(0.05);
  PSDHistogramSlice_H->GetYaxis()->SetTitleOffset(1.1);
  PSDHistogramSlice_H->GetYaxis()->SetLabelSize(0.05);
  PSDHistogramSlice_H->GetYaxis()->CenterTitle();
  
  PSDHistogramSlice_H->SetFillColor(4);
  PSDHistogramSlice_H->Draw("B");
  
  // Update the standalone canvas
  PSDSlice_C->Update();

  gPad->GetCanvas()->FeedbackMode(kFALSE);
  
  // Reset the main embedded canvas to active
  TheCanvas->cd();
}
