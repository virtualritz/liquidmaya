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
#include <liquidRibParticleData.h>
#include <liquidMemory.h>

extern int debugMode;

extern RtFloat liqglo_sampleTimes[5];
extern liquidlong liqglo_motionSamples;
extern bool liqglo_doDef;
extern bool liqglo_doMotion;

liquidRibParticleData::liquidRibParticleData( MObject partobj )
//
//  Description:
//      create a RIB compatible representation of a Maya particles
//
:  nparticles(0)

{
    if ( debugMode ) { printf("-> creating particles\n"); }
    MStatus status = MS::kSuccess;
    MFnDependencyNode  fnNode(partobj);

    /* FEATURE: August 8th 2001 - initial implementation of multiple particle types */

    MPlug particleRenderTypePlug = fnNode.findPlug( "particleRenderType", &status );
    short particleTypeVal;
    particleRenderTypePlug.getValue( particleTypeVal );
    particleType = (pType)particleTypeVal;
	
    /* particle age information */
    MPlug agePlug = fnNode.findPlug( "age", &status );
    MObject ageObject;
    agePlug.getValue( ageObject );
    MFnDoubleArrayData ageArray( ageObject, &status );
	
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

    /* id information */
    MPlug idPlug = fnNode.findPlug( "particleId", &status );
    MObject idObject;
    idPlug.getValue( idObject );
    MFnDoubleArrayData idArray( idObject, &status );

    status.clear();

	/* FEATURE: August 8th 2001 - added in for custom particle parameters */

    if ( particleType == MPTBlobbies ) {
	if ( debugMode ) { printf("-> Reading Blobby Particles\n"); }
	MPlug positionPlug = fnNode.findPlug( "position", &status );
	status.clear();
	float radius = 1.0;
	bool radiusArray = false;

	if ( debugMode ) { printf("-> Reading Blobby Radius\n"); }
	MPlug widthPlug = fnNode.findPlug( "radius", &status );
	if ( status == MS::kSuccess ) widthPlug.getValue( radius );
	status.clear();

	MDoubleArray widthArray;
	widthPlug = fnNode.findPlug( "radiusPP", &status );
	if ( status == MS::kSuccess ) {
	    MObject widthObject;
	    widthPlug.getValue( widthObject );
	    MFnDoubleArrayData widthArrayData( widthObject, &status );
	    widthArray = widthArrayData.array();
	    radiusArray = true;
	}
	status.clear();

	if ( debugMode ) { printf("-> Reading Blobby Count\n"); }
	MObject posObject;
	positionPlug.getValue( posObject );
	MFnVectorArrayData posArray( posObject, &status );
	nparticles = posArray.length();

	/* setup the arrays to store the data to pass the correct codes to the implicit surface command */
	MIntArray codeArray;
	MFloatArray floatArray;

	if ( debugMode ) { printf("-> Reading Particle Data\n"); }
	int numUsed = 0;
	int floatOn = 0;
	unsigned k;
	for ( k = 0; k < nparticles; k++ ) {
	    /* check to make sure the particle hasn't been born throughout the number of samples. */
	    double birthTime = birthArray[k] * 24.0;
	    double sampleTime = (double)liqglo_sampleTimes[0];
	    double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
	    double lastSampleTime;
	    if ( liqglo_doMotion || liqglo_doDef ) {
		    lastSampleTime = (double)liqglo_sampleTimes[liqglo_motionSamples - 1];
	    } else {
		    lastSampleTime = (double)liqglo_sampleTimes[0];
	    }


	    if ( ( deathTime > lastSampleTime + 1.0 ) && ( birthTime < sampleTime - 1.0) ) {
		/* add the particle to the list. */
		codeArray.append( 1001 );
		codeArray.append( floatOn );
		if ( radiusArray ) radius = widthArray[k];
		floatArray.append( radius * 2.0 ); floatArray.append( 0.0 ); floatArray.append( 0.0 ); floatArray.append( 0.0 );
		floatArray.append( 0.0 ); floatArray.append( radius * 2.0 ); floatArray.append( 0.0 ); floatArray.append( 0.0 );
		floatArray.append( 0.0 ); floatArray.append( 0.0 ); floatArray.append( radius * 2.0 ); floatArray.append( 0.0 );
		floatArray.append( posArray[k].x ); floatArray.append( posArray[k].y ); floatArray.append( posArray[k].z ); floatArray.append( 1.0 );
		numUsed++;
		floatOn += 16;
	    } 
	}
	nparticles = numUsed;

	if ( nparticles > 0 ) {
		codeArray.append( 0 );
		codeArray.append( nparticles - 1 );
		for ( k = 0; k < nparticles - 1; k++ ) {
			codeArray.append( k );
		}
	}

	// we now setup the appropriate arrays! Yeaha!

	if ( debugMode ) { printf("-> Setting up implicit data\n"); }

	bCodeArraySize = codeArray.length();
	bCodeArray = ( RtInt* )lmalloc( sizeof( RtInt ) * bCodeArraySize );
	codeArray.get( bCodeArray );

	bFloatArraySize = floatArray.length();
	bFloatArray = ( RtFloat* )lmalloc( sizeof( RtFloat ) * bFloatArraySize );
	floatArray.get( bFloatArray );

    } else {
	if ( debugMode ) { printf("-> Reading Point Particles\n"); }
	MPlug positionPlug = fnNode.findPlug( "position", &status );

	if ( status == MS::kSuccess ) {
	    MObject posObject;
	    positionPlug.getValue( posObject );
	    MFnVectorArrayData posArray( posObject, &status );
	    nparticles = posArray.length();

	    liqTokenPointer tokenPointerPair;
	    tokenPointerPair.set( "P", rPoint, false, true, false, nparticles );
	    tokenPointerPair.setDetailType( rVertex );


	    int numUsed = 0;
	    unsigned k;
	    for ( k = 0; k < nparticles; k++ ) {
		/* check to make sure the particle hasn't been born throughout the number of samples. */
		double birthTime = birthArray[k] * 24.0;
		double sampleTime = (double)liqglo_sampleTimes[0];
		double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
		double lastSampleTime;
		if ( liqglo_doMotion || liqglo_doDef ) {
			lastSampleTime = (double)liqglo_sampleTimes[liqglo_motionSamples - 1];
		} else {
			lastSampleTime = (double)liqglo_sampleTimes[0];
		}

		if ( ( deathTime > lastSampleTime ) && ( birthTime < sampleTime) ) {
			/* add the particle to the list. */
			tokenPointerPair.setTokenFloat( numUsed, posArray[k].x, posArray[k].y, posArray[k].z );
			numUsed++;
		} 
	    }
    	    tokenPointerPair.adjustArraySize( numUsed );
	    tokenPointerArray.push_back( tokenPointerPair );
	    nparticles = numUsed;
	}

	status.clear();
	/* check for point colors */
	MPlug colorPlug = fnNode.findPlug( "rgbPP", &status );
	if ( debugMode ) { printf("-> getting particles color\n"); }
	if ( status == MS::kSuccess ) {
	    MObject colObject;
	    colorPlug.getValue( colObject );
	    MFnVectorArrayData colArray( colObject, &status );
	    int ncolor = colArray.length();
	    liqTokenPointer tokenPointerPair;
	    tokenPointerPair.set( "Cs", rColor, false, true, false, ncolor );
	    tokenPointerPair.setDetailType( rVertex );

	    unsigned k, numUsed = 0;
	    for ( k = 0; k < ncolor; k++ ) {
		/* check to make sure the particle hasn't been born throughout the number of samples. */
		double birthTime = birthArray[k] * 24.0;
		double sampleTime = (double)liqglo_sampleTimes[0];
		double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
		double lastSampleTime;
		if ( liqglo_doMotion || liqglo_doDef ) {
			lastSampleTime = (double)liqglo_sampleTimes[liqglo_motionSamples - 1];
		} else {
			lastSampleTime = (double)liqglo_sampleTimes[0];
		}

		if ( ( deathTime >= lastSampleTime ) && ( birthTime <= sampleTime) ) {
			tokenPointerPair.setTokenFloat( numUsed, colArray[k].x, colArray[k].y, colArray[k].z );
			numUsed++;
		}
	    }

	    tokenPointerPair.adjustArraySize( numUsed );
	    tokenPointerArray.push_back( tokenPointerPair );
	}
	status.clear();

	/* check for point widths */
	if ( debugMode ) { printf("-> getting particles width\n"); }
	MPlug widthPlug = fnNode.findPlug( "radiusPP", &status );
	if ( status == MS::kSuccess ) {
	    MObject widthObject;
	    widthPlug.getValue( widthObject );
	    MFnDoubleArrayData widthArray( widthObject, &status );
	    int nwidth = widthArray.length();
	    liqTokenPointer tokenPointerPair;
	    tokenPointerPair.set( "width", rFloat, false, true, false, nwidth );
	    tokenPointerPair.setDetailType( rVertex );

	    unsigned k, numUsed = 0;
	    for ( k = 0; k < nwidth; k++ ) {
		/* check to make sure the particle hasn't been born throughout the number of samples. */
		double birthTime = birthArray[k] * 24.0;
		double sampleTime = (double)liqglo_sampleTimes[0];
		double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
		double lastSampleTime;
		if ( liqglo_doMotion || liqglo_doDef ) {
			lastSampleTime = (double)liqglo_sampleTimes[liqglo_motionSamples - 1];
		} else {
			lastSampleTime = (double)liqglo_sampleTimes[0];
		}

		if ( ( deathTime >= lastSampleTime ) && ( birthTime <= sampleTime) ) {
			tokenPointerPair.setTokenFloat( numUsed, widthArray[k] );
			numUsed++;
		}
	    }
	    tokenPointerPair.adjustArraySize( numUsed );
	    tokenPointerArray.push_back( tokenPointerPair );
	}
	status.clear();

    }

    /* FEATURE: August 8th 2001 - added in for custom particle parameters */
    addAdditionalParticleParameters( partobj );

}

liquidRibParticleData::~liquidRibParticleData()
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
void liquidRibParticleData::write()
//
//  Description:
//      Write the RIB for this surface
//
{
	if ( debugMode ) { printf("-> writing particles\n"); }
	RiArchiveRecord( RI_COMMENT, "Number of Particles: %d", nparticles );
	unsigned numTokens = tokenPointerArray.size();
	RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * numTokens );
	RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * numTokens );
	
	/* FEATURE: August 8th 2001 - initial implementation of multiple particle types */

	if ( particleType == MPTBlobbies ) {
		assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );

		/* TEMPCOMMENT */
		/*RiBlobbyV( nparticles, bCodeArraySize, bCodeArray, bFloatArraySize, bFloatArray, 0, bStringArray, numTokens, tokenArray, pointerArray );*/
	} else {
		assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );
	
		RiPointsV( nparticles, numTokens, tokenArray, pointerArray );
	}
}

bool liquidRibParticleData::compare( const liquidRibData & otherObj ) const
//
//  Description:
//      Compare this curve to the other for the purpose of determining
//      if it is animated.
//
{	
    if ( debugMode ) { printf("-> comparing particles\n"); }
    if ( otherObj.type() != MRT_Particles ) return false;
    const liquidRibParticleData & other = (liquidRibParticleData&)otherObj;
    
    if ( nparticles != other.nparticles ) 
    {
        return false;
    }
    
    return true;
}

ObjectType liquidRibParticleData::type() const
//
//  Description:
//      return the geometry type
//
{
	if ( debugMode ) { printf("-> returning particle type\n"); }
	return MRT_Particles;
}

/* FEATURE: August 8th 2001 - added in for custom particle parameters */

void liquidRibParticleData::addAdditionalParticleParameters( MObject node )
//
//  Description:
//      this replaces the standard method for attaching custom attributes to a particle set to be passed
//      into the rib stream for access in a prman shader
//
{
	
    if ( debugMode ) { printf("-> scanning for additional rman surface attributes \n"); }

    MStatus status = MS::kSuccess;
    unsigned i;

    // find how many additional
    MFnDependencyNode nodeFn( node );

    /* particle age information */
    MPlug agePlug = nodeFn.findPlug( "age", &status );
    MObject ageObject;
    agePlug.getValue( ageObject );
    MFnDoubleArrayData ageArray( ageObject, &status );

    /* birth time information */
    MPlug birthPlug = nodeFn.findPlug( "birthTime", &status );
    MObject birthObject;
    birthPlug.getValue( birthObject );
    MFnDoubleArrayData birthArray( birthObject, &status );

    /* lifespan information */
    MStatus lifePPStatus;
    MPlug lifePPPlug = nodeFn.findPlug( "finalLifespanPP", &lifePPStatus );
    MObject lifePPObject;
    lifePPPlug.getValue( lifePPObject );
    MFnDoubleArrayData lifePPArray( lifePPObject, &status );


    // find the attributes
    MStringArray floatAttributesFound = FindAttributesByPrefix( "rmanF", nodeFn );
    MStringArray pointAttributesFound = FindAttributesByPrefix( "rmanP", nodeFn );
    MStringArray vectorAttributesFound = FindAttributesByPrefix( "rmanV", nodeFn );
    MStringArray colourAttributesFound = FindAttributesByPrefix( "rmanC", nodeFn );

    if ( floatAttributesFound.length() > 0 ) {
	for ( i = 0; i < floatAttributesFound.length(); i++ ) {
	    liqTokenPointer tokenPointerPair;
	    MString cutString = floatAttributesFound[i].substring(5, floatAttributesFound[i].length());
	    MPlug fPlug = nodeFn.findPlug( floatAttributesFound[i] );
	    MObject plugObj;
	    status = fPlug.getValue( plugObj );
	    if ( plugObj.apiType() == MFn::kDoubleArrayData ) 
	    {
		MFnDoubleArrayData  fnDoubleArrayData( plugObj );
		int nwidth = fnDoubleArrayData.length();

		unsigned k, numUsed = 0;
		tokenPointerPair.set( cutString.asChar(), rFloat, false, true, false, nwidth );

		for ( k = 0; k < nwidth; k++ ) {
		    /* check to make sure the particle hasn't been born throughout the number of samples. */
		    double birthTime = birthArray[k] * 24.0;
		    double sampleTime = (double)liqglo_sampleTimes[0];
		    double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
		    double lastSampleTime;
		    if ( liqglo_doMotion || liqglo_doDef ) {
			    lastSampleTime = (double)liqglo_sampleTimes[liqglo_motionSamples - 1];
		    } else {
			    lastSampleTime = (double)liqglo_sampleTimes[0];
		    }

		    if ( ( deathTime >= lastSampleTime ) && ( birthTime <= sampleTime) ) {
			    tokenPointerPair.setTokenFloat( numUsed, fnDoubleArrayData[k] );
			    numUsed++;
		    }
		}
	    	tokenPointerPair.adjustArraySize( numUsed );
		tokenPointerPair.setDetailType( rVertex );
		tokenPointerArray.push_back( tokenPointerPair );
	    } else {
		float floatValue;
		fPlug.getValue( floatValue );
		tokenPointerPair.set( cutString.asChar(), rFloat, false, false, false, 0 );
    	    	tokenPointerPair.setTokenFloat( 0, floatValue );
		tokenPointerPair.setDetailType( rConstant );
		tokenPointerArray.push_back( tokenPointerPair );
		}
	}
    }
    // Hmmmmm should write a function for three next blocks ...
    if ( pointAttributesFound.length() > 0 ) {
	for ( i = 0; i < pointAttributesFound.length(); i++ ) {
	    liqTokenPointer tokenPointerPair;
	    MString cutString = pointAttributesFound[i].substring(5, pointAttributesFound[i].length());
	    MPlug pPlug = nodeFn.findPlug( pointAttributesFound[i] );
	    MObject plugObj;
	    status = pPlug.getValue( plugObj );
	    if ( plugObj.apiType() == MFn::kPointArrayData ) 
	    {
		MFnVectorArrayData  fnPointArrayData( plugObj );
		nparticles = fnPointArrayData.length();
		tokenPointerPair.set( cutString.asChar(), rPoint, false, true, false, ( unsigned int ) nparticles );

		int numUsed = 0;
		unsigned k;
		for ( k = 0; k < nparticles; k++ ) {
		    /* check to make sure the particle hasn't been born throughout the number of samples. */
		    double birthTime = birthArray[k] * 24.0;
		    double sampleTime = (double)liqglo_sampleTimes[0];
		    double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
		    double lastSampleTime;
		    if ( liqglo_doMotion || liqglo_doDef ) {
			    lastSampleTime = (double)liqglo_sampleTimes[liqglo_motionSamples - 1];
		    } else {
			    lastSampleTime = (double)liqglo_sampleTimes[0];
		    }

		    if ( ( deathTime >= lastSampleTime ) && ( birthTime <= sampleTime) ) {
			    /* add the particle to the list. */
			    tokenPointerPair.setTokenFloat( numUsed, fnPointArrayData[k].x, fnPointArrayData[k].y, fnPointArrayData[k].z );
			    numUsed++;
		    } 
		}

	    	tokenPointerPair.adjustArraySize( numUsed );
		tokenPointerPair.setDetailType( rVertex );
		tokenPointerArray.push_back( tokenPointerPair );
	    } else {
		tokenPointerPair.set( cutString.asChar(), rPoint, false, false, false, 0 );
    	    	float x, y, z;
		pPlug.child(0).getValue( x );
		pPlug.child(1).getValue( y );
		pPlug.child(2).getValue( z );
    	    	tokenPointerPair.setTokenFloat( 0, x, y, z );
		tokenPointerPair.setDetailType( rConstant );
		tokenPointerArray.push_back( tokenPointerPair );
	    }
	}
    }

    if ( vectorAttributesFound.length() > 0 ) {
	for ( i = 0; i < vectorAttributesFound.length(); i++ ) {
	    liqTokenPointer tokenPointerPair;
	    MString cutString = vectorAttributesFound[i].substring(5, vectorAttributesFound[i].length());
	    MPlug vPlug = nodeFn.findPlug( vectorAttributesFound[i] );
	    MObject plugObj;
	    status = vPlug.getValue( plugObj );
	    if ( plugObj.apiType() == MFn::kVectorArrayData ) 
	    {
		MFnVectorArrayData  fnVectorArrayData( plugObj );
		MVectorArray vectorArrayData = fnVectorArrayData.array( &status );
		int ncolor = vectorArrayData.length();
		tokenPointerPair.set( cutString.asChar(), rVector, false, true, false, ( unsigned int ) ncolor );

		unsigned k, numUsed = 0;
		for ( k = 0; k < ncolor; k++ ) {
		    /* check to make sure the particle hasn't been born throughout the number of samples. */
		    double birthTime = birthArray[k] * 24.0;
		    double sampleTime = (double)liqglo_sampleTimes[0];
		    double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
		    double lastSampleTime;
		    if ( liqglo_doMotion || liqglo_doDef ) {
			    lastSampleTime = (double)liqglo_sampleTimes[liqglo_motionSamples - 1];
		    } else {
			    lastSampleTime = (double)liqglo_sampleTimes[0];
		    }

		    if ( ( deathTime >= lastSampleTime ) && ( birthTime <= sampleTime) ) {
			    tokenPointerPair.setTokenFloat( numUsed, vectorArrayData[k].x, vectorArrayData[k].y, vectorArrayData[k].z );
			    numUsed++;
		    }
		    // Hmmmmm looks like this should be inside if
		    //numUsed++;
		}

	    	tokenPointerPair.adjustArraySize( numUsed );
		tokenPointerPair.setDetailType( rVertex );
		tokenPointerArray.push_back( tokenPointerPair );
	    } else {
		tokenPointerPair.set( cutString.asChar(), rVector, false, false, false, 0 );
    	    	float x, y, z;
		vPlug.child(0).getValue( x );
		vPlug.child(1).getValue( y );
		vPlug.child(2).getValue( z );
    	    	tokenPointerPair.setTokenFloat( 0, x, y, z );
		tokenPointerPair.setDetailType( rConstant );
		tokenPointerArray.push_back( tokenPointerPair );
	    }
	}
    }

    if ( colourAttributesFound.length() > 0 ) {
	for ( i = 0; i < colourAttributesFound.length(); i++ ) {
	    liqTokenPointer tokenPointerPair;
	    MString cutString = colourAttributesFound[i].substring(5, colourAttributesFound[i].length());
	    MPlug vPlug = nodeFn.findPlug( colourAttributesFound[i] );
	    MObject plugObj;
	    status = vPlug.getValue( plugObj );
	    if ( plugObj.apiType() == MFn::kVectorArrayData ) {
		MFnVectorArrayData  fnVectorArrayData( plugObj );
		MVectorArray vectorArrayData = fnVectorArrayData.array( &status );
		int ncolor = vectorArrayData.length();
		tokenPointerPair.set( cutString.asChar(), rColor, false, true, false, ( unsigned int ) ncolor );

		unsigned k, numUsed = 0;
		for ( k = 0; k < ncolor; k++ ) {
		    /* check to make sure the particle hasn't been born throughout the number of samples. */
		    double birthTime = birthArray[k] * 24.0;
		    double sampleTime = (double)liqglo_sampleTimes[0];
		    double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
		    double lastSampleTime;
		    if ( liqglo_doMotion || liqglo_doDef ) {
			    lastSampleTime = (double)liqglo_sampleTimes[liqglo_motionSamples - 1];
		    } else {
			    lastSampleTime = (double)liqglo_sampleTimes[0];
		    }

		    if ( ( deathTime >= lastSampleTime ) && ( birthTime <= sampleTime) ) {
			    tokenPointerPair.setTokenFloat( numUsed, vectorArrayData[k].x, vectorArrayData[k].y, vectorArrayData[k].z );
			    numUsed++;
		    }
		}

	    	tokenPointerPair.adjustArraySize( numUsed );
		tokenPointerPair.setDetailType( rVertex );
		tokenPointerArray.push_back( tokenPointerPair );
	    } else {
		tokenPointerPair.set( cutString.asChar(), rColor, false, false, false, 0 );
    	    	float x, y, z;
		vPlug.child(0).getValue( x );
		vPlug.child(1).getValue( y );
		vPlug.child(2).getValue( z );
    	    	tokenPointerPair.setTokenFloat( 0, x, y, z );
		tokenPointerPair.setDetailType( rConstant );
		tokenPointerArray.push_back( tokenPointerPair );
	    }
	}
    }
}
