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
#include <sys/types.h>

#ifndef _WIN32
// Dynamic Object Headers
#include <dlfcn.h>
#endif

#ifdef _WIN32
#pragma warning(disable:4786)
#endif

// DLL export symbols must be specified under Win32
#ifdef _WIN32
#define LIQUID_EXPORT _declspec(dllexport)
#else
#define LIQUID_EXPORT
#endif

// Renderman Headers
extern "C" {
#include <ri.h>
}

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
#include <liqPreviewShader.h>
#include <liquidMemory.h>

extern  bool	liquidBin;

#define LIQVENDOR "Colin_Doncaster_and_friends"

////////////////////// EXPORTS /////////////////////////////////////////////////////////
LIQUID_EXPORT MStatus initializePlugin(MObject obj)
//
//  Description:
//      Register the command when the plug-in is loaded
//
{
    liquidBin = false;
		
    MFnPlugin plugin( obj, LIQVENDOR, LIQUIDVERSION, "Any");

    MStatus status;
    MString command;
    MString UserClassify;

    MGlobal::displayInfo("\nLiquid by Colin Doncaster\n");

    status = plugin.registerCommand("liquid", liquidRibTranslator::creator );
    LIQCHECKSTATUS( status, "Can't register liquid translator plugin" );

    // register the liquidAttachPrefAttribute command
    status = plugin.registerCommand( "liquidAttachPrefAttribute", liquidAttachPrefAttribute::creator );
    LIQCHECKSTATUS( status, "Can't register liquidAttachPrefAttribute plugin" );

    // register the liquidAttachPrefAttribute command
    status = plugin.registerCommand( "liquidPreviewShader", liqPreviewShader::creator );
    LIQCHECKSTATUS( status, "Can't register liquidPreviewShader plugin" );

    // register the liquidGetSloInfo command
    status = plugin.registerCommand( "liquidGetSloInfo", liquidGetSloInfo::creator );
    LIQCHECKSTATUS( status, "Can't register liquidGetSloInfo plugin" );

    // register the liquidGetAttr command
    status = plugin.registerCommand( "liquidGetAttr", liquidGetAttr::creator );
    LIQCHECKSTATUS( status, "Can't register liquidGetAttr plugin" );

    // setup all of the base liquid interface
    command = "source liquidStartup.mel";
    status = MGlobal::executeCommand(command);

    status = plugin.registerUI("liquidStartup", "liquidShutdown");
    LIQCHECKSTATUS( status, "Can't register liquidStartup and  liquidShutdown plugin" );
    return MS::kSuccess;
}

LIQUID_EXPORT MStatus uninitializePlugin(MObject obj)
//
//  Description:
//      Deregister the command when the plug-in is deloaded
//
{
    MString UserClassify;
    MString command;
    MStatus status;
    MFnPlugin plugin(obj);

    status = plugin.deregisterCommand("liquid");
    LIQCHECKSTATUS( status, "Can't deregister liquid plugin" );

    status = plugin.deregisterCommand("liquidAttachPrefAttribute");
    LIQCHECKSTATUS( status, "Can't deregister liquidAttachPrefAttribute plugin" );

    status = plugin.deregisterCommand("liquidPreviewShader");
    LIQCHECKSTATUS( status, "Can't deregister liquidPreviewShader plugin" );

    status = plugin.deregisterCommand("liquidGetSloInfo");
    LIQCHECKSTATUS( status, "Can't deregister liquidGetSloInfo plugin" );

    status = plugin.deregisterCommand("liquidGetAttr");
    LIQCHECKSTATUS( status, "Can't deregister liquidGetAttr plugin" );
    // remove the UI
    MGlobal::displayInfo("\n--* Liquid *--\n");
    MGlobal::displayInfo("\nUninitialized...\n");
    return MS::kSuccess;
}
