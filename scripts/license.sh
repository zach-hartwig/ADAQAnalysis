#!/bin/bash
#
# name: license.sh
# date: 17 Nov 14
# auth: Zach Hartwig
# mail: hartwig@psfc.mit.edu
# desc: Bash script to add license header to all files
# 2run: ./license.sh <arg>
# args: 'remove' : removes license header from all files

HeaderFiles="$(find ../include -name '*.h*')"
SourceFiles="$(find ../src -name '*.cc')"
Files=$HeaderFiles' '$SourceFiles

TmpFile=/tmp/tmp.txt

for File in $Files
do
    # Remove the license header
    if [ "$1" == "remove" ]
    then
	tail -n +13 "$File" > $TmpFile
	mv $TmpFile $File
	continue
    fi

    # Warn the user that license header exists; skip file
    CopyrightLine="$(sed '3q;d' $File)"
    if [[ $CopyrightLine == *Copyright* ]]
    then
	echo -e "Warning! The file '$File' already has a copyright header! Skipping ..."
	continue
    fi
    
    # Add the license header as shown below to top of file
    cat > $TmpFile << EOL
/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//                           Copyright (C) 2012-2021                           //
//                 Zachary Seth Hartwig : All rights reserved                  //
//                                                                             //
//      The ADAQAnalysis source code is licensed under the GNU GPL v3.0.       //
//      You have the right to modify and/or redistribute this source code      //      
//      under the terms specified in the license, which may be found online    //
//      at http://www.gnu.org/licenses or at \$ADAQANALYSIS/License.txt.       //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////

EOL

    cat $File >> $TmpFile
    mv $TmpFile $File
done

rm $TmpFile -f
