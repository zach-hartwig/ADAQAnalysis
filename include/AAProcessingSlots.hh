/////////////////////////////////////////////////////////////////////////////////
// 
// name: AAProcessing.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAProcessingSlots class contains widget slot methods to
//       handle signals generated from widgets contained on the
//       "processing" tab of the ADAQAnalysis GUI
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef __AAProcessingSlots_hh__
#define __AAProcessingSlots_hh__ 1

#include <TObject.h>

#include "AAComputation.hh"
#include "AAGraphics.hh"
#include "AAInterpolation.hh"

class AAInterface;

class AAProcessingSlots : public TObject
{
public:
  AAProcessingSlots(AAInterface *);
  ~AAProcessingSlots();

  void HandleCheckButtons();
  void HandleComboBoxes(int, int);
  void HandleNumberEntries();
  void HandleRadioButtons();
  void HandleTextButtons();

  ClassDef(AAProcessingSlots, 0);

private:
  AAInterface *TheInterface;

  AAComputation *ComputationMgr;
  AAGraphics *GraphicsMgr;
  AAInterpolation *InterpolationMgr;
};

#endif
