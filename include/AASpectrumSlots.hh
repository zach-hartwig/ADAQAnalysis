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
