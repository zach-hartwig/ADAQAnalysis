#********************************************************************
#  name: Makefile                  
#  date: 22 Jan 13
#  auth: Zach Hartwig              
#
#  desc: GNUmakefile for building ADAQAnalysisGUI code in seqential
#        and parallel versions (options for building are provided
#        below). The intermediary build files are placed in the build/
#        directory; the final binaries are build in the bin/
#        directory.
#
#  Dependencies
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

#**** MACRO DEFINITIONS ****#

# Include the Makefile for ROOT-based projects
RC:=root-config
ROOTSYS:=$(shell $(RC) --prefix)
ROOTMAKE:=$(ROOTSYS)/etc/Makefile.arch
ROOTLIB = -lSpectrum
include $(ROOTMAKE)

# Specify the locatino of the build directory
BUILDDIR = build

# Specify the locatino of the binary directory
BINDIR = bin

# Specify the location of the source and header files
SRCDIR = src
INCLDIR = include

# Specify all object files (to be built in the build/ directory)
OBJS = $(BUILDDIR)/ADAQAnalysisGUI.o $(BUILDDIR)/ADAQAnalysisGUIDict.o 

# Specify the includes for the ROOT dictionary build
INCLUDES=$(ADAQHOME)/source/root/GUI/trunk/include
CXXFLAGS += -I$(INCLUDES) -I$(INCLDIR) #-I.

# Specify the required Boost libraries
#CXXFLAGS += -lboost_thread-mt
BOOSTLIBS = -lboost_thread-mt

# If the user desires to build a parallel version of the binary then
# set a number of key macros for the build. Note that the Open MPI C++
# compiler is specifed (rather than gcc) and the MPI_ENABLED macro
# will be passed to the compiler. The MPI_ENABLED macro is used as a
# preprocessing directive in the source code to protect MPI code in
# case the user would like to build on a system without Open MPI
ifeq ($(ARCH),mpi)
   PAR_TARGET = $(BINDIR)/ADAQAnalysisGUI_MPI
   CXX := mpic++
   CXXFLAGS += -DMPI_ENABLED -I.
   MPI_ENABLED := 1
   TMP := $(OBJS)
   OBJS = $(patsubst %.o, %_MPI.o, $(TMP))

# If the user desires to build teh sequential version of the binary
# then set the macros requires for the sequential build
else	
   SEQ_TARGET = $(BINDIR)/ADAQAnalysisGUI

   ifeq ($(HOSTNAME),TheBlackArrow)
      CXX := clang++
   else
      CXX := g++
   endif

endif

#**** RULES ****#

# Rules to build the sequential (SEQ) and the paralle (PAR) binary
# versions of ADAQAnalysisGUI

$(SEQ_TARGET) : $(OBJS)
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(ROOTGLIBS) $(ROOTLIB) $(BOOSTLIBS)
	@echo -e "\n$@ build is complete!\n"

$(PAR_TARGET) : $(OBJS)
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(ROOTGLIBS) $(ROOTLIB) $(BOOSTLIBS)
	@echo -e "\n$@ build is complete!\n"

# Rules to build the sequential object files

$(BUILDDIR)/ADAQAnalysisGUI.o : $(SRCDIR)/ADAQAnalysisGUI.cc 
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/ADAQAnalysisGUIDict.o : $(BUILDDIR)/ADAQAnalysisGUIDict.cc
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Rules to build the parallel object files

$(BUILDDIR)/ADAQAnalysisGUI_MPI.o : $(SRCDIR)/ADAQAnalysisGUI.cc
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/ADAQAnalysisGUIDict_MPI.o : $(BUILDDIR)/ADAQAnalysisGUIDict.cc
	@echo -e "\nBuilding $@ ..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Rule to generate the necessary ROOT dictionary
$(BUILDDIR)/ADAQAnalysisGUIDict.cc : $(INCLDIR)/ADAQAnalysisGUI.hh $(INCLDIR)/RootLinkDef.h
	@echo -e "\nGenerating ROOT dictionary $@ ..."
	rootcint -f $@ -c -I$(INCLUDES) $^ 

# Clean the directory of all build files and binaries
.PHONY: 
clean:
	@echo -e "\nCleaning up the build files ..."
	rm -f $(BUILDDIR)/* $(BINDIR)/*
	@echo -e ""

.PHONY:	
par:
	@echo -e "\nBuilding parallel version of ADAQAnalysisGUI ...\n"
	@make ARCH=mpi -j3
	@echo -e ""

.PHONY:
both:
	@echo -e "\nBuilding sequential and parallel versions of ADAQAnalysisGUI ...\n"
	@make -j3
	@make ARCH=mpi -j3
	@echo -e ""

# Useful notes for the uninitiated:
#
# target : dependency list
#  --TAB-- rule
#
# "$@" == subst. the word to the left of the ":"
# "$^" == subst. the words to the right of ":"
# "$<" == subst. first item in dependency list
