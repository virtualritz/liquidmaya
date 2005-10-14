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

#ifndef liqGlobalHelpers_H
#define liqGlobalHelpers_H

/* ______________________________________________________________________
** 
** Liquid Liquid Global Helpers Header File
** ______________________________________________________________________
*/

#include <string>
#include <vector>

#include <maya/MString.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>

#include <liquid.h>
#include <liqTokenPointer.h>
#include <liqShader.h>


// Moritz: basename() is missing in Windoze, we define our own in liqGlobalHelpers.cpp
#ifdef _WIN32
char * basename( const char *filename );
#else
#  include <libgen.h> // for basename()
#endif

#define LIQ_GET_SHADER_FILE_NAME(a, b, c) if( b ) a = basename( const_cast<char *>(c.file.c_str())); else a = const_cast<char *>(c.file.c_str());

// Moritz: this macro is needed to get absolute pathnames for
// creating RIBs, archives and the renderscript in case the user
// has choosen to have all paths relative
#define LIQ_GET_ABS_REL_FILE_NAME(rel, name, dir) ((rel) ? ((dir) + (name)) : (name))


MStringArray findAttributesByPrefix(const char* pPrefix, MFnDependencyNode& NodeFn );
bool isObjectTwoSided( const MDagPath & path );
bool isObjectVisible( const MDagPath & path );
bool isObjectPrimaryVisible( const MDagPath & path );
bool isObjectTemplated( const MDagPath & path );
bool isObjectCastsShadows( const MDagPath & path );
bool isObjectMotionBlur( const MDagPath & path );
bool areObjectAndParentsVisible( const MDagPath & path );
bool areObjectAndParentsTemplated( const MDagPath & path );
void assignTokenArrays( unsigned numTokens, liqTokenPointer tokenPointerArray[], RtToken tokens[], RtPointer pointers[] );
void assignTokenArraysV( std::vector<liqTokenPointer> *tokenPointerArray, RtToken tokens[], RtPointer pointers[] );
MObject findFacetShader( MObject mesh, int polygonIndex );
bool fileExists( const MString & filename );
bool fileIsNewer( const MString & file1, const MString & file2 );
MString getFullPathFromRelative ( const MString & filename );
MString getFileName( const MString & fullpath );
MString parseString( const MString & inputString ); 
MString parseCommandString( const MString & inputString );
MString liquidTransGetSceneName();
MString liquidTransGetFullSceneName();
void liquidInfo( MString info );
void liquidGetGlobal( MString globalName, double &value, MStatus &returnStatus );
liquidlong liquidHash( const char *str );
MString liquidSanitizePath( MString & inputString );

#endif
