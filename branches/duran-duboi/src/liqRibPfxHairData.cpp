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
** Liquid Rib pfxHair Curve Data Source
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


#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liqRibData.h>
#include <liqRibPfxHairData.h>

extern int debugMode;

liqRibPfxHairData::liqRibPfxHairData( MObject pfxHair )
//
//  Description:
//      create a RIB compatible representation of a Maya pfxHair node as RiCurves
//
  : nverts( NULL ),
    CVs( NULL ),
    curveWidth( NULL ),
    cvColor( NULL ),
    cvOpacity( NULL )
{
  LIQDEBUGPRINTF( "-> creating pfxHair curve\n" );
  MStatus status = MS::kSuccess;

  //cout <<"[liquid] >> getting the pfxHair node... "<<endl;

  MFnPfxGeometry pfxhair( pfxHair, &status );

  if ( status == MS::kSuccess ) {

    //cout <<"[liquid] >> got the pfxHair node ! : "<<pfxtoon.partialPathName()<<endl;

    //cout <<"[liquid] >> create line arrays.... "<<endl;
    MRenderLineArray profileArray;
    MRenderLineArray creaseArray;
    MRenderLineArray intersectionArray;
    MRenderLineArray copy;

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
    status = pfxhair.getLineData( profileArray, creaseArray, intersectionArray, doLines, doTwist, doWidth, doFlatness, doParameter, doColor, doIncandescence, doTransparency, doWorldSpace );

    if ( status == MS::kSuccess ) {

      //cout <<"[liquid] >> line arrays filled !"<<endl;

      // get the lines and fill the arrays.
      ncurves = profileArray.length();

      {
        MFnDependencyNode pfxNode( pfxHair );
        MString info( "[liquid] pfxHair node " );
        info += pfxNode.name() + " : " + ncurves + " curves.";
        cout << info << endl << flush;
      }

      unsigned int totalNumberOfVertices = 0, totalNumberOfSpans = 0;

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

            MVectorArray vertices = theLine.getLine();
            MDoubleArray width    = theLine.getWidth();
            MVectorArray vertexColor = theLine.getColor();
            MVectorArray vertexTransparency = theLine.getTransparency();

            //cout <<"line "<<i<<" contains "<<vertices.length()<<" vertices."<<endl;
            //cout <<vertexTransparency<<endl;

            nverts[i] = vertices.length() + 2;
            totalNumberOfVertices += vertices.length() + 2;
            totalNumberOfSpans += vertices.length();
            unsigned int vertIndex = 0;

            // allocate memory
            CVs = (RtFloat*)realloc( CVs, sizeof( RtFloat ) * ( totalNumberOfVertices * 3 ) );
            if ( CVs == NULL ) {
              MString err("liqRibPfxHairData failed to allocate CV memory !");
              cout <<err<<endl<<flush;
              throw(err);
              return;
            } else cvPtr = CVs + ( totalNumberOfVertices * 3 - nverts[i] * 3 ) ;

            curveWidth = (RtFloat*)realloc( curveWidth, sizeof( RtFloat ) * ( totalNumberOfSpans ) );
            if ( curveWidth == NULL ) {
              MString err("liqRibPfxHairData failed to allocate per vertex width memory !");
              cout <<err<<endl<<flush;
              throw(err);
              return;
            } else widthPtr = curveWidth + ( totalNumberOfSpans - vertices.length() ) ;

            cvColor = (RtFloat*)realloc( cvColor, sizeof( RtFloat ) * ( totalNumberOfVertices * 3 ) );
            if ( cvColor == NULL ) {
              MString err("liqRibPfxHairData failed to allocate CV color memory !");
              cout <<err<<endl<<flush;
              throw(err);
              return;
            } else colorPtr = cvColor + ( totalNumberOfVertices * 3 - nverts[i] * 3 ) ;

            cvOpacity = (RtFloat*)realloc( cvOpacity, sizeof( RtFloat ) * ( totalNumberOfVertices * 3 ) );
            if ( cvOpacity == NULL ) {
              MString err("liqRibPfxHairData failed to allocate CV opacity memory !");
              cout <<err<<endl<<flush;
              throw(err);
              return;
            } else opacityPtr = cvOpacity + ( totalNumberOfVertices * 3 - nverts[i] * 3 ) ;



            *cvPtr      = (RtFloat) vertices[vertIndex].x;                          *cvPtr++;
            *cvPtr      = (RtFloat) vertices[vertIndex].y;                          *cvPtr++;
            *cvPtr      = (RtFloat) vertices[vertIndex].z;                          *cvPtr++;

            *colorPtr   = (RtFloat) vertexColor[vertIndex].x ;                      *colorPtr++;
            *colorPtr   = (RtFloat) vertexColor[vertIndex].y ;                      *colorPtr++;
            *colorPtr   = (RtFloat) vertexColor[vertIndex].z ;                      *colorPtr++;

            *opacityPtr = (RtFloat) ( 1.0f - vertexTransparency[vertIndex].x ) ;    *opacityPtr++;
            *opacityPtr = (RtFloat) ( 1.0f - vertexTransparency[vertIndex].y ) ;    *opacityPtr++;
            *opacityPtr = (RtFloat) ( 1.0f - vertexTransparency[vertIndex].z ) ;    *opacityPtr++;


            for ( ; vertIndex < vertices.length(); vertIndex++ ) {
              *cvPtr      = (RtFloat) vertices[vertIndex].x;                        *cvPtr++;
              *cvPtr      = (RtFloat) vertices[vertIndex].y;                        *cvPtr++;
              *cvPtr      = (RtFloat) vertices[vertIndex].z;                        *cvPtr++;

              *widthPtr   = (RtFloat) width[vertIndex];                             *widthPtr++;

              *colorPtr   = (RtFloat) vertexColor[vertIndex].x ;                    *colorPtr++;
              *colorPtr   = (RtFloat) vertexColor[vertIndex].y ;                    *colorPtr++;
              *colorPtr   = (RtFloat) vertexColor[vertIndex].z ;                    *colorPtr++;

              *opacityPtr = (RtFloat) ( 1.0f - vertexTransparency[vertIndex].x ) ;  *opacityPtr++;
              *opacityPtr = (RtFloat) ( 1.0f - vertexTransparency[vertIndex].y ) ;  *opacityPtr++;
              *opacityPtr = (RtFloat) ( 1.0f - vertexTransparency[vertIndex].z ) ;  *opacityPtr++;
            }


            *cvPtr = (float) vertices[vertIndex-1].x;                               *cvPtr++;
            *cvPtr = (float) vertices[vertIndex-1].y;                               *cvPtr++;
            *cvPtr = (float) vertices[vertIndex-1].z;                               *cvPtr++;

            *colorPtr = (RtFloat) vertexColor[vertIndex-1].x ;                      *colorPtr++;
            *colorPtr = (RtFloat) vertexColor[vertIndex-1].y ;                      *colorPtr++;
            *colorPtr = (RtFloat) vertexColor[vertIndex-1].z ;                      *colorPtr++;

            *opacityPtr = (RtFloat) ( 1.0f - vertexTransparency[vertIndex-1].x ) ;  *opacityPtr++;
            *opacityPtr = (RtFloat) ( 1.0f - vertexTransparency[vertIndex-1].y ) ;  *opacityPtr++;
            *opacityPtr = (RtFloat) ( 1.0f - vertexTransparency[vertIndex-1].z ) ;  *opacityPtr++;

          }
        }

        // store for output
        //cout <<"store P for output... ";
        liqTokenPointer points_pointerPair;
        int test = points_pointerPair.set( "P", rPoint, false, true, false, totalNumberOfVertices );
        if ( test == 0 ) {
          MString err("liqRibPfxHairData: liqTokenPointer failed to allocate CV memory !");
          cout <<err<<endl;
          throw(err);
          return;
        }// else cout <<"points_pointerPair = "<<test<<endl;
        points_pointerPair.setDetailType( rVertex );
        points_pointerPair.setTokenFloats( CVs );
        tokenPointerArray.push_back( points_pointerPair );
        if ( CVs != NULL ) { lfree( CVs ); CVs = NULL; }
        //cout <<"Done !"<<endl;


        // store width params
        //cout <<"store width for output... ";
        liqTokenPointer width_pointerPair;
        test = width_pointerPair.set( "width", rFloat, false, true, false, totalNumberOfSpans );
        if ( test == 0 ) {
          MString err("liqRibPfxHairData: liqTokenPointer failed to allocate width memory !");
          cout <<err<<endl;
          throw(err);
          return;
        }// else cout <<"width_pointerPair = "<<test<<endl;
        width_pointerPair.setDetailType( rVarying );
        width_pointerPair.setTokenFloats( curveWidth );
        tokenPointerArray.push_back( width_pointerPair );
        if ( curveWidth != NULL ) { lfree( curveWidth ); curveWidth = NULL; }
        //cout <<"Done !"<<endl;

        // store color params
        //cout <<"store color for output... ";
        liqTokenPointer color_pointerPair;
        test = color_pointerPair.set( "pfxHair_vtxColor", rColor, false, true, false, totalNumberOfVertices );
        if ( test == 0 ) {
          MString err("liqRibPfxHairData: liqTokenPointer failed to allocate color memory !");
          cout <<err<<endl;
          throw(err);
          return;
        } //else cout <<"color_pointerPair = "<<test<<endl;
        color_pointerPair.setDetailType( rVertex );
        color_pointerPair.setTokenFloats( cvColor );
        tokenPointerArray.push_back( color_pointerPair );
        if ( cvColor != NULL ) { lfree( cvColor ); cvColor = NULL; }
        //cout <<"Done !"<<endl;

        // store opacity params
        //cout <<"store opacity for output... ";
        liqTokenPointer opacity_pointerPair;
        test = opacity_pointerPair.set( "pfxHair_vtxOpacity", rColor, false, true, false, totalNumberOfVertices );
        if ( test == 0 ) {
          MString err("liqRibPfxHairData: liqTokenPointer failed to allocate opacity memory !");
          cout <<err<<endl<<flush;
          throw(err);
          return;
        } //else cout <<"opacity_pointerPair = "<<test<<endl;
        opacity_pointerPair.setDetailType( rVertex );
        opacity_pointerPair.setTokenFloats( cvOpacity );
        tokenPointerArray.push_back( opacity_pointerPair );
        if ( cvOpacity != NULL ) { lfree( cvOpacity ); cvOpacity = NULL; }
        //cout <<"Done !"<<endl;

        // additional rman* params
        addAdditionalSurfaceParameters( pfxHair );

      }

      // delete line arrays
      //cout <<"[liquid] >> delete line arrays.... ";
      profileArray.deleteArray();
      creaseArray.deleteArray();
      intersectionArray.deleteArray();
      //cout <<"Done !"<<endl;

    }

  }

}

liqRibPfxHairData::~liqRibPfxHairData()
//  Description:
//      class destructor
{
  //cout <<"~ killing pfxHair curve !"<<endl;
  if ( ncurves > 0 ) {
    // Free all arrays
    LIQDEBUGPRINTF( "-> killing pfxHair curves\n" );
    if ( CVs != NULL )        { lfree( CVs );         CVs = NULL;         }
    if ( nverts != NULL )     { lfree( nverts );      nverts = NULL;      }
    if ( curveWidth != NULL ) { lfree( curveWidth );  curveWidth = NULL;  }
    if ( cvColor != NULL )    { lfree( cvColor );     cvColor = NULL;     }
    if ( cvOpacity != NULL )  { lfree( cvOpacity );   cvOpacity = NULL;   }
  }
}

void liqRibPfxHairData::write()
//
//  Description:
//      Write the RIB for this surface
//
{
  LIQDEBUGPRINTF( "-> writing pfxHair curves\n" );

  if ( ncurves > 0 ) {

    //cout <<"allocating ram...";
    unsigned numTokens = tokenPointerArray.size();
    RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * numTokens );
    RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * numTokens );
    //cout <<"Done !"<<endl;

    //cout <<"assigning tokens... ";
    assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );
    //cout <<"Done !"<<endl;

    //cout <<"make Ri call... ";
    RiCurvesV( "cubic", ncurves, nverts, "nonperiodic", numTokens, tokenArray, pointerArray );
    //cout <<"Done !"<<endl;
  }
}

bool liqRibPfxHairData::compare( const liqRibData & otherObj ) const
//
//  Description:
//      Compare this curve to the other for the purpose of determining
//      if it is animated.
//
{
  LIQDEBUGPRINTF( "-> comparing pfxHair curves\n");
  //cout <<"-> comparing pfxHair curves..."<<endl;

  if ( otherObj.type() != MRT_PfxHair ) return false;
  const liqRibPfxHairData & other = (liqRibPfxHairData&)otherObj;

  if ( ncurves != other.ncurves ) return false;

  return false;
}

ObjectType liqRibPfxHairData::type() const
//
//  Description:
//      return the geometry type
//
{
  LIQDEBUGPRINTF( "-> returning pfxHair curve type\n" );
  return MRT_PfxHair;
}


