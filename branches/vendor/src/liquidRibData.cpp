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

RibData::~RibData() 
{
	// clean up and additional data
	if ( debugMode ) { printf("-> freeing addition ribdata.\n" ); }
	int k = 0;
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
	tokenPointerArray.clear();
	if ( debugMode ) { printf("-> finished freeing addition ribdata.\n" ); }
}

void RibData::addAdditionalSurfaceParameters( MObject node )
{
	
	if ( debugMode ) { printf("-> scanning for additional rman surface attributes \n"); }
	
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
			rTokenPointer tokenPointerPair;
			MString cutString = floatAttributesFound[i].substring(5, floatAttributesFound[i].length());
			sprintf( tokenPointerPair.tokenName, cutString.asChar() ) ;
			MPlug fPlug = nodeFn.findPlug( floatAttributesFound[i] );
			MObject plugObj;
			status = fPlug.getValue( plugObj );
			tokenPointerPair.pType = rFloat;
			if ( type() == MRT_Nurbs || type() == MRT_NuCurve ) {
				tokenPointerPair.isNurbs = true;
			} else {
				tokenPointerPair.isNurbs = false;
			}
			if ( plugObj.apiType() == MFn::kDoubleArrayData ) 
			{
				MFnDoubleArrayData  fnDoubleArrayData( plugObj );
				MDoubleArray doubleArrayData = fnDoubleArrayData.array( &status );
				tokenPointerPair.arraySize = doubleArrayData.length();
				tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * ( tokenPointerPair.arraySize ) );
				doubleArrayData.get( tokenPointerPair.tokenFloats );
				tokenPointerPair.isArray = true;
				tokenPointerPair.isUArray = false;
				if ( ( type() == MRT_NuCurve ) && ( cutString == MString( "width" ) ) ) {
					tokenPointerPair.dType = rVarying;
				} else if ( ( ( type() == MRT_Mesh ) || ( type() == MRT_Subdivision ) ) && ( doubleArrayData.length() == faceVaryingCount ) ) {
					tokenPointerPair.dType = rFaceVarying;
				} else {
					tokenPointerPair.dType = rVertex;
				}
				tokenPointerArray.push_back( tokenPointerPair );
			} else {
				float floatValue;
				tokenPointerPair.arraySize = 0;
				tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 1 );
				fPlug.getValue( floatValue );
				tokenPointerPair.tokenFloats[0]  = floatValue ;
				tokenPointerPair.isArray = false;
				tokenPointerPair.isUArray = false;
				tokenPointerPair.dType = rConstant;
				tokenPointerArray.push_back( tokenPointerPair );
			}
		}
	}
	if ( pointAttributesFound.length() > 0 ) {
		for ( i = 0; i < pointAttributesFound.length(); i++ ) {
			rTokenPointer tokenPointerPair;
			MString cutString = pointAttributesFound[i].substring(5, pointAttributesFound[i].length());
			sprintf( tokenPointerPair.tokenName, cutString.asChar() ) ;
			MPlug pPlug = nodeFn.findPlug( pointAttributesFound[i] );
			MObject plugObj;
			status = pPlug.getValue( plugObj );
			tokenPointerPair.pType = rPoint;
			if ( type() == MRT_Nurbs || type() == MRT_NuCurve ) {
				tokenPointerPair.isNurbs = true;
			} else {
				tokenPointerPair.isNurbs = false;
			}
			tokenPointerPair.isUArray = false;
			if ( plugObj.apiType() == MFn::kPointArrayData ) 
			{
				MFnPointArrayData  fnPointArrayData( plugObj );
				MPointArray pointArrayData = fnPointArrayData.array( &status );
				tokenPointerPair.arraySize = pointArrayData.length();
				if ( type() == MRT_Nurbs || type() == MRT_NuCurve ) {
					tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * ( tokenPointerPair.arraySize * 4 ) );
					for ( int kk = 0; kk < pointArrayData.length(); kk++ )
					{
						tokenPointerPair.tokenFloats[kk * 4] = pointArrayData[kk].x; 
						tokenPointerPair.tokenFloats[kk * 4 + 1] = pointArrayData[kk].y; 
						tokenPointerPair.tokenFloats[kk * 4 + 2] = pointArrayData[kk].z; 
						tokenPointerPair.tokenFloats[kk * 4 + 3] = pointArrayData[kk].w; 
					}
				} else {
					tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * ( tokenPointerPair.arraySize * 3 ) );
					for ( int kk = 0; kk < pointArrayData.length(); kk++ )
					{
						tokenPointerPair.tokenFloats[kk * 3] = pointArrayData[kk].x; 
						tokenPointerPair.tokenFloats[kk * 3 + 1] = pointArrayData[kk].y; 
						tokenPointerPair.tokenFloats[kk * 3 + 2] = pointArrayData[kk].z; 
					}
				}	
				tokenPointerPair.isArray = true;
				tokenPointerPair.dType = rVertex;
				tokenPointerArray.push_back( tokenPointerPair );
			} else {
				tokenPointerPair.arraySize = 0;
				tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3 );
				pPlug.child(0).getValue( tokenPointerPair.tokenFloats[0] );
				pPlug.child(1).getValue( tokenPointerPair.tokenFloats[1] );
				pPlug.child(2).getValue( tokenPointerPair.tokenFloats[2] );
				tokenPointerPair.isArray = false;
				tokenPointerPair.dType = rConstant;
				tokenPointerArray.push_back( tokenPointerPair );
			}
		}
	}
	if ( vectorAttributesFound.length() > 0 ) {
		for ( i = 0; i < vectorAttributesFound.length(); i++ ) {
			rTokenPointer tokenPointerPair;
			MString cutString = vectorAttributesFound[i].substring(5, vectorAttributesFound[i].length());
			sprintf( tokenPointerPair.tokenName, cutString.asChar() ) ;
			MPlug vPlug = nodeFn.findPlug( vectorAttributesFound[i] );
			MObject plugObj;
			status = vPlug.getValue( plugObj );
			tokenPointerPair.pType = rVector;
			if ( type() == MRT_Nurbs || type() == MRT_NuCurve ) {
				tokenPointerPair.isNurbs = true;
			} else {
				tokenPointerPair.isNurbs = false;
			}
			tokenPointerPair.isUArray = false;
			if ( plugObj.apiType() == MFn::kVectorArrayData ) 
			{
				MFnVectorArrayData  fnVectorArrayData( plugObj );
				MVectorArray vectorArrayData = fnVectorArrayData.array( &status );
				tokenPointerPair.arraySize = vectorArrayData.length();
				tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * ( tokenPointerPair.arraySize * 3 ) );
				for ( int kk = 0; kk < vectorArrayData.length(); kk++ )
				{
					tokenPointerPair.tokenFloats[kk * 3] = vectorArrayData[kk].x; 
					tokenPointerPair.tokenFloats[kk * 3 + 1] = vectorArrayData[kk].y; 
					tokenPointerPair.tokenFloats[kk * 3 + 2] = vectorArrayData[kk].z; 
				}
				tokenPointerPair.isArray = true;
				tokenPointerPair.dType = rVertex;
				tokenPointerArray.push_back( tokenPointerPair );
			} else {
				tokenPointerPair.arraySize = 0;
				tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3 );
				vPlug.child(0).getValue( tokenPointerPair.tokenFloats[0] );
				vPlug.child(1).getValue( tokenPointerPair.tokenFloats[1] );
				vPlug.child(2).getValue( tokenPointerPair.tokenFloats[2] );
				tokenPointerPair.isArray = false;
				tokenPointerPair.dType = rConstant;
				tokenPointerArray.push_back( tokenPointerPair );
			}
		}
	}
	if ( normalAttributesFound.length() > 0 ) {
		for ( i = 0; i < normalAttributesFound.length(); i++ ) {
			rTokenPointer tokenPointerPair;
			MString cutString = normalAttributesFound[i].substring(5, normalAttributesFound[i].length());
			sprintf( tokenPointerPair.tokenName, cutString.asChar() ) ;
			MPlug nPlug = nodeFn.findPlug( normalAttributesFound[i] );
			MObject plugObj;
			status = nPlug.getValue( plugObj );
			tokenPointerPair.pType = rNormal;
			if ( type() == MRT_Nurbs || type() == MRT_NuCurve ) {
				tokenPointerPair.isNurbs = true;
			} else {
				tokenPointerPair.isNurbs = false;
			}
			if ( plugObj.apiType() == MFn::kVectorArrayData ) 
			{
				MFnVectorArrayData  fnVectorArrayData( plugObj );
				MVectorArray vectorArrayData = fnVectorArrayData.array( &status );
				tokenPointerPair.arraySize = vectorArrayData.length();
				tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * ( tokenPointerPair.arraySize * 3 ) );
				for ( int kk = 0; kk < vectorArrayData.length(); kk++ )
				{
					tokenPointerPair.tokenFloats[kk * 3] = vectorArrayData[kk].x; 
					tokenPointerPair.tokenFloats[kk * 3 + 1] = vectorArrayData[kk].y; 
					tokenPointerPair.tokenFloats[kk * 3 + 2] = vectorArrayData[kk].z; 
				}
				tokenPointerPair.isArray = true;
				tokenPointerPair.isUArray = false;
				tokenPointerPair.dType = rVertex;
				tokenPointerArray.push_back( tokenPointerPair );
			} else {
				tokenPointerPair.arraySize = 0;
				tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3 );
				nPlug.child(0).getValue( tokenPointerPair.tokenFloats[0] );
				nPlug.child(1).getValue( tokenPointerPair.tokenFloats[1] );
				nPlug.child(2).getValue( tokenPointerPair.tokenFloats[2] );
				tokenPointerPair.isArray = false;
				tokenPointerPair.isUArray = false;
				tokenPointerPair.dType = rConstant;
				tokenPointerArray.push_back( tokenPointerPair );
			}
		}
	}
	if ( colorAttributesFound.length() > 0 ) {
		for ( i = 0; i < colorAttributesFound.length(); i++ ) {
			rTokenPointer tokenPointerPair;
			MString cutString = colorAttributesFound[i].substring(5, colorAttributesFound[i].length());
			sprintf( tokenPointerPair.tokenName, cutString.asChar() ) ;
			MPlug cPlug = nodeFn.findPlug( colorAttributesFound[i] );
			MObject plugObj;
			status = cPlug.getValue( plugObj );
			tokenPointerPair.pType = rColor;
			if ( type() == MRT_Nurbs || type() == MRT_NuCurve ) {
				tokenPointerPair.isNurbs = true;
			} else {
				tokenPointerPair.isNurbs = false;
			}
			tokenPointerPair.isUArray = false;
			if ( plugObj.apiType() == MFn::kVectorArrayData ) 
			{
				MFnVectorArrayData  fnVectorArrayData( plugObj );
				MVectorArray vectorArrayData = fnVectorArrayData.array( &status );
				tokenPointerPair.arraySize = vectorArrayData.length();
				tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * ( tokenPointerPair.arraySize * 3 ) );
				for ( int kk = 0; kk < vectorArrayData.length(); kk++ )
				{
					tokenPointerPair.tokenFloats[kk * 3] = vectorArrayData[kk].x; 
					tokenPointerPair.tokenFloats[kk * 3 + 1] = vectorArrayData[kk].y; 
					tokenPointerPair.tokenFloats[kk * 3 + 2] = vectorArrayData[kk].z; 
				}
				tokenPointerPair.isArray = true;
				tokenPointerPair.dType = rVertex;
				tokenPointerArray.push_back( tokenPointerPair );
			} else {
				tokenPointerPair.arraySize = 0;
				tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3 );
				cPlug.child(0).getValue( tokenPointerPair.tokenFloats[0] );
				cPlug.child(1).getValue( tokenPointerPair.tokenFloats[1] );
				cPlug.child(2).getValue( tokenPointerPair.tokenFloats[2] );
				tokenPointerPair.isArray = false;
				tokenPointerPair.dType = rConstant;
				tokenPointerArray.push_back( tokenPointerPair );
			}
		}
	}
	
	if ( stringAttributesFound.length() > 0 ) {
		for ( i = 0; i < stringAttributesFound.length(); i++ ) {
			rTokenPointer tokenPointerPair;
			MString cutString = stringAttributesFound[i].substring(5, stringAttributesFound[i].length());
			sprintf( tokenPointerPair.tokenName, cutString.asChar() ) ;
			MPlug sPlug = nodeFn.findPlug( stringAttributesFound[i] );
			MObject plugObj;
			status = sPlug.getValue( plugObj );
			tokenPointerPair.pType = rString;
			if ( type() == MRT_Nurbs || type() == MRT_NuCurve ) {
				tokenPointerPair.isNurbs = true;
			} else {
				tokenPointerPair.isNurbs = false;
			}
			tokenPointerPair.arraySize = 0;
			MString stringVal;
			sPlug.getValue( stringVal );
			if ( stringVal.length() != 0 ) {
				tokenPointerPair.tokenString = (char *)lmalloc( sizeof( char ) * stringVal.length() );
				sprintf( tokenPointerPair.tokenString, stringVal.asChar() );
			} else {
				tokenPointerPair.tokenString = "";
			}
			tokenPointerPair.isArray = false;
			tokenPointerPair.isUArray = false;
			tokenPointerPair.dType = rConstant;
			tokenPointerArray.push_back( tokenPointerPair );
		}
	}
}
