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
** Liquid Rib Mesh Data Source 
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
#include <maya/MPlug.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MIntArray.h>
#include <maya/MFnMesh.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MMatrix.h>
#include <maya/MFloatPointArray.h>

#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liquidRibMeshData.h>
#include <liqMemory.h>

extern int debugMode;

liquidRibMeshData::liquidRibMeshData( MObject mesh )
//
//  Description:
//      create a RIB compatible representation of a Maya polygon mesh
//
:   npolys( 0 ),
nverts( NULL ),
verts( NULL ),
vertexParam( NULL ),
normalParam( NULL ),
totalNumOfVertices( 0 )
{
	areaLight = false;
    if ( debugMode ) { printf("-> creating mesh\n"); }
    MFnMesh     fnMesh( mesh );
    objDagPath = fnMesh.dagPath();

    name = fnMesh.name();
	
    // To handle the cases where there are multiple normals per
    // vertices (hard-edges) we store a vertex for each normal.
    //
    npolys = fnMesh.numPolygons();
    nverts       = (RtInt*)  lmalloc( sizeof( RtInt )   * npolys );

    MStatus status;
    MStatus astatus;
    astatus == MS::kFailure;

    MPlug areaPlug = fnMesh.findPlug( "areaIntensity", &astatus );
    if ( astatus == MS::kSuccess ) { 
	    areaLight = true;
    } else {
	    areaLight = false;
    }

    if ( areaLight ) {
	    MDagPath meshDagPath;
	    meshDagPath = fnMesh.dagPath();
	    MTransformationMatrix worldMatrix = meshDagPath.inclusiveMatrix();
	    MMatrix worldMatrixM = worldMatrix.asMatrix();    	
	    worldMatrixM.get( transformationMatrix );
	    areaPlug.getValue( areaIntensity );
    }

    bool altMeshMethod;
    altMeshMethod = false;
    MPlug altMeshPlug = fnMesh.findPlug( "altMeshExport", &status );
    if ( status == MS::kSuccess ) altMeshMethod = true;

    MFloatPointArray pos;
    if ( altMeshMethod ) {
	    totalNumOfVertices = 0;
	    for ( uint pOn = 0; pOn < npolys; pOn++ )
	    {
		    nverts[ pOn ] = fnMesh.polygonVertexCount( pOn );
		    totalNumOfVertices += nverts[ pOn ];
	    }
    } else {
	    totalNumOfVertices = fnMesh.numNormals();
    }
	
    // Allocate memory for arrays
    //
    liqTokenPointer vertextpp;
    vertextpp.set( "P", rPoint, false, true, false, totalNumOfVertices );
    vertextpp.setDetailType( rVertex );
    vertexParam = vertextpp.getTokenFloatArray();;

    liqTokenPointer uvtpp;
    uvtpp.set( "st", rFloat, false, true, false, 2 * totalNumOfVertices );
    uvtpp.setDetailType( rVertex );


    liqTokenPointer normaltpp;
    normaltpp.set( "N", rNormal, false, true, false, totalNumOfVertices );
    normaltpp.setDetailType( rVertex );
    normalParam = normaltpp.getTokenFloatArray();

    MIntArray perPolyVertices;
    float uval, vval;
    MPoint position;
    unsigned count, index = 0, vindex = 0;
	
	// If the altMeshExport attribute was found on the object then use an alternative 
	// way of storing the data.  It takes more memory but combats problems with mesh's that
	// store face/vertex specific uv values in maya.  MTOR still has this problem.  
	
	if ( altMeshMethod ) {
    if ( debugMode ) { printf("-> using altMeshExport!\n"); }
		MFloatVectorArray normals;
		fnMesh.getNormals( normals );
		
		long pOn = 0;
		for ( MItMeshPolygon polyIt( mesh ); !polyIt.isDone(); polyIt.next() ) 
		//for ( uint pOn = 0; pOn < npolys; pOn++ )
		{
			MIntArray pVs;
			fnMesh.getPolygonVertices( pOn, pVs );
			for ( long vOn = ( nverts[ pOn ] - 1 ); vOn >= 0; vOn-- ) 
			{
				MPoint position;
				fnMesh.getPoint( pVs[vOn], position );
				vertextpp.setTokenFloat( vindex, position.x, position.y, position.z );
				int nIndex = polyIt.normalIndex( vOn );
				normaltpp.setTokenFloat( vindex, normals[ nIndex ].x, normals[ nIndex ].y, normals[ nIndex ].z );
				fnMesh.getPolygonUV( pOn, vOn, uval, vval);
				uvtpp.setTokenFloat( vindex * 2, uval );
				uvtpp.setTokenFloat( vindex * 2 +1, 1 - vval );
				perPolyVertices.append( vindex );
				vindex++;
			}
			pOn++;
		}
	} else {		
    if ( debugMode ) { printf("-> not using altMeshExport!\n"); }
		for ( MItMeshPolygon polyIt( mesh ); !polyIt.isDone(); polyIt.next() ) {
			count = polyIt.polygonVertexCount();
			nverts[index] = count;
			
			// Note that we need to reverse the normals for RIB so we
			// get the vertex id's in reverse order
			do {
				count--;
				unsigned normalIndex = polyIt.normalIndex( count );
				perPolyVertices.append( normalIndex );
				position = polyIt.point( count );
				vertextpp.setTokenFloat( normalIndex, position.x, position.y, position.z );
				//this next step outputs the texture uv coordinates for the mesh
				fnMesh.getPolygonUV( index, count, uval, vval);
				uvtpp.setTokenFloat( normalIndex * 2, uval );
				uvtpp.setTokenFloat( normalIndex * 2 +1, 1 - vval );
			} while (count != 0);
			++index;
		}
		
		// write out the correct shading normals
		MFloatVectorArray normals;
		fnMesh.getNormals( normals );
		for ( index = 0; index<totalNumOfVertices; index++ ) {
    	    	    normaltpp.setTokenFloat( index, normals[ index ].x, normals[ index ].y, normals[ index ].z );
		}
	}
    
	verts = (RtInt*)lmalloc( sizeof( RtInt ) * perPolyVertices.length() );
	perPolyVertices.get( (int*)verts );
	
	// add all of our surface parameters to the vector container
	tokenPointerArray.push_back( vertextpp );
	tokenPointerArray.push_back( uvtpp );	
	tokenPointerArray.push_back( normaltpp );	
	addAdditionalSurfaceParameters( mesh );
}

liquidRibMeshData::~liquidRibMeshData()
//
//  Description:
//      class destructor
//
{
    if ( debugMode ) { printf("-> killing mesh\n"); }
    lfree( nverts ); nverts = NULL;
    lfree( verts ); verts = NULL;
}

void liquidRibMeshData::write()
//
//  Description:
//      Write the RIB for this mesh
//
{
	if ( debugMode ) { printf("-> writing mesh\n"); }
	RtLightHandle handle = NULL;
	if ( areaLight ) { 
		if ( debugMode ) { printf("-> mesh is area light\n"); }
		//	RiAttributeBegin();
		const char * namePtr = name.asChar();
		RiAttribute( "identifier", "name", &namePtr, RI_NULL );
		RiTransform( transformationMatrix );
		
		handle = RiAreaLightSource( "arealight", "intensity", &areaIntensity, RI_NULL );
	}
	
	// Each loop has one polygon, so we just want an array of 1's of
	// the correct size
	RtInt * nloops = (RtInt*)alloca( sizeof( RtInt ) * npolys );
	for ( unsigned i = 0; i < npolys; ++i ) {
		nloops[i] = 1;
	}
	
	unsigned numTokens = tokenPointerArray.size();
	RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * numTokens );
	RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * numTokens );
	
	assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );
	
	RiPointsGeneralPolygonsV( npolys, nloops, nverts, verts, numTokens, tokenArray, pointerArray ); 
	
	if ( areaLight ) { 
		//	RiAttributeEnd();
		RiIlluminate( handle, 1 );
	}
}

bool liquidRibMeshData::compare( const liquidRibData & otherObj ) const
//
//  Description:
//      Compare this mesh to the other for the purpose of determining
//      if its animated
//
{
	if ( debugMode ) { printf("-> comparing mesh\n"); }
    if ( otherObj.type() != MRT_Mesh ) return false;
    const liquidRibMeshData & other = (liquidRibMeshData&)otherObj;
    
    if ( npolys != other.npolys ) return false;
    
    unsigned i;
    unsigned totalPolyVerts = 0;
    for ( i = 0; i < npolys; ++i ) {
        if ( nverts[i] != other.nverts[i] ) return false;
        totalPolyVerts += nverts[i];
    }
    
    for ( i = 0; i < totalPolyVerts; ++i ) {
        if ( verts[i] != other.verts[i] ) return false;
    }
    
    for ( i = 0; i < totalNumOfVertices; ++i ) {
		unsigned a = i * 3;
		unsigned b = a + 1;
		unsigned c = a + 2;
        if ( !equiv( vertexParam[a], other.vertexParam[a] ) ||
			!equiv( vertexParam[b], other.vertexParam[b] ) ||
			!equiv( vertexParam[c], other.vertexParam[c] ) ||
			!equiv( normalParam[a], other.normalParam[a] ) ||
			!equiv( normalParam[b], other.normalParam[b] ) ||
			!equiv( normalParam[c], other.normalParam[c] ) )
		{
            return false;
        }
    }
    return true;
}

ObjectType liquidRibMeshData::type() const
//
//  Description:
//      return the geometry type
//
{
	if ( debugMode ) { printf("-> returning mesh type\n"); }
	if ( areaLight ) {
		return MRT_Light;
	} else {
		return MRT_Mesh;
	}
}
