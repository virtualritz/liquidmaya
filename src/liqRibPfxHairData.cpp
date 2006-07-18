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
  LIQDEBUGPRINTF( "-> creating pfxHair nurbs curve\n" );
  MStatus status = MS::kSuccess;

  //cout <<"[liquid] >> getting the pfxHair node... "<<endl;

  MFnPfxGeometry pfxtoon( pfxHair, &status );

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
    status = pfxtoon.getLineData( profileArray, creaseArray, intersectionArray, doLines, doTwist, doWidth, doFlatness, doParameter, doColor, doIncandescence, doTransparency, doWorldSpace );

    //cout <<"[liquid] >> profileArray contains "<<profileArray.length()<<" curves !"<<endl;
    //cout <<"[liquid] >> creaseArray contains "<<creaseArray.length()<<" curves !"<<endl;
    //cout <<"[liquid] >> intersectionArray contains "<<intersectionArray.length()<<" curves !"<<endl;

    if ( status == MS::kSuccess ) {

      //cout <<"[liquid] >> line arrays filled !"<<endl;

      // get the lines and fill the arrays.
      ncurves = profileArray.length();

      unsigned int totalNumberOfVertices = 0;
      MFloatArray CV, vWidth, vColor, vOpacity;

      if ( ncurves > 0 ) {

        nverts = (RtInt*)lmalloc( sizeof( RtInt ) * ( ncurves ) );

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
            unsigned int vertIndex = 0;

            CV.append( (float) vertices[vertIndex].x );
            CV.append( (float) vertices[vertIndex].y );
            CV.append( (float) vertices[vertIndex].z );
            vColor.append( (float) vertexColor[vertIndex].x );
            vColor.append( (float) vertexColor[vertIndex].y );
            vColor.append( (float) vertexColor[vertIndex].z );
            vOpacity.append( (float) 1 - vertexTransparency[vertIndex].x );
            vOpacity.append( (float) 1 - vertexTransparency[vertIndex].y );
            vOpacity.append( (float) 1 - vertexTransparency[vertIndex].z );

            for ( ; vertIndex < vertices.length(); vertIndex++ ) {
              CV.append( (float) vertices[vertIndex].x );
              CV.append( (float) vertices[vertIndex].y );
              CV.append( (float) vertices[vertIndex].z );
              vWidth.append( (float) width[vertIndex] );
              vColor.append( (float) vertexColor[vertIndex].x );
              vColor.append( (float) vertexColor[vertIndex].y );
              vColor.append( (float) vertexColor[vertIndex].z );
              vOpacity.append( (float) 1 - vertexTransparency[vertIndex].x );
              vOpacity.append( (float) 1 - vertexTransparency[vertIndex].y );
              vOpacity.append( (float) 1 - vertexTransparency[vertIndex].z );
            }

            //cout <<"last index is "<<vertIndex<<endl;

            CV.append( (float) vertices[vertIndex-1].x );
            CV.append( (float) vertices[vertIndex-1].y );
            CV.append( (float) vertices[vertIndex-1].z );
            vColor.append( (float) vertexColor[vertIndex-1].x );
            vColor.append( (float) vertexColor[vertIndex-1].y );
            vColor.append( (float) vertexColor[vertIndex-1].z );
            vOpacity.append( (float) 1 - vertexTransparency[vertIndex-1].x );
            vOpacity.append( (float) 1 - vertexTransparency[vertIndex-1].y );
            vOpacity.append( (float) 1 - vertexTransparency[vertIndex-1].z );

          }
        }

        // store for output
        //cout <<"store for output"<<endl;
        liqTokenPointer points_pointerPair;
        points_pointerPair.set( "P", rPoint, false, true, false, totalNumberOfVertices );
        points_pointerPair.setDetailType( rVertex );
        CVs   = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( totalNumberOfVertices * 3 ) );
        CV.get( CVs );
        points_pointerPair.setTokenFloats( CVs );
        tokenPointerArray.push_back( points_pointerPair );

        // store width params
        liqTokenPointer width_pointerPair;
        width_pointerPair.set( "width", rFloat, false, true, false, totalNumberOfVertices );
        width_pointerPair.setDetailType( rVarying );
        curveWidth = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( totalNumberOfVertices ) );
        vWidth.get( curveWidth );
        width_pointerPair.setTokenFloats( curveWidth );
        tokenPointerArray.push_back( width_pointerPair );

        // store color params
        liqTokenPointer color_pointerPair;
        color_pointerPair.set( "pfxHair_vtxColor", rColor, false, true, false, totalNumberOfVertices );
        color_pointerPair.setDetailType( rVertex );
        cvColor = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( totalNumberOfVertices * 3 ) );
        vColor.get( cvColor );
        color_pointerPair.setTokenFloats( cvColor );
        tokenPointerArray.push_back( color_pointerPair );

        // store opacity params
        liqTokenPointer opacity_pointerPair;
        opacity_pointerPair.set( "pfxHair_vtxOpacity", rColor, false, true, false, totalNumberOfVertices );
        opacity_pointerPair.setDetailType( rVertex );
        cvOpacity = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( totalNumberOfVertices * 3 ) );
        vOpacity.get( cvOpacity );
        opacity_pointerPair.setTokenFloats( cvOpacity );
        tokenPointerArray.push_back( opacity_pointerPair );

        // additional rman* params
        addAdditionalSurfaceParameters( pfxHair );

      }

      // delete line arrays
      //cout <<"[liquid] >> delete line arrays.... "<<endl;
      profileArray.deleteArray();
      creaseArray.deleteArray();
      intersectionArray.deleteArray();

    }

  }

}

liqRibPfxHairData::~liqRibPfxHairData()
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
    if ( cvOpacity != NULL ) { lfree( cvOpacity ); cvOpacity = NULL; }
  }
}

void liqRibPfxHairData::write()
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

    //RiBasis( RiBSplineBasis, 1, RiBSplineBasis, 1 );
    RiCurvesV( "cubic", ncurves, nverts, "nonperiodic", numTokens, tokenArray, pointerArray );

  }
}

bool liqRibPfxHairData::compare( const liqRibData & otherObj ) const
//
//  Description:
//      Compare this curve to the other for the purpose of determining
//      if it is animated.
//
{
  LIQDEBUGPRINTF( "-> comparing nurbs curve\n");
  if ( otherObj.type() != MRT_PfxHair ) return false;
  //const liqRibPfxHairData & other = (liqRibPfxHairData&)otherObj;

  // // Check CVs
  // last = nverts[0] * 3;
  // for ( i = 0; i < last; ++i ) {
  //     if ( !equiv( CVs[i], other.CVs[i] ) ) return false;
  // }

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


