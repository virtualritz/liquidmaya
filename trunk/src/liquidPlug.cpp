/*
**
** The contents of this file are subject to the Mozilla Public License Version
** 1.1 (the "License"); you may not use this file except in compliance with
** the License. You may obtain a copy of the License at
** http://www.mozilla.org/MPL/ 
** 
** Software distributed under the License is distributed on an "AS IS" basis,
** WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
** for the specific language governing rights and limitations under the
** License. 
**
** The Original Code is the Liquid Rendering Toolkit. 
** 
** The Initial Developer of the Original Code is Colin Doncaster. Portions
** created by Colin Doncaster are Copyright (C) 2002. All Rights Reserved. 
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
//#include <fstream.h>
#include <liqIOStream.h>
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
#include <liqRibTranslator.h>
#include <liqGetSloInfo.h>
#include <liqGetAttr.h>
#include <liqAttachPrefAttribute.h>
#include <liqMemory.h>
#include <liqPreviewShader.h>
#include <liqWriteArchive.h>
#include <liqGlobalHelpers.h>

extern bool liquidBin;

#define LIQVENDOR "http://liquidmaya.sourceforge.net/"

#if defined(_WIN32) && !defined(DEFINED_LIQUIDVERSION)
// unix build gets this from the Makefile
static const char * LIQUIDVERSION = 
#include "liquid.version"
;
#define DEFINED_LIQUIDVERSION
#endif


////////////////////// EXPORTS /////////////////////////////////////////////////////////
LIQUID_EXPORT MStatus initializePlugin(MObject obj)
//  Description:
//      Register the command when the plug-in is loaded
{
  liquidBin = false;

  MStatus status;
  
  MFnPlugin plugin( obj, LIQVENDOR, LIQUIDVERSION, "Any");

  MGlobal::displayInfo(MString("Initializing Liquid v") + LIQUIDVERSION);
  MGlobal::displayInfo("Initial Liquid code by Colin Doncaster");

  status = plugin.registerCommand("liquid", liqRibTranslator::creator, liqRibTranslator::syntax );
  LIQCHECKSTATUS( status, "Can't register liquid translator command" );

  // register the liquidAttachPrefAttribute command
  status = plugin.registerCommand( "liquidAttachPrefAttribute", liqAttachPrefAttribute::creator );
  LIQCHECKSTATUS( status, "Can't register liquidAttachPrefAttribute command" );

  // register the liquidAttachPrefAttribute command
  status = plugin.registerCommand( "liquidPreviewShader", liqPreviewShader::creator );
  LIQCHECKSTATUS( status, "Can't register liqPreviewShader command" );

  // register the liqGetSloInfo command
  status = plugin.registerCommand( "liquidGetSloInfo", liqGetSloInfo::creator );
  LIQCHECKSTATUS( status, "Can't register liquidGetSloInfo command" );

  // register the liquidGetAttr command
  status = plugin.registerCommand( "liquidGetAttr", liqGetAttr::creator );
  LIQCHECKSTATUS( status, "Can't register liquidGetAttr command" );

  // register the liquidWriteArchive command
  status = plugin.registerCommand( "liquidWriteArchive", liqWriteArchive::creator, liqWriteArchive::syntax );
  LIQCHECKSTATUS( status, "Can't register liquidWriteArchive command" );

  // setup all of the base liquid interface
  MString sourceLine("source ");
  MString tmphome(getenv( "LIQUIDHOME" ));
  if( tmphome != "" ) {
  	sourceLine += "\"" + liquidSanitizePath( tmphome ) + "/mel/" + "liquidStartup.mel\"";
  } else {
	  sourceLine += "liquidStartup.mel";
  }
  
  status = MGlobal::executeCommand(sourceLine);

  status = plugin.registerUI("liquidStartup", "liquidShutdown");
  LIQCHECKSTATUS( status, "Can't register liquidStartup and liquidShutdown interface scripts" );
  return MS::kSuccess;
}

LIQUID_EXPORT MStatus uninitializePlugin(MObject obj)
//  Description:
//      Deregister the command when the plug-in is unloaded
{
  MStatus status;
  MFnPlugin plugin(obj);

  status = plugin.deregisterCommand("liquid");
  LIQCHECKSTATUS( status, "Can't deregister liquid command" );

  status = plugin.deregisterCommand("liquidAttachPrefAttribute");
  LIQCHECKSTATUS( status, "Can't deregister liquidAttachPrefAttribute command" );

  status = plugin.deregisterCommand("liquidPreviewShader");
  LIQCHECKSTATUS( status, "Can't deregister liquidPreviewShader command" );

  status = plugin.deregisterCommand("liquidGetSloInfo");
  LIQCHECKSTATUS( status, "Can't deregister liquidGetSloInfo command" );

  status = plugin.deregisterCommand("liquidGetAttr");
  LIQCHECKSTATUS( status, "Can't deregister liquidGetAttr command" );

  status = plugin.deregisterCommand("liquidWriteArchive");
  LIQCHECKSTATUS( status, "Can't deregister liquidWriteArchive command" );

  return MS::kSuccess;
}
