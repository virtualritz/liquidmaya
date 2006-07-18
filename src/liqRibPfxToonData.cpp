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
** Liquid Rib Nurbs Curve Data Source
** ______________________________________________________________________
*/

// Renderman Headers
extern "C" {
#include <ri.h>
}

// Maya's Headers
#include <maya/MDoubleArray.h>
#include <maya/MItCurveCV.h>
#include <maya/MPoint.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MPlug.h>

#include <maya/MFnPfxGeometry.h>
#include <maya/MRenderLineArray.h>
#include <maya/MRenderLine.h>
#include <maya/MVectorArray.h>
#include <maya/MIntArray.h>
#include <maya/MFloatArray.h>
#include <maya/MColorArray.h>
#include <maya/MDagModifier.h>
#include <maya/MSelectionList.h>
#include <maya/MMatrix.h>


#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liqRibData.h>
#include <liqRibPfxToonData.h>

extern int debugMode;
extern MString liqglo_renderCamera;

liqRibPfxToonData::liqRibPfxToonData( MObject pfxToon )
//
//  Description:
//      create a RIB compatible representation of a Maya pfxToon node as RiCurves
//
  : nverts( NULL ),
    CVs( NULL ),
    curveWidth( NULL ),
    cvColor( NULL )
{
  LIQDEBUGPRINTF( "-> creating pfxToon nurbs curve\n" );
  MStatus status = MS::kSuccess;

  // update the pfxToon node with the renderCamera's position
  // otherwise the resulting outline might be incorrect
  MDagPath cameraPath;
  MSelectionList camList;
  camList.add( liqglo_renderCamera );
  camList.getDagPath( 0, cameraPath );
  MMatrix cam_mat = cameraPath.inclusiveMatrix();
  MFnDependencyNode pfxToonNode( pfxToon );
  pfxToonNode.findPlug( "cameraPointX" ).setValue( cam_mat(3,0) );
  pfxToonNode.findPlug( "cameraPointY" ).setValue( cam_mat(3,1) );
  pfxToonNode.findPlug( "cameraPointZ" ).setValue( cam_mat(3,2) );


  //cout <<"[liquid] >> getting the pfxToon node... "<<endl;

  MFnPfxGeometry pfxtoon( pfxToon, &status );

  if ( status == MS::kSuccess ) {

    //cout <<"[liquid] >> got the pfxToon node ! : "<<pfxtoon.partialPathName()<<endl;

    //cout <<"[liquid] >> create line arrays.... "<<endl;
    MRenderLineArray profileArray;
    MRenderLineArray creaseArray;
    MRenderLineArray intersectionArray;
    MRenderLineArray copy;

    bool doLines          = true;
    bool doTwist          = true;
    bool doWidth          = true;
    bool doFlatness       = false;
    bool doParameter      = false;
    bool doColor          = true;
    bool doIncandescence  = false;
    bool doTransparency   = false;
    bool doWorldSpace     = false;

    //cout <<"[liquid] >> fill line arrays.... "<<endl;
    status = pfxtoon.getLineData( profileArray, creaseArray, intersectionArray, doLines, doTwist, doWidth, doFlatness, doParameter, doColor, doIncandescence, doTransparency, doWorldSpace );

    //cout <<"[liquid] >> profileArray contains "<<profileArray.length()<<" curves !"<<endl;
    //cout <<"[liquid] >> creaseArray contains "<<creaseArray.length()<<" curves !"<<endl;
    //cout <<"[liquid] >> intersectionArray contains "<<intersectionArray.length()<<" curves !"<<endl;

    if ( status == MS::kSuccess ) {

      //cout <<"[liquid] >> line arrays filled !"<<endl;

      // get the lines and fill the arrays.
      ncurves = profileArray.length();

      unsigned int totalNumberOfVertices = 0;
      MFloatArray CV, vWidth, vColor;

      if ( ncurves > 0 ) {

        nverts = (RtInt*)lmalloc( sizeof( RtInt ) * ( ncurves ) );

        unsigned i=0;
        for ( ; i<ncurves; i++ ) {

          MRenderLine theLine = profileArray.renderLine( i, &status );

          if ( status == MS::kSuccess ) {

            MVectorArray vertices = theLine.getLine();
            MDoubleArray width    = theLine.getWidth();
            MVectorArray vertexColor = theLine.getColor();

            //cout <<"line "<<i<<" contains "<<vertices.length()<<" vertices."<<endl;
            //cout <<vertexColor<<endl;

            nverts[i] = vertices.length();
            totalNumberOfVertices += vertices.length();
            unsigned int vertIndex = 0;

            for ( ; vertIndex < vertices.length(); vertIndex++ ) {
              CV.append( (float) vertices[vertIndex].x );
              CV.append( (float) vertices[vertIndex].y );
              CV.append( (float) vertices[vertIndex].z );
              vWidth.append( (float) width[vertIndex] );
              vColor.append( (float) vertexColor[vertIndex].x );
              vColor.append( (float) vertexColor[vertIndex].y );
              vColor.append( (float) vertexColor[vertIndex].z );
            }

          }
        }

        // store for output
        //cout <<"store for output"<<endl;
        liqTokenPointer profile_pointsPointerPair;
        profile_pointsPointerPair.set( "P", rPoint, false, true, false, totalNumberOfVertices );
        profile_pointsPointerPair.setDetailType( rVertex );
        CVs   = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( totalNumberOfVertices * 3 ) );
        CV.get( CVs );
        profile_pointsPointerPair.setTokenFloats( CVs );
        tokenPointerArray.push_back( profile_pointsPointerPair );

        // store width params
        liqTokenPointer profile_widthPointerPair;
        profile_widthPointerPair.set( "width", rFloat, false, true, false, totalNumberOfVertices );
        profile_widthPointerPair.setDetailType( rVarying );
        curveWidth = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( totalNumberOfVertices ) );
        vWidth.get( curveWidth );
        profile_widthPointerPair.setTokenFloats( curveWidth );
        tokenPointerArray.push_back( profile_widthPointerPair );

        // store color params
        liqTokenPointer profile_colorPointerPair;
        profile_colorPointerPair.set( "pfxToon_vtxColor", rColor, false, true, false, totalNumberOfVertices );
        profile_colorPointerPair.setDetailType( rVertex );
        cvColor = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( totalNumberOfVertices * 3 ) );
        vColor.get( cvColor );
        profile_colorPointerPair.setTokenFloats( cvColor );
        tokenPointerArray.push_back( profile_colorPointerPair );

        addAdditionalSurfaceParameters( pfxToon );

      }

      // delete line arrays
      //cout <<"[liquid] >> delete line arrays.... "<<endl;
      profileArray.deleteArray();
      creaseArray.deleteArray();
      intersectionArray.deleteArray();

    }

  }

}

liqRibPfxToonData::~liqRibPfxToonData()
//  Description:
//      class destructor
{
  if ( ncurves > 0 ) {
    // Free all arrays
    LIQDEBUGPRINTF( "-> killing nurbs curve\n" );
    if ( CVs != NULL ) { lfree( CVs ); CVs = NULL; }
    if ( nverts != NULL ) { lfree( nverts ); nverts = NULL; }
    if ( curveWidth != NULL ) { lfree( curveWidth ); curveWidth = NULL; }
    if ( cvColor != NULL ) { lfree( cvColor ); cvColor = NULL; }
  }
}

void liqRibPfxToonData::write()
//
//  Description:
//      Write the RIB for this surface
//
{
  LIQDEBUGPRINTF( "-> writing nurbs curve\n" );

  if ( ncurves > 0 ) {

    unsigned numTokens = tokenPointerArray.size();
    RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * numTokens );
    RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * numTokens );

    assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );

    RiCurvesV( "linear", ncurves, nverts, "nonperiodic", numTokens, tokenArray, pointerArray );

  }
}

bool liqRibPfxToonData::compare( const liqRibData & otherObj ) const
//
//  Description:
//      Compare this curve to the other for the purpose of determining
//      if it is animated.
//
{
  LIQDEBUGPRINTF( "-> comparing nurbs curve\n");
  if ( otherObj.type() != MRT_PfxToon ) return false;
  //const liqRibPfxToonData & other = (liqRibPfxToonData&)otherObj;

  // // Check CVs
  // last = nverts[0] * 3;
  // for ( i = 0; i < last; ++i ) {
  //     if ( !equiv( CVs[i], other.CVs[i] ) ) return false;
  // }

  return true;
}

ObjectType liqRibPfxToonData::type() const
//
//  Description:
//      return the geometry type
//
{
  LIQDEBUGPRINTF( "-> returning pfxToon curve type\n" );
  return MRT_PfxToon;
}


