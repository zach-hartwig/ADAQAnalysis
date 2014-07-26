#ifndef __AAWaveformSlots_hh__
#define __AAWaveformSlots_hh__ 1

#include <TObject.h>

class AAInterface;

class AAWaveformSlots : public TObject
{
public:
  AAWaveformSlots(AAInterface *);
  ~AAWaveformSlots();

  void HandleCheckButtons();
  void HandleComboBoxes();
  void HandleNumberEntries();
  void HandleRadioButtons();

  ClassDef(AAWaveformSlots, 0);

private:
  AAInterface *TheInterface;
};

#endif
