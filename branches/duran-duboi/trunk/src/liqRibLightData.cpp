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
** Liquid Rib Light Data Source
** ______________________________________________________________________
*/

// Renderman headers
extern "C" {
#include <ri.h>
}

// Maya headers
#include <maya/MFnDependencyNode.h>
#include <maya/MFnLight.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MDoubleArray.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MFnSpotLight.h>
#include <maya/MFnAreaLight.h>
#include <maya/MColor.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>

// Liquid headers
#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liqRibLightData.h>
#include <liqGetSloInfo.h>
#include <liqRenderer.h>
#include <liqShaderFactory.h>

// Standard/Boost headers
#include <boost/scoped_array.hpp>

extern int debugMode;

extern long         liqglo_lframe;
extern MString      liqglo_sceneName;
extern MString      liqglo_textureDir;
extern bool         liqglo_isShadowPass;
//extern bool         liqglo_expandShaderArrays;
//extern bool         liqglo_useBMRT;
extern bool         liqglo_doShadows;
extern bool         liqglo_shapeOnlyInShadowNames;
extern bool         liqglo_shortShaderNames;
//extern MStringArray liqglo_DDimageName;
extern bool         liqglo_doExtensionPadding;
extern liquidlong   liqglo_outPadding;
extern MString      liqglo_projectDir;
extern bool         liqglo_relativeFileNames;

extern liqRenderer liquidRenderer;

using namespace std;

/** Create a RIB compatible representation of a Maya light.
 */
liqRibLightData::liqRibLightData( const MDagPath & light )
{
	// Init
	lightType = MRLT_Unknown;
	color[0] = 0;
	color[1] = 0;
	color[2] = 0;
	decay = 0;
	intensity = 0;
	coneAngle = 0;
	penumbraAngle = 0;
	dropOff = 0;
	shadowBlur = 0;
	// spot lights
	barnDoors = 0;
	leftBarnDoor = 0;
	rightBarnDoor = 0;
	topBarnDoor = 0;
	bottomBarnDoor = 0;
	decayRegions = 0;
	startDistance1 = 0;
	endDistance1 = 0;
	startDistance2 = 0;
	endDistance2 = 0;
	startDistance3 = 0;
	endDistance3 = 0;
	startDistanceIntensity1 = 0;
	endDistanceIntensity1 = 0;
	startDistanceIntensity2 = 0;
	endDistanceIntensity2 = 0;
	startDistanceIntensity3 = 0;
	endDistanceIntensity3 = 0;
	// Area Lights
	lightMap = "";
	lightMapSaturation = 0;
	// General
	nonDiffuse = 0;
	nonSpecular = 0;
	RtMatrix transformationMatrixTmp = {{1, 0, 0, 0},  {0, 1, 0, 0},  {0, 0, 1, 0},  {0, 0, 0, 1}};
	bcopy(transformationMatrixTmp, transformationMatrix, sizeof(RtMatrix));
	handle = NULL;
	usingShadow = false;
	deepShadows = 0;
	rayTraced = false;
	raySamples = 16;
	shadowRadius = 0;
	excludeFromRib = false;
	bothSidesEmit = 0;
	userShadowName = "";
	lightName = "";
	shadowType = stStandart;
	shadowHiderType = shMin;
	everyFrame = true;
	renderAtFrame = 0;
	geometrySet = "";
	shadowName = "";
	shadowNamePx = "";
	shadowNameNx = "";
	shadowNamePy = "";
	shadowNameNy = "";
	shadowNamePz = "";
	shadowNameNz = "";
	shadowBias = 0;
	shadowFilterSize = 0;
	shadowSamples = 0;
	shadowColor[0] = 0;
	shadowColor[1] = 0;
	shadowColor[2] = 0;
	lightCategory = "NULL";
	lightID = 0;
	hitmode = 0;
	rmanShader = NULL;

	MStatus status;
	LIQDEBUGPRINTF( "-> creating light\n" );
	rmanLight = false;
	MFnDependencyNode lightDepNode( light.node() );
	MFnDependencyNode lightMainDepNode( light.node() );
	MFnLight fnLight( light );

	// philippe : why this liquidExcludeFromRib attr ? Shouldn't LiqInvisible do the trick ?
	// not in the mel gui either.
	status.clear();
	MPlug excludeFromRibPlug = fnLight.findPlug( "liquidExcludeFromRib", &status );
	if ( MS::kSuccess == status )
	{
		excludeFromRibPlug.getValue( excludeFromRib );
	}

	// check if the light should be in the shadow pass - deep shadows only.
	//status.clear();
	//MPlug outputInShadowPlug = fnLight.findPlug( "outputInShadow", &status );
	//if ( MS::kSuccess == status ) {
	//  outputInShadowPlug.getValue( outputLightInShadow );
	//}

	status.clear();
	MPlug userShadowNamePlug( fnLight.findPlug( "liquidShadowName", &status ) );
	if ( MS::kSuccess == status )
	{
		MString varVal;
		userShadowNamePlug.getValue( varVal );
		userShadowName = parseString( varVal, false );
	}

	// check to see if the light is using raytraced shadows
	if( liquidRenderer.supports_RAYTRACE )
	{
		rayTraced = fnLight.useRayTraceShadows();
		if( rayTraced )
		{
			usingShadow = true;
			int raysamples( 1 );
			lightDepNode.findPlug( MString( "shadowRays" ) ).getValue( raysamples );
			raySamples = raysamples;
		}
	}
	lightName = fnLight.name();

	MPlug rmanLightPlug = lightDepNode.findPlug( MString( "liquidLightShaderNode" ), &status );
#if 1
	if ( MS::kSuccess == status && rmanLightPlug.isConnected() )
	{
		MPlugArray rmanLightPlugs;
		rmanLightPlug.connectedTo( rmanLightPlugs, true, true );
		MObject liquidShaderNodeDep = rmanLightPlugs[0].node();
		liqGenericShader &lightShader = liqShaderFactory::instance().getShader(liquidShaderNodeDep);
		if( lightShader.hasErrors )
		{
			liquidMessage( "[liqRibLightData] Reading shader '" + std::string( assignedRManShader.asChar() ) + "' failed", messageError );
			rmanLight = false;
		}
		else
		{
			rmanLight = true;
			rmanShader = &lightShader;
		}
	}
	else
	{
		if( !rayTraced )
		{
			fnLight.findPlug( "useDepthMapShadows" ).getValue( usingShadow );
			if ( usingShadow )
			{
				MPlug deepShadowsPlug( lightDepNode.findPlug( "deepShadows", &status ) );
				if ( MS::kSuccess == status )
				{
					deepShadowsPlug.getValue( deepShadows );
					if ( deepShadows )
						shadowType = stDeep;
				}
				MPlug paramPlug = lightDepNode.findPlug( "everyFrame", &status );
				if ( MS::kSuccess == status )
				{
					paramPlug.getValue( everyFrame );
				}
				paramPlug = lightDepNode.findPlug( "renderAtFrame", &status );
				if ( MS::kSuccess == status )
				{
					float tmp;
					paramPlug.getValue( tmp );
					renderAtFrame = ( int )tmp;
				}
				paramPlug = lightDepNode.findPlug( "geometrySet", &status );
				if ( MS::kSuccess == status )
				{
					paramPlug.getValue( geometrySet );
				}
			}
		}
	}
#else
	rmanLight = false;
#endif

	addAdditionalSurfaceParameters( fnLight.object() );

	MColor colorVal = fnLight.color();
	color[ 0 ]  = colorVal.r;
	color[ 1 ]  = colorVal.g;
	color[ 2 ]  = colorVal.b;

	intensity = fnLight.intensity();

	// get the light transform and flip it as maya's light work in the opposite direction
	// this seems to work correctly!
	RtMatrix rLightFix = {{ 1.0, 0.0,  0.0, 0.0},
						{ 0.0, 1.0,  0.0, 0.0},
						{ 0.0, 0.0, -1.0, 0.0},
						{ 0.0, 0.0,  0.0, 1.0}};

	MMatrix lightFix( rLightFix );

	/*
	philippe : why not use fnLight.lightDiffuse() and fnLight.lightSpecular() ?
	answer   : when decay regions are enabled on a spotlight,
			   suddenly these calls return 0 and the light goes black.
			   On the other hand, the attributes still have the correct value,
			   that's why we use them.
	*/
	fnLight.findPlug( "emitDiffuse"  ).getValue( nonDiffuse );
	fnLight.findPlug( "emitSpecular" ).getValue( nonSpecular );
	nonDiffuse  = 1 - nonDiffuse;
	nonSpecular = 1 - nonSpecular;

	colorVal       = fnLight.shadowColor();
	shadowColor[ 0 ]  = colorVal.r;
	shadowColor[ 1 ]  = colorVal.g;
	shadowColor[ 2 ]  = colorVal.b;

	/* added the ablility to use the lights scale - Dan Kripac - 01/03/06 */
	bool useLightScale( false );
	MPlug useLightScalePlug( fnLight.findPlug( "liquidUseLightScale", &status ) );
	if ( status == MS::kSuccess )
	{
		useLightScalePlug.getValue( useLightScale );
	}
	MTransformationMatrix worldMatrix = light.inclusiveMatrix();
	if ( ! useLightScalePlug )
	{
		double scale[] = { 1.0, 1.0, -1.0 };
		worldMatrix.setScale( scale, MSpace::kTransform );
		MMatrix worldMatrixM( worldMatrix.asMatrix() );
		worldMatrixM.get( transformationMatrix );
	}
	else
	{
		worldMatrix.asMatrix().get( transformationMatrix );
	}

	if( rmanLight )
	{
		lightType = MRLT_Rman;
	}
	else
	{
		// DIRECT MAYA LIGHTS SUPPORT

		if ( light.hasFn( MFn::kAmbientLight ) )
		{
			lightType = MRLT_Ambient;
		}
		else if ( light.hasFn( MFn::kDirectionalLight ) )
		{
			MFnNonExtendedLight fnDistantLight( light );
			lightType = MRLT_Distant;
			
			if ( liqglo_doShadows && usingShadow )
			{
				if ( !rayTraced )
				{
					if ( ( shadowName == "" ) || ( shadowName.substring( 0, 9 ).toLowerCase() == "autoshadow" ))
					{
						shadowName = autoShadowName();
					}
					MPlug samplePlug( lightDepNode.findPlug( "liqShadowMapSamples", &status ) );
					if( MS::kSuccess == status )
					{
						samplePlug.getValue(shadowSamples);
					}
					else
					{
						shadowSamples = fnDistantLight.numShadowSamples( &status );
					}
				}
				else
				{
					shadowName = "raytrace";
					shadowSamples = fnDistantLight.numShadowSamples( &status );
				}
				MPlug blurPlug( lightDepNode.findPlug( "liqShadowBlur", &status ) );
				if( MS::kSuccess == status )
				{
					blurPlug.getValue(shadowBlur);
				}
				shadowFilterSize = fnDistantLight.depthMapFilterSize( &status );
				shadowBias       = fnDistantLight.depthMapBias( &status );
				// Philippe : on a distant light, it seems that shadow radius always returns 0.
				shadowRadius     = fnDistantLight.shadowRadius( &status );
			}
			
			MPlug catPlug = lightDepNode.findPlug( "__category", &status );
			if( MS::kSuccess == status )
			{
				catPlug.getValue(lightCategory);
			}
			else
			{
				lightCategory = "";
			}
			MPlug idPlug = lightDepNode.findPlug( "lightID", &status );
			if( MS::kSuccess == status )
			{
				idPlug.getValue(lightID);
			}
			else
			{
				lightID = 0;
			}
		}
		else if ( light.hasFn( MFn::kPointLight ) )
		{
			MFnNonExtendedLight fnPointLight( light );
			lightType = MRLT_Point;
			decay = fnPointLight.decayRate();
			
			if ( liqglo_doShadows && usingShadow )
			{
				shadowFilterSize = fnPointLight.depthMapFilterSize( &status );
				shadowBias       = fnPointLight.depthMapBias( &status );
				shadowRadius     = fnPointLight.shadowRadius( &status );
				if ( !rayTraced )
				{
					MPlug samplePlug = lightDepNode.findPlug( "liqShadowMapSamples", &status );
					if ( MS::kSuccess == status )
					{
						samplePlug.getValue(shadowSamples);
					}
					else
					{
						shadowSamples = fnPointLight.numShadowSamples( &status );
					}
				}
				else
				{
					shadowSamples = fnPointLight.numShadowSamples( &status );
				}
				MPlug blurPlug( lightDepNode.findPlug( "liqShadowBlur", &status ) );
				if( MS::kSuccess == status )
				{
					blurPlug.getValue(shadowBlur);
				}
			}
			
			MPlug catPlug = lightDepNode.findPlug( "__category", &status );
			if( MS::kSuccess == status )
			{
				catPlug.getValue(lightCategory);
			}
			else
			{
				lightCategory = "";
			}
			MPlug idPlug = lightDepNode.findPlug( "lightID", &status );
			if( MS::kSuccess == status )
			{
				idPlug.getValue(lightID);
			}
			else
			{
				lightID = 0;
			}
		}
		else if( light.hasFn( MFn::kSpotLight ) )
		{
			MFnSpotLight fnSpotLight( light );
			lightType         = MRLT_Spot;
			decay             = fnSpotLight.decayRate();
			coneAngle         = fnSpotLight.coneAngle() / 2.0;
			penumbraAngle     = fnSpotLight.penumbraAngle();
			dropOff           = fnSpotLight.dropOff();
			barnDoors         = fnSpotLight.barnDoors();
			leftBarnDoor      = fnSpotLight.barnDoorAngle( MFnSpotLight::kLeft   );
			rightBarnDoor     = fnSpotLight.barnDoorAngle( MFnSpotLight::kRight  );
			topBarnDoor       = fnSpotLight.barnDoorAngle( MFnSpotLight::kTop    );
			bottomBarnDoor    = fnSpotLight.barnDoorAngle( MFnSpotLight::kBottom );
			decayRegions      = fnSpotLight.useDecayRegions();
			startDistance1    = fnSpotLight.startDistance( MFnSpotLight::kFirst  );
			endDistance1      = fnSpotLight.endDistance(   MFnSpotLight::kFirst  );
			startDistance2    = fnSpotLight.startDistance( MFnSpotLight::kSecond );
			endDistance2      = fnSpotLight.endDistance(   MFnSpotLight::kSecond );
			startDistance3    = fnSpotLight.startDistance( MFnSpotLight::kThird  );
			endDistance3      = fnSpotLight.endDistance(   MFnSpotLight::kThird  );
			MPlug intensityPlug = lightDepNode.findPlug( "startDistanceIntensity1", &status );
			if( MS::kSuccess == status )
			{
				intensityPlug.getValue( startDistanceIntensity1 );
			}
			else
			{
				startDistanceIntensity1 = 1.0;
			}
			intensityPlug = lightDepNode.findPlug( "endDistanceIntensity1", &status );
			if( MS::kSuccess == status )
			{
				intensityPlug.getValue( endDistanceIntensity1 );
			}
			else
			{
				endDistanceIntensity1 = 1.0;
			}
			intensityPlug = lightDepNode.findPlug( "startDistanceIntensity2", &status );
			if( MS::kSuccess == status )
			{
				intensityPlug.getValue( startDistanceIntensity2 );
			}
			else
			{
				startDistanceIntensity2 = 1.0;
			}
			intensityPlug = lightDepNode.findPlug( "endDistanceIntensity2", &status );
			if( MS::kSuccess == status )
			{
				intensityPlug.getValue( endDistanceIntensity2 );
			}
			else
			{
				endDistanceIntensity2 = 1.0;
			}
			intensityPlug = lightDepNode.findPlug( "startDistanceIntensity3", &status );
			if( MS::kSuccess == status )
			{
				intensityPlug.getValue( startDistanceIntensity3 );
			}
			else
			{
				startDistanceIntensity3 = 1.0;
			}
			intensityPlug = lightDepNode.findPlug( "endDistanceIntensity3", &status );
			if( MS::kSuccess == status )
			{
				intensityPlug.getValue( endDistanceIntensity3 );
			}
			else
			{
				endDistanceIntensity3 = 1.0;
			}

			if ( liqglo_doShadows && usingShadow )
			{
				if ( !rayTraced )
				{
					if ( ( shadowName == "" ) || ( shadowName.substring( 0, 9 ).toLowerCase() == "autoshadow" ) )
					{
						shadowName = autoShadowName();
					}
					MPlug samplePlug = lightDepNode.findPlug( "liqShadowMapSamples", &status );
					if ( MS::kSuccess == status )
					{
						samplePlug.getValue(shadowSamples);
					}
					else
					{
						shadowSamples = fnSpotLight.numShadowSamples( &status );
					}
				}
				else
				{
					shadowName = "raytrace";
					shadowSamples    = fnSpotLight.numShadowSamples( &status );
				}
				MPlug blurPlug( lightDepNode.findPlug( "liqShadowBlur", &status ) );
				if( MS::kSuccess == status )
				{
					blurPlug.getValue(shadowBlur);
				}
				shadowFilterSize = fnSpotLight.depthMapFilterSize( &status );
				shadowBias       = fnSpotLight.depthMapBias( &status );
				shadowRadius     = fnSpotLight.shadowRadius( &status );
			}
				
			MPlug catPlug = lightDepNode.findPlug( "__category", &status );
			if ( MS::kSuccess == status )
			{
				catPlug.getValue(lightCategory);
			}
			else
			{
				lightCategory = "";
			}
			MPlug idPlug = lightDepNode.findPlug( "lightID", &status );
			if( MS::kSuccess == status )
			{
				idPlug.getValue(lightID);
			}
			else
			{
				lightID = 0;
			}
		}
		else if ( light.hasFn( MFn::kAreaLight ) )
		{
		  MFnAreaLight fnAreaLight( light );
		  lightType      = MRLT_Area;
		  decay          = fnAreaLight.decayRate();
		  shadowSamples  = 64.0f;
		  if ( liqglo_doShadows && usingShadow ) {
			if ( !rayTraced ) {
			  if ( ( shadowName == "" ) || ( shadowName.substring( 0, 9 ).toLowerCase() == "autoshadow" ) ) {
				shadowName   = autoShadowName();
			  }
			} else {
			  shadowName = "raytrace";
			}
			shadowFilterSize = fnAreaLight.depthMapFilterSize( &status );
			shadowBias       = fnAreaLight.depthMapBias( &status );
			shadowSamples    = fnAreaLight.numShadowSamples( &status );
			shadowRadius     = fnAreaLight.shadowRadius( &status );
		  }
		  MPlug xtraPlug = lightDepNode.findPlug( "liqBothSidesEmit", &status );
		  bool bothsides = false;
		  if ( MS::kSuccess == status ) {
			xtraPlug.getValue( bothsides );
		  }
		  bothSidesEmit = ( bothsides == true) ? 1.0 : 0.0;
		  MPlug catPlug( lightDepNode.findPlug( "__category", &status ) );
		  if ( MS::kSuccess == status ) {
			catPlug.getValue( lightCategory );
		  } else {
			lightCategory = "";
		  }

		  MPlug idPlug( lightDepNode.findPlug( "lightID", &status ) );
		  if ( MS::kSuccess == status ) {
			idPlug.getValue( lightID );
		  } else {
			lightID = 0;
		  }

		  MPlug hitmodePlug( lightDepNode.findPlug( "liqAreaHitmode", &status ) );
		  if ( MS::kSuccess == status ) {
			hitmodePlug.getValue( hitmode );
		  } else {
			hitmode = 1;
		  }

		  MPlug lightmapPlug( lightDepNode.findPlug( "liqLightMap", &status ) );
		  if ( MS::kSuccess == status ) {
			lightmapPlug.getValue( lightMap );
			lightMap = parseString( lightMap, false );
		  } else {
			lightMap = "";
		  }
		  MPlug lightmapsatPlug( lightDepNode.findPlug( "liqLightMapSaturation", &status ) );
		  if ( MS::kSuccess == status ) {
			lightmapsatPlug.getValue( lightMapSaturation );
		  } else {
			lightMapSaturation = 1.0;
		  }
		}
	}
}




/** Write the RIB for this light.
 */
void liqRibLightData::write()
{
	if ( !excludeFromRib )
	{
		LIQDEBUGPRINTF( "-> writing light %s \n", lightName.asChar());
		RiConcatTransform( * const_cast< RtMatrix* >( &transformationMatrix ) );
		if ( liqglo_isShadowPass )
		{
			if ( usingShadow )
			{
				RtString sName( const_cast< char* >( shadowName.asChar() ) );
				// Hmmmmm got to set a LIQUIDHOME env var and use it ...
				// May be set relative name shadowPassLight and resolve path with RIB searchpath
				// Moritz: solved through default shader searchpath in liqRibTranslator
				handle = RiLightSource( "liquidshadowpasslight", "string shadowname", &sName, RI_NULL );
			}
		}
		else
		{
			RtString cat( const_cast< char* >( lightCategory.asChar() ) );
			switch ( lightType )
			{
				case MRLT_Ambient:
				{
					handle = RiLightSource( "ambientlight",
											"intensity",  &intensity,
											"lightcolor", color,
											RI_NULL );
					break;
				}
				case MRLT_Distant:
				{
					if ( liqglo_doShadows && usingShadow )
					{
						RtString shadowname = const_cast< char* >( shadowName.asChar() );
						handle = RiLightSource( "liquiddistant",
												"intensity",              &intensity,
												"lightcolor",             color,
												"string shadowname",      &shadowname,
												"float shadowfiltersize", rayTraced ? &shadowRadius : &shadowFilterSize,
												"float shadowbias",       &shadowBias,
												"float shadowsamples",    &shadowSamples,
												"float shadowblur",       &shadowBlur,
												"color shadowcolor",      &shadowColor,
												"float __nondiffuse",     &nonDiffuse,
												"float __nonspecular",    &nonSpecular,
												"string __category",      &cat,
												"float lightID",          &lightID,
												RI_NULL );
					}
					else
					{
						handle = RiLightSource( "liquiddistant",
												"intensity",            &intensity,
												"lightcolor",           color,
												"color shadowcolor",    &shadowColor,
												"float __nondiffuse",   &nonDiffuse,
												"float __nonspecular",  &nonSpecular,
												"string __category",    &cat,
												"float lightID",        &lightID,
												RI_NULL );
					}
					break;
				}
				case MRLT_Point:
				{
					if ( liqglo_doShadows && usingShadow )
					{
						MString	px = rayTraced ? "raytrace" : autoShadowName( pPX );
						MString	nx = autoShadowName( pNX );
						MString	py = autoShadowName( pPY );
						MString	ny = autoShadowName( pNY );
						MString	pz = autoShadowName( pPZ );
						MString	nz = autoShadowName( pNZ );
						RtString sfpx = const_cast<char*>( px.asChar() );
						RtString sfnx = const_cast<char*>( nx.asChar() );
						RtString sfpy = const_cast<char*>( py.asChar() );
						RtString sfny = const_cast<char*>( ny.asChar() );
						RtString sfpz = const_cast<char*>( pz.asChar() );
						RtString sfnz = const_cast<char*>( nz.asChar() );
						handle = RiLightSource( "liquidpoint",
												"intensity",                  &intensity,
												"lightcolor",                 color,
												"float decay",                &decay,
												"string shadownamepx",        &sfpx,
												"string shadownamenx",        &sfnx,
												"string shadownamepy",        &sfpy,
												"string shadownameny",        &sfny,
												"string shadownamepz",        &sfpz,
												"string shadownamenz",        &sfnz,
												"float shadowfiltersize",     rayTraced ? &shadowRadius : &shadowFilterSize,
												"float shadowbias",           &shadowBias,
												"float shadowsamples",        &shadowSamples,
												"float shadowblur",           &shadowBlur,
												"color shadowcolor",          &shadowColor,
												"float __nondiffuse",         &nonDiffuse,
												"float __nonspecular",        &nonSpecular,
												"string __category",          &cat,
												"float lightID",              &lightID,
												RI_NULL );
					}
					else
					{
					handle = RiLightSource( "liquidpoint",
											"intensity",            &intensity,
											"lightcolor",           color,
											"color shadowcolor",    &shadowColor,
											"float decay",          &decay,
											"float __nondiffuse",   &nonDiffuse,
											"float __nonspecular",  &nonSpecular,
											"string __category",    &cat,
											"float lightID",        &lightID,
											RI_NULL );
					}
					break;
				}
				case MRLT_Spot:
				{
					if (liqglo_doShadows && usingShadow)
					{
						/* if ( ( shadowName == "" ) || ( shadowName.substring( 0, 9 ) == "autoshadow" ) ) {
						shadowName = liqglo_texDir + autoShadowName();
						} */
						RtString shadowname = const_cast< char* >( shadowName.asChar() );
						handle = RiLightSource( "liquidspot",
												"intensity",                    &intensity,
												"lightcolor",                   color,
												"float coneangle",              &coneAngle,
												"float penumbraangle",          &penumbraAngle,
												"float dropoff",                &dropOff,
												"float decay",                  &decay,
												"float barndoors",              &barnDoors,
												"float leftbarndoor",           &leftBarnDoor,
												"float rightbarndoor",          &rightBarnDoor,
												"float topbarndoor",            &topBarnDoor,
												"float bottombarndoor",         &bottomBarnDoor,
												"float decayRegions",           &decayRegions,
												"float startDistance1",         &startDistance1,
												"float endDistance1",           &endDistance1,
												"float startDistance2",         &startDistance2,
												"float endDistance2",           &endDistance2,
												"float startDistance3",         &startDistance3,
												"float endDistance3",           &endDistance3,
												"float startDistanceIntensity1",&startDistanceIntensity1,
												"float endDistanceIntensity1",  &endDistanceIntensity1,
												"float startDistanceIntensity2",&startDistanceIntensity2,
												"float endDistanceIntensity2",  &endDistanceIntensity2,
												"float startDistanceIntensity3",&startDistanceIntensity3,
												"float endDistanceIntensity3",  &endDistanceIntensity3,
												"string shadowname",            &shadowname,
												"float shadowfiltersize",       rayTraced ? &shadowRadius : &shadowFilterSize,
												"float shadowbias",             &shadowBias,
												"float shadowsamples",          &shadowSamples,
												"float shadowblur",             &shadowBlur,
												"color shadowcolor",            &shadowColor,
												"float __nondiffuse",           &nonDiffuse,
												"float __nonspecular",          &nonSpecular,
												"string __category",            &cat,
												"float lightID",                &lightID,
												RI_NULL );
					}
					else
					{
						RtString shadowname = const_cast< char* >( shadowName.asChar() );
						handle = RiLightSource( "liquidspot",
												"intensity",                    &intensity,
												"lightcolor",                   color,
												"color shadowcolor",            &shadowColor,
												"float coneangle",              &coneAngle,
												"float penumbraangle",          &penumbraAngle,
												"float dropoff",                &dropOff,
												"float decay",                  &decay,
												"float barndoors",              &barnDoors,
												"float leftbarndoor",           &leftBarnDoor,
												"float rightbarndoor",          &rightBarnDoor,
												"float topbarndoor",            &topBarnDoor,
												"float bottombarndoor",         &bottomBarnDoor,
												"float decayRegions",           &decayRegions,
												"float startDistance1",         &startDistance1,
												"float endDistance1",           &endDistance1,
												"float startDistance2",         &startDistance2,
												"float endDistance2",           &endDistance2,
												"float startDistance3",         &startDistance3,
												"float endDistance3",           &endDistance3,
												"float startDistanceIntensity1",&startDistanceIntensity1,
												"float endDistanceIntensity1",  &endDistanceIntensity1,
												"float startDistanceIntensity2",&startDistanceIntensity2,
												"float endDistanceIntensity2",  &endDistanceIntensity2,
												"float startDistanceIntensity3",&startDistanceIntensity3,
												"float endDistanceIntensity3",  &endDistanceIntensity3,
												"string shadowname",            &shadowname,
												"float shadowbias",             &shadowBias,
												"float shadowsamples",          &shadowSamples,
												"float __nondiffuse",           &nonDiffuse,
												"float __nonspecular",          &nonSpecular,
												"string __category",            &cat,
												"float lightID",                &lightID,
												RI_NULL );
					}
					break;
				}
				case MRLT_Rman:
				{
					if( rmanShader )
					{
						handle = rmanShader->write(liqglo_shortShaderNames, 0);
					}
					else
					{
						liquidMessage( "[liqRibLightData::write] rman shader wasn't initialized, failed", messageError );
					}
					break;
				}
				case MRLT_Area:
				{
					RtString shadowname = const_cast< char* >( shadowName.asChar() );

					MString coordsys = (lightName+"CoordSys");
					RtString areacoordsys = const_cast< char* >( coordsys.asChar() );

					MString areashader( getenv("LIQUIDHOME") );
					areashader += "/shaders/liquidarea";

					RtString rt_hitmode;
					switch( hitmode )
					{
						case 1:
							rt_hitmode = const_cast< char* >( "primitive" );
							break;
						case 2:
							rt_hitmode = const_cast< char* >( "shader" );
							break;
						default:
							rt_hitmode = const_cast< char* >( "default" );
							break;
					}
					// if raytraced shadows are off, we get a negative value, so we correct it here.
					RtString rt_lightmap( const_cast< char* >( lightMap.asChar() ) );
					handle = RiLightSource( const_cast< char* >( areashader.asChar() ),
											"float intensity",            &intensity,
											"color lightcolor",           color,
											"float decay",                &decay,
											"string coordsys",            &areacoordsys,
											"float lightsamples",         &shadowSamples,
											"float doublesided",          &bothSidesEmit,
											"string shadowname",          &shadowname,
											"color shadowcolor",          &shadowColor,
											"string lightmap",            &rt_lightmap,
											"float lightmapsaturation",   &lightMapSaturation,
											"float lightID",              &lightID,
											"string hitmode",             &rt_hitmode,
											"string __category",          &cat,
											RI_NULL );
					break;
				}
				case MRLT_Unknown:
				{
					break;
				}
			}
		}
	}
}

/** Light comparisons are not supported in this version.
 */
bool liqRibLightData::compare( const liqRibData & otherObj ) const
{
	otherObj.type(); // reference it to avoid unused param compiler warning
	LIQDEBUGPRINTF( "-> comparing light\n" );
	return true;
}


/** Return the object type.
 */
ObjectType liqRibLightData::type() const
{
	LIQDEBUGPRINTF( "-> returning light type\n" );
	return MRT_Light;
}


RtLightHandle liqRibLightData::lightHandle() const
{
	return handle;
}


MString liqRibLightData::autoShadowName( int PointLightDir ) const
{
	MString frame;
	MString shadowName;

	if( userShadowName.length() )
	{
		shadowName = userShadowName;
	}
	else
	{
		shadowName = liquidGetRelativePath( liqglo_relativeFileNames, liqglo_textureDir, liqglo_projectDir );
		if ( !liqglo_shapeOnlyInShadowNames )
		{
			shadowName += liqglo_sceneName;
			shadowName = parseString( shadowName, false );
			shadowName += "_";
		}
		shadowName += sanitizeNodeName( lightName );
		shadowName += ( shadowType == stDeep )? "DSH": "SHD";

		if ( PointLightDir != -1 )
		{
			switch ( PointLightDir )
			{
				case pPX:
					shadowName += "_PX";
					break;
				case pPY:
					shadowName += "_PY";
					break;
				case pPZ:
					shadowName += "_PZ";
					break;
				case pNX:
					shadowName += "_NX";
					break;
				case pNY:
					shadowName += "_NY";
					break;
				case pNZ:
					shadowName += "_NZ";
					break;
			}
		}
		shadowName += ".";
		if ( geometrySet != "" )
		{
			shadowName += geometrySet + ".";
		}
		if ( everyFrame )
		{
			frame += (int) liqglo_lframe;
		}
		else
		{
			frame += (int) renderAtFrame;
		}
		if ( liqglo_doExtensionPadding )
		{
			while( frame.length() < liqglo_outPadding )
			{
				frame = "0" + frame;
			}
		}
		shadowName += frame;
		shadowName += ".tex";
	}
	//cout <<"liqRibLightData::autoShadowName : "<<shadowName.asChar()<<"  ( "<<liqglo_sceneName.asChar()<<" )"<<endl;
	return shadowName;
}


MString liqRibLightData::extraShadowName( const MFnDependencyNode & lightShaderNode, const int & index ) const
{
  MString frame;
  MString shadowName        = "";
  MStatus status;

  MString shdCamName        = "";
  bool shdCamDeepShadows    = false;
  bool shdCamEveryFrame     = true;
  int shdCamRenderAtFrame   = 0;
  MString shdCamGeometrySet = "";

  status.clear();
  MPlug shadowCamerasPlug = lightShaderNode.findPlug( "shadowCameras", &status );
  if ( MS::kSuccess == status ) {
    MPlug theShadowCamPlug = shadowCamerasPlug.elementByPhysicalIndex( index, &status );
    MPlugArray shadowCamPlugArray;
    if ( MS::kSuccess == status && theShadowCamPlug.connectedTo( shadowCamPlugArray, true, false ) ) {
      MFnDependencyNode shadowCamDepNode = shadowCamPlugArray[0].node();
      shdCamName = shadowCamDepNode.name();
      MPlug shadowCamParamPlug = shadowCamDepNode.findPlug( "liqDeepShadows", &status );
      if ( MS::kSuccess == status ) shadowCamParamPlug.getValue( shdCamDeepShadows );
      status.clear();
      shadowCamParamPlug = shadowCamDepNode.findPlug( "liqEveryFrame", &status );
      if ( MS::kSuccess == status ) shadowCamParamPlug.getValue( shdCamEveryFrame );
      status.clear();
      shadowCamParamPlug = shadowCamDepNode.findPlug( "liqRenderAtFrame", &status );
      if ( MS::kSuccess == status ) shadowCamParamPlug.getValue( shdCamRenderAtFrame );
      status.clear();
      shadowCamParamPlug = shadowCamDepNode.findPlug( "liqGeometrySet", &status );
      if ( MS::kSuccess == status ) shadowCamParamPlug.getValue( shdCamGeometrySet );

      shadowName += liqglo_sceneName;
      shadowName =  parseString( shadowName, false );
      shadowName += "_";
      shadowName += sanitizeNodeName( shdCamName );
      shadowName += ( shdCamDeepShadows )? "DSH": "SHD";
      shadowName += ".";
      if ( shdCamGeometrySet != "" ) {
        shadowName += shdCamGeometrySet + ".";
      }
      if ( shdCamEveryFrame ) frame += (int) liqglo_lframe;
      else frame += (int) shdCamRenderAtFrame;
      if ( liqglo_doExtensionPadding ) {
        while( frame.length() < liqglo_outPadding ) frame = "0" + frame;
      }
      shadowName += frame;
      shadowName += ".tex";

    } else {

      //error message here !!

      MString err = "Liquid : could not evaluate shadow camera connected to ";
      err += lightShaderNode.name();
      err += " !";
      status.perror( err );

    }
  } else {
    //error message here !!
    MString err( "Could not find a shadowCameras attribute on '" + lightShaderNode.name() + "'!" );
    liquidMessage( err.asChar(), messageError );
  }

  //cout <<"liqRibLightData::extraShadowName : "<<shadowName.asChar()<<"  ( "<<liqglo_sceneName.asChar()<<" )"<<endl;

  return shadowName;
}



