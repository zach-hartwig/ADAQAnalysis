#********************************************************************
#  name: Makefile                  
#  date: 02 Feb 14
#  auth: Zach Hartwig              
#
#  desc: GNUmakefile for building ADAQAnalysis code in seqential and
#        parallel versions (options for building are provided
#        below). The intermediary build files are placed in build/;
#        the final binaries are placed in bin/.
#
#  dpnd: The build system depends on the following:
#  -- ROOT (mandatory)
#  -- ADAQ libraries (mandatory)
#  -- Boost, including Boost thread libraries (mandatory)
#  -- Open MPI (optional; code will compiles without)
# 
#  To build the sequential binary
#  $ make 
#
#  To build parallel binary (requires Open MPI on system)
#  $ make par
#
#  To build both sequential and parallel binaries
#  $ make both
#
#  To clean the bin/ and build/ directories
#  # make clean
#
#********************************************************************

#***************************#
#**** MACRO DEFINITIONS ****#
#***************************#

# Include the Makefile for ROOT-based projects
RC:=root-config
ROOTSYS:=$(shell $(RC) --prefix)
ROOTMAKE:=$(ROOTSYS)/etc/Makefile.arch
ROOTLIB = -lSpectrum
include $(ROOTMAKE)

# Get the execution directory of the makefile
EXECDIR = $(PWD)

# Specify the locatino of the build directory
BUILDDIR = $(EXECDIR)/build

# Specify the locatino of the binary directory
BINDIR = $(EXECDIR)/bin

# Specify the location of the source and header files
SRCDIR = $(EXECDIR)/src
INCLDIR = $(EXECDIR)/include

# Specify all object files (to be built in the build/ directory)
SRCFILES = $(wildcard $(SRCDIR)/*.cc)
TMP = $(patsubst %.cc,%.o,$(SRCFILES))
OBJS = $(subst /src/,/build/,$(TMP))

# Specify the includes for the ROOT dictionary build
CXXFLAGS += -I$(ADAQHOME)/include -I$(INCLDIR)

# Specify the required Boost libraries. Note that the sws/cmodws
# cluster recently upgraded GCC, which breaks the ancient fucking
# version of Boost (1.42.0 from Feb 2010!) that is in the RHEL
# repository. I have, therefore, built the most recent Boost (1.53.0)
# that must be used if compiling on sws/cmodws cluster

DOMAIN=$(findstring .psfc.mit.edu, $(HOSTNAME))

ifeq ($(DOMAIN),.psfc.mit.edu)
   BOOSTLIBS = -L/home/hartwig/scratch/boost/boost_1_53_0/lib -lboost_thread -lboost_system
   CXXFLAGS += -I/home/hartwig/scratch/boost/boost_1_53_0/include
else
   BOOSTLIBS = -lboost_thread
endif

# If the user desires to build a parallel version of the binary then
# set a number of key macros for the build. Note that the Open MPI C++
# compiler is specifed (rather than gcc) and the MPI_ENABLED macro
# will be passed to the compiler. The MPI_ENABLED macro is used as a
# preprocessing directive in the source code to protect MPI code in
# case the user would like to build on a system without Open MPI
ifeq ($(ARCH),mpi)
   PAR_TARGET = $(BINDIR)/ADAQAnalysis_MPI
   CXX := mpic++
   CXXFLAGS += -DMPI_ENABLED -I.
   MPI_ENABLED := 1
   TMP := $(OBJS)
   OBJS = $(patsubst %.o, %_MPI.o, $(TMP))

# If the user desires to build the sequential version of the binary
# then set the macros requires for the sequential build
else	
   SEQ_TARGET = $(BINDIR)/ADAQAnalysis
   CXX := clang++ -ferror-limit=5 -w
endif


#***************#
#**** RULES ****#
#***************#

$(SEQ_TARGET) : $(OBJS) $(BUILDDIR)/ADAQAnalysisDict.o
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(ROOTGLIBS) $(ROOTLIB) $(BOOSTLIBS)
	@echo -e "\n$@ build is complete!\n"

$(PAR_TARGET) : $(OBJS) $(BUILDDIR)/ADAQAnalysisDict.o
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(ROOTGLIBS) $(ROOTLIB) $(BOOSTLIBS)
	@echo -e "\n$@ build is complete!\n"

#********************************************#
# Rules to build the sequential object files

$(BUILDDIR)/ADAQAnalysis.o : $(SRCDIR)/ADAQAnalysis.cc 
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/AAInterface.o : $(SRCDIR)/AAInterface.cc $(INCLDIR)/AAInterface.hh
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/AAComputation.o : $(SRCDIR)/AAComputation.cc $(INCLDIR)/AAComputation.hh
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/AAGraphics.o : $(SRCDIR)/AAGraphics.cc $(INCLDIR)/AAGraphics.hh
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/AAParallel.o : $(SRCDIR)/AAParallel.cc $(INCLDIR)/AAParallel.hh
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/AAInterpolation.o : $(SRCDIR)/AAInterpolation.cc $(INCLDIR)/AAInterpolation.hh $(INCLDIR)/AAInterpolationData.hh
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/ADAQAnalysisDict.o : $(BUILDDIR)/ADAQAnalysisDict.cc
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<


#******************************************#
# Rules to build the parallel object files

$(BUILDDIR)/ADAQAnalysis_MPI.o : $(SRCDIR)/ADAQAnalysis.cc
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/AAInterface_MPI.o : $(SRCDIR)/AAInterface.cc $(INCLDIR)/AAInterface.hh
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/AAComputation_MPI.o : $(SRCDIR)/AAComputation.cc $(INCLDIR)/AAComputation.hh
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/AAGraphics_MPI.o : $(SRCDIR)/AAGraphics.cc $(INCLDIR)/AAGraphics.hh
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/AAParallel_MPI.o : $(SRCDIR)/AAParallel.cc $(INCLDIR)/AAParallel.hh
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/AAInterpolation_MPI.o : $(SRCDIR)/AAInterpolation.cc $(INCLDIR)/AAInterpolation.hh $(INCLDIR)/AAInterpolationData.hh
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/ADAQAnalysisDict_MPI.o : $(BUILDDIR)/ADAQAnalysisDict.cc
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<


#************************************************#
# Rule to generate the necessary ROOT dictionary

$(BUILDDIR)/ADAQAnalysisDict.cc : $(INCLDIR)/*.hh $(INCLDIR)/RootLinkDef.h
	@echo -e "\nGenerating ROOT dictionary $@ ..."
	rootcint -f $@ -c -I$(ADAQHOME)/include $^ 


#*************#
# Phony rules
.PHONY: 
clean:
	@echo -e "\nCleaning up the build files ..."
	rm -f $(BUILDDIR)/* $(BINDIR)/*
	@echo -e ""

.PHONY:	
par:
	@echo -e "\nBuilding parallel version of ADAQAnalysis ...\n"
	@make ARCH=mpi -j2
	@echo -e ""

.PHONY:
both:
	@echo -e "\nBuilding sequential and parallel versions of ADAQAnalysis ...\n"
	@make -j3
	@make ARCH=mpi -j2
	@echo -e ""

.PHONY:
test:
	@echo "$(OBJS)"

# Useful notes for the uninitiated:
#
# target : dependency list
#  --TAB-- rule
#
# "$@" == subst. the word to the left of the ":"
# "$^" == subst. the words to the right of ":"
# "$<" == subst. first item in dependency list
