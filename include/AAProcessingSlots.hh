#ifndef __AAProcessingSlots_hh__
#define __AAProcessingSlots_hh__ 1

#include <TObject.h>

class AAInterface;

class AAProcessingSlots : public TObject
{
public:
  AAProcessingSlots(AAInterface *);
  ~AAProcessingSlots();

  void HandleCheckButtons();
  void HandleComboBoxes();
  void HandleNumberEntries();
  void HandleRadioButtons();

  ClassDef(AAProcessingSlots, 0);

private:
  AAInterface *TheInterface;
};

#endif
