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
** Liquid Rib Subdivision Mesh Data Source
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
#include <maya/MPlugArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MIntArray.h>
#include <maya/MFnMesh.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MMatrix.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFnSet.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>

#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liqRibSubdivisionData.h>
#include <liqMemory.h>

extern int debugMode;
extern bool liqglo_outputMeshUVs;
extern bool liqglo_useMtorSubdiv;  // interpret mtor subdiv attributes

liqRibSubdivisionData::liqRibSubdivisionData( MObject mesh )
//  Description: create a RIB compatible subdivision surface representation using a Maya polygon mesh
  : numFaces( 0 ),
    numPoints ( 0 ),
    nverts( NULL ),
    verts( NULL ),
    vertexParam( NULL ),
    interpolateBoundary( false )
{
  LIQDEBUGPRINTF( "-> creating subdivision surface\n" );
  MFnMesh fnMesh( mesh );
  name = fnMesh.name();

  MStatus status;

  /*
  MPlug interpolateBoundaryPlug = fnMesh.findPlug( "liqSubdivInterpolateBoundary", &status );
  if ( status == MS::kSuccess ) {
    interpolateBoundaryPlug.getValue( interpolateBoundary );
  }

  bool interpolateBoundaryOld = false;
  MPlug oldinterpolateBoundaryPlug = fnMesh.findPlug( "interpBoundary", &status );
  if ( status == MS::kSuccess ) {
    interpolateBoundaryPlug.getValue( interpolateBoundaryOld );
  }

  bool interpolateBoundaryMtor = false;
  if ( liqglo_useMtorSubdiv ) {
    MPlug mtor_interpolateBoundaryPlug = fnMesh.findPlug( "mtorSubdivInterp", &status );
    if ( status == MS::kSuccess ) {
      mtor_interpolateBoundaryPlug.getValue( interpolateBoundaryMtor );
    }
  }

  interpolateBoundary |= interpolateBoundaryMtor | interpolateBoundaryOld;
  */

  checkExtraTags( mesh );

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

  pointsPointerPair.set( "P", rPoint, false, numPoints );
  pointsPointerPair.setDetailType( rVertex );

  if ( numSTs > 0 ) {
    pVertexSTPointerPair = new liqTokenPointer;
    pVertexSTPointerPair->set( "st", rFloat, false, numFaceVertices, 2 );
#ifdef DELIGHT
    pVertexSTPointerPair->setDetailType( rFaceVertex );
#else
  pVertexSTPointerPair->setDetailType( rFaceVarying );
#endif

  if( liqglo_outputMeshUVs ) {
    pFaceVertexSPointer = new liqTokenPointer;
      pFaceVertexTPointer = new liqTokenPointer;
      // Match MTOR, which also outputs face-varying STs as well for some reason - Paul
      pFaceVertexSPointer->set( "u", rFloat, false, numFaceVertices );
#ifdef DELIGHT
      pFaceVertexSPointer->setDetailType( rFaceVertex );
#else
      pFaceVertexSPointer->setDetailType( rFaceVarying );
#endif
      pFaceVertexTPointer->set( "v", rFloat, false, numFaceVertices );
#ifdef DELIGHT
      pFaceVertexTPointer->setDetailType( rFaceVertex );
#else
    pFaceVertexTPointer->setDetailType( rFaceVarying );
#endif
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
      point = polyIt.point( count, MSpace::kObject );
      pointsPointerPair.setTokenFloat( vertex, point.x, point.y, point.z );

      if( numSTs > 0 ) {
        fnMesh.getPolygonUV( face, count, S, T );

        pVertexSTPointerPair->setTokenFloat( faceVertex, 0, S );
        pVertexSTPointerPair->setTokenFloat( faceVertex, 1, 1 - T );

    if( liqglo_outputMeshUVs ) {
          // Match MTOR, which always outputs face-varying STs as well for some reason - Paul
          pFaceVertexSPointer->setTokenFloat( faceVertex, S );
          pFaceVertexTPointer->setTokenFloat( faceVertex, 1 - T );
    }
      }

      ++faceVertex;
    }

    ++face;
  }

  // Add tokens to array and clean up after
  tokenPointerArray.push_back( pointsPointerPair );

  if( pVertexSTPointerPair != NULL ) {
    tokenPointerArray.push_back( *pVertexSTPointerPair );
    delete pVertexSTPointerPair;
  }

  if( pFaceVertexSPointer != NULL ) {
    tokenPointerArray.push_back( *pFaceVertexSPointer );
    delete pFaceVertexSPointer;
  }

  if( pFaceVertexTPointer != NULL ) {
    tokenPointerArray.push_back( *pFaceVertexTPointer );
    delete pFaceVertexTPointer;
  }

  addAdditionalSurfaceParameters( mesh );
}

liqRibSubdivisionData::~liqRibSubdivisionData()
// Description: class destructor
{
  LIQDEBUGPRINTF( "-> killing subdivision surface\n" );
  lfree( nverts );
  lfree( verts );
  nverts = NULL;
  verts  = NULL;
}

void liqRibSubdivisionData::write()
// Description: Write the RIB for this mesh
{
  LIQDEBUGPRINTF( "-> writing subdivision surface\n" );

  // Each loop has one polygon, so we just want an array of 1's of
  // the correct size
  RtInt * nloops = (RtInt*)alloca( sizeof( RtInt ) * numFaces );
  for ( unsigned i = 0; i < numFaces; ++i ) {
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

  /*
  if ( interpolateBoundary ) {
    ntags++;
    tags = (RtToken *) alloca( sizeof(RtToken) * 2);
    tags[0] = "interpolateboundary";
    tags[1] = NULL;
    nargs = (RtInt *)  alloca( sizeof(RtInt) * 2);
    nargs[0] = 0;
    nargs[1] = 0;
  }*/

  if ( !v_tags.empty() ) { // we have extra tags
    int j;
    tags = (RtToken *) lmalloc( sizeof(RtToken) * (v_tags.size() + 1) );
    std::vector <RtToken>::iterator t;
    for( j = 0, t = v_tags.begin(); t != v_tags.end() ; t++, j++ )  {
      tags[j] = *t;
    }
    tags[j] = NULL;
    ntags = v_tags.size();
    if ( !v_nargs.empty() ) {
      nargs = (RtInt *) lmalloc( sizeof(RtInt) * v_nargs.size() );
      std::vector <RtInt>::iterator n;
      for( j = 0, n = v_nargs.begin(); n != v_nargs.end() ; n++, j++ )  {
        nargs[j] = *n;
      }
    }
    if ( !v_intargs.empty() ) {
      intargs = (RtInt *) lmalloc( sizeof(RtInt) * v_intargs.size() );
      std::vector <RtInt>::iterator i;
      for( j = 0, i = v_intargs.begin(); i != v_intargs.end() ; i++, j++ )  {
        intargs[j] = *i;
      }
    }
    if ( !v_floatargs.empty() ) {
      floatargs = (RtFloat *) lmalloc( sizeof(RtInt) * v_floatargs.size() );
      std::vector <RtFloat>::iterator f;
      for( j = 0, f = v_floatargs.begin(); f != v_floatargs.end() ; f++, j++ )  {
        floatargs[j] = *f;
      }
    }
  }  
  
  RiSubdivisionMeshV( "catmull-clark", numFaces, nverts, verts, ntags, tags, nargs, intargs, floatargs, numTokens, tokenArray, pointerArray );
}


bool liqRibSubdivisionData::compare( const liqRibData & otherObj ) const
// Description: Compare this mesh to the other for the purpose of determining if its animated
{
  unsigned i;
  unsigned numFaceVertices = 0;

  LIQDEBUGPRINTF( "-> comparing mesh\n" );
  if ( otherObj.type() != MRT_Subdivision ) return false;
  const liqRibSubdivisionData & other = (liqRibSubdivisionData&)otherObj;

  if ( numFaces != other.numFaces ) return false;
  if ( numPoints != other.numPoints ) return false;

  for ( i = 0; i < numFaces; ++i ) {
    if ( nverts[i] != other.nverts[i] ) return false;
    numFaceVertices += nverts[i];
  }

  for ( i = 0; i < numFaceVertices; ++i ) {
    if ( verts[i] != other.verts[i] ) return false;
  }

  for ( i = 0; i < numPoints; ++i ) {
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

ObjectType liqRibSubdivisionData::type() const
// Description: return the geometry type
{
  LIQDEBUGPRINTF( "-> returning subdivision surface type\n" );
  return MRT_Subdivision;
}

// Creases, corners, holes are organized by MEL script into maya sets
// that are connected to shape and have corresponded attributes:
// liqSubdivCrease, liqSubdivCorner, liqSubdivHole, liqSubdivStitch
// 'stitch' -- new tag introduced in PRman 11 suited for seamless
// stitching two subdiv surfaces aligned by common edge line.
// If global flag liqglo_useMtorSubdiv is set, then procedure looks also
// for analog mtor attributes
//
void liqRibSubdivisionData::checkExtraTags( MObject &mesh ) {
  MStatus status = MS::kSuccess;
  MPlugArray array;
  MFnMesh    fnMesh( mesh );
  MFnDependencyNode depNode( mesh );
  depNode.getConnections( array ); // collect all plugs connected to mesh

  // for each plug check if it is connected as dst to maya set
  // and this set has attribute for subdiv extra tag with non zero value
  for ( unsigned i = 0 ; i < array.length() ; i++ ) {
    MPlugArray connections;
    MPlug curPlug = array[i];

    if ( !curPlug.connectedTo( connections, false, true ) )
      continue; /* look only for plugs connected as dst (src = false) */

    for ( unsigned i = 0 ; i < connections.length() ; i++ ) {
      MPlug dstPlug = connections[i];
      MObject dstNode = dstPlug.node();

      if ( dstNode.hasFn( MFn::kSet ) ) {	/* if connected to set */
        float extraTagValue;
        MFnDependencyNode setNode( dstNode, &status );
        if ( status != MS::kSuccess )
          continue;
        MPlug extraTagPlug = setNode.findPlug( "liqSubdivCrease", &status );
        if ( status == MS::kSuccess ) {
          extraTagPlug.getValue( extraTagValue );
          if ( extraTagValue ) // skip zero values
            addExtraTags( dstNode, extraTagValue, TAG_CREASE );
        } else {
          MPlug extraTagPlug = setNode.findPlug( "liqSubdivCorner", &status );
          if ( status == MS::kSuccess ) {
            extraTagPlug.getValue( extraTagValue );
            if ( extraTagValue ) // skip zero values
              addExtraTags( dstNode, extraTagValue, TAG_CORNER );
          } else {
            MPlug extraTagPlug = setNode.findPlug( "liqSubdivHole", &status );
            if ( status == MS::kSuccess ) {
              extraTagPlug.getValue( extraTagValue );
              if ( extraTagValue ) // skip zero values
                addExtraTags( dstNode, extraTagValue, TAG_HOLE );
            } else {
              MPlug extraTagPlug = setNode.findPlug( "liqSubdivStitch", &status );
              if ( status == MS::kSuccess ) {
                extraTagPlug.getValue( extraTagValue );
                if ( extraTagValue ) // skip zero values
                  addExtraTags( dstNode, extraTagValue, TAG_STITCH );
              }
            }
          }
        }
        if ( liqglo_useMtorSubdiv ) {	// check mtor subdivisions extra tag
          MPlug extraTagPlug = setNode.findPlug( "mtorSubdivCrease", &status );
          if ( status == MS::kSuccess ) {
            extraTagPlug.getValue( extraTagValue );
            if ( extraTagValue ) // skip zero values
              addExtraTags( dstNode, extraTagValue, TAG_CREASE );
          } else {
            MPlug extraTagPlug = setNode.findPlug( "mtorSubdivCorner", &status );
            if ( status == MS::kSuccess ) {
              extraTagPlug.getValue( extraTagValue );
              if ( extraTagValue ) // skip zero values
                addExtraTags( dstNode, extraTagValue, TAG_CORNER );
            } else {
              MPlug extraTagPlug = setNode.findPlug( "mtorSubdivHole", &status );
              if ( status == MS::kSuccess ) {
                extraTagPlug.getValue( extraTagValue );
                // if ( debugMode ) { printf("==> %s has mtorSubdivHole [%d]\n",depNode.name().asChar(), extraTagValue ); }
                if ( extraTagValue ) // skip zero values
                  addExtraTags( dstNode, extraTagValue, TAG_HOLE );
              }
            }
          }

        }
      } // kSet
    }
  } // for

  MPlug interpolateBoundaryPlug = fnMesh.findPlug( "liqSubdivInterpolateBoundary", &status );
  if ( status == MS::kSuccess ) {
    interpolateBoundaryPlug.getValue( interpolateBoundary );
  }

  bool interpolateBoundaryOld = false;
  MPlug oldinterpolateBoundaryPlug = fnMesh.findPlug( "interpBoundary", &status );
  if ( status == MS::kSuccess ) {
    interpolateBoundaryPlug.getValue( interpolateBoundaryOld );
  }

  bool mtor_interpolateBoundary = false;
  if ( liqglo_useMtorSubdiv ) {
    MPlug mtor_interpolateBoundaryPlug = fnMesh.findPlug( "mtorSubdivInterp", &status );
    if ( status == MS::kSuccess ) {
      mtor_interpolateBoundaryPlug.getValue( mtor_interpolateBoundary );
    }
  }

  interpolateBoundary |= mtor_interpolateBoundary | interpolateBoundaryOld;

  if ( interpolateBoundary )
    addExtraTags( mesh, 0, TAG_BOUNDARY );

}

void liqRibSubdivisionData::addExtraTags( MObject &dstNode, float extraTagValue, SBD_EXTRA_TAG extraTag ) {

  if ( extraTag == TAG_BOUNDARY ) {
    v_tags.push_back( "interpolateboundary" );
    v_nargs.push_back( 0 );		// 0 intargs
    v_nargs.push_back( 0 );		// 0 floatargs
    return;
  }

  MStatus status = MS::kSuccess;
  MFnSet elemSet( dstNode, &status ); // dstNode is maya components set
  if ( status == MS::kSuccess ) {
    MSelectionList members;
    status = elemSet.getMembers( members, true ); // get flatten members list
    if ( status == MS::kSuccess ) {
      for ( unsigned i = 0 ; i < members.length() ; i++ ) {
        MObject component;
        MDagPath dagPath;
        members.getDagPath ( i, dagPath, component );

        switch ( extraTag ) {

        case TAG_CREASE:

          if ( !component.isNull() && component.hasFn( MFn::kMeshEdgeComponent ) ) {
            MItMeshEdge edgeIter( dagPath, component );
            for(  ; !edgeIter.isDone(); edgeIter.next() ) {
              v_tags.push_back( "crease" );
              v_nargs.push_back( 2 );                 // 2 intargs
              v_nargs.push_back( 1 );                 // 1 floatargs
              v_intargs.push_back( edgeIter.index( 0 ) );
              v_intargs.push_back( edgeIter.index( 1 ) );
              v_floatargs.push_back( extraTagValue ); // 1 floatargs
            }
          }
	  break;

        case TAG_CORNER:

          if ( !component.isNull() && component.hasFn( MFn::kMeshVertComponent ) ) {
            MItMeshVertex  vertexIter( dagPath, component );
            for(  ; !vertexIter.isDone(); vertexIter.next() ) {
              v_tags.push_back( "corner" );
              v_nargs.push_back( 1 );                 // 1 intargs
              v_nargs.push_back( 1 );                 // 1 floatargs
              v_intargs.push_back( vertexIter.index() );
              v_floatargs.push_back( extraTagValue ); // 1 floatargs
            }
          }
	  break;
          
        case TAG_HOLE:

          if ( !component.isNull() && component.hasFn( MFn::kMeshPolygonComponent ) ) {
            MItMeshPolygon  faceIter( dagPath, component );
            for(  ; !faceIter.isDone(); faceIter.next() ) {
              v_tags.push_back( "hole" );
              v_nargs.push_back( 1 );                // 1 intargs
              v_nargs.push_back( 0 );                // 0 floatargs
              v_intargs.push_back( faceIter.index() );
            }
          }
	  break;

        case TAG_STITCH:

          if ( !component.isNull() && component.hasFn( MFn::kMeshVertComponent ) ) {
            MItMeshVertex vertexIter( dagPath, component );
            v_tags.push_back( "stitch" );
            v_nargs.push_back( vertexIter.count() + 1 ); // vertex count in chain + 1 integer identifier
            v_nargs.push_back( 0 );                      // 0 floatargs
            v_intargs.push_back( ( int ) extraTagValue );
            for(  ; !vertexIter.isDone(); vertexIter.next() ) {
              v_intargs.push_back( vertexIter.index() );
            }
          }
	  break;
	  
	case TAG_BOUNDARY:
	default:
	  break;	  

        }
	

      }
    }
  }
}
