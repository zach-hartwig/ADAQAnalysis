#ifndef __AASpectrumSlots_hh__
#define __AASpectrumSlots_hh__ 1

#include <TObject.h>

class AAInterface;

class AASpectrumSlots : public TObject
{
public:
  AASpectrumSlots(AAInterface *);
  ~AASpectrumSlots();

  void HandleCheckButtons();
  void HandleComboBoxes();
  void HandleNumberEntries();
  void HandleRadioButtons();

  ClassDef(AASpectrumSlots, 0);

private:
  AAInterface *TheInterface;
};

#endif
