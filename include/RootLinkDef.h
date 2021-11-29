/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                           Copyright (C) 2012-2021                           //
//                 Zachary Seth Hartwig : All rights reserved                  //
//                                                                             //
//      The ADAQAnalysis source code is licensed under the GNU GPL v3.0.       //
//      You have the right to modify and/or redistribute this source code      //      
//      under the terms specified in the license, which may be found online    //
//      at http://www.gnu.org/licenses or at $ADAQANALYSIS/License.txt.       //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// 
// name: RootLinkDef.h
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: RootLinkDef is the mandatory header file required by rootcint
//       to generate dictionaries for custom C++ classes used in
//       ADAQAnalysis. Note that it's suffix is ".h" rather than ".hh"
//       such that the Makefile does not include it when it wildcards
//       for the other header files; this is done since RootLinkDef.h
//       must be the final header file included to rootcint.
//
/////////////////////////////////////////////////////////////////////////////////

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// ADAQAnalysis classes
#pragma link C++ class AAInterface+;
#pragma link C++ class AAWaveformSlots+;
#pragma link C++ class AASpectrumSlots+;
#pragma link C++ class AAAnalysisSlots+;
#pragma link C++ class AAPSDSlots+;
#pragma link C++ class AAGraphicsSlots+;
#pragma link C++ class AAProcessingSlots+;
#pragma link C++ class AANontabSlots+;
#pragma link C++ class AAComputation+;
#pragma link C++ class AAGraphics+;
#pragma link C++ class AAParallel+;
#pragma link C++ class AASettings+;
#pragma link C++ class AAParallelResults+;

// General ADAQ classes
#pragma link C++ class ADAQRootMeasParams+;

// ACRONYM classes
#pragma link C++ class acroEvent+;
#pragma link C++ class acroRun+;

#endif
