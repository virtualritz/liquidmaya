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
#include <liqGlobalHelpers.h>
#include <liqRibData.h>
#include <liqRibParticleData.h>
#include <liqMemory.h>

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

    int m_particleNum; // index into the per particle attribute arrays
    int m_particleId; // global particle id
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
    MPlug  posPlug = fnNode.findPlug( "position", &status );
    MObject  posObject;

    posPlug.getValue( posObject );

    MFnVectorArrayData posArray( posObject, &status );
    status.clear();

    m_numParticles = posArray.length();

    // we need to sort the particles by id. The position array doesn't keep
    // things in particle id order so things can get pretty screwed up when
    // we're motion blurring
    MPlug  idPlug = fnNode.findPlug( "id", &status );
    MObject  idObject;

    idPlug.getValue( idObject );

    MFnDoubleArrayData idArray( idObject, &status );
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

    liq_particleInfoVector particlesForSorting;;

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

    // Check for a multi-count parameter (set if using multi-point
    // or multi-streak particles). Default to 1, otherwise.
    //
    MPlug multiCountPlug = fnNode.findPlug( "multiCount", &status );
    if ( status == MS::kSuccess &&
         (particleType == MPTMultiPoint || particleType == MPTMultiStreak ) )
    {
        multiCountPlug.getValue( m_multiCount );
    }
    else
    {
        m_multiCount = 1;
    }

    // Check for a multi-count radius parameter (again, only for
    // multi-point or multi-streak particles).
    //
    float multiRadius = 0;
    MPlug multiRadiusPlug = fnNode.findPlug( "multiRadius", &status );
    if ( status == MS::kSuccess &&
         (particleType == MPTMultiPoint || particleType == MPTMultiStreak ) )
    {
        multiRadiusPlug.getValue( multiRadius );
    }

    // Get the velocity information (used for streak, multi-streak).
    //
    MPlug velPlug = fnNode.findPlug( "velocity", &status );
    MObject velObject;
    velPlug.getValue( velObject );
    MFnVectorArrayData velArray( velObject, &status );

    // Check for the tail size parameter (only for streak, multi-streak).
    //
    float tailSize = 0;
    MPlug tailSizePlug = fnNode.findPlug( "tailSize", &status );
    if ( status == MS::kSuccess &&
         (particleType == MPTStreak || particleType == MPTMultiStreak ) )
    {
        tailSizePlug.getValue( tailSize );
    }

    // Check for the tail fade parameter (only for streak, multi-streak).
    //
    float tailFade = 1;
    MPlug tailFadePlug = fnNode.findPlug( "tailFade", &status );
    if ( status == MS::kSuccess &&
         (particleType == MPTStreak || particleType == MPTMultiStreak ) )
    {
        tailFadePlug.getValue( tailFade );
    }
    
    // then we get the particle radius data
    if ( debugMode ) { printf("-> Reading Particle Radius\n"); }

    MDoubleArray radiusArray;
    MPlug  radiusPPPlug = fnNode.findPlug( "radiusPP", &status );
    bool  haveRadiusArray = false;
    float  radius = 1.0;

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
            else
            {
                // Try lineWidth (used by streak and multi-streak).
                //
                MPlug lineWidthPlug = fnNode.findPlug( "lineWidth", &status );
                if ( status == MS::kSuccess )
                {
                    lineWidthPlug.getValue( radius );
                }
            }
        }
    }

    status.clear();

    // then we get the particle color info
    if ( debugMode ) { printf("-> Reading Particle Color\n"); }

    MVectorArray rgbArray;
    MPlug  rgbPPPlug = fnNode.findPlug( "rgbPP", &status );
    bool  haveRgbArray = false;

    if ( status == MS::kSuccess ) {
        MObject rgbPPObject;

        rgbPPPlug.getValue( rgbPPObject );

        MFnVectorArrayData rgbArrayData( rgbPPObject, &status );

        rgbArray = rgbArrayData.array();

        haveRgbArray = true;
    }

    status.clear();

    // Then we get the per-particle opacity info
    //
    if ( debugMode ) { printf("-> Reading Particle Opacity\n"); }

    MDoubleArray opacityArray;
    MPlug  opacityPPPlug = fnNode.findPlug( "opacityPP", &status );
    bool  haveOpacityArray = false;

    if ( status == MS::kSuccess ) {
        MObject opacityPPObject;
        opacityPPPlug.getValue( opacityPPObject );
        MFnDoubleArrayData opacityArrayData( opacityPPObject, &status );

        opacityArray = opacityArrayData.array();
        haveOpacityArray = true;
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

    case MPTMultiPoint:
    case MPTPoints:
    {
        liqTokenPointer Pparameter;

        Pparameter.set( "P", rPoint, false, true, false, m_numValidParticles*m_multiCount );

        Pparameter.setDetailType( rVertex );

        for ( unsigned part_num = 0;
              part_num < m_numValidParticles;
              part_num++ ) {
            // Seed the random number generator using the particle ID
            // (this ensures that the multi-points won't jump during animations,
            //  and won't jump when other particles die)
            //
            srand(particlesForSorting[part_num]->m_particleId);
            for (unsigned multiNum = 0; multiNum < m_multiCount; multiNum++)
            {
                float xDir=0, yDir=0, zDir=0, vLen=0, rad=0;

                // Only do the random placement if we're dealing with
                // multi-point or multi-streak particles.
                //
                if ( m_multiCount > 1 )
                {
                    do
                    {
                        xDir = rand() / (float)RAND_MAX - 0.5;
                        yDir = rand() / (float)RAND_MAX - 0.5;
                        zDir = rand() / (float)RAND_MAX - 0.5;
                        vLen = sqrt(pow(xDir, 2) + pow(yDir, 2) + pow(zDir, 2));
                    } while (vLen == 0.0);

                    xDir /= vLen;
                    yDir /= vLen;
                    zDir /= vLen;
                    rad = rand() / (float)RAND_MAX * multiRadius / 2.0;
                }

                Pparameter.setTokenFloat( part_num*m_multiCount + multiNum,
                                          posArray[m_validParticles[part_num]].x + rad*xDir,
                                          posArray[m_validParticles[part_num]].y + rad*yDir,
                                          posArray[m_validParticles[part_num]].z + rad*zDir );
            }
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
                                m_numValidParticles*m_multiCount );

            widthParameter.setDetailType( rVertex );

            for ( unsigned part_num = 0;
                  part_num < m_numValidParticles*m_multiCount;
                  part_num++ ) {

                widthParameter.setTokenFloat( part_num,
                                              radiusArray[m_validParticles[part_num/m_multiCount]]*2);
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

    case MPTMultiStreak:
    case MPTStreak:
    {
        // Streak particles have a head and a tail, so double the vertex count.
        //
        m_multiCount *= 2;

        liqTokenPointer Pparameter;
        Pparameter.set( "P", rPoint, false, true, false, m_numValidParticles*m_multiCount );

        Pparameter.setDetailType( rVertex );

        for ( unsigned part_num = 0;
              part_num < m_numValidParticles;
              part_num++ ) {
            // Seed the random number generator using the particle ID
            // (this ensures that the multi-points won't jump during animations,
            //  and won't jump when other particles die)
            //
            srand(particlesForSorting[part_num]->m_particleId);
            for (unsigned multiNum = 0; multiNum < m_multiCount; multiNum+=2)
            {
                float xDir=0, yDir=0, zDir=0, vLen=0, rad=0;

                // Only do the random placement if we're dealing with
                // multi-point or multi-streak particles.
                //
                if ( m_multiCount > 1 )
                {
                    do
                    {
                        xDir = rand() / (float)RAND_MAX - 0.5;
                        yDir = rand() / (float)RAND_MAX - 0.5;
                        zDir = rand() / (float)RAND_MAX - 0.5;
                        vLen = sqrt(pow(xDir, 2) + pow(yDir, 2) + pow(zDir, 2));
                    } while (vLen == 0.0);

                    xDir /= vLen;
                    yDir /= vLen;
                    zDir /= vLen;
                    rad = rand() / (float)RAND_MAX * multiRadius / 2.0;
                }

                extern double liqglo_FPS;

                // Tail (the formula below is a bit of a guess as to how Maya places the tail).
                //
                Pparameter.setTokenFloat( part_num*m_multiCount + multiNum,
                                          posArray[m_validParticles[part_num]].x + rad*xDir -
                                          velArray[m_validParticles[part_num]].x * tailSize / liqglo_FPS,
                                          posArray[m_validParticles[part_num]].y + rad*yDir -
                                          velArray[m_validParticles[part_num]].y * tailSize / liqglo_FPS,
                                          posArray[m_validParticles[part_num]].z + rad*zDir -
                                          velArray[m_validParticles[part_num]].z * tailSize / liqglo_FPS );

                // Head
                //
                Pparameter.setTokenFloat( part_num*m_multiCount + multiNum + 1,
                                          posArray[m_validParticles[part_num]].x + rad*xDir,
                                          posArray[m_validParticles[part_num]].y + rad*yDir,
                                          posArray[m_validParticles[part_num]].z + rad*zDir );
            }
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
                                m_numValidParticles*m_multiCount );
            widthParameter.setDetailType( rVertex );

            for ( unsigned part_num = 0;
                  part_num < m_numValidParticles*m_multiCount;
                  part_num++ ) {

                widthParameter.setTokenFloat( part_num,
                                              radiusArray[m_validParticles[part_num/m_multiCount]]*2);
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

    case MPTSpheres: {
        liqTokenPointer Pparameter;
        liqTokenPointer radiusParameter;
        
        Pparameter.set( "P", rPoint, false, true, false, m_numValidParticles );
        Pparameter.setDetailType( rVertex );
        
        radiusParameter.set("radius", rFloat, false, true, false, 
                            m_numValidParticles);
        radiusParameter.setDetailType( rVertex );
        
        for ( unsigned part_num = 0;
              part_num < m_numValidParticles;
              part_num++ ) 
        {
            
            Pparameter.setTokenFloat( part_num,
                                      posArray[m_validParticles[part_num]].x,
                                      posArray[m_validParticles[part_num]].y,
                                      posArray[m_validParticles[part_num]].z );
            if(haveRadiusArray)
            {
                radiusParameter.setTokenFloat(part_num,
                                              radiusArray[m_validParticles[part_num]]);
            }
            else
            {
                radiusParameter.setTokenFloat(part_num, radius);
            }
        }
     
        tokenPointerArray.push_back( Pparameter );
        tokenPointerArray.push_back(radiusParameter);
     
    }
    break;

    case MPTNumeric:
    case MPTSprites:
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

        CsParameter.set( "Cs", rColor, false, true, false, m_numValidParticles*m_multiCount );
        CsParameter.setDetailType( rVertex );

        for ( unsigned part_num = 0;
              part_num < m_numValidParticles*m_multiCount;
              part_num++ ) {

            // For most of our parameters, we only have values for each "chunk"
            // (where a "chunk" is all the particles in a multi block)
            //
            int part_chunk = part_num / m_multiCount;

            CsParameter.setTokenFloat( part_num,
                                       rgbArray[m_validParticles[part_chunk]].x,
                                       rgbArray[m_validParticles[part_chunk]].y,
                                       rgbArray[m_validParticles[part_chunk]].z );
        }

        tokenPointerArray.push_back( CsParameter );
    }


    // And we add the Os Parameter (if needed).
    //
    if (haveOpacityArray)
    {
        liqTokenPointer OsParameter;

        OsParameter.set( "Os", rColor, false, true, false, m_numValidParticles*m_multiCount );
        OsParameter.setDetailType( rVarying );

        for ( unsigned part_num = 0;
              part_num < m_numValidParticles*m_multiCount;
              part_num++ ) {

            // For most of our parameters, we only have values for each "chunk"
            // (where a "chunk" is all the particles in a multi block)
            //
            int part_chunk = part_num / m_multiCount;

            // Fade out the even particles (the tails) if streaks.
            //
            if ((particleType == MPTStreak || particleType == MPTMultiStreak) &&
                ((part_num & 0x01) == 0))
            {
                OsParameter.setTokenFloat( part_num,
                                           opacityArray[m_validParticles[part_chunk]] * tailFade,
                                           opacityArray[m_validParticles[part_chunk]] * tailFade,
                                           opacityArray[m_validParticles[part_chunk]] * tailFade);
            }
            else
            {
                OsParameter.setTokenFloat( part_num,
                                           opacityArray[m_validParticles[part_chunk]],
                                           opacityArray[m_validParticles[part_chunk]],
                                           opacityArray[m_validParticles[part_chunk]]);
            }
        }

        tokenPointerArray.push_back( OsParameter );
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

    unsigned numTokens = tokenPointerArray.size();
    RtToken* tokenArray = (RtToken *)alloca( sizeof(RtToken) * numTokens );
    RtPointer* pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * numTokens );
    
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

    case MPTMultiPoint:
    case MPTPoints: {
        assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );
        RiPointsV( m_numValidParticles*m_multiCount, numTokens, tokenArray, pointerArray );
    }
    break;

    case MPTMultiStreak:
    case MPTStreak: {
        RtInt *verts = new RtInt[m_numValidParticles*m_multiCount/2];
        for (int i = 0; i < m_numValidParticles*m_multiCount/2; i++)
        {
            verts[i] = 2;
        }
        assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );
        RiCurvesV("linear", m_numValidParticles*m_multiCount/2, verts, "nonperiodic", numTokens, tokenArray, pointerArray);
        delete [] verts;
    }
    break;

    case MPTSpheres: {
        int posAttr=-1,
            radAttr=-1,
            colAttr=-1,
            opacAttr=-1;

        assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );

        for ( int i = 0; i < tokenPointerArray.size(); i++ )
        {
            if ( strcmp(tokenArray[i], "P") == 0 )
            {
                posAttr = i;
            }
            else if ( strcmp(tokenArray[i], "radius") == 0 )
            {
                radAttr = i;
            }
            else if ( strcmp(tokenArray[i], "Cs") == 0 )
            {
                colAttr = i;
            }
            else if ( strcmp(tokenArray[i], "Os") == 0 )
            {
                opacAttr = i;
            }
        }

        for (int i = 0; i < m_numValidParticles; i++)
        {
            if ( colAttr != -1 )
            {
                RiColor( &((RtFloat*)pointerArray[colAttr])[i*3] );
            }
            if ( opacAttr != -1 )
            {
                RiOpacity( &((RtFloat*)pointerArray[opacAttr])[i*3] );
            }
            RiTransformBegin();
            RiTranslate(((RtFloat*)pointerArray[posAttr])[i*3+0],
                        ((RtFloat*)pointerArray[posAttr])[i*3+1],
                        ((RtFloat*)pointerArray[posAttr])[i*3+2]);

            RtFloat radius = ((RtFloat*)pointerArray[radAttr])[i];
            RiSphere(radius, -radius, radius, 360, RI_NULL);
            RiTransformEnd();
        }
    }
    break;

    case MPTSprites:
    case MPTNumeric:
    case MPTCloudy:
    case MPTTube:
        // do nothing. These are not supported
        break;
    }
}

bool liqRibParticleData::compare( const liqRibData & otherObj ) const
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
    MStringArray foundAttributes = FindAttributesByPrefix( "rmanF", nodeFn );
    MStatus  status;

    for ( int i = 0; i < foundAttributes.length(); i++ ) {
        liqTokenPointer floatParameter;
        MString  currAttribute = foundAttributes[i];
        MString  cutString = currAttribute.substring(5, currAttribute.length());

        MPlug  fPlug = nodeFn.findPlug( currAttribute );
        MObject  plugObj;

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
    MStringArray foundAttributes = FindAttributesByPrefix( "rmanP", nodeFn );
    MStatus  status;

    for ( int i = 0; i < foundAttributes.length(); i++ ) {
        liqTokenPointer pointParameter;
        MString  currAttribute = foundAttributes[i];
        MString  cutString = currAttribute.substring(5, currAttribute.length());

        MPlug  pPlug = nodeFn.findPlug( currAttribute );
        MObject  plugObj;

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
    MStringArray foundAttributes = FindAttributesByPrefix( "rmanV", nodeFn );
    MStatus  status;

    for ( int i = 0; i < foundAttributes.length(); i++ ) {
        liqTokenPointer vectorParameter;
        MString  currAttribute = foundAttributes[i];
        MString  cutString = currAttribute.substring(5, currAttribute.length());

        MPlug  vPlug = nodeFn.findPlug( currAttribute );
        MObject  plugObj;

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
    MStringArray foundAttributes = FindAttributesByPrefix( "rmanC", nodeFn );
    MStatus  status;

    for ( int i = 0; i < foundAttributes.length(); i++ ) {
        liqTokenPointer colorParameter;
        MString  currAttribute = foundAttributes[i];
        MString  cutString = currAttribute.substring(5, currAttribute.length());

        MPlug  cPlug = nodeFn.findPlug( currAttribute );
        MObject  plugObj;

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
