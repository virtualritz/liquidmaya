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
** Liquid Rib Bake Subdivision Mesh Data Source
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
#include <liqRibBakeSubdivisionData.h>
#include <liqMemory.h>

extern int debugMode;

liqRibBakeSubdivisionData::liqRibBakeSubdivisionData( MObject mesh )
//  Description: create a RIB compatible subdivision surface representation using a Maya polygon mesh
  : numFaces( 0 ),
    numPoints ( 0 ),
    nverts( NULL ),
    verts( NULL ),
    vertexParam( NULL ),
    interpBoundary( false )
{
  if ( debugMode ) { printf("-> baking subdivision surface\n"); }
  MFnMesh     fnMesh( mesh );
  name = fnMesh.name();

  MStatus status;

// Why do we need a flag here?  We know if the mesh is face-varying or not - Paul
/*bool facevaryingUVs = false;
  MPlug facevaryingUVsPlug = fnMesh.findPlug( "facevaryingUVs", &status );
  if ( status == MS::kSuccess ) {
    facevaryingUVsPlug.getValue( facevaryingUVs );
  }*/

  MPlug interpBoundaryPlug = fnMesh.findPlug( "interpBoundary", &status );
  if ( status == MS::kSuccess ) {
    interpBoundaryPlug.getValue( interpBoundary );
  }

  numPoints = fnMesh.numVertices();
  const unsigned numSTs = fnMesh.numUVs();
  numFaces = fnMesh.numPolygons();
  const unsigned numFaceVertices = fnMesh.numFaceVertices();
  unsigned face = 0;
  unsigned faceVertex = 0;
  unsigned count;
  unsigned vertex;
  float S;
  float T;
  MPoint point;
  liqTokenPointer pointsPointerPair;
  liqTokenPointer* pVertexSTPointerPair = NULL;
  liqTokenPointer* pFaceVertexSPointer = NULL;
  liqTokenPointer* pFaceVertexTPointer = NULL;

  // Allocate memory and tokens
  numFaces = numFaces;
  nverts = (RtInt*) lmalloc( sizeof( RtInt ) * numFaces );
  verts = (RtInt*) lmalloc( sizeof( RtInt ) * numFaceVertices );

  pointsPointerPair.set( "P", rPoint, false, true, false, numPoints );
  pointsPointerPair.setDetailType( rVertex );

  if ( numSTs > 0 ) {
    pFaceVertexSPointer = new liqTokenPointer;
    pFaceVertexTPointer = new liqTokenPointer;

    if ( numSTs == numPoints ) {
      pVertexSTPointerPair = new liqTokenPointer;
      pVertexSTPointerPair->set( "st", rFloat, false, true, true, numPoints * 2);
      pVertexSTPointerPair->setDetailType( rVertex );

      // Match MTOR, which also outputs face-varying STs as well for some reason - Paul
      pFaceVertexSPointer->set( "u", rFloat, false, true, false, numFaceVertices );
      pFaceVertexSPointer->setDetailType( rFaceVarying );
      pFaceVertexTPointer->set( "v", rFloat, false, true, false, numFaceVertices );
      pFaceVertexTPointer->setDetailType( rFaceVarying );
    } else {
      pFaceVertexSPointer->set( "s", rFloat, false, true, false, numFaceVertices );
      pFaceVertexSPointer->setDetailType( rFaceVarying );
      pFaceVertexTPointer->set( "t", rFloat, false, true, false, numFaceVertices );
      pFaceVertexTPointer->setDetailType( rFaceVarying );
    }
    
  }

  vertexParam = pointsPointerPair.getTokenFloatArray();

  // Read the mesh from Maya
  for ( MItMeshPolygon polyIt ( mesh ); polyIt.isDone() == false; polyIt.next() ) {
    count = polyIt.polygonVertexCount();
    nverts[face] = count;

    while ( count > 0 ) {
      --count;
      vertex = polyIt.vertexIndex( count );
      verts[faceVertex] = vertex;

      if( numSTs > 0 ) {
        fnMesh.getPolygonUV( face, count, S, T );

        pointsPointerPair.setTokenFloat( faceVertex, S, 1 - T, 0 );

        if( numSTs == numPoints ) {
          pVertexSTPointerPair->setTokenFloat( vertex * 2 + 0, S );
          pVertexSTPointerPair->setTokenFloat( vertex * 2 + 1, 1 - T );
        }

        // Match MTOR, which always outputs face-varying STs as well for some reason - Paul
        pFaceVertexSPointer->setTokenFloat( faceVertex, S );
        pFaceVertexTPointer->setTokenFloat( faceVertex, 1 - T );
        
      }

      ++faceVertex;
    }

    ++face;
  }

  // Add tokens to array and clean up after
  tokenPointerArray.push_back( pointsPointerPair );

  addAdditionalSurfaceParameters( mesh );
}

liqRibBakeSubdivisionData::~liqRibBakeSubdivisionData()
// Description: class destructor
{
  if ( debugMode ) { printf("-> killing bake subdivision surface\n"); }
  lfree( nverts );
  lfree( verts );
  nverts = NULL;
  verts  = NULL;
}

void liqRibBakeSubdivisionData::write()
// Description: Write the RIB for this mesh
{
  if ( debugMode ) { printf("-> writing bake subdivision surface\n"); }

  // Each loop has one polygon, so we just want an array of 1's of
  // the correct size
  RtInt * nloops = (RtInt*)alloca( sizeof( RtInt ) * numFaces );
  for ( unsigned i = 0; i < (unsigned) numFaces; ++i ) {
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
    tags = (RtToken *) alloca( sizeof(RtToken) * 2);
    tags[0] = "interpolateboundary";
    tags[1] = NULL;
    nargs = (RtInt *)  alloca( sizeof(RtInt) * 2);
    nargs[0] = 0;
    nargs[1] = 0;
  }

  RiSubdivisionMeshV( "catmull-clark", numFaces, nverts, verts, ntags, tags, nargs, intargs, floatargs, numTokens, tokenArray, pointerArray );
}

bool liqRibBakeSubdivisionData::compare( const liqRibData & otherObj ) const
// Description: Compare this mesh to the other for the purpose of determining if its animated
{
  unsigned i;
  unsigned numFaceVertices = 0;

  if ( debugMode ) { printf("-> comparing mesh\n"); }
  if ( otherObj.type() != MRT_Subdivision ) return false;
  const liqRibBakeSubdivisionData & other = (liqRibBakeSubdivisionData&)otherObj;

  if ( numFaces != other.numFaces ) return false;
  if ( numPoints != other.numPoints ) return false;

  for ( i = 0; i < (unsigned) numFaces; ++i ) {
    if ( nverts[i] != other.nverts[i] ) return false;
    numFaceVertices += nverts[i];
  }

  for ( i = 0; i < numFaceVertices; ++i ) {
    if ( verts[i] != other.verts[i] ) return false;
  }

  for ( i = 0; i < (unsigned) numPoints; ++i ) {
    const unsigned a = i * 3;
    const unsigned b = a + 1;
    const unsigned c = a + 2;
    if ( !equiv( vertexParam[a], other.vertexParam[a] ) ||
      !equiv( vertexParam[b], other.vertexParam[b] ) ||
      !equiv( vertexParam[c], other.vertexParam[c] ) )
    {
      return false;
    }
  }

  return true;
}

ObjectType liqRibBakeSubdivisionData::type() const
// Description: return the geometry type
{
  LIQDEBUGPRINTF( "-> returning bake subdivision surface type\n" );
  return MRT_BakeSubdivision;
}
