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

// Standard Headers
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <iostream.h>

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
#else
#include <unistd.h>
#include <stdlib.h>
#include <alloca.h>
#endif

// Maya's Headers
#include <maya/MStringArray.h>
#include <maya/MFnDependencyNode.h>
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
#include <liqRibCoordData.h>
#include <liqRibGenData.h>
#include <liqRibCustomNode.h>
#include <liqMemory.h>

extern int debugMode;


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
    if ( debugMode ) { printf("-> creating dag node handle rep\n"); }
    
    MStatus status;
    MObject obj = path.node();
    
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

    if ( debugMode ) { printf("-> checking handles display status\n"); }

    ignore = !areObjectAndParentsVisible( path );
    if ( !ignore ) {
      ignore = !areObjectAndParentsTemplated( path );
    }
    if ( !ignore ) {
      ignore = !isObjectPrimaryVisible( path );
    }
    ignoreShadow = !isObjectCastsShadows( path );
    if ( !ignoreShadow ) {
      ignoreShadow = !areObjectAndParentsVisible( path );
    }
    if ( !ignoreShadow ) {
      ignoreShadow = !areObjectAndParentsTemplated( path );
    }

    // don't bother storing it if it's not going to be visible!
    if ( debugMode ) { printf("-> about to create rep\n"); }

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
          data = new liqRibCustomNode( obj, customNode );
        } else if ( obj.hasFn(MFn::kNurbsSurface) ) {
          type = MRT_Nurbs;
          data = new liqRibSurfaceData( obj );
        } else if ( obj.hasFn(MFn::kNurbsCurve) ) {
          type = MRT_NuCurve;
          data = new liqRibNuCurveData( obj );
        } else if ( obj.hasFn(MFn::kParticle) ) {
          type = MRT_Particles;
          data = new liqRibParticleData( obj );
        } else if ( obj.hasFn(MFn::kMesh) ) {
          // we know we are dealing with a mesh here, now we check to see if it
          // needs to be handled as a subdivision surface
          bool usingSubD = false;
          MPlug subDivPlug = nodeFn.findPlug( "subDMesh", &status );
          if ( status == MS::kSuccess ) {
            subDivPlug.getValue( usingSubD );
          }
          if ( usingSubD ) {
            // we've got a subdivision surface
            type = MRT_Subdivision;
            data = new liqRibSubdivisionData( obj );
            type = data->type();
          } else {
            // it's a regular mesh
            type = MRT_Mesh;
            data = new liqRibMeshData( obj );
            type = data->type();
          }
        } else if ( obj.hasFn(MFn::kLight)) {
          type = MRT_Light;
          data = new liqRibLightData( path );
        } else if ( obj.hasFn(MFn::kPlace3dTexture)) {
          type = MRT_Coord;
          data = new liqRibCoordData( obj );
        } else if ( obj.hasFn(MFn::kLocator) ) {
          type = MRT_Locator;
          data = new liqRibLocatorData( obj );
        }
      }
      data->objDagPath = path;
    }
    if ( debugMode ) { printf("-> done creating rep\n"); }
}

liqRibObj::~liqRibObj()
//
//  Description: 
//      Class destructor
//  
{
  if ( debugMode ) { printf("-> killing ribobj data\n"); }
  delete data;
  if ( debugMode ) { printf("-> killing ribobj matrices\n"); }
  delete [] instanceMatrices;
  if ( debugMode ) { printf("-> finished killing ribobj\n"); }
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
  if ( debugMode ) { printf("-> creating light node handle rep\n"); }
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
  if ( debugMode ) { printf("-> comparing rib node handle rep matrix\n"); }
  return (matrix( instance ) == o->matrix( instance ) ? MRX_Const : MRX_Animated);
}

AnimType liqRibObj::compareBody(const liqRibObj *o)
// 
//  Description:
//      compare the two object's geometry.  This comparision is used to
//      determine if motion blurring should be done 
//
{
  if ( debugMode ) { printf("-> comparing rib node handle body\n"); }
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
  if ( debugMode ) { printf("-> writing rib node handle rep\n"); }
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
    if ( debugMode ) { printf( "-> deleting this ribobj.\n" ); }
    delete this;   
  }
}
