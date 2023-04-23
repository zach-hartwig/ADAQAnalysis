/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                           Copyright (C) 2012-2023                           //
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
// name: AASpectrumSlots.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AASpectrumSlots class contains widget slot methods to
//       handle signals generated from widgets contained on the
//       "spectrum" tab of the ADAQAnalysis GUI
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef __AASpectrumSlots_hh__
#define __AASpectrumSlots_hh__ 1

#include <TObject.h>

#include "AAComputation.hh"
#include "AAGraphics.hh"

class AAInterface;

class AASpectrumSlots : public TObject
{
public:
  AASpectrumSlots(AAInterface *);
  ~AASpectrumSlots();

  void HandleCheckButtons();
  void HandleComboBoxes(int, int);
  void HandleNumberEntries();
  void HandleRadioButtons();
  void HandleTextButtons();

  ClassDef(AASpectrumSlots, 0);

private:
  AAInterface *TheInterface;

  AAComputation *ComputationMgr;
  AAGraphics *GraphicsMgr;
};

#endif
