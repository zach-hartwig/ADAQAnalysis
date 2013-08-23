#include "AAParallel.hh"

#ifdef MPI_ENABLED
#include <mpi.h>
#endif

#include <iostream>
#include <cstdlib>
using namespace std;

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

  // ADAQAnalysis is configured separately for two types of users;
  // users and developers. The user type is automatically set by the
  // ADAQ Setup.sh configuration script.
  
  if(getenv("ADAQUSER")==NULL){
    cout << "\nError! The 'ADAQUSER' environmental variable must be set to properly configure\n"
	 <<   "       ADAQAnalysis! Please use the provided ADAQ Setup.sh script.\n"
	 << endl;
    exit(-42);
  }

  // Get ADAQ user type
  string ADAQUSER = getenv("ADAQUSER");

  // Get the linux user handle
  string USER = getenv("USER");

  
  // ADAQ users run the installed binaries from /usr/local/adaq
  if(ADAQUSER == "user")
    ParallelBinaryName = "/usr/local/adaq/ADAQAnalysis_MPI";

  // ADAQ developers require settings relative to their top-level SVN
  // checkout directory of the source code
  else if(ADAQUSER == "developer"){
    if(getenv("ADAQHOME")==NULL){
      cout << "\nError! The 'ADAQHOME' environmental variable must be set to point to the top-level\n"
	   <<   "       ADAQ SVN checkout directory! Please check your environment settings.\n"
	   << endl;
      exit(-42);
    }

    string ADAQHOME = getenv("ADAQHOME");
    
    if(VersionString == "Development")
      ParallelBinaryName = ADAQHOME + "/analysis/ADAQAnalysis/trunk/bin/ADAQAnalysis_MPI";
    else
      ParallelBinaryName = ADAQHOME + "/analysis/ADAQAnalysis/versions/" + VersionString + "/bin/ADAQAnalysis_MPI";
  }
 
  // Set the location of the transient parallel processing file
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
