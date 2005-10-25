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

#include <maya/MPlug.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnDoubleArrayData.h>

#include <liquid.h>
#include <liqShader.h>
#include <liqGlobalHelpers.h>

extern int debugMode;


liqShader::liqShader()
{
  numTPV = 0;
  name = "";
  file = "";
  hasShadingRate = false;
  shadingRate = 1.0;
  hasDisplacementBound = false;
  displacementBound = 0.0;
  hasErrors = false;
  shader_type = SHADER_TYPE_UNKNOWN;
}

liqShader::liqShader( const liqShader & src )
{
  numTPV = src.numTPV;
  for( unsigned int i = 0; i < numTPV; i++ ) {
    tokenPointerArray[i] = src.tokenPointerArray[i];
  }
  name                 = src.name;
  file                 = src.file;
  rmColor[0]           = src.rmColor[0];
  rmColor[1]           = src.rmColor[1];
  rmColor[2]           = src.rmColor[2];
  rmOpacity[0]         = src.rmOpacity[0];
  rmOpacity[1]         = src.rmOpacity[1];
  rmOpacity[2]         = src.rmOpacity[2];
  hasShadingRate       = src.hasShadingRate;
  shadingRate          = src.shadingRate;
  hasDisplacementBound = src.hasDisplacementBound;
  displacementBound    = src.displacementBound;
  hasErrors            = src.hasErrors;
  shader_type          = src.shader_type;
}


liqShader::liqShader( MObject shaderObj )
{
  MString rmShaderStr;
  MStatus status;

  MFnDependencyNode shaderNode( shaderObj );
  MPlug rmanShaderNamePlug = shaderNode.findPlug( MString( "rmanShaderLong" ) );
  rmanShaderNamePlug.getValue( rmShaderStr );

  LIQDEBUGPRINTF( "-> Using Renderman Shader %s. \n", rmShaderStr.asChar() );

  int numArgs;
  numTPV = 0;
  hasShadingRate = false;
  hasDisplacementBound = false;
  hasErrors = false;

  // if this shader instance isn't currently used already then load it into the
  // lookup set it as my slo lookup
  name = shaderNode.name().asChar();
  file = rmShaderStr.substring( 0, rmShaderStr.length() - 5 ).asChar();

  liqGetSloInfo shaderInfo;
  int success = shaderInfo.setShader( rmShaderStr );
  if ( !success ) {
    fprintf(stderr, "Error Using Shader %s!\n", shaderNode.name().asChar() );
    rmColor[0] = 1.0;
    rmColor[1] = 0.0;
    rmColor[2] = 0.0;
    name = "plastic";
    numTPV = 0;
    hasErrors = true;
  } else {
    /* Used to handling shading rates set in the surface shader,
    this is a useful way for shader writers to ensure that their
    shaders are always rendered as they were designed.  This value
    overrides the global shading rate but gets overridden with the
    node specific shading rate. */

    shader_type = shaderInfo.getType();
    // Set RiColor and RiOpacity
    MPlug colorPlug = shaderNode.findPlug( "color" );

    colorPlug.child(0).getValue( rmColor[0] );
    colorPlug.child(1).getValue( rmColor[1] );
    colorPlug.child(2).getValue( rmColor[2] );

    MPlug opacityPlug = shaderNode.findPlug( "opacity" );

    // Moritz: changed opacity from float to color in MEL
    opacityPlug.child(0).getValue( rmOpacity[0] );
    opacityPlug.child(1).getValue( rmOpacity[1] );
    opacityPlug.child(2).getValue( rmOpacity[2] );

    // Moritz: below code is obsolete as opacity is a color now
    //double opacityVal;
    //opacityPlug.getValue( opacityVal );
    //rmOpacity[0] = RtFloat( opacityVal );
    //rmOpacity[1] = RtFloat( opacityVal );
    //rmOpacity[2] = RtFloat( opacityVal );

    // find the parameter details and declare them in the rib stream
    numArgs = shaderInfo.getNumParam();
    for (unsigned int i = 0; i < numArgs; i++ ) {
      if ( shaderInfo.getArgName( i ) == "liquidShadingRate" ) {
        // BUGFIX: Monday 6th August - fixed shading rate bug where it only accepted the default value
        MPlug floatPlug = shaderNode.findPlug( shaderInfo.getArgName( i ), &status );
        if ( status == MS::kSuccess ) {
          float floatPlugVal;
          floatPlug.getValue( floatPlugVal );
          shadingRate = floatPlugVal;
        } else {
          shadingRate = shaderInfo.getArgFloatDefault( i, 0 );
        }
        hasShadingRate = true;
        continue;
      }
      switch ( shaderInfo.getArgDetail(i) ) {
      case SHADER_DETAIL_UNIFORM: {
        tokenPointerArray[numTPV].setDetailType( rUniform );
        break;
      }
      case SHADER_DETAIL_VARYING: {
        tokenPointerArray[numTPV].setDetailType( rVarying);
        break;
      }
      case SHADER_DETAIL_UNKNOWN:
        tokenPointerArray[numTPV].setDetailType( rUniform);
        break;
      }
      switch ( shaderInfo.getArgType( i ) ) {
      case SHADER_TYPE_STRING: {
        MPlug stringPlug = shaderNode.findPlug( shaderInfo.getArgName( i ), &status );
        if ( status == MS::kSuccess ) {
          MString stringPlugVal;
          stringPlug.getValue( stringPlugVal );
          MString stringDefault = shaderInfo.getArgStringDefault( i, 0 );
          if ( stringPlugVal != stringDefault ) {
            MString stringVal = parseString( stringPlugVal );
            tokenPointerArray[ numTPV ].set( shaderInfo.getArgName( i ).asChar(), rString, false, false, false, 0 );
            tokenPointerArray[ numTPV ].setTokenString( stringVal.asChar(), stringVal.length() );
            numTPV++;
          }
        }
        break;
      }
      case SHADER_TYPE_SCALAR: {
        MPlug floatPlug = shaderNode.findPlug( shaderInfo.getArgName( i ), &status );
        if ( status == MS::kSuccess ) {
          unsigned int arraySize = shaderInfo.getArgArraySize( i );
          if ( arraySize > 0 ) {

            bool isArrayAttr = floatPlug.isArray( &status );

            if ( isArrayAttr ) {

              // philippe : new way to store float arrays as multi attr

              MPlug plugObj;
              tokenPointerArray[ numTPV ].set( shaderInfo.getArgName( i ).asChar(), rFloat, false, false, true, arraySize );

              for( unsigned int kk = 0; kk < arraySize; kk++ ) {

                plugObj = floatPlug.elementByLogicalIndex( kk, &status );

                if ( status == MS::kSuccess ) {
                  float x;
                  plugObj.getValue(x);
                  tokenPointerArray[numTPV].setTokenFloat( kk, x  );
                }

              }

            } else {

              // philippe : old way to store float arrays as floatArray attr

              MObject plugObj;
              floatPlug.getValue( plugObj );
              MFnDoubleArrayData  fnDoubleArrayData( plugObj );
              MDoubleArray doubleArrayData = fnDoubleArrayData.array( &status );
              // Hmmmmmmm Really a uArray ?
              tokenPointerArray[ numTPV ].set( shaderInfo.getArgName( i ).asChar(), rFloat, false, false, true, arraySize );
              for( unsigned int kk = 0; kk < arraySize; kk++ ) {
                tokenPointerArray[numTPV].setTokenFloat( kk, doubleArrayData[kk] );
              }

            }
          } else {
            float floatPlugVal;
            floatPlug.getValue( floatPlugVal );
            tokenPointerArray[ numTPV ].set( shaderInfo.getArgName( i ).asChar(), rFloat, false, false, false, 0 );
            tokenPointerArray[ numTPV ].setTokenFloat( 0, floatPlugVal );
          }
          numTPV++;
        }
        break;
      }
      case SHADER_TYPE_COLOR: {
        unsigned int arraySize = shaderInfo.getArgArraySize( i );
        if ( arraySize > 0 ) {
          //fprintf( stderr, "Got %i colors in array !\n", arraySize );
          status = liqShaderParseVectorArrayAttr( shaderNode, shaderInfo.getArgName( i ).asChar(), rColor, arraySize );
          //fprintf( stderr, "Done with color array !\n", arraySize );
        } else {
          status = liqShaderParseVectorAttr( shaderNode, shaderInfo.getArgName( i ).asChar(), rColor );
        }
        break;
      }
      case SHADER_TYPE_POINT: {
        unsigned int arraySize = shaderInfo.getArgArraySize( i );
        if ( arraySize > 0 ) {
          status = liqShaderParseVectorArrayAttr( shaderNode, shaderInfo.getArgName( i ).asChar(), rPoint, arraySize );
        } else {
          status = liqShaderParseVectorAttr( shaderNode,  shaderInfo.getArgName( i ).asChar(), rPoint );
        }
        break;
      }
      case SHADER_TYPE_VECTOR: {
        unsigned int arraySize = shaderInfo.getArgArraySize( i );
        if ( arraySize > 0 ) {
          status = liqShaderParseVectorArrayAttr( shaderNode, shaderInfo.getArgName( i ).asChar(), rVector, arraySize );
        } else {
          status = liqShaderParseVectorAttr( shaderNode,  shaderInfo.getArgName( i ).asChar(), rVector );
        }
        break;
      }
      case SHADER_TYPE_NORMAL: {
        unsigned int arraySize = shaderInfo.getArgArraySize( i );
        if ( arraySize > 0 ) {
          status = liqShaderParseVectorArrayAttr( shaderNode, shaderInfo.getArgName( i ).asChar(), rNormal, arraySize );
        } else {
          status = liqShaderParseVectorAttr( shaderNode,  shaderInfo.getArgName( i ).asChar(), rNormal );
        }
        break;
      }
      case SHADER_TYPE_MATRIX: {
        printf( "WHAT IS THE MATRIX!\n" );
        break;
      }
      case SHADER_TYPE_UNKNOWN :
      default:
        printf("Unknown\n");
        break;
      }
    }
  }
  shaderInfo.resetIt();
}

MStatus liqShader::liqShaderParseVectorAttr ( MFnDependencyNode & shaderNode, const char * argName, ParameterType pType )
{
  MStatus status = MS::kSuccess;
  MPlug triplePlug = shaderNode.findPlug( argName, &status );
  if ( status == MS::kSuccess ) {
    float x, y, z;
    tokenPointerArray[ numTPV ].set( argName, pType, false, false, false, 0 );
    triplePlug.child(0).getValue( x );
    triplePlug.child(1).getValue( y );
    triplePlug.child(2).getValue( z );
    tokenPointerArray[ numTPV ].setTokenFloat( 0, x, y, z );
    numTPV++;
  }
  return status;
}

// philippe : multi attr support
MStatus liqShader::liqShaderParseVectorArrayAttr ( MFnDependencyNode & shaderNode, const char * argName, ParameterType pType, unsigned int arraySize )
{
  MStatus status = MS::kSuccess;
  tokenPointerArray[ numTPV ].set( argName, pType, false, false, true, arraySize );
  MPlug triplePlug;

  triplePlug = shaderNode.findPlug( argName, true, &status );

  for( unsigned int kk = 0; kk < arraySize; kk++ ) {

    MPlug argNameElement = triplePlug.elementByLogicalIndex( kk );

    if ( status == MS::kSuccess ) {

      float x, y, z;
      argNameElement.child(0).getValue( x );
      argNameElement.child(1).getValue( y );
      argNameElement.child(2).getValue( z );
      tokenPointerArray[numTPV].setTokenFloat( kk, x, y, z );
    }

  }

  numTPV++;

  return status;
}


liqShader & liqShader::operator=( const liqShader & src )
{
  freeShader();
  numTPV = src.numTPV;
  for( unsigned int i = 0; i < numTPV; i++ ) {
    tokenPointerArray[i] = src.tokenPointerArray[i];
  }
  name = src.name;
  file = src.file;
  rmColor[0] = src.rmColor[0];
  rmColor[1] = src.rmColor[1];
  rmColor[2] = src.rmColor[2];
  rmOpacity[0] = src.rmOpacity[0];
  rmOpacity[1] = src.rmOpacity[1];
  rmOpacity[2] = src.rmOpacity[2];
  hasShadingRate = src.hasShadingRate;
  shadingRate = src.shadingRate;
  hasDisplacementBound = src.hasDisplacementBound;
  displacementBound = src.displacementBound;
  hasErrors = src.hasErrors;
  shader_type = shader_type;
  return *this;
}

liqShader::~liqShader()
{
  freeShader();
}

void liqShader::freeShader( )
{
  unsigned int k = 0;
  while ( k < numTPV ) {
    tokenPointerArray[k].reset();
    k++;
  }
  numTPV = 0;
}
