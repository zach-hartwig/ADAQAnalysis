#!/bin/bash
#
# name: install.sh
# date: 10 Oct 12
# auth: Zach Hartwig
#
# desc: Bash script for installing ADAQAnalysisGUi into a publicly
#       accessible location on the ADAQ server, which is at present
#       /usr/local/agnostic/adaq in this case. The main purpose is to
#       give Harold and Brandon ability to use the analysis tools as
#       ADAQ "users" without accessing the SVN repository as ADAQ
#       "developers". The main purpose of this script is to facilitate
#       keeping the most up-to-date version of the code publicly
#       available with minimal amount of work for me!
#
#       The script must be run as root, either by logging in as root
#       with the 'su -m" or from the command line with 'sudo -E' to
#       preserve environmental variables
#
# 2run: sudo -E ./install

# Check to ensure script is being run on the AGNOSTIC DAQ server
if [ "$HOSTNAME" != 'agnostic-daq' ]; then
    echo -e "\nError! The ADAQAnalysisGUI install script may only be run on agnostic-daq server!\n"
    exit
fi

# Define required ROOT environmental variables for the root superuser
export ROOTSYS=/usr/local/root/root_v5.32.01/root
export PATH=$ROOTSYS/bin:$PATH
export LD_LIBRARY_PATH=$ROOTSYS/lib:$LD_LIBRARY_PATH

# Define the source (origin) directory
SOURCE_DIR=$ADAQHOME

# Define the installation (destination) directory
INSTALL_DIR=/usr/local/agnostic/adaq

if [ -d $INSTALL_DIR ]; then
    rm $INSTALL_DIR -rf
fi

# Define useful relative directory paths
GUI_DIR=analysis/ADAQAnalysisGUI
INCL_DIR1=source/root/GUI/trunk/include
INCL_DIR2=source/ADAQ/include

# Make the required directory structure
mkdir -p $INSTALL_DIR/$GUI_DIR 
mkdir -p $INSTALL_DIR/$INCL_DIR1
mkdir -p $INSTALL_DIR/$INCL_DIR2

# Copy the ADAQAnalysisGUI source and required header files to their
# appropriate locations within the directory structure
cp -vr $SOURCE_DIR/$GUI_DIR/* $INSTALL_DIR/$GUI_DIR
cp -v $SOURCE_DIR/$INCL_DIR1/ADAQRootGUIClasses.hh $INSTALL_DIR/$INCL_DIR1
cp -v $SOURCE_DIR/$INCL_DIR2/ADAQRootClasses.hh $INSTALL_DIR/$INCL_DIR2

# Change the ADAQHOME env. var. to point to the newly installed
# directory.(required by the GNU makefile there to find the header
# locations). Go there and build the source code in both sequential
# and parallel versions.
export ADAQHOME=$INSTALL_DIR
cd $ADAQHOME/$GUI_DIR
./parallel.sh

# Modify the permissions and ownership of directories and files in the
# new directory to make ADAQAnalysis "public"
chown -R root:adaq_users $INSTALL_DIR
chmod -R 755 $INSTALL_DIR

# Finish with a flourish!
echo -e "\n***********************************************"
echo -e   "**   ADAQAnalysisGUI has been installed in:  **"
echo -e   "**        $INSTALL_DIR           **"
echo -e   "***********************************************\n"





