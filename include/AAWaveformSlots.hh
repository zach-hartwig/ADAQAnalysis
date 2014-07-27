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
