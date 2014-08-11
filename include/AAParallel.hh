/////////////////////////////////////////////////////////////////////////////////
// 
// name: AAParallel.hh
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

#ifndef __AAParallel_hh__
#define __AAParallel_hh__ 1

#include <TObject.h>

#include <string>
using namespace std;

class AAParallel : public TObject
{
public:
  AAParallel();
  ~AAParallel();

  static AAParallel *GetInstance();

  void Initialize(int, char **);
  void Finalize();

  double *SumDoubleArrayToMaster(double *, size_t);
  double SumDoublesToMaster(double);

  int GetRank() {return MPI_Rank;}
  int GetSize() {return MPI_Size;}

  bool GetIsMaster() {return IsMaster;}
  bool GetIsSlave() {return IsSlave;}

  string GetParallelBinaryName() {return ParallelBinaryName;}
  string GetParallelFileName() {return ParallelFileName;}


private:
  static AAParallel *TheParallelManager;

  int MPI_Rank, MPI_Size;
  bool IsMaster, IsSlave;

  string ParallelBinaryName, ParallelFileName;

  ClassDef(AAParallel, 1)
};

#endif
