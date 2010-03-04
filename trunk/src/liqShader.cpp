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
#include <maya/MGlobal.h>

#include <liquid.h>
#include <liqShader.h>
#include <liqGlobalHelpers.h>

extern int debugMode;

liqShader::liqShader()
{
  //numTPV                = 0;
  name                  = "";
  file                  = "";
  rmColor[0]            = 1.0;
  rmColor[1]            = 1.0;
  rmColor[2]            = 1.0;
  rmOpacity[0]          = 1.0;
  rmOpacity[1]          = 1.0;
  rmOpacity[2]          = 1.0;
  hasShadingRate        = false;
  shadingRate           = 1.0;
  hasDisplacementBound  = false;
  displacementBound     = 0.0;
  outputInShadow        = false;
  hasErrors             = false;
  shader_type           = SHADER_TYPE_UNKNOWN;
  shaderSpace           = "";
  dirtyAtEveryFrame     = 0;
  tokenPointerArray.push_back( liqTokenPointer() ); // ENsure we have a 0 element
}

liqShader::liqShader( const liqShader& src )
{
  //numTPV               = src.numTPV;
  tokenPointerArray    = src.tokenPointerArray;
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
  outputInShadow       = src.outputInShadow;
  hasErrors            = src.hasErrors;
  shader_type          = src.shader_type;
  shaderSpace          = src.shaderSpace;
  dirtyAtEveryFrame    = src.dirtyAtEveryFrame;
}


liqShader::liqShader( MObject shaderObj )
{
	MString rmShaderStr;
	MStatus status;

	MFnDependencyNode shaderNode( shaderObj );
	MPlug rmanShaderNamePlug = shaderNode.findPlug( MString( "rmanShaderLong" ) );
	rmanShaderNamePlug.getValue( rmShaderStr );

	LIQDEBUGPRINTF( "-> Using Renderman Shader %s. \n", rmShaderStr.asChar() );

	unsigned numArgs;
	//numTPV = 0;
	hasShadingRate = false;
	hasDisplacementBound = false;
	outputInShadow = false;
	hasErrors = false;
	tokenPointerArray.push_back( liqTokenPointer() );

	// if this shader instance isn't currently used already then load it into the
	// lookup set it as my slo lookup
	name = shaderNode.name().asChar();
	file = rmShaderStr.substring( 0, rmShaderStr.length() - 5 ).asChar();

	rmColor[0]            = 1.0;
	rmColor[1]            = 1.0;
	rmColor[2]            = 1.0;
	rmOpacity[0]          = 1.0;
	rmOpacity[1]          = 1.0;
	rmOpacity[2]          = 1.0;

	liqGetSloInfo shaderInfo;

// commented out for it generates errors - Alf
	int success = ( shaderInfo.setShaderNode( shaderNode ) );
	if ( !success )
	{
		liquidMessage( "Problem using shader '" + string( shaderNode.name().asChar() ) + "'", messageError );
		rmColor[0] = 1.0;
		rmColor[1] = 0.0;
		rmColor[2] = 0.0;
		name = "plastic";
//		numTPV = 0;
		hasErrors = true;
	}
	else
	{
		/* Used to handling shading rates set in the surface shader,
		this is a useful way for shader writers to ensure that their
		shaders are always rendered as they were designed.  This value
		overrides the global shading rate but gets overridden with the
		node specific shading rate. */

		shader_type = shaderInfo.getType();

		// Set RiColor and RiOpacity
		status.clear();
		MPlug colorPlug = shaderNode.findPlug( "color" );
		if ( MS::kSuccess == status )
		{
			colorPlug.child(0).getValue( rmColor[0] );
			colorPlug.child(1).getValue( rmColor[1] );
			colorPlug.child(2).getValue( rmColor[2] );
		}

		status.clear();
		MPlug opacityPlug( shaderNode.findPlug( "opacity" ) );
		// Moritz: changed opacity from float to color in MEL
		if ( MS::kSuccess == status )
		{
		  opacityPlug.child(0).getValue( rmOpacity[0] );
		  opacityPlug.child(1).getValue( rmOpacity[1] );
		  opacityPlug.child(2).getValue( rmOpacity[2] );
		}

		status.clear();
		MPlug shaderSpacePlug( shaderNode.findPlug( "shaderSpace" ) );
		if ( MS::kSuccess == status )
		{
			shaderSpacePlug.getValue( shaderSpace );
		}

		status.clear();
		MPlug outputInShadowPlug( shaderNode.findPlug( "outputInShadow" ) );
		if ( MS::kSuccess == status )
		{
			outputInShadowPlug.getValue( outputInShadow );
		}

		status.clear();
		MPlug dirtyAtEveryFramePlug( shaderNode.findPlug( "dirtyAtEveryFrame" ) );
		if ( MS::kSuccess == status )
		{
			dirtyAtEveryFramePlug.getValue( dirtyAtEveryFrame );
		}

		// find the parameter details and declare them in the rib stream
		numArgs = shaderInfo.getNumParam();
		for (unsigned int i( 0 ); i < numArgs; i++ )
		{
			if ( shaderInfo.getArgName( i ) == "liquidShadingRate" )
			{
				// BUGFIX: Monday 6th August - fixed shading rate bug where it only accepted the default value
				MPlug floatPlug = shaderNode.findPlug( shaderInfo.getArgName( i ), &status );
				if ( MS::kSuccess == status )
				{
					float floatPlugVal;
					floatPlug.getValue( floatPlugVal );
					shadingRate = floatPlugVal;
				}
				else
					shadingRate = shaderInfo.getArgFloatDefault( i, 0 );
				
				hasShadingRate = true;
				continue;
			}
			switch ( shaderInfo.getArgDetail(i) )
			{
				case SHADER_DETAIL_UNIFORM:
				{
					tokenPointerArray.rbegin()->setDetailType( rUniform );
					break;
				}
				case SHADER_DETAIL_VARYING:
				{
					tokenPointerArray.rbegin()->setDetailType( rVarying);
					break;
				}
				case SHADER_DETAIL_UNKNOWN:
					tokenPointerArray.rbegin()->setDetailType( rUniform);
					break;
			}
			switch ( shaderInfo.getArgType( i ) )
			{
				case SHADER_TYPE_STRING:
				{
					MPlug stringPlug = shaderNode.findPlug( shaderInfo.getArgName( i ), &status );
					if ( MS::kSuccess == status )
					{
						

						unsigned int arraySize( shaderInfo.getArgArraySize( i ) );
						if ( arraySize > 0 ) 
						{
							bool isArrayAttr( stringPlug.isArray( &status ) );
							if ( isArrayAttr )
							{
								MPlug plugObj;
								tokenPointerArray.rbegin()->set( shaderInfo.getArgName( i ).asChar(), rString, arraySize );
								for( unsigned int kk( 0 ); kk < arraySize; kk++ )
								{
									plugObj = stringPlug.elementByLogicalIndex( kk, &status );
									if ( MS::kSuccess == status )
									{
										MString stringPlugVal;
										plugObj.getValue( stringPlugVal );
										MString stringVal = parseString( stringPlugVal );
										tokenPointerArray.rbegin()->setTokenString( kk, stringVal.asChar() );
									}
								}
								tokenPointerArray.push_back( liqTokenPointer() );
							}
						}
						else
						{
							MString stringPlugVal;
							stringPlug.getValue( stringPlugVal );
							MString stringDefault( shaderInfo.getArgStringDefault( i, 0 ) );
							if ( stringPlugVal != stringDefault )
							{
								MString stringVal( parseString( stringPlugVal ) );
								LIQDEBUGPRINTF("[liqShader::liqShader] parsed string for param %s = %s \n", shaderInfo.getArgName( i ).asChar(), stringVal.asChar() );
								tokenPointerArray.rbegin()->set( shaderInfo.getArgName( i ).asChar(), rString );
								tokenPointerArray.rbegin()->setTokenString( 0, stringVal.asChar() );
								tokenPointerArray.push_back( liqTokenPointer() );
							}
						}
					}
					break;
				}
				case SHADER_TYPE_SCALAR:
				{
					MPlug floatPlug( shaderNode.findPlug( shaderInfo.getArgName( i ), &status ) );
					if ( MS::kSuccess == status )
					{
						unsigned arraySize( shaderInfo.getArgArraySize( i ) );
						if ( arraySize )
						{
							bool isArrayAttr( floatPlug.isArray( &status ) );
							if ( isArrayAttr )
							{
								// philippe : new way to store float arrays as multi attr
								MPlug plugObj;
								tokenPointerArray.rbegin()->set( shaderInfo.getArgName( i ).asChar(), rFloat, false, true, arraySize );
								for( unsigned kk( 0 ); kk < arraySize; kk++ )
								{
									plugObj = floatPlug.elementByLogicalIndex( kk, &status );
									if ( MS::kSuccess == status )
									{
										float x;
										plugObj.getValue( x );
										tokenPointerArray.rbegin()->setTokenFloat( kk, x );
									}
								}
							}
							else
							{
								// philippe : old way to store float arrays as floatArray attr
								MObject plugObj;
								floatPlug.getValue( plugObj );
								MFnDoubleArrayData fnDoubleArrayData( plugObj );
								const MDoubleArray& doubleArrayData( fnDoubleArrayData.array( &status ) );
								// Hmmmmmmm Really a uArray ?
								tokenPointerArray.rbegin()->set( shaderInfo.getArgName( i ).asChar(), rFloat, false, true, arraySize );
								for( unsigned kk( 0 ); kk < arraySize; kk++ )
								{
									tokenPointerArray.rbegin()->setTokenFloat( kk, ( float )doubleArrayData[ kk ] );
								}

							}
						}
						else
						{
							float floatPlugVal;
							floatPlug.getValue( floatPlugVal );
							tokenPointerArray.rbegin()->set( shaderInfo.getArgName( i ).asChar(), rFloat );
							tokenPointerArray.rbegin()->setTokenFloat( 0, floatPlugVal );
						}
						tokenPointerArray.push_back( liqTokenPointer() );
					}
					break;
				}
				case SHADER_TYPE_COLOR:
				{
					unsigned int arraySize( shaderInfo.getArgArraySize( i ) );
					if ( arraySize > 0 )
					{
						//fprintf( stderr, "Got %i colors in array !\n", arraySize );
						status = liqShaderParseVectorArrayAttr( shaderNode, shaderInfo.getArgName( i ).asChar(), rColor, arraySize );
						//fprintf( stderr, "Done with color array !\n", arraySize );
					} else
						status = liqShaderParseVectorAttr( shaderNode, shaderInfo.getArgName( i ).asChar(), rColor );
					break;
				}
				case SHADER_TYPE_POINT:
				{
					unsigned int arraySize( shaderInfo.getArgArraySize( i ) );
					if ( arraySize > 0 )
						status = liqShaderParseVectorArrayAttr( shaderNode, shaderInfo.getArgName( i ).asChar(), rPoint, arraySize );
					else
						status = liqShaderParseVectorAttr( shaderNode,  shaderInfo.getArgName( i ).asChar(), rPoint );
					break;
				}
				case SHADER_TYPE_VECTOR:
				{
					unsigned int arraySize( shaderInfo.getArgArraySize( i ) );
					if ( arraySize > 0 )
						status = liqShaderParseVectorArrayAttr( shaderNode, shaderInfo.getArgName( i ).asChar(), rVector, arraySize );
					else
						status = liqShaderParseVectorAttr( shaderNode,  shaderInfo.getArgName( i ).asChar(), rVector );
					break;
				}
				case SHADER_TYPE_NORMAL:
				{
					unsigned int arraySize( shaderInfo.getArgArraySize( i ) );
					if ( arraySize > 0 )
						status = liqShaderParseVectorArrayAttr( shaderNode, shaderInfo.getArgName( i ).asChar(), rNormal, arraySize );
					else
						status = liqShaderParseVectorAttr( shaderNode,  shaderInfo.getArgName( i ).asChar(), rNormal );
					break;
				}
				case SHADER_TYPE_MATRIX:
				{
					liquidMessage( "WHAT IS THE MATRIX!", messageError );
					break;
				}
				case SHADER_TYPE_UNKNOWN :
				default:
					liquidMessage( "Unknown shader type", messageError );
					break;
			}
		}
	}
	shaderInfo.resetIt();
}

MStatus liqShader::liqShaderParseVectorAttr ( const MFnDependencyNode& shaderNode, const string& argName, ParameterType pType )
{
	MStatus status( MS::kSuccess );

	MPlug triplePlug( shaderNode.findPlug( argName.c_str(), &status ) );

	if ( MS::kSuccess == status )
	{
		float x, y, z;
		tokenPointerArray.rbegin()->set( argName.c_str(), pType );
		triplePlug.child( 0 ).getValue( x );
		triplePlug.child( 1 ).getValue( y );
		triplePlug.child( 2 ).getValue( z );
		tokenPointerArray.rbegin()->setTokenFloat( 0, x, y, z );

		tokenPointerArray.push_back( liqTokenPointer() );
	}
  return status;
}

// philippe : multi attr support
MStatus liqShader::liqShaderParseVectorArrayAttr ( const MFnDependencyNode& shaderNode, const string& argName, ParameterType pType, unsigned int arraySize )
{
  MStatus status( MS::kSuccess );

  MPlug triplePlug( shaderNode.findPlug( argName.c_str(), true, &status ) );

  if ( MS::kSuccess == status ) {
    tokenPointerArray.rbegin()->set( argName, pType, false, true, arraySize );
    for( unsigned int kk( 0 ); kk < arraySize; kk++ ) {
      MPlug argNameElement( triplePlug.elementByLogicalIndex( kk ) );

      if ( MS::kSuccess == status ) {
        float x, y, z;
        argNameElement.child( 0 ).getValue( x );
        argNameElement.child( 1 ).getValue( y );
        argNameElement.child( 2 ).getValue( z );
        tokenPointerArray.rbegin()->setTokenFloat( kk, x, y, z );
      }
    }
    tokenPointerArray.push_back( liqTokenPointer() );
  }

  return status;
}


liqShader & liqShader::operator=( const liqShader & src )
{
  //numTPV = src.numTPV;
  tokenPointerArray     = src.tokenPointerArray;
  name                  = src.name;
  file                  = src.file;
  rmColor[0]            = src.rmColor[0];
  rmColor[1]            = src.rmColor[1];
  rmColor[2]            = src.rmColor[2];
  rmOpacity[0]          = src.rmOpacity[0];
  rmOpacity[1]          = src.rmOpacity[1];
  rmOpacity[2]          = src.rmOpacity[2];
  hasShadingRate        = src.hasShadingRate;
  shadingRate           = src.shadingRate;
  hasDisplacementBound  = src.hasDisplacementBound;
  displacementBound     = src.displacementBound;
  outputInShadow        = src.outputInShadow;
  hasErrors             = src.hasErrors;
  shader_type           = src.shader_type;
  shaderSpace           = src.shaderSpace;
  return *this;
}
