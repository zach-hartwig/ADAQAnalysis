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
// name: AAParallel.cc
// date: 11 Aug 14
// auth: Zach Hartwig
// mail: hartwig@psfc.mit.edu
// 
// desc: The AAParallel class handles all parallel Open MPI processing
//       functionality. This includes initializing and finalizing MPI
//       sessions, set/get methods for important MPI variables like
//       processing number and node rank, and assignment of the MPI
//       binary name for execution from the sequential GUI.
//
/////////////////////////////////////////////////////////////////////////////////

// MPI
#ifdef MPI_ENABLED
#include <mpi.h>
#endif

// C++
#include <iostream>
#include <cstdlib>
using namespace std;

// ADAQAnalysis
#include "AAParallel.hh"
#include "AAVersion.hh"


AAParallel *AAParallel::TheParallelManager = 0;


AAParallel *AAParallel::GetInstance()
{ return TheParallelManager; }


AAParallel::AAParallel()
  : MPI_Rank(0), MPI_Size(1), IsMaster(true), IsSlave(false)
{
  if(TheParallelManager){
    cout << "\nERROR! TheParallelManager was constructed twice!\n" << endl;
    exit(-42);
  }
  TheParallelManager = this;
  
  if(getenv("ADAQANALYSIS_HOME")==NULL){
    cout << "\nError! The 'ADAQANALYSIS_HOME' environmental variable must be set to properly configure\n"
	 <<   "       ADAQAnalysis! Please use the provided ADAQ Setup.sh script.\n"
	 << endl;
    exit(-42);
  }
  
  // Get the absolute path to the ADAQAnalysis installation
  string ADAQANALYSIS_HOME = getenv("ADAQANALYSIS_HOME");

  // Set the absolute path to the parallel ADAQAnalysis binary
  ParallelBinaryName = ADAQANALYSIS_HOME + "/bin/ADAQAnalysis_MPI";
  
  // Set the location of the transient parallel processing file. Use
  // the linux user name to ensure a unique file name is created.
  string USER = getenv("USER");
  ParallelFileName = "/tmp/ADAQParallelProcessing_" + USER + ".root";
}


AAParallel::~AAParallel()
{;}


void AAParallel::Initialize(int argc, char *argv[])
{
#ifdef MPI_ENABLED
  MPI::Init_thread(argc, argv, MPI::THREAD_SERIALIZED);


  MPI_Rank = MPI::COMM_WORLD.Get_rank();
  MPI_Size = MPI::COMM_WORLD.Get_size();

  IsMaster = (MPI_Rank == 0);
  IsSlave = (MPI_Rank != 0);
#endif
}


void AAParallel::Finalize()
{
#ifdef MPI_ENABLED
  MPI::Finalize();
#endif
}


// Method used to aggregate arrays of doubles on each node to a single
// array on the MPI master node (master == node 0). A pointer to the
// array on each node (*SlaveArray) as well as the size of the array
// (ArraySize) must be passed as function arguments from each node.
double *AAParallel::SumDoubleArrayToMaster(double *SlaveArray, size_t ArraySize)
{
  double *MasterSum = new double[ArraySize];
#ifdef MPI_ENABLED
  MPI::COMM_WORLD.Reduce(SlaveArray, MasterSum, ArraySize, MPI::DOUBLE, MPI::SUM, 0);
#endif
  return MasterSum;
}


// Method used to aggregate doubles on each node to a single double on
// the MPI master node (master == node 0).
double AAParallel::SumDoublesToMaster(double SlaveDouble)
{
  double MasterSum = 0;
#ifdef MPI_ENABLED
  MPI::COMM_WORLD.Reduce(&SlaveDouble, &MasterSum, 1, MPI::DOUBLE, MPI::SUM, 0);
#endif
  return MasterSum;
}
