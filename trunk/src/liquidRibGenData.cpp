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
** Liquid Rib Gen Data Source 
** ______________________________________________________________________
*/

// Standard Headers
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>

#ifndef _WIN32
// Dynamic Object Headers
#include <dlfcn.h>
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
#include <maya/MDagPath.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MPlugArray.h>

#include <liquid.h>
#include <liquidGlobalHelpers.h>
#include <liquidRibGenData.h>
#include <liquidMemory.h>
#include <liquidRIBGen.h>

extern int debugMode;

extern FILE *liqglo_ribFP;
extern long liqglo_lframe;
extern structJob liqglo_currentJob;
extern bool liqglo_doMotion;
extern bool liqglo_doDef;
extern bool liqglo_doCompression;
extern bool liqglo_doBinary;
extern RtFloat liqglo_sampleTimes[5];
extern liquidlong liqglo_motionSamples;
extern float liqglo_shutterTime;

liquidRibGenData::liquidRibGenData( MObject obj, MDagPath path )
//
//  Description:
//      create a RIB Gen
//
{
    if ( debugMode ) { printf("-> creating ribgen\n"); }
	MFnDependencyNode fnNode( obj );
	MPlug ribGenNodePlug = fnNode.findPlug( "liquidRibGen" );
	MObject ribGenObj;
	/* check the node to make sure it's not using the old ribGen assignment method, this is for backwards
	compatibility.  If it's a kTypedAttribute that it's more than likely going to be a string! */
	if ( ribGenNodePlug.attribute().apiType() == MFn::kTypedAttribute ) {
		MString ribGenNode;
		ribGenNodePlug.getValue( ribGenNode );
		MSelectionList ribGenList;
		MStatus ribGenAddStatus = ribGenList.add( ribGenNode );
		ribGenList.getDependNode( 0, ribGenObj );
	} else {
		if ( ribGenNodePlug.isConnected() ) {
			MPlugArray ribGenNodeArray;
			ribGenNodePlug.connectedTo( ribGenNodeArray, true, true );
			ribGenObj = ribGenNodeArray[0].node();
		}
	}
	MFnDependencyNode fnRibGenNode( ribGenObj );
	MPlug ribGenPlug = fnRibGenNode.findPlug( "RibGenSo" );
	MString plugVal;
	ribGenPlug.getValue( plugVal );
	ribGenSoName = parseString( plugVal );
	ribStatus.objectName = (char *)lmalloc( sizeof(char) * ( fnNode.name().length() + 1 ) );
	strcpy( ribStatus.objectName, fnNode.name().asChar() );
	ribStatus.dagPath = path;
}

liquidRibGenData::~liquidRibGenData()
//
//  Description:
//      class destructor
//
{
    if ( debugMode ) { printf("-> killing ribgen\n"); }
		lfree( ribStatus.objectName ); 
		ribStatus.objectName = NULL; 
}

void liquidRibGenData::write()
{
	if ( debugMode ) { printf("-> writing ribgen\n"); }
#ifdef PRMAN
#ifndef _WIN32
	void *handle;
#else 
	HINSTANCE handle;
#endif
	char *dlStatus = NULL;
	// Hmmmmmm do not really understand what's going on here 
	ribStatus.ribFP = liqglo_ribFP;
	ribStatus.frame = liqglo_lframe;
	if ( liqglo_currentJob.isShadow ) {
		ribStatus.renderPass = liquidRIBStatus::rpShadow;
	} else {
		ribStatus.renderPass = liquidRIBStatus::rpFinal;
	}
	ribStatus.transBlur = liqglo_doMotion;
	ribStatus.defBlur = liqglo_doDef;
	ribStatus.compressed = liqglo_doCompression;
	ribStatus.binary = liqglo_doBinary;
	liqglo_currentJob.camera[0].mat.get( ribStatus.cameraMatrix );
	ribStatus.sampleTimes = liqglo_sampleTimes;
	if ( liqglo_doMotion || liqglo_doDef ) {
		if ( !liqglo_currentJob.isShadow ) {
			ribStatus.motionSamples = liqglo_motionSamples;
		} else {
			ribStatus.motionSamples = 1;
		}
	} else {
		ribStatus.motionSamples = 1;
	}		
	ribStatus.shutterAngle = liqglo_shutterTime;
    
	/*
	* rib stream connection call from the Affine toolkit.
	* until an equivilent is found in prman leave commented out. 
	* dan-b 7/2/03
	*/
	//ribStatus.RiConnection = RiDetach();
	
	typedef liquidRIBGen *(*createRibGen)();
	typedef void (*destroyRibGen)( liquidRIBGen * );
	
	createRibGen		createRibGenFunc;
	destroyRibGen		destroyRibGenFunc;
	
#ifndef _WIN32
	handle = dlopen( ribGenSoName.asChar(), RTLD_LAZY );
	dlStatus = dlerror();
	if ( dlStatus == NULL ) {
		createRibGenFunc = (createRibGen)dlsym( handle, "RIBGenCreate" );
		destroyRibGenFunc = (destroyRibGen)dlsym( handle, "RIBGenDestroy" );
		dlStatus = dlerror();
		if ( dlStatus == NULL ) {
#else
	handle = LoadLibrary( ribGenSoName.asChar() );
	if ( handle != NULL ) {
		createRibGenFunc = (createRibGen)GetProcAddress( handle, "RIBGenCreate" );
		destroyRibGenFunc = (destroyRibGen)GetProcAddress( handle, "RIBGenDestroy" );
		if ( ( createRibGenFunc != NULL ) && ( destroyRibGenFunc != NULL ) ) {
#endif
			liquidRIBGen *ribGen = (*createRibGenFunc)();
			int i = ribGen->_GenRIB( &ribStatus );
			(*destroyRibGenFunc)( ribGen );
		} else {
			MString errorString = "Error reading RIBGenCreate or RIBGenDestroy in RibGen: ";
			errorString += ribGenSoName;
			liquidInfo( errorString );
		}
#ifndef _WIN32
		dlclose( handle );
#else
		FreeLibrary( handle );
#endif
	} else {
		MString errorString = "Error opening RibGen: ";
		errorString += ribGenSoName;
		errorString += " ";
		errorString += dlStatus;
		errorString += " on object: ";
		errorString += ribStatus.objectName;
		liquidInfo( errorString );
	}

	/*
	* rib stream connection call from the Affine toolkit.
	* until an equivilent is found in prman leave commented out. 
	* dan-b 7/2/03
	*/
	// RiFreeConnection( ( char * )ribStatus.RiConnection );
	
#else
    liquidInfo( "Sorry : Can't handle Rib Gen ...\n" );
#endif
}
		
bool liquidRibGenData::compare( const liquidRibData & otherObj ) const
//
//  Description:
//      Compare this ribgen to the other for the purpose of determining
//      if its animated
//
{
	if ( debugMode ) { printf("-> comparing RibGen\n"); }
	if ( otherObj.type() != MRT_RibGen ) return false;
	return true;
}
		
ObjectType liquidRibGenData::type() const
//
//  Description:
//      return the geometry type
//
{
	if ( debugMode ) { printf("-> returning RibGen type\n"); }
	return MRT_RibGen;
}
