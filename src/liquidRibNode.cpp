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
** Liquid Rib Node Source 
** ______________________________________________________________________
*/

// Standard Headers
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

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#include <stdlib.h>
#include <alloca.h>
#endif

// Maya's Headers
#include <maya/MPlug.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MColor.h>
#include <maya/MFnPhongShader.h>
#include <maya/MFnBlinnShader.h>
#include <maya/MFnLambertShader.h>
#include <maya/MPlugArray.h>
#include <maya/MObjectArray.h>
#include <maya/MFnSet.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDagNode.h>
#include <maya/MBoundingBox.h>
#include <maya/MFnAttribute.h>

#include <liquid.h>
#include <liquidGlobalHelpers.h>
#include <liquidRibNode.h>
#include <liquidMemory.h>

extern int debugMode;
extern MStringArray preReadArchive;
extern MStringArray preRibBox;
extern MStringArray preReadArchiveShadow;
extern MStringArray preRibBoxShadow;

RibNode::RibNode( RibNode * instanceOfNode )
// 
//  Description:
//      construct a new hash table entry
//
:   next( NULL ),
matXForm( MRX_Const ),
bodyXForm( MRX_Const ),
instance( instanceOfNode )
{
	if ( debugMode ) { printf("-> creating rib node\n"); }
	objects[0] = NULL;
	objects[1] = NULL;
	objects[2] = NULL;
	objects[3] = NULL;
	objects[4] = NULL;
	name.clear();
	isRibBox = false;
	isArchive = false;
	isDelayedArchive = false;
	matteMode = false;
	doDef = true;
	doMotion = true;
	nodeShadingRateSet = false;
	nodeShadingRate = 0.0;
}

RibNode::~RibNode()
// 
//  Description:
//      class destructor
//
{
    if ( debugMode ) { printf("-> killing rib node %s\n", name.asChar() ); }
    if ( debugMode ) { printf("-> killing first ref\n" ); }
    if (objects[0] != NULL) { objects[0]->unref(); objects[0] = NULL; }
    if ( debugMode ) { printf("-> killing second ref\n" ); }
    if (objects[1] != NULL) { objects[1]->unref(); objects[1] = NULL; }
    if ( debugMode ) { printf("-> killing third ref\n" ); }
    if (objects[2] != NULL) { objects[2]->unref(); objects[2] = NULL; }
    if ( debugMode ) { printf("-> killing fourth ref\n" ); }
    if (objects[3] != NULL) { objects[3]->unref(); objects[3] = NULL; }
    if ( debugMode ) { printf("-> killing fifth ref\n" ); }
    if (objects[4] != NULL) { objects[4]->unref(); objects[4] = NULL; }
    if ( debugMode ) { printf("-> killing no obj\n" ); }
    no = NULL;
	name.clear();
	ribBoxString.clear();
	archiveString.clear();
	delayedArchiveString.clear();
    if ( debugMode ) { printf("-> finished killing rib node.\n" ); }
}	

RibObj * RibNode::object(int interval)
// 
//  Description:
//      get the object (surface, mesh, light, etc) refered to by this node
//
{
    return objects[interval];
}

void RibNode::set( MDagPath &path, int sample, ObjectType objType )
// 
//  Description:
//      set this node with the given path.  If this node already refers to
//      given object, then it is assumed that the path represents the object
//      at the next frame.
//
{
	if ( debugMode ) { printf("-> setting rib node\n"); }
	DagPath = path;
	int instanceNum = path.instanceNumber();
	
	isRibBox = false;
	isArchive = false;
	isDelayedArchive = false;
	MStatus status;
	MFnDagNode fnNode( path );
	MPlug nPlug;
	status.clear();
	nPlug = fnNode.findPlug( MString( "transformationBlur" ), &status );
	if ( status == MS::kSuccess ) {
		nPlug.getValue( doMotion );
	}
	status.clear();
	nPlug = fnNode.findPlug( MString( "deformationBlur" ), &status );
	if ( status == MS::kSuccess ) {
		nPlug.getValue( doDef );
	}
	status.clear();
	nPlug = fnNode.findPlug( MString( "shadingRate" ), &status );
	if ( status == MS::kSuccess ) {
		nodeShadingRateSet = true;
		nPlug.getValue( nodeShadingRate );
	}
	status.clear();
	nPlug = fnNode.findPlug( MString( "ribBox" ), &status );
	if ( status == MS::kSuccess ) {
		MString ribBoxValue;
		nPlug.getValue( ribBoxValue );
		if ( ribBoxValue.substring(0,2) == "*H*" ) {
			MString parseThis = ribBoxValue.substring(3, ribBoxValue.length() - 1 );
			preRibBox.append( parseString( parseThis ) );
		} else if ( ribBoxValue.substring(0,3) == "*SH*" ) {
			MString parseThis = ribBoxValue.substring(3, ribBoxValue.length() - 1 );
			preRibBoxShadow.append( parseString( parseThis ) );
		} else { 
			ribBoxString = parseString( ribBoxValue );
			isRibBox = true;
		}
	}
	status.clear();
	nPlug = fnNode.findPlug( MString( "ribArchive" ), &status );
	if ( status == MS::kSuccess ) {
		MString archiveValue;
		nPlug.getValue( archiveValue );
		if ( archiveValue.substring(0,2) == "*H*" ) {
			MString parseThis = archiveValue.substring(3, archiveValue.length() - 1 );
			preReadArchive.append( parseString( parseThis ) );
		} else if ( archiveValue.substring(0,3) == "*SH*" ) {
			MString parseThis = archiveValue.substring(3, archiveValue.length() - 1 );
			preReadArchiveShadow.append( parseString( parseThis ) );
		} else {			
			archiveString = parseString( archiveValue );
			isArchive = true;
		}
	}
	status.clear();
	nPlug = fnNode.findPlug( MString( "ribDelayedArchive" ), &status );
	if ( status == MS::kSuccess ) {
		MString delayedArchiveValue;
		nPlug.getValue( delayedArchiveValue );
		delayedArchiveString = parseString( delayedArchiveValue );
		isDelayedArchive = true;

		MStatus Dstatus;
		MPlug delayedPlug = fnNode.findPlug( MString( "ribDelayedArchiveBBox" ), &Dstatus );
		if ( ( Dstatus == MS::kSuccess ) && ( delayedPlug.isConnected() ) ) {
			MPlugArray delayedNodeArray;
			delayedPlug.connectedTo( delayedNodeArray, true, true );
			MObject delayedNodeObj;
			delayedNodeObj = delayedNodeArray[0].node();
			MFnDagNode delayedfnNode( delayedNodeObj );

			MBoundingBox bounding = delayedfnNode.boundingBox();
			MPoint bMin = bounding.min();
			MPoint bMax = bounding.max();
			bound[0] = bMin.x;
			bound[1] = bMin.y;
			bound[2] = bMin.z;
			bound[3] = bMax.x;
			bound[4] = bMax.y;
			bound[5] = bMax.z;
		}
	}
	
	// Get the object's color
	//	  
	if ( objType != MRT_Shader ) {
		MObject shadingGroup = findShadingGroup( path );
		if ( shadingGroup != MObject::kNullObj ) {
			assignedShadingGroup.setObject( shadingGroup );
			MObject surfaceShader = findShader( shadingGroup );
			assignedShader.setObject( surfaceShader );
			assignedDisp.setObject( findDisp( shadingGroup ) ); 
			assignedVolume.setObject( findVolume( shadingGroup ) ); 
			if ( ( surfaceShader == MObject::kNullObj ) || !getColor( surfaceShader, color ) ) {
				// This is how we specify that the color was not found.
				//
				color.r = -1.0;
			}
			matteMode = getMatteMode( surfaceShader );
		} else {
			color.r = -1.0;
		}
		doubleSided = isObjectTwoSided( path );
	}
	
	// Create a new RIB object for the given path
	//
	if ( debugMode ) { printf("-> creating rib object for given path\n"); }
	
	MObject obj = path.node();
	no = new RibObj( path, objType );
	if ( debugMode ) { printf("-> creating rib object for reference\n"); }
	no->ref();
	
	
	if ( debugMode ) { printf("-> getting objects name\n"); }
	name = path.fullPathName();
	
	if (name == "") {
		if ( debugMode ) { printf("-> name unknown - searching for name\n"); }
		MDagPath		parentPath = path;
		parentPath.pop();
		MStatus     returnStatus;
		
		name = parentPath.fullPathName(&returnStatus);
		if ( debugMode ) { printf("-> found name\n"); }
	}
	if ( objType == MRT_RibGen ) name += "RIBGEN";
	
	if ( debugMode ) { printf("-> inserting object into ribnode's obj sample table\n"); }
	if (objects[sample] == NULL) {
		objects[sample] = no;
	}
	if ( debugMode ) { printf("-> done creating rib object for given path\n"); }
}

MDagPath & RibNode::path()
//
//  Description:
//      Return the path in the DAG to the instance that this node represents
//
{
	return DagPath;   
}

MObject RibNode::findShadingGroup( const MDagPath& path )
//
//  Description:
//      Find the shading group assigned to the given object
//
{
	if ( debugMode ) { printf("-> finding rib node shading group\n"); }
	MSelectionList objects;
	objects.add( path );
	MObjectArray setArray;
    
    // Get all of the sets that this object belongs to
    //
	MGlobal::getAssociatedSets( objects, setArray );
	MObject mobj;	
	
    // Look for a set that is a "shading group"
    //
	for ( int i=0; i<setArray.length(); i++ ) 
	{
		mobj = setArray[i];
		MFnSet fnSet( mobj ); 
		MStatus stat;
		if ( MFnSet::kRenderableOnly == fnSet.restriction(&stat) )
		{
			return mobj;		
		}
	}
	return MObject::kNullObj;
}

MObject RibNode::findShader( MObject& group )
//
//  Description:
//      Find the shading node for the given shading group
//
{
	if ( debugMode ) { printf("-> finding shader for rib node shading group\n"); }
	MFnDependencyNode fnNode( group );
	MPlug shaderPlug = fnNode.findPlug( "surfaceShader" );
	
	if ( !shaderPlug.isNull() ) {			
		MPlugArray connectedPlugs;
		bool asSrc = false;
		bool asDst = true;
		shaderPlug.connectedTo( connectedPlugs, asDst, asSrc );
		if ( connectedPlugs.length() != 1 ) {
			//cerr << "Error getting shader\n";
		}
		else 
			return connectedPlugs[0].node();
	}			
	
	return MObject::kNullObj;
}

MObject RibNode::findDisp( MObject& group )
//
//  Description:
//      Find the shading node for the given shading group
//
{
	if ( debugMode ) { printf("-> finding shader for rib node shading group\n"); }
	MFnDependencyNode fnNode( group );
	MPlug shaderPlug = fnNode.findPlug( "displacementShader" );
	
	if ( !shaderPlug.isNull() ) {			
		MPlugArray connectedPlugs;
		bool asSrc = false;
		bool asDst = true;
		shaderPlug.connectedTo( connectedPlugs, asDst, asSrc );
		if ( connectedPlugs.length() != 1 ) {
			//cerr << "Error getting shader\n";
		}
		else 
			return connectedPlugs[0].node();
	}			
	
	return MObject::kNullObj;
}

MObject RibNode::findVolume( MObject& group )
//
//  Description:
//      Find the shading node for the given shading group
//
{
	if ( debugMode ) { printf("-> finding shader for rib node shading group\n"); }
	MFnDependencyNode fnNode( group );
	MPlug shaderPlug = fnNode.findPlug( "volumeShader" );
	
	if ( !shaderPlug.isNull() ) {			
		MPlugArray connectedPlugs;
		bool asSrc = false;
		bool asDst = true;
		shaderPlug.connectedTo( connectedPlugs, asDst, asSrc );
		if ( connectedPlugs.length() != 1 ) {
			//cerr << "Error getting shader\n";
		}
		else 
			return connectedPlugs[0].node();
	}			
	
	return MObject::kNullObj;
}

void RibNode::getIgnoredLights( MObject& group, MObjectArray& ignoredLights )
//
//  Description:
//      Get the list of all ignored lights for the given shading group
//
{
	if ( debugMode ) { printf("-> getting ignored lights\n"); }
	MFnDependencyNode fnNode( group );
	MPlug ilPlug = fnNode.findPlug( "ignoredLights" );
	
	if ( !ilPlug.isNull() ) {
		MPlugArray connectedPlugs;
		bool asSrc = false;
		bool asDst = true;
		
		// The ignoredLights attribute is an array so this should
		// never happen
		//
		if ( !ilPlug.isArray() )
			return;
		
		for ( unsigned i=0; i<ilPlug.numConnectedElements(); i++ )
		{
			MPlug elemPlug = ilPlug.elementByPhysicalIndex( i );
			connectedPlugs.clear();
			elemPlug.connectedTo( connectedPlugs, asDst, asSrc );
			
			// Since elemPlug is a destination there should
			// only be 1 connection to it
			//
			ignoredLights.append( connectedPlugs[0].node() );
		}
	}			
}

void RibNode::getIgnoredLights( MObjectArray& ignoredLights )
//
//  Description:
//      Get the list of all ignored lights for the given for *this* node
//
{
	MStatus status;
	if ( debugMode ) { printf("-> getting ignored lights\n"); }
	MFnDependencyNode fnNode( DagPath.node() );
	MPlug msgPlug = fnNode.findPlug( "message", &status );
	
	if ( status != MS::kSuccess ) return;

	MPlugArray llPlugs;
	msgPlug.connectedTo(llPlugs, true, true);

	for ( unsigned i=0; i<llPlugs.length(); i++ )
	{
		MPlug llPlug = llPlugs[i];
		MObject llPlugAttr = llPlug.attribute();
		MFnAttribute llPlugAttrFn(llPlugAttr);

		if (llPlugAttrFn.name() == MString( "objectIgnored" ))
		{
			MPlug llParentPlug = llPlug.parent(&status);
			int numChildren  = llParentPlug.numChildren();

			for (int k=0; k<numChildren; k++)
			{
				MPlug   childPlug  = llParentPlug.child(k);
				MObject llChildAttr = childPlug.attribute();
				MFnAttribute llChildAttrFn(llChildAttr);

				if (llChildAttrFn.name() == MString( "lightIgnored" ))
				{
					MPlugArray connectedPlugs;
					childPlug.connectedTo(connectedPlugs,true,true);
					if ( connectedPlugs[0].node().hasFn( MFn::kSet ) ) {
						MStatus setStatus;
						MFnDependencyNode listSetNode( connectedPlugs[0].node() );
						MPlug setPlug = fnNode.findPlug( "dagSetMembers", &setStatus );
						if ( setStatus == MS::kSuccess ) {
							MPlugArray setConnectedPlugs;
							setPlug.connectedTo(setConnectedPlugs,true,true);
							ignoredLights.append( setConnectedPlugs[0].node() );
						}
					} else { 
						ignoredLights.append( connectedPlugs[0].node() );
					}
				}
			}
		}
	}	
}


bool RibNode::getColor( MObject& shader, MColor& color )
//
//  Description:
//      Get the color of the given shading node.
//
{
	if ( debugMode ) { printf("-> getting a shader color\n"); }
	switch ( 
		shader.apiType() )
	{ 
	case MFn::kLambert :
		{			
			MFnLambertShader fnShader( shader );
			color = fnShader.color();
			break;
		}
	case MFn::kBlinn :
		{
			MFnBlinnShader fnShader( shader );
			color = fnShader.color();
			break;
		}
	case MFn::kPhong :
		{
			MFnPhongShader fnShader( shader );
			color = fnShader.color();
			break;	
		}
	default:
		{
			MFnDependencyNode fnNode( shader );
			MPlug colorPlug = fnNode.findPlug( "outColor" );
			colorPlug[0].getValue( color.r );
			colorPlug[1].getValue( color.g );
			colorPlug[2].getValue( color.b );
			return false;			 	
		}
	}
	return true;
}	

bool RibNode::getMatteMode( MObject& shader )
//
//  Description:
//      check to see if we should make this a matte object.
//
{
	MObject matteModeObj;
	short matteModeInt;
	MStatus myStatus;
	if ( debugMode ) { printf("-> getting matte mode\n"); }
	if ( !shader.isNull() ) {
		MFnDependencyNode fnNode( shader );
		MPlug mattePlug = fnNode.findPlug( "matteOpacityMode", &myStatus );
		if ( myStatus == MS::kSuccess ) {
			mattePlug.getValue( matteModeInt );
			if ( debugMode ) { printf( "-> matte mode: %d \n", matteModeInt ); }
			if ( matteModeInt == 0 ) {
				return true;
			}
		}
	}
	return false;
}

