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
** Liquid RibData Soruce File
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
#include <maya/MFn.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFloatArray.h>
#include <maya/MIntArray.h>
#include <maya/MPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MString.h>
#include <maya/MVector.h>
#include <maya/MFloatVector.h>
#include <maya/MColor.h>
#include <maya/MFnStringData.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MVectorArray.h>
#include <maya/MStringArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>

#include <liquid.h>
#include <liquidGlobalHelpers.h>
#include <liquidRibData.h>
#include <liquidMemory.h>

extern int debugMode;

liquidRibData::~liquidRibData() 
{
    // clean up and additional data
    LIQDEBUGPRINTF("-> freeing additional ribdata: " );
    LIQDEBUGPRINTF(objDagPath.fullPathName().asChar());
    LIQDEBUGPRINTF("\n" );
    // Class destructor should be called
#if 0	
	std::vector<rTokenPointer>::iterator iter = tokenPointerArray.begin();
	while ( iter != tokenPointerArray.end() )
	{
		if ( debugMode ) { printf("-> freeing addition ribdata: %s\n", iter->tokenName ); }
		if ( iter->tokenFloats != NULL ) {
			lfree( iter->tokenFloats );
			iter->tokenFloats = NULL;
		}
		if ( iter->tokenString != NULL ) {
			lfree( iter->tokenString );
			iter->tokenString = NULL;
		}
		++iter;
	}
#endif
    tokenPointerArray.clear();
    LIQDEBUGPRINTF("-> finished freeing additional ribdata: " );
    LIQDEBUGPRINTF(objDagPath.fullPathName().asChar());
    LIQDEBUGPRINTF("\n" );
}

void liquidRibData::parseVectorAttributes( MFnDependencyNode & nodeFn, MStringArray & strArray, ParameterType pType )
{
    int i;
    MStatus status;
    if ( strArray.length() > 0 ) {
	for ( i = 0; i < strArray.length(); i++ ) {
	    liqTokenPointer tokenPointerPair;
	    MString cutString = strArray[i].substring(5, strArray[i].length());
	    MPlug vPlug = nodeFn.findPlug( strArray[i] );
	    MObject plugObj;
	    status = vPlug.getValue( plugObj );
	    if ( plugObj.apiType() == MFn::kVectorArrayData ) 
	    {
		MFnVectorArrayData  fnVectorArrayData( plugObj );
		MVectorArray vectorArrayData = fnVectorArrayData.array( &status );
		tokenPointerPair.set( 	cutString.asChar(), 
		    	    	    	pType, 
					( type() == MRT_Nurbs || type() == MRT_NuCurve ) ? true : false,
					true,
					false,
					vectorArrayData.length() );
		for ( int kk = 0; kk < vectorArrayData.length(); kk++ )
		{
    	    	    tokenPointerPair.setTokenFloat( kk, vectorArrayData[kk].x, vectorArrayData[kk].y, vectorArrayData[kk].z );
		}
		tokenPointerPair.setDetailType( rVertex );
		tokenPointerArray.push_back( tokenPointerPair );
	    } else {
    	    	// Hmmmm float ? double ?
		float x, y, z;
		tokenPointerPair.set( 	cutString.asChar(), 
		    	    	    	pType, 
					( type() == MRT_Nurbs || type() == MRT_NuCurve ) ? true : false,
					false,
					false,
					0 );
		vPlug.child(0).getValue( x );
		vPlug.child(1).getValue( y );
		vPlug.child(2).getValue( z );
		tokenPointerPair.setTokenFloat( 0, x, y, z );
		tokenPointerPair.setDetailType( rConstant );
		tokenPointerArray.push_back( tokenPointerPair );
	    }
	}
    }
}

void liquidRibData::addAdditionalSurfaceParameters( MObject node )
{
	
    LIQDEBUGPRINTF("-> scanning for additional rman surface attributes \n");
    MStatus status = MS::kSuccess;
    unsigned i;

    /* work out how many elements there would be in a facevarying array if a mesh or subD*/
    unsigned faceVaryingCount = 0;
    if ( ( type() == MRT_Mesh ) || ( type() == MRT_Subdivision ) ) {
    	MFnMesh fnMesh( node );
	for ( uint pOn = 0; pOn < fnMesh.numPolygons(); pOn++ )
	{
    	    faceVaryingCount += fnMesh.polygonVertexCount( pOn );
	}
    }
	
    // find how many additional
    MFnDependencyNode nodeFn( node );
	
    // find the attributes
    MStringArray floatAttributesFound = FindAttributesByPrefix( "rmanF", nodeFn );
    MStringArray pointAttributesFound = FindAttributesByPrefix( "rmanP", nodeFn );
    MStringArray vectorAttributesFound = FindAttributesByPrefix( "rmanV", nodeFn );
    MStringArray normalAttributesFound = FindAttributesByPrefix( "rmanN", nodeFn );
    MStringArray colorAttributesFound = FindAttributesByPrefix( "rmanC", nodeFn );
    MStringArray stringAttributesFound = FindAttributesByPrefix( "rmanS", nodeFn );

    if ( floatAttributesFound.length() > 0 ) {
	for ( i = 0; i < floatAttributesFound.length(); i++ ) {
	    liqTokenPointer tokenPointerPair;
	    MString cutString = floatAttributesFound[i].substring(5, floatAttributesFound[i].length());
	    MPlug fPlug = nodeFn.findPlug( floatAttributesFound[i] );
	    MObject plugObj;
	    status = fPlug.getValue( plugObj );
	    if ( plugObj.apiType() == MFn::kDoubleArrayData ) 
	    {
		MFnDoubleArrayData  fnDoubleArrayData( plugObj );
		MDoubleArray doubleArrayData = fnDoubleArrayData.array( &status );
		tokenPointerPair.set( 	cutString.asChar(), 
		    	    	    	rFloat, 
					( type() == MRT_Nurbs || type() == MRT_NuCurve ) ? true : false,
					true,
					false,
					doubleArrayData.length() );
		for( unsigned int kk = 0; kk < doubleArrayData.length(); kk++ ) {
		    tokenPointerPair.setTokenFloat( kk, doubleArrayData[kk] );
		}
		if ( ( type() == MRT_NuCurve ) && ( cutString == MString( "width" ) ) ) {
			tokenPointerPair.setDetailType( rVarying);
		} else if ( ( ( type() == MRT_Mesh ) || ( type() == MRT_Subdivision ) ) && ( doubleArrayData.length() == faceVaryingCount ) ) {
			tokenPointerPair.setDetailType( rFaceVarying);
		} else {
			tokenPointerPair.setDetailType( rVertex );
		}
	    } else {
		float floatValue;
		tokenPointerPair.set( 	cutString.asChar(), 
		    	    	    	rFloat, 
					( type() == MRT_Nurbs || type() == MRT_NuCurve ) ? true : false,
					false,
					false,
					0 );
		fPlug.getValue( floatValue );
		tokenPointerPair.setTokenFloat( 0, floatValue );
		tokenPointerPair.setDetailType( rConstant );
	    }
	    tokenPointerArray.push_back( tokenPointerPair );
	}
    }
    if ( pointAttributesFound.length() > 0 ) {
	for ( i = 0; i < pointAttributesFound.length(); i++ ) {
	    liqTokenPointer tokenPointerPair;
	    MString cutString = pointAttributesFound[i].substring(5, pointAttributesFound[i].length());
	    MPlug pPlug = nodeFn.findPlug( pointAttributesFound[i] );
	    MObject plugObj;
	    status = pPlug.getValue( plugObj );
	    if ( plugObj.apiType() == MFn::kPointArrayData ) 
	    {
		MFnPointArrayData  fnPointArrayData( plugObj );
		MPointArray pointArrayData = fnPointArrayData.array( &status );
		tokenPointerPair.set( 	cutString.asChar(), 
		    	    	    	rPoint, 
					( type() == MRT_Nurbs || type() == MRT_NuCurve ) ? true : false,
					true,
					false,
					pointArrayData.length() );
		if ( type() == MRT_Nurbs || type() == MRT_NuCurve ) {
		    for ( int kk = 0; kk < pointArrayData.length(); kk++ )
		    {
    	    	    	tokenPointerPair.setTokenFloat( kk, pointArrayData[kk].x, pointArrayData[kk].y, pointArrayData[kk].z, pointArrayData[kk].w );
		    }
		} else {
		    for ( int kk = 0; kk < pointArrayData.length(); kk++ )
		    {
    	    	    	tokenPointerPair.setTokenFloat( kk, pointArrayData[kk].x, pointArrayData[kk].y, pointArrayData[kk].z );
		    }
		}	
		tokenPointerPair.setDetailType( rVertex );
	    } else {
		// Hmmmm float ? double ?
		float x, y, z;
		tokenPointerPair.set( 	cutString.asChar(), 
		    	    	    	rPoint, 
					( type() == MRT_Nurbs || type() == MRT_NuCurve ) ? true : false,
					false,
					false,
					0 );
		// Hmmm should check as for arrays if we are in nurbs mode : 4 values
		pPlug.child(0).getValue( x );
		pPlug.child(1).getValue( y );
		pPlug.child(2).getValue( z );
    	    	tokenPointerPair.setTokenFloat( 0, x, y, z );
		tokenPointerPair.setDetailType( rConstant );
	    }
	    tokenPointerArray.push_back( tokenPointerPair );
	}
    }
    parseVectorAttributes( nodeFn, vectorAttributesFound, rVector );
    parseVectorAttributes( nodeFn, normalAttributesFound, rNormal );
    parseVectorAttributes( nodeFn, colorAttributesFound, rColor );
	
    if ( stringAttributesFound.length() > 0 ) {
	for ( i = 0; i < stringAttributesFound.length(); i++ ) {
	    liqTokenPointer tokenPointerPair;
	    MString cutString = stringAttributesFound[i].substring(5, stringAttributesFound[i].length());
	    MPlug sPlug = nodeFn.findPlug( stringAttributesFound[i] );
	    MObject plugObj;
	    status = sPlug.getValue( plugObj );
	    tokenPointerPair.set(   cutString.asChar(), 
		    	    	    rString, 
				    ( type() == MRT_Nurbs || type() == MRT_NuCurve ) ? true : false,
				    false,
				    false,
				    0 );
	    MString stringVal;
	    sPlug.getValue( stringVal );
	    tokenPointerPair.setTokenString( stringVal.asChar(), stringVal.length() );
    	    tokenPointerPair.setDetailType( rConstant );
	    tokenPointerArray.push_back( tokenPointerPair );
	}
    }
}