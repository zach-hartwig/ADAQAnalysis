#include "AAParallel.hh"

#ifdef MPI_ENABLED
#include <mpi.h>
#endif

#include <iostream>
#include <cstdlib>
using namespace std;

#include "ADAQAnalysisVersion.hh"

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


  string ADAQHOME = getenv("ADAQHOME");
  string USER = getenv("USER");

  if(VersionString == "Development")
    ParallelBinaryName = ADAQHOME + "/analysis/ADAQAnalysisGUI/trunk/bin/ADAQAnalysis_MPI";
  else
    ParallelBinaryName = ADAQHOME + "/analysis/ADAQAnalysisGUI/versions/" + VersionString + "/bin/ADAQAnalysis_MPI";

  
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
