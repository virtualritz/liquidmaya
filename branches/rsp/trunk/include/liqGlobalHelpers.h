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


typedef enum {
  verbosityOff = 0,
  verbosityErrors = 1,
  messageError = 1,
  verbosityErrorsWarnings = 2,
  messageWarning = 2,
  verbosityAll = 3,
  messageInfo = 3
} liquidVerbosityType;

// Moritz: basename() is missing in Windoze, we define our own in liqGlobalHelpers.cpp
#ifdef _WIN32
char* basename( const char* filename );
#else
#  include <libgen.h> // for basename()
#endif

#define LIQ_GET_SHADER_FILE_NAME(a, b, c) if( b ) a = basename( const_cast<char *>(c.file.c_str())); else a = const_cast<char *>(c.file.c_str());


MStringArray findAttributesByPrefix(const char* pPrefix, MFnDependencyNode& NodeFn );
bool isObjectTwoSided( const MDagPath & path );
bool isObjectReversed( const MDagPath & path );
bool isObjectVisible( const MDagPath & path );
bool isObjectPrimaryVisible( const MDagPath & path );
bool isObjectTemplated( const MDagPath & path );
bool isObjectCastsShadows( const MDagPath & path );
bool isObjectMotionBlur( const MDagPath & path );
bool areObjectAndParentsVisible( const MDagPath & path );
bool areObjectAndParentsTemplated( const MDagPath & path );
void assignTokenArrays( unsigned numTokens, const liqTokenPointer tokenPointerArray[], RtToken tokens[], RtPointer pointers[] );
void assignTokenArraysV( const vector<liqTokenPointer>& tokenPointerArray, RtToken tokens[], RtPointer pointers[] );
MObject findFacetShader( MObject mesh, int polygonIndex );
bool fileExists( const MString & filename );
bool fileIsNewer( const MString & file1, const MString & file2 );
bool fileFullyAccessible( const MString& path );
MString getFullPathFromRelative ( const MString & filename );
MString getFileName( const MString & fullpath );
MString parseString( const MString & inputString, bool doEscaped = true );
MString parseCommandString( const MString & inputString );
MString liquidTransGetSceneName();
MString liquidTransGetFullSceneName();
void liquidGetGlobal( MString globalName, double &value, MStatus &returnStatus );
liquidlong liquidHash( const char *str );
MString liquidSanitizePath( const MString& inputString );
string liquidSanitizePath( const string& inputString );
MString liquidSanitizeSearchPath( const MString& inputString );
string liquidSanitizeSearchPath( const string& inputString );
string liquidGetRelativePath( bool relative, const string& name, const string& dir );
MString liquidGetRelativePath( bool relative, const MString& name, const MString& dir );
MString removeEscapes( const MString & inputString );
MObject getNodeByName( MString name, MStatus *returnStatus );
string getEnvironment( const string& envVar );
vector< int > generateFrameNumbers( const string& mayaSeq );
bool makeFullPath( const string& name, int mode );
string sanitizeNodeName( const string& name );
MString sanitizeNodeName( const MString& name );
RtString& getLiquidRibName( const string& name );
void liquidMessage( const string& msg, liquidVerbosityType type );

class arrayDeleter // needed to free a shared/scoped/weak_ptr to an array
{
  public:
    template< typename T >
    void operator()( T* t ) {
      delete[] t;
    }
};



#endif