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
#include <malloc.h>
#include <sys/types.h>

// Renderman Headers
extern "C" {
	#include <ri.h>
	#include <slo.h>
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
	
	MStatus					status;
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
	sprintf( systemTempDirectory, tempRibName.asChar() );
	RiBegin( systemTempDirectory );
#else 
	FILE *fp = popen("render", "w"); 
  if (fp) { 
    RtInt fd = fileno(fp); 
    RiOption("rib", "pipe", (RtPointer)&fd, RI_NULL); 
	RiBegin( RI_NULL );
#endif
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
	
	MString rmShaderStr;
	MPlug rmanShaderNamePlug = assignedShader.findPlug( "rmanShaderLong" );
	rmanShaderNamePlug.getValue( rmShaderStr );
	char *assignedRManShader = (char *)alloca(rmShaderStr.length());
	sprintf(assignedRManShader, rmShaderStr.asChar());
	
  int 			numArgs;
	shaderStruct currentShader;
	currentShader.numTPV = 0;

	currentShader.name = assignedShader.name().asChar();
	MString shaderType;
	int err = -1;
	rmShaderStr += ".slo";
	err = Slo_SetShader( assignedRManShader );
	if (err != 0) {
	 	perror("Slo_SetShader");
	 	printf("Slo_SetShader(%s) failed in liquid output! \n", assignedRManShader);
	 	currentShader.rmColor[0] = 1.0;
	 	currentShader.rmColor[1] = 0.0;
	 	currentShader.rmColor[2] = 0.0;
	 	currentShader.name = "plastic";
		currentShader.numTPV = 0;
	} else { 
		shaderType = Slo_TypetoStr( Slo_GetType() );

		if ( MString("surface") == shaderType ) {
	    // Set RiColor and RiOpacity
  	  MPlug colorPlug = assignedShader.findPlug( "color" );

    	colorPlug.child(0).getValue( currentShader.rmColor[0] );
	    colorPlug.child(1).getValue( currentShader.rmColor[1] );
  	  colorPlug.child(2).getValue( currentShader.rmColor[2] );

    	MPlug opacityPlug = assignedShader.findPlug( "opacity" );
		
			double opacityVal;
  	  opacityPlug.getValue( opacityVal );
    	currentShader.rmOpacity[0] = RtFloat( opacityVal );
			currentShader.rmOpacity[1] = RtFloat( opacityVal );
			currentShader.rmOpacity[2] = RtFloat( opacityVal );
		}

	  // find the parameter details and declare them in the rib stream

    numArgs = Slo_GetNArgs();
    int i;
    for ( i = 1; i <= numArgs; i++ )
    {
			SLO_VISSYMDEF *arg;
  
  		arg = Slo_GetArgById( i );
			SLO_TYPE currentArgType = arg->svd_type;
			SLO_DETAIL currentArgDetail = arg->svd_detail;
			currentShader.tokenPointerArray[currentShader.numTPV].isNurbs = false;
			switch (currentArgDetail) {
				case SLO_DETAIL_UNIFORM: {
	      	currentShader.tokenPointerArray[currentShader.numTPV].dType = rUniform;
	       	break;
	     	}
				case SLO_DETAIL_VARYING: {
					currentShader.tokenPointerArray[currentShader.numTPV].dType = rVarying;
					break; 
	     	}
			}	     
  		switch (currentArgType) {
      	case SLO_TYPE_STRING: {
  				MPlug stringPlug = assignedShader.findPlug( arg->svd_name, &status );
  				if ( status == MS::kSuccess ) {
						MString stringPlugVal;
						stringPlug.getValue( stringPlugVal );
						if ( stringPlugVal != "" ) {
						  sprintf( currentShader.tokenPointerArray[currentShader.numTPV].tokenName , arg->svd_name );
							MString stringVal = parseString( stringPlugVal );
							currentShader.tokenPointerArray[ currentShader.numTPV ].tokenString = ( char * )lmalloc( sizeof( char ) * stringVal.length() );
						  sprintf( currentShader.tokenPointerArray[ currentShader.numTPV ].tokenString, stringVal.asChar() );
						  currentShader.tokenPointerArray[ currentShader.numTPV ].pType = rString;
						  currentShader.tokenPointerArray[ currentShader.numTPV ].arraySize = 0;
						  currentShader.tokenPointerArray[ currentShader.numTPV ].isArray = false;
						  currentShader.tokenPointerArray[ currentShader.numTPV ].isUArray = false;
						  currentShader.numTPV++;
						}
		      }
		      break; }
		    case SLO_TYPE_SCALAR: {
		      MPlug floatPlug = assignedShader.findPlug( arg->svd_name, &status );
		      if ( status == MS::kSuccess ) {
						sprintf( currentShader.tokenPointerArray[currentShader.numTPV].tokenName , arg->svd_name );
						currentShader.tokenPointerArray[currentShader.numTPV].pType = rFloat;
						currentShader.tokenPointerArray[currentShader.numTPV].arraySize = arg->svd_arraylen;
						if ( currentShader.tokenPointerArray[currentShader.numTPV].arraySize > 0 ) {
						  MObject plugObj;
						  floatPlug.getValue( plugObj );
			  			MFnDoubleArrayData  fnDoubleArrayData( plugObj );
					  	MDoubleArray doubleArrayData = fnDoubleArrayData.array( &status );
						  currentShader.tokenPointerArray[currentShader.numTPV].isArray = false;
				  		currentShader.tokenPointerArray[currentShader.numTPV].isUArray = true;
							currentShader.tokenPointerArray[currentShader.numTPV].uArraySize = currentShader.tokenPointerArray[currentShader.numTPV].arraySize;
						  currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * currentShader.tokenPointerArray[currentShader.numTPV].arraySize );
						  doubleArrayData.get( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats );
						} else {
						  currentShader.tokenPointerArray[currentShader.numTPV].isArray = false;
			  			currentShader.tokenPointerArray[currentShader.numTPV].isUArray = false;
						  float floatPlugVal;
						  floatPlug.getValue( floatPlugVal );
				  		currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 1);
						  currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[0] = floatPlugVal;
						}
						currentShader.numTPV++;
		      }
		      break; }
		    case SLO_TYPE_COLOR: {
		      MPlug triplePlug = assignedShader.findPlug( arg->svd_name, &status );
		      currentShader.tokenPointerArray[currentShader.numTPV].isArray = false;
		      currentShader.tokenPointerArray[currentShader.numTPV].isUArray = false;
		      if ( status == MS::kSuccess ) {
						sprintf( currentShader.tokenPointerArray[currentShader.numTPV].tokenName , arg->svd_name );
						currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3);
						triplePlug.child(0).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[0] );
						triplePlug.child(1).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[1] );
						triplePlug.child(2).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[2] );
						currentShader.tokenPointerArray[currentShader.numTPV].pType = rColor;
						currentShader.tokenPointerArray[currentShader.numTPV].arraySize = 0;
						currentShader.numTPV++;
		      }
		      break; }
		    case SLO_TYPE_POINT: {
		      MPlug triplePlug = assignedShader.findPlug( arg->svd_name, &status );
		      currentShader.tokenPointerArray[currentShader.numTPV].isArray = false;
		      currentShader.tokenPointerArray[currentShader.numTPV].isUArray = false;
		      if ( status == MS::kSuccess ) { 
						sprintf( currentShader.tokenPointerArray[currentShader.numTPV].tokenName , arg->svd_name );
						currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3);
						triplePlug.child(0).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[0] );
						triplePlug.child(1).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[1] );
						triplePlug.child(2).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[2] );
						currentShader.tokenPointerArray[currentShader.numTPV].pType = rPoint;
						currentShader.tokenPointerArray[currentShader.numTPV].arraySize = 0;
						currentShader.numTPV++;
		      }
		      break; }
		    case SLO_TYPE_VECTOR: {
		      currentShader.tokenPointerArray[currentShader.numTPV].isArray = false;
		      currentShader.tokenPointerArray[currentShader.numTPV].isUArray = false;
		      MPlug triplePlug = assignedShader.findPlug( arg->svd_name, &status );
		      if ( status == MS::kSuccess ) {
						sprintf( currentShader.tokenPointerArray[currentShader.numTPV].tokenName , arg->svd_name );
						currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3);
						triplePlug.child(0).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[0] );
						triplePlug.child(1).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[1] );
						triplePlug.child(2).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[2] );
						currentShader.tokenPointerArray[currentShader.numTPV].pType = rVector;
						currentShader.tokenPointerArray[currentShader.numTPV].arraySize = 0;
						currentShader.numTPV++;
		      }
		      break; }
		    case SLO_TYPE_NORMAL: {
		      currentShader.tokenPointerArray[currentShader.numTPV].isArray = false;
		      currentShader.tokenPointerArray[currentShader.numTPV].isUArray = false;
		      MPlug triplePlug = assignedShader.findPlug( arg->svd_name, &status );
		      if ( status == MS::kSuccess ) {
						sprintf( currentShader.tokenPointerArray[currentShader.numTPV].tokenName , arg->svd_name );
						currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3);
						triplePlug.child(0).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[0] );
						triplePlug.child(1).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[1] );
						triplePlug.child(2).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[2] );
						currentShader.tokenPointerArray[currentShader.numTPV].pType = rNormal;
						currentShader.tokenPointerArray[currentShader.numTPV].arraySize = 0;
						currentShader.numTPV++;
		      }
		      break; }
		    case SLO_TYPE_MATRIX: {
		      currentShader.tokenPointerArray[currentShader.numTPV].isArray = false;
		      currentShader.tokenPointerArray[currentShader.numTPV].isUArray = false;
		      printf( "WHAT IS THE MATRIX!\n" );
		      break; }
		    default:
		      printf("Unknown\n");
		      break;
				}
		}
		Slo_EndShader();
	} 
			
  RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * currentShader.numTPV );
	RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * currentShader.numTPV );

	assignTokenArrays( currentShader.numTPV, currentShader.tokenPointerArray, tokenArray, pointerArray );

	RiTranslate( 0, 0, 1.5 );	
	
	double displacementBounds = 0.0;
	assignedShader.findPlug( "displacementBound", &status ).getValue( displacementBounds );
	if ( displacementBounds > 0.0 ) {
		RiAttribute( "bound", "displacement", &displacementBounds, RI_NULL );
	}
	
	if ( MString("surface") == shaderType ) {
		RiColor( currentShader.rmColor );
  	RiOpacity( currentShader.rmOpacity );
	  RiSurfaceV ( assignedRManShader, currentShader.numTPV, tokenArray, pointerArray );
	} else if ( MString("displacement") == shaderType ) {
    RiDisplacementV ( assignedRManShader, currentShader.numTPV, tokenArray, pointerArray );
	}

	RiSphere( 0.5, 0.0, 0.5, 360.0, RI_NULL );
	
	RiAttributeEnd();
	RiWorldEnd();
	RiFrameEnd();
	RiEnd();
#ifdef _WIN32
	_spawnlp(_P_DETACH, "prman", "prman", tempRibName.asChar(), NULL);
	free( systemTempDirectory );
#else
  } 
#endif
		

	return MS::kSuccess; 
};



