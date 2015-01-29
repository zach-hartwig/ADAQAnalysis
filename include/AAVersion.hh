/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                           Copyright (C) 2012-2015                           //
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
// name: AAVersion.hh
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: AAVersion is a header file that contains a single global
//       string describing the version of ADAQAnalysis as a
//       development version or as the production version number. The
//       string is utilized by AAInterface to create the appropriate
//       window titles for to inform the user on what version he/she
//       is using. Whening using the install.sh script to install a
//       production version, The header file is modified with the
//       version number at build/install time.
//       
/////////////////////////////////////////////////////////////////////////////////

#ifndef __AAVersion_hh__
#define __AAVersion_hh__ 1

const string VersionString = "1.0.0-beta";

#endif
