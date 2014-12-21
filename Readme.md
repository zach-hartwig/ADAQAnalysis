![AIMS Logo](doc/figures/AIMSLogo_BoldPastelColors.png "Accelerator-based In-situ Materials Surveillance")  
**A**ccelerator-based **I**n-situ **M**aterials **S**urveillance


## ADAQAnalysis ##

**ADAQAnalysis** is a program that provides comprehensive offline
analysis of digitized detector waveforms that were created and stored
with the **ADAQAcquisition** program. The program enables the user to
completely inspect and operate on raw waveforms (view, baseline
calculation, peak finding, pulse shape discriminate, etc), create and
analyze pulse spectra (calibrate, background subtraction, peak
finding, integration, etc), and to create publication quality graphics
in vector (EPS, PDF) and raster (JPG, PNG) formats. Methods are
provided to save all objects (waveforms, spectra, pulse shape
discrimination histograms, etc) to either ROOT files, ASCII files, or
CSV files for further analysis outside of **ADAQAnalysis**. Optically,
processing of waveforms can be performed in parallel to significantly
reduce computation time on systems where multiple cores are available.


### License and disclaimer ###

The **ADAQAnalysis** source code is licensed under the GNU General
Public License v3.0.  You have the right to modify and/or redistribute
this source code under the terms specified in the license,

**ADAQAnalysis** is provided *without any warranty nor guarantee of
fitness for any particular purpose*. The author(s) shall have no
liability for direct, indirect, or other undesirable consequences of
any character that may result from the use of this source code. This
may include - but is not limited - to irrevocable changes to the
user's firmware, software, hardware, or data. By your use of
**ADAQAnalysis**, you implicitly agree to absolve the author(s) of
any liability whatsoever. The reader is encouraged to consult the
**ADAQAnalysis** User's Guide and is advised that the use of this
source code is at his or her own risk.

A copy of the GNU General Public License v3.0 may be found within this
repository at $ADAQACQUISITION/License.md or is available online at
http://www.gnu.org/licenses.


### Build instructions  ###

The following lines should first be added to your .bashrc file to configure
your environment correctly:

```bash 
    # ADAQAnalysis configuration
    export ADAQANALYSIS_HOME = /full/path/to/ADAQAcquisition
    export ADAQANALYSIS_USER = dev # Setting for local install (developer)
    # ADAQANALYSIS_USER = usr # Setting for global install (user)
    source $ADAQANALYSIS_HOME/scripts/setup.sh $ADAQANALYSIS_USER >& /dev/null
```
Don't forget to open a new terminal for the settings to take effect!

On Linux or MacOS, clone into the repository and then use the provided
GNU Makefile to build the **ADAQAnalysis** binary:

```bash
  # Clone ADAQAnalysis source code from GitHub
  git clone https://github.com/ADAQAnalysis/ADAQAnalysis.git

  # Move to the ADAQAnalysis source code directory:
  cd ADAQAnalysis
  
  # To build the sequential binary locally
  make  

  # To build the parallel binary locally
  make par

  # To build both sequential and parallel binaries locally
  make both

  # To cleanup all build files:  
  make clean  
```


### Code dependencies ###

**ADAQAnalysis** depends on the following external codes and
libraries. Note that all *mandatory* dependencies mustbe properly
configured on the user's system prior to building ADAQAnalysis. Note
that the use of Open MPI to enable parallel processing of waveforms
with **ADAQAnalysis** is *optional*, as the code will compile
successfully with or without parallelization.

1. [GNU make](http://www.gnu.org/software/make/) : Mandatory

2. [ROOT](http://root.cern.ch/drupal/) : Mandatory

3. The ADAQ libraries : Mandatory

4. [Boost](http://www.boost.org/) : Mandatory

5. [Open MPI](http://www.open-mpi.org/) : Optional


### Directory structure ###

The ADAQAnalysis directory structure and build system are pretty
straightforward and easy to understand:

  - **bin/**       : Contains final binary

  - **build/**     : Contains transient build files

  - **doc/**       : LaTeX code for the ADAQAnalysis User's Guide

  - **include/**   : C++ header files, ROOT dictionary header file

  - **scripts/**   : Collection of Bash utility scripts

  - **src/**       : C++ source code 
  
  - **test/**      : Collection of test files in ADAQ format used in demo'ing, development, debugging

  - **Changelog.md** : List of major content updates and fixes for each Formulary release
  
  - **License.md**   : Markdown version of the GNU GPL v3.0 
  
  - **Makefile**     : GNU makefile for building ADAQAnalysis

  - **Readme.md**  : You're reading it


### Contact ###

Dr. Zachary S Hartwig  

Department of Nuclear Science and Engineering &  
Plasma Science and Fusion Center  
Massachusetts Institute of Technology  

phone: +1 617 253 5471  
email: [hartwig@psfc.mit.edu](mailto:hartwig@psfc.mit.edu)  
smail: 77 Massachusetts Ave, NW17-115, Cambridge MA 02139, USA



