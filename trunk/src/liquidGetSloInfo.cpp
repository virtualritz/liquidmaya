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
** Liquid Get .slo Info Source 
** ______________________________________________________________________
*/

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
// Entropy Headers
#include <sleargs.h>

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
#include <maya/MCommandResult.h>
#include <maya/MStringArray.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>

#include <liquidGetSloInfo.h>
#include <liquidMemory.h>

extern int debugMode;

int SLEtoSLOMAP[21] = { 0, 3, 2, 1, 11, 12, 13, 4, 5, 7, 6, 8, 10, 0, 0, 0, 5, 7, 6, 8, 10 };
char* shaderTypeStr[14] = { "unknown", 
							"point",
							"color",
							"float",
							"string",
							"surface",
							"light",
							"displacement",
							"volume",
							"transformation",
							"imager",
							"vector",
							"normal",
							"matrix" };

char* shaderDetailStr[3] = { "unknown",
							 "varying",
							 "uniform" };

void* liquidGetSloInfo::creator()
//
//  Description:
//      Create a new instance of the translator
//
{
    return new liquidGetSloInfo();
}

liquidGetSloInfo::~liquidGetSloInfo()
//
//  Description:
//      Class destructor
//
{
} 

MString liquidGetSloInfo::getTypeStr() 
{
	return MString( shaderTypeStr[ shaderType ] );
}

MString liquidGetSloInfo::getArgTypeStr( int num ) 
{ 
	return MString( shaderTypeStr[ ( int )argType[ num ] ] );
}

MString liquidGetSloInfo::getArgDetailStr( int num )
{ 
	return MString( shaderDetailStr[ ( int )argDetail[ num ] ] ); 
}

MString liquidGetSloInfo::getArgStringDefault( int num, int entry ) 
{
	return MString( ( char * )argDefault[ num ] ); 
}

float liquidGetSloInfo::getArgFloatDefault( int num, int entry ) 
{
	float *floats = ( float * )argDefault[ num ];
	return floats[ entry ]; 
}

int liquidGetSloInfo::setShader( MString shaderName )
{
	resetIt();
	MString shaderExtension = shaderName.substring( shaderName.length() - 3, shaderName.length() - 1 );
	MString shaderFileName = shaderName.substring( 0, shaderName.length() - 5 );
	if ( shaderExtension == MString( "slo" ) ) {
		/* Pixar's Photorealistic Renderman Shader */
		char *sloFileName = (char *)alloca(shaderFileName.length() + 1 );
		sprintf(sloFileName, shaderFileName.asChar());
		int err = Slo_SetShader( sloFileName );
		if (err != 0) {
			printf( "Error finding shader %s \n",shaderFileName.asChar() ); 
			resetIt();
			return 0;
		} else {
			shaderName = Slo_GetName();
			shaderType = ( SHADER_TYPE )Slo_GetType();
			numParam = Slo_GetNArgs();
			for ( unsigned k = 1; k <= numParam; k++ ) {
				SLO_VISSYMDEF *arg;
				arg = Slo_GetArgById( k );
				argName.push_back( arg->svd_name );
				argType.push_back( ( SHADER_TYPE )arg->svd_type );
				argArraySize.push_back( arg->svd_arraylen );
				argDetail.push_back( ( SHADER_DETAIL )arg->svd_detail );
				if ( arg->svd_valisvalid ) { 
					switch ( arg->svd_type ) {
					case SLO_TYPE_STRING: {
						char *strings = ( char * )lmalloc( sizeof( char ) * sizeof( arg->svd_default.stringval ) );
						strcpy( strings, arg->svd_default.stringval );
						argDefault.push_back( ( void * )strings );
						break;
										  }
					case SLO_TYPE_SCALAR: {
						if ( arg->svd_arraylen > 0 ) {
							SLO_VISSYMDEF *subarg;
							float *floats = ( float *)lmalloc( sizeof( float ) * arg->svd_arraylen );
							for (int kk = 0; kk < arg->svd_arraylen; kk ++ ) {
								subarg = Slo_GetArrayArgElement(arg, kk);
								floats[kk] = *subarg->svd_default.scalarval;
							}
							argDefault.push_back( ( void * )floats );
						} else {
							float *floats = ( float *)lmalloc( sizeof( float ) * 1 );
							floats[0] = *arg->svd_default.scalarval;
							argDefault.push_back( ( void * )floats );
						}
						break;
										  }
					case SLO_TYPE_COLOR:
					case SLO_TYPE_POINT:
					case SLO_TYPE_VECTOR:
					case SLO_TYPE_NORMAL: {
						float *floats = ( float *)lmalloc( sizeof( float ) * 3 );
						floats[0] = arg->svd_default.pointval->xval;
						floats[1] = arg->svd_default.pointval->yval;
						floats[2] = arg->svd_default.pointval->zval;
						argDefault.push_back( ( void * )floats );
						break;
										  }
					case SLO_TYPE_MATRIX: {
						printf("\"%s\" [%f %f %f %f\n",
							arg->svd_spacename,
							(double) (arg->svd_default.matrixval[0]),
							(double) (arg->svd_default.matrixval[1]),
							(double) (arg->svd_default.matrixval[2]),
							(double) (arg->svd_default.matrixval[3]));
						printf("\t\t\t%f %f %f %f\n",
							(double) (arg->svd_default.matrixval[4]),
							(double) (arg->svd_default.matrixval[5]),
							(double) (arg->svd_default.matrixval[6]),
							(double) (arg->svd_default.matrixval[7]));
						printf("\t\t\t%f %f %f %f\n",
							(double) (arg->svd_default.matrixval[8]),
							(double) (arg->svd_default.matrixval[9]),
							(double) (arg->svd_default.matrixval[10]),
							(double) (arg->svd_default.matrixval[11]));
						printf("\t\t\t%f %f %f %f]\n",
							(double) (arg->svd_default.matrixval[12]),
							(double) (arg->svd_default.matrixval[13]),
							(double) (arg->svd_default.matrixval[14]),
							(double) (arg->svd_default.matrixval[15]));
						break;
										  }
					default: {
						argDefault.push_back( NULL );
						break;
							 }
					}
				}
			}
		}
		Slo_EndShader();
	} else if ( shaderExtension == MString( "sle" ) ) {
		/* Exluna's Entropy Shader */
		char *sleFileName = (char *)alloca(shaderFileName.length() + 1 );
		sprintf(sleFileName, shaderFileName.asChar());
		sleArgs currentShader( sleFileName );
		if ( sleFileName == NULL ) {
			printf( "Error finding shader %s \n",shaderFileName.asChar() ); fflush( stdout );
			resetIt();
			return 0;
		} else {
			shaderName = currentShader.shadername();
			shaderType = ( SHADER_TYPE )SLEtoSLOMAP[ currentShader.shadertype() ];
			numParam = currentShader.nargs();
			for ( unsigned k = 0; k < numParam; k++ ) {
				const sleArgs::Symbol *arg = currentShader.getarg( k );
				argName.push_back( MString( arg->name ) );
				argType.push_back( ( SHADER_TYPE )SLEtoSLOMAP[ arg->type ] );
				argArraySize.push_back( arg->arraylen );

				if ( arg->varying ) {
					argDetail.push_back( SHADER_DETAIL_VARYING );
				} else {
					argDetail.push_back( SHADER_DETAIL_UNIFORM );
				}
				if ( arg->valisvalid() ) { 
					switch ( arg->type ) {
					case sleArgs::TYPE_STRING: {
						char *strings = ( char *)lmalloc( sizeof( char) * (sizeof( arg->stringval(0))+1) );
						strcpy( strings, arg->stringval(0) );
						argDefault.push_back( ( void * )strings );
						break;
											   }
					case sleArgs::TYPE_FLOAT: {
						if ( arg->arraylen > 0 ) {
							float *floats = ( float *)lmalloc( sizeof( float ) * arg->arraylen );
							for (int kk = 0; kk < arg->arraylen; kk ++ ) {
								floats[kk] = arg->floatval(kk);
							}
							argDefault.push_back( ( void * )floats );
						} else {
							float *floats = ( float *)lmalloc( sizeof( float ) * 1 );
							floats[0] = arg->floatval(0);
							argDefault.push_back( ( void * )floats );
						}
						break;
											  }
					case sleArgs::TYPE_COLOR:
					case sleArgs::TYPE_POINT:
					case sleArgs::TYPE_VECTOR:
					case sleArgs::TYPE_NORMAL: {
						float *floats = ( float *)lmalloc( sizeof( float ) * 3 );
						floats[0] = arg->floatval(0);
						floats[1] = arg->floatval(1);
						floats[2] = arg->floatval(2);
						argDefault.push_back( ( void * )floats );
						break;
											   }
					case sleArgs::TYPE_MATRIX: {
						printf( "[%f %f %f %f\n",
							(double) (arg->floatval(0)),
							(double) (arg->floatval(1)),
							(double) (arg->floatval(2)),
							(double) (arg->floatval(3)));
						printf("\t\t\t%f %f %f %f\n",
							(double) (arg->floatval(4)),
							(double) (arg->floatval(5)),
							(double) (arg->floatval(6)),
							(double) (arg->floatval(7)));
						printf("\t\t\t%f %f %f %f\n",
							(double) (arg->floatval(8)),
							(double) (arg->floatval(9)),
							(double) (arg->floatval(10)),
							(double) (arg->floatval(11)));
						printf("\t\t\t%f %f %f %f]\n",
							(double) (arg->floatval(12)),
							(double) (arg->floatval(13)),
							(double) (arg->floatval(14)),
							(double) (arg->floatval(15)));
						break;
											   }
					default: {
						argDefault.push_back( NULL );
						break;
							 }
					}
				}
			}
		}
	}
	return 1;
}

void liquidGetSloInfo::resetIt()
{
	numParam = 0;
	shaderName.clear();
	argName.clear();
	argType.clear();
	argDetail.clear();
	argArraySize.clear();
	int k;
	for ( k = 0; k < argDefault.size(); k++ ) {
		lfree( argDefault[k] );
	}
}

MStatus	liquidGetSloInfo::doIt( const MArgList& args )
{
	
	MStatus					status;
	unsigned				i;
	
	try {
		if ( args.length() < 2 ) throw( "Not enough arguments specified for liquidGetSloInfo!\n" );
		MString shaderFileName = args.asString( args.length() - 1, &status );
		int success = setShader( shaderFileName );
		if ( !success ) throw( "Error loading shader specified for liquidGetSloInfo!\n" );
		for ( i = 0; i < args.length() - 1; i++ ) {
			if ( MString( "-name" ) == args.asString( i, &status ) )  {
				setResult( getName() );
			}
			if ( MString( "-type" ) == args.asString( i, &status ) )  {
				setResult( getTypeStr() );
			}
			if ( MString( "-numParam" ) == args.asString( i, &status ) )  {
				setResult( getNumParam() );
			}
			if ( MString( "-argName" ) == args.asString( i, &status ) )  {
				i++;
				int argNum = args.asInt( i, &status );
				setResult( getArgName( argNum ) );
			}
			if ( MString( "-argType" ) == args.asString( i, &status ) )  {
				i++;
				int argNum = args.asInt( i, &status );
				setResult( getArgTypeStr( argNum ) );
			}
			if ( MString( "-argArraySize" ) == args.asString( i, &status ) )  {
				i++;
				int argNum = args.asInt( i, &status );
				setResult( getArgArraySize( argNum ) );
			}
			if ( MString( "-argDetail" ) == args.asString( i, &status ) )  {
				i++;
				int argNum = args.asInt( i, &status );
				setResult( getArgDetailStr( argNum ) );
			}
			if ( MString( "-argDefault" ) == args.asString( i, &status ) )  {
				i++;
				int argNum = args.asInt( i, &status );
				MStringArray defaults;
				switch ( getArgType( argNum ) ) {
					case SHADER_TYPE_STRING: {
						defaults.append( getArgStringDefault( argNum, 0 ) );
						break;
											 }
					case SHADER_TYPE_SCALAR: {
						if ( getArgArraySize( argNum ) > 0 ) {
							for ( int kk = 0; kk < getArgArraySize( argNum ); kk++ ) {
								char defaultTmp[256];
								sprintf( defaultTmp, "%f", getArgFloatDefault( argNum, kk ) );
								defaults.append( defaultTmp );
							}
						} else {
							char defaultTmp[256];
							sprintf( defaultTmp, "%f", getArgFloatDefault( argNum, 0 ) );
							defaults.append( defaultTmp );
						}
						break;
											 }
					case SHADER_TYPE_COLOR:
					case SHADER_TYPE_POINT:
					case SHADER_TYPE_VECTOR:
					case SHADER_TYPE_NORMAL: {
							char defaultTmp[256];
							sprintf( defaultTmp, "%f", getArgFloatDefault( argNum, 0 ) );
							defaults.append( defaultTmp );
							sprintf( defaultTmp, "%f", getArgFloatDefault( argNum, 1 ) );
							defaults.append( defaultTmp );
							sprintf( defaultTmp, "%f", getArgFloatDefault( argNum, 2 ) );
							defaults.append( defaultTmp );
						break;
											   }
					default: {
						defaults.append( MString( "unknown" ) );
						break;
							 }
				}
				setResult( defaults );
			}
			if ( MString( "-argExists" ) == args.asString( i, &status ) )  {
				i++;
				MString name = args.asString( i, &status );
				for ( unsigned k = 0; k < getNumParam(); k++ ) {
					if ( argName[k] == name ) {
						setResult( true );
					} else {
						setResult( false );
					}
				}
			}
		}
	} catch ( MString errorMessage ) {
		resetIt();
		MGlobal::displayError( errorMessage );
	} catch ( ... ) {
		resetIt();
		cerr << "liquidGetSloInfo: Unknown exception thrown\n" << endl;
	}
	resetIt();
	return MS::kSuccess; 
};
