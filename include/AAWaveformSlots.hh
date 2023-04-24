/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                            Copyright (C) 2012-2023                          //
//                  Zachary Seth Hartwig : All rights reserved                 //
//                                                                             //
//      The ADAQAnalysis source code is licensed under the GNU GPL v3.0.       //
//      You have the right to modify and/or redistribute this source code      //
//      under the terms specified in the license, which is found online        //
//      at http://www.gnu.org/licenses or at $ADAQANALYSIS/License.txt.        //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// 
// name: AAWaveformSlots.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAWaveformSlots class contains widget slot methods to
//       handle signals generated from widgets contained on the
//       "waveform" tab of the ADAQAnalysis GUI
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef __AAWaveformSlots_hh__
#define __AAWaveformSlots_hh__ 1

#include <TObject.h>

#include "AAComputation.hh"
#include "AAGraphics.hh"

class AAInterface;

class AAWaveformSlots : public TObject
{
public:
  AAWaveformSlots(AAInterface *);
  ~AAWaveformSlots();

  void HandleCheckButtons();
  void HandleComboBoxes(int, int);
  void HandleNumberEntries();
  void HandleRadioButtons();

  ClassDef(AAWaveformSlots, 0);

private:
  AAInterface *TheInterface;

  AAComputation *ComputationMgr;
  AAGraphics *GraphicsMgr;
};

#endif
