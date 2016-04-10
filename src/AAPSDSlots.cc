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
// name: AAPSDSlots.cc
// date: 10 Apr 16
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAPSDSlots class contains widget slot methods to handle
//       signals generated from widgets contained on the "PSD" tab of
//       the ADAQAnalysis GUI
//
/////////////////////////////////////////////////////////////////////////////////

// ROOT
#include <TGFileDialog.h>
#include <TList.h>

// C++
#include <sstream>

// ADAQAnalysis
#include "AAPSDSlots.hh"
#include "AANontabSlots.hh"
#include "AAInterface.hh"
#include "AAGraphics.hh"


AAPSDSlots::AAPSDSlots(AAInterface *TI)
  : TheInterface(TI)
{
  ComputationMgr = AAComputation::GetInstance();
  GraphicsMgr = AAGraphics::GetInstance();
}


AAPSDSlots::~AAPSDSlots()
{;}


void AAPSDSlots::HandleCheckButtons()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGCheckButton *CheckButton = (TGCheckButton *) gTQSender;
  int CheckButtonID = CheckButton->WidgetId();

  TheInterface->SaveSettings();
  
  switch(CheckButtonID){
  }
}


void AAPSDSlots::HandleComboBoxes(int ComboBoxID, int SelectedID)
{
  if(!TheInterface->EnableInterface)
    return;

  TheInterface->SaveSettings();

  switch(ComboBoxID){
  }
}


void AAPSDSlots::HandleNumberEntries()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGNumberEntry *NumberEntry = (TGNumberEntry *) gTQSender;
  int NumberEntryID = NumberEntry->WidgetId();
  
  TheInterface->SaveSettings();

  switch(NumberEntryID){
  }
}


void AAPSDSlots::HandleRadioButtons()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGRadioButton *RadioButton = (TGRadioButton *) gTQSender;
  int RadioButtonID = RadioButton->WidgetId();
  
  TheInterface->SaveSettings();

  switch(RadioButtonID){
  }
}


void AAPSDSlots::HandleTextButtons()
{
  if(!TheInterface->EnableInterface)
    return;
  
  TGTextButton *TextButton = (TGTextButton *) gTQSender;
  int TextButtonID  = TextButton->WidgetId();
  
  TheInterface->SaveSettings();
  
  switch(TextButtonID){
  }
}
