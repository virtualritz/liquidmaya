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
** Liquid Rib Particle Data Source 
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

#include <algorithm>

// Maya's Headers
#include <maya/MFnVectorArrayData.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MPoint.h>
#include <maya/MFnMesh.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFloatArray.h>
#include <maya/MIntArray.h>
#include <maya/MPointArray.h>

#include <liquid.h>
#include <liquidGlobalHelpers.h>
#include <liquidRibData.h>
#include <liqRibParticleData.h>
#include <liquidMemory.h>

extern int debugMode;

extern RtFloat liqglo_sampleTimes[5];
extern liquidlong liqglo_motionSamples;
extern bool liqglo_doDef;
extern bool liqglo_doMotion;

// these classes are needed to produce a list of particles sorted by their
// ids.
class liq_particleInfo
{
public:
    liq_particleInfo(int num, int id)
    : m_particleNum(num), m_particleId(id)
    {
	// nothing else needed
    }

    int m_particleNum;	// index into the per particle attribute arrays
    int m_particleId;	// global particle id
};

class liq_particleInfoVector : public std::vector<liq_particleInfo*>
{
public:
    // Make sure that the history items are de-allocated so we don't leak
    ~liq_particleInfoVector()
    {
	for (iterator pItem=begin(); pItem != end(); ++pItem)
	delete *pItem;
    }
};

class liq_particleIdSort
{
public:
    bool operator()(liq_particleInfo* const & a, liq_particleInfo* const & b)
    {
	return a->m_particleId < b->m_particleId;
    }
};


liqRibParticleData::liqRibParticleData( MObject partobj )
//
//  Description:
//      create a RIB compatible representation of a Maya particles
//
{
    if ( debugMode ) { printf("-> creating particles\n"); }
    MStatus status = MS::kSuccess;
    MFnDependencyNode  fnNode(partobj);

    MPlug particleRenderTypePlug = fnNode.findPlug( "particleRenderType", &status );
    short particleTypeVal;
    particleRenderTypePlug.getValue( particleTypeVal );
    particleType = (pType)particleTypeVal;
	
    /* birth time information */
    MPlug birthPlug = fnNode.findPlug( "birthTime", &status );
    MObject birthObject;
    birthPlug.getValue( birthObject );
    MFnDoubleArrayData birthArray( birthObject, &status );

    /* lifespan information */
    MStatus lifePPStatus;
    MPlug lifePPPlug = fnNode.findPlug( "finalLifespanPP", &lifePPStatus );
    MObject lifePPObject;
    lifePPPlug.getValue( lifePPObject );
    MFnDoubleArrayData lifePPArray( lifePPObject, &status );

    if ( debugMode ) { printf("-> Reading Particle Count\n"); }

    // first get the particle position data
    MPlug		posPlug = fnNode.findPlug( "position", &status );
    MObject		posObject;

    posPlug.getValue( posObject );

    MFnVectorArrayData	posArray( posObject, &status );
    status.clear();

    m_numParticles = posArray.length();

    // we need to sort the particles by id. The position array doesn't keep
    // things in particle id order so things can get pretty screwed up when
    // we're motion blurring
    MPlug		idPlug = fnNode.findPlug( "id", &status );
    MObject		idObject;

    idPlug.getValue( idObject );

    MFnDoubleArrayData	idArray( idObject, &status );
    status.clear();

    // then we need to check if there are any particles that have been born or
    // died during our shutter open period (between our first and last
    // sample).  Any such particles are not used since they screw up motion
    // blurring.
    // We accomplish this by putting the indices of valid particles in the
    // m_validParticles array.  Later we can iterate over the valid particles
    // by using the indices in this array

    // TODO: need to account for different frame rates here (the 24.0 numbers
    // below refer to a film render at 24fps)

    liq_particleInfoVector	particlesForSorting;;

    for ( unsigned part_num = 0; part_num < m_numParticles; part_num++ ) {

	double birthTime = birthArray[part_num] * 24.0;
	double shutterOpen = (double)liqglo_sampleTimes[0];
	double deathTime = ( birthArray[part_num] + lifePPArray[part_num] ) * 24.0;
	double shutterClose;

	if ( liqglo_doMotion || liqglo_doDef ) {
	    shutterClose = (double)liqglo_sampleTimes[liqglo_motionSamples - 1];
	} else {
	    shutterClose = (double)liqglo_sampleTimes[0];
	}
	
	// TODO: there's some funny behaviour here having to do with particles
	// dying before their actual lifespan is up.  (eg current time is 82.2
	// and the death time at 81.8 was 82.9 but the particle's not around
	// at 82.2).  The floor() below is to hack around that.  More fiddling
	// needed. Berj
	//if ( ( deathTime > shutterClose ) && ( birthTime < shutterOpen ) ) {
	if ( ( floor(deathTime) > shutterClose ) && ( birthTime < shutterOpen ) ) {
	    particlesForSorting.push_back(new liq_particleInfo(part_num,
					  static_cast<int>(idArray[part_num])));
	}
    }

    // sort in increasing id order
    std::sort(particlesForSorting.begin(),
	      particlesForSorting.end(),
	      liq_particleIdSort());

    // lastly.. we get all of the particle numbers into our m_validParticles
    // array for use later (we don't need the ids any more)
    for (int i = 0; i < particlesForSorting.size(); ++i)
    {
	m_validParticles.append(particlesForSorting[i]->m_particleNum);
    }

    m_numValidParticles = m_validParticles.length();

    
    // then we get the particle radius data
    if ( debugMode ) { printf("-> Reading Particle Radius\n"); }

    MDoubleArray	radiusArray;
    MPlug		radiusPPPlug = fnNode.findPlug( "radiusPP", &status );
    bool		haveRadiusArray = false;
    float		radius = 1.0;

    // check if there's a per-particle radius attribute
    if ( status == MS::kSuccess ) {
	MObject radiusPPObject;

	radiusPPPlug.getValue( radiusPPObject );

	MFnDoubleArrayData radiusArrayData( radiusPPObject, &status );

	radiusArray = radiusArrayData.array();

	haveRadiusArray = true;
    } else {
	// no per-particle radius. Try for a global radius

	MPlug radiusPlug = fnNode.findPlug( "radius", &status );

	if ( status == MS::kSuccess )
	    radiusPlug.getValue( radius );
	else {
	    // no radius attribute.. try pointSize

	    MPlug pointSizePlug = fnNode.findPlug( "pointSize", &status );

	    if ( status == MS::kSuccess )
		pointSizePlug.getValue( radius );
	}
    }

    status.clear();

    // then we get the particle color info
    if ( debugMode ) { printf("-> Reading Particle Color\n"); }

    MVectorArray	rgbArray;
    MPlug		rgbPPPlug = fnNode.findPlug( "rgbPP", &status );
    bool		haveRgbArray = false;

    if ( status == MS::kSuccess ) {
	MObject rgbPPObject;

	rgbPPPlug.getValue( rgbPPObject );

	MFnVectorArrayData rgbArrayData( rgbPPObject, &status );

	rgbArray = rgbArrayData.array();

	haveRgbArray = true;
    }

    status.clear();

    // and then we do any particle type specific work
    switch (particleType)
    {
	case MPTBlobbies: {
	    if ( debugMode ) { printf("-> Reading Blobby Particles\n"); }
		    
	    // setup the arrays to store the data to pass the correct codes to the
	    // implicit surface command 
	    MIntArray codeArray;
	    MFloatArray floatArray;
	    
	    if ( debugMode ) { printf("-> Reading Particle Data\n"); }

	    int floatOn = 0;

	    for ( unsigned part_num = 0;
		  part_num < m_numValidParticles;
		  part_num++ ) {

		/* add the particle to the list. */
		codeArray.append( 1001 );
		codeArray.append( floatOn );

		if ( haveRadiusArray )
		    radius = radiusArray[m_validParticles[part_num]];
		// else radius was set to either a scalar attribute or 1.0
		// above

		floatArray.append( radius * 2.0 );
		floatArray.append( 0.0 );
		floatArray.append( 0.0 );
		floatArray.append( 0.0 );

		floatArray.append( 0.0 );
		floatArray.append( radius * 2.0 );
		floatArray.append( 0.0 );
		floatArray.append( 0.0 );

		floatArray.append( 0.0 );
		floatArray.append( 0.0 );
		floatArray.append( radius * 2.0 );
		floatArray.append( 0.0 );

		floatArray.append( posArray[m_validParticles[part_num]].x );
		floatArray.append( posArray[m_validParticles[part_num]].y );
		floatArray.append( posArray[m_validParticles[part_num]].z );
		floatArray.append( 1.0 );

		floatOn += 16;
	    }
	    
	    if ( m_numValidParticles > 0 ) {
		codeArray.append( 0 );
		codeArray.append( m_numValidParticles );

		for ( unsigned k = 0; k < m_numValidParticles; k++ ) {
		    codeArray.append( k );
		}
	    }

	    if ( debugMode ) { printf("-> Setting up implicit data\n"); }
	    
	    bCodeArraySize = codeArray.length();
	    bCodeArray = ( RtInt* )lmalloc( sizeof( RtInt ) * bCodeArraySize );
	    codeArray.get( bCodeArray );

	    bFloatArraySize = floatArray.length();
	    bFloatArray = ( RtFloat* )lmalloc( sizeof( RtFloat ) * bFloatArraySize );
	    floatArray.get( bFloatArray );
	}
	break;

	case MPTPoints: {

	    liqTokenPointer Pparameter;

	    Pparameter.set( "P", rPoint, false, true, false, m_numValidParticles );

	    Pparameter.setDetailType( rVertex );

	    for ( unsigned part_num = 0;
		  part_num < m_numValidParticles;
		  part_num++ ) {

		Pparameter.setTokenFloat( part_num,
					  posArray[m_validParticles[part_num]].x,
					  posArray[m_validParticles[part_num]].y,
					  posArray[m_validParticles[part_num]].z );
	    }

	    tokenPointerArray.push_back( Pparameter );


	    // TODO: have we got to do some unit conversion here? what units
	    // are the radii in?  What unit is Maya in?
	    if (haveRadiusArray) {
		liqTokenPointer widthParameter;

		widthParameter.set( "width",
				    rFloat,
				    false,
				    true,
				    false,
				    m_numValidParticles );

		widthParameter.setDetailType( rVertex );

		for ( unsigned part_num = 0;
		      part_num < m_numValidParticles;
		      part_num++ ) {

		    widthParameter.setTokenFloat( part_num,
					     radiusArray[m_validParticles[part_num]]*2);
		}

		tokenPointerArray.push_back( widthParameter );
	    } else {

		liqTokenPointer constantwidthParameter;

		constantwidthParameter.set("constantwidth",
					   rFloat,
					   false,
					   false,
					   false,
					   0);
		constantwidthParameter.setDetailType(rConstant);
		constantwidthParameter.setTokenFloat(0, radius*2);

		tokenPointerArray.push_back( constantwidthParameter );
	    }
	}
	break;

	case MPTMultiPoint:
	case MPTMultiStreak:
	case MPTNumeric:
	case MPTSpheres:
	case MPTSprites:
	case MPTStreak:
	case MPTCloudy:
	case MPTTube:
	// do nothing. These are not supported
	break;
    }

    // and we add the Cs Parameter (if needed) after we've done everything
    // else
    if (haveRgbArray)
    {
	liqTokenPointer CsParameter;

	CsParameter.set( "Cs", rColor, false, true, false, m_numValidParticles );
	CsParameter.setDetailType( rVertex );

	for ( unsigned part_num = 0;
	      part_num < m_numValidParticles;
	      part_num++ ) {

	    CsParameter.setTokenFloat( part_num,
				       rgbArray[m_validParticles[part_num]].x,
				       rgbArray[m_validParticles[part_num]].y,
				       rgbArray[m_validParticles[part_num]].z );
	}

	tokenPointerArray.push_back( CsParameter );
    }

    addAdditionalParticleParameters( partobj );
}

liqRibParticleData::~liqRibParticleData()
//
//  Description:
//      class destructor
//
{
    // Free all arrays
    //
    if ( debugMode ) { printf("-> killing particles\n"); }
    
    if ( particleType == MPTBlobbies ) {
	lfree( bCodeArray );
	lfree( bFloatArray );
    }

}
void liqRibParticleData::write()
//
//  Description:
//      Write the RIB for this surface
//
{
    if ( debugMode ) { printf("-> writing particles\n"); }

    RiArchiveRecord( RI_COMMENT, "Number of Particles: %d", m_numValidParticles );

    RiArchiveRecord( RI_COMMENT,
		     "Number of Discarded Particles: %d",
		     m_numParticles - m_numValidParticles );

    unsigned	numTokens = tokenPointerArray.size();
    RtToken*	tokenArray = (RtToken *)alloca( sizeof(RtToken) * numTokens );
    RtPointer*	pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * numTokens );
    
    switch (particleType) {

	case MPTBlobbies: {
	    assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );

	    /*
	    RiBlobbyV( m_numValidParticles,
		       bCodeArraySize,
		       bCodeArray,
		       bFloatArraySize,
		       bFloatArray,
		       0,
		       bStringArray,
		       numTokens,
		       tokenArray,
		       pointerArray );
	    */
	}
	break;

	case MPTPoints: {
	    assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );

	    RiPointsV( m_numValidParticles, numTokens, tokenArray, pointerArray );
	}
	break;

	case MPTMultiPoint:
	case MPTMultiStreak:
	case MPTNumeric:
	case MPTSpheres:
	case MPTSprites:
	case MPTStreak:
	case MPTCloudy:
	case MPTTube:
	// do nothing. These are not supported
	break;
    }
}

bool liqRibParticleData::compare( const liquidRibData & otherObj ) const
//
//  Description:
//      Compare this curve to the other for the purpose of determining
//      if it is animated.
//
{	
    if ( debugMode ) { printf("-> comparing particles\n"); }

    if ( otherObj.type() != MRT_Particles )
	return false;

    const liqRibParticleData & other = (liqRibParticleData&)otherObj;
    
    if ( m_numParticles != other.m_numParticles ) 
    {
        return false;
    }
    
    return true;
}

ObjectType liqRibParticleData::type() const
//
//  Description:
//      return the geometry type
//
{
    if ( debugMode ) { printf("-> returning particle type\n"); }
    return MRT_Particles;
}

void liqRibParticleData::addAdditionalParticleParameters( MObject node )
//
//  Description:
//      this replaces the standard method for attaching custom attributes to a
//      particle set to be passed into the rib stream for access in a prman
//      shader
//
{
    if ( debugMode ) {printf("-> scanning for additional rman surface attributes \n");}
    
    MFnDependencyNode nodeFn( node );

    addAdditionalFloatParameters( nodeFn );
    addAdditionalPointParameters( nodeFn );
    addAdditionalVectorParameters( nodeFn );
    addAdditionalColorParameters( nodeFn );
}

void liqRibParticleData::addAdditionalFloatParameters( MFnDependencyNode nodeFn )
{
    MStringArray	foundAttributes = FindAttributesByPrefix( "rmanF", nodeFn );
    MStatus		status;

    for ( int i = 0; i < foundAttributes.length(); i++ ) {
	liqTokenPointer	floatParameter;
	MString		currAttribute = foundAttributes[i];
	MString		cutString = currAttribute.substring(5, currAttribute.length());

	MPlug		fPlug = nodeFn.findPlug( currAttribute );
	MObject		plugObj;

	status = fPlug.getValue( plugObj );

	if ( plugObj.apiType() == MFn::kDoubleArrayData ) {
	    MFnDoubleArrayData  attributeData( plugObj );

	    floatParameter.set( cutString.asChar(),
				rFloat,
				false,
				true,
				false,
				m_numValidParticles );
	    floatParameter.setDetailType(rVertex);

	    for ( unsigned part_num = 0; part_num < m_numValidParticles; part_num++ ) {
		floatParameter.setTokenFloat(part_num,
					     attributeData[m_validParticles[part_num]]);
	    }
	} else {
	    float floatValue;

	    fPlug.getValue( floatValue );

	    floatParameter.set( cutString.asChar(),
				rFloat,
				false,
				false,
				false,
				0 );
	    floatParameter.setDetailType(rConstant);
	    floatParameter.setTokenFloat( 0, floatValue );
	}

	tokenPointerArray.push_back( floatParameter );
    }
}

void liqRibParticleData::addAdditionalPointParameters( MFnDependencyNode nodeFn )
{
    MStringArray	foundAttributes = FindAttributesByPrefix( "rmanP", nodeFn );
    MStatus		status;

    for ( int i = 0; i < foundAttributes.length(); i++ ) {
	liqTokenPointer	pointParameter;
	MString		currAttribute = foundAttributes[i];
	MString		cutString = currAttribute.substring(5, currAttribute.length());

	MPlug		pPlug = nodeFn.findPlug( currAttribute );
	MObject		plugObj;

	status = pPlug.getValue( plugObj );

	if ( plugObj.apiType() == MFn::kVectorArrayData ) {
	    MFnVectorArrayData  attributeData( plugObj );

	    pointParameter.set( cutString.asChar(),
				rPoint,
				false,
				true,
				false,
				m_numValidParticles );
	    pointParameter.setDetailType(rVertex);

	    for ( unsigned part_num = 0; part_num < m_numValidParticles; part_num++ ) {
		pointParameter.setTokenFloat( part_num,
					  attributeData[m_validParticles[part_num]].x,
					  attributeData[m_validParticles[part_num]].y,
					  attributeData[m_validParticles[part_num]].z );
	    }

	    tokenPointerArray.push_back( pointParameter );
	} else if (plugObj.apiType() == MFn::kData3Double) {
	    float x, y, z;
	    pPlug.child(0).getValue( x );
	    pPlug.child(1).getValue( y );
	    pPlug.child(2).getValue( z );

	    pointParameter.set( cutString.asChar(), rPoint, false, false, false, 0 );
	    pointParameter.setTokenFloat( 0, x, y, z );
	    pointParameter.setDetailType( rConstant );

	    tokenPointerArray.push_back( pointParameter );
	}
	// else ignore this attribute
    }
}

void liqRibParticleData::addAdditionalVectorParameters( MFnDependencyNode nodeFn )
{
    MStringArray	foundAttributes = FindAttributesByPrefix( "rmanV", nodeFn );
    MStatus		status;

    for ( int i = 0; i < foundAttributes.length(); i++ ) {
	liqTokenPointer	vectorParameter;
	MString		currAttribute = foundAttributes[i];
	MString		cutString = currAttribute.substring(5, currAttribute.length());

	MPlug		vPlug = nodeFn.findPlug( currAttribute );
	MObject		plugObj;

	status = vPlug.getValue( plugObj );

	if ( plugObj.apiType() == MFn::kVectorArrayData ) {
	    MFnVectorArrayData  attributeData( plugObj );

	    vectorParameter.set( cutString.asChar(),
				 rVector,
				 false,
				 true,
				 false,
				 m_numValidParticles );
	    vectorParameter.setDetailType(rVertex);

	    for ( unsigned part_num = 0; part_num < m_numValidParticles; part_num++ ) {
		vectorParameter.setTokenFloat( part_num,
					  attributeData[m_validParticles[part_num]].x,
					  attributeData[m_validParticles[part_num]].y,
					  attributeData[m_validParticles[part_num]].z );
	    }

	    tokenPointerArray.push_back( vectorParameter );
	} else if (plugObj.apiType() == MFn::kData3Double) {

	    float x, y, z;
	    vPlug.child(0).getValue( x );
	    vPlug.child(1).getValue( y );
	    vPlug.child(2).getValue( z );

	    vectorParameter.set( cutString.asChar(), rVector, false, false, false, 0 );
	    vectorParameter.setTokenFloat( 0, x, y, z );
	    vectorParameter.setDetailType( rConstant );

	    tokenPointerArray.push_back( vectorParameter );
	}
	// else ignore this attribute
    }
}

void liqRibParticleData::addAdditionalColorParameters( MFnDependencyNode nodeFn )
{
    MStringArray	foundAttributes = FindAttributesByPrefix( "rmanC", nodeFn );
    MStatus		status;

    for ( int i = 0; i < foundAttributes.length(); i++ ) {
	liqTokenPointer	colorParameter;
	MString		currAttribute = foundAttributes[i];
	MString		cutString = currAttribute.substring(5, currAttribute.length());

	MPlug		cPlug = nodeFn.findPlug( currAttribute );
	MObject		plugObj;

	status = cPlug.getValue( plugObj );

	if ( plugObj.apiType() == MFn::kVectorArrayData ) {
	    MFnVectorArrayData  attributeData( plugObj );

	    colorParameter.set( cutString.asChar(),
				rColor,
				false,
				true,
				false,
				m_numValidParticles );
	    colorParameter.setDetailType(rVertex);

	    for ( unsigned part_num = 0; part_num < m_numValidParticles; part_num++ ) {
		colorParameter.setTokenFloat( part_num,
					  attributeData[m_validParticles[part_num]].x,
					  attributeData[m_validParticles[part_num]].y,
					  attributeData[m_validParticles[part_num]].z );
	    }

	    tokenPointerArray.push_back( colorParameter );
	} else if (plugObj.apiType() == MFn::kData3Double) {
	    float r, g, b;
	    cPlug.child(0).getValue( r );
	    cPlug.child(1).getValue( g );
	    cPlug.child(2).getValue( b );

	    colorParameter.set( cutString.asChar(), rColor, false, false, false, 0 );
	    colorParameter.setTokenFloat( 0, r, g, b );
	    colorParameter.setDetailType( rConstant );

	    tokenPointerArray.push_back( colorParameter );
	}
	// else ignore this attribute
    }
}
