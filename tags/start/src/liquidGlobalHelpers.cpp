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
** Liquid Global Helpers
** ______________________________________________________________________
*/

// Standard Headers
#include <time.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>

// Renderman Headers
extern "C" {
#include <ri.h>
}

#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif

#include <vector>

// Maya's Headers
#include <maya/MPxCommand.h>
#include <maya/MPlug.h>
#include <maya/MFnDagNode.h>
#include <maya/MStringArray.h>
#include <maya/MFnAttribute.h>
#include <maya/MIntArray.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MFnDoubleArrayData.h>

#include <liquid.h>
#include <liquidGlobalHelpers.h>
#include <liquidMemory.h>
#include <liquidGetSloInfo.h>

extern long			lframe;
extern bool     expandShaderArrays;
extern int debugMode;
extern bool liquidBin;
extern MString sceneName;
extern MString ribDir;
extern MString projectDir;
extern MStringArray DDimageName;

extern std::vector<shaderStruct> shaders;

void liquidInfo( MString info )
//
// Description:
// Standard function to send messages to either the 
// maya console or the shell for user feedback	
//
{
	if ( !liquidBin ) {
		MString infoOutput( "Liquid: " );
		infoOutput += info;
		MGlobal::displayInfo( infoOutput );
	} else {
		std::cout << "Liquid: " << info.asChar() << "\n" << std::flush;
	}
}

MStringArray FindAttributesByPrefix(const char* pPrefix, MFnDependencyNode& NodeFn )
// 
//  Description:
//	Check to see if the node NodeFn has any attributes starting with pPrefix and store those
//	in Matches to return
//
{
    MStringArray Matches;
	
    for( int i = 0; i < NodeFn.attributeCount(); i++ )
    {
        MFnAttribute AttributeFn = NodeFn.attribute(i);
        MString AttributeName = AttributeFn.name();
        if (!strncmp(AttributeName.asChar(), pPrefix, strlen(pPrefix) ))
            Matches.append(AttributeName);
    }
    return Matches;
}

bool isObjectTwoSided( const MDagPath & path )
//  
//  Description:
//      Check if the given object is visible
//  
{
	MStatus status;
    MFnDagNode fnDN( path );
    MPlug dPlug = fnDN.findPlug( "doubleSided", &status );
    bool doubleSided = true;
	if ( status == MS::kSuccess ) {
		dPlug.getValue( doubleSided );
	}
    return  doubleSided;
}


bool isObjectVisible( const MDagPath & path )
//  
//  Description:
//      Check if the given object is visible
//  
{
	MStatus status;
    MFnDagNode fnDN( path );
    // Check the visibility attribute of the node
    //
    if ( debugMode ) { printf("-> checking visibility attribute\n"); }
    MPlug vPlug = fnDN.findPlug( "visibility", &status );
    if ( debugMode ) { printf("-> checking visibility setting\n"); }
	bool visible = true;
	if ( status == MS::kSuccess ) {
		vPlug.getValue( visible );
	}
	status.clear();
    if ( debugMode ) { printf("-> done checking visibility attribute\n"); }
    // Also check to see if the node is an intermediate object in
    // a computation.  For example, it could be in the middle of a 
    // chain of deformations.  Intermediate objects are not visible.
    //
    if ( debugMode ) { printf("-> checking intermediate object\n"); }
    MPlug iPlug = fnDN.findPlug( "intermediateObject", &status );
    bool intermediate = false;
	if ( status == MS::kSuccess ) {
		iPlug.getValue( intermediate );
	}
	status.clear();
    if ( debugMode ) { printf("-> done checking intermediate object\n"); }
	
    return  visible && !intermediate;
}

bool isObjectPrimaryVisible( const MDagPath & path )
//  
//  Description:
//      Check if the given object is visible
//  
{
	MStatus status;
    MFnDagNode fnDN( path );
	MObject obj = path.node();
    if ( debugMode ) { printf("-> checking overrideEnabled\n"); }
	status.clear();
	MPlug oPlug = fnDN.findPlug( MString( "overrideEnabled" ), &status );
	bool isOver = false;
	if ( status == MS::kSuccess ) {
		oPlug.getValue( isOver );
	}
    if ( debugMode ) { printf("-> done checking overrideEnabled\n"); }
	status.clear();
    MPlug vPlug = fnDN.findPlug( MString( "primaryVisibility" ), &status );
    bool primaryVisibility = true;
	if ( status == MS::kSuccess ) {
		vPlug.getValue( primaryVisibility );
	}
	if ( primaryVisibility && isOver ) {
		status.clear();
		MPlug oPlug = fnDN.findPlug( MString( "overrideVisibility" ), &status );
		if ( status == MS::kSuccess ) {
			oPlug.getValue( primaryVisibility );
		}
	}
    return  primaryVisibility;
}

bool isObjectTemplated( const MDagPath & path )
//  
//  Description:
//      Check if the given object is visible
//  
{
	MStatus status;
    MFnDagNode fnDN( path );
    MPlug vPlug = fnDN.findPlug( "template", &status );
    bool templated = false;
	if ( status == MS::kSuccess ) {
		vPlug.getValue( templated );
	}
    status.clear();
    return  templated;
}

bool isObjectCastsShadows( const MDagPath & path )
//  
//  Description:
//      Check if the given object is visible
//  
{
	MStatus status;
    MFnDagNode fnDN( path );
    // Check the visibility attribute of the node
    //
    MPlug vPlug = fnDN.findPlug( MString( "castsShadows" ), &status );
    bool castsShadows = true;
	if ( status == MS::kSuccess ) {
		vPlug.getValue( castsShadows );
	}
	status.clear();
	MPlug oPlug = fnDN.findPlug( MString( "overrideEnabled" ), &status );
	bool isOver = false;
	if ( status == MS::kSuccess ) {
		oPlug.getValue( isOver );
	}
	status.clear();
	if ( castsShadows && isOver ) {
		MPlug oPlug = fnDN.findPlug( MString( "overrideVisibility" ), &status );
		if ( status == MS::kSuccess ) {
			oPlug.getValue( castsShadows );
		}
	}
	status.clear();
    
    return  castsShadows;
}

bool isObjectMotionBlur( const MDagPath & path )
//  
//  Description:
//      Check if the given object is visible
//  
{
	MStatus status;
    MFnDagNode fnDN( path );
    // Check the visibility attribute of the node
    //
    MPlug vPlug = fnDN.findPlug( "motionBlur", &status );
    bool motionBlur = false;
	if ( status == MS::kSuccess ) {
		vPlug.getValue( motionBlur );
	}
	status.clear();
    
    return  motionBlur;
}

bool areObjectAndParentsVisible( const MDagPath & path )
//  
//  Description:
//      Check if this object and all of its parents are visible.  In Maya,
//      visibility is determined by  heirarchy.  So, if one of a node's
//      parents is invisible, then so is the node.
//  
{
    bool result = true;
    if ( debugMode ) { printf("-> getting searchpath\n"); }
    MDagPath searchPath( path );
    
    if ( debugMode ) { printf("-> stepping through search path\n"); }
	bool searching = true;
    while ( searching ) {
		if ( debugMode ) { printf("-> checking visibility\n"); }
        if ( !isObjectVisible( searchPath )  ){
            result = false;
            searching = false;
        }
        if ( searchPath.length() == 1 ) searching = false;
        searchPath.pop();
    }
    return result;
}

bool areObjectAndParentsTemplated( const MDagPath & path )
//  
//  Description:
//      Check if this object and all of its parents are visible.  In Maya,
//      visibility is determined by  heirarchy.  So, if one of a node's
//      parents is invisible, then so is the node.
//  
{
    bool result = true;
    MDagPath searchPath( path );
    
    while ( true ) {
        if ( isObjectTemplated( searchPath )  ){
            result = false;
            break;
        }
        if ( searchPath.length() == 1 ) break;
        searchPath.pop();
    }
    return result;
}

/* Build the correct token/array pairs from the scene data to correctly pass to Renderman. */
void assignTokenArrays( unsigned numTokens, rTokenPointer tokenPointerArray[], RtToken tokens[], RtPointer pointers[] )
{
	unsigned i;
	char	    detailType[20];
	char	    paramType[100];
	char	    declare[120];
	
	for ( i = 0; i < numTokens; i++ ) 
	{
		switch ( tokenPointerArray[i].dType ) {
		case rVertex: { 
			sprintf( detailType,  "vertex" ); 
			break; }
		case rVarying: {
			sprintf( detailType, "varying" );
			break; }
		case rUniform: {
			sprintf( detailType, "uniform" );			
			break; }
		case rConstant: {
			sprintf( detailType, "constant" );			
			break; }
		case rFaceVarying: {
			sprintf( detailType, "facevarying" );			
			break; }
		}
		tokens[i] = tokenPointerArray[i].tokenName;
		switch ( tokenPointerArray[i].pType ) {
		case rString: {
			sprintf( paramType, "string");
			pointers[i] = RtPointer(&( tokenPointerArray[i].tokenString ));
			break;
					  }
		case rFloat: {
			if ( tokenPointerArray[i].isUArray ) {
				sprintf( paramType, "float[%d]", tokenPointerArray[i].uArraySize );
			} else {
				sprintf( paramType, "float" );
			}
			pointers[i] = RtPointer( tokenPointerArray[i].tokenFloats );
			break;
					 }
		case rPoint: {
			if ( tokenPointerArray[i].isNurbs )
			{
				if ( tokenPointerArray[i].isArray )
				{
					sprintf( paramType, "hpoint");
				} else {
					sprintf( paramType, "point");
				}
			} else {
				sprintf( paramType, "point");
			}
			pointers[i] = RtPointer(tokenPointerArray[i].tokenFloats);
			break;
					 }
		case rVector: {
			sprintf( paramType, "vector");
			pointers[i] = RtPointer(tokenPointerArray[i].tokenFloats);
			break;
					  }
		case rNormal: {
			sprintf( paramType, "normal");
			pointers[i] = RtPointer(tokenPointerArray[i].tokenFloats);
			break;
					  }
		case rColor: {
			sprintf( paramType, "color");
			pointers[i] = RtPointer(tokenPointerArray[i].tokenFloats);
			break;
					 }
		}
		sprintf( declare, "%s %s", detailType, paramType );
		std::string tokenName = tokenPointerArray[i].tokenName;
		if ( ( tokenName != "st" ) ) {
			RiDeclare( tokens[i], declare ); 
		} else if ( ( tokenName == "st" ) && ( tokenPointerArray[i].dType == rFaceVarying ) ) {
			RiDeclare( tokens[i], declare ); 
		} 
	}
}

/* Build the correct token/array pairs from the scene data to correctly pass to Renderman. */
/* this is another version that takes a std::vector as input instead of a static array */
void assignTokenArraysV( std::vector<rTokenPointer> *tokenPointerArray, RtToken tokens[], RtPointer pointers[] )
{
	unsigned i = 0;
	char	    detailType[20];
	char	    paramType[100];
	char	    declare[120];
	
	std::vector<rTokenPointer>::iterator iter = tokenPointerArray->begin();
	while ( iter != tokenPointerArray->end() )
	{
		switch ( iter->dType ) {
		case rVertex: { 
			sprintf( detailType,  "vertex" ); 
			break; }
		case rVarying: {
			sprintf( detailType, "varying" );
			break; }
		case rUniform: {
			sprintf( detailType, "uniform" );			
			break; }
		case rConstant: {
			sprintf( detailType, "constant" );			
			break; }
		case rFaceVarying: {
			sprintf( detailType, "facevarying" );			
			break; }
		}
		tokens[i] = iter->tokenName;
		switch ( iter->pType ) {
		case rString: {
			sprintf( paramType, "string");
			pointers[i] = RtPointer(&( iter->tokenString ));
			break;
					  }
		case rFloat: {
			if ( iter->isUArray ) {
				sprintf( paramType, "float[%d]", iter->uArraySize );
			} else {
				sprintf( paramType, "float" );
			}
			pointers[i] = RtPointer( iter->tokenFloats );
			break;
					 }
		case rPoint: {
			if ( iter->isNurbs )
			{
				if ( iter->isArray )
				{
					sprintf( paramType, "hpoint");
				} else {
					sprintf( paramType, "point");
				}
			} else {
				sprintf( paramType, "point");
			}
			pointers[i] = RtPointer(iter->tokenFloats);
			break;
					 }
		case rVector: {
			sprintf( paramType, "vector");
			pointers[i] = RtPointer(iter->tokenFloats);
			break;
					  }
		case rNormal: {
			sprintf( paramType, "normal");
			pointers[i] = RtPointer(iter->tokenFloats);
			break;
					  }
		case rColor: {
			sprintf( paramType, "color");
			pointers[i] = RtPointer(iter->tokenFloats);
			break;
					 }
		}
		sprintf( declare, "%s %s", detailType, paramType );
		std::string tokenName = iter->tokenName;
		if ( ( tokenName != "st" ) ) {
			RiDeclare( tokens[i], declare ); 
		} else if ( ( tokenName == "st" ) && ( iter->dType == rFaceVarying ) ) {
			RiDeclare( tokens[i], declare ); 
		} 
		++iter;
		i++;
	}
}

MObject findFacetShader( MObject mesh, int polygonIndex ){
	MFnMesh     fnMesh( mesh );
	MObjectArray shaders;
	MIntArray indices;
	MDagPath path;
	
	if (!fnMesh.getConnectedShaders( 0, shaders, indices ))
		std::cerr << "ERROR: MFnMesh::getConnectedShaders\n" << std::flush;
    
	MObject shaderNode = shaders[ indices[ polygonIndex ] ];
	
	return shaderNode;    
}

/* Check to see if a file exists - seems to work correctly for both platforms */
bool fileExists( MString & filename ) {
    struct stat sbuf;
#ifdef _WIN32
	bool exists = !( 1 + (stat(filename.asChar(), &sbuf)));
#else
	bool exists = !(stat(filename.asChar(), &sbuf));
#endif
    return exists;
}

// function to parse strings sent to liquid to replace defined 
// characters with specific variables
MString parseString( MString & inputString )
{
	MString constructedString;
	MString tokenString;
	bool inToken = false;
	int sLength = inputString.length();
	int i;
	for ( i = 0; i < sLength; i++ ) {
		if ( inputString.substring(i, i) == "$" ) {
			tokenString.clear();
			inToken = true;
		} else if ( inToken ) {
			tokenString += inputString.substring(i, i);
			if ( tokenString == "F" ) {
				constructedString += (int)lframe;
				inToken = false;
				tokenString.clear();
			} else if ( tokenString == "SCN" ) {
				constructedString += sceneName;
				inToken = false;
				tokenString.clear();
			} else if ( tokenString == "IMG" ) {
				constructedString += DDimageName[0];
				inToken = false;
				tokenString.clear();
			} else if ( tokenString == "PDIR" ) {
				constructedString += projectDir;
				inToken = false;
				tokenString.clear();
			} else if ( tokenString == "RDIR" ) {
				constructedString += ribDir;
				inToken = false;
				tokenString.clear();
			} else {
				constructedString += "$";
				constructedString += tokenString; 
				tokenString.clear();
				inToken = false;
			}
		} else if ( inputString.substring(i, i) == "@" && inputString.substring(i - 1, i - 1) != "\\" ) {
			constructedString += (int)lframe;
		} else if ( inputString.substring(i, i) == "#" && inputString.substring(i - 1, i - 1) != "\\" ) {
			int paddingSize = 0;
			while ( inputString.substring(i, i) == "#" ) {
				paddingSize++;
				i++;
			}
			i--;
			if ( paddingSize == 1 ) paddingSize = 4;
			if ( paddingSize > 20 ) paddingSize = 20;
			char paddedFrame[20];
			sprintf( paddedFrame, "%0*d", paddingSize, lframe );
			constructedString += paddedFrame;
		} else if ( inputString.substring(i, i) == "%" && inputString.substring(i - 1, i - 1) != "\\" ) {
			MString envString;
			char *envVal = NULL;
			i++;
			while ( inputString.substring(i, i) != "%" ) {
				envString += inputString.substring(i, i);
				i++;
			}
			envVal = getenv( envString.asChar() );
			constructedString += envVal;
		} else if ( inputString.substring(i + 1, i + 1 ) == "#" && inputString.substring(i, i) == "\\" ) {
		} else {
			constructedString += inputString.substring(i, i);
		}
	}
	return constructedString;
}

MString liquidTransGetSceneName() 
{
    MString fullName;
	MString fileName;
	MGlobal::executeCommand( "file -q -a", fullName );
	
	// move backwards across the string until we hit a dirctory / and
	// take the info from there on
	char *myslash = "/";
	char *mydot = ".";
	int i = fullName.rindex( *myslash );
	int j = fullName.rindex( *mydot );
	fileName = fullName.substring( i + 1, j - 1 );
    return fileName;
}

MString liquidTransGetFullSceneName() 
{
	MString fileName;
	MGlobal::executeCommand( "file -q -sn", fileName );
	
    return fileName;
}

MString liquidResolveWinPaths( MString inPath ) 
{
	MString newName;
	for ( unsigned int i = 0; i < inPath.length(); i++ ) {
	}
	return newName;
}

liquidlong liquidHash(const char *str)
//
//  Description:
//      hash function for strings
//
{
    if ( debugMode ) { printf("-> hashing\n"); }
    liquidlong hc = 0;
    
    while(*str) {
			//hc = hc * 13 + *str * 27;   // old hash function
			hc = hc + *str;   // change this to a better hash func
			str++;
    }
	
    return (liquidlong)hc;
}

shaderStruct liquidGetShader( MObject shaderObj )
{
	MString rmShaderStr;
	MStatus status;

	MFnDependencyNode shaderNode( shaderObj );
	MPlug rmanShaderNamePlug = shaderNode.findPlug( MString( "rmanShaderLong" ) );
	rmanShaderNamePlug.getValue( rmShaderStr );
	char *assignedRManShader = (char *)alloca(rmShaderStr.length());
	sprintf(assignedRManShader, rmShaderStr.asChar());
				
	if ( debugMode ) { printf("-> Using Renderman Shader %s. \n", assignedRManShader ) ;}
				
	int 			numArgs;
	shaderStruct currentShader;
	currentShader.numTPV = 0;
	currentShader.hasShadingRate = false;
	currentShader.hasDisplacementBound = false;
	currentShader.hasErrors = false;
				
	bool usePre = false;
	std::vector<shaderStruct>::iterator iter = shaders.begin();
	while ( iter != shaders.end() ){
		std::string shaderNodeName = shaderNode.name().asChar();
		if ( iter->name == shaderNodeName ) {
			currentShader = *iter;
			usePre = true;
		}
		++iter;
	}
				
	/* if this shader instant isn't currently used already then load it into the lookup */
	if ( !usePre ) {
		// set it as my slo lookup
		currentShader.name = shaderNode.name().asChar();
		currentShader.file = rmShaderStr.substring( 0, rmShaderStr.length() - 5 ).asChar();
		
		liquidGetSloInfo shaderInfo;
		int success = shaderInfo.setShader( rmShaderStr );
		if ( !success ) {
			perror("Slo_SetShader");
			printf("Error Using Shader %s!\n", shaderNode.name().asChar() );
			currentShader.rmColor[0] = 1.0;
			currentShader.rmColor[1] = 0.0;
			currentShader.rmColor[2] = 0.0;
			currentShader.name = "plastic";
			currentShader.numTPV = 0;
			currentShader.hasErrors = true;
		} else { 
		/* Used to handling shading rates set in the surface shader, 
		this is a useful way for shader writers to ensure that their 
		shaders are always rendered as they were designed.  This value
		overrides the global shading rate but gets overridden with the 
			node specific shading rate. */
			
			// Set RiColor and RiOpacity
			MPlug colorPlug = shaderNode.findPlug( "color" );
			
			colorPlug.child(0).getValue( currentShader.rmColor[0] );
			colorPlug.child(1).getValue( currentShader.rmColor[1] );
			colorPlug.child(2).getValue( currentShader.rmColor[2] );
			
			MPlug opacityPlug = shaderNode.findPlug( "opacity" );
			
			double opacityVal;
			opacityPlug.getValue( opacityVal );
			currentShader.rmOpacity[0] = RtFloat( opacityVal );
			currentShader.rmOpacity[1] = RtFloat( opacityVal );
			currentShader.rmOpacity[2] = RtFloat( opacityVal );
			
			// find the parameter details and declare them in the rib stream
			numArgs = shaderInfo.getNumParam();
			int i;
			for ( i = 0; i < numArgs; i++ )
			{
				SHADER_TYPE currentArgType = shaderInfo.getArgType( i );
				SHADER_DETAIL currentArgDetail = shaderInfo.getArgDetail( i );
				currentShader.tokenPointerArray[currentShader.numTPV].isNurbs = false;
				if ( shaderInfo.getArgName( i ) == "liquidShadingRate" ) {
					
					/* BUGFIX: Monday 6th August - fixed shading rate bug where it only accepted the default value */
					
					MPlug floatPlug = shaderNode.findPlug( shaderInfo.getArgName( i ), &status );
					if ( status == MS::kSuccess ) {
						float floatPlugVal;
						floatPlug.getValue( floatPlugVal );
						currentShader.shadingRate = floatPlugVal;
					} else {
						currentShader.shadingRate = shaderInfo.getArgFloatDefault( i, 0 );
					}
					currentShader.hasShadingRate = true;
					continue;
				}
				switch ( shaderInfo.getArgDetail(i) ) {
				case SHADER_DETAIL_UNIFORM: {
					currentShader.tokenPointerArray[currentShader.numTPV].dType = rUniform;
					break;
											}
				case SHADER_DETAIL_VARYING: {
					currentShader.tokenPointerArray[currentShader.numTPV].dType = rVarying;
					break; 
											}
				}	     
				switch ( shaderInfo.getArgType( i ) ) {
				case SHADER_TYPE_STRING: {
					MPlug stringPlug = shaderNode.findPlug( shaderInfo.getArgName( i ), &status );
					if ( status == MS::kSuccess ) {
						MString stringPlugVal;
						stringPlug.getValue( stringPlugVal );
						MString stringDefault = shaderInfo.getArgStringDefault( i, 0 );
						if ( stringPlugVal != stringDefault ) {
							sprintf( currentShader.tokenPointerArray[currentShader.numTPV].tokenName , shaderInfo.getArgName( i ).asChar() );
							MString stringVal = parseString( stringPlugVal );
							currentShader.tokenPointerArray[ currentShader.numTPV ].tokenString = ( char * )lmalloc( sizeof( char ) * stringVal.length() + 1 );
							sprintf( currentShader.tokenPointerArray[ currentShader.numTPV ].tokenString, stringVal.asChar() );
							currentShader.tokenPointerArray[ currentShader.numTPV ].pType = rString;
							currentShader.tokenPointerArray[ currentShader.numTPV ].arraySize = 0;
							currentShader.tokenPointerArray[ currentShader.numTPV ].isArray = false;
							currentShader.tokenPointerArray[ currentShader.numTPV ].isUArray = false;
							currentShader.numTPV++;
						}
					}
					break; }
				case SHADER_TYPE_SCALAR: {
					MPlug floatPlug = shaderNode.findPlug( shaderInfo.getArgName( i ), &status );
					if ( status == MS::kSuccess ) {
						sprintf( currentShader.tokenPointerArray[currentShader.numTPV].tokenName , shaderInfo.getArgName( i ).asChar() );
						currentShader.tokenPointerArray[currentShader.numTPV].pType = rFloat;
						currentShader.tokenPointerArray[currentShader.numTPV].arraySize = shaderInfo.getArgArraySize( i );
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
							currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) );
							currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[0] = floatPlugVal;
						}
						currentShader.numTPV++;
					}
					break; }
				case SHADER_TYPE_COLOR: {
					MPlug triplePlug = shaderNode.findPlug( shaderInfo.getArgName( i ), &status );
					currentShader.tokenPointerArray[currentShader.numTPV].isArray = false;
					currentShader.tokenPointerArray[currentShader.numTPV].isUArray = false;
					if ( status == MS::kSuccess ) {
						sprintf( currentShader.tokenPointerArray[currentShader.numTPV].tokenName , shaderInfo.getArgName( i ).asChar() );
						currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3);
						triplePlug.child(0).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[0] );
						triplePlug.child(1).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[1] );
						triplePlug.child(2).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[2] );
						currentShader.tokenPointerArray[currentShader.numTPV].pType = rColor;
						currentShader.tokenPointerArray[currentShader.numTPV].arraySize = 0;
						currentShader.numTPV++;
					}
					break; }
				case SHADER_TYPE_POINT: {
					MPlug triplePlug = shaderNode.findPlug( shaderInfo.getArgName( i ), &status );
					currentShader.tokenPointerArray[currentShader.numTPV].isArray = false;
					currentShader.tokenPointerArray[currentShader.numTPV].isUArray = false;
					if ( status == MS::kSuccess ) { 
						sprintf( currentShader.tokenPointerArray[currentShader.numTPV].tokenName , shaderInfo.getArgName( i ).asChar() );
						currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3);
						triplePlug.child(0).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[0] );
						triplePlug.child(1).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[1] );
						triplePlug.child(2).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[2] );
						currentShader.tokenPointerArray[currentShader.numTPV].pType = rPoint;
						currentShader.tokenPointerArray[currentShader.numTPV].arraySize = 0;
						currentShader.numTPV++;
					}
					break; }
				case SHADER_TYPE_VECTOR: {
					currentShader.tokenPointerArray[currentShader.numTPV].isArray = false;
					currentShader.tokenPointerArray[currentShader.numTPV].isUArray = false;
					MPlug triplePlug = shaderNode.findPlug( shaderInfo.getArgName( i ), &status );
					if ( status == MS::kSuccess ) {
						sprintf( currentShader.tokenPointerArray[currentShader.numTPV].tokenName , shaderInfo.getArgName( i ).asChar() );
						currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3);
						triplePlug.child(0).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[0] );
						triplePlug.child(1).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[1] );
						triplePlug.child(2).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[2] );
						currentShader.tokenPointerArray[currentShader.numTPV].pType = rVector;
						currentShader.tokenPointerArray[currentShader.numTPV].arraySize = 0;
						currentShader.numTPV++;
					}
					break; }
				case SHADER_TYPE_NORMAL: {
					currentShader.tokenPointerArray[currentShader.numTPV].isArray = false;
					currentShader.tokenPointerArray[currentShader.numTPV].isUArray = false;
					MPlug triplePlug = shaderNode.findPlug( shaderInfo.getArgName( i ), &status );
					if ( status == MS::kSuccess ) {
						sprintf( currentShader.tokenPointerArray[currentShader.numTPV].tokenName , shaderInfo.getArgName( i ).asChar() );
						currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats = (RtFloat *)lmalloc( sizeof(RtFloat) * 3);
						triplePlug.child(0).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[0] );
						triplePlug.child(1).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[1] );
						triplePlug.child(2).getValue( currentShader.tokenPointerArray[currentShader.numTPV].tokenFloats[2] );
						currentShader.tokenPointerArray[currentShader.numTPV].pType = rNormal;
						currentShader.tokenPointerArray[currentShader.numTPV].arraySize = 0;
						currentShader.numTPV++;
					}
					break; }
				case SHADER_TYPE_MATRIX: {
					currentShader.tokenPointerArray[currentShader.numTPV].isArray = false;
					currentShader.tokenPointerArray[currentShader.numTPV].isUArray = false;
					printf( "WHAT IS THE MATRIX!\n" );
					break; }
				default:
					printf("Unknown\n");
					break;
					}
			}
			shaders.push_back( currentShader );
		}
		shaderInfo.resetIt();
	}
	return currentShader;
}
