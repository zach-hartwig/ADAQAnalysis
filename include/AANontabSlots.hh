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
