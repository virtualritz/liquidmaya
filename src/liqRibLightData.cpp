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
#include <maya/MFnAreaLight.h>
#include <maya/MColor.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>


#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liqRibLightData.h>
#include <liqGetSloInfo.h>

extern int debugMode;

extern long         liqglo_lframe;
extern MString      liqglo_sceneName;
extern MString      liqglo_textureDir;
extern bool         liqglo_isShadowPass;
extern bool         liqglo_expandShaderArrays;
extern bool         liqglo_useBMRT;
extern bool         liqglo_doShadows;
extern bool         liqglo_shapeOnlyInShadowNames;
extern bool         liqglo_shortShaderNames;
extern MStringArray liqglo_DDimageName;
extern bool         liqglo_doExtensionPadding;
extern liquidlong   liqglo_outPadding;
extern MString      liqglo_projectDir;
extern bool         liqglo_relativeFileNames;
extern MString      liqglo_textureDir;

liqRibLightData::liqRibLightData( const MDagPath & light )
//
//  Description:
//      create a RIB compatible representation of a Maya light
//
  : handle( NULL )
{
  usingShadow         = false;
  shadowType          = stStandart;
  shadowHiderType     = shMin;
  rayTraced           = false;
  raySamples          = 16;
  shadowRadius        = 0;
  excludeFromRib      = false;
  //outputLightInShadow = false;

  everyFrame          = true;
  renderAtFrame       = 0;
  geometrySet         = "";

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
  if ( status == MS::kSuccess ) {
    excludeFromRibPlug.getValue( excludeFromRib );
  }

  // check if the light should be in the shadow pass - deep shadows only.
  //status.clear();
  //MPlug outputInShadowPlug = fnLight.findPlug( "outputInShadow", &status );
  //if ( status == MS::kSuccess ) {
  //  outputInShadowPlug.getValue( outputLightInShadow );
  //}

  status.clear();
  MPlug userShadowNamePlug = fnLight.findPlug( "liquidShadowName", &status );
  if ( status == MS::kSuccess ) {
    MString varVal;
    userShadowNamePlug.getValue( varVal );
    userShadowName = parseString( varVal );
  }

  // check to see if the light is using raytraced shadows
#if defined ( DELIGHT ) || defined ( PRMAN ) || defined( PIXIE ) || defined( AIR )
  rayTraced = fnLight.useRayTraceShadows();
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
        if ( deepShadows ) shadowType = stDeep;
      }
      MPlug paramPlug = lightDepNode.findPlug( "everyFrame", &status );
      if ( status == MS::kSuccess ) {
        paramPlug.getValue( everyFrame );
      }
      paramPlug = lightDepNode.findPlug( "renderAtFrame", &status );
      if ( status == MS::kSuccess ) {
        float tmp;
        paramPlug.getValue( tmp );
        renderAtFrame = (int) tmp;
      }
      paramPlug = lightDepNode.findPlug( "geometrySet", &status );
      if ( status == MS::kSuccess ) {
        paramPlug.getValue( geometrySet );
      }

      liqGetSloInfo shaderInfo;
      int success = shaderInfo.setShaderNode( lightDepNode );
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
                // string array
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
                      if ( parsingString.substring( 0, 9 ) == "autoshadow" ) {
                        if ( parsingString.length() > 10 && parsingString.substring( 10, parsingString.length()-1 ).isInt() ) {
                          int extraShadowSlot = parsingString.substring( 10, parsingString.length()-1 ).asInt();
                          //cout <<"this is an extra shadow call for slot "<<extraShadowSlot<" !!"<<endl;
                          stringPlugVal = extraShadowName( lightDepNode, extraShadowSlot );
                        } else stringPlugVal = autoShadowName();
                      } else stringPlugVal = parseString( parsingString );
                      tokenPointerPair.setTokenString( kk, stringPlugVal.asChar(), stringPlugVal.length() );
                    }
                  }
                  tokenPointerArray.push_back( tokenPointerPair );
                }
              } else {
                // simple string
                MString stringPlugVal;
                stringPlug.getValue( stringPlugVal );
                MString stringDefault( shaderInfo.getArgStringDefault( i, 0 ) );
                if ( stringPlugVal != "" && stringPlugVal != stringDefault ) {
                  MString parsingString = stringPlugVal;
                  if ( parsingString.substring( 0, 9 ) == "autoshadow" ) {
                    if ( parsingString.length() > 10 && parsingString.substring( 10, parsingString.length()-1 ).isInt() ) {
                      int extraShadowSlot = parsingString.substring( 10, parsingString.length()-1 ).asInt();
                      //cout <<"this is an extra shadow call for slot "<<extraShadowSlot<" !!"<<endl;
                      stringPlugVal = extraShadowName( lightDepNode, extraShadowSlot );
                    } else stringPlugVal = autoShadowName();
                  } else stringPlugVal = parseString( parsingString );
                  parsingString = stringPlugVal;
                  parsingString.toLowerCase();
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
  } else {
    if( !rayTraced ) {

      fnLight.findPlug( "useDepthMapShadows" ).getValue( usingShadow );

      if ( usingShadow ) {

        MPlug deepShadowsPlug = lightDepNode.findPlug( "deepShadows", &status );
        if ( status == MS::kSuccess ) {
          deepShadowsPlug.getValue( deepShadows );
          if ( deepShadows ) shadowType = stDeep;
        }
        MPlug paramPlug = lightDepNode.findPlug( "everyFrame", &status );
        if ( status == MS::kSuccess ) {
          paramPlug.getValue( everyFrame );
        }
        paramPlug = lightDepNode.findPlug( "renderAtFrame", &status );
        if ( status == MS::kSuccess ) {
          float tmp;
          paramPlug.getValue( tmp );
          renderAtFrame = (int) tmp;
        }
        paramPlug = lightDepNode.findPlug( "geometrySet", &status );
        if ( status == MS::kSuccess ) {
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

    // DIRECT MAYA LIGHTS SUPPORT

    if ( light.hasFn( MFn::kAmbientLight ) ) {
      lightType      = MRLT_Ambient;

    } else if ( light.hasFn( MFn::kDirectionalLight ) ) {
      MFnNonExtendedLight fnDistantLight( light );
      lightType      = MRLT_Distant;

      if ( liqglo_doShadows && usingShadow ) {
        if ( !rayTraced ) {
          if ( ( shadowName == "" ) || ( shadowName.substring( 0, 9 ) == "autoshadow" )) {
            shadowName       = autoShadowName();
          }
          MPlug samplePlug = lightDepNode.findPlug( "liqShadowMapSamples", &status );
          if ( status == MS::kSuccess ) samplePlug.getValue(shadowSamples);
          else shadowSamples = fnDistantLight.numShadowSamples( &status );
        } else {
          shadowName = "raytrace";
          shadowSamples = fnDistantLight.numShadowSamples( &status );
        }

        shadowFilterSize = fnDistantLight.depthMapFilterSize( &status );
        shadowBias       = fnDistantLight.depthMapBias( &status );
        // Philippe : on a distant light, it seems that shadow radius always returns 0.
        shadowRadius     = fnDistantLight.shadowRadius( &status );
      }

      MPlug catPlug = lightDepNode.findPlug( "__category", &status );
      if ( status == MS::kSuccess ) catPlug.getValue(lightCategory);
      else lightCategory = "";
      MPlug idPlug = lightDepNode.findPlug( "lightID", &status );
      if ( status == MS::kSuccess ) idPlug.getValue(lightID);
      else lightID = 0;

    } else if ( light.hasFn( MFn::kPointLight ) ) {
      MFnNonExtendedLight fnPointLight( light );
      lightType      = MRLT_Point;
      decay          = fnPointLight.decayRate();

      if ( liqglo_doShadows && usingShadow ) {
        shadowFilterSize = fnPointLight.depthMapFilterSize( &status );
        shadowBias       = fnPointLight.depthMapBias( &status );
        shadowRadius     = fnPointLight.shadowRadius( &status );
        if ( !rayTraced ) {
          MPlug samplePlug = lightDepNode.findPlug( "liqShadowMapSamples", &status );
          if ( status == MS::kSuccess ) samplePlug.getValue(shadowSamples);
          else shadowSamples = fnPointLight.numShadowSamples( &status );
        } else {
          shadowSamples = fnPointLight.numShadowSamples( &status );
        }
      }

      MPlug catPlug = lightDepNode.findPlug( "__category", &status );
      if ( status == MS::kSuccess ) catPlug.getValue(lightCategory);
      else lightCategory = "";
      MPlug idPlug = lightDepNode.findPlug( "lightID", &status );
      if ( status == MS::kSuccess ) idPlug.getValue(lightID);
      else lightID = 0;

    } else if ( light.hasFn( MFn::kSpotLight ) ) {
      MFnSpotLight fnSpotLight( light );
      lightType      = MRLT_Spot;
      decay          = fnSpotLight.decayRate();
      coneAngle      = fnSpotLight.coneAngle() / 2.0;
      penumbraAngle  = fnSpotLight.penumbraAngle();
      dropOff        = fnSpotLight.dropOff();
      barnDoors      = fnSpotLight.barnDoors();
      leftBarnDoor   = fnSpotLight.barnDoorAngle( MFnSpotLight::kLeft );
      rightBarnDoor  = fnSpotLight.barnDoorAngle( MFnSpotLight::kRight );
      topBarnDoor    = fnSpotLight.barnDoorAngle( MFnSpotLight::kTop );
      bottomBarnDoor = fnSpotLight.barnDoorAngle( MFnSpotLight::kBottom );

      if ( liqglo_doShadows && usingShadow ) {
        if ( !rayTraced ) {
          if ( ( shadowName == "" ) || ( shadowName.substring( 0, 9 ) == "autoshadow" ) ) {
            shadowName       = autoShadowName();
          }
          MPlug samplePlug = lightDepNode.findPlug( "liqShadowMapSamples", &status );
          if ( status == MS::kSuccess ) samplePlug.getValue(shadowSamples);
          else shadowSamples = fnSpotLight.numShadowSamples( &status );
        } else {
          shadowName = "raytrace";
          shadowSamples    = fnSpotLight.numShadowSamples( &status );
        }
        shadowFilterSize = fnSpotLight.depthMapFilterSize( &status );
        shadowBias       = fnSpotLight.depthMapBias( &status );
        shadowRadius     = fnSpotLight.shadowRadius( &status );
      }

      MPlug catPlug = lightDepNode.findPlug( "__category", &status );
      if ( status == MS::kSuccess ) catPlug.getValue(lightCategory);
      else lightCategory = "";
      MPlug idPlug = lightDepNode.findPlug( "lightID", &status );
      if ( status == MS::kSuccess ) idPlug.getValue(lightID);
      else lightID = 0;

    } else if ( light.hasFn( MFn::kAreaLight ) ) {
      MFnAreaLight fnAreaLight( light );
      lightType      = MRLT_Area;
      decay          = fnAreaLight.decayRate();
      if ( liqglo_doShadows && usingShadow ) {
        if ( !rayTraced ) {
          if ( ( shadowName == "" ) || ( shadowName.substring( 0, 9 ) == "autoshadow" ) ) {
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
      if ( status == MS::kSuccess ) {
        xtraPlug.getValue( bothsides );
      }
      bothSidesEmit = (bothsides == true)? 1.0:0.0;
      MPlug catPlug = lightDepNode.findPlug( "__category", &status );
      if ( status == MS::kSuccess ) catPlug.getValue(lightCategory);
      else lightCategory = "";
      MPlug idPlug = lightDepNode.findPlug( "lightID", &status );
      if ( status == MS::kSuccess ) idPlug.getValue(lightID);
      else lightID = 0;
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

      RtString cat = const_cast< char* >( lightCategory.asChar() );

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

          RtString shadowname = const_cast< char* >( shadowName.asChar() );
          handle = RiLightSource( "liquiddistant",
                                  "intensity",              &intensity,
                                  "lightcolor",             color,
                                  "string shadowname",      &shadowname,
                                  "float shadowfiltersize", rayTraced ? &shadowRadius : &shadowFilterSize,
                                  "float shadowbias",       &shadowBias,
                                  "float shadowsamples",    &shadowSamples,
                                  "color shadowcolor",      &shadowColor,
                                  "float __nondiffuse",     &nonDiffuse,
                                  "float __nonspecular",    &nonSpecular,
                                  "string __category",      &cat,
                                  "float lightID",          &lightID,
                                  RI_NULL );
        } else {
          handle = RiLightSource( "liquiddistant",
                                  "intensity",            &intensity,
                                  "lightcolor",           color,
                                  "float __nondiffuse",   &nonDiffuse,
                                  "float __nonspecular",  &nonSpecular,
                                  "string __category",    &cat,
                                  "float lightID",        &lightID,
                                  RI_NULL );
        }
        break;
      case MRLT_Point:
        if ( liqglo_doShadows && usingShadow ) {
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
                                  "color shadowcolor",          &shadowColor,
                                  "float __nondiffuse",         &nonDiffuse,
                                  "float __nonspecular",        &nonSpecular,
                                  "string __category",          &cat,
                                  "float lightID",              &lightID,
                                  RI_NULL );
        } else {
          handle = RiLightSource( "liquidpoint",
                                  "intensity",            &intensity,
                                  "lightcolor",           color,
                                  "float decay",          &decay,
                                  "float __nondiffuse",   &nonDiffuse,
                                  "float __nonspecular",  &nonSpecular,
                                  "string __category",    &cat,
                                  "float lightID",        &lightID,
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
                                  "intensity",              &intensity,
                                  "lightcolor",             color,
                                  "float coneangle",        &coneAngle,
                                  "float penumbraangle",    &penumbraAngle,
                                  "float dropoff",          &dropOff,
                                  "float decay",            &decay,
                                  "float barndoors",        &barnDoors,
                                  "float leftbarndoor",     &leftBarnDoor,
                                  "float rightbarndoor",    &rightBarnDoor,
                                  "float topbarndoor",      &topBarnDoor,
                                  "float bottombarndoor",   &bottomBarnDoor,
                                  "string shadowname",      &shadowname,
                                  "float shadowfiltersize", rayTraced ? &shadowRadius : &shadowFilterSize,
                                  "float shadowbias",       &shadowBias,
                                  "float shadowsamples",    &shadowSamples,
                                  "color shadowcolor",      &shadowColor,
                                  "float __nondiffuse",     &nonDiffuse,
                                  "float __nonspecular",    &nonSpecular,
                                  "string __category",      &cat,
                                  "float lightID",          &lightID,
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
                                  "string __category",    &cat,
                                  "float lightID",        &lightID,
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
      case MRLT_Area: {
        RtString shadowname = const_cast< char* >( shadowName.asChar() );

        MString coordsys = (lightName+"CoordSys");
        RtString areacoordsys = const_cast< char* >( coordsys.asChar() );

        MString areashader( getenv("LIQUIDHOME") );
        areashader += "/shaders/liquidarea";

        // if raytraced shadows are off, we get a negative value, so we correct it here.
        if ( shadowSamples < 1 ) shadowSamples = 64.0f;

        handle = RiLightSource( const_cast< char* >( areashader.asChar() ),
                                "float intensity",      &intensity,
                                "color lightcolor",     color,
                                "float decay",          &decay,
                                "string coordsys",      &areacoordsys,
                                "float lightsamples",   &shadowSamples,
                                "float doublesided",    &bothSidesEmit,
                                "string shadowname",    &shadowname,
                                "color shadowcolor",    &shadowColor,
                                "string __category",    &cat,
                                "float lightID",        &lightID,
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


MString liqRibLightData::autoShadowName( int PointLightDir ) const
{
  MString frame;
  MString shadowName;
  MString tmpShadowName = LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, liqglo_textureDir, liqglo_projectDir );

  shadowName += tmpShadowName;
  if ( !liqglo_shapeOnlyInShadowNames ) {
    shadowName += liqglo_sceneName;
    shadowName =  parseString( shadowName );
    shadowName += "_";
  }
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


  if ( geometrySet != "" ) {
    shadowName += geometrySet + ".";
  }

  if ( everyFrame ) frame += (int) liqglo_lframe;
  else frame += (int) renderAtFrame;


  if ( liqglo_doExtensionPadding ) {
    while( frame.length() < liqglo_outPadding ) frame = "0" + frame;
  }
  shadowName += frame;
  shadowName += ".tex";

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
  if ( status == MS::kSuccess ) {
    MPlug theShadowCamPlug = shadowCamerasPlug.elementByPhysicalIndex( index, &status );
    MPlugArray shadowCamPlugArray;
    if ( status == MS::kSuccess && theShadowCamPlug.connectedTo( shadowCamPlugArray, true, false ) ) {
      MFnDependencyNode shadowCamDepNode = shadowCamPlugArray[0].node();
      shdCamName = shadowCamDepNode.name();
      MPlug shadowCamParamPlug = shadowCamDepNode.findPlug( "liqDeepShadows", &status );
      if ( status == MS::kSuccess ) shadowCamParamPlug.getValue( shdCamDeepShadows );
      status.clear();
      shadowCamParamPlug = shadowCamDepNode.findPlug( "liqEveryFrame", &status );
      if ( status == MS::kSuccess ) shadowCamParamPlug.getValue( shdCamEveryFrame );
      status.clear();
      shadowCamParamPlug = shadowCamDepNode.findPlug( "liqRenderAtFrame", &status );
      if ( status == MS::kSuccess ) shadowCamParamPlug.getValue( shdCamRenderAtFrame );
      status.clear();
      shadowCamParamPlug = shadowCamDepNode.findPlug( "liqGeometrySet", &status );
      if ( status == MS::kSuccess ) shadowCamParamPlug.getValue( shdCamGeometrySet );

      shadowName += liqglo_sceneName;
      shadowName =  parseString( shadowName );
      shadowName += "_";
      shadowName += shdCamName;
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
    MString err = "Liquid : could not find a shadowCameras attribute on ";
    err += lightShaderNode.name();
    err += " !";
    status.perror( err );
  }



  //cout <<"liqRibLightData::extraShadowName : "<<shadowName.asChar()<<"  ( "<<liqglo_sceneName.asChar()<<" )"<<endl;

  return shadowName;
}



