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
** Liquid Rib Mesh Data Source 
** ______________________________________________________________________
*/

/* Standard Headers */
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>

#ifndef _WIN32
/* Dynamic Object Headers */
#include <dlfcn.h>
#endif

/* Renderman Headers */
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

/* Maya's Headers */
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
#include <liqRibSubdivisionData.h>
#include <liqMemory.h>

extern int debugMode;

liqRibSubdivisionData::liqRibSubdivisionData( MObject mesh )
//  Description: create a RIB compatible subdivision surface representation using a Maya polygon mesh
  : npolys( 0 ),
    nverts( NULL ),
    verts( NULL ),
    vertexParam( NULL ),
    normalParam( NULL ),
    polyuvParam( NULL ),
    totalNumOfVertices( 0 ),
    interpBoundary( false )
{
  if ( debugMode ) { printf("-> creating subdivision surface\n"); }
  MFnMesh     fnMesh( mesh );
  name = fnMesh.name();

  // To handle the cases where there are multiple normals per vertices (hard-edges) we store a vertex for each normal
  npolys = fnMesh.numPolygons();
  nverts = (RtInt*)  lmalloc( sizeof( RtInt )   * npolys );

  MStatus status;

  bool facevaryingUVs = false;
  MPlug facevaryingUVsPlug = fnMesh.findPlug( "facevaryingUVs", &status );
  if ( status == MS::kSuccess ) {
    facevaryingUVsPlug.getValue( facevaryingUVs );
  }

  MPlug interpBoundaryPlug = fnMesh.findPlug( "interpBoundary", &status );
  if ( status == MS::kSuccess ) {
    interpBoundaryPlug.getValue( interpBoundary );
  }
  
  MFloatPointArray pos;
  fnMesh.getPoints( pos );
  totalNumOfVertices = pos.length();

  // Allocate memory for arrays
  // Vertices of the mesh or control cage
  liqTokenPointer vertexPointerPair;
  vertexPointerPair.set( "P", rPoint, false, true, false, totalNumOfVertices );
  vertexParam = vertexPointerPair.getTokenFloatArray( );
  vertexPointerPair.setDetailType( rVertex );

  // If we are handling the facevarying type that are used by prman 10 and entropy 3.2 we have to grab a UV
  // for each vertex in each face, if not, just grab one for each vertex, so the memory allocations will be different
  int numberOfUVs = 0;
  if ( facevaryingUVs ) {
    for ( uint pOn = 0; pOn < npolys; pOn++ ) {
      numberOfUVs += fnMesh.polygonVertexCount( pOn );
    }
  } else {
    numberOfUVs = totalNumOfVertices;
  }

  MIntArray perPolyVertices;
  MPoint position;
  unsigned count, index = 0;

  for ( int vOn = 0; vOn < totalNumOfVertices; vOn++ ) {
    vertexPointerPair.setTokenFloat( vOn, pos[ vOn ].x, pos[ vOn ].y, pos[ vOn ].z );
  }

  liqTokenPointer uvPointerPair;
  // Hmmmmmm shall use isUArray inside set method to compute size * 2
  uvPointerPair.set( "st", rFloat, false, true, true, 2 * numberOfUVs );
  polyuvParam = uvPointerPair.getTokenFloatArray();
  if ( facevaryingUVs ) {
    uvPointerPair.setDetailType( rFaceVarying );
  } else {
    uvPointerPair.setDetailType( rVertex );
  }
  // Hmmmmmmmm got to check that
  //tokenPointerPair.uArraySize = 2;

  unsigned int UVIndex = 0;
  for ( MItMeshPolygon polyIt( mesh ); !polyIt.isDone(); polyIt.next() ) {
    if ( debugMode ) { printf("-> getting poly count\n"); }
    count = polyIt.polygonVertexCount();
    nverts[index] = count;

    if ( debugMode ) { printf("-> looping through verticies\n"); }
    //for ( unsigned int vertOn = 0; vertOn < count; vertOn++ ) {
    int vertOn = count;
    do {
      vertOn--;
      unsigned vertexIndex = polyIt.vertexIndex( vertOn );
      float u, v;
      int tempUVindex;
      status = polyIt.getUVIndex( vertOn, tempUVindex, u, v );
      perPolyVertices.append( vertexIndex );
      if ( facevaryingUVs ) {
        uvPointerPair.setTokenFloat( UVIndex * 2 + 0, u );
        uvPointerPair.setTokenFloat( UVIndex * 2 + 1, 1 - v );
        UVIndex++;
      } else {
        uvPointerPair.setTokenFloat( vertexIndex * 2 + 0, u );
        uvPointerPair.setTokenFloat( vertexIndex * 2 + 1, 1 - v );
      }
    } while ( vertOn != 0 );
    ++index;
  }

  verts = (RtInt*)lmalloc( sizeof( RtInt ) * perPolyVertices.length() );
  perPolyVertices.get( (int*)verts );

  // add all of our surface parameters to the vector container
  tokenPointerArray.push_back( vertexPointerPair );
  tokenPointerArray.push_back( uvPointerPair );

  addAdditionalSurfaceParameters( mesh );
}

liqRibSubdivisionData::~liqRibSubdivisionData()
// Description: class destructor
{
  if ( debugMode ) { printf("-> killing subdivision surface\n"); }
  lfree( nverts );
  lfree( verts );
  nverts = NULL;
  verts  = NULL;
}

void liqRibSubdivisionData::write()
// Description: Write the RIB for this mesh
{
  if ( debugMode ) { printf("-> writing subdivision surface\n"); }

  // Each loop has one polygon, so we just want an array of 1's of
  // the correct size
  RtInt * nloops = (RtInt*)alloca( sizeof( RtInt ) * npolys );
  for ( unsigned i = 0; i < npolys; ++i ) {
    nloops[i] = 1;
  }

  unsigned numTokens = tokenPointerArray.size();
  RtToken   *tokenArray   = (RtToken *)   alloca( sizeof(RtToken)   * numTokens );
  RtPointer *pointerArray = (RtPointer *) alloca( sizeof(RtPointer) * numTokens );
  assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );

  RtInt    ntags     = 0;
  RtToken *tags      = NULL;
  RtInt   *nargs     = NULL;
  RtInt   *intargs   = NULL;
  RtFloat *floatargs = NULL;
  
  if ( interpBoundary ) {
    ntags++;
    tags = (RtToken *) alloca( sizeof(RtToken));
    tags[0] = "interpolateboundary";
    tags[1] = NULL;
    nargs = (RtInt *)  alloca( sizeof(RtInt) * 2);
    nargs[0] = 0;
    nargs[1] = 0;
  }
  
  RiSubdivisionMeshV( "catmull-clark", npolys, nverts, verts, ntags, tags, nargs, intargs, floatargs, numTokens, tokenArray, pointerArray ); 
}

bool liqRibSubdivisionData::compare( const liqRibData & otherObj ) const
// Description: Compare this mesh to the other for the purpose of determining if its animated
{
  if ( debugMode ) { printf("-> comparing mesh\n"); }
  if ( otherObj.type() != MRT_Subdivision ) return false;
  const liqRibSubdivisionData & other = (liqRibSubdivisionData&)otherObj;

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

ObjectType liqRibSubdivisionData::type() const
// Description: return the geometry type
{
  if ( debugMode ) { printf("-> returning subdivision surface type\n"); }
  return MRT_Subdivision; 
}
