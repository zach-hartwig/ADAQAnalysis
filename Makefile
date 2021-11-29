#********************************************************************
#
#  name: Makefile                  
#  date: 29 Nov 21
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
# 	 dpnd: The build system depends on the following:
#        -- ROOT (mandatory) : v6.24.06 (most recent)
#        -- The ADAQ libraries: ADAQReadout, ASIMStorage (mandatory)
#        -- The Boost libraries: boost_thread, boost_system (mandatory)
#        -- Open MPI (optional) : v4.1.2 (most recent)
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

# Include the ROOT makefile from a system-wide installation. 
RC:=root-config
ROOTSYS:=$(shell $(RC) --etcdir)
ROOTMAKE:=$(ROOTSYS)/Makefile.arch
include $(ROOTMAKE)

# Include the ROOT extended library required for spectra processing
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

# Specify the necessary Boost libraries for linking
BOOSTLIBS = -lboost_thread -lboost_system

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
endif

# Include ADAQ header files; link against the ADAQReadout
# (experimental data) and ASIMReadout (simulated data) libraries
CXXFLAGS += -I$(ADAQHOME)/include
ADAQLIBS = -L$(ADAQHOME)/lib/$(HOSTTYPE) -lADAQReadout -lASIMStorage

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
	rootcling -f $@ -c -I$(ADAQHOME)/include $^
	cp $(BUILDDIR)/*.pcm $(BINDIR)


#*************#
# Phony rules
.PHONY: 
clean:
	@echo -e "\nCleaning up the build files ..."
	@rm -f $(BUILDDIR)/* $(BINDIR)/*
	@echo -e ""

# Automatically determine the number of processors on the
# system. Unsure if this works for MacOS so we should be on the
# lookout for issues flagged by users...
NPROCS:=$(shell nproc)

par:
	@echo -e "\nBuilding parallel version of ADAQAnalysis ...\n"
	@make ARCH=mpi -j$(NPROCS)
	@echo -e ""

both:
	@echo -e "\nBuilding sequential and parallel versions of ADAQAnalysis ...\n"
	@make -j$(NPROCS)
	@make ARCH=mpi -j$(NPROCS)
	@echo -e ""

# Useful notes for the uninitiated:
#
# target : dependency list
#  --TAB-- rule
#
# "$@" == subst. the word to the left of the ":"
# "$^" == subst. the words to the right of ":"
# "$<" == subst. first item in dependency list
