/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                           Copyright (C) 2012-2021                           //
//                 Zachary Seth Hartwig : All rights reserved                  //
//                                                                             //
//      The ADAQAnalysis source code is licensed under the GNU GPL v3.0.       //
//      You have the right to modify and/or redistribute this source code      //      
//      under the terms specified in the license, which may be found online    //
//      at http://www.gnu.org/licenses or at $ADAQANALYSIS/License.txt.       //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// 
// name: AAGraphics.cc
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

// ROOT
#include <TROOT.h>
#include <TStyle.h>
#include <TMarker.h>
#include <TPaletteAxis.h>
#include <TFrame.h>
#include <TPad.h>
#include <TColorWheel.h>
#include <TGClient.h>
#include <TRootCanvas.h>
#include <THashList.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TF1.h>

// C++
#include <iostream>
#include <cmath>
#include <sstream>
using namespace std;

#include "AAGraphics.hh"


// The static meyer's singleton 
AAGraphics *AAGraphics::TheGraphicsManager = 0;


// Access method to the Meyer's singleton pointer
AAGraphics *AAGraphics::GetInstance()
{ return TheGraphicsManager; }


AAGraphics::AAGraphics()
  : Trigger_L(new TLine), Floor_L(new TLine), ZSCeiling_L(new TLine),
    Analysis_B(new TBox), Baseline_B(new TBox), 
    LPeakDelimiter_L(new TLine), RPeakDelimiter_L(new TLine), IntegrationRegion_B(new TBox),
    PSDTotal_B(new TBox), PSDPeak_L(new TLine), PSDTail_L0(new TLine), PSDTail_L1(new TLine),
    HCalibration_L(new TLine), VCalibration_L(new TLine), EdgeBoundingBox_B(new TBox),
    EALine_L(new TLine), EABox_B(new TBox), DerivativeReference_L(new TLine),
    PSDRegionProgress_G(new TGraph),
    CanvasContentType(zEmpty), 
    WaveformColor(kBlue), WaveformLineWidth(1), WaveformMarkerSize(1.),
    SpectrumLineColor(kBlue), SpectrumLineWidth(2), 
    SpectrumFillColor(kRed), SpectrumFillStyle(3002), PSDFigureOfMerit(0.)
{
  if(TheGraphicsManager)
    cout << "\nERROR! TheGraphicsManager was constructed twice\n" << endl;
  TheGraphicsManager = this;

  // Initialize the color palette
  gStyle->SetPalette(kBird);
  
  // All of the graphical objects for waveform and spectral analysis
  // (TLines, TBoxes, etc) are declared once in order to save memory
  // and prevent memory leaks. Attributes are set here and then actual
  // drawing is carried out via the TLine::DrawLine() or
  // TBox::DrawBox() methods.

  // Graphical objects for waveform analysis

  Trigger_L->SetLineStyle(7);
  Trigger_L->SetLineColor(kRed);
  Trigger_L->SetLineWidth(2);

  Floor_L->SetLineStyle(7);
  Floor_L->SetLineColor(kRed);
  Floor_L->SetLineWidth(2);

  ZSCeiling_L->SetLineStyle(7);
  ZSCeiling_L->SetLineColor(kViolet+1);
  ZSCeiling_L->SetLineWidth(2);
  
  Analysis_B->SetFillStyle(3002);
  Analysis_B->SetFillColor(kBlue);
  
  Baseline_B->SetFillStyle(3002);
  Baseline_B->SetFillColor(kRed);

  LPeakDelimiter_L->SetLineStyle(7);
  LPeakDelimiter_L->SetLineColor(kGreen+3);
  LPeakDelimiter_L->SetLineWidth(2);
  
  RPeakDelimiter_L->SetLineStyle(7);
  RPeakDelimiter_L->SetLineColor(kGreen+1);
  RPeakDelimiter_L->SetLineWidth(2);

  IntegrationRegion_B->SetFillColor(kGreen+2);
  IntegrationRegion_B->SetFillStyle(3003);

  PSDTotal_B->SetFillStyle(3003);
  PSDTotal_B->SetFillColor(kMagenta+2);

  PSDPeak_L->SetLineStyle(7);
  PSDPeak_L->SetLineColor(kMagenta+3);
  PSDPeak_L->SetLineWidth(2);

  PSDTail_L0->SetLineStyle(7);
  PSDTail_L0->SetLineColor(kMagenta+1);
  PSDTail_L0->SetLineWidth(2);

  PSDTail_L1->SetLineStyle(7);
  PSDTail_L1->SetLineColor(kMagenta+1);
  PSDTail_L1->SetLineWidth(2);

  // Graphical objects for spectral analysis
  
  HCalibration_L->SetLineStyle(7);
  HCalibration_L->SetLineColor(kRed);
  HCalibration_L->SetLineWidth(2);

  VCalibration_L->SetLineStyle(7);
  VCalibration_L->SetLineColor(kRed);
  VCalibration_L->SetLineWidth(2);

  EdgeBoundingBox_B->SetFillColor(kRed);
  EdgeBoundingBox_B->SetFillStyle(3002);

  EALine_L->SetLineStyle(7);
  EALine_L->SetLineColor(kOrange+7);
  EALine_L->SetLineWidth(2);
  
  EABox_B->SetFillColor(kOrange+7);
  EABox_B->SetFillStyle(3002);

  DerivativeReference_L->SetLineStyle(7);
  DerivativeReference_L->SetLineColor(kRed);
  DerivativeReference_L->SetLineWidth(2);

  // Get the pointer to the computation manager
  ComputationMgr = AAComputation::GetInstance();

  ColorWheel = new TColorWheel;
}


AAGraphics::~AAGraphics()
{
  delete TheGraphicsManager;
}


void AAGraphics::PlotWaveform(int Color)
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
  
  if(ADAQSettings->WaveformAnalysis)
    ComputationMgr->AnalyzeWaveform(Waveform_H);
  

  // Determine the X-axis size and min/max values
  int XAxisSize = Waveform_H->GetNbinsX();

  double XMin = ADAQSettings->XAxisMin * XAxisSize;
  double XMax = ADAQSettings->XAxisMax * XAxisSize;

  // Determine the Y-axis size (dependent on whether or not the user
  // has opted to allow the Y-axis range to be automatically
  // determined by the size of the waveform or fix the Y-Axis range
  // with the vertical double slider positions) and min/max values

  double YMin, YMax, YAxisSize;

  // In order to correctly size the maximum y-axis value for waveform
  // plotting, we need to know the maximum ADC value (set by the
  // digitizer bit depth that was used to acquire the data). This
  // information was NOT stored in the legacy ADAQ files so the
  // default in these cases should be a bit depth of 14 (with a
  // maximum ADC value of 2**14-1 = 16383). However, thew new
  // production ADAQ files make this information available via the
  // ADAQReadoutInforrmation class 
  
  Int_t MaxADC = 16383;
  if(!ComputationMgr->GetADAQLegacyFileLoaded()){
    Int_t BitDepth = ComputationMgr->GetADAQReadoutInformation()->GetDGBitDepth();
    MaxADC = pow(2,BitDepth);
  }
  
  if(ADAQSettings->CanvasYAxisLog){
    YMin = MaxADC * (1 - ADAQSettings->YAxisMax);
    YMax = MaxADC * (1 - ADAQSettings->YAxisMin);
    if(YMin == 0)
      YMin = 0.05;
  }
  else if(ADAQSettings->PlotYAxisWithAutoRange){
    YMin = Waveform_H->GetBinContent(Waveform_H->GetMinimumBin()) - 10;
    YMax = Waveform_H->GetBinContent(Waveform_H->GetMaximumBin()) + 10;
  }
  else{
    YMin = (MaxADC * (1 - ADAQSettings->YAxisMax))-30;
    YMax = MaxADC * (1 - ADAQSettings->YAxisMin);
  }
  
  YAxisSize = YMax - YMin;


  string Title, XTitle, YTitle;
  
  if(ADAQSettings->OverrideGraphicalDefault){
    Title = ADAQSettings->PlotTitle;
    XTitle = ADAQSettings->XAxisTitle;
    YTitle = ADAQSettings->YAxisTitle;
  }
  else{

    string XString;
    if(!ComputationMgr->GetADAQLegacyFileLoaded()){
      Int_t SamplingRate = 
	ComputationMgr->GetADAQReadoutInformation()->GetDGSamplingRate();

      Double_t SampleTime = 1e3 / SamplingRate;

      stringstream SS;
      SS << "Time [" << SampleTime << " ns sample]";
      XString = SS.str();
    }
    else
      XString = "Time [sample]";
    
    Title = "Digitized Waveform";
    XTitle = XString;
    YTitle = "Voltage [ADC]";
  }
  
  int XDivs = ADAQSettings->XDivs;
  int YDivs = ADAQSettings->YDivs;

  TheCanvas->SetLeftMargin(0.13);
  TheCanvas->SetBottomMargin(0.12);
  TheCanvas->SetRightMargin(0.05);

  gPad->SetGrid(ADAQSettings->CanvasGrid, ADAQSettings->CanvasGrid);
  gPad->SetLogx(ADAQSettings->CanvasXAxisLog);
  gPad->SetLogy(ADAQSettings->CanvasYAxisLog);

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

  Waveform_H->SetLineColor(WaveformColor);
  Waveform_H->SetLineWidth(ADAQSettings->WaveformLineWidth);

  Waveform_H->SetStats(false);  
  
  Waveform_H->SetMarkerStyle(24);
  Waveform_H->SetMarkerSize(ADAQSettings->WaveformMarkerSize);
  Waveform_H->SetMarkerColor(WaveformColor);
  
  string DrawString;
  if(ADAQSettings->WaveformLine)
    DrawString = "";
  else if(ADAQSettings->WaveformCurve)
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

  if(ADAQSettings->PlotTrigger){
    
    Int_t Trigger = 0;
    if(ComputationMgr->GetADAQLegacyFileLoaded())
      Trigger = ComputationMgr->GetADAQMeasurementParameters()->TriggerThreshold[Channel]; 
    else
      Trigger = ComputationMgr->GetADAQReadoutInformation()->GetTrigger()[Channel];
    
    Trigger_L->DrawLine(XMin, Trigger, XMax, Trigger);
  }

  if(ADAQSettings->PlotAnalysisRegion and !ADAQSettings->ZSWaveform){
    Analysis_B->DrawBox(ADAQSettings->AnalysisRegionMin,
			YMin + YMax*0.05,
			ADAQSettings->AnalysisRegionMax,
			YMax*0.95);
  }
  
  if(ADAQSettings->PlotBaselineRegion and !ADAQSettings->ZSWaveform){
    double Baseline = ComputationMgr->CalculateBaseline(Waveform_H);
    double BaselinePlotMinValue = Baseline - (0.04 * YAxisSize);
    double BaselinePlotMaxValue = Baseline + (0.04 * YAxisSize);

    if(ADAQSettings->BSWaveform){
      BaselinePlotMinValue = -0.04 * YAxisSize;
      BaselinePlotMaxValue = 0.04 * YAxisSize;
    }
    Baseline_B->DrawBox(ADAQSettings->BaselineRegionMin,
			BaselinePlotMinValue,
			ADAQSettings->BaselineRegionMax,
			BaselinePlotMaxValue);
  }
  
  if(ADAQSettings->PlotFloor)
    Floor_L->DrawLine(XMin, ADAQSettings->Floor, XMax, ADAQSettings->Floor);
  
  if(ADAQSettings->FindPeaks)
    ComputationMgr->FindPeaks(Waveform_H, zPeakFinder);
  else
    ComputationMgr->FindPeaks(Waveform_H, zWholeWaveform);
    
    
  if(ADAQSettings->UsePSDRegions[ADAQSettings->WaveformChannel])
    ComputationMgr->CalculatePSDIntegrals(false);
    
  vector<PeakInfoStruct> PeakInfoVec = ComputationMgr->GetPeakInfoVec();
  
  vector<PeakInfoStruct>::iterator it;
  for(it = PeakInfoVec.begin(); it != PeakInfoVec.end(); it++){

    ////////////////////////
    // Color waveform by PSD 
    
    if(ADAQSettings->UsePSDRegions[ADAQSettings->WaveformChannel]){
      
      if((*it).PSDFilterFlag)
	Waveform_H->SetLineColor(kRed);
      else
	Waveform_H->SetLineColor(kGreen+2);
      
      string NewDrawString = DrawString + " SAME";
      Waveform_H->Draw(DrawString.c_str());
    }
    
    if(ADAQSettings->FindPeaks){
      
      ////////////////////
      // Plot peak markers
      
      TMarker *PeakMarker = new TMarker((*it).PeakPosX,
					(*it).PeakPosY*1.15,
					it-PeakInfoVec.begin());
      
      PeakMarker->SetMarkerColor(kRed);
      PeakMarker->SetMarkerStyle(23);
      PeakMarker->SetMarkerSize(2.0);
      PeakMarker->Draw();
      
      
      /////////////////
      // Plot crossings
      
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
      
      
      //////////////////////////
      // Plot integration region
      
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
    }
      
      
    //////////////////////////////
    // Plot PSD integration region
    
    if(ADAQSettings->PSDPlotIntegrationLimits){
      
      Double_t Peak = (*it).PeakPosX;
    
      Double_t TotalStart = Peak + ADAQSettings->PSDTotalStart;
      Double_t TotalStop = Peak + ADAQSettings->PSDTotalStop;
      Double_t TailStart = Peak + ADAQSettings->PSDTailStart;
      Double_t TailStop = Peak + ADAQSettings->PSDTailStop;
    
      PSDTotal_B->DrawBox(TotalStart, YMin, TotalStop, YMax);
      PSDPeak_L->DrawLine(Peak, YMin, Peak, YMax);
      PSDTail_L0->DrawLine(TailStart, YMin, TailStart, YMax);
      PSDTail_L1->DrawLine(TailStop, YMin, TailStop, YMax);
    }
    
    // Set the class member int that tells the code what is currently
    // plotted on the embedded canvas. This is important for influencing
    // the behavior of the vertical and horizontal double sliders used
    // to "zoom" on the canvas contents
    CanvasContentType = zWaveform;
    
  }
  TheCanvas->Update();
}


void AAGraphics::PlotSpectrum()
{
  //////////////////////////////////
  // Determine main spectrum to plot

  if(ADAQSettings->FindBackground and ADAQSettings->PlotLessBackground)
    Spectrum_H = ComputationMgr->GetSpectrumWithoutBackground();
  else
    Spectrum_H = ComputationMgr->GetSpectrum();

  if(!Spectrum_H)
    return;

  // Set the spectrum x-axis range
  
  Double_t XMinBin = ADAQSettings->SpectrumMinBin;
  Double_t XMaxBin = ADAQSettings->SpectrumMaxBin;
  Double_t Range = XMaxBin - XMinBin;

  Double_t XMin = XMinBin + (Range * ADAQSettings->XAxisMin);
  Double_t XMax = XMinBin + (Range * ADAQSettings->XAxisMax);

  Spectrum_H->GetXaxis()->SetRangeUser(XMin, XMax);


  // Set the spectrum y-axis range

  double YMin, YMax;
  if(ADAQSettings->CanvasYAxisLog and ADAQSettings->YAxisMax==1)
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
  else if(ComputationMgr->GetASIMFileLoaded()){
    Title = "ASIM Spectrum";    
    
    if(ADAQSettings->ASIMSpectrumTypeEnergy)
      XTitle = "Energy deposited [MeV]";
    else if(ADAQSettings->ASIMSpectrumTypePhotonsCreated)
      XTitle = "Visible photons created [#]";
    else if(ADAQSettings->ASIMSpectrumTypePhotonsDetected)
      XTitle = "Visible photons detected [#]";
    
    YTitle = "Counts";
  }
  else{
    Title = "ADAQ Spectrum";
    
    if(ComputationMgr->GetUseSpectraCalibrations()[ADAQSettings->WaveformChannel]){
      if(ADAQSettings->EnergyUnit == 0)
	XTitle = "Energy deposited [keV]";
      else
	XTitle = "Energy deposited [MeV]";
    }
    else{
      if(ADAQSettings->ADAQSpectrumTypePAS)
	XTitle = "Pulse area [ADC]";
      else if (ADAQSettings->ADAQSpectrumTypePHS)
	XTitle = "Pulse height [ADC]";
    }
    YTitle = "Counts";
  }
  
  // Get the desired axes divisions
  int XDivs = ADAQSettings->XDivs;
  int YDivs = ADAQSettings->YDivs;

  
  ///////////////////////
  // Histogram statistics

  Spectrum_H->SetStats(ADAQSettings->HistogramStats);
  
  ////////////////////////////////
  // Axis and graphical attributes

  gPad->SetGrid(ADAQSettings->CanvasGrid, ADAQSettings->CanvasGrid);
  gPad->SetLogx(ADAQSettings->CanvasXAxisLog);
  gPad->SetLogy(ADAQSettings->CanvasYAxisLog);

  TheCanvas->SetLeftMargin(0.13);
  TheCanvas->SetBottomMargin(0.12);
  TheCanvas->SetRightMargin(0.05);

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

  Spectrum_H->SetLineColor(SpectrumLineColor);
  Spectrum_H->SetLineWidth(ADAQSettings->SpectrumLineWidth);

  /////////////////////////
  // Plot the main spectrum

  // "HIST" is required to prevent forced drawing of error bars style
  // since the TH1::Sumw2() function is called during calculationg of
  // the background spectrum for proper error propagation

  string DrawString;
  if(ADAQSettings->SpectrumBars){
    Spectrum_H->SetFillStyle(4100);
    Spectrum_H->SetFillColor(SpectrumLineColor);
    DrawString = "HIST B";
  }
  else if(ADAQSettings->SpectrumLine){
    Spectrum_H->SetFillStyle(4000);
    DrawString = "HIST";
  }
  else if(ADAQSettings->SpectrumCurve){
    Spectrum_H->SetFillStyle(4000);
    DrawString = "HIST C";
  }
  else if (ADAQSettings->SpectrumError){
    Spectrum_H->SetMarkerStyle(24);
    Spectrum_H->SetMarkerColor(SpectrumLineColor);
    Spectrum_H->SetMarkerSize(0.5);
    gStyle->SetEndErrorSize(5);
    
    DrawString = "E1";
  }
  
  Spectrum_H->Draw(DrawString.c_str());

  // Overplot a thin curve on the error bars to help the user's eye
  // make sense of what is typically difficult-to-interpret data
  if(ADAQSettings->SpectrumError){
    TH1F *SpectrumOverplot_H = (TH1F *)Spectrum_H->Clone("SpectrumOverplot_H");
    SpectrumOverplot_H->SetLineColor(SpectrumLineColor);
    SpectrumOverplot_H->SetLineWidth(1);
    SpectrumOverplot_H->Draw("C SAME");
  }
  
  ////////////////////////////////////////////
  // Overlay the background spectra if desired
  if(ADAQSettings->FindBackground and ADAQSettings->PlotWithBackground){
    TH1F *SpectrumBackground_H = ComputationMgr->GetSpectrumBackground();
    SpectrumBackground_H->GetXaxis()->SetRangeUser(XMin, XMax);
    SpectrumBackground_H->Draw("C SAME");
  }
  
  if(ADAQSettings->SpectrumFindIntegral){
    TH1F *SpectrumIntegral_H = ComputationMgr->GetSpectrumIntegral();

    SpectrumIntegral_H->SetLineColor(SpectrumLineColor);
    SpectrumIntegral_H->SetLineWidth(ADAQSettings->SpectrumLineWidth);
    SpectrumIntegral_H->SetFillColor(SpectrumFillColor);
    SpectrumIntegral_H->SetFillStyle(ADAQSettings->SpectrumFillStyle);
    SpectrumIntegral_H->Draw("HIST B SAME");
   
    if(ADAQSettings->SpectrumLine or
       ADAQSettings->SpectrumBars){
      SpectrumIntegral_H->SetLineWidth(0);
      SpectrumIntegral_H->Draw("HIST SAME");
    }
    else if(ADAQSettings->SpectrumCurve or
	    ADAQSettings->SpectrumError)
      SpectrumIntegral_H->Draw("HIST C SAME");
  }
    
  if(ADAQSettings->SpectrumUseGaussianFit){
    TF1 *SpectrumFit_F = ComputationMgr->GetSpectrumFit();
    SpectrumFit_F->Draw("HIST SAME");
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
    Title = "Pulse shape discrimination histogram";
    
    if(ADAQSettings->PSDXAxisEnergy){
      if(ADAQSettings->EnergyUnit == 0)
	XTitle = "Energy deposited [keVee]";
      else
	XTitle = "Energy deposited [MeVee]";
    }
    else
      XTitle = "Waveform total integral [ADC]";

    if(ADAQSettings->PSDYAxisTail)
      YTitle = "Waveform tail integral [ADC]";
    else
      YTitle = "PSD parameter (tail/total)";
    
    ZTitle = "Number of waveforms";
    PaletteTitle ="";
  }
  
  // Get the desired axes divisions
  int XDivs = ADAQSettings->XDivs;
  int YDivs = ADAQSettings->YDivs;
  int ZDivs = ADAQSettings->ZDivs;

  PSDHistogram_H->SetStats(ADAQSettings->HistogramStats);
  
  TheCanvas->SetLeftMargin(0.13);
  TheCanvas->SetBottomMargin(0.12);
  TheCanvas->SetRightMargin(0.17);

  ////////////////////////////////
  
  // Axis and graphical attributes

  gPad->SetGrid(ADAQSettings->CanvasGrid, ADAQSettings->CanvasGrid);
  gPad->SetLogx(ADAQSettings->CanvasXAxisLog);
  gPad->SetLogy(ADAQSettings->CanvasYAxisLog);
  gPad->SetLogz(ADAQSettings->CanvasZAxisLog);

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

  PSDHistogram_H->Draw(ADAQSettings->PSDPlotType.c_str());

  // The canvas must be updated before the TPaletteAxis is accessed
  TheCanvas->Update();

  TPaletteAxis *ColorPalette = NULL;
  ColorPalette = (TPaletteAxis *)PSDHistogram_H->GetListOfFunctions()->FindObject("palette");
  if(ColorPalette != NULL){
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


  ///////////////////
  // Logarithmic axes

  gPad->SetLogx(ADAQSettings->CanvasXAxisLog);
  gPad->SetLogy(ADAQSettings->CanvasYAxisLog);

  TheCanvas->SetLeftMargin(0.13);
  TheCanvas->SetBottomMargin(0.12);
  TheCanvas->SetRightMargin(0.05);


  ///////////////////
  // Create the graph
  
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
  
  gStyle->SetEndErrorSize(5);
  SpectrumDerivative_G->SetLineColor(SpectrumLineColor);
  SpectrumDerivative_G->SetLineWidth(SpectrumLineWidth);
  SpectrumDerivative_G->SetMarkerStyle(20);
  SpectrumDerivative_G->SetMarkerSize(1.0);
  SpectrumDerivative_G->SetMarkerColor(SpectrumLineColor);
  
  
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
    TGraphErrors *SpectrumDerivativeError_G = (TGraphErrors *)
      SpectrumDerivative_G->Clone("SpectrumDerivativeError_G");
    
    SpectrumDerivativeError_G->SetLineColor(kBlack);
    SpectrumDerivativeError_G->SetLineWidth(SpectrumLineWidth);
    
    SpectrumDerivative_G->SetLineWidth(1);
    SpectrumDerivative_G->Draw("APC");

    SpectrumDerivativeError_G->Draw("E");
  }
  else
    SpectrumDerivative_G->Draw("APC");
  
  // Set the class member int denoting that the canvas now contains
  // only a spectrum derivative
  CanvasContentType = zSpectrumDerivative;
  
  TheCanvas->Update();
}


void AAGraphics::PlotCalibration(Int_t Channel)
{
  vector<TGraph *> CalibrationData = ComputationMgr->GetSpectraCalibrationData();
  vector<TF1 *> Calibrations;

  // Determine if the calibration is using a fit (== 0) or
  // interpolation (== 1)
  Int_t CalibrationType = ComputationMgr->GetSpectraCalibrationType()[Channel];


  string Title, XTitle, YTitle;
  stringstream SS;
  
  // Enable the user to custom-set titles
  if(ADAQSettings->OverrideGraphicalDefault){
    Title = ADAQSettings->PlotTitle;
    XTitle = ADAQSettings->XAxisTitle;
    YTitle = ADAQSettings->YAxisTitle;
  }
  else{

    SS << "Spectrum calibration TGraph for channel[" << Channel << "]";
    Title = SS.str();

    if(ADAQSettings->ADAQSpectrumTypePAS)
      XTitle = "Pulse area [ADC]";
    else if(ADAQSettings->ADAQSpectrumTypePHS)
      XTitle = "Pulse height [ADC]";
    
    YTitle = "Energy deposited [MeV]";
  }

  // Plot the calibration data points
  CalibrationData[Channel]->SetTitle(Title.c_str());
  
  CalibrationData[Channel]->GetXaxis()->SetTitle(XTitle.c_str());
  CalibrationData[Channel]->GetXaxis()->SetTitleSize(0.05);
  CalibrationData[Channel]->GetXaxis()->SetTitleOffset(1.2);
  CalibrationData[Channel]->GetXaxis()->CenterTitle();
  CalibrationData[Channel]->GetXaxis()->SetLabelSize(0.05);
  
  CalibrationData[Channel]->GetYaxis()->SetTitle(YTitle.c_str());
  CalibrationData[Channel]->GetYaxis()->SetTitleSize(0.05);
  CalibrationData[Channel]->GetYaxis()->SetTitleOffset(1.2);
  CalibrationData[Channel]->GetYaxis()->CenterTitle();
  CalibrationData[Channel]->GetYaxis()->SetLabelSize(0.05);
  
  CalibrationData[Channel]->SetMarkerStyle(20);
  CalibrationData[Channel]->SetMarkerSize(2.0);
  CalibrationData[Channel]->SetMarkerColor(kBlue);
  if(CalibrationType == 0)
    CalibrationData[Channel]->Draw("AP");
  else if(CalibrationType == 1){
    CalibrationData[Channel]->SetLineColor(kBlue);
    CalibrationData[Channel]->SetLineWidth(2);
    CalibrationData[Channel]->Draw("APL");
  }
  
  if(CalibrationType == 0){
    
    // Overplot the fit to the calibration data
    Calibrations = ComputationMgr->GetSpectraCalibrations();

    Calibrations[Channel]->SetLineStyle(1);
    Calibrations[Channel]->SetLineWidth(2);
    Calibrations[Channel]->SetLineColor(kRed);
    Calibrations[Channel]->Draw("L SAME");
  }

  // Create a legend

  TLegend *TheLegend_L = new TLegend(0.2, 0.7, 0.4, 0.9);
  TheLegend_L->AddEntry(CalibrationData[Channel], "Data", "P");
  if(CalibrationType == 0)
    TheLegend_L->AddEntry(Calibrations[Channel], "Fit", "L");
  TheLegend_L->Draw();

  // Plot an equation of the fit on the canvas

  if(CalibrationType == 0){
  
    SS.str("");
    SS << "E = ";
    
    const Int_t NParameters = Calibrations[Channel]->GetNpar();
    Double_t *Parameters = Calibrations[Channel]->GetParameters();
    
    vector<string> Variables;
    Variables.push_back("1");
    Variables.push_back("P");
    Variables.push_back("P^{2}");
    
    for(Int_t p=0; p<NParameters; p++){
      SS << Parameters[p] << " * " << Variables[p];
      if((p+1) != NParameters)
	SS << " + ";
    }
    
    TString EquationString = SS.str();
    
    TLatex *Equation = new TLatex;
    Equation->SetTextColor(kRed);
    Equation->SetTextSize(0.045);
    Equation->DrawTextNDC(0.22, 0.15, EquationString);
  }
  TheCanvas->Update();
}

// Horizontal calibration line
void AAGraphics::PlotHCalibrationLine(double Y, bool Refresh)
{ 
  if(Refresh)
    PlotSpectrum();

  double XMin = gPad->GetFrame()->GetX1();
  double XMax = gPad->GetFrame()->GetX2();
  
  HCalibration_L->DrawLine(XMin, Y, XMax, Y);
  
  if(Refresh)
    TheCanvas->Update();
}


// Vertical calibration line
void AAGraphics::PlotVCalibrationLine(double X, bool refresh)
{
  if(refresh)
    PlotSpectrum();
  
  double YMin = gPad->GetFrame()->GetY1();
  double YMax = gPad->GetFrame()->GetY2();
  
  // If the YAxis is in log then {Y1,Y2} must be converted to linear
  // scale in order to correctly draw the line
  if(gPad->GetLogy()){
    YMin = pow(10, YMin);
    YMax = pow(10, YMax);
  }

  VCalibration_L->DrawLine(X, YMin, X, YMax);

  if(refresh)
    TheCanvas->Update();
}


// A "cross" of horiz. and vert. calibration lines
void AAGraphics::PlotCalibrationCross(double EdgePos, double HalfHeight)
{
  PlotSpectrum();
  PlotVCalibrationLine(EdgePos, false);
  PlotHCalibrationLine(HalfHeight, false);
  TheCanvas->Update();
}


// A transparent box that encompasses the calibration edge
void AAGraphics::PlotEdgeBoundingBox(double X0, double Y0, double X1, double Y1)
{
  PlotSpectrum();

  if(gPad->GetLogy()){
    Y0 = pow(10, Y0);
    Y1 = pow(10, Y1);
  }

  EdgeBoundingBox_B->DrawBox(X0, Y0, X1, Y1);
  TheCanvas->Update();
}


void AAGraphics::PlotEALine(double X, double Error, bool ErrorBox, bool EscapePeaks)
{
  PlotSpectrum();
  
  double YMin = gPad->GetFrame()->GetY1();
  double YMax = gPad->GetFrame()->GetY2();
  
  // If the YAxis is in log then {Y1,Y2} must be converted to linear
  // scale in order to correctly draw the line
  if(gPad->GetLogy()){
    YMin = pow(10, YMin);
    YMax = pow(10, YMax);
  }
  
  EALine_L->DrawLine(X, YMin, X, YMax);
  
  // Convert error from % to decimal
  Error *= 0.01;
  
  if(ErrorBox)
    EABox_B->DrawBox(X*(1-Error), YMin, X*(1+Error), YMax);
  
  // The first and double escape peak energy differences below the
  // full energy gamma energy deposition peak. Be sure to account for
  // the user's calibration units, keV or MeV.

  double Peak0 = 0.511; // [MeV]
  double Peak1 = 1.022; // [MeV]
  if(ADAQSettings->EnergyUnit == 0){
    Peak0 *= 1000;
    Peak1 *= 1000;
  }

  if(EscapePeaks){
    EALine_L->DrawLine(X-Peak0, YMin, X-Peak0, YMax);
    EALine_L->DrawLine(X-Peak1, YMin, X-Peak1, YMax);
  }
  
  TheCanvas->Update();
}


void AAGraphics::PlotPSDRegionProgress()
{
  vector<Double_t> X = ComputationMgr->GetPSDRegionXPoints();
  vector<Double_t> Y = ComputationMgr->GetPSDRegionYPoints();
  
  if(PSDRegionProgress_G != NULL)
    delete PSDRegionProgress_G;
  
  PSDRegionProgress_G = new TGraph(X.size(), &X[0], &Y[0]);
  PSDRegionProgress_G->SetMarkerStyle(20);
  PSDRegionProgress_G->SetMarkerSize(1.);
  PSDRegionProgress_G->SetMarkerColor(kMagenta);
  PSDRegionProgress_G->SetLineStyle(7);
  PSDRegionProgress_G->SetLineWidth(2);
  PSDRegionProgress_G->SetLineColor(kMagenta);  
  PSDRegionProgress_G->Draw("LP");

  TheCanvas->Update();
}

void AAGraphics::PlotPSDRegion()
{
  // Delete the TGraph that shows the user's progress of creating the
  // PSDRegion in order to remove it from the canvas

  if(PSDRegionProgress_G != NULL){
    delete PSDRegionProgress_G;
    PSDRegionProgress_G = new TGraph;
  }

  // Get the TCutG object from the computation manager and draw it

  vector<TCutG *> PSDRegions = ComputationMgr->GetPSDRegions();
  
  int Channel = ADAQSettings->WaveformChannel;
  
  PSDRegions[Channel]->SetMarkerStyle(20);
  PSDRegions[Channel]->SetMarkerSize(1.);
  PSDRegions[Channel]->SetMarkerColor(kMagenta);
  PSDRegions[Channel]->SetLineStyle(1);
  PSDRegions[Channel]->SetLineWidth(2);
  PSDRegions[Channel]->SetLineColor(kMagenta);
  PSDRegions[Channel]->SetFillColor(kMagenta);
  PSDRegions[Channel]->SetFillStyle(3002);
  PSDRegions[Channel]->Draw("PF");
  PSDRegions[Channel]->Draw("L");
  
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
    PSDSlice_C->SetGrid(ADAQSettings->CanvasGrid, ADAQSettings->CanvasGrid);
    PSDSlice_C->SetLeftMargin(0.13);
    PSDSlice_C->SetBottomMargin(0.13);
    PSDSlice_C->Connect("Closed()", "AAGraphics", this, "ClosePSDSliceWindow()");
  }

  /////////////////////////
  // Assign the axis titles

  // Ensure that the 2D PSD histogram canvas is active in order to
  // extract the correct X and Y values in ADC
  TheCanvas->cd();

  string Title, XTitle;
  stringstream ss;
    
  if(ADAQSettings->PSDXSlice){

    double XPosInADC = gPad->AbsPixeltoX(XPixel);

    ss << "PSDHistogram X slice at " << XPosInADC;
    if(ADAQSettings->PSDXAxisEnergy){
      if(ADAQSettings->EnergyUnit == 0)
	ss << " keVee";
      else
	ss << " MeVee";
    }
    else
      ss << " ADC";
    
    Title = ss.str();

    if(ADAQSettings->PSDYAxisTail)
      XTitle = "PSD tail integral [ADC]";
    else
      XTitle = "PSD parameter (tail/total)";
  }
  else if(ADAQSettings->PSDYSlice){
    double YPosInADC = gPad->AbsPixeltoY(YPixel);
    
    ss << "PSDHistogram Y slice at " << YPosInADC << " ADC";
    Title = ss.str();
    
    if(ADAQSettings->PSDXAxisEnergy){
      if(ADAQSettings->EnergyUnit == 0)
	XTitle = "PSD total integral [keVee]";
      else
	XTitle = "PSD total integral [MeVee]";
    }
    else
      XTitle = "PSD total integral [ADC]";
  }
  
  /////////////////////////////////
  // Set slice histogram attributes

  // Ensure that the PSD slice canvas is now active
  PSDSlice_C->cd();
    
  PSDHistogramSlice_H->SetName(SliceHistogramName.c_str());
  PSDHistogramSlice_H->SetTitle(Title.c_str());

  PSDHistogramSlice_H->SetStats(false);

  PSDHistogramSlice_H->GetXaxis()->SetTitle(XTitle.c_str());
  PSDHistogramSlice_H->GetXaxis()->SetTitleSize(0.06);
  PSDHistogramSlice_H->GetXaxis()->SetTitleOffset(1.1);
  PSDHistogramSlice_H->GetXaxis()->SetLabelSize(0.06);
  PSDHistogramSlice_H->GetXaxis()->SetNdivisions(505);
  PSDHistogramSlice_H->GetXaxis()->CenterTitle();
  
  PSDHistogramSlice_H->GetYaxis()->SetTitle("Counts");
  PSDHistogramSlice_H->GetYaxis()->SetTitleSize(0.06);
  PSDHistogramSlice_H->GetYaxis()->SetTitleOffset(1.1);
  PSDHistogramSlice_H->GetYaxis()->SetLabelSize(0.06);
  PSDHistogramSlice_H->GetYaxis()->SetNdivisions(505);
  PSDHistogramSlice_H->GetYaxis()->CenterTitle();
  
  PSDHistogramSlice_H->SetLineColor(kBlue);
  PSDHistogramSlice_H->SetLineWidth(2);
  PSDHistogramSlice_H->Draw("");
  
  // Because the PSD histogram slice only exists temporarily within
  // this method, we need to perform the fitting here (rather than in
  // the AAComputation class) and store the results within the
  // AAGraphics class for later access
  
  if(ADAQSettings->PSDCalculateFOM){

    // Fit the lower gaussian (typically gamma / electron group)

    TF1 *LowerFOMFit = new TF1("LowerFOMFit",
			       "gaus",
			       ADAQSettings->PSDLowerFOMFitMin,
			       ADAQSettings->PSDLowerFOMFitMax);
    LowerFOMFit->SetLineColor(kGreen+2);
    LowerFOMFit->SetLineWidth(2);

    PSDHistogramSlice_H->Fit("LowerFOMFit", "RNQ");

    LowerFOMFit->Draw("SAME");
    
    // Fit the upper gaussian (typically neutron / proton group)
    
    TF1 *UpperFOMFit = new TF1("UpperFOMFit",
			       "gaus",
			       ADAQSettings->PSDUpperFOMFitMin,
			       ADAQSettings->PSDUpperFOMFitMax);
    UpperFOMFit->SetLineColor(kRed);
    UpperFOMFit->SetLineWidth(2);

    PSDHistogramSlice_H->Fit("UpperFOMFit", "RNQ");

    UpperFOMFit->Draw("SAME");

    // Calculate the PSD figure-of-merit (FOM)

    Double_t LowerMean = LowerFOMFit->GetParameter(1);
    Double_t LowerFWHM = LowerFOMFit->GetParameter(2) * 2.35;
    
    Double_t UpperMean = UpperFOMFit->GetParameter(1);
    Double_t UpperFWHM = UpperFOMFit->GetParameter(2) * 2.35;

    PSDFigureOfMerit = (UpperMean - LowerMean) / (UpperFWHM + LowerFWHM);
  }
  
  // Update the standalone canvas
  PSDSlice_C->Update();

  gPad->GetCanvas()->FeedbackMode(kFALSE);
  
  // Reset the main embedded canvas to active
  TheCanvas->cd();
}


void AAGraphics::SetWaveformColor()
{
  ColorToSet = zWaveformColor;
  CreateColorWheel();
}


void AAGraphics::SetSpectrumLineColor()
{
  ColorToSet = zSpectrumLineColor;
  CreateColorWheel();
}


void AAGraphics::SetSpectrumFillColor()
{
  ColorToSet = zSpectrumFillColor;
  CreateColorWheel();
}


void AAGraphics::CreateColorWheel()
{
  // Create and draw the standard ROOT color wheel

  ColorWheel = new TColorWheel;
  ColorWheel->Draw();

  // In order to allow the user to select colors via the mouse, get
  // the canvas and connect it the AAGraphics::PickColor() slot

  TCanvas *ColorWheelCanvas = (TCanvas *)ColorWheel->GetCanvas();
  ColorWheelCanvas->Connect("ProcessedEvent(int, int, int, TObject *)", "AAGraphics", this, "PickColor(int, int, int, TObject *)");

  // Obtain a pointer to the default window (a TRootCanvas class,
  // which is derived from TGWindow) containing the color wheel. This
  // is not exactly intuitive, but the method works by iterating over
  // the list of windows to find the only TRootCanvas (which contains
  // the TColorWheel object) in the list.

  TIter next(gClient->GetListOfWindows());
  TObject *Object = NULL;
  TRootCanvas *Window = NULL;

  while( (Object = next()) ){
    if(Object->InheritsFrom(TRootCanvas::Class()))
      Window = (TRootCanvas *)gClient->GetListOfWindows()->FindObject(Object);
  }

  gClient->WaitFor(Window);
}


void AAGraphics::PickColor(int EventID, int XPixel, int YPixel, TObject *Selected)
{
  int PickedColor = 0;
  if(EventID == 1){
    PickedColor = ColorWheel->GetColor(XPixel, YPixel);
    TCanvas *ColorWheelCanvas = (TCanvas *)gROOT->FindObject("wheel");
    
    if(PickedColor > 0){

      switch(ColorToSet){

      case zWaveformColor:
	WaveformColor = PickedColor;
	break;

      case zSpectrumLineColor:
	SpectrumLineColor = PickedColor;
	break;

      case zSpectrumFillColor:
	SpectrumFillColor = PickedColor;
	break;

      default:
	break;
      }
      ColorWheelCanvas->Close();
    }
  }
}


void AAGraphics::ClosePSDSliceWindow()
{
  if(ADAQSettings->EnableHistogramSlicing)
    TheInterface->UpdateForPSDHistogramSlicingFinished();
}
