#********************************************************************
#
#  name: Makefile                  
#  date: 11 Aug 14
#  auth: Zach Hartwig              
#  mail: hartwig@psfc.mit.edu
#
#  desc: GNUmakefile for building ADAQAnalysis code in sequential and
#        parallel versions (options for building are provided
#        below). The intermediary build files are placed in build/;
#        the final binaries are placed in bin/. Note that a list of
#        object files and build rules are automatically generated
#        based on the presence of source files for convenience.
#
#  dpnd: The build system depends on the following:
#        -- ROOT (mandatory)
#        -- The ADAQ{Hardware,Simulation}Readout libraries (mandatory)
#        -- Boost, including Boost thread libraries (mandatory)
#        -- Open MPI (optional)
# 
#  To build the sequential binary
#  $ make 
#
#  To build parallel binary (requires Open MPI)
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

# Include the ROOT makefile and tack on the TSpectrum library to
# ROOTGLIBS (general usage libraries plus graphics)
RC:=root-config
ROOTSYS:=$(shell $(RC) --prefix)
ROOTMAKE:=$(ROOTSYS)/etc/Makefile.arch
include $(ROOTMAKE)
ROOTGLIBS+=-lSpectrum

# Specify the the binary, build, and source directories
BUILDDIR = build
BINDIR = bin
SRCDIR = src

# Specify header files directory and tack it on to the CXXFLAGS. Note
# that this must be an absolute path to ensure the ROOT dictionary
# files can find the headers
INCLDIR = $(PWD)/include
CXXFLAGS += -I$(INCLDIR)

# Specify all header files
INCLS = $(INCLDIR)/*.hh

# Specify all object files (to be built in the build/ directory)
SRCS = $(wildcard $(SRCDIR)/*.cc)
TMP = $(patsubst %.cc,%.o,$(SRCS))
OBJS = $(subst src/,build/,$(TMP))

# Add the mandatory ROOT dictionary object file
OBJS += $(BUILDDIR)/ADAQAnalysisDict.o

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
   OBJS = $(patsubst %.o,%_MPI.o,$(TMP))

# If the user desires to build the sequential version of the binary
# then set the macros requires for the sequential build
else	
   SEQ_TARGET = $(BINDIR)/ADAQAnalysis
   CXX := clang++ -ferror-limit=5 -w
endif

# Include ADAQ header files; link against the ADAQSimulationReadout library
CXXFLAGS += -I$(ADAQHOME)/include
ADAQLIBS = -L$(ADAQHOME)/lib/$(HOSTTYPE) -lADAQSimulationReadout

#***************#
#**** RULES ****#
#***************#

# Rules to build the sequential binary

$(SEQ_TARGET) : $(OBJS) 
	@echo -e "\nBuilding sequential binary '$@' ..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(ADAQLIBS) $(ROOTGLIBS) $(BOOSTLIBS)
	@echo -e "\n$@ build is complete!\n"

$(BUILDDIR)/%.o : $(SRCDIR)/%.cc $(INCLS)
	@echo -e "\nBuilding object file '$@' ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<


# Rules to build the parallel binary

$(PAR_TARGET) : $(OBJS)
	@echo -e "\nBuilding parallel binary '$@' ..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(ADAQLIBS) $(ROOTGLIBS) $(BOOSTLIBS)
	@echo -e "\n$@ build is complete!\n"

$(BUILDDIR)/%_MPI.o : $(SRCDIR)/%.cc $(INCLS)
	@echo -e "\nBuilding object file '$@' ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<


#***************************************************#
# Rules to generate the necessary ROOT dictionaries

$(BUILDDIR)/ADAQAnalysisDict.o : $(BUILDDIR)/ADAQAnalysisDict.cc
	@echo -e "\nBuilding '$@' ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/ADAQAnalysisDict_MPI.o : $(BUILDDIR)/ADAQAnalysisDict.cc
	@echo -e "\nBuilding '$@' ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/ADAQAnalysisDict.cc : $(INCLS) $(INCLDIR)/RootLinkDef.h
	@echo -e "\nGenerating ROOT dictionary '$@' ..."
	rootcint -f $@ -c -I$(ADAQHOME)/include $^ 


#*************#
# Phony rules
.PHONY: 
clean:
	@echo -e "\nCleaning up the build files ..."
	rm -f $(BUILDDIR)/* $(BINDIR)/*
	@echo -e ""

par:
	@echo -e "\nBuilding parallel version of ADAQAnalysis ...\n"
	@make ARCH=mpi -j2
	@echo -e ""

both:
	@echo -e "\nBuilding sequential and parallel versions of ADAQAnalysis ...\n"
	@make -j3
	@make ARCH=mpi -j2
	@echo -e ""

# Useful notes for the uninitiated:
#
# target : dependency list
#  --TAB-- rule
#
# "$@" == subst. the word to the left of the ":"
# "$^" == subst. the words to the right of ":"
# "$<" == subst. first item in dependency list
