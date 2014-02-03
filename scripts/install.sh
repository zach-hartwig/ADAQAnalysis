#!/bin/bash
#
# name: install.sh
# date: 02 Feb 14
# auth: Zach Hartwig
#
# desc: Bash script for installing production version ADAQAnalysis
#       binaries and the required libraries into the publicly
#       accessible directory /usr/local/adaq. The list of production
#       versions may be obtained by running "git tags"
#
# 2run: (sudo -E) ./install <Version.number.ID> 

# Check to ensure the user has specified the version to copy

if [ ! "$#" == "1" ]; then
    echo -e "\nInstall script error! Only a single argument is allowed from the user:"
    echo -e   "   arg1 == ADAQAnalysis version number"
    echo -e   "   example: ./install 1.3.2"
    exit
fi

# Set the version number
VERSION=$1








echo -e "***********************************************"
echo -e "**   ADAQAnalysisGUI has been installed in:"
echo -e "**        $INSTALL_DIR"
echo -e "***********************************************\n"
