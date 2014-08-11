/////////////////////////////////////////////////////////////////////////////////
// 
// name: AASpectrumSlots.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AASpectrumSlots class contains widget slot methods to
//       handle signals generated from widgets contained on the
//       "spectrum" tab of the ADAQAnalysis GUI
//
/////////////////////////////////////////////////////////////////////////////////

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
