/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                           Copyright (C) 2012-2016                           //
//                 Zachary Seth Hartwig : All rights reserved                  //
//                                                                             //
//      The ADAQAnalysis source code is licensed under the GNU GPL v3.0.       //
//      You have the right to modify and/or redistribute this source code      //      
//      under the terms specified in the license, which may be found online    //
//      at http://www.gnu.org/licenses or at $ADAQANALYSIS/License.txt.        //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// 
// name: AAParallelResults.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAParallelResults class stores values computed during
//       parallel processing that are required for analysis or GUI
//       settings by the sequential ADAQAnalysis GUI and binary. The
//       provides a method of "persistency" for values computed in
//       parallel processing.
//
/////////////////////////////////////////////////////////////////////////////////


#ifndef __AAParallelResults_hh__
#define __AAParallelResults_hh__  1

#include <TObject.h>

class AAParallelResults : public TObject
{
public:
  double DeuteronsInTotal;
  
  ClassDef(AAParallelResults, 1);
};


#endif
