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

// Renderman Headers
extern "C" {
#include <ri.h>
}

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
#include <liqGlobalHelpers.h>
#include <liqRibLightData.h>
#include <liqGetSloInfo.h>

extern int debugMode;

extern long liqglo_lframe;
extern MString liqglo_sceneName;
extern MString liqglo_textureDir;
extern bool liqglo_isShadowPass;
extern bool liqglo_expandShaderArrays;
extern bool liqglo_useBMRT;
extern bool liqglo_doShadows;
extern bool liqglo_shortShaderNames;
extern MStringArray liqglo_DDimageName;

liqRibLightData::liqRibLightData( const MDagPath & light )
//
//  Description:
//      create a RIB compatible representation of a Maya light
//
  : handle( NULL )
{
  usingShadow     = false;
  shadowType      = stStandart;
  shadowHiderType = shMin;
  rayTraced       = false;
  raySamples      = 16;
  excludeFromRib  = false;
  MStatus status;
  LIQDEBUGPRINTF( "-> creating light\n" );
  rmanLight = false;
  MFnDependencyNode lightDepNode( light.node() );
  MFnDependencyNode lightMainDepNode( light.node() );
  MFnLight fnLight( light );

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
#if defined DELIGHT || PRMAN
  lightDepNode.findPlug( MString( "useRayTraceShadows" ) ).getValue( rayTraced );
  if( rayTraced ) {
    usingShadow = true;
    int raysamples = 1;
    lightDepNode.findPlug( MString( "shadowRays" ) ).getValue( raysamples );
    raySamples = raysamples;
  }
#endif

  lightName = fnLight.name();

  MPlug rmanLightPlug = lightDepNode.findPlug( MString( "liquidLightShaderNode" ), &status );
#if 1
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
      // Hmmmmmmm this length is simply equal to rmShaderStr.length() - 5 + 1, no ?
      assignedRManShader = rmShaderStr.substring( 0, rmShaderStr.length() - 5 ).asChar();
      LIQDEBUGPRINTF( "-> Using Renderman Shader %s. \n", assignedRManShader.asChar() );

      MPlug deepShadowsPlug = lightDepNode.findPlug( "deepShadows", &status );
      if ( status == MS::kSuccess ) {
        deepShadowsPlug.getValue( deepShadows );
      }

      liqGetSloInfo shaderInfo;
      int success = shaderInfo.setShader( rmShaderStr );
      if ( !success ) {
        perror("Slo_SetShader");
        printf("Slo_SetShader(%s) failed in liquid output! \n", assignedRManShader.asChar() );
        rmanLight = false;
      } else {
        int    numArgs = shaderInfo.getNumParam();
        int    i;

        rmanLight = true;
        for ( i = 0; i < numArgs; i++ )
        {
          liqTokenPointer tokenPointerPair;

          // checking to make sure no duplicate attributes end up in the light line
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
          switch (currentArgDetail) {
          case SHADER_DETAIL_UNIFORM:
          {
            tokenPointerPair.setDetailType( rUniform);
            break;
          }
          case SHADER_DETAIL_VARYING:
          {
            tokenPointerPair.setDetailType( rVarying );
            break;
          }
          case SHADER_DETAIL_UNKNOWN:
          {
            break;
          }
          }


          switch (currentArgType) {

          case SHADER_TYPE_STRING: {
            MPlug stringPlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
            if ( status == MS::kSuccess ) {
              MString blah = shaderInfo.getArgName( i ).asChar();
              if ( shaderInfo.getArgArraySize( i ) > 0 ) {
                bool isArrayAttr = stringPlug.isArray( &status );
                if ( isArrayAttr ) {
                  MPlug plugObj;
                  unsigned int arraySize = shaderInfo.getArgArraySize( i );
                  tokenPointerPair.set( shaderInfo.getArgName( i ).asChar(), rString, false, arraySize, 0 );
                  for( unsigned int kk = 0; kk < arraySize; kk++ ) {
                    plugObj = stringPlug.elementByLogicalIndex( kk, &status );
                    if ( status == MS::kSuccess ) {
                      MString stringPlugVal;
                      plugObj.getValue(stringPlugVal);
                      MString parsingString = stringPlugVal;
                      stringPlugVal = parseString( parsingString );
                      tokenPointerPair.setTokenString( kk, stringPlugVal.asChar(), stringPlugVal.length() );
                    }
                  }
                  tokenPointerArray.push_back( tokenPointerPair );
                }
              } else {
                MString stringPlugVal;
                stringPlug.getValue( stringPlugVal );
                MString stringDefault( shaderInfo.getArgStringDefault( i, 0 ) );
                if ( stringPlugVal != "" && stringPlugVal != stringDefault ) {
                  MString parsingString = stringPlugVal;
                  stringPlugVal = parseString( parsingString );
                  parsingString = stringPlugVal;
                  parsingString.toLowerCase();
                  MString curStrArgName = shaderInfo.getArgName(i);
                  if ( curStrArgName == "shadowname") {
                    if ( (parsingString.substring(0, 9) == "autoshadow") || (parsingString == "") ) {
                      MString suffix = "";
                      if ( stringPlugVal.length() > 10 )
                      {
                        suffix = stringPlugVal.substring( 10, stringPlugVal.length() - 1 );
                      }
                      if ( liqglo_doShadows ) {
                        shadowName = liqglo_textureDir;
                        if ( userShadowName == MString( "" ) )
                        {
                          shadowName += autoShadowName( -1 ) + suffix;
                        } else {
                          shadowName += userShadowName;
                        }
                        usingShadow = true;
                        stringPlugVal = shadowName;
                      }
                    }
                  }
					
                  tokenPointerPair.set( shaderInfo.getArgName( i ).asChar(), rString, false, 0, 0 );
                  tokenPointerPair.setTokenString( 0, stringPlugVal.asChar(), stringPlugVal.length() );
                  tokenPointerArray.push_back( tokenPointerPair );
                }
              }
            }
            break; }

          case SHADER_TYPE_SCALAR: {
            MPlug floatPlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
            if ( status == MS::kSuccess ) {
              if ( shaderInfo.getArgArraySize( i ) > 0 ) {
                // philippe : check we have a multi attr, not a floatArray attr
                bool isArrayAttr = floatPlug.isArray( &status );
                if ( isArrayAttr ) {
                  // philippe : new way to store float arrays as multi attr
                  MPlug plugObj;
                  unsigned int arraySize = shaderInfo.getArgArraySize( i );
                  tokenPointerPair.set( shaderInfo.getArgName( i ).asChar(), rFloat, false, false, true, arraySize );
                  for( unsigned int kk = 0; kk < arraySize; kk++ ) {
                    plugObj = floatPlug.elementByLogicalIndex( kk, &status );
                    if ( status == MS::kSuccess ) {
                      float x;
                      plugObj.getValue(x);
                      tokenPointerPair.setTokenFloat( kk, x  );
                    }
                  }
                  tokenPointerArray.push_back( tokenPointerPair );
                } else {
                  // philippe : keep the old stuff for compatibility's sake
                  if ( liqglo_expandShaderArrays ) {
                    MObject plugObj;
                    floatPlug.getValue( plugObj );
                    MFnDoubleArrayData  fnDoubleArrayData( plugObj );
                    MDoubleArray doubleArrayData = fnDoubleArrayData.array( &status );
                    int k;
                    char tmpStr[256];
                    for ( k = 0; k < shaderInfo.getArgArraySize( i ); k++ ) {
                      sprintf( tmpStr , "%s%d", shaderInfo.getArgName( i ).asChar(), ( k + 1 ) );
                      tokenPointerPair.set( tmpStr, rFloat, false, false, false, 0 );
                      tokenPointerPair.setTokenFloat( 0, doubleArrayData[k] );
                      tokenPointerArray.push_back( tokenPointerPair );
                    }
                  } else {
                    MObject plugObj;
                    floatPlug.getValue( plugObj );
                    MFnDoubleArrayData  fnDoubleArrayData( plugObj );
                    MDoubleArray doubleArrayData = fnDoubleArrayData.array( &status );
                    unsigned int arraySize = shaderInfo.getArgArraySize( i );
                    // Hmmmmmm really a uArray here ?
                    tokenPointerPair.set( shaderInfo.getArgName( i ).asChar(), rFloat, false, false, true, arraySize );
                    for( int k = 0; k < arraySize; k++ )
                      tokenPointerPair.setTokenFloat( k, doubleArrayData[k] );
                    tokenPointerArray.push_back( tokenPointerPair );
                  }
                }
              } else {
                float floatPlugVal;
                floatPlug.getValue( floatPlugVal );
                tokenPointerPair.set( shaderInfo.getArgName( i ).asChar(), rFloat, false, false, false, 0 );
                tokenPointerPair.setTokenFloat( 0, floatPlugVal );
                tokenPointerArray.push_back( tokenPointerPair );
              }
            }
            break; }

          case SHADER_TYPE_COLOR: {
            MPlug triplePlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
            unsigned int arraySize = shaderInfo.getArgArraySize( i );
            if ( arraySize > 0 ) {
              // philippe : array support
              if ( status == MS::kSuccess ) {
                tokenPointerPair.set( shaderInfo.getArgName( i ).asChar(), rColor, false, false, true, arraySize );
                for( unsigned int kk = 0; kk < arraySize; kk++ ) {
                  MPlug argNameElement = triplePlug.elementByLogicalIndex( kk );
                  if ( status == MS::kSuccess ) {
                    float x, y, z;
                    argNameElement.child(0).getValue( x );
                    argNameElement.child(1).getValue( y );
                    argNameElement.child(2).getValue( z );
                    tokenPointerPair.setTokenFloat( kk, x, y, z );
                  }
                }
                tokenPointerArray.push_back( tokenPointerPair );
              }
            } else {
              if ( status == MS::kSuccess ) {
                double x, y, z;
                triplePlug.child(0).getValue( x );
                triplePlug.child(1).getValue( y );
                triplePlug.child(2).getValue( z );
                tokenPointerPair.set( shaderInfo.getArgName( i ).asChar(), rColor, false, false, false, 0 );
                tokenPointerPair.setTokenFloat( 0, x, y, z );
                tokenPointerArray.push_back( tokenPointerPair );
              }
            }
            break; }

          case SHADER_TYPE_POINT: {
            MPlug triplePlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
            unsigned int arraySize = shaderInfo.getArgArraySize( i );
            if ( arraySize > 0 ) {
              // philippe : array support
              if ( status == MS::kSuccess ) {
                tokenPointerPair.set( shaderInfo.getArgName( i ).asChar(), rPoint, false, false, true, arraySize );
                for( unsigned int kk = 0; kk < arraySize; kk++ ) {
                  MPlug argNameElement = triplePlug.elementByLogicalIndex( kk );
                  if ( status == MS::kSuccess ) {
                    float x, y, z;
                    argNameElement.child(0).getValue( x );
                    argNameElement.child(1).getValue( y );
                    argNameElement.child(2).getValue( z );
                    tokenPointerPair.setTokenFloat( kk, x, y, z );
                  }
                }
                tokenPointerArray.push_back( tokenPointerPair );
              }
            } else {
              if ( status == MS::kSuccess ) {
                double x, y, z;
                triplePlug.child(0).getValue( x );
                triplePlug.child(1).getValue( y );
                triplePlug.child(2).getValue( z );
                tokenPointerPair.set( shaderInfo.getArgName( i ).asChar(), rPoint, false, false, false, 0 );
                tokenPointerPair.setTokenFloat( 0, x, y, z );
                tokenPointerArray.push_back( tokenPointerPair );
              }
            }
            break; }

          case SHADER_TYPE_VECTOR: {
            MPlug triplePlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
            unsigned int arraySize = shaderInfo.getArgArraySize( i );
            if ( arraySize > 0 ) {
              // philippe : array support
              if ( status == MS::kSuccess ) {
                tokenPointerPair.set( shaderInfo.getArgName( i ).asChar(), rVector, false, false, true, arraySize );
                for( unsigned int kk = 0; kk < arraySize; kk++ ) {
                  MPlug argNameElement = triplePlug.elementByLogicalIndex( kk );
                  if ( status == MS::kSuccess ) {
                    float x, y, z;
                    argNameElement.child(0).getValue( x );
                    argNameElement.child(1).getValue( y );
                    argNameElement.child(2).getValue( z );
                    tokenPointerPair.setTokenFloat( kk, x, y, z );
                  }
                }
                tokenPointerArray.push_back( tokenPointerPair );
              }
            } else {
              MPlug triplePlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
              if ( status == MS::kSuccess ) {
                double x, y, z;
                triplePlug.child(0).getValue( x );
                triplePlug.child(1).getValue( y );
                triplePlug.child(2).getValue( z );
                tokenPointerPair.set( shaderInfo.getArgName( i ).asChar(), rVector, false, false, false, 0 );
                tokenPointerPair.setTokenFloat( 0, x, y, z );
                tokenPointerArray.push_back( tokenPointerPair );
              }
            }
            break; }

          case SHADER_TYPE_NORMAL: {
            MPlug triplePlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
            unsigned int arraySize = shaderInfo.getArgArraySize( i );
            if ( arraySize > 0 ) {
              // philippe : array support
              if ( status == MS::kSuccess ) {
                tokenPointerPair.set( shaderInfo.getArgName( i ).asChar(), rNormal, false, false, true, arraySize );
                for( unsigned int kk = 0; kk < arraySize; kk++ ) {
                  MPlug argNameElement = triplePlug.elementByLogicalIndex( kk );
                  if ( status == MS::kSuccess ) {
                    float x, y, z;
                    argNameElement.child(0).getValue( x );
                    argNameElement.child(1).getValue( y );
                    argNameElement.child(2).getValue( z );
                    tokenPointerPair.setTokenFloat( kk, x, y, z );
                  }
                }
                tokenPointerArray.push_back( tokenPointerPair );
              }
            } else {
              MPlug triplePlug = lightDepNode.findPlug( shaderInfo.getArgName( i ), &status );
              if ( status == MS::kSuccess ) {
                double x, y, z;
                triplePlug.child(0).getValue( x );
                triplePlug.child(1).getValue( y );
                triplePlug.child(2).getValue( z );
                tokenPointerPair.set( shaderInfo.getArgName( i ).asChar(), rNormal, false, false, false, 0 );
                tokenPointerPair.setTokenFloat( 0, x, y, z );
                tokenPointerArray.push_back( tokenPointerPair );
              }
            }
            break; }
          case SHADER_TYPE_MATRIX: {
            printf( "WHAT IS THE MATRIX?!\n" );
            break; }
          default:
            printf( "Unknown\n" );
            break; }
        }
      }
      shaderInfo.resetIt();
    }
  }// else {
    if( !rayTraced ) {
      fnLight.findPlug( "useDepthMapShadows" ).getValue( usingShadow );
		}
  //}
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

  nonDiffuse     = fnLight.lightDiffuse() ? 0 : 1;
  nonSpecular    = fnLight.lightSpecular() ? 0 : 1;
  colorVal       = fnLight.shadowColor();
  shadowColor[ 0 ]  = colorVal.r;
  shadowColor[ 1 ]  = colorVal.g;
  shadowColor[ 2 ]  = colorVal.b;

  MTransformationMatrix worldMatrix = light.inclusiveMatrix();
  double scale[] = { 1.0, 1.0, -1.0 };
  worldMatrix.setScale( scale, MSpace::kTransform );
  MMatrix worldMatrixM = worldMatrix.asMatrix();
  worldMatrixM.get( transformationMatrix );

  if ( rmanLight ) {
    lightType  = MRLT_Rman;
  } else {

    if ( light.hasFn( MFn::kAmbientLight ) ) {
      lightType      = MRLT_Ambient;

    } else if ( light.hasFn( MFn::kDirectionalLight ) ) {
      MFnNonExtendedLight fnDistantLight( light );
      lightType      = MRLT_Distant;

      if ( liqglo_doShadows && usingShadow && !rayTraced ) {
        if ( ( shadowName == "" ) || ( shadowName.substring( 0, 9 ) == "autoshadow" )) {
          shadowName       = liqglo_textureDir + autoShadowName();
        }
        shadowFilterSize = fnDistantLight.depthMapFilterSize( &status );
        shadowBias       = fnDistantLight.depthMapBias( &status );
        shadowSamples    = raySamples;
      }

    } else if ( light.hasFn( MFn::kPointLight ) ) {
      MFnNonExtendedLight fnPointLight( light );
      lightType      = MRLT_Point;
      decay          = fnPointLight.decayRate();


      if ( liqglo_doShadows && usingShadow ) {
        shadowFilterSize = fnPointLight.depthMapFilterSize( &status );
        shadowBias       = fnPointLight.depthMapBias( &status );
        shadowSamples    = raySamples;
      }

    } else if ( light.hasFn( MFn::kSpotLight ) ) {
      MFnSpotLight fnSpotLight( light );
      lightType      = MRLT_Spot;
      decay          = fnSpotLight.decayRate();
      coneAngle      = fnSpotLight.coneAngle() / 2.0;
      penumbraAngle  = fnSpotLight.penumbraAngle() / 2.0;
      dropOff        = fnSpotLight.dropOff();
      barnDoors      = fnSpotLight.barnDoors();
      leftBarnDoor   = fnSpotLight.barnDoorAngle( MFnSpotLight::kLeft );
      rightBarnDoor  = fnSpotLight.barnDoorAngle( MFnSpotLight::kRight );
      topBarnDoor    = fnSpotLight.barnDoorAngle( MFnSpotLight::kTop );
      bottomBarnDoor = fnSpotLight.barnDoorAngle( MFnSpotLight::kBottom );

      if ( liqglo_doShadows && usingShadow && !rayTraced ) {
        if ( ( shadowName == "" ) || ( shadowName.substring( 0, 9 ) == "autoshadow" ) ) {
          shadowName       = liqglo_textureDir + autoShadowName();

        }
        shadowFilterSize = fnSpotLight.depthMapFilterSize( &status );
        shadowBias       = fnSpotLight.depthMapBias( &status );
        shadowSamples    = raySamples;
      }
    }
  }
}

liqRibLightData::~liqRibLightData()
{
  LIQDEBUGPRINTF( "-> killing light data.\n" );

  assignedRManShader.clear();
  userShadowName.clear();
  lightName.clear();

  shadowName.clear();
  shadowNamePx.clear();
  shadowNameNx.clear();
  shadowNamePy.clear();
  shadowNameNy.clear();
  shadowNamePz.clear();
  shadowNameNz.clear();

  LIQDEBUGPRINTF( "-> finished killing light data.\n" );
}

void liqRibLightData::write()
//
//  Description:
//      Write the RIB for this light
//
{
  if ( !excludeFromRib ) {
    LIQDEBUGPRINTF( "-> writing light\n" );

    RiConcatTransform( transformationMatrix );
    if ( liqglo_isShadowPass ) {
      if ( usingShadow ) {
        RtString sName = const_cast< char* >( shadowName.asChar() );
        // Hmmmmm got to set a LIQUIDHOME env var and use it ...
        // May be set relative name shadowPassLight and resolve path with RIB searchpath
        // Moritz: solved through default shader searchpath in liqRibTranslator
        handle = RiLightSource( "liquidshadowpasslight", "string shadowname", &sName, RI_NULL );
      }
    } else {

#ifdef DELIGHT
      // If the light is casting raytraced shadows then set the attribute
      // and the samples for shadow oversampling
      if ( rayTraced ) {
        RtString param = "on";
        RiAttribute( ( RtToken ) "light", ( RtToken ) "shadows", &param, "samples", &raySamples, RI_NULL );
      }
#endif

      switch ( lightType ) {
      case MRLT_Ambient:
        handle = RiLightSource( "ambientlight",
                                "intensity",  &intensity,
                                "lightcolor", color,
                                RI_NULL );
        break;
      case MRLT_Distant:
        if ( liqglo_doShadows && usingShadow ) {
          /*if ( ( shadowName == "" ) || ( shadowName.substring( 0, 9 ) == "autoshadow" )) {
            shadowName = liqglo_texDir + autoShadowName();
          }*/
          RtString shadowname = const_cast< char* >( shadowName.asChar() );
          handle = RiLightSource( "liquiddistant",
                                  "intensity",            &intensity,
                                  "lightcolor",           color,
                                  "string shadowname",    &shadowname,
                                  "float shadowfiltersize", &shadowFilterSize,
                                  "float shadowbias",     &shadowBias,
                                  "float shadowsamples",  &shadowSamples,
                                  "color shadowcolor",    &shadowColor,
                                  "float __nondiffuse",   &nonDiffuse,
                                  "float __nonspecular",  &nonSpecular,
                                  RI_NULL );
        } else {
          handle = RiLightSource( "liquiddistant",
                                  "intensity",            &intensity,
                                  "lightcolor",           color,
                                  "float __nondiffuse",   &nonDiffuse,
                                  "float __nonspecular",  &nonSpecular,
                                  RI_NULL );
        }
        break;
      case MRLT_Point:
        if ( liqglo_doShadows && usingShadow ) {
          /*if (( shadowName == "" ) || ( shadowName.substring(0, 9) == "autoshadow" )) {
            shadowName = liqglo_texDir + autoShadowName();
          }*/
          MString	px = liqglo_textureDir + autoShadowName( pPX );
          MString	nx = liqglo_textureDir + autoShadowName( pNX );
          MString	py = liqglo_textureDir + autoShadowName( pPY );
          MString	ny = liqglo_textureDir + autoShadowName( pNY );
          MString	pz = liqglo_textureDir + autoShadowName( pPZ );
          MString	nz = liqglo_textureDir + autoShadowName( pNZ );
          RtString sfpx = const_cast<char*>( px.asChar() );
          RtString sfnx = const_cast<char*>( nx.asChar() );
          RtString sfpy = const_cast<char*>( py.asChar() );
          RtString sfny = const_cast<char*>( ny.asChar() );
          RtString sfpz = const_cast<char*>( pz.asChar() );
          RtString sfnz = const_cast<char*>( nz.asChar() );

          handle = RiLightSource( "liquidpoint",
                                  "intensity",            &intensity,
                                  "lightcolor",           color,
                                  "float decay",          &decay,
                                  "string shadownamepx",         &sfpx,
                                  "shadownamenx",         &sfnx,
                                  "shadownamepy",         &sfpy,
                                  "shadownameny",         &sfny,
                                  "shadownamepz",         &sfpz,
                                  "shadownamenz",         &sfnz,
                                  "float shadowfiltersize", &shadowFilterSize,
                                  "float shadowbias",     &shadowBias,
                                  "float shadowsamples",  &shadowSamples,
                                  "color shadowcolor",    &shadowColor,
                                  "float __nondiffuse",   &nonDiffuse,
                                  "float __nonspecular",  &nonSpecular,
                                  RI_NULL );
        } else {
          handle = RiLightSource( "liquidpoint",
                                  "intensity",            &intensity,
                                  "lightcolor",           color,
                                  "float decay",          &decay,
                                  "float __nondiffuse",   &nonDiffuse,
                                  "float __nonspecular",  &nonSpecular,
                                  RI_NULL );
        }
        break;
      case MRLT_Spot:
        if (liqglo_doShadows && usingShadow) {
          /* if ( ( shadowName == "" ) || ( shadowName.substring( 0, 9 ) == "autoshadow" ) ) {
            shadowName = liqglo_texDir + autoShadowName();
          } */
          RtString shadowname = const_cast< char* >( shadowName.asChar() );
          handle = RiLightSource( "liquidspot",
                                  "intensity",            &intensity,
                                  "lightcolor",           color,
                                  "float coneangle",      &coneAngle,
                                  "float penumbraangle",  &penumbraAngle,
                                  "float dropoff",        &dropOff,
                                  "float decay",          &decay,
                                  "float barndoors",      &barnDoors,
                                  "float leftbarndoor",   &leftBarnDoor,
                                  "float rightbarndoor",  &rightBarnDoor,
                                  "float topbarndoor",    &topBarnDoor,
                                  "float bottombarndoor", &bottomBarnDoor,
                                  "string shadowname",    &shadowname,
                                  "float shadowfiltersize", &shadowFilterSize,
                                  "float shadowbias",     &shadowBias,
                                  "float shadowsamples",  &shadowSamples,
                                  "color shadowcolor",    &shadowColor,
                                  "float __nondiffuse",   &nonDiffuse,
                                  "float __nonspecular",  &nonSpecular,
                                  RI_NULL );
          } else {
          RtString shadowname = const_cast< char* >( shadowName.asChar() );
          handle = RiLightSource( "liquidspot",
                                  "intensity",            &intensity,
                                  "lightcolor",           color,
                                  "float coneangle",      &coneAngle,
                                  "float penumbraangle",  &penumbraAngle,
                                  "float dropoff",        &dropOff,
                                  "float decay",          &decay,
                                  "float barndoors",      &barnDoors,
                                  "float leftbarndoor",   &leftBarnDoor,
                                  "float rightbarndoor",  &rightBarnDoor,
                                  "float topbarndoor",    &topBarnDoor,
                                  "float bottombarndoor", &bottomBarnDoor,
                                  "string shadowname",    &shadowname,
                                  "float shadowbias",     &shadowBias,
                                  "float shadowsamples",  &shadowSamples,
                                  "float __nondiffuse",   &nonDiffuse,
                                  "float __nonspecular",  &nonSpecular,
                                  RI_NULL );
        }
        break;
      case MRLT_Rman: {
        RtToken   *tokenArray   = (RtToken *)   alloca( sizeof(RtToken)   * tokenPointerArray.size() );
        RtPointer *pointerArray = (RtPointer *) alloca( sizeof(RtPointer) * tokenPointerArray.size() );
        assignTokenArraysV( &tokenPointerArray, tokenArray, pointerArray );

        if ( liqglo_shortShaderNames ) {
          assignedRManShader = basename( const_cast<char*>(assignedRManShader.asChar()) );
        }

        RtString shaderName = const_cast<char*>(assignedRManShader.asChar());
        handle = RiLightSourceV( shaderName, tokenPointerArray.size(), tokenArray, pointerArray );
        break;
      }
      case MRLT_Area:
      {
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

bool liqRibLightData::compare( const liqRibData & otherObj ) const
//
//  Description:
//      Light comparisons are not supported in this version.
//
{
  otherObj.type(); // reference it to avoid unused param compiler warning
  LIQDEBUGPRINTF( "-> comparing light\n" );
  return true;
}
ObjectType liqRibLightData::type() const
//
//  Description:
//      return the object type
//
{
  LIQDEBUGPRINTF( "-> returning light type\n" );
  return MRT_Light;
}
RtLightHandle liqRibLightData::lightHandle() const
{
  return handle;
}


/*
MString liqRibLightData::autoShadowName( MString suffix ) const
{
  MString shadowName;
  shadowName += liqglo_sceneName;
  shadowName += "_";
  shadowName += lightName;
  shadowName += "SHD";
  if ( suffix != "" )
  {
    shadowName += ( "_" + suffix );
  }
  shadowName += ".";
  shadowName += ( int )liqglo_lframe;

  // Deepshadows need ".shd", and regular need ".tex"
  //
  if ( deepShadows )
  {
    shadowName += ".shd";
  }
  else
  {
    shadowName += ".tex";
  }

  return shadowName;
}*/


MString liqRibLightData::autoShadowName( int PointLightDir ) const
{
  MString shadowName;
  if ( ( liqglo_DDimageName[0] == "" ) ) {
    shadowName += liqglo_sceneName;
  } else {
    int pointIndex = liqglo_DDimageName[0].index( '.' );
    shadowName += liqglo_DDimageName[0].substring(0, pointIndex-1).asChar();
  }
  shadowName = parseString( shadowName );
  shadowName += "_";
  shadowName += lightName;
  shadowName += ( shadowType == stDeep )? "DSH": "SHD";

  if ( PointLightDir != -1 ) {
    switch ( PointLightDir ) {
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
  shadowName += (int)liqglo_lframe;
  shadowName += ".tex";
  return shadowName;
}

