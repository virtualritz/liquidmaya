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
** Contributor(s): Philippe Leprince.
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
** Liquid Rib pfxToon Curve Data Source
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
    cvColor( NULL ),
    cvOpacity( NULL )
{
  LIQDEBUGPRINTF( "-> creating pfxToon curves\n" );
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

    bool doLines          = true;
    bool doTwist          = false;
    bool doWidth          = true;
    bool doFlatness       = false;
    bool doParameter      = false;
    bool doColor          = true;
    bool doIncandescence  = false;
    bool doTransparency   = true;
    bool doWorldSpace     = false;

    //cout <<"[liquid] >> fill line arrays.... "<<endl;
    status = pfxtoon.getLineData( profileArray, creaseArray, intersectionArray, doLines, doTwist, doWidth, doFlatness, doParameter, doColor, doIncandescence, doTransparency, doWorldSpace );


    if ( status == MS::kSuccess ) {

      //cout <<"[liquid] >> line arrays filled !"<<endl;

      // get the lines and fill the arrays.
      ncurves = profileArray.length();

      {
        MFnDependencyNode pfxNode( pfxToon );
        MString info( "[liquid] pfxToon node " );
        info += pfxNode.name() + " : " + ncurves + " curves.";
        cout << info << endl << flush;
      }

      unsigned int totalNumberOfVertices = 0;

      if ( ncurves > 0 ) {

        nverts = (RtInt*)lmalloc( sizeof( RtInt ) * ( ncurves ) );
        RtFloat* cvPtr      = CVs;
        RtFloat* widthPtr   = curveWidth;
        RtFloat* colorPtr   = cvColor;
        RtFloat* opacityPtr = cvOpacity;

        unsigned i=0;
        for ( ; i<ncurves; i++ ) {

          MRenderLine theLine = profileArray.renderLine( i, &status );

          if ( status == MS::kSuccess ) {

            MVectorArray vertices           = theLine.getLine();
            MDoubleArray width              = theLine.getWidth();
            MVectorArray vertexColor        = theLine.getColor();
            MVectorArray vertexTransparency = theLine.getTransparency();

            //cout <<"line "<<i<<" contains "<<vertices.length()<<" vertices."<<endl;
            //cout <<vertexColor<<endl;

            nverts[i] = vertices.length();
            totalNumberOfVertices += vertices.length();
            unsigned int vertIndex = 0;

            // allocate memory
            CVs = (RtFloat*)realloc( CVs, sizeof( RtFloat ) * ( totalNumberOfVertices * 3 ) );
            if ( CVs == NULL ) {
              MString err("liqRibPfxToonData failed to allocate CV memory !");
              cout <<err<<endl<<flush;
              throw(err);
              return;
            } else cvPtr = CVs + ( totalNumberOfVertices * 3 - nverts[i] * 3 ) ;

            curveWidth = (RtFloat*)realloc( curveWidth, sizeof( RtFloat ) * ( totalNumberOfVertices ) );
            if ( curveWidth == NULL ) {
              MString err("liqRibPfxToonData failed to allocate per vertex width memory !");
              cout <<err<<endl<<flush;
              throw(err);
              return;
            } else widthPtr = curveWidth + ( totalNumberOfVertices - nverts[i] ) ;

            cvColor = (RtFloat*)realloc( cvColor, sizeof( RtFloat ) * ( totalNumberOfVertices * 3 ) );
            if ( cvColor == NULL ) {
              MString err("liqRibPfxToonData failed to allocate CV color memory !");
              cout <<err<<endl<<flush;
              throw(err);
              return;
            } else colorPtr = cvColor + ( totalNumberOfVertices * 3 - nverts[i] * 3 ) ;

            cvOpacity = (RtFloat*)realloc( cvOpacity, sizeof( RtFloat ) * ( totalNumberOfVertices * 3 ) );
            if ( cvOpacity == NULL ) {
              MString err("liqRibPfxToonData failed to allocate CV opacity memory !");
              cout <<err<<endl<<flush;
              throw(err);
              return;
            } else opacityPtr = cvOpacity + ( totalNumberOfVertices * 3 - nverts[i] * 3 ) ;


            for ( ; vertIndex < vertices.length(); vertIndex++ ) {
              *cvPtr      = (RtFloat) vertices[vertIndex].x;                        *cvPtr++;
              *cvPtr      = (RtFloat) vertices[vertIndex].y;                        *cvPtr++;
              *cvPtr      = (RtFloat) vertices[vertIndex].z;                        *cvPtr++;

              *widthPtr   = (RtFloat) ( width[vertIndex] );                         *widthPtr++;

              *colorPtr   = (RtFloat) vertexColor[vertIndex].x ;                    *colorPtr++;
              *colorPtr   = (RtFloat) vertexColor[vertIndex].y ;                    *colorPtr++;
              *colorPtr   = (RtFloat) vertexColor[vertIndex].z ;                    *colorPtr++;

              *opacityPtr = (RtFloat) ( 1.0f - vertexTransparency[vertIndex].x ) ;  *opacityPtr++;
              *opacityPtr = (RtFloat) ( 1.0f - vertexTransparency[vertIndex].y ) ;  *opacityPtr++;
              *opacityPtr = (RtFloat) ( 1.0f - vertexTransparency[vertIndex].z ) ;  *opacityPtr++;
            }

          }
        }

        // store for output
        // cout <<"store P for output... ";
        liqTokenPointer points_pointerPair;
        int test = points_pointerPair.set( "P", rPoint, false, true, false, totalNumberOfVertices );
        if ( test == 0 ) {
          MString err("liqRibPfxToonData: liqTokenPointer failed to allocate CV memory !");
          cout <<err<<endl;
          throw(err);
          return;
        } //else cout <<"points_pointerPair = "<<test<<endl;
        points_pointerPair.setDetailType( rVertex );
        points_pointerPair.setTokenFloats( CVs );
        tokenPointerArray.push_back( points_pointerPair );
        if ( CVs != NULL ) { lfree( CVs ); CVs = NULL; }
        // cout <<"Done !"<<endl;


        // store width params
        // cout <<"store width for output... ";
        liqTokenPointer width_pointerPair;
        test = width_pointerPair.set( "width", rFloat, false, true, false, totalNumberOfVertices );
        if ( test == 0 ) {
          MString err("liqRibPfxToonData: liqTokenPointer failed to allocate width memory !");
          cout <<err<<endl;
          throw(err);
          return;
        }// else cout <<"width_pointerPair = "<<test<<endl;
        width_pointerPair.setDetailType( rVarying );
        width_pointerPair.setTokenFloats( curveWidth );
        tokenPointerArray.push_back( width_pointerPair );
        if ( curveWidth != NULL ) { lfree( curveWidth ); curveWidth = NULL; }
        // cout <<"Done !"<<endl;

        // store color params
        // cout <<"store color for output... ";
        liqTokenPointer color_pointerPair;
        test = color_pointerPair.set( "pfxToon_vtxColor", rColor, false, true, false, totalNumberOfVertices );
        if ( test == 0 ) {
          MString err("liqRibPfxToonData: liqTokenPointer failed to allocate color memory !");
          cout <<err<<endl;
          throw(err);
          return;
        } //else cout <<"color_pointerPair = "<<test<<endl;
        color_pointerPair.setDetailType( rVertex );
        color_pointerPair.setTokenFloats( cvColor );
        tokenPointerArray.push_back( color_pointerPair );
        if ( cvColor != NULL ) { lfree( cvColor ); cvColor = NULL; }
        // cout <<"Done !"<<endl;

        // store opacity params
        // cout <<"store opacity for output... ";
        liqTokenPointer opacity_pointerPair;
        test = opacity_pointerPair.set( "pfxToon_vtxOpacity", rColor, false, true, false, totalNumberOfVertices );
        if ( test == 0 ) {
          MString err("liqRibPfxToonData: liqTokenPointer failed to allocate opacity memory !");
          cout <<err<<endl<<flush;
          throw(err);
          return;
        } //else cout <<"opacity_pointerPair = "<<test<<endl;
        opacity_pointerPair.setDetailType( rVertex );
        opacity_pointerPair.setTokenFloats( cvOpacity );
        tokenPointerArray.push_back( opacity_pointerPair );
        if ( cvOpacity != NULL ) { lfree( cvOpacity ); cvOpacity = NULL; }
        // cout <<"Done !"<<endl;

        addAdditionalSurfaceParameters( pfxToon );

      }

      // delete line arrays
      // cout <<"delete line arrays.... "<<endl;
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
    LIQDEBUGPRINTF( "-> killing pfxToon curves\n" );
    if ( CVs != NULL )        { lfree( CVs );         CVs = NULL;         }
    if ( nverts != NULL )     { lfree( nverts );      nverts = NULL;      }
    if ( curveWidth != NULL ) { lfree( curveWidth );  curveWidth = NULL;  }
    if ( cvColor != NULL )    { lfree( cvColor );     cvColor = NULL;     }
    if ( cvOpacity != NULL )  { lfree( cvOpacity );   cvOpacity = NULL;   }
  }
}

void liqRibPfxToonData::write()
//
//  Description:
//      Write the RIB for this surface
//
{
  LIQDEBUGPRINTF( "-> writing pfxToon curve\n" );

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
  LIQDEBUGPRINTF( "-> comparing pfxToon curves\n");
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


