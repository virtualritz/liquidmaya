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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

// Renderman Headers
extern "C" {
#include <ri.h>
}

#ifdef _WIN32
#  include <malloc.h>
#else
#  include <alloca.h>
#  include <stdlib.h>
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
#include <maya/MFnSet.h>
#include <maya/MItDag.h>
#include <maya/MItSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MCommandResult.h>

#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liqMemory.h>
#include <liqGetSloInfo.h>

extern int debugMode;
extern bool liquidBin;

extern long    liqglo_lframe;
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

MStringArray findAttributesByPrefix( const char* pPrefix, MFnDependencyNode& NodeFn )
//
//  Description:
//	Check to see if the node NodeFn has any attributes starting with pPrefix and store those
//	in Matches to return
//
{
  MStringArray Matches;

  for( unsigned i = 0; i < NodeFn.attributeCount(); i++ ) {
    MFnAttribute AttributeFn = NodeFn.attribute(i);
    MString AttributeName = AttributeFn.name();
    if (!strncmp(AttributeName.asChar(), pPrefix, strlen(pPrefix) )) {
      Matches.append(AttributeName);
    }
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
  LIQDEBUGPRINTF( "-> checking visibility attribute\n");
  MPlug vPlug = fnDN.findPlug( "visibility", &status );
  LIQDEBUGPRINTF( "-> checking visibility setting\n");
  bool visible = true;
  if ( status == MS::kSuccess ) {
    vPlug.getValue( visible );
  }
  status.clear();

  // Moritz:
  // Check for liquidInvisible attribute. Similar to mtorInvis,
  // this attributes allows objects that have their visibility
  // checked to be skipped for Liquid's output
  bool liquidInvisible = false;
  MPlug liPlug = fnDN.findPlug( "liqInvisible", &status );
  if ( status == MS::kSuccess ) {
    liPlug.getValue( liquidInvisible );
  }
  status.clear();

  LIQDEBUGPRINTF( "-> done checking visibility attribute\n" );
  // Also check to see if the node is an intermediate object in
  // a computation.  For example, it could be in the middle of a
  // chain of deformations.  Intermediate objects are not visible.
  //
  LIQDEBUGPRINTF( "-> checking intermediate object\n" );
  MPlug iPlug = fnDN.findPlug( "intermediateObject", &status );
  bool intermediate = false;
  if ( status == MS::kSuccess ) {
    iPlug.getValue( intermediate );
  }
  status.clear();
  LIQDEBUGPRINTF( "-> done checking intermediate object\n" );

  return  visible && !liquidInvisible && !intermediate;
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
  LIQDEBUGPRINTF( "-> checking overrideEnabled\n" );
  status.clear();
  MPlug oPlug = fnDN.findPlug( MString( "overrideEnabled" ), &status );
  bool isOver = false;
  if ( status == MS::kSuccess ) {
    oPlug.getValue( isOver );
  }
  LIQDEBUGPRINTF( "-> done checking overrideEnabled\n" );
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
//      Check if the given object casts shadows
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

bool isObjectReceivesShadows( const MDagPath & path )
//
//  Description:
//      Check if the given object receives shadows
//
{
  MStatus status;
  MFnDagNode fnDN( path );
  // Check the visibility attribute of the node
  //
  MPlug vPlug = fnDN.findPlug( MString( "receivesShadows" ), &status );
  bool receivesShadows = true;
  if ( status == MS::kSuccess ) {
    vPlug.getValue( receivesShadows );
  }
  status.clear();

  return  receivesShadows;
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
  LIQDEBUGPRINTF( "-> getting searchpath\n" );
  MDagPath searchPath( path );

  LIQDEBUGPRINTF( "-> stepping through search path\n" );
  bool searching = true;
  while ( searching ) {
    LIQDEBUGPRINTF( "-> checking visibility\n" );
    if ( !isObjectVisible( searchPath ) ) {
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
    if ( isObjectTemplated( searchPath ) ) {
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
  for ( unsigned i = 0; i < numTokens; i++ ) {
    tokens[i] = tokenPointerArray[i].getDetailedTokenName();
    pointers[i] = tokenPointerArray[i].getRtPointer();
  }
}

/* Build the correct token/array pairs from the scene data to correctly pass
 * to Renderman.  this is another version that takes a std::vector as input
 * instead of a static array */
void assignTokenArraysV( std::vector<liqTokenPointer> *tokenPointerArray, RtToken tokens[], RtPointer pointers[] )
{
  unsigned i = 0;
  std::vector<liqTokenPointer>::iterator iter = tokenPointerArray->begin();
  while ( iter != tokenPointerArray->end() ) {
    tokens[i] = iter->getDetailedTokenName();
    pointers[i] = iter->getRtPointer();
    ++iter;
    i++;
  }
}

MObject findFacetShader( MObject mesh, int polygonIndex ){
  MFnMesh     fnMesh( mesh );
  MObjectArray shaders;
  MIntArray indices;
  MDagPath path;

  if (!fnMesh.getConnectedShaders( 0, shaders, indices )) {
    std::cerr << "ERROR: MFnMesh::getConnectedShaders\n" << std::flush;
  }

  MObject shaderNode = shaders[ indices[ polygonIndex ] ];

  return shaderNode;
}

/* Check to see if a file exists - seems to work correctly for both platforms */
bool fileExists( const MString & filename ) {
    struct stat sbuf;
    int result = stat( filename.asChar(), &sbuf );
#ifdef _WIN32
    // under Win32, stat fails if path is a directory name ending in a slash
    // so we check for DIR/. which works - go figure
    if( result && ( filename.rindex( '/' ) == filename.length() - 1 ) ) {
      result = stat( ( filename + "." ).asChar(), &sbuf );
    }
#endif
    return ( result == 0 );
}

// Check to see if a file1 is newer than file2
bool fileIsNewer( const MString & file1, const MString & file2 ) {

  struct stat sbuf1, sbuf2;
  stat( file1.asChar(), &sbuf1 );
  stat( file2.asChar(), &sbuf2 );

  return ( sbuf1.st_mtime > sbuf2.st_mtime );
}

// If filename is relative to project dir -- returns fullpathname
MString getFullPathFromRelative ( const MString & filename ) {
  MString ret;
  extern MString liqglo_projectDir;

  if( filename.index( 0 ) != '/' ) // relative path, add prefix project folder
    ret = liqglo_projectDir + "/" + filename;
  else
    ret = filename;

  return ret;
}

MString getFileName ( const MString & fullpath ) {

  return fullpath.substring( fullpath.rindex('/') + 1, fullpath.length() - 1 );
}

// function to parse strings sent to liquid to replace defined
// characters with specific variables
MString parseString( const MString & inputString )
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
      if ( paddingSize == 1 ) {
        paddingSize = 4;
      }
      if ( paddingSize > 20 ) {
        paddingSize = 20;
      }
      char paddedFrame[20];
      sprintf( paddedFrame, "%0*ld", paddingSize, liqglo_lframe );
      constructedString += paddedFrame;
    } else if ( inputString.substring(i, i) == "%" && inputString.substring(i - 1, i - 1) != "\\" ) {
      MString	envString;
      char*	envVal = NULL;

      i++;

      // loop through the string looking for the closing %
      if (i < sLength) {
        while ( i < sLength && inputString.substring(i, i) != "%" ) {
          envString += inputString.substring(i, i);
          i++;
        }

        envVal = getenv( envString.asChar() );

        if (envVal != NULL) {
          constructedString += envVal;
        }
        // else environment variable doesn't exist.. do nothing
      }
      // else early exit: % was the last character in the string.. do nothing
    } else if ( inputString.substring(i + 1, i + 1 ) == "#" && inputString.substring(i, i) == "\\" ) {
      // do nothing
    } else if ( inputString.substring(i + 1, i + 1 ) == "n" && inputString.substring(i, i) == "\\" ) {
      constructedString += "\n";
      i++;
    } else if ( inputString.substring(i + 1, i + 1 ) == "t" && inputString.substring(i, i) == "\\" ) {
      constructedString += "\t";
      i++;
    } else {
      constructedString += inputString.substring(i, i);
    }
  }

  // Moritz: now parse for MEL command sequences
  //return parseCommandString( constructedString );
  return constructedString;
}

// Moritz: added below code for simple MEL parameter expression scriptig support
// syntax: [mel commands]
MString parseCommandString( const MString & inputString )
{
  MString constructedString;
  MString tokenString;
  unsigned sLength = inputString.length();

  for ( unsigned i = 0; i < sLength; i++ ) {
    if ( inputString.substring(i, i) == "[" && inputString.substring(i - 1, i - 1) != "\\" ) {
      MString	melCmdString;
      i++;

      // loop through the string looking for the closing %
      if ( i < sLength ) {
        while ( i < sLength && inputString.substring(i, i) != "]" && inputString.substring(i - 1, i - 1) != "\\" ) {
          melCmdString += inputString.substring(i, i);
          i++;
        }

        MCommandResult melCmdResult;

#ifdef DEBUG
        // Output command to Script window for debugging
        if ( MS::kSuccess == MGlobal::executeCommand( melCmdString, melCmdResult, true, false ) ) {
#else
        if ( MS::kSuccess == MGlobal::executeCommand( melCmdString, melCmdResult, false, false ) ) {
#endif
          // convert result to string
          MString tmpStr;

          switch( melCmdResult.resultType() ) {

            case MCommandResult::kInt: {
              int tmpInt;
              melCmdResult.getResult( tmpInt );
              tmpStr = tmpInt;
            }
            break;
            case MCommandResult::kIntArray: {
              MIntArray tmpIntArray;
              melCmdResult.getResult( tmpIntArray );
              for( unsigned j = 0; j < tmpIntArray.length(); j++ ) {
                if( j > 0 )
                  tmpStr += " ";
                tmpStr += tmpIntArray[ j ];
              }
            }
            break;
            case MCommandResult::kDouble: {
              double tmpDbl;
              melCmdResult.getResult( tmpDbl );
              tmpStr = tmpDbl;
            }
            break;
            case MCommandResult::kDoubleArray: {
              MDoubleArray tmpDblArray;
              melCmdResult.getResult( tmpDblArray );
              for( unsigned j = 0; j < tmpDblArray.length(); j++ ) {
                if( j > 0 )
                  tmpStr += " ";
                tmpStr += tmpDblArray[ j ];
              }
            }
            break;
            case MCommandResult::kString: {
              melCmdResult.getResult( tmpStr );
            }
            break;
            case MCommandResult::kStringArray: {
              MStringArray tmpStrArray;
              melCmdResult.getResult( tmpStrArray );
              for( unsigned j = 0; j < tmpStrArray.length(); j++ ) {
                if( j > 0 )
                  tmpStr += " ";
                tmpStr += tmpStrArray[ j ];
              }
            }
            break;
            case MCommandResult::kVector: {
              MVector tmpVec;
              melCmdResult.getResult( tmpVec );
              for( int j = 0; j < tmpVec.length(); j++ ) {
                if( i > 0 )
                  tmpStr += " ";
                tmpStr += tmpVec[ i ];
              }
            }
            break;
            case MCommandResult::kVectorArray: {
              MVectorArray tmpVecArray;
              melCmdResult.getResult( tmpVecArray );
              for( unsigned j = 0; j < tmpVecArray.length(); j++ ) {
                if( j > 0 )
                  tmpStr += " ";
                for( unsigned k = 0; tmpVecArray[ j ].length(); k++ ) {
                  if( k > 0 )
                    tmpStr += " ";
                  tmpStr += tmpVecArray[ j ] [ k ];
                }
              }
            }
            break;
            case MCommandResult::kMatrix: {
              MDoubleArray tmpMtx;
              int rows, cols;
              melCmdResult.getResult( tmpMtx, rows, cols );
              for( int j = 0; j < rows * cols; j++ ) {
                if( j > 0 )
                  tmpStr += " ";
                tmpStr += tmpMtx[ j ];
              }
            }
            break;
            case MCommandResult::kInvalid:
            default: {
              // do nothing
              // should we return some error string here?
            }
            break;
          }
          constructedString += tmpStr;
        }
        // else MEL command contained an error.. do nothing
      }
      // else early exit: ] was the last character in the string.. do nothing
    } else if ( inputString.substring(i + 1, i + 1 ) == "#" && inputString.substring(i, i) == "\\" ) {
      // do nothing
    } else if ( inputString.substring(i + 1, i + 1 ) == "n" && inputString.substring(i, i) == "\\" ) {
      constructedString += "\n";
      i++;
    } else if ( inputString.substring(i + 1, i + 1 ) == "t" && inputString.substring(i, i) == "\\" ) {
      constructedString += "\t";
      i++;
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
  // from Maya 6, unsaved files have no extension anymore, we have
  // to account for this here as the ending delimiting '.' is missing
  if( ( j < i + 2 ) || ( j == -1 ) )
    j = fullName.length();

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
  LIQDEBUGPRINTF( "-> hashing string\n" );
  liquidlong hc = 0;

  while(*str) {
    //hc = hc * 13 + *str * 27;   // old hash function
    hc = hc + *str;   // change this to a better hash func
    str++;
  }

  LIQDEBUGPRINTF( "-> done hashing string\n" );
  return (liquidlong)hc;
}

#ifdef _WIN32
char * basename( const char *filename ) {
//
//  Description:
//      returns the filename portion of a path
//
  char *p = strrchr( filename, '/' );
  return p ? p + 1 : (char *) filename;
}
#endif

//
//  Description:
//      converts '\' into '/' and <driveletter>: into //<driveletter>
//
MString liquidSanitizePath( MString & inputString )
{
  MString constructedString;

  const char *str = inputString.asChar();
  char buffer[ 2 ] = { 0, 0 };

  for ( unsigned i = 0; i < inputString.length(); i++ ) {
    buffer[ 0 ] = str[ i ];
    if ( str[ i ] == '\\' ) {
      buffer[ 0 ] = '/';
      if ( str[ i + 1 ] == '\\' )
        ++i; // skip double slashes
    }
    constructedString += buffer;
  }

#if defined DELIGHT || PRMAN
  // Convert from "C:/path" into "//C/path"
  if( inputString.substring( 1, 1 ) == ":" )
    constructedString = "//" + constructedString.substring( 0, 0 ) + constructedString.substring( 2, inputString.length() - 1 );
#endif // defined DELIGHT || PRMAN
  
  return constructedString;
}

