
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


// Renderman headers
extern "C" {
#include <ri.h>
}

// Maya headers
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshVertex.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MSelectionList.h>

// Liquid headers
#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liqRibSubdivisionData.h>

// Standard/Boost headers
#include <boost/scoped_array.hpp>
#include <boost/shared_array.hpp>


#if defined( AIR ) || defined( DELIGHT )
  #define PERPIECE rFaceVertex
#else
  #define PERPIECE rFaceVarying
#endif

using namespace boost;

extern int debugMode;
extern bool liqglo_outputMeshUVs;
extern bool liqglo_useMtorSubdiv;  // interpret mtor subdiv attributes

/** Create a RIB compatible subdivision surface representation using a Maya polygon mesh.
 */
liqRibSubdivisionData::liqRibSubdivisionData( MObject mesh )
  : numFaces( 0 ),
    numPoints ( 0 ),
    nverts(),
    verts(),
    vertexParam( NULL ),
    interpolateBoundary( false )
{
  LIQDEBUGPRINTF( "-> creating subdivision surface\n" );
  MFnMesh fnMesh( mesh );
  name = fnMesh.name();

  /*
  MStatus status;

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

  // UV sets -----------------
  //
  const unsigned numSTs = fnMesh.numUVs();
  const unsigned numUVSets = fnMesh.numUVSets();
  MString currentUVSetName;
  MStringArray extraUVSetNames;
  fnMesh.getCurrentUVSetName( currentUVSetName );
  {
    MStringArray UVSetNames;
    fnMesh.getUVSetNames( UVSetNames );

    for ( unsigned i=0; i<numUVSets; i++ ) {
      if ( UVSetNames[i] != currentUVSetName )
        extraUVSetNames.append( UVSetNames[i] );
    }
    //cout <<UVSetNames<<endl;
  }

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
  liqTokenPointer pFaceVertexSPointer;
  liqTokenPointer pFaceVertexTPointer;

  // Allocate memory and tokens
  numFaces = numFaces;
  nverts = shared_array< RtInt >( new RtInt[ numFaces ] );
  verts = shared_array< RtInt >( new RtInt[ numFaceVertices ] );

  pointsPointerPair.set( "P", rPoint, numPoints );
  pointsPointerPair.setDetailType( rVertex );

  std::vector<liqTokenPointer> UVSetsArray;
  UVSetsArray.reserve( 1 + extraUVSetNames.length() );

  if ( numSTs > 0 ) {
    liqTokenPointer pFaceVertexPointerPair;

    pFaceVertexPointerPair.set( "st", rFloat, numFaceVertices, 2 );
    pFaceVertexPointerPair.setDetailType( PERPIECE );

    UVSetsArray.push_back( pFaceVertexPointerPair );

    for ( unsigned j( 0 ); j<extraUVSetNames.length(); j++) {
      liqTokenPointer pFaceVertexPointerPair;

      pFaceVertexPointerPair.set( extraUVSetNames[j].asChar(), rFloat, numFaceVertices, 2 );
      pFaceVertexPointerPair.setDetailType( PERPIECE );

      UVSetsArray.push_back( pFaceVertexPointerPair );
    }

    if( liqglo_outputMeshUVs ) {
      // Match MTOR, which also outputs face-varying STs as well for some reason - Paul
      // not anymore - Philippe
      pFaceVertexSPointer.set( "u", rFloat, numFaceVertices );
      pFaceVertexSPointer.setDetailType( PERPIECE );

      pFaceVertexTPointer.set( "v", rFloat, numFaceVertices );
      pFaceVertexTPointer.setDetailType( PERPIECE );
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

      if ( UVSetsArray.size() ) {
        fnMesh.getPolygonUV( face, count, S, T );

        UVSetsArray[0].setTokenFloat( faceVertex, 0, S );
        UVSetsArray[0].setTokenFloat( faceVertex, 1, 1 - T );

        for ( unsigned j=1; j<=extraUVSetNames.length(); j++ ) {
          fnMesh.getPolygonUV( face, count, S, T, &extraUVSetNames[j] );

          UVSetsArray[j].setTokenFloat( faceVertex, 0, S );
          UVSetsArray[j].setTokenFloat( faceVertex, 1, 1 - T );
        }

        if( liqglo_outputMeshUVs ) {
          // Match MTOR, which always outputs face-varying STs as well for some reason - Paul
          pFaceVertexSPointer.setTokenFloat( faceVertex, S );
          pFaceVertexTPointer.setTokenFloat( faceVertex, 1 - T );
        }
      }

      ++faceVertex;
    }

    ++face;
  }

  // Add tokens to array and clean up after
  tokenPointerArray.push_back( pointsPointerPair );

  if( UVSetsArray.size() ) {
    tokenPointerArray.insert( tokenPointerArray.end(), UVSetsArray.begin(), UVSetsArray.end() );
  }

  if( liqglo_outputMeshUVs ) {
    assert( !pFaceVertexSPointer );
    tokenPointerArray.push_back( pFaceVertexSPointer );
    assert( !pFaceVertexTPointer );
    tokenPointerArray.push_back( pFaceVertexTPointer );
  }

  addAdditionalSurfaceParameters( mesh );
}


/** Write the RIB for this mesh.
 */
void liqRibSubdivisionData::write()
{
  LIQDEBUGPRINTF( "-> writing subdivision surface\n" );

  unsigned numTokens( tokenPointerArray.size() );
  scoped_array< RtToken > tokenArray( new RtToken[ numTokens ] );
  scoped_array< RtPointer > pointerArray( new RtPointer[ numTokens ] );
  assignTokenArraysV( tokenPointerArray, tokenArray.get(), pointerArray.get() );

  RiSubdivisionMeshV( "catmull-clark", numFaces, nverts.get(), verts.get(),
                      v_tags.size(), v_tags.size() ? &v_tags[0] : NULL,
                      v_nargs.size() ? &v_nargs[0] : NULL,
                      v_intargs.size() ? &v_intargs[0] : NULL,
                      v_floatargs.size() ? &v_floatargs[0] : NULL,
                      numTokens, tokenArray.get(), pointerArray.get() );
}

/** Compare this mesh to the other for the purpose of determining if its animated
 */
bool liqRibSubdivisionData::compare( const liqRibData & otherObj ) const
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

/** Return the geometry type.
 */
ObjectType liqRibSubdivisionData::type() const
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
