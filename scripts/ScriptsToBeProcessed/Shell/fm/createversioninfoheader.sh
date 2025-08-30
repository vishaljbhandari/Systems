#!/bin/bash

## TAG=`svn info | sed -n 's/^URL:.*10.113.32.127\/\(.*Server\).*/\1/gp'`
TAG="svn://10.113.32.127/NikiraV6/branches/8.0/Roadmap/ROC-FM-8.0-SHM/Server"
if [ "$TAG" = "" ]
then
	echo "Error! Failed to retrieve Tag"
	exit 1
fi

## REVISION=`svn info | sed -n 's/^Revision: \(.*\)/\1/gp'`
REVISION="100141"
if [ "$REVISION" = ""  ]
then
	echo "Error! Failed to retrieve Revision"
	exit 1
fi

COMPILEDON=`date +%A", "%d/%B/%Y", "%r`
if [ $? -ne 0 ]
then
	echo "Error! Failed to retrieve Compilation Date"
	exit 1
fi

CONFIGUREDWITH=`cat $1/mk | sed -e 's/.*configure //g' -e 's/ --prefix.*//g'`
if [ "$CONFIGUREDWITH" = "" ]
then
	echo "Error! Failed to retrieve Configuration Options"
	exit 1
fi

cat > versioninfo.h << END_OF_VERSION_HEADER
/********************************************************************************
 *  Copyright (c) Subex Systems Limited 2001. All rights reserved.              *
 *  The copyright to the computer program(s) herein is the property of Subex    *
 *  Systems Limited. The program(s) may be used and/or copied with the written  *
 *  permission from Subex Systems Limited or in accordance with the terms and   *
 *  conditions stipulated in the agreement/contract under which the program(s)  *
 *  have been supplied.                                                         *
 *******************************************************************************/
#ifndef __VERSIONINFO_H__
#define __VERSIONINFO_H__

#include<iostream>
#include<string>

using namespace std ;

class CVersionInfo
{
    public:
        CVersionInfo()
        {
            strTag = "$TAG" ;
            strRevision = "$REVISION" ;
            strCompiledOn = "$COMPILEDON" ;			
            strConfiguredWith = "$CONFIGUREDWITH" ;
        }

		~CVersionInfo() {}

        void DisplayVersionInfo()
        {
            cout << "Tag/Branch       : " << strTag << endl ;
            cout << "Revision         : " << strRevision << endl ;
            cout << "Compiled On      : " << strCompiledOn << endl ;
            cout << "Configured With  : " << strConfiguredWith << endl  ;
        }

    private:
        string strRevision ;
        string strTag ;
        string strCompiledOn ;
        string strConfiguredWith ;
} ;

#endif
END_OF_VERSION_HEADER

exit 0
