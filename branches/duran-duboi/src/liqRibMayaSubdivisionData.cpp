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
** Liquid Rib Maya Style Subdivision Data Source 
** ______________________________________________________________________
*/


/* Renderman Headers */
extern "C" {
#include <ri.h>
}

// Maya's Headers
#include <maya/MPlug.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MVectorArray.h>
#include <maya/MPointArray.h>
#include <maya/MPoint.h>
#include <maya/MUint64Array.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MIntArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnSubd.h>
#include <maya/MFnSubdNames.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MMatrix.h>
#include <maya/MFloatPointArray.h>

#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liqRibMayaSubdivisionData.h>
#include <liqMemory.h>

#include <map>

extern int debugMode;
extern bool liqglo_outputMeshUVs;

typedef std::map<MUint64, int> IDMAP;


liqRibMayaSubdivisionData::liqRibMayaSubdivisionData( MObject mesh )
  // Description: create a RIB compatible subdivision surface representation
  // from Maya's version of subd's
  : npolys(0)
  , nverts(NULL)
  , verts(NULL)
  , vertexParam(NULL)
  , stTexCordParam(NULL)
  , numtexCords(0)
  , hasCreases(false)
  , hasCorners(false)
{
  LIQDEBUGPRINTF( "-> creating maya style subdivision surface\n" );
  MStatus status;
  MFnSubd fnSurf( mesh );
  name = fnSurf.name();

  int level = 0;

  
  ////////////
  // Initialize the nverts and vertices array
  npolys = fnSurf.polygonCount( level, &status );
  MIntArray nvertsArray;

  IDMAP idmap;
  MUint64Array polyvertsIds;
  MUint64Array children;
  for( unsigned u = 0; u < npolys; u++ ) {
    MUint64 index;
    MFnSubdNames::toMUint64( index, u, 0, 0, 0, 0 );
    fnSurf.polygonVertices( index, children );
    nvertsArray.append( children.length() );
    for( unsigned i = 0; i < children.length(); i++ ) {
      //append vertIds to the 64 bit int array
      polyvertsIds.append( children[i] );
      // append the verts to a hash
      idmap[children[i]] = 0;
    }
    children.clear();
  }
  
  // de 64bit-ize everything
  
  
  IDMAP::iterator mi;
  {
  unsigned i = 0;
  for( mi = idmap.begin(); mi != idmap.end(); ++mi ) {
    mi->second = i;
    i++;
  }
  }
  verts = (RtInt*) lmalloc( sizeof( RtInt ) * polyvertsIds.length() );
  for( unsigned i = 0; i < polyvertsIds.length(); i++ ) {
    verts[i] = idmap[polyvertsIds[i]];
  }  
  npolys = nvertsArray.length();
  nverts = (RtInt*) lmalloc( sizeof( RtInt ) * nvertsArray.length() );
  nvertsArray.get( nverts );

  // End nverts and vertices array
  ////////////

  ////////////
  // Add vertex positions

  // Allocate memory for arrays
  // Vertices of the mesh or control cage
  liqTokenPointer vertexPointerPair;
  vertexPointerPair.set( "P", rPoint, false, idmap.size() );
  vertexParam = vertexPointerPair.getTokenFloatArray( );
  vertexPointerPair.setDetailType( rVertex );
  totalNumOfVertices = idmap.size();
  MPoint pt;
  for( mi = idmap.begin(); mi != idmap.end(); ++mi ) {
    fnSurf.vertexPositionGet( mi->first, pt, MSpace::kObject );
    vertexPointerPair.setTokenFloat( mi->second, pt.x, pt.y, pt.z );
  }

  
  LIQDEBUGPRINTF( "-> adding vertex token\n" );
  tokenPointerArray.push_back( vertexPointerPair );
  // End Vertex positions
  ////////////

  ////////////
  // Add texture coordinates
  // Aqsis apparently does not support face/vertex format uv's, and I dont
  // have prman to render against.  Someone will have to patch this to support
  // the face/vertex uv's
  // Oh so dirty.  Must wash my hands.  At least there are no mem leaks here
  
  liqTokenPointer* pFaceVertexSPointer = NULL;
  liqTokenPointer* pFaceVertexTPointer = NULL;
  
#if defined(PRMAN) || defined(DELIGHT) // face/vertex format, which is desireable
  MDoubleArray uVal, vVal;
  MDoubleArray uMap, vMap;
  for( unsigned ii = 0; ii < npolys; ii++ ) {
    MUint64 index;
    MFnSubdNames::toMUint64( index, ii, 0, 0, 0, 0 );
    fnSurf.polygonGetVertexUVs( index, uVal, vVal );
    for( unsigned j = 0; j < uVal.length(); j++ ) {
      uMap.append( uVal[j] );
      vMap.append( 1 - vVal[j] );
    }
    uVal.clear(); vVal.clear();
  }
  
  liqTokenPointer stTexCordPair;
  stTexCordPair.set( "st", rFloat, false, uMap.length(), 2 );
#ifndef DELIGHT
  stTexCordPair.setDetailType( rFaceVarying );
#else
  stTexCordPair.setDetailType( rFaceVertex );
#endif
    
  if( liqglo_outputMeshUVs ) {
    pFaceVertexSPointer = new liqTokenPointer;
    pFaceVertexTPointer = new liqTokenPointer;
    // Match MTOR, which also outputs face-varying STs as well for some reason - Paul
    // There should be a flag in the globals to disable/enable this as it creates
    // bloated RIBs for no reason if you're not concerned about MtoR - Moritz
    pFaceVertexSPointer->set( "u", rFloat, false, uMap.length() );
#ifndef DELIGHT
    pFaceVertexSPointer->setDetailType( rFaceVarying );
#else
    pFaceVertexSPointer->setDetailType( rFaceVertex );
#endif
    pFaceVertexTPointer->set( "v", rFloat, false, uMap.length() );
#ifndef DELIGHT
    pFaceVertexTPointer->setDetailType( rFaceVarying );
#else
    pFaceVertexTPointer->setDetailType( rFaceVertex );
#endif
  }
    
  stTexCordParam = stTexCordPair.getTokenFloatArray();
  for( unsigned k = 0; k < uMap.length(); k++ ) {
    stTexCordPair.setTokenFloat( k, 0, uMap[ k ] );
    stTexCordPair.setTokenFloat( k, 1, vMap[ k ] );

    if( liqglo_outputMeshUVs ) {
      pFaceVertexSPointer->setTokenFloat( k, uMap[ k ] );
      pFaceVertexTPointer->setTokenFloat( k, vMap[ k ] );
    }
  }
#else // #if defined(PRMAN) || defined(DELIGHT)
  // some renderers don't comprehend the rFaceVarying flag, other is best
  // this method just has one uv cord for each vertex
  MDoubleArray uVal; MDoubleArray vVal;
  typedef std::map<MUint64, double> DOUBLEMAP;
  DOUBLEMAP sMap;
  DOUBLEMAP tMap;
  children.clear();
  for( unsigned uu = 0; uu < npolys; uu++ ) {
    MUint64 index;
    MFnSubdNames::toMUint64( index, uu, 0, 0, 0, 0 );
    fnSurf.polygonGetVertexUVs( index, uVal, vVal );
    fnSurf.polygonVertices( index, children );
    for( unsigned j = 0; j < children.length(); j++ ) {
      sMap[children[j]] = uVal[j];
      tMap[children[j]] = 1 - vVal[j];
    }
    uVal.clear(); vVal.clear(); children.clear();
  }
  liqTokenPointer stTexCordPair;
  stTexCordPair.set( "st", rFloat, false, sMap.size(), 2 );
  stTexCordPair.setDetailType( rVertex );
  
  if( liqglo_outputMeshUVs ) {
    pFaceVertexSPointer = new liqTokenPointer;
    pFaceVertexTPointer = new liqTokenPointer;
    // Match MTOR, which also outputs face-varying STs as well for some reason - Paul
    // There should be a flag in the globals to disable/enable this as it creates
    // bloated RIBs for no reason if you're not concerned about MtoR - Moritz
    pFaceVertexSPointer->set( "u", rFloat, false, sMap.size() );
    pFaceVertexSPointer->setDetailType( rVertex );
    pFaceVertexTPointer->set( "v", rFloat, false, tMap.size() );
    pFaceVertexTPointer->setDetailType( rVertex );
  }  
    
  numtexCords = sMap.size() * 2;
  stTexCordParam = stTexCordPair.getTokenFloatArray();  
  DOUBLEMAP::iterator di1; DOUBLEMAP::iterator di2;
  for( di1 = sMap.begin(), di2 = tMap.begin();
       di1 != sMap.end(); ++di1, ++di2 ) {
    stTexCordPair.setTokenFloat( idmap[di1->first], 0, di1->second );
    stTexCordPair.setTokenFloat( idmap[di2->first], 1, di2->second );

    if( liqglo_outputMeshUVs ) {
      pFaceVertexSPointer->setTokenFloat( idmap[di1->first], di1->second );
      pFaceVertexTPointer->setTokenFloat( idmap[di2->first], di2->second );
    }
  }
#endif // #if defined(PRMAN) || defined(DELIGHT)

  tokenPointerArray.push_back( stTexCordPair );
  
  if( pFaceVertexSPointer != NULL ) {
    tokenPointerArray.push_back( *pFaceVertexSPointer );
    delete pFaceVertexSPointer;
  }

  if( pFaceVertexTPointer != NULL ) {
    tokenPointerArray.push_back( *pFaceVertexTPointer );
    delete pFaceVertexTPointer;
  }  
  // End Texture coordinates
  ////////////

  ////////////
  // Add vertex creases and corners
  MUint64Array vertCreases; MUint64Array edgeCreases;
  fnSurf.creasesGetAll( vertCreases, edgeCreases );
  creases.clear();
  if( edgeCreases.length() > 0 ) {
    hasCreases = true;
    MUint64 v1; MUint64 v2;
    for( unsigned i = 0; i < edgeCreases.length(); i++ ) {
      if( MFnSubdNames::level( edgeCreases[i] ) > 0 ) continue;
      fnSurf.edgeVertices( edgeCreases[i], v1, v2 );
      creases.append( idmap[v1] );
      creases.append( idmap[v2] );
    }
  }
  corners.clear();
  if( vertCreases.length() > 0 ) {
    hasCorners = true;
    for( unsigned i = 0; i < vertCreases.length(); i++ ) {
      if( MFnSubdNames::level( vertCreases[i] ) > 0 ) continue;
      corners.append( idmap[vertCreases[i]] );
    }
  }
  // End Vertex Creases
  ////////////

  addAdditionalSurfaceParameters( mesh );
  
  LIQDEBUGPRINTF( "-> done creating maya style subdivision surface\n" );
}

liqRibMayaSubdivisionData::~liqRibMayaSubdivisionData()
// Description: class destructor
{
  if ( debugMode ) { printf("-> killing maya subdivision surface\n"); }
  lfree( nverts ); nverts = NULL;
  lfree( verts ); verts = NULL;
}

void liqRibMayaSubdivisionData::write()
// Description: Write the RIB for this mesh
{
  if ( debugMode ) { printf("-> writing maya subdivision surface\n"); }

  unsigned numTokens = tokenPointerArray.size();
  RtToken *tokenArray = (RtToken *) alloca( sizeof(RtToken) * numTokens );
  RtPointer *pointerArray =
    (RtPointer *) alloca( sizeof(RtPointer) * numTokens );

  assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );

  RtInt   ntags      = 1;
  if( hasCreases ) ntags += (creases.length()/2);
  if( hasCorners ) ntags++;
  RtToken* tags = (RtToken*) alloca( sizeof( RtToken ) * (ntags + 1) );
  tags[ntags] = RI_NULL;
  tags[0] = "interpolateboundary";
  RtInt* nargs = (RtInt*) alloca( sizeof( RtInt ) * ( ntags * 2 ));
  nargs[0] = 0;
  nargs[1] = 0;
  RtInt   *intargs   = NULL;
  RtFloat *floatargs = NULL;
  if( hasCreases && hasCorners ) {
    int k;
    int numCreases = creases.length() / 2;
    for( k = 1; k <= numCreases; k++ ) {
      tags[k] = "crease";
      nargs[k*2] = 2;
      nargs[(k*2)+1] = 1;
    }
    tags[k] = "corner";
    nargs[k*2] = corners.length();
    nargs[(k*2)+1] = 1;
    intargs = (RtInt*) alloca( sizeof( RtInt ) * 
			       ( creases.length() + corners.length() ) );
    for( int i = 0; i < creases.length(); i++ ) {
      intargs[i] = creases[i];
    }
    for( int j = 0; j < corners.length(); j++ ) {
      intargs[j+creases.length()] = corners[j];
    }
    floatargs = (RtFloat*) alloca( sizeof( RtFloat ) * (numCreases + 1 ) );
    for( k = 0; k <= numCreases; k++ ) {
      floatargs[k] = 6; // RI_INFINITY;
    }
  }
  else if( hasCreases && !hasCorners ) {
    int k;
    int numCreases = creases.length() / 2;
    for( k = 1; k <= numCreases; k++ ) {
      tags[k] = "crease";
      nargs[k*2] = 2;
      nargs[(k*2)+1] = 1;
    }
    intargs = (RtInt*) alloca( sizeof( RtInt ) * creases.length() );
    for( int i = 0; i < creases.length(); i++ ) {
      intargs[i] = creases[i];
    }
    floatargs = (RtFloat*) alloca( sizeof( RtFloat ) * numCreases);
    for( k = 0; k < numCreases; k++ ) {
      floatargs[k] = 6; // RI_INFINITY;
    }
  }
  else if( hasCorners && !hasCreases ) {
    tags[1] = "corner";
    nargs[2] = corners.length();
    nargs[3] = 1;
    intargs = (RtInt*) alloca( sizeof( RtInt ) * corners.length() );
    for( int i = 0; i < corners.length(); i++ ) {
      intargs[i] = corners[i];
    }
    floatargs = (RtFloat*) alloca( sizeof( RtFloat ) );
    floatargs[0] = 6; // RI_INFINITY;
  }

  RiSubdivisionMeshV( "catmull-clark", npolys, nverts, verts, ntags, tags, nargs, intargs, floatargs, numTokens, tokenArray, pointerArray ); 
}

bool liqRibMayaSubdivisionData::compare( const liqRibData & otherObj ) const
// Description: Compare this mesh to the other for the purpose of determining if its animated
{
  if ( debugMode ) { printf("-> comparing maya subdiv\n"); }
  if ( otherObj.type() != MRT_MayaSubdivision ) return false;
  const liqRibMayaSubdivisionData & other = (liqRibMayaSubdivisionData&)otherObj;

  if ( npolys != other.npolys ) return false;
  if( hasCreases != other.hasCreases ) return false;
  if( hasCorners != other.hasCorners ) return false;

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
         !equiv( vertexParam[c], other.vertexParam[c] ) )
    {
      return false;
    }
  }

  if( numtexCords != other.numtexCords ) return false;
  for( i = 0; i < numtexCords; ++i ) {
    if( !equiv( stTexCordParam[i], other.stTexCordParam[i] ) ) return false;
  }

  return true;  
}

ObjectType liqRibMayaSubdivisionData::type() const
// Description: return the geometry type
{
  if ( debugMode ) { printf("-> returning subdivision surface type\n"); }
  return MRT_MayaSubdivision; 
}
