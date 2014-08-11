/////////////////////////////////////////////////////////////////////////////////
// 
// name: AAAnalysisSlots.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAAnalysisSlots class contains widget slot methods to
//       handle signals generated from widgets contained on the
//       "analysis" tab of the ADAQAnalysis GUI
//
/////////////////////////////////////////////////////////////////////////////////

#ifndef __AAAnalysisSlots_hh__
#define __AAAnalysisSlots_hh__ 1

#include <TObject.h>

#include "AAComputation.hh"
#include "AAGraphics.hh"
#include "AAInterpolation.hh"

class AAInterface;

class AAAnalysisSlots : public TObject
{
public:
  AAAnalysisSlots(AAInterface *);
  ~AAAnalysisSlots();

  void HandleCheckButtons();
  void HandleComboBoxes(int, int);
  void HandleNumberEntries();
  void HandleRadioButtons();

  ClassDef(AAAnalysisSlots, 0);

private:
  AAInterface *TheInterface;

  AAComputation *ComputationMgr;
  AAGraphics *GraphicsMgr;
  AAInterpolation *InterpolationMgr;
};

#endif
