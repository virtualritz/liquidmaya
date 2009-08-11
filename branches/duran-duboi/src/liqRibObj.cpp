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
**
*/

/* ______________________________________________________________________
**
** Liquid Rib Object Source
** ______________________________________________________________________
*/


// Renderman Headers
extern "C" {
#include <ri.h>
}

// Maya's Headers
#include <maya/MFnDagNode.h>
#include <maya/MDagPathArray.h>
#include <maya/MPlug.h>
#include <maya/MPxNode.h>

#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liqRibObj.h>
#include <liqRibSurfaceData.h>
#include <liqRibLightData.h>
#include <liqRibLocatorData.h>
#include <liqRibMeshData.h>
#include <liqRibParticleData.h>
#include <liqRibNuCurveData.h>
#include <liqRibSubdivisionData.h>
#include <liqRibMayaSubdivisionData.h>
#include <liqRibClipPlaneData.h>
#include <liqRibCoordData.h>
#include <liqRibGenData.h>
#include <liqRibCustomNode.h>
#include <liqRibPfxToonData.h>
#include <liqRibPfxHairData.h>

extern int debugMode;
extern bool liqglo_useMtorSubdiv;


liqRibObj::liqRibObj( const MDagPath &path, ObjectType objType )
//
//  Description:
//      Create a RIB representation of the given node in the DAG as a ribgen!
//
: instanceMatrices( NULL ),
  objectHandle( NULL ),
  referenceCount( 0 ),
  data( NULL )
{
    LIQDEBUGPRINTF( "-> creating dag node handle rep\n");

    MStatus status;
    MObject obj = path.node();
    MObject skip;

    written = 0;
    lightSources = NULL;
    MFnDagNode nodeFn( obj );

    // Store the matrices for all instances of this node at this time
    // so that they can be used to determine if this node's transformation
    // is animated.  This information is used for doing motion blur.
    //
    MDagPathArray instanceArray;
    nodeFn.getAllPaths( instanceArray );
    unsigned last = instanceArray.length();
    instanceMatrices = new MMatrix[last];
    for ( unsigned i = 0; i < last; i++ ) {
      instanceMatrices[i] = instanceArray[i].inclusiveMatrix();
    }

    LIQDEBUGPRINTF( "-> checking handles display status\n");

    ignore = !areObjectAndParentsVisible( path );
    if ( !ignore ) {
      ignore = !areObjectAndParentsTemplated( path );
    }
    if ( !ignore ) {
      ignore = !isObjectPrimaryVisible( path );
    }

    // check that the shape's transform does not a have a liqIgnoreShapes attribute.
    ignoreShapes = false;
    {
      MDagPath searchPath( path );
      while ( searchPath.apiType() != ( MFn::kTransform ) && searchPath.length() > 1 ) {
        searchPath.pop();
      }
      MFnDagNode transformDN( searchPath );
      status.clear();
      MPlug ignorePlug = transformDN.findPlug( "liqIgnoreShapes", &status );
      if ( status == MS::kSuccess ) {
        //cout <<"found ignoreShapes on "<<path.fullPathName().asChar()<<endl;
        ignorePlug.getValue( ignoreShapes );
      }
    }


    ignoreShadow = !isObjectCastsShadows( path );
    if ( !ignoreShadow ) {
      ignoreShadow = !areObjectAndParentsVisible( path );
    }
    if ( !ignoreShadow ) {
      ignoreShadow = !areObjectAndParentsTemplated( path );
    }

    // don't bother storing it if it's not going to be visible!
    LIQDEBUGPRINTF( "-> about to create rep\n");

    if ( !ignore || !ignoreShadow ) {
      if ( objType == MRT_RibGen ) {
        type = MRT_RibGen;
        data = new liqRibGenData( obj, path );
      } else {

        // check to see if object's class is derived from liqCustomNode
        liqCustomNode *customNode = NULL;
        MFnDependencyNode mfnDepNode(obj, &status);
        if (status) {
          MPxNode *mpxNode = mfnDepNode.userNode();
          if (mpxNode) {
            customNode = dynamic_cast<liqCustomNode*>(mpxNode); // will be NULL if cast is not invalid
          }
        }

        // Store the geometry/light/shader data for this object in RIB format
        if (customNode) {
          type = MRT_Custom;
          if ( !ignoreShapes ) data = new liqRibCustomNode( obj, customNode );
          else data = new liqRibCustomNode( skip, customNode );
        } else if ( obj.hasFn(MFn::kNurbsSurface) ) {
          type = MRT_Nurbs;
          if ( !ignoreShapes ) data = new liqRibSurfaceData( obj );
          else data = new liqRibSurfaceData( skip );
        } else if ( obj.hasFn(MFn::kSubdiv) ) {
          type = MRT_Subdivision;
          if ( !ignoreShapes ) data = new liqRibMayaSubdivisionData( obj );
          else data = new liqRibMayaSubdivisionData( skip );
        } else if ( obj.hasFn(MFn::kNurbsCurve) ) {
          type = MRT_NuCurve;
          if ( !ignoreShapes ) data = new liqRibNuCurveData( obj );
          else data = new liqRibNuCurveData( skip );
        } else if ( obj.hasFn( MFn::kPfxToon ) ) {
          type = MRT_PfxToon;
          if ( !ignoreShapes ) data = new liqRibPfxToonData( obj );
          else data = new liqRibPfxToonData( skip );
        } else if ( obj.hasFn( MFn::kPfxHair ) ) {
          type = MRT_PfxHair;
          if ( !ignoreShapes ) data = new liqRibPfxHairData( obj );
          else data = new liqRibPfxHairData( skip );
        } else if ( obj.hasFn(MFn::kParticle) ) {
          type = MRT_Particles;
          if ( !ignoreShapes ) data = new liqRibParticleData( obj );
          else data = new liqRibParticleData( skip );
        } else if ( obj.hasFn(MFn::kMesh) ) {
          // we know we are dealing with a mesh here, now we check to see if it
          // needs to be handled as a subdivision surface
          bool usingSubdiv = false;
          MPlug subdivPlug = nodeFn.findPlug( "liqSubdiv", &status );
          if ( status == MS::kSuccess ) {
            subdivPlug.getValue( usingSubdiv );
          }

          bool usingSubdivOld = false;
          MPlug oldSubdivPlug = nodeFn.findPlug( "subDMesh", &status );
          if ( status == MS::kSuccess ) {
            oldSubdivPlug.getValue( usingSubdivOld );
          }

          // make Liquid understand MTOR subdiv attribute
          bool usingSubdivMtor = false;
          if ( liqglo_useMtorSubdiv ) {
            MPlug mtorSubdivPlug = nodeFn.findPlug( "mtorSubdiv", &status );
            if ( status == MS::kSuccess ) {
              mtorSubdivPlug.getValue( usingSubdivMtor );
            }
          }

          usingSubdiv |= usingSubdivMtor | usingSubdivOld;

          if ( usingSubdiv ) {
            // we've got a subdivision surface
            type = MRT_Subdivision;
            if ( !ignoreShapes ) data = new liqRibSubdivisionData( obj );
            else data = new liqRibSubdivisionData( skip );
            type = data->type();
          } else {
            // it's a regular mesh
            type = MRT_Mesh;
            if ( !ignoreShapes ) data = new liqRibMeshData( obj );
            else data = new liqRibMeshData( skip );
            type = data->type();
          }
        } else if ( obj.hasFn(MFn::kLight)) {
          type = MRT_Light;
          data = new liqRibLightData( path );
        } else if ( mfnDepNode.typeName() == "liquidCoordSys" ) {
          MStatus status;
          int coordSysType = 0;
          MPlug typePlug = mfnDepNode.findPlug( "type", &status );
          if ( MS::kSuccess == status ) typePlug.getValue( coordSysType );
          if ( coordSysType == 5 ) {
            type = MRT_ClipPlane;
            data = new liqRibClipPlaneData( obj );
          } else {
            type = MRT_Coord;
            data = new liqRibCoordData( obj );
          }
        } else if ( obj.hasFn(MFn::kLocator) && mfnDepNode.typeName() != "liquidCoordSys" ) {
          type = MRT_Locator;
          data = new liqRibLocatorData( obj );
        }
      }
      data->objDagPath = path;
    }
    LIQDEBUGPRINTF( "-> done creating rep\n");
}

liqRibObj::~liqRibObj()
//
//  Description:
//      Class destructor
//
{
  LIQDEBUGPRINTF( "-> killing ribobj data\n");
  delete data;
  LIQDEBUGPRINTF( "-> killing ribobj matrices\n");
  delete [] instanceMatrices;
  LIQDEBUGPRINTF( "-> finished killing ribobj\n");
}

inline RtObjectHandle liqRibObj::handle() const
//
//  Description:
//      return the RenderMan instance handle.  This is used to refer to
//      RIB data that was previously written in the frame prologue.
//
{
  return objectHandle;
}

inline void liqRibObj::setHandle( RtObjectHandle handle )
//
//  Description:
//      set the RenderMan instance handle
//
{
  objectHandle = handle;
}

RtLightHandle liqRibObj::lightHandle() const
//
//  Description:
//      return the RenderMan handle handle for this light
//
{
  LIQDEBUGPRINTF( "-> creating light node handle rep\n");
  //assert( type == MRT_Light );
  RtLightHandle lHandle = NULL;
  if ( type == MRT_Light ) {
    liqRibLightData * light = (liqRibLightData*)data;
    lHandle = light->lightHandle();
  }
  return lHandle;
}

AnimType liqRibObj::compareMatrix(const liqRibObj *o, int instance )
//
//  Description:
//      compare the two object's world transform matrices.  This method also
//      works with instanced objects.  This comparision is used to determine
//      if motion blurring should be done.
//
{
  LIQDEBUGPRINTF( "-> comparing rib node handle rep matrix\n");
  return (matrix( instance ) == o->matrix( instance ) ? MRX_Const : MRX_Animated);
}

AnimType liqRibObj::compareBody(const liqRibObj *o)
//
//  Description:
//      compare the two object's geometry.  This comparision is used to
//      determine if motion blurring should be done
//
{
  LIQDEBUGPRINTF( "-> comparing rib node handle body\n");
  //cout <<"-> comparing rib node handle body"<<endl;
  AnimType cmp = MRX_Const;
  if (data == NULL || o->data == NULL) {
    cmp = MRX_Const;
  } else {
    if ( !data->compare( *(o->data) ) ) {
      cmp = MRX_Animated;
    }
  }
  return cmp;
}

void liqRibObj::writeObject()
//
//  Description:
//      write the object directly.  We do not get a RIB handle in this case
//
{
  LIQDEBUGPRINTF( "-> writing rib node handle rep\n");
  if ( NULL != data ) {
    if ( MRT_Light == type ) {
      data->write();
    } else {
      if ( type == MRT_Nurbs ) {
        liqRibSurfaceData * surfData = (liqRibSurfaceData*)data;
        if ( surfData->hasTrimCurves() ) {
          surfData->writeTrimCurves();
        }
      }
      data->write();
    }
  }
}

MMatrix liqRibObj::matrix( int instance ) const
//
//  Description:
//      return the inclusive matrix for the given instance
//
{
  assert(instance>=0);
  return instanceMatrices[instance];
}

void liqRibObj::setMatrix( int instance, MMatrix matrix )
{
  assert(instance>=0);
  instanceMatrices[instance] = matrix;
}


void liqRibObj::ref()
//
//  Description:
//      bump reference count up by one
//
{
  if ( debugMode ) {
    //printf("-> referencing ribobj: %s\n", data->objDagPath.fullPathName().asChar() );
    printf("-> number of ribobj references prior: %d\n", referenceCount );
  }
  referenceCount++;
}

void liqRibObj::unref()
//
//  Description:
//      bump reference count down by one and delete if necessary
//
{
  if ( debugMode ) {
    printf("-> unreferencing ribobj.\n" );
    printf("-> number of ribobj references prior: %d\n", referenceCount );
  }
  assert( referenceCount >= 0 );

  referenceCount--;
  if ( referenceCount <= 0 ) {
    LIQDEBUGPRINTF(  "-> deleting this ribobj.\n" );
    delete this;
  }
}
