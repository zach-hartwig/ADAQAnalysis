#ifndef __AANontabSlots_hh__
#define __AANontabSlots_hh__ 1

#include <TObject.h>

class AAInterface;

class AANontabSlots : public TObject
{
public:
  AANontabSlots(AAInterface *);
  ~AANontabSlots();

  void HandleCheckButtons();
  void HandleComboBoxes();
  void HandleNumberEntries();
  void HandleRadioButtons();

  ClassDef(AANontabSlots, 0);

private:
  AAInterface *TheInterface;
};

#endif
