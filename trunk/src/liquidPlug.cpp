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

// Maya's Headers
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>
#include <maya/MSwatchRenderRegister.h>

#include <liquid.h>
#include <liqRibTranslator.h>
#include <liqGetAttr.h>
#include <liqAttachPrefAttribute.h>
#include <liqPreviewShader.h>
#include <liqWriteArchive.h>
#include <liqNodeSwatch.h>
#include <liqSurfaceNode.h>
#include <liqDisplacementNode.h>
#include <liqVolumeNode.h>
#include <liqLightNode.h>
#include <liqLightNodeBehavior.h>
#include <liqRibboxNode.h>
#include <liqCoordSysNode.h>
#include <liqGlobalHelpers.h>
#include <liqMayaRenderView.h>
#include <liqGlobalsNode.h>
#include <liqJobList.h>


#define LIQVENDOR "http://liquidmaya.sourceforge.net/"

#if defined(_WIN32) /*&& !defined(DEFINED_LIQUIDVERSION)*/
// unix build gets this from the Makefile
static const char * LIQUIDVERSION =
#include "liquid.version"
;
#define DEFINED_LIQUIDVERSION
#endif

extern bool liquidBin;


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
  status = plugin.registerCommand( "liquidAttachPrefAttribute", liqAttachPrefAttribute::creator, liqAttachPrefAttribute::syntax );
  LIQCHECKSTATUS( status, "Can't register liquidAttachPrefAttribute command" );

  // register the liquidPreviewShader command
  status = plugin.registerCommand( "liquidPreviewShader", liqPreviewShader::creator, liqPreviewShader::syntax );
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

  // register the liquidRenderView command
  status = plugin.registerCommand( "liquidRenderView", liqMayaRenderCmd::creator ,liqMayaRenderCmd::newSyntax);
  LIQCHECKSTATUS( status, "Can't register liquidRenderView command" );

  // register the liquidJobList command
  status = plugin.registerCommand( "liquidJobList", liqJobList::creator ,liqJobList::syntax);
  LIQCHECKSTATUS( status, "Can't register liquidJobList command" );

  // register the liquidShader node
  const MString UserClassify( "shader/surface:swatch/liqSurfSwatch" );
  status = plugin.registerNode( "liquidSurface", liqSurfaceNode::id, liqSurfaceNode::creator, liqSurfaceNode::initialize, MPxNode::kDependNode, &UserClassify );
  LIQCHECKSTATUS( status, "Can't register liquidSurface node" );
  status.clear();
  status = MSwatchRenderRegister::registerSwatchRender( "liqSurfSwatch", liqNodeSwatch::creator );
  LIQCHECKSTATUS( status, "Can't register liquidSurface swatch" );

  // register the liquidDisplacement node
  const MString UserClassify1( "shader/displacement:swatch/liqDispSwatch" );
  status = plugin.registerNode( "liquidDisplacement", liqDisplacementNode::id, liqDisplacementNode::creator, liqDisplacementNode::initialize, MPxNode::kDependNode, &UserClassify1 );
  LIQCHECKSTATUS( status, "Can't register liquidDisp node" );
  status.clear();
  status = MSwatchRenderRegister::registerSwatchRender( "liqDispSwatch", liqNodeSwatch::creator );
  LIQCHECKSTATUS( status, "Can't register liquidDisplacement swatch" );

  // register the liquidVolume node
  const MString UserClassify2( "shader/volume:swatch/liqVolSwatch" );
  status = plugin.registerNode( "liquidVolume", liqVolumeNode::id, liqVolumeNode::creator, liqVolumeNode::initialize, MPxNode::kDependNode, &UserClassify2 );
  LIQCHECKSTATUS( status, "Can't register liquidVolume node" );
  status.clear();
  status = MSwatchRenderRegister::registerSwatchRender( "liqVolSwatch", liqNodeSwatch::creator );
  LIQCHECKSTATUS( status, "Can't register liquidVolume swatch" );

  // register the liquidLight node
  const MString UserClassify3( "shader/surface:swatch/liqLightSwatch" );
  status = plugin.registerNode( "liquidLight", liqLightNode::id, liqLightNode::creator, liqLightNode::initialize, MPxNode::kDependNode, &UserClassify3 );
  LIQCHECKSTATUS( status, "Can't register liquidLight node" );
  status.clear();
  status = MSwatchRenderRegister::registerSwatchRender( "liqLightSwatch", liqNodeSwatch::creator );
  LIQCHECKSTATUS( status, "Can't register liqLightSwatch swatch" );
  status = plugin.registerDragAndDropBehavior( "liquidLightBehavior", liqLightNodeBehavior::creator);
  LIQCHECKSTATUS( status, "Can't register liquidLight behavior" );

  // register the liquidRibbox node
  const MString UserClassify4( "utility/general:swatch/liqRibSwatch" );
  status = plugin.registerNode( "liquidRibBox", liqRibboxNode::id, liqRibboxNode::creator, liqRibboxNode::initialize, MPxNode::kDependNode, &UserClassify4 );
  LIQCHECKSTATUS( status, "Can't register liquidRibbox node" );
  status.clear();
  status = MSwatchRenderRegister::registerSwatchRender( "liqRibSwatch", liqNodeSwatch::creator );
  LIQCHECKSTATUS( status, "Can't register liquidRibbox swatch" );

  // register the liquidCoordSys node
  status = plugin.registerNode( "liquidCoordSys", liqCoordSysNode::id, liqCoordSysNode::creator, liqCoordSysNode::initialize, MPxNode::kLocatorNode );
  LIQCHECKSTATUS( status, "Can't register liquidCoordSys node" );
  status.clear();

  // register the liquidGlobals node
  status = plugin.registerNode( "liquidGlobals", liqGlobalsNode::id, liqGlobalsNode::creator, liqGlobalsNode::initialize, MPxNode::kDependNode );
  LIQCHECKSTATUS( status, "Can't register liquidGlobals node" );
  status.clear();

  // setup all of the base liquid interface
  MString sourceLine("source ");
  char *tmphomeChar;
  if( ( tmphomeChar = getenv( "LIQUIDHOME" ) ) ) {

#ifndef WIN32

    MString tmphome( tmphomeChar );
    sourceLine += "\"" + liquidSanitizePath( tmphome ) + "/mel/" + "liquidStartup.mel\"";
#else
	for (unsigned k( 0 );k<strlen(tmphomeChar); k++ ) {
		if ( tmphomeChar[ k ] == '\\' ) tmphomeChar[ k ] = '/';
	}

	MString tmphome( tmphomeChar );
	sourceLine += "\"" + tmphome + "/mel/" + "liquidStartup.mel\"";
#endif
  } else {
    sourceLine += "\"liquidStartup.mel\"";
  }

  status = MGlobal::executeCommand(sourceLine);

  status = plugin.registerUI("liquidStartup", "liquidShutdown");
  LIQCHECKSTATUS( status, "Can't register liquidStartup and liquidShutdown interface scripts" );
  cout << "Liquid " << LIQUIDVERSION << " registered"<< endl;
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

  status = plugin.deregisterCommand("liquidRenderView");
  LIQCHECKSTATUS( status, "Can't deregister liquidRenderView command" );

  status = plugin.deregisterCommand("liquidJobList");
  LIQCHECKSTATUS( status, "Can't deregister liquidJobList command" );

  status = plugin.deregisterNode( liqSurfaceNode::id );
  LIQCHECKSTATUS( status, "Can't deregister liquidSurface node" );
  status = MSwatchRenderRegister::unregisterSwatchRender( "liqSurfSwatch" );
  LIQCHECKSTATUS( status, "Can't deregister liquidSurface swatch generator" );

  status = plugin.deregisterNode( liqDisplacementNode::id );
  LIQCHECKSTATUS( status, "Can't deregister liquidDisp node" );
  status = MSwatchRenderRegister::unregisterSwatchRender( "liqDispSwatch" );
  LIQCHECKSTATUS( status, "Can't deregister liquidDisp swatch generator" );

  status = plugin.deregisterNode( liqVolumeNode::id );
  LIQCHECKSTATUS( status, "Can't deregister liquidVolume node" );
  status = MSwatchRenderRegister::unregisterSwatchRender( "liqVolSwatch" );
  LIQCHECKSTATUS( status, "Can't deregister liquidVolume swatch generator" );

  status = plugin.deregisterNode( liqLightNode::id );
  LIQCHECKSTATUS( status, "Can't deregister liquidLight node" );
  status = MSwatchRenderRegister::unregisterSwatchRender( "liqLightSwatch" );
  LIQCHECKSTATUS( status, "Can't deregister liquidLight swatch generator" );
  status = plugin.deregisterDragAndDropBehavior( "liquidLightBehavior" );
  LIQCHECKSTATUS( status, "Can't deregister liquidLight behavior" );

  status = plugin.deregisterNode( liqRibboxNode::id );
  LIQCHECKSTATUS( status, "Can't deregister liquidRibbox node" );
  status = MSwatchRenderRegister::unregisterSwatchRender( "liqRibSwatch" );
  LIQCHECKSTATUS( status, "Can't deregister liquidRibbox swatch generator" );

  status = plugin.deregisterNode( liqCoordSysNode::id );
  LIQCHECKSTATUS( status, "Can't deregister liquidCoordSys node" );

  status = plugin.deregisterNode( liqGlobalsNode::id );
  LIQCHECKSTATUS( status, "Can't deregister liquidGlobals node" );

  cout <<"Liquid "<< LIQUIDVERSION << " unregistered"<<endl<<endl;

  return MS::kSuccess;
}
