//////////////////////////////////////////////////////////////////////////////////////////
//
// name: ADAQAnalysisInterface.hh
// date: 17 May 13
// auth: Zach Hartwig
//
// desc: 
//
//////////////////////////////////////////////////////////////////////////////////////////

#ifndef __ADAQAnalysisManager_hh__
#define __ADAQAnalysisManager_hh__ 1

#include <TObject.h>

class ADAQAnalysisManager : public TObject
{
public:
  ADAQAnalysisManager();
  ~ADAQAnalysisManager();

private:

  // Define the class to ROOT
  ClassDef(ADAQAnalysisManager, 1)
};

#endif
