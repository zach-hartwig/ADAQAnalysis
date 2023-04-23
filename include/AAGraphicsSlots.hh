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
// name: AAGraphicsSlots.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAGraphicsSlots class contains widget slot methods to
//       handle signals generated from widgets contained on the
//       "graphics" tab of the ADAQAnalysis GUI
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef __AAGraphicsSlots_hh__
#define __AAGraphicsSlots_hh__ 1

#include <TObject.h>

#include "AAComputation.hh"
#include "AAGraphics.hh"
#include "AAInterpolation.hh"

class AAInterface;

class AAGraphicsSlots : public TObject
{
public:
  AAGraphicsSlots(AAInterface *);
  ~AAGraphicsSlots();

  void HandleCheckButtons();
  void HandleNumberEntries();
  void HandleRadioButtons();
  void HandleTextButtons(); 

  ClassDef(AAGraphicsSlots, 0);

private:
  AAInterface *TheInterface;

  AAComputation *ComputationMgr;
  AAGraphics *GraphicsMgr;
  AAInterpolation *InterpolationMgr;
};

#endif
