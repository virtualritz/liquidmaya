/*
**
** The contents of this file are subject to the Mozilla Public License Version 1.1 (the 
** "License"); you may not use this file except in compliance with the License. You may 
** obtain a copy of the License at http://www.mozilla.org/MPL/ 
** 
** Software distributed under the License is distributed on an "AS IS" basis, WITHOUT 
** WARRANTY OF ANY KIND, either express or implied. See the License for the specific 
** language governing rights and limitations under the License. 
**
** The Original Code is the Liquid Rendering Toolkit. 
** 
** The Initial Developer of the Original Code is Colin Doncaster. Portions created by 
** Colin Doncaster are Copyright (C) 2002. All Rights Reserved. 
** 
** Contributor(s): Berj Bannayan. 
**
** 
** The RenderMan (R) Interface Procedures and Protocol are:
** Copyright 1988, 1989, Pixar
** All Rights Reserved
**
**
** RenderMan (R) is a registered trademark of Pixar
*/

/* ______________________________________________________________________
** 
** Liquid Plug-In
** ______________________________________________________________________
*/

// Standard Headers
#include <iostream.h>
#include <fstream.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>

#ifndef _WIN32
// Dynamic Object Headers
#include <dlfcn.h>
#endif

// Renderman Headers
extern "C" {
	#include <ri.h>
	#include <slo.h>
}

// STL headers
#include <list>
#include <vector>
#include <string>

#ifdef _WIN32
	#include <process.h>
	#include <malloc.h>
#else
	#include <unistd.h>
	#include <stdlib.h>
	#include <alloca.h>
#endif

// Maya's Headers
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>

#include <liquid.h>
#include <liquidRibTranslator.h>
#include <liquidGetSloInfo.h>
#include <liquidGetAttr.h>
#include <liquidAttachPrefAttribute.h>
#include <liquidPreviewShader.h>
#include <liquidMemory.h>

	bool			isRegistered;
	extern	float liquidVersion;
	extern  bool	liquidBin;

////////////////////// EXPORTS /////////////////////////////////////////////////////////
MStatus initializePlugin(MObject obj)
//
//  Description:
//      Register the command when the plug-in is loaded
//
{
		liquidBin = false;
		
    MFnPlugin plugin( obj, VENDOR, "1.2", "Any");

    MStatus status;
    MString command;
    MString UserClassify;

#ifdef _WIN32
    printf("\nLiquid by Colin Doncaster\n");
#else
    MGlobal::displayInfo("\nLiquid by Colin Doncaster\n");
		MString expireString( "Expires on ");
		expireString += expday;
		expireString += "/";
		expireString += expmonth;
		expireString += "/";
		expireString += expyear;
		expireString += "\n";
    MGlobal::displayInfo( expireString );
#endif

	printf("Expires on %d/%d/%d \n", expday, expmonth, expyear + 1900 );
	// make sure the beta version expires, this will be the license handling routines also!
	time_t ltime;
	struct tm *today;
	time( &ltime );
	today = localtime( &ltime );
	//int wyear, wmonth, wday;
	isRegistered = true;
	if ( today->tm_year > expyear ) {
		printf("Liquid Expired! contact buzz1@paradise.net.nz or canuck@wetafx.co.nz! ");
		isRegistered = false;
	}
	if ( today->tm_mon > (expmonth - 1) && today->tm_year == expyear ) {
		printf("Liquid Expired! contact buzz1@paradise.net.nz or canuck@wetafx.co.nz! ");
		isRegistered = false;
	}
	if ( today->tm_mday > expday && today->tm_mon == (expmonth - 1) && today->tm_year == expyear ) {
		printf("Liquid Expired! contact buzz1@paradise.net.nz or canuck@wetafx.co.nz! ");
		isRegistered = false;
	}

	isRegistered = true;
	
	if (isRegistered) {
		// register the liquid command
		status = plugin.registerCommand("liquid", RibTranslator::creator );
		if (!status) {
			return MS::kFailure;
	    }
	} else {
		printf("%s \n", NOLICMSG);
		printf("You may proceed to set up a render - but won't be able to output it. \n");
	}

	// register the liquidAttachPrefAttribute command
	status = plugin.registerCommand( "liquidAttachPrefAttribute", liquidAttachPrefAttribute::creator );
	if ( !status )
        	return MS::kFailure;

	// register the liquidAttachPrefAttribute command
	status = plugin.registerCommand( "liquidPreviewShader", liquidPreviewShader::creator );
	if ( !status )
        	return MS::kFailure;

	// register the liquidGetSloInfo command
	status = plugin.registerCommand( "liquidGetSloInfo", liquidGetSloInfo::creator );
	if ( !status )
		return MS::kFailure;
	
	// register the liquidGetAttr command
	status = plugin.registerCommand( "liquidGetAttr", liquidGetAttr::creator );
	if ( !status )
		return MS::kFailure;
	
	// setup all of the base liquid interface
	command = "source liquidStartup.mel";
	status = MGlobal::executeCommand(command);

	status = plugin.registerUI("liquidStartup", "liquidShutdown");
	
	if (!status) {
		return MS::kFailure;
    }
    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
//
//  Description:
//      Deregister the command when the plug-in is deloaded
//
{
	MString UserClassify;
	MString command;
	MStatus status;
	MFnPlugin plugin(obj);
	
	//deregister liquid command
	if (isRegistered != false) {
		status = plugin.deregisterCommand("liquid");
		if (!status) {
			return MS::kFailure;
	    }
	}

	//deregister liquidAttachPrefAttribute command
        if (isRegistered != false) {
                status = plugin.deregisterCommand("liquidAttachPrefAttribute");
                if (!status) {
                        return MS::kFailure;
            }
        }

					//deregister liquidAttachPrefAttribute command
        if (isRegistered != false) {
                status = plugin.deregisterCommand("liquidPreviewShader");
                if (!status) {
                        return MS::kFailure;
            }
        }


	//deregister liquidGetSloInfo command
	if (isRegistered != false) {
		status = plugin.deregisterCommand("liquidGetSloInfo");
		if (!status) {
			return MS::kFailure;
	    }
	}

	//deregister liquidGetAttr command
	if (isRegistered != false) {
		status = plugin.deregisterCommand("liquidGetAttr");
		if (!status) {
			return MS::kFailure;
	    }
	}
	
	// remove the UI
	MGlobal::displayInfo("\n--* Liquid *--\n");
	MGlobal::displayInfo("\nUninitialized...\n");
	return MS::kSuccess;
}
