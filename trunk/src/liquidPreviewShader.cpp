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

/* liquid command to export a shader ball with the selected shader */

// Standard Headers
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <iostream.h>
#include <sys/types.h>

// Renderman Headers
extern "C" {
#include <ri.h>
#ifdef PRMAN
#include <slo.h>
#endif
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
#include <maya/MFn.h>
#include <maya/MString.h>
#include <maya/MPxCommand.h>
#include <maya/MCommandResult.h>
#include <maya/MStringArray.h>
#include <maya/MArgList.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MPlug.h>
#include <maya/MDoubleArray.h>

#include <liqShader.h>
#include <liquidPreviewShader.h>
#include <liquidMemory.h>
#include <liquid.h>
#include <liquidGlobalHelpers.h>

void* liquidPreviewShader::creator()
//
//  Description:
//      Create a new instance of the translator
//
{
    return new liquidPreviewShader();
}

liquidPreviewShader::~liquidPreviewShader()
//
//  Description:
//      Class destructor
//
{
} 

MStatus	liquidPreviewShader::doIt( const MArgList& args )
{
#if defined( PRMAN ) || defined( ENTROPY )
    MStatus status;
    int i;
    MString	shaderName;
    bool useIt = true;

    for ( i = 0; i < args.length(); i++ ) {
      if ( MString( "-sphere" ) == args.asString( i, &status ) )  {
	    } else if ( MString( "-box" ) == args.asString( i, &status ) )  {
	    } else if ( MString( "-plane" ) == args.asString( i, &status ) )  {
	    } else if ( MString( "-noIt" ) == args.asString( i, &status ) )  {
		    useIt = false;
	    } else if ( MString( "-shader" ) == args.asString( i, &status ) )  {
		    i++;
		    shaderName = args.asString( i, &status );
	    }
    }

#ifdef _WIN32
    char *systemTempDirectory;
    systemTempDirectory = getenv("TEMP");
    MString tempRibName( systemTempDirectory );
    tempRibName += "/temp.rib";
    free( systemTempDirectory );
    systemTempDirectory = (char *)malloc( sizeof( char ) * 256 );
    strcpy( systemTempDirectory, tempRibName.asChar() );
    RiBegin( systemTempDirectory );
#else
#ifdef PRMAN
    FILE *fp = popen("render", "w");
    if( !fp )
    {
    	return MS::kFailure;
    }
    RtInt fd = fileno(fp); 
    RiOption("rib", "pipe", (RtPointer)&fd, RI_NULL); 
    RiBegin( RI_NULL );
#endif // PRMAN
#ifdef ENTROPY
    // Hmmmmmm Should be "|entropy" but does not seem to work
    // This output to stdout
    RiBegin( NULL );
#endif // ENTROPY
#endif // _WIN32
    RiFrameBegin( 1 );
    RiFormat( 100, 100, 1 );
    if ( useIt ) {
    	RiDisplay( "liquidpreview", "it", "rgba", RI_NULL );
    } else {
    	RiDisplay( "liquidpreview", "framebuffer", "rgba", RI_NULL );
    }
    RtFloat fov = 38;
    RiProjection( "perspective", "fov", &fov, RI_NULL );
    RiWorldBegin();
    RtLightHandle ambientLightH, directionalLightH;
    RtFloat intensity;
    intensity = float(0.2);
    ambientLightH = RiLightSource( "ambientlight", "intensity", &intensity, RI_NULL );
    intensity = 1.0;
    RtPoint from;
    from[0] = -2.0; from[1] = -1.0; from[2] = 1.0;
    RtPoint to;
    to[0] = 0.0; to[1] = 0.0; to[2] = 0.0;
    directionalLightH = RiLightSource( "distantlight", "intensity", &intensity, "from", &from, "to", &to, RI_NULL );

    RiAttributeBegin();
    MSelectionList shaderNameList;
    MObject	shaderObj;
    shaderNameList.add( shaderName );
    shaderNameList.getDependNode( 0, shaderObj );
    MFnDependencyNode assignedShader( shaderObj );
    liqShader currentShader( shaderObj );
    RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * currentShader.numTPV );
    RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * currentShader.numTPV );

    assignTokenArrays( currentShader.numTPV, currentShader.tokenPointerArray, tokenArray, pointerArray );

    RiTranslate( 0, 0, 1.5 );	
	
    double displacementBounds = 0.0;
    assignedShader.findPlug( "displacementBound", &status ).getValue( displacementBounds );
    if ( displacementBounds > 0.0 ) {
    	RiAttribute( "bound", "displacement", &displacementBounds, RI_NULL );
    }
	
    if ( currentShader.shader_type == SHADER_TYPE_SURFACE ) {
    	RiColor( currentShader.rmColor );
  	RiOpacity( currentShader.rmOpacity );
    	RiSurfaceV ( const_cast<char *>( currentShader.file.c_str() ), currentShader.numTPV, tokenArray, pointerArray );
    } else if ( currentShader.shader_type == SHADER_TYPE_DISPLACEMENT ) {
    	RiDisplacementV ( const_cast<char *>( currentShader.file.c_str() ), currentShader.numTPV, tokenArray, pointerArray );
    }

    RiSphere( 0.5, 0.0, 0.5, 360.0, RI_NULL );
	
    RiAttributeEnd();
    RiWorldEnd();
    RiFrameEnd();
    RiEnd();
#ifdef _WIN32
    // Hmmmmmmm should do something here for entropy 
	_spawnlp(_P_DETACH, "prman", "prman", tempRibName.asChar(), NULL);
	free( systemTempDirectory );
#endif
	return MS::kSuccess; 
#else // PRMAN || ENTROPY
    return MS::kFailure; 
#endif
};



