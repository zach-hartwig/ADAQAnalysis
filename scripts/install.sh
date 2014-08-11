#!/bin/bash
#
# name: install.sh
# date: 10 Mar 14
# auth: Zach Hartwig
# mail: hartwig@psfc.mit.edu
#
# desc: Bash script for installing production version ADAQAnalysis
#       binaries and the required libraries into the publicly
#       accessible directory /usr/local/adaq. The list of production
#       versions may be obtained by running "git tags". Note that only
#       Git-tagged production version may be installed with this
#       script. The tagged version is checked out into its own
#       transient branch, where it is built and from which it is
#       installed into /usr/local/adaq.
#
# 2run: ./install <Git.Version.Tag> 

# Check to ensure the user has specified the version to copy

if [ ! "$#" == "1" ]; then
    echo -e "\nInstall script error! Only a single argument is allowed from the user:"
    echo -e   "   arg1 == ADAQAnalysis version number"
    echo -e   "   example: ./install.sh v1.3.2\n"
    exit
fi

# Set the version number
VERSION=$1

# Check to ensure that the desired version exists
AVAILABLE=$(git tag)
if [[ $AVAILABLE != *$VERSION* ]]; then
    echo -e "\nInstall script error! The available versions are:"
    echo -e   "--> $AVAILABLE\n"
    exit
fi

# Set the installation location
INSTALLDIR=/usr/local/adaq

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TOPDIR=${SCRIPTDIR///scripts/}

# Checkout the tagged version to a separate branch, build and install
# the binaries, and the remove the temporary branch
cd $TOPDIR

echo -e "\nChecking out tagged version from the Git repository ...\n"
git co $VERSION -b $VERSION

# Overwrite the include/AAVersion.hh header such that the correct
# production version number is accessible to the binary build
VERSION_ID=${VERSION/v/}
cat > include/AAVersion.hh << EOL
#ifndef __AAVersion_hh__
#define __AAVersion_hh__ 1

const string VersionString = "$VERSION_ID";

#endif 
EOL

echo -e "\nBuilding the ADAQAnalysis binaries ..."
make clean >& /dev/null
make -j2 both >& /dev/null

echo -e "\nCopying the binaries to $INSTALLDIR ..."
sudo cp bin/* $INSTALLDIR >& /dev/null

echo -e "\nRemoving the temporary tag branch ..."
git stash >& /dev/null
git co master >& /dev/null
git br -D $VERSION >& /dev/null

echo -e "\n*********************************************"
echo -e   "**   ADAQAnalysis has been installed in:   **"
echo -e   "**           $INSTALLDIR               **"
echo -e   "*********************************************\n"
