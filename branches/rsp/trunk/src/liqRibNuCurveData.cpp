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

#include <boost/scoped_array.hpp>
#include <boost/shared_array.hpp>

using namespace boost;

extern int debugMode;


/** Create a RIB compatible representation of a Maya nurbs curve.
 */
liqRibNuCurveData::liqRibNuCurveData( MObject curve )
: nverts(),
  order(),
  knot(),
  CVs(),
  NuCurveWidth()
{
  LIQDEBUGPRINTF( "-> creating nurbs curve\n" );
  MStatus status = MS::kSuccess;
  MFnNurbsCurve nurbs( curve, &status );
  assert(status==MS::kSuccess);

  // Extract the order and number of CVs in the surface keeping
  // in mind that UV order is switched between Renderman and Maya
  ncurves = 1;  //RiNuCurves can be passed many curves but right now it only passes one from maya at a time
  nverts = shared_array< RtInt >( new RtInt[ ncurves ] );
  order = shared_array< RtInt >( new RtInt[ ncurves ] );

  order[0] = nurbs.degree() + 1;
#ifndef DELIGHT
  nverts[0] = nurbs.numCVs() + 4;
#else
  nverts[0] = nurbs.numCVs();
#endif

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

#ifdef DELIGHT
  min = shared_array< RtFloat >( new RtFloat[ ncurves ] );
  max = shared_array< RtFloat >( new RtFloat[ ncurves ] );
  double Min_d, Max_d;
  nurbs.getKnotDomain(Min_d, Max_d);
  min[0] = (RtFloat)Min_d;
  max[0] = (RtFloat)Max_d;

  // Read the knot information
  //
  MDoubleArray Knots;
  nurbs.getKnots(Knots);

  knot = shared_array< RtFloat >( new RtFloat[ Knots.length() + 2 ] );

    unsigned k;
  for ( k = 0; k < Knots.length(); k++ ) {
    knot[k+1] = (RtFloat)Knots[k];
  }

  // Maya doesn't store the first and last knots, so we double them up
  // manually
  //
  knot[0] = knot[1];
  knot[k+1] = knot[k];
#endif

  // Read CV information
  //
#ifndef DELIGHT
  CVs   = shared_array< RtFloat >( new RtFloat[ nverts[ 0 ] * 3 ] );
#else
  CVs   = shared_array< RtFloat >( new RtFloat[ nverts[ 0 ] * 4 ] );
#endif
  MItCurveCV cvs( curve, &status );
  RtFloat* cvPtr = CVs.get();

  // Double up start and end to simulate knot, MToor style (we should really be using RiNuCurves) - Paul
  MPoint pt = cvs.position(MSpace::kObject);

#ifndef DELIGHT
  *cvPtr = (RtFloat)pt.x; cvPtr++;
  *cvPtr = (RtFloat)pt.y; cvPtr++;
  *cvPtr = (RtFloat)pt.z; cvPtr++;

  *cvPtr = (RtFloat)pt.x; cvPtr++;
  *cvPtr = (RtFloat)pt.y; cvPtr++;
  *cvPtr = (RtFloat)pt.z; cvPtr++;
#endif

  while(!cvs.isDone()) {
    pt = cvs.position(MSpace::kObject);
    *cvPtr = (RtFloat)pt.x; cvPtr++;
    *cvPtr = (RtFloat)pt.y; cvPtr++;
    *cvPtr = (RtFloat)pt.z; cvPtr++;
#ifdef DELIGHT
    *cvPtr = (RtFloat)pt.w; cvPtr++;
#endif
    cvs.next();
  }

#ifndef DELIGHT
  *cvPtr = (RtFloat)pt.x; cvPtr++;
  *cvPtr = (RtFloat)pt.y; cvPtr++;
  *cvPtr = (RtFloat)pt.z; cvPtr++;

  *cvPtr = (RtFloat)pt.x; cvPtr++;
  *cvPtr = (RtFloat)pt.y; cvPtr++;
  *cvPtr = (RtFloat)pt.z; cvPtr++;
#endif

  liqTokenPointer pointsPointerPair;

#ifndef DELIGHT
  pointsPointerPair.set( "P", rPoint, nverts[0] );
#else
  pointsPointerPair.set( "Pw", rHpoint, nverts[0] );
#endif
  pointsPointerPair.setDetailType( rVertex );
  pointsPointerPair.setTokenFloats( CVs ); // Warning: CVs shares ownership with of its data with pointsPointerPair now!
                       // Saves us from redundant copying as long as we know what we are doing
  tokenPointerArray.push_back( pointsPointerPair );

  // Constant width, MToor style - Paul
  MPlug curveWidthPlug( nurbs.findPlug( "liquidCurveWidth", &status ) );

  if ( MS::kSuccess == status ) {
    float curveWidth;
    curveWidthPlug.getValue( curveWidth );
    liqTokenPointer pConstWidthPointerPair;
#ifndef DELIGHT
    pConstWidthPointerPair.set( "constantwidth", rFloat );
    pConstWidthPointerPair.setDetailType( rUniform );
    pConstWidthPointerPair.setTokenFloat( 0, curveWidth );
#else // 3Delight wants "constantwidth" per curve segment so we use "width" where 3Delight automatically does the right thing(tm)
    pConstWidthPointerPair.set( "width", rFloat );
    pConstWidthPointerPair.setDetailType( rUniform );
    pConstWidthPointerPair.setTokenFloat( 0, curveWidth );
#endif
    tokenPointerArray.push_back( pConstWidthPointerPair );
  }

  addAdditionalSurfaceParameters( curve );
}


/**  Write the RIB for this curve.
 */
void liqRibNuCurveData::write()
{
  LIQDEBUGPRINTF( "-> writing nurbs curve\n" );

  unsigned numTokens( tokenPointerArray.size() );
  scoped_array< RtToken > tokenArray( new RtToken[ numTokens ] );
  scoped_array< RtPointer > pointerArray( new RtPointer[ numTokens ] );
  assignTokenArraysV( tokenPointerArray, tokenArray.get(), pointerArray.get() );

#if defined(AIR) || defined(DELIGHT)
  RiNuCurvesV( ncurves, nverts.get(), order.get(), knot.get(), min.get(), max.get(), numTokens, tokenArray.get(), pointerArray.get() );
#else
  RiCurvesV( "cubic", ncurves, nverts.get(), "nonperiodic", numTokens, tokenArray.get(), pointerArray.get() );
#endif
}

/** Compare this curve to the other for the purpose of determining
 *  if it is animated.
 */
bool liqRibNuCurveData::compare( const liqRibData & otherObj ) const
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


/** Return the geometry type.
 */
ObjectType liqRibNuCurveData::type() const
{
  LIQDEBUGPRINTF( "-> returning nurbs curve type\n" );
  return MRT_NuCurve;
}
