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
** The RenderMan (R) Interface Procedures and Protocol are: Copyright 1988,
** 1989, Pixar All Rights Reserved
**
**
** RenderMan (R) is a registered trademark of Pixar
**
*/

/* ______________________________________________________________________
** 
** Liquid Attach __Pref Attribute Source 
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
#include <maya/MSelectionList.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MArgList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItSurfaceCV.h>
#include <maya/MPointArray.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MPxCommand.h>

#include <liqAttachPrefAttribute.h>
#include <liqEntropyRenderer.h>
#include <liqPrmanRenderer.h>
#include <liqAqsisRenderer.h>
#include <liqDelightRenderer.h>
#include <liqMemory.h>


liqAttachPrefAttribute::~liqAttachPrefAttribute()
{
  // nothing else needed
}

void* liqAttachPrefAttribute::creator()
{
  return new liqAttachPrefAttribute;
}

MStatus	liqAttachPrefAttribute::doIt( const MArgList& args )
{
  MFnTypedAttribute tAttr;
  MStatus status;
    
  for ( unsigned int i = 0; i < args.length(); i++ ) {
    MSelectionList		nodeList;
    nodeList.add( args.asString( i, &status ) );
    MObject depNodeObj;
    nodeList.getDependNode(0, depNodeObj);
    MFnDependencyNode depNode( depNodeObj );
    MObject prefAttr;

    if (liquidRenderer().requires(liqRenderer::__PREF)) {
      prefAttr = tAttr.create( "rmanP__Pref", "rmanP__Pref", MFnData::kPointArray );
    } else {
      prefAttr = tAttr.create( "rmanPPref", "rmanPPref", MFnData::kPointArray );
    }

    if ( depNodeObj.hasFn( MFn::kNurbsSurface )) {
      MFnNurbsSurface nodeFn( depNodeObj );

      MPointArray nodePArray;

      MItSurfaceCV cvs( depNodeObj, liquidRenderer().requires(liqRenderer::SWAPPED_UVS) == false);

      while(!cvs.isDone()) {
        while(!cvs.isRowDone()) {
          MPoint pt = cvs.position(MSpace::kObject);
          nodePArray.append(pt);
          cvs.next();
        }
        cvs.nextRow();
      }

      nodeFn.addAttribute( prefAttr );
      MFnPointArrayData pArrayData;

      MObject prefDefault = pArrayData.create( nodePArray );
      MPlug nodePlug( depNodeObj, prefAttr );
      nodePlug.setValue( prefDefault );

    } else if ( depNodeObj.hasFn( MFn::kMesh ) ) {
      MFnMesh		nodeFn( depNodeObj );
      MPointArray		nodePArray;
      unsigned int	count;

      nodeFn.addAttribute( prefAttr );

      // TODO: do we need to account for the altMeshExport algo that's
      // used in liquidRibMeshData? 
      for ( MItMeshPolygon polyIt( depNodeObj ); !polyIt.isDone(); polyIt.next()) {
        count = polyIt.polygonVertexCount();

        do {
          count--;
          unsigned	normalIndex = polyIt.normalIndex( count );
          MPoint	nodePoint = polyIt.point( count );

          nodePArray.set( nodePoint, normalIndex );

        } while (count != 0);
      }

      MFnPointArrayData pArrayData;
      MObject prefDefault = pArrayData.create( nodePArray );
      MPlug nodePlug( depNodeObj, prefAttr );
      nodePlug.setValue( prefDefault );
    }
  }

  return MS::kSuccess;
}
