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
#include <malloc.h>
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
extern long msampleOn;
extern RtFloat sampleTimes[5];
extern liquidlong motionSamples;
extern bool doDef;
extern bool doMotion;

RibParticleData::RibParticleData( MObject partobj )
//
//  Description:
//      create a RIB compatible representation of a Maya particles
//
:   particles( NULL ), pcolor( NULL ), pwidth( NULL ), popac( NULL )

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
			double sampleTime = (double)sampleTimes[0];
			double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
			double lastSampleTime;
			if ( doMotion || doDef ) {
				lastSampleTime = (double)sampleTimes[motionSamples - 1];
			} else {
				lastSampleTime = (double)sampleTimes[0];
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
			particles   = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( nparticles * 3 ) );
			
			RtFloat* partPtr = particles;
			int numUsed = 0;
			unsigned k;
			for ( k = 0; k < nparticles; k++ ) {
				/* check to make sure the particle hasn't been born throughout the number of samples. */
				double birthTime = birthArray[k] * 24.0;
				double sampleTime = (double)sampleTimes[0];
				double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
				double lastSampleTime;
				if ( doMotion || doDef ) {
					lastSampleTime = (double)sampleTimes[motionSamples - 1];
				} else {
					lastSampleTime = (double)sampleTimes[0];
				}

				if ( ( deathTime > lastSampleTime ) && ( birthTime < sampleTime) ) {
					/* add the particle to the list. */
					*partPtr++ = (RtFloat)posArray[k].x;
					*partPtr++ = (RtFloat)posArray[k].y;
					*partPtr++ = (RtFloat)posArray[k].z;
					numUsed++;
				} 
			}
			
			rTokenPointer tokenPointerPair;
			tokenPointerPair.pType = rPoint;
			sprintf( tokenPointerPair.tokenName, "P" );
			tokenPointerPair.arraySize = numUsed;
			tokenPointerPair.tokenFloats = particles;
			tokenPointerPair.isArray = true;
			tokenPointerPair.isUArray = false;
			tokenPointerPair.isNurbs = false;
			tokenPointerPair.dType = rVertex;
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
			pcolor   = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( ncolor * 3 ) );
			
			RtFloat* partCPtr = pcolor;
			unsigned k, numUsed = 0;
			for ( k = 0; k < ncolor; k++ ) {
				/* check to make sure the particle hasn't been born throughout the number of samples. */
				double birthTime = birthArray[k] * 24.0;
				double sampleTime = (double)sampleTimes[0];
				double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
				double lastSampleTime;
				if ( doMotion || doDef ) {
					lastSampleTime = (double)sampleTimes[motionSamples - 1];
				} else {
					lastSampleTime = (double)sampleTimes[0];
				}
				
				if ( ( deathTime >= lastSampleTime ) && ( birthTime <= sampleTime) ) {
					*partCPtr++ = (RtFloat)colArray[k].x;
					*partCPtr++ = (RtFloat)colArray[k].y;
					*partCPtr++ = (RtFloat)colArray[k].z;
					numUsed++;
				}
			}
			
			rTokenPointer tokenPointerPair;
			tokenPointerPair.pType = rColor;
			sprintf( tokenPointerPair.tokenName, "Cs" );
			tokenPointerPair.arraySize = numUsed;
			tokenPointerPair.tokenFloats = pcolor;
			tokenPointerPair.isArray = true;
			tokenPointerPair.isUArray = false;
			tokenPointerPair.isNurbs = false;
			tokenPointerPair.dType = rVertex;
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
			pwidth   = (RtFloat*)lmalloc( sizeof( RtFloat ) * nwidth );
			
			RtFloat* partPtr = pwidth;
			unsigned k, numUsed = 0;
			for ( k = 0; k < nwidth; k++ ) {
				/* check to make sure the particle hasn't been born throughout the number of samples. */
				double birthTime = birthArray[k] * 24.0;
				double sampleTime = (double)sampleTimes[0];
				double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
				double lastSampleTime;
				if ( doMotion || doDef ) {
					lastSampleTime = (double)sampleTimes[motionSamples - 1];
				} else {
					lastSampleTime = (double)sampleTimes[0];
				}
				
				if ( ( deathTime >= lastSampleTime ) && ( birthTime <= sampleTime) ) {
					*partPtr++ = (RtFloat)widthArray[k];
					numUsed++;
				}
			}
			
			rTokenPointer tokenPointerPair;
			tokenPointerPair.pType = rFloat;
			sprintf( tokenPointerPair.tokenName, "width" );
			tokenPointerPair.arraySize = numUsed;
			tokenPointerPair.tokenFloats = pwidth;
			tokenPointerPair.isArray = true;
			tokenPointerPair.isUArray = false;
			tokenPointerPair.isNurbs = false;
			tokenPointerPair.dType = rVertex;
			tokenPointerArray.push_back( tokenPointerPair );
		}
		status.clear();
		
	}

	/* FEATURE: August 8th 2001 - added in for custom particle parameters */
	addAdditionalParticleParameters( partobj );

}

RibParticleData::~RibParticleData()
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
void RibParticleData::write()
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

bool RibParticleData::compare( const RibData & otherObj ) const
//
//  Description:
//      Compare this curve to the other for the purpose of determining
//      if it is animated.
//
{	
    if ( debugMode ) { printf("-> comparing particles\n"); }
    if ( otherObj.type() != MRT_Particles ) return false;
    const RibParticleData & other = (RibParticleData&)otherObj;
    
    if ( nparticles != other.nparticles ) 
    {
        return false;
    }
    
    return true;
}

ObjectType RibParticleData::type() const
//
//  Description:
//      return the geometry type
//
{
	if ( debugMode ) { printf("-> returning particle type\n"); }
	return MRT_Particles;
}

/* FEATURE: August 8th 2001 - added in for custom particle parameters */

void RibParticleData::addAdditionalParticleParameters( MObject node )
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
			rTokenPointer tokenPointerPair;
			MString cutString = floatAttributesFound[i].substring(5, floatAttributesFound[i].length());
			sprintf( tokenPointerPair.tokenName, cutString.asChar() ) ;
			MPlug fPlug = nodeFn.findPlug( floatAttributesFound[i] );
			MObject plugObj;
			status = fPlug.getValue( plugObj );
			tokenPointerPair.pType = rFloat;
			tokenPointerPair.isNurbs = false;
			if ( plugObj.apiType() == MFn::kDoubleArrayData ) 
			{
				MFnDoubleArrayData  fnDoubleArrayData( plugObj );
				int nwidth = fnDoubleArrayData.length();
				pwidth   = (RtFloat*)lmalloc( sizeof( RtFloat ) * nwidth );
				
				RtFloat* partPtr = pwidth;
				unsigned k, numUsed = 0;

				for ( k = 0; k < nwidth; k++ ) {
					/* check to make sure the particle hasn't been born throughout the number of samples. */
					double birthTime = birthArray[k] * 24.0;
					double sampleTime = (double)sampleTimes[0];
					double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
					double lastSampleTime;
					if ( doMotion || doDef ) {
						lastSampleTime = (double)sampleTimes[motionSamples - 1];
					} else {
						lastSampleTime = (double)sampleTimes[0];
					}
					
					if ( ( deathTime >= lastSampleTime ) && ( birthTime <= sampleTime) ) {
						*partPtr++ = (RtFloat)fnDoubleArrayData[k];
						numUsed++;
					}
				}

				tokenPointerPair.pType = rFloat;
				tokenPointerPair.arraySize = numUsed;
				tokenPointerPair.tokenFloats = pwidth;
				tokenPointerPair.isArray = true;
				tokenPointerPair.isUArray = false;
				tokenPointerPair.isNurbs = false;
				tokenPointerPair.dType = rVertex;
				tokenPointerArray.push_back( tokenPointerPair );
			} else {
				float floatValue;
				tokenPointerPair.arraySize = 0;
				tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 1 );
				fPlug.getValue( floatValue );
				tokenPointerPair.tokenFloats[0]  = floatValue ;
				tokenPointerPair.isArray = false;
				tokenPointerPair.isUArray = false;
				tokenPointerPair.dType = rConstant;
				tokenPointerArray.push_back( tokenPointerPair );
			}
		}
	}

	if ( pointAttributesFound.length() > 0 ) {
		for ( i = 0; i < pointAttributesFound.length(); i++ ) {
			rTokenPointer tokenPointerPair;
			MString cutString = pointAttributesFound[i].substring(5, pointAttributesFound[i].length());
			sprintf( tokenPointerPair.tokenName, cutString.asChar() ) ;
			MPlug pPlug = nodeFn.findPlug( pointAttributesFound[i] );
			MObject plugObj;
			status = pPlug.getValue( plugObj );
			tokenPointerPair.pType = rPoint;
			tokenPointerPair.isNurbs = false;
			tokenPointerPair.isUArray = false;
			if ( plugObj.apiType() == MFn::kPointArrayData ) 
			{
				MFnVectorArrayData  fnPointArrayData( plugObj );
				nparticles = fnPointArrayData.length();
				particles   = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( nparticles * 3 ) );
				
				RtFloat* partPtr = particles;
				int numUsed = 0;
				unsigned k;
				for ( k = 0; k < nparticles; k++ ) {
					/* check to make sure the particle hasn't been born throughout the number of samples. */
					double birthTime = birthArray[k] * 24.0;
					double sampleTime = (double)sampleTimes[0];
					double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
					double lastSampleTime;
					if ( doMotion || doDef ) {
						lastSampleTime = (double)sampleTimes[motionSamples - 1];
					} else {
						lastSampleTime = (double)sampleTimes[0];
					}
					
					if ( ( deathTime >= lastSampleTime ) && ( birthTime <= sampleTime) ) {
						/* add the particle to the list. */
						*partPtr++ = (RtFloat)fnPointArrayData[k].x;
						*partPtr++ = (RtFloat)fnPointArrayData[k].y;
						*partPtr++ = (RtFloat)fnPointArrayData[k].z;
						numUsed++;
					} 
				}
				
				tokenPointerPair.arraySize = numUsed;
				tokenPointerPair.tokenFloats = particles;
				tokenPointerPair.isArray = true;
				tokenPointerPair.dType = rVertex;
				tokenPointerArray.push_back( tokenPointerPair );
			} else {
				tokenPointerPair.arraySize = 0;
				tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3 );
				pPlug.child(0).getValue( tokenPointerPair.tokenFloats[0] );
				pPlug.child(1).getValue( tokenPointerPair.tokenFloats[1] );
				pPlug.child(2).getValue( tokenPointerPair.tokenFloats[2] );
				tokenPointerPair.isArray = false;
				tokenPointerPair.dType = rConstant;
				tokenPointerArray.push_back( tokenPointerPair );
			}
		}
	}

	if ( vectorAttributesFound.length() > 0 ) {
		for ( i = 0; i < vectorAttributesFound.length(); i++ ) {
			rTokenPointer tokenPointerPair;
			MString cutString = vectorAttributesFound[i].substring(5, vectorAttributesFound[i].length());
			sprintf( tokenPointerPair.tokenName, cutString.asChar() ) ;
			MPlug vPlug = nodeFn.findPlug( vectorAttributesFound[i] );
			MObject plugObj;
			status = vPlug.getValue( plugObj );
			tokenPointerPair.pType = rVector;
			tokenPointerPair.isNurbs = false;
			tokenPointerPair.isUArray = false;
			if ( plugObj.apiType() == MFn::kVectorArrayData ) 
			{
				MFnVectorArrayData  fnVectorArrayData( plugObj );
				MVectorArray vectorArrayData = fnVectorArrayData.array( &status );
				int ncolor = vectorArrayData.length();
				pcolor   = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( ncolor * 3 ) );
				
				RtFloat* partCPtr = pcolor;
				unsigned k, numUsed = 0;
				for ( k = 0; k < ncolor; k++ ) {
					/* check to make sure the particle hasn't been born throughout the number of samples. */
					double birthTime = birthArray[k] * 24.0;
					double sampleTime = (double)sampleTimes[0];
					double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
					double lastSampleTime;
					if ( doMotion || doDef ) {
						lastSampleTime = (double)sampleTimes[motionSamples - 1];
					} else {
						lastSampleTime = (double)sampleTimes[0];
					}
					
					if ( ( deathTime >= lastSampleTime ) && ( birthTime <= sampleTime) ) {
						*partCPtr++ = (RtFloat)vectorArrayData[k].x;
						*partCPtr++ = (RtFloat)vectorArrayData[k].y;
						*partCPtr++ = (RtFloat)vectorArrayData[k].z;
					}
					numUsed++;
				}
				
				tokenPointerPair.arraySize = numUsed;
				tokenPointerPair.tokenFloats = pcolor;
				tokenPointerPair.isArray = true;
				tokenPointerPair.dType = rVertex;
				tokenPointerArray.push_back( tokenPointerPair );
			} else {
				tokenPointerPair.arraySize = 0;
				tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3 );
				vPlug.child(0).getValue( tokenPointerPair.tokenFloats[0] );
				vPlug.child(1).getValue( tokenPointerPair.tokenFloats[1] );
				vPlug.child(2).getValue( tokenPointerPair.tokenFloats[2] );
				tokenPointerPair.isArray = false;
				tokenPointerPair.dType = rConstant;
				tokenPointerArray.push_back( tokenPointerPair );
			}
		}
	}

	if ( colourAttributesFound.length() > 0 ) {
		for ( i = 0; i < colourAttributesFound.length(); i++ ) {
			rTokenPointer tokenPointerPair;
			MString cutString = colourAttributesFound[i].substring(5, colourAttributesFound[i].length());
			sprintf( tokenPointerPair.tokenName, cutString.asChar() ) ;
			MPlug vPlug = nodeFn.findPlug( colourAttributesFound[i] );
			MObject plugObj;
			status = vPlug.getValue( plugObj );
			tokenPointerPair.pType = rColor;
			tokenPointerPair.isNurbs = false;
			tokenPointerPair.isUArray = false;
			if ( plugObj.apiType() == MFn::kVectorArrayData ) 
			{
				MFnVectorArrayData  fnVectorArrayData( plugObj );
				MVectorArray vectorArrayData = fnVectorArrayData.array( &status );
				int ncolor = vectorArrayData.length();
				pcolor   = (RtFloat*)lmalloc( sizeof( RtFloat ) * ( ncolor * 3 ) );
				
				RtFloat* partCPtr = pcolor;
				unsigned k, numUsed = 0;
				for ( k = 0; k < ncolor; k++ ) {
					/* check to make sure the particle hasn't been born throughout the number of samples. */
					double birthTime = birthArray[k] * 24.0;
					double sampleTime = (double)sampleTimes[0];
					double deathTime = ( birthArray[k] + lifePPArray[k] ) * 24.0;
					double lastSampleTime;
					if ( doMotion || doDef ) {
						lastSampleTime = (double)sampleTimes[motionSamples - 1];
					} else {
						lastSampleTime = (double)sampleTimes[0];
					}
					
					if ( ( deathTime >= lastSampleTime ) && ( birthTime <= sampleTime) ) {
						*partCPtr++ = (RtFloat)vectorArrayData[k].x;
						*partCPtr++ = (RtFloat)vectorArrayData[k].y;
						*partCPtr++ = (RtFloat)vectorArrayData[k].z;
						numUsed++;
					}
				}
				
				tokenPointerPair.arraySize = numUsed;
				tokenPointerPair.tokenFloats = pcolor;
				tokenPointerPair.isArray = true;
				tokenPointerPair.dType = rVertex;
				tokenPointerArray.push_back( tokenPointerPair );
			} else {
				tokenPointerPair.arraySize = 0;
				tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3 );
				vPlug.child(0).getValue( tokenPointerPair.tokenFloats[0] );
				vPlug.child(1).getValue( tokenPointerPair.tokenFloats[1] );
				vPlug.child(2).getValue( tokenPointerPair.tokenFloats[2] );
				tokenPointerPair.isArray = false;
				tokenPointerPair.dType = rConstant;
				tokenPointerArray.push_back( tokenPointerPair );
			}
		}
	}
}
