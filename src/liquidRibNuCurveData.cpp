/*
**
** The contents of this file are subject to the Mozilla Public License Version 1.1 (the 
** "License"); you may not use this file except in compliance with the License. You may 
** obtain a copy of the License at http://www.mozilla.org/MPL/ 
** 
** Software distributed under the License is distributed on an "AS IS" basis, WITHOUT 
** WARRANTY OF ANY KIND, either express or implied. See the License for the specific 
** language governing rights and limitations under the License. 
**
** The Original Code is the Liquid Rendering Toolkit. 
** 
** The Initial Developer of the Original Code is Colin Doncaster. Portions created by 
** Colin Doncaster are Copyright (C) 2002. All Rights Reserved. 
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
#include <maya/MDoubleArray.h>
#include <maya/MItCurveCV.h>
#include <maya/MPoint.h>
#include <maya/MFnNurbsCurve.h>

#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liquidRibData.h>
#include <liquidRibNuCurveData.h>
#include <liqMemory.h>

extern int debugMode;

liquidRibNuCurveData::liquidRibNuCurveData( MObject curve )
//
//  Description:
//      create a RIB compatible representation of a Maya nurbs surface
//
:   nverts( NULL ), order( NULL ), knot( NULL ), CVs( NULL )

{
	if ( debugMode ) { printf("-> creating nurbs curve\n"); }
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
	
	nverts[0] = nurbs.numCVs();       
	
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
	MDoubleArray Knots;
	nurbs.getKnots(Knots); 
	
    double Min_d, Max_d;
    nurbs.getKnotDomain(Min_d, Max_d);
	min[0] = (RtFloat)Min_d; 
	max[0] = (RtFloat)Max_d; 
	
	// Allocate CV and knot storage
	//
	CVs   = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( nverts[0] * 4 ) );
	knot = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( Knots.length() + 2 ) );
	NuCurveWidth = (RtFloat*)lmalloc( sizeof( RtFloat ) * nverts[0] );
	
    unsigned k;
	for ( k = 0; k < Knots.length(); k++ ) knot[k+1] = (RtFloat)Knots[k];
    // Maya doesn't store the first and last knots, so we double them up
    // manually
    //
    knot[0] = knot[1];
    knot[k+1] = knot[k];
	
    
	// Read CV information
	//
	MItCurveCV cvs( curve, &status );
	RtFloat* cvPtr = CVs;
	while(!cvs.isDone()) {
		MPoint pt = cvs.position(MSpace::kObject);
		*cvPtr = (RtFloat)pt.x; cvPtr++;
		*cvPtr = (RtFloat)pt.y; cvPtr++;
		*cvPtr = (RtFloat)pt.z; cvPtr++;
		*cvPtr = (RtFloat)pt.w; cvPtr++;
		cvs.next();
	}
	// Setup curve width info
	for (k = 0; k < nverts[0]; k++) {
		NuCurveWidth[k] = 0.5;
	}

	liqTokenPointer tokenPointerPair;
	tokenPointerPair.set( "Pw", rPoint, true, true, false, nverts[0] );
	tokenPointerPair.setDetailType( rVertex );
	tokenPointerPair.setTokenFloats( CVs );
	tokenPointerArray.push_back( tokenPointerPair );
	
	addAdditionalSurfaceParameters( curve );
}

liquidRibNuCurveData::~liquidRibNuCurveData()
//  Description:
//      class destructor
{
	// Free all arrays
	if ( debugMode ) { printf("-> killing nurbs curve\n"); }
	if ( knot != NULL ) { lfree( knot ); knot = NULL; }
	// this is freed with the ribdata destructor
	// this is not true anymore
	if ( CVs != NULL ) { lfree( CVs ); CVs = NULL; }
	if ( NuCurveWidth != NULL ) { lfree( NuCurveWidth ); NuCurveWidth = NULL; }
	if ( nverts != NULL ) { lfree( nverts ); nverts = NULL; }
	if ( order != NULL ) { lfree( order ); order = NULL; }
	if ( min != NULL ) { lfree( min ); min = NULL; }
	if ( max != NULL ) { lfree( max ); max = NULL; }
}

void liquidRibNuCurveData::write()
//
//  Description:
//      Write the RIB for this surface
//
{
	if ( debugMode ) { printf("-> writing nurbs curve\n"); }
	
	unsigned numTokens = tokenPointerArray.size();
	RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * numTokens );
	RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * numTokens );
	
	assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );
	RiCurvesV( "cubic", ncurves, nverts, "nonperiodic", numTokens, tokenArray, pointerArray );
}

bool liquidRibNuCurveData::compare( const liquidRibData & otherObj ) const
//
//  Description:
//      Compare this curve to the other for the purpose of determining
//      if it is animated.
//
{	
	if ( debugMode ) { printf("-> comparing nurbs curve\n"); }
    if ( otherObj.type() != MRT_NuCurve ) return false;
    const liquidRibNuCurveData & other = (liquidRibNuCurveData&)otherObj;
    
    if ( ( nverts[0] != other.nverts[0] ) ||
		( order != other.order ) ||
		!equiv( min[0], other.min[0] ) ||
		!equiv( max[0], other.max[0] )) 
    {
        return false;
    }
    
    // Check Knots
    //
    unsigned i;
    unsigned last = nverts[0] + order[0];
    for ( i = 0; i < last; ++i ) {
        if ( !equiv( knot[i], other.knot[i] ) ) return false;
    }
	
    // Check CVs
    //
    last = nverts[0] * 4;
    for ( i = 0; i < last; ++i ) {
        if ( !equiv( CVs[i], other.CVs[i] ) ) return false;
    }
    
    return true;
}

ObjectType liquidRibNuCurveData::type() const
//
//  Description:
//      return the geometry type
//
{
	if ( debugMode ) { printf("-> returning nurbs curve type\n"); }
	return MRT_NuCurve;
}
