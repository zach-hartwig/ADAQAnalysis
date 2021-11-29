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
// name: AANontabSlots.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AANontabSlots class contains widget slot methods to
//       handle signals generated from widgets that are not contained
//       on one of the five tabs. This includes the file
//       menu, the sliders, and the canvas amongst others.
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef __AANontabSlots_hh__
#define __AANontabSlots_hh__ 1

#include <TObject.h>

#include "AAComputation.hh"
#include "AAGraphics.hh"
#include "AAInterpolation.hh"

class AAInterface;

class AANontabSlots : public TObject
{
public:
  AANontabSlots(AAInterface *);
  ~AANontabSlots();

  // "Slot" methods to recieve and act upon ROOT widget "signals"
  void HandleCanvas(int, int, int, TObject *);
  void HandleDoubleSliders();
  void HandleMenu(int);
  void HandleSliders(int);
  void HandleTerminate();
  void HandleTripleSliderPointer();

  ClassDef(AANontabSlots, 0);

private:
  AAInterface *TheInterface;

  AAComputation *ComputationMgr;
  AAGraphics *GraphicsMgr;
  AAInterpolation *InterpolationMgr;
};

#endif
