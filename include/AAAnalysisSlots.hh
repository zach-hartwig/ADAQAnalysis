#ifndef __AAAnalysisSlots_hh__
#define __AAAnalysisSlots_hh__ 1

#include <TObject.h>

class AAInterface;

class AAAnalysisSlots : public TObject
{
public:
  AAAnalysisSlots(AAInterface *);
  ~AAAnalysisSlots();

  void HandleCheckButtons();
  void HandleComboBoxes();
  void HandleNumberEntries();
  void HandleRadioButtons();

  ClassDef(AAAnalysisSlots, 0);

private:
  AAInterface *TheInterface;
};

#endif
