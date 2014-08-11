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
