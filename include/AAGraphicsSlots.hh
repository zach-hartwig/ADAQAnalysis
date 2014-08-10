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
