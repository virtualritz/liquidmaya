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

#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liqRibData.h>
#include <liqRibNuCurveData.h>

extern int debugMode;

liqRibNuCurveData::liqRibNuCurveData( MObject curve )
//
//  Description:
//      create a RIB compatible representation of a Maya nurbs surface
//
  : nverts( NULL ),
    order( NULL ),
    knot( NULL ),
    CVs( NULL ),
    NuCurveWidth( NULL )
{
	LIQDEBUGPRINTF( "-> creating nurbs curve\n" );
	MStatus status = MS::kSuccess;
	MFnNurbsCurve nurbs( curve, &status );
	assert(status==MS::kSuccess);

	// Extract the order and number of CVs in the surface keeping
	// in mind that UV order is switched between Renderman and Maya
	ncurves = 1;  //RiNuCurves can be passed many curves but right now it only passes one from maya at a time
	nverts = (RtInt*)lmalloc( sizeof( RtInt ) * ( ncurves ) );
	order = (RtInt*)lmalloc( sizeof( RtInt ) * ( ncurves ) );
	min = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( ncurves ) );
	max = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( ncurves ) );

	order[0] = nurbs.degree() + 1;
	nverts[0] = nurbs.numCVs() + 4;

	/*if (nverts[0] < 2) {
	// seems like some ill-defined patches are sometimes
	// present in the database (cf: test scene)
	// Might be able to add some sort of throw here where it passes it to RiCurve
	//
	MString error("Ill-defined nurbs curve: ");
	error += nurbs.name();
	printf( "Bad Curve!" );
	throw( error );
}*/

	// Read the knot information
	//
/*	MDoubleArray Knots;
	nurbs.getKnots(Knots);

	double Min_d, Max_d;
	nurbs.getKnotDomain(Min_d, Max_d);
	min[0] = (RtFloat)Min_d;
	max[0] = (RtFloat)Max_d;
*/
	// Allocate CV and knot storage
	//
//	CVs   = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( nverts[0] * 4 ) );
	CVs   = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( nverts[0] * 3 ) );
//	knot = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( Knots.length() + 2 ) );

/*  unsigned k;
	for ( k = 0; k < Knots.length(); k++ ) knot[k+1] = (RtFloat)Knots[k];
	// Maya doesn't store the first and last knots, so we double them up
	// manually
	//
	knot[0] = knot[1];
	knot[k+1] = knot[k];
  */

	// Read CV information
	//
	MItCurveCV cvs( curve, &status );
	RtFloat* cvPtr = CVs;

	// Double up start and end to simulate knot, MToor style (we should really be using RiNuCurves) - Paul
	MPoint pt = cvs.position(MSpace::kObject);
	*cvPtr = (RtFloat)pt.x; cvPtr++;
	*cvPtr = (RtFloat)pt.y; cvPtr++;
	*cvPtr = (RtFloat)pt.z; cvPtr++;

	*cvPtr = (RtFloat)pt.x; cvPtr++;
	*cvPtr = (RtFloat)pt.y; cvPtr++;
	*cvPtr = (RtFloat)pt.z; cvPtr++;

	while(!cvs.isDone()) {
		pt = cvs.position(MSpace::kObject);
		*cvPtr = (RtFloat)pt.x; cvPtr++;
		*cvPtr = (RtFloat)pt.y; cvPtr++;
		*cvPtr = (RtFloat)pt.z; cvPtr++;
	//	*cvPtr = (RtFloat)pt.w; cvPtr++;
		cvs.next();
	}

	*cvPtr = (RtFloat)pt.x; cvPtr++;
	*cvPtr = (RtFloat)pt.y; cvPtr++;
	*cvPtr = (RtFloat)pt.z; cvPtr++;

	*cvPtr = (RtFloat)pt.x; cvPtr++;
	*cvPtr = (RtFloat)pt.y; cvPtr++;
	*cvPtr = (RtFloat)pt.z; cvPtr++;

	liqTokenPointer pointsPointerPair;
	liqTokenPointer* pConstWidthPointerPair = NULL;

	pointsPointerPair.set( "P", rPoint, false, true, false, nverts[0] );
	pointsPointerPair.setDetailType( rVertex );
	pointsPointerPair.setTokenFloats( CVs );
	tokenPointerArray.push_back( pointsPointerPair );

	// Constant width, MToor style - Paul
	MPlug curveWidthPlug = nurbs.findPlug( "liquidCurveWidth", &status );

	if ( status == MS::kSuccess ) {
		float curveWidth;
		curveWidthPlug.getValue( curveWidth );
		pConstWidthPointerPair = new liqTokenPointer;
#ifndef DELIGHT
		pConstWidthPointerPair->set( "constantwidth", rFloat, false, false, false, 0 );
		pConstWidthPointerPair->setDetailType( rUniform );
		pConstWidthPointerPair->setTokenFloat(0, curveWidth );
#else // Arrgh! 3Delight wants "constantwidth" per curve segment	so we make it varying :(
		pConstWidthPointerPair->set( "constantwidth", rFloat, false, true, false, nverts[0] - 2 );
		pConstWidthPointerPair->setDetailType( rVarying );
		for( int i = 0; i < nverts[0] - 2; i++ ) {
			pConstWidthPointerPair->setTokenFloat( i, curveWidth );
		}
#endif
		tokenPointerArray.push_back( *pConstWidthPointerPair );
		delete pConstWidthPointerPair;

	}

	addAdditionalSurfaceParameters( curve );
}

liqRibNuCurveData::~liqRibNuCurveData()
//  Description:
//      class destructor
{
	// Free all arrays
	LIQDEBUGPRINTF( "-> killing nurbs curve\n" );
	// uncomment below if 'knot' is used again
	// if ( knot != NULL ) { lfree( knot ); knot = NULL; }
	// this is freed with the ribdata destructor
	// this is not true anymore
	if ( CVs != NULL ) { lfree( CVs ); CVs = NULL; }
	if ( NuCurveWidth != NULL ) { lfree( NuCurveWidth ); NuCurveWidth = NULL; }
	if ( nverts != NULL ) { lfree( nverts ); nverts = NULL; }
	if ( order != NULL ) { lfree( order ); order = NULL; }
	if ( min != NULL ) { lfree( min ); min = NULL; }
	if ( max != NULL ) { lfree( max ); max = NULL; }
}

void liqRibNuCurveData::write()
//
//  Description:
//      Write the RIB for this surface
//
{
	LIQDEBUGPRINTF( "-> writing nurbs curve\n" );

	unsigned numTokens = tokenPointerArray.size();
	RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * numTokens );
	RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * numTokens );

	assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );
	RiCurvesV( "cubic", ncurves, nverts, "nonperiodic", numTokens, tokenArray, pointerArray );
}

bool liqRibNuCurveData::compare( const liqRibData & otherObj ) const
//
//  Description:
//      Compare this curve to the other for the purpose of determining
//      if it is animated.
//
{
  LIQDEBUGPRINTF( "-> comparing nurbs curve\n");
  if ( otherObj.type() != MRT_NuCurve ) return false;
  const liqRibNuCurveData & other = (liqRibNuCurveData&)otherObj;

  if ( ( nverts[0] != other.nverts[0] ) ||
       ( order != other.order ) ||
		   !equiv( min[0], other.min[0] ) ||
		   !equiv( max[0], other.max[0] ))
  {
    return false;
  }

  // Check Knots
  unsigned i;
  unsigned last = nverts[0] + order[0];
  for ( i = 0; i < last; ++i ) {
    if ( !equiv( knot[i], other.knot[i] ) ) return false;
  }

  // Check CVs
  last = nverts[0] * 3;
  for ( i = 0; i < last; ++i ) {
      if ( !equiv( CVs[i], other.CVs[i] ) ) return false;
  }

  return true;
}

ObjectType liqRibNuCurveData::type() const
//
//  Description:
//      return the geometry type
//
{
  LIQDEBUGPRINTF( "-> returning nurbs curve type\n" );
	return MRT_NuCurve;
}
