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
#include <stdlib.h>
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
#include <liqGetSloInfo.h>

extern int debugMode;
extern bool liquidBin;


extern long 	liqglo_lframe;
extern MString liqglo_sceneName;
extern MString liqglo_ribDir;
extern MString liqglo_projectDir;
extern MStringArray liqglo_DDimageName;


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
void assignTokenArrays( unsigned int numTokens, liqTokenPointer tokenPointerArray[],  RtToken tokens[], RtPointer pointers[] )
{
    unsigned i;
    char declare[256];
    for ( i = 0; i < numTokens; i++ ) 
    {
	tokens[i] = tokenPointerArray[i].getTokenName();
	pointers[i] = tokenPointerArray[i].getRtPointer();
	if( ! tokenPointerArray[i].isBasicST() )
	{
	    tokenPointerArray[i].getRiDeclare( declare );
	     RiDeclare( tokens[i], declare );
	}
    }
}

/* Build the correct token/array pairs from the scene data to correctly pass
 * to Renderman.  this is another version that takes a std::vector as input
 * instead of a static array */
void assignTokenArraysV( std::vector<liqTokenPointer> *tokenPointerArray, RtToken tokens[], RtPointer pointers[] )
{
    unsigned i = 0;
    char declare[256];
    std::vector<liqTokenPointer>::iterator iter = tokenPointerArray->begin();
    while ( iter != tokenPointerArray->end() )
    {
	tokens[i] = iter->getTokenName();
	pointers[i] = iter->getRtPointer();
	if( ! iter->isBasicST() )
	{
	    iter->getRiDeclare( declare );
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
bool fileExists(const MString & filename) {
    struct stat sbuf;
		int result = stat(filename.asChar(), &sbuf);
#ifdef _WIN32
		// under Win32, stat fails if path is a directory name ending in a slash
		// so we check for DIR/. which works - go figure
		if (result && (filename.rindex('/') == filename.length()-1)) {
			result = stat((filename + ".").asChar(), &sbuf);
		}
#endif
    return (result == 0);
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
		constructedString += (int)liqglo_lframe;
		inToken = false;
		tokenString.clear();
	    } else if ( tokenString == "SCN" ) {
		constructedString += liqglo_sceneName;
		inToken = false;
		tokenString.clear();
	    } else if ( tokenString == "IMG" ) {
		constructedString += liqglo_DDimageName[0];
		inToken = false;
		tokenString.clear();
	    } else if ( tokenString == "PDIR" ) {
		constructedString += liqglo_projectDir;
		inToken = false;
		tokenString.clear();
	    } else if ( tokenString == "RDIR" ) {
		constructedString += liqglo_ribDir;
		inToken = false;
		tokenString.clear();
	    } else {
		constructedString += "$";
		constructedString += tokenString; 
		tokenString.clear();
		inToken = false;
	    }
	} else if ( inputString.substring(i, i) == "@" && inputString.substring(i - 1, i - 1) != "\\" ) {
	    constructedString += (int)liqglo_lframe;
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
	    sprintf( paddedFrame, "%0*ld", paddingSize, liqglo_lframe );
	    constructedString += paddedFrame;
	} else if ( inputString.substring(i, i) == "%" && inputString.substring(i - 1, i - 1) != "\\" ) {
	    MString	envString;
	    char*	envVal = NULL;

	    i++;

	    // loop through the string looking for the closing %
	    if (i < sLength)
	    {
		while (   i < sLength
		       && inputString.substring(i, i) != "%" ) {
		    envString += inputString.substring(i, i);
		    i++;
		}

		envVal = getenv( envString.asChar() );
		
		if (envVal != NULL)
		    constructedString += envVal;
		// else environment variable doesn't exist.. do nothing
	    }
	    // else early exit: % was the last character in the string.. do nothing
	    
	} else if ( inputString.substring(i + 1, i + 1 ) == "#" && inputString.substring(i, i) == "\\" ) {
	    // do nothing
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
    int i = fullName.rindex( '/' );
    int j = fullName.rindex( '.' );
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
