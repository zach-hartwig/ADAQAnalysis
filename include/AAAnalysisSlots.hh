#ifndef __AAAnalysisSlots_hh__
#define __AAAnalysisSlots_hh__ 1

#include <TObject.h>

#include "AAComputation.hh"
#include "AAGraphics.hh"
#include "AAInterpolation.hh"

class AAInterface;

class AAAnalysisSlots : public TObject
{
public:
  AAAnalysisSlots(AAInterface *);
  ~AAAnalysisSlots();

  void HandleCheckButtons();
  void HandleComboBoxes(int, int);
  void HandleNumberEntries();
  void HandleRadioButtons();

  ClassDef(AAAnalysisSlots, 0);

private:
  AAInterface *TheInterface;

  AAComputation *ComputationMgr;
  AAGraphics *GraphicsMgr;
  AAInterpolation *InterpolationMgr;
};

#endif
