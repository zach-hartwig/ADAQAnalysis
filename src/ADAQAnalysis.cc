//////////////////////////////////////////////////////////////////////////////////////////
// 
// name: ADAQAnalysis.cc
// date: 17 May 13
// auth: Zach Hartwig
// 
// desc:
//
//////////////////////////////////////////////////////////////////////////////////////////

#include <TApplication.h>

#include <iostream>
#include <string>

#ifdef MPI_ENABLED
#include <mpi.h>
#endif

#include "ADAQAnalysisInterface.hh"
#include "ADAQAnalysisManager.hh"
#include "ADAQGraphicsManager.hh"

int main(int argc, char *argv[])
{
  // Initialize the MPI session
#ifdef MPI_ENABLED
  MPI::Init_thread(argc, argv, MPI::THREAD_SERIALIZED);
#endif
  
  // Assign the binary architecture (sequential or parallel) to a bool
  // that will be used frequently in the code to determine arch. type
  bool ParallelArchitecture = false;
#ifdef MPI_ENABLED
  ParallelArchitecture = true;
#endif
  
  // A word on the use of the first cmd line arg: the first cmd line
  // arg is used in different ways depending on the binary
  // architecture. For sequential arch, the user may specify a valid
  // ADAQ ROOT file as the first cmd line arg to open automatically
  // upon launching the ADAQAnalysis binary. For parallel arch, the
  // automatic call to the parallel binaries made by ADAQAnalysisGUI
  // contains the type of processing to be performed in parallel as
  // the first cmd line arg, which, at present, is "histogramming" for
  // spectra creation and "desplicing" for despliced file creation.
  
  // Get the zeroth command line arg (the binary name)
  string CmdLineBin = argv[0];
  
  // Get the first command line argument 
  string CmdLineArg;

  // If no first cmd line arg was specified then we will start the
  // analysis with no ADAQ ROOT file loaded via "unspecified" string
  if(argc==1)
    CmdLineArg = "unspecified";
  
  // If only 1 cmd line arg was specified then assign it to
  // corresponding string that will be passed to the
  // ADAQAnalysisManager class constructor for use
  else if(argc==2)
    CmdLineArg = argv[1];

  // Disallow any other combination of cmd line args and notify the
  // user about his/her mistake appropriately (arch. specific)
  else{
    if(CmdLineBin == "ADAQAnalysis"){
      cout << "\nError! Unspecified command line arguments to ADAQAnalysis!\n"
	   <<   "       Usage: ADAQAnalysis </path/to/filename>\n"
	   << endl;
      exit(-42);
    }
    else{
      cout << "\nError! Unspecified command line arguments to ADAQAnalysis_MPI!\n"
	   <<   "       Usage: ADAQAnalysis <WaveformAnalysisType: {histogramming, desplicing, discriminating}\n"
	   << endl;
      exit(-42);
    }
  }    
  
  // Run ROOT in standalone mode
  TApplication *TheApplication = new TApplication("ADAQAnalysis", &argc, argv);
  

  // Create the singleton analysis manager
  ADAQAnalysisManager *TheAnalysis = new ADAQAnalysisManager(ParallelArchitecture);
  
  if(!ParallelArchitecture){
    // Create the graphics plotter
    ADAQGraphicsManager *TheGraphics = new ADAQGraphicsManager;

    // Create the graphical user interface
    ADAQAnalysisInterface *TheInterface = new ADAQAnalysisInterface;
    TheInterface->Connect("CloseWindow()", "ADAQAnalysisInterface", TheInterface, "HandleTerminate()");
  
    // Run the application
    TheApplication->Run();
    
    delete TheGraphics;
    delete TheInterface;
  }

  delete TheAnalysis;

  delete TheApplication;
  
  // Finalize the MPI session
#ifdef MPI_ENABLED
  MPI::Finalize();
#endif
  
  return 0;
}
