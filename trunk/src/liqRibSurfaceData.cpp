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
** Liquid Rib Surface Data Source 
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
#include<maya/MPoint.h>
#include<maya/MPlug.h>
#include<maya/MFnNurbsCurve.h>
#include<maya/MObjectArray.h>
#include<maya/MDoubleArray.h>
#include<maya/MPointArray.h>
#include<maya/MFloatArray.h>
#include<maya/MItSurfaceCV.h>
#include<maya/MFnNurbsSurface.h>
#include<maya/MIntArray.h>

#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liqRibSurfaceData.h>
#include <liqMemory.h>
#include <liqRenderer.h>

extern int		debugMode;
extern liqRenderer*	liqglo_renderer;

liqRibSurfaceData::liqRibSurfaceData( MObject surface )
//  Description:
//      create a RIB compatible representation of a Maya nurbs surface

:   hasTrims( false ),
    uknot( NULL ),
    vknot( NULL ),
    CVs( NULL ),
    ncurves( NULL ),
    order( NULL ),
    n( NULL ),
    knot( NULL ),
    minKnot( NULL ),
    maxKnot( NULL ),
    u( NULL ),
    v( NULL ),
    w( NULL )

{
    if ( debugMode ) { 
	printf("-> creating nurbs surface\n"); 
	MFnDependencyNode myDep( surface );
	MString name = myDep.name();
	printf("-> surface path %s \n", name.asChar() );
    }
    
    // Hmmmmm Was global but never changed ...
    bool normalizeNurbsUV = true;
	
    MStatus status = MS::kSuccess;
    MFnNurbsSurface nurbs( surface, &status );
    if (status == MS::kSuccess) {
		
	// Extract the order and number of CVs in the surface

	MDoubleArray uKnots, vKnots;

	if (liqglo_renderer->requires(liqRenderer::SWAPPED_UVS))
	{
	    if ( debugMode ) { printf("-> swapping uvs\n"); }

	    uorder = nurbs.degreeV() + 1; // uv order is switched
	    vorder = nurbs.degreeU() + 1; // uv order is switched
	    nv = nurbs.numCVsInU();       // uv order is switched
	    nu = nurbs.numCVsInV();       // uv order is switched
	    
	    // Read the knot information
	    
	    nurbs.getKnotsInU(vKnots); // uv order is switched
	    nurbs.getKnotsInV(uKnots); // uv order is switched
	    
	    double uMin_d, uMax_d, vMin_d, vMax_d;
	    nurbs.getKnotDomain(uMin_d, uMax_d, vMin_d, vMax_d);
	    umin = (RtFloat)vMin_d; // uv order is switched
	    umax = (RtFloat)vMax_d; // uv order is switched
	    vmin = (RtFloat)uMin_d; // uv order is switched
	    vmax = (RtFloat)uMax_d; // uv order is switched
	}
	else
	{
	    if ( debugMode ) { printf("-> not swapping uvs\n"); }

	    uorder = nurbs.degreeU() + 1;
	    vorder = nurbs.degreeV() + 1;
	    nu = nurbs.numCVsInU();
	    nv = nurbs.numCVsInV();
	    
	    // Read the knot information
	    
	    nurbs.getKnotsInU(uKnots);
	    nurbs.getKnotsInV(vKnots);
	    
	    double uMin_d, uMax_d, vMin_d, vMax_d;
	    nurbs.getKnotDomain(uMin_d, uMax_d, vMin_d, vMax_d);
	    umin = (RtFloat)uMin_d;
	    umax = (RtFloat)uMax_d;
	    vmin = (RtFloat)vMin_d;
	    vmax = (RtFloat)vMax_d;
	}
	    
	float uKnotMult = 1;
	float vKnotMult = 1;
	
	// this was added to simulate MTOR's parameterization handling
	// it, by default, normalizes the U and V coordinates.
	
	MPlug noNormalizeNurbsPlug = nurbs.findPlug( "noNormalizeNurbs", &status );  
	
	if ( normalizeNurbsUV && ( status != MS::kSuccess ) ) {
	    uKnotMult = 1 / ( umax - umin );
	    vKnotMult = 1 / ( vmax - vmin );
	}
	
	// Allocate CV and knot storage
	
	CVs   = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( nu * nv * 4 ) );
	uknot = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( uKnots.length() + 2 ) );
	vknot = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( vKnots.length() + 2 ) );
	
	unsigned k;
	if ( normalizeNurbsUV && ( status != MS::kSuccess ) ) {
	    for ( k = 0; k < uKnots.length(); k++ )
		uknot[k+1] = ( (RtFloat)uKnots[k] - umin ) * uKnotMult;
	    umin = 0; umax = 1;
	} else {
	    for ( k = 0; k < uKnots.length(); k++ )
		uknot[k+1] = (RtFloat)uKnots[k];
	}		
	// Maya doesn't store the first and last knots, so we double them up
	// manually
	//
	uknot[0] = uknot[1];
	uknot[k+1] = uknot[k];
	
	if ( normalizeNurbsUV && ( status != MS::kSuccess ) ) {
	    for ( k = 0; k < vKnots.length(); k++ )
		vknot[k+1] = ( (RtFloat)vKnots[k] - vmin ) * vKnotMult; 
	    vmin = 0; vmax = 1;
	} else {
	    for ( k = 0; k < vKnots.length(); k++ )
		vknot[k+1] = (RtFloat)vKnots[k]; 
	}
	
	// Maya doesn't store the first and last knots, so we double them up
	// manually
	//
	vknot[0] = vknot[1];
	vknot[k+1] = vknot[k];
	
	// Read CV information
	//
	MItSurfaceCV cvs( surface,
			  liqglo_renderer->requires(liqRenderer::SWAPPED_UVS) == false );
	RtFloat* cvPtr = CVs;

	while(!cvs.isDone()) {
	    while(!cvs.isRowDone()) {
		MPoint pt = cvs.position(MSpace::kObject);
		*cvPtr = (RtFloat)pt.x; cvPtr++;
		*cvPtr = (RtFloat)pt.y; cvPtr++;
		*cvPtr = (RtFloat)pt.z; cvPtr++;
		*cvPtr = (RtFloat)pt.w; cvPtr++;
		cvs.next();
	    }
	    cvs.nextRow();
	}
	
	// Store trim information
	//
	if (nurbs.isTrimmedSurface()) {
	    hasTrims = true;
	    if ( debugMode ) { printf("-> storing trim information\n"); }
	    
	    unsigned numRegions, numBoundaries, numEdges, numCurves;
	    unsigned r, b, e, c;
	    
	    // Get the number of loops
	    //
	    numRegions = nurbs.numRegions();
	    nloops = 0;
	    for ( r = 0; r < numRegions; r++ ) {
		    numBoundaries = nurbs.numBoundaries( r );
		    nloops += numBoundaries;
	    }
	    
	    MIntArray numCurvesPerLoop, orderArray, numCVsArray;
	    MFloatArray knotArray, minArray, maxArray;
	    MPointArray cvArray;
	    
	    // Get the number of trim curves in each loop and gather curve 
	    // information
	    //
	    for ( r = 0; r < nloops; r++ ) {
		numBoundaries = nurbs.numBoundaries( r );
		for ( b = 0; b < numBoundaries; b++ ) {
		    numCurves = 0;
		    numEdges = nurbs.numEdges( r, b );
		    for ( e = 0; e < numEdges; e++ ) {
			MObjectArray curves = nurbs.edge( r, b, e, true );
			numCurves += curves.length();
			
			// Gather extra stats for each curve
			//
			for ( c = 0; c < curves.length(); c++ ) {
			    unsigned i;
			    
			    // Read the # of CVs in and the order of each curve
			    //
			    MFnNurbsCurve curveFn(curves[c]);
			    orderArray.append( curveFn.degree() + 1 );
			    numCVsArray.append( curveFn.numCVs() );
			    
			    // Read the CVs for each curve
			    //
			    MPoint pnt;
			    unsigned last = curveFn.numCVs();
			    for ( i = 0; i < last; ++i ) {
				    curveFn.getCV( i, pnt ); 
				    cvArray.append( pnt );
			    }
			    
			    // Read the knot array for each curve
			    //
			    MDoubleArray knotsTmpArray;
			    curveFn.getKnots( knotsTmpArray );
			    last = knotsTmpArray.length();
			    knotArray.append( knotsTmpArray[0] );
			    for ( i = 0; i < last; ++i ) {
				    knotArray.append( (float)knotsTmpArray[i] );
			    }
			    knotArray.append( knotsTmpArray[last-1] );
			    
			    // Read the knot domain for each curve
			    //
			    double start, end;
			    curveFn.getKnotDomain( start, end );
			    minArray.append( start );
			    maxArray.append( end );
			}
		    }
		    numCurvesPerLoop.append( numCurves );
		}
	    }
	    
	    // Store the trim information in RIB format
	    //
	    ncurves = (RtInt*)lmalloc( sizeof( RtInt ) * numCurvesPerLoop.length() );
	    numCurvesPerLoop.get( (int*)ncurves );
	    
	    order = (RtInt*)lmalloc( sizeof( RtInt ) * orderArray.length() );
	    orderArray.get( (int*)order );
	    
	    n = (RtInt*)lmalloc( sizeof( RtInt ) * numCVsArray.length() );
	    numCVsArray.get( (int*)n );
	    
	    knot = (RtFloat*)lmalloc( sizeof( RtFloat ) * knotArray.length() );
	    knotArray.get( knot );
	    
	    minKnot = (RtFloat*)lmalloc( sizeof( RtFloat ) * minArray.length() );
	    minArray.get( minKnot );
	    
	    maxKnot = (RtFloat*)lmalloc( sizeof( RtFloat ) * maxArray.length() );
	    maxArray.get( maxKnot );
	    
	    unsigned last = cvArray.length();
	    u = (RtFloat*)lmalloc( sizeof( RtFloat ) * last );
	    v = (RtFloat*)lmalloc( sizeof( RtFloat ) * last );
	    w = (RtFloat*)lmalloc( sizeof( RtFloat ) * last );
	    for ( unsigned i = 0; i < last; ++i ) {
		u[i] = (RtFloat)( cvArray[i].y * cvArray[i].w ); // u
		v[i] = (RtFloat)( cvArray[i].x * cvArray[i].w ); // v
		w[i] = (RtFloat) cvArray[i].w;                   // w
	    }
	    
	    numCurvesPerLoop.clear();
	    orderArray.clear();
	    numCVsArray.clear();
	    knotArray.clear();
	    minArray.clear();
	    maxArray.clear();
	    cvArray.clear();
	}
	
	// now place our tokens and parameters into our tokenlist
	
	liqTokenPointer tokenPointerPair;
	tokenPointerPair.set( "Pw", rPoint, true, true, false , nu * nv );
	tokenPointerPair.setDetailType( rVertex );
	tokenPointerPair.setTokenFloats( CVs );
	tokenPointerArray.push_back( tokenPointerPair );
	
	addAdditionalSurfaceParameters( surface );
    }
}

liqRibSurfaceData::~liqRibSurfaceData()
//  Description:
//      class destructor
{
    // free all arrays
    if ( debugMode ) { printf("-> killing nurbs surface\n"); }
    if ( uknot != NULL ) { lfree( uknot ); uknot = NULL; }
    if ( vknot != NULL ) { lfree( vknot ); vknot = NULL; }
	// this is freed by the ribdata destructor
	// this is not true anymore 
    if ( CVs != NULL ) { lfree( CVs ); CVs = NULL; }
    if ( ncurves != NULL ) { lfree( ncurves ); ncurves = NULL; }
    if ( order != NULL ) { lfree( order ); order = NULL; }
    if ( n != NULL ) { lfree( n ); n = NULL; }
    if ( knot != NULL ) { lfree( knot ); knot = NULL; }
    if ( minKnot != NULL ) { lfree( minKnot ); minKnot = NULL; }
    if ( maxKnot != NULL ) { lfree( maxKnot ); maxKnot = NULL; }
    if ( u != NULL ) { lfree( u ); u = NULL; }
    if ( v != NULL ) { lfree( v ); v = NULL; }
    if ( w != NULL ) { lfree( w ); w = NULL; }
    if ( debugMode ) { printf("-> finished killing nurbs surface\n"); }
}

void liqRibSurfaceData::write()
//  Description:
//      Write the RIB for this surface
{
    if ( debugMode ) { printf("-> writing nurbs surface\n"); }
    
    if ( tokenPointerArray.size() > 0 ) {
	unsigned numTokens = tokenPointerArray.size();
	RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * numTokens );
	RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * numTokens );
	
	assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );
	
	RiNuPatchV( nu,
		    uorder,
		    uknot,
		    umin,
		    umax,
		    nv,
		    vorder,
		    vknot,
		    vmin,
		    vmax,
		    numTokens,
		    tokenArray,
		    pointerArray );
    } else {
	if ( debugMode ) { printf("-> ignoring nurbs surface\n"); }
    }

    if ( debugMode ) { printf("-> done writing nurbs surface\n"); }
}

bool liqRibSurfaceData::compare( const liqRibData & otherObj ) const
//
//  Description:
//      Compare this surface to the other for the purpose of determining
//      if it is animated.
//
{
    if ( debugMode ) { printf("-> comparing nurbs surface\n"); }
    if ( otherObj.type() != MRT_Nurbs ) return false;

    const liqRibSurfaceData & other = (liqRibSurfaceData&)otherObj;
    
    if ( ( nu != other.nu ) ||
	 ( nv != other.nv ) ||
	 ( uorder != other.uorder ) ||
	 ( vorder != other.vorder ) ||
	 !equiv( umin, other.umin ) ||
	 !equiv( umax, other.umax ) ||
	 !equiv( vmin, other.vmin ) ||
	 !equiv( vmax, other.vmax ) ) 
    {
        return false;
    }
    
    // Check Knots
    //
    unsigned i;
    unsigned last = nu + uorder;
    for ( i = 0; i < last; ++i ) {
        if ( !equiv( uknot[i], other.uknot[i] ) )
	    return false;
    }
    last = nv + vorder;
    for ( i = 0; i < last; ++i ) {
        if ( !equiv( vknot[i], other.vknot[i] ) )
	    return false;
    }
    
    // Check CVs
    //
    last = nu * nv * 4;
    for ( i = 0; i < last; ++i ) {
        if ( !equiv( CVs[i], other.CVs[i] ) )
	    return false;
    }
	
    // TODO: Check trims as well
    return true;
}

ObjectType liqRibSurfaceData::type() const
//
//  Description:
//      return the geometry type
//
{
    if ( debugMode ) { printf("-> returning nurbs surface type\n"); }
    return MRT_Nurbs;
}

bool liqRibSurfaceData::hasTrimCurves() const
{
    if ( debugMode ) { printf("-> checking for nurbs surface trims\n"); }
    return hasTrims;   
}

void liqRibSurfaceData::writeTrimCurves() const
{
    if ( debugMode ) { printf("-> writing nurbs surface trims\n"); }
    if ( hasTrims ) {
	RiTrimCurve( nloops,
		     ncurves,
		     order,
		     knot,
		     minKnot,
		     maxKnot,
		     n,
		     u, 
		     v,
		     w );
    }
}
