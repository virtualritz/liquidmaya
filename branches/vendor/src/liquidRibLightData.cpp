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
** Liquid Rib Light Data Source 
** ______________________________________________________________________
*/

// Standard Headers
#ifndef _WIN32
#include <libgen.h>
#endif
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
#include <maya/MFnDependencyNode.h>
#include <maya/MFnLight.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MDoubleArray.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MFnSpotLight.h>
#include <maya/MColor.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>

#include <liquid.h>
#include <liquidGlobalHelpers.h>
#include <liquidRibLightData.h>
#include <liquidMemory.h>
#include <liquidGetSloInfo.h>

extern int debugMode;
extern long lframe;
extern long outPadding;
extern MString animExt;
extern bool useFrameExt;
extern bool animation;
extern MString sceneName;
extern MString texDir;
extern MString projectDir;
extern bool isShadowPass;
extern bool	expandShaderArrays;
extern bool useBMRT;
extern bool doShadows;
extern bool shortShaderNames;
extern MStringArray DDimageName;

RibLightData::RibLightData( const MDagPath & light )
//
//  Description:
//      create a RIB compatible representation of a Maya light
//
:   handle( NULL ), assignedRManShader( NULL ) 	
{
	usingShadow = false;
	raytraced = false;
	excludeFromRib = false;
    MStatus status;
    if ( debugMode ) { printf("-> creating light\n"); }
    rmanLight = false;
    MFnDependencyNode lightDepNode( light.node() );
    MFnDependencyNode lightMainDepNode( light.node() );
    MFnLight    fnLight( light );
	
	status.clear();
	MPlug excludeFromRibPlug = fnLight.findPlug( "liquidExcludeFromRib", &status );
	if ( status == MS::kSuccess ) {
		excludeFromRibPlug.getValue( excludeFromRib );
	}
	
	status.clear();
	MPlug userShadowNamePlug = fnLight.findPlug( "liquidShadowName", &status );
	if ( status == MS::kSuccess ) {
		MString varVal;
		userShadowNamePlug.getValue( varVal );
		userShadowName = parseString( varVal );
	}
	
	
	
	// check to see if the light is using raytraced shadows
   	lightDepNode.findPlug( MString( "useRayTraceShadows" ) ).getValue( raytraced );
	
	name = fnLight.name();
	
	MPlug rmanLightPlug = lightDepNode.findPlug( MString( "liquidLightShaderNode" ), &status );
	if ( status == MS::kSuccess && rmanLightPlug.isConnected() ) {
		
		MString liquidShaderNodeName;
		MPlugArray rmanLightPlugs;
		rmanLightPlug.connectedTo( rmanLightPlugs, true, true );
		MObject liquidShaderNodeDep = rmanLightPlugs[0].node();
		
		lightDepNode.setObject( liquidShaderNodeDep );
		MPlug rmanShaderPlug = lightDepNode.findPlug( "rmanShaderLong", &status );
		if ( status == MS::kSuccess ) {
			MString rmShaderStr;
			rmanShaderPlug.getValue( rmShaderStr );
			assignedRManShader = (char *)lmalloc(rmShaderStr.substring( 0, rmShaderStr.length() - 5 ).length()+1);
			sprintf(assignedRManShader, rmShaderStr.substring( 0, rmShaderStr.length() - 5 ).asChar());
			if ( debugMode ) { printf("-> Using Renderman Shader %s. \n", assignedRManShader ) ;}
			
			liquidGetSloInfo shaderInfo;
			int success = shaderInfo.setShader( rmShaderStr );
			if ( !success ) {
				perror("Slo_SetShader");
				printf("Slo_SetShader(%s) failed in liquid output! \n", assignedRManShader);
				rmanLight = false;
			} else {
				int	  numArgs = shaderInfo.getNumParam();
				int	  i;
				
				rmanLight = true;
				for ( i = 0; i < numArgs; i++ )
				{
					rTokenPointer tokenPointerPair;
					
					/* checking to make sure no duplicate attributes end up in the light line */
					
					MString testAttr;
					testAttr = "rmanF"; testAttr += shaderInfo.getArgName( i );
					status.clear();
					lightMainDepNode.findPlug( testAttr, &status );
					if ( status == MS::kSuccess ) continue;
					testAttr = "rmanP"; testAttr += shaderInfo.getArgName( i );
					status.clear();
					lightMainDepNode.findPlug( testAttr, &status );
					if ( status == MS::kSuccess ) continue;
					testAttr = "rmanV"; testAttr += shaderInfo.getArgName( i );
					status.clear();
					lightMainDepNode.findPlug( testAttr, &status );
					if ( status == MS::kSuccess ) continue;
					testAttr = "rmanN"; testAttr += shaderInfo.getArgName( i );
					status.clear();
					lightMainDepNode.findPlug( testAttr, &status );
					if ( status == MS::kSuccess ) continue;
					testAttr = "rmanC"; testAttr += shaderInfo.getArgName( i );
					status.clear();
					lightMainDepNode.findPlug( testAttr, &status );
					if ( status == MS::kSuccess ) continue;
					testAttr = "rmanS"; testAttr += shaderInfo.getArgName( i );
					status.clear();
					lightMainDepNode.findPlug( testAttr, &status );
					if ( status == MS::kSuccess ) continue;
					
					SHADER_TYPE currentArgType = shaderInfo.getArgType( i );
					SHADER_DETAIL currentArgDetail = shaderInfo.getArgDetail( i );
					tokenPointerPair.isNurbs = false;
					switch (currentArgDetail) {
					case SHADER_DETAIL_UNIFORM: {
						tokenPointerPair.dType = rUniform;
						break;
												}
					case SHADER_DETAIL_VARYING: {
						tokenPointerPair.dType = rVarying;
						break; 
												}
					}	     
					switch (currentArgType) {
					case SLO_TYPE_STRING: {
						MPlug stringPlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
						if ( status == MS::kSuccess ) {
							MString stringPlugVal;
							stringPlug.getValue( stringPlugVal );
							MString stringDefault( shaderInfo.getArgStringDefault( i, 0 ) );
							if ( stringPlugVal != "" && stringPlugVal != stringDefault ) {
								MString parsingString = stringPlugVal;
								stringPlugVal = parseString( parsingString );
								parsingString = stringPlugVal;
								parsingString.toLowerCase();
								if ( parsingString.substring(0, 9) == "autoshadow"  ) {
									if ( doShadows ) {
										if ( userShadowName == MString( "" ) )
										{ 
											// build the shadow name
											shadowName = texDir;
											if ( ( DDimageName[0] == "" ) ) {
												shadowName += sceneName; 
											} else {
												char *mydot = ".";
												int pointIndex = DDimageName[0].index( *mydot );
												shadowName += DDimageName[0].substring(0, pointIndex-1).asChar();
											}
											shadowName += "_";
											shadowName += fnLight.name();
											shadowName += "SHD";
											if ( parsingString.length() == 12 ) {
												shadowName += "_";
												shadowName += parsingString.substring(10, 11).toUpperCase();
											}
											shadowName += ".";
											shadowName += (int)lframe;
											shadowName += ".tex";
										} else {
											shadowName = texDir;
											shadowName += userShadowName;
										}
										
										usingShadow = true;
										tokenPointerPair.tokenString = (char *)lmalloc( shadowName.length() + 10 );
										sprintf( tokenPointerPair.tokenString, shadowName.asChar() );
										sprintf( tokenPointerPair.tokenName , shaderInfo.getArgName( i ).asChar() );
										tokenPointerPair.pType = rString;
										tokenPointerPair.arraySize = 0;
										tokenPointerPair.isArray = false;
										tokenPointerPair.isUArray = false;
										tokenPointerArray.push_back( tokenPointerPair );
									}
								} else {
									if ( stringPlugVal != MString( "" ) ){
										tokenPointerPair.tokenString = (char *)lmalloc(stringPlugVal.length() + 10);
									} else {
										tokenPointerPair.tokenString = RI_NULL;
									}
									sprintf( tokenPointerPair.tokenString, stringPlugVal.asChar() ); 
									sprintf( tokenPointerPair.tokenName , shaderInfo.getArgName( i ).asChar() );
									tokenPointerPair.pType = rString;
									tokenPointerPair.arraySize = 0;
									tokenPointerPair.isArray = false;
									tokenPointerPair.isUArray = false;
									tokenPointerArray.push_back( tokenPointerPair );
								}
							}
						}
						break; }
					case SLO_TYPE_SCALAR: {
						MPlug floatPlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
						if ( status == MS::kSuccess ) {
							if ( shaderInfo.getArgArraySize( i ) > 0 ) {
								if ( expandShaderArrays ) {
									MObject plugObj;
									floatPlug.getValue( plugObj );
									MFnDoubleArrayData  fnDoubleArrayData( plugObj );
									MDoubleArray doubleArrayData = fnDoubleArrayData.array( &status );
									int k;
									for ( k = 0; k < shaderInfo.getArgArraySize( i ); k++ ) {
										sprintf( tokenPointerPair.tokenName , "%s%d", shaderInfo.getArgName( i ).asChar(), ( k + 1 ) );
										tokenPointerPair.pType = rFloat;
										tokenPointerPair.arraySize = 0;
										tokenPointerPair.isArray = false;
										tokenPointerPair.isUArray = false;
										float floatPlugVal = doubleArrayData[k];
										tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 1);
										tokenPointerPair.tokenFloats[0] = floatPlugVal;
										tokenPointerArray.push_back( tokenPointerPair );
									}
								} else {
									sprintf( tokenPointerPair.tokenName , shaderInfo.getArgName( i ).asChar() );
									tokenPointerPair.pType = rFloat;
									tokenPointerPair.arraySize = shaderInfo.getArgArraySize( i );
									tokenPointerPair.uArraySize = tokenPointerPair.arraySize;
									MObject plugObj;
									floatPlug.getValue( plugObj );
									MFnDoubleArrayData  fnDoubleArrayData( plugObj );
									MDoubleArray doubleArrayData = fnDoubleArrayData.array( &status );
									tokenPointerPair.isArray = false;
									tokenPointerPair.isUArray = true;
									tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * tokenPointerPair.arraySize );
									doubleArrayData.get( tokenPointerPair.tokenFloats );
									tokenPointerArray.push_back( tokenPointerPair );
								}
							} else {
								sprintf( tokenPointerPair.tokenName , shaderInfo.getArgName( i ).asChar() );
								tokenPointerPair.pType = rFloat;
								tokenPointerPair.arraySize = shaderInfo.getArgArraySize( i );
								tokenPointerPair.isArray = false;
								tokenPointerPair.isUArray = false;
								float floatPlugVal;
								floatPlug.getValue( floatPlugVal );
								tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 1);
								tokenPointerPair.tokenFloats[0] = floatPlugVal;
								tokenPointerArray.push_back( tokenPointerPair );
							}
						}
						break; }
					case SLO_TYPE_COLOR: {
						MPlug triplePlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
						tokenPointerPair.isArray = false;
						tokenPointerPair.isUArray = false;
						if ( status == MS::kSuccess ) {
							sprintf( tokenPointerPair.tokenName , shaderInfo.getArgName( i ).asChar() );
							tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3);
							triplePlug.child(0).getValue( tokenPointerPair.tokenFloats[0] );
							triplePlug.child(1).getValue( tokenPointerPair.tokenFloats[1] );
							triplePlug.child(2).getValue( tokenPointerPair.tokenFloats[2] );
							tokenPointerPair.pType = rColor;
							tokenPointerPair.arraySize = 0;
							tokenPointerArray.push_back( tokenPointerPair );
						}
						break; }
					case SLO_TYPE_POINT: {
						MPlug triplePlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
						tokenPointerPair.isArray = false;
						tokenPointerPair.isUArray = false;
						if ( status == MS::kSuccess ) {
							sprintf( tokenPointerPair.tokenName , shaderInfo.getArgName( i ).asChar() );							tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3);
							triplePlug.child(0).getValue( tokenPointerPair.tokenFloats[0] );
							triplePlug.child(1).getValue( tokenPointerPair.tokenFloats[1] );
							triplePlug.child(2).getValue( tokenPointerPair.tokenFloats[2] );
							tokenPointerPair.pType = rPoint;
							tokenPointerPair.arraySize = 0;
							tokenPointerArray.push_back( tokenPointerPair );
						}
						break; }
					case SLO_TYPE_VECTOR: {
						MPlug triplePlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
						tokenPointerPair.isArray = false;
						tokenPointerPair.isUArray = false;
						if ( status == MS::kSuccess ) {
							sprintf( tokenPointerPair.tokenName , shaderInfo.getArgName( i ).asChar() );							tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3);
							triplePlug.child(0).getValue( tokenPointerPair.tokenFloats[0] );
							triplePlug.child(1).getValue( tokenPointerPair.tokenFloats[1] );
							triplePlug.child(2).getValue( tokenPointerPair.tokenFloats[2] );
							tokenPointerPair.pType = rVector;
							tokenPointerPair.arraySize = 0;
							tokenPointerArray.push_back( tokenPointerPair );
						}
						break; }
					case SLO_TYPE_NORMAL: {
						MPlug triplePlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
						tokenPointerPair.isArray = false;
						tokenPointerPair.isUArray = false;
						if ( status == MS::kSuccess ) {
							sprintf( tokenPointerPair.tokenName , shaderInfo.getArgName( i ).asChar() );							tokenPointerPair.tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3);
							triplePlug.child(0).getValue( tokenPointerPair.tokenFloats[0] );
							triplePlug.child(1).getValue( tokenPointerPair.tokenFloats[1] );
							triplePlug.child(2).getValue( tokenPointerPair.tokenFloats[2] );
							tokenPointerPair.pType = rNormal;
							tokenPointerPair.arraySize = 0;
							tokenPointerArray.push_back( tokenPointerPair );
						}
						break; }
					case SLO_TYPE_MATRIX: {
						tokenPointerPair.isArray = false;
						tokenPointerPair.isUArray = false;
						printf( "WHAT IS THE MATRIX!\n" );
						break; }
					default:
						printf("Unknown\n");
						break; }
						}
				}
				shaderInfo.resetIt();
		}
    }
	addAdditionalSurfaceParameters( fnLight.object() );
	
    MColor      colorVal = fnLight.color();
    color[0]  = colorVal.r;
    color[1]  = colorVal.g;
    color[2]  = colorVal.b;
	
    intensity = fnLight.intensity();
	
    // get the light transform and flip it as maya's light work in the opposite direction
    // this seems to work correctly!
	
    RtMatrix rLightFix = {{ 1.0, 0.0, 0.0, 0.0},
	{ 0.0, 1.0, 0.0, 0.0},
	{ 0.0, 0.0, -1.0, 0.0},
	{ 0.0, 0.0, 0.0, 1.0} };
	
    MMatrix lightFix( rLightFix );
	
    MTransformationMatrix worldMatrix = light.inclusiveMatrix();
    //MMatrix worldMatrixM = worldMatrix.asMatrix();
	double scale[] = { 1.0, 1.0, -1.0 };
	worldMatrix.setScale( scale, MSpace::kTransform );
    //MMatrix worldMatrixM = lightFix * worldMatrix.asMatrix();
    MMatrix worldMatrixM = worldMatrix.asMatrix();
    worldMatrixM.get( transformationMatrix );
    
    if ( rmanLight ) {
		lightType	= MRLT_Rman;
    } else if ( light.hasFn(MFn::kAmbientLight)) {
		lightType = MRLT_Ambient;
    } else if ( light.hasFn(MFn::kDirectionalLight)) {
		lightType = MRLT_Distant;
    } else if ( light.hasFn(MFn::kPointLight)) {
		lightType = MRLT_Point;
    } else if ( light.hasFn(MFn::kSpotLight)) {
		MFnSpotLight fnSpotLight( light );
		lightType     = MRLT_Spot;
		coneAngle     = fnSpotLight.coneAngle() / 2.0;
		penumbraAngle = fnSpotLight.penumbraAngle() / 2.0;
		dropOff       = fnSpotLight.dropOff();
    } 
}

RibLightData::~RibLightData() 
{
	if ( debugMode ) { printf("-> killing light data.\n" ); }
	shadowName.clear();
	if ( assignedRManShader != NULL ) { lfree( assignedRManShader ); assignedRManShader = NULL; }
	if ( debugMode ) { printf("-> finished killing light data.\n" ); }
}

void RibLightData::write()
//
//  Description:
//      Write the RIB for this light
//
{
	if ( !excludeFromRib ) {
		if ( debugMode ) { printf("-> writing light\n"); }
		const char * namePtr = name.asChar();
		RiAttribute( "identifier", "name", &namePtr, RI_NULL );
		RiTransformBegin();
		RiConcatTransform( transformationMatrix );
		if ( isShadowPass ) {
			if ( usingShadow ) {
				char *sName = ( char * )alloca( sizeof( char ) * shadowName.length() );
				sprintf( sName, shadowName.asChar() );
				RiDeclare( "shadowname", "uniform string" );
				handle = RiLightSource( "/usr/home/canuck/Dev/Liquid/lib/shaders/shadowPassLight", "shadowname", &sName, RI_NULL );
			}					 			
		} else {
			
			// If we are using BMRT and the light is casting raytraced shadows then set the attribute
			if ( useBMRT && raytraced ) {
				RtString param = "on"; 
				RiAttribute( "light", "string shadows", &param, RI_NULL );
			}	else if ( useBMRT && !raytraced ) {
				RtString param = "off"; 
				RiAttribute( "light", "string shadows", &param, RI_NULL );
			}
			
			switch ( lightType ) {
			case MRLT_Ambient:
				handle = RiLightSource( "ambientlight", 
					"intensity", &intensity,
					"lightcolor", color,
					RI_NULL );
				break;
			case MRLT_Distant:
				handle = RiLightSource( "distantlight", 
					"intensity", &intensity,
					"lightcolor", color,
					RI_NULL );
				break;
			case MRLT_Point:
				handle = RiLightSource( "pointlight", 
					"intensity", &intensity,
					"lightcolor", color,
					RI_NULL );
				break;
			case MRLT_Spot:
				handle = RiLightSource( "spotlight", 
					"intensity", &intensity,
					"lightcolor", color,
					"coneangle", &coneAngle,
					"conedeltaangle", &penumbraAngle,
					"beamdistribution", &dropOff,
					RI_NULL );
				break;
			case MRLT_Rman: {
				RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * tokenPointerArray.size() );
				RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * tokenPointerArray.size() );
				
				assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );
				
#ifndef _WIN32
				if ( shortShaderNames ) {
					MString shaderString = basename( assignedRManShader );
					lfree( assignedRManShader ); assignedRManShader = NULL;
					assignedRManShader = (char *)lmalloc(shaderString.length()+1);
					sprintf(assignedRManShader, shaderString.asChar());
				}
#endif
				handle = RiLightSourceV( assignedRManShader, tokenPointerArray.size(), tokenArray, pointerArray );
				break;
							}
			}
		}
		RiTransformEnd();
	}
}

bool RibLightData::compare( const RibData & otherObj ) const
//
//  Description:
//      Light comparisons are not supported in this version.
//
{
	if ( debugMode ) { printf("-> comparing light\n"); }
	return true;  
}
ObjectType RibLightData::type() const
//
//  Description:
//      return the object type
//
{
	if ( debugMode ) { printf("-> returning light type\n"); }
	return MRT_Light;
}
RtLightHandle RibLightData::lightHandle() const
{
	return handle;   
}
