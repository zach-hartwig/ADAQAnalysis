#ifndef __AAGraphicsSlots_hh__
#define __AAGraphicsSlots_hh__ 1

#include <TObject.h>

class AAInterface;

class AAGraphicsSlots : public TObject
{
public:
  AAGraphicsSlots(AAInterface *);
  ~AAGraphicsSlots();

  void HandleCheckButtons();
  void HandleComboBoxes();
  void HandleNumberEntries();
  void HandleRadioButtons();

  ClassDef(AAGraphicsSlots, 0);

private:
  AAInterface *TheInterface;
};

#endif
