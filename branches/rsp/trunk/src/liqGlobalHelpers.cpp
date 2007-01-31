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

// Maya Headers
#include <maya/MPlug.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnMesh.h>
#include <maya/MItSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MCommandResult.h>
#include <maya/MFnRenderLayer.h>

// Standard headers
#include <set>
#ifdef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h> //for _mdir()

//#include <process.h>
#include <io.h>

#endif

// Boost headers
#include <boost/tokenizer.hpp>
#include <boost/scoped_array.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <liqGlobalHelpers.h>


extern int debugMode;
extern bool liquidBin;

extern long   liqglo_lframe;
extern MString liqglo_sceneName;
extern MString liqglo_ribDir;
extern MString liqglo_projectDir;
extern MString liqglo_textureDir;
extern MStringArray liqglo_DDimageName;

extern MString liqglo_currentNodeName;
extern MString liqglo_currentNodeShortName;
extern MString liqglo_shotName;
extern MString liqglo_shotVersion;
extern MString liqglo_layer;

extern liquidVerbosityType liqglo_verbosity;

using namespace std;
using namespace boost;

/**  Check to see if the node NodeFn has any attributes starting with pPrefix and store those
 *  in Matches to return.
 */
MStringArray findAttributesByPrefix( const char* pPrefix, MFnDependencyNode& NodeFn )
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

/** Checks if the given object is double-sided.
 */
bool isObjectTwoSided( const MDagPath & path )
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

/** Checks if the given object is reversed.
 *
 *  That is if the opposite attribute is on.
 */
bool isObjectReversed( const MDagPath & path )
{
  MStatus status;
  MFnDagNode fnDN( path );
  MPlug dPlug = fnDN.findPlug( "opposite", &status );
  MString type = fnDN.typeName( &status );
  //cout <<"type is "<<type.asChar()<<endl;
  bool reversed = false;
  if ( status == MS::kSuccess ) {
    dPlug.getValue( reversed );
  }
  if ( type == "nurbsSurface" ) reversed = !reversed;
  return  reversed;
}


/** Check if the given object is visible.
 */
bool isObjectVisible( const MDagPath & path )
{
  MStatus status;
  MFnDagNode fnDN( path );
  // Check the visibility attribute of the node
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
  // chain of deformations.  Intermediate objects are not visible
  LIQDEBUGPRINTF( "-> checking intermediate object\n" );
  MPlug iPlug = fnDN.findPlug( "intermediateObject", &status );
  bool intermediate = false;
  if ( status == MS::kSuccess ) {
    iPlug.getValue( intermediate );
  }
  status.clear();
  LIQDEBUGPRINTF( "-> done checking intermediate object\n" );

  return visible && !liquidInvisible && !intermediate;
}

/** Checks if the given object is visible to the camera.
 */
bool isObjectPrimaryVisible( const MDagPath & path )
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

/** Checks if the given object is templated.
 */
bool isObjectTemplated( const MDagPath & path )
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


/** Checks if the given object casts shadows.
 */
bool isObjectCastsShadows( const MDagPath & path )
{
  MStatus status;
  MFnDagNode fnDN( path );
  // Check the visibility attribute of the node
  MPlug vPlug( fnDN.findPlug( MString( "castsShadows" ), &status ) );
  bool castsShadows( true );
  if ( status == MS::kSuccess ) {
    vPlug.getValue( castsShadows );
  }
  status.clear();
  MPlug oPlug( fnDN.findPlug( MString( "overrideEnabled" ), &status ) );
  bool isOver( false );
  if ( status == MS::kSuccess ) {
    oPlug.getValue( isOver );
  }
  status.clear();
  if ( castsShadows && isOver ) {
    MPlug oPlug( fnDN.findPlug( MString( "overrideVisibility" ), &status ) );
    if ( status == MS::kSuccess ) {
      oPlug.getValue( castsShadows );
    }
  }
  status.clear();

  return  castsShadows;
}


/** Check if the given object receives shadows.
 */
bool isObjectReceivesShadows( const MDagPath & path )
{
  MStatus status;
  MFnDagNode fnDN( path );
  // Check the visibility attribute of the node
  MPlug vPlug = fnDN.findPlug( MString( "receivesShadows" ), &status );
  bool receivesShadows = true;
  if ( status == MS::kSuccess ) {
    vPlug.getValue( receivesShadows );
  }
  status.clear();

  return  receivesShadows;
}

/** Check if the given object is motion-blurred.
 */
bool isObjectMotionBlur( const MDagPath & path )
{
  MStatus status;
  MFnDagNode fnDN( path );
  // Check the motionBlur attribute of the node
  MString motionBlurAttr;
  if ( path.hasFn(MFn::kPfxHair) ||
       path.hasFn(MFn::kPfxHair)  )
    motionBlurAttr = "motionBlurred";
  else
    motionBlurAttr = "motionBlur";

  MPlug vPlug = fnDN.findPlug( motionBlurAttr, &status );
  bool motionBlur = false;
  if ( status == MS::kSuccess ) {
    vPlug.getValue( motionBlur );
  }
  status.clear();

  return  motionBlur;
}


/** Check if this object and all of its parents are visible.
 *
 *  In Maya, visibility is determined by hierarchy.  So, if one of a node's
 *  parents is invisible, then so is the node.
 */
bool areObjectAndParentsVisible( const MDagPath & path )
{
  bool result = true;
  LIQDEBUGPRINTF( "-> getting searchpath\n" );
  MDagPath searchPath( path );
  MStatus status;

  // Philippe:
  // Check if the path belongs to the current render layers
  MFnRenderLayer renderLayer;

  LIQDEBUGPRINTF( "-> stepping through search path\n" );
  bool searching = true;
  bool isInCurrentRenderLayer = true;
  while ( searching ) {
    LIQDEBUGPRINTF( "-> checking visibility\n" );
    isInCurrentRenderLayer = renderLayer.inCurrentRenderLayer( path, &status );
    if ( !isInCurrentRenderLayer || !isObjectVisible( searchPath ) ) {
      result = false;
      searching = false;
    }
    if ( searchPath.length() == 1 ) searching = false;
    searchPath.pop();
  }

  return result;
}


/** Check if this object and all of its parents are visible.
 *
 *  In Maya, visibility is determined by hierarchy.  So, if one of a node's
 *  parents is invisible, then so is the node.
 */
bool areObjectAndParentsTemplated( const MDagPath & path )
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

/** Build the correct token/array pairs from the scene data to correctly pass
 *  to RenderMan.
 */
void assignTokenArrays( unsigned int numTokens, const liqTokenPointer tokenPointerArray[], RtToken tokens[], RtPointer pointers[] )
{
  for ( unsigned i( 0 ); i < numTokens; i++ ) {
    tokens[i] = const_cast< RtString >( const_cast< liqTokenPointer* >( &tokenPointerArray[i] )->getDetailedTokenName().c_str() );
    pointers[i] = const_cast< liqTokenPointer* >( &tokenPointerArray[i] )->getRtPointer();
  }
}

/** Build the correct token/array pairs from the scene data to correctly pass
 *  to RenderMan.
 *
 *  This is another version that takes a vector as input instead of a static array.
 */
void assignTokenArraysV( const vector<liqTokenPointer>& tokenPointerArray, RtToken tokens[], RtPointer pointers[] )
{
  unsigned i( 0 );
  for ( vector< liqTokenPointer >::const_iterator iter( tokenPointerArray.begin() ); iter != tokenPointerArray.end(); iter++, i++ ) {
    tokens[ i ] = const_cast< RtString >( const_cast< liqTokenPointer* >( &( *iter ) )->getDetailedTokenName().c_str() );
    pointers[ i ] = const_cast< liqTokenPointer* >( &( *iter ) )->getRtPointer();
  }
}

MObject findFacetShader( MObject mesh, int polygonIndex ){
  MFnMesh     fnMesh( mesh );
  MObjectArray shaders;
  MIntArray indices;
  MDagPath path;

  if (!fnMesh.getConnectedShaders( 0, shaders, indices )) {
    liquidMessage( "MFnMesh::getConnectedShaders", messageError );
  }

  MObject shaderNode = shaders[ indices[ polygonIndex ] ];

  return shaderNode;
}

/** Checks if a file exists.
 */
bool fileExists( const MString& filename ) {
#ifdef _WIN32
  struct _stat sbuf;
  int result = _stat( filename.asChar(), &sbuf );
    // under Win32, stat fails if path is a directory name ending in a slash
    // so we check for DIR/. Which works - go figure
    if( result && ( filename.rindex( '/' ) == filename.length() - 1 ) ) {
      result = _stat( ( filename + "." ).asChar(), &sbuf );
    }
#else
    struct stat sbuf;
  int result = stat( filename.asChar(), &sbuf );
#endif
    return ( result == 0 );
}

/** Checks if file1 is newer than file2
 */
bool fileIsNewer( const MString& file1, const MString& file2 ) {
#ifdef _WIN32
  struct _stat sbuf1, sbuf2;
  _stat( file1.asChar(), &sbuf1 );
  _stat( file2.asChar(), &sbuf2 );
#else
  struct stat sbuf1, sbuf2;
  stat( file1.asChar(), &sbuf1 );
  stat( file2.asChar(), &sbuf2 );
#endif

  return sbuf1.st_mtime > sbuf2.st_mtime;
}


bool fileFullyAccessible( const MString& path ) {
#ifdef _WIN32
  // Read & Write
  //cerr << "Moin " << string( path.asChar() ) << endl;
  //cerr << "Moin " << _access( path.asChar(), 6 ) << endl;
  return _access( path.asChar(), 6 ) != -1;
#else
  return !access( path.asChar(), R_OK | W_OK | X_OK | F_OK );
#endif

}

/** If transforms relative into absolute paths.
 *
 *  @return Full path
 */
MString getFullPathFromRelative ( const MString& filename ) {
  MString ret;
  extern MString liqglo_projectDir;

  if( filename.index( 0 ) != '/' ) // relative path, add prefix project folder
    ret = liqglo_projectDir + "/" + filename;
  else
    ret = filename;

  return ret;
}

MString getFileName ( const MString& fullpath ) {

  return fullpath.substring( fullpath.rindex('/') + 1, fullpath.length() - 1 );
}

/** Parse strings sent to Liquid to replace defined
 *  characters with specific variables.
 */
MString parseString( const MString& inputString, bool doEscaped )
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
      } else if ( tokenString == "PDIR" || tokenString == "PROJDIR" ) {
        constructedString += liqglo_projectDir;
        inToken = false;
        tokenString.clear();
      } else if ( tokenString == "RDIR" || tokenString == "RIBDIR" ) {
        constructedString += liqglo_ribDir;
        inToken = false;
        tokenString.clear();
      } else if ( tokenString == "TDIR" || tokenString == "TEXDIR" ) {
        constructedString += liqglo_textureDir;
        inToken = false;
        tokenString.clear();
      } else if ( tokenString == "OBJ" && inputString.substring(i+1, i+4) != "PATH" ) {
        constructedString += liqglo_currentNodeShortName;
        inToken = false;
        tokenString.clear();
      } else if ( tokenString == "OBJPATH" ) {
        constructedString += liqglo_currentNodeName;
        inToken = false;
        tokenString.clear();
      } else if ( tokenString == "SHOT" ) {
        constructedString += liqglo_shotName;
        inToken = false;
        tokenString.clear();
      } else if ( tokenString == "VER" ) {
        constructedString += liqglo_shotVersion;
        inToken = false;
        tokenString.clear();
      } else if ( tokenString == "LYR" || tokenString == "LAYER" ) {
        constructedString += liqglo_layer;
        inToken = false;
        tokenString.clear();
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
      MString envString;
      char* envVal = NULL;

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
    } else if ( doEscaped && inputString.substring(i + 1, i + 1 ) == "n" && inputString.substring(i, i) == "\\" ) {
      constructedString += "\n";
      i++;
    } else if ( doEscaped && inputString.substring(i + 1, i + 1 ) == "t" && inputString.substring(i, i) == "\\" ) {
      constructedString += "\t";
      i++;
    } else {
      constructedString += inputString.substring(i, i);
    }
  }

  // Moritz: now parse for MEL command sequences
  constructedString = parseCommandString( constructedString );

  return constructedString;
}

// Moritz: added below code for simple MEL parameter expression scripting support
// syntax: `mel commands`
MString parseCommandString( const MString& inputString )
{
  MString constructedString;
  MString tokenString;
  unsigned sLength = inputString.length();


  for ( unsigned i = 0; i < sLength; i++ ) {
    if ( inputString.substring(i, i) == "`" && inputString.substring(i - 1, i - 1) != "\\" ) {
      MString  melCmdString;
      i++;

      // loop through the string looking for the closing %
      if ( i < sLength ) {
        while ( i < sLength && inputString.substring(i, i) != "`" && inputString.substring(i - 1, i - 1) != "\\" ) {
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

  constructedString = removeEscapes( constructedString );

  return constructedString;
}

MString liquidTransGetSceneName()
{
  MString fullName;
  MGlobal::executeCommand( "file -q -sn -shn", fullName );
  fullName = (fullName != "")? fullName:"untitled.mb";

  // Move backwards across the string until we hit a dirctory / and
  // take the info from there on
  int i = fullName.rindex( '/' );
  int j = fullName.rindex( '.' );
  // From Maya 6, unsaved files have no extension anymore, we have
  // to account for this here as the ending delimiting '.' is missing
  if( ( j < i + 2 ) || ( j == -1 ) )
    j = fullName.length();

  return fullName.substring( i + 1, j - 1 );
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

/** Calculates a hashes from a string.
 */
liquidlong liquidHash(const char *str)
{
  LIQDEBUGPRINTF( "-> hashing string\n" );
  liquidlong hc = 0;

  while( *str ) {
    //hc = hc * 13 + *str * 27;   // old hash function
    hc = hc + *str;   // change this to a better hash func
    str++;
  }

  LIQDEBUGPRINTF( "-> done hashing string\n" );
  return (liquidlong)hc;
}

#ifdef _WIN32
char* basename( const char *filename ) {
//      returns the filename portion of a path
#ifdef MSVC6
  char *p = strrchr( filename, '/' );
#else
  char *p = const_cast< char* >( strrchr( filename, '/' ) );
#endif
  return p ? p + 1 : ( char* ) filename;
}
#endif

/** Converts '\' into '/'
 */
MString liquidSanitizePath( const MString& inputString ) {
  const string str( inputString.asChar() );
  string constructedString, buffer;

  for( unsigned i( 0 ); i < inputString.length(); i++ ) {
    if ( '\\' == str[ i ] ) {
      constructedString += "/";
    } else {
      constructedString += str.substr( i, 1 );
    }
  }

  return constructedString.c_str();
}

/** Convert <driveletter>: into //<driveletter>
 */
MString liquidSanitizeSearchPath( const MString& inputString ) {
  MString constructedString( liquidSanitizePath( inputString ) );

#if defined ( DELIGHT ) || defined ( PRMAN )
  // Convert from "C:/path" into "//C/path"
  if( inputString.substring( 1, 1 ) == ":" )
    constructedString = "//" + constructedString.substring( 0, 0 ) + constructedString.substring( 2, inputString.length() - 1 ).toLowerCase();
#endif // defined DELIGHT || PRMAN

  return constructedString;
}


string liquidSanitizePath( const string& inputString ) {
  string constructedString, buffer;

  for( unsigned i( 0 ); i < inputString.length(); i++ ) {
    if ( '\\' == inputString[ i ] ) {
      if ( '\\' == inputString[ i + 1 ] ) {
        ++i; // skip double slashes
        buffer = "\\";
      } else {
        buffer = "/";
      }
  } else {
      buffer = inputString.substr( i, 1 );
  }
    constructedString += buffer;
  }

  return constructedString;
}


string liquidSanitizeSearchPath( const string& inputString ) {
  string constructedString( liquidSanitizePath( inputString ) );

#if defined ( DELIGHT ) || defined ( PRMAN )
  // Convert from "C:/path" into "//C/path"
  if( inputString[ 1 ] == ':' ) {
    constructedString = "//" +
    constructedString.substr( 0, 1 )
    + to_lower_copy( constructedString.substr( 2 ) );
  }
#endif // defined DELIGHT || PRMAN

  return constructedString;
}

/** Get absolute pathnames for creating RIBs,
 *  archives and the renderscript in case the user
 *  has choosen to have all paths to be relative
 */
string liquidGetRelativePath( bool relative, const string& name, const string& dir ) {
  if( !relative && ( string::npos != name.find( '/' ) ) && ( ':' != name[ 1 ] ) ) {
    return dir + name;
  } else {
    return name;
  }
}


MString liquidGetRelativePath( bool relative, const MString& name, const MString& dir ) {
  if( !relative && ( 0 != name.index('/') ) && ( name.substring( 1, 1 ) != ":" ) ) {
    return dir + name;
  } else {
    return name;
  }
}


MString removeEscapes( const MString& inputString ) {
  MString constructedString;
  MString tokenString;
  int sLength = inputString.length();
  int i;

  for ( i = 0; i < sLength; i++ ) {
    if ( inputString.substring(i, i+1) == "\\@" ) {
      constructedString += "@";
      i++;
    } else if ( inputString.substring(i, i+1) == "\\#" ) {
      constructedString += "#";
      i++;
    } else if ( inputString.substring(i, i+1) == "\\[" ) {
      constructedString += "[";
      i++;
    } else if ( inputString.substring(i, i+1) == "\\]" ) {
      constructedString += "]";
      i++;
    } else constructedString += inputString.substring(i, i);
  }
  return constructedString;
}

MObject getNodeByName( MString name, MStatus *returnStatus )
{
  MObject node;
  MSelectionList list;

  *returnStatus = MGlobal::getSelectionListByName( name, list );

  if ( MS::kSuccess != *returnStatus ){
    MGlobal::displayError( "Cound't get node '" + name + "'. There might be multiple nodes with this name" );
    return node;
  }

  *returnStatus=list.getDependNode(0,node);

  if ( MS::kSuccess != *returnStatus ) {
    MGlobal::displayError("Cound't get node '"+ name + "'. There might be multiple nodes with this name" );
    return MObject::kNullObj;
  }

  return node;
}

string getEnvironment( const string& envVar )
{
  string ret;
  char* tmp( getenv( envVar.c_str() ) );
  if( tmp )
    ret = tmp;
  return ret;
}

vector< int > generateFrameNumbers( const string& seq ) {
  // We maintain a set to ensure we don't insert a frame into the list twice
  set< int > theSeq;
  vector< int > theFrames;

  typedef tokenizer< char_separator< char > > tokenizer;

  char_separator< char > comma( "," );
  tokenizer frames( seq, comma );

  for( tokenizer::iterator it( frames.begin() ); it != frames.end(); it++ ) {
      size_t pos( it->find( "-" ) );
      if( string::npos == pos ) {
          float f( ( float )atof( it->c_str() ) );
          if( theSeq.end() == theSeq.find( ( int )f ) ) {
              theSeq.insert( ( int )f );
              theFrames.push_back( ( int )f );
          }
      } else {
          float startFrame( ( float )atof( it->substr( 0, pos ).c_str() ) );
          float endFrame, frameStep;
          size_t pos2( it->find( "@" ) );
          size_t pos3( it->find( "x" ) );
          if( string::npos == pos2 ) {
            if( string::npos != pos3 ) {
              pos2 = pos3;
            }
          }
          // Support both RSP- & Shake frame sequence syntax
          if( string::npos == pos2 ) {
              endFrame = ( float )atof( it->substr( pos + 1 ).c_str() );
              frameStep = 1;
          }
          else {
              endFrame = ( float )atof( it->substr( pos + 1, pos2 - pos ).c_str() );
              frameStep = ( float )fabs( atof( it->substr( pos2 + 1 ).c_str() ) );
          }
          if( startFrame < endFrame ) {
              for( float f( startFrame ); f <= endFrame; f += frameStep ) {
                  if( theSeq.end() == theSeq.find( ( int )f ) ) {
                      theSeq.insert( ( int )f );
                      theFrames.push_back( ( int )f );
                  }
              }
              if( theSeq.end() == theSeq.find( ( int )endFrame ) )
                  theFrames.push_back( ( int )endFrame );
          }
          else {
              for( float f( startFrame ); f >= endFrame; f -= frameStep ) {
                  if( theSeq.end() == theSeq.find( ( int )f ) ) {
                      theSeq.insert( ( int )f );
                      theFrames.push_back( ( int )f );
                  }
              }
              if( theSeq.end() == theSeq.find( ( int )endFrame ) )
                  theFrames.push_back( ( int )endFrame );
          }
      }
  }

  return theFrames;
}


/**
 *  Create a full path
 */
bool makeFullPath( const string& name, int mode ) {

  // Get some space to store out tokenized string
  scoped_array< char > tmp( new char[ name.length() + 1 ] );

  // Need to copy the input string since strtok changes its input
  strncpy( tmp.get(), name.c_str(), name.length() + 1 );

  // Tokenize
  char* token( strtok( tmp.get(), "/" ) );

  // Our path for mkdir()
  string path( token );
  if( '/' == name[ 0 ] ) {
    path = string( "/" ) + path;
  }

  while( true ) {

#ifdef _WIN32
    // skip drive letter part of path
    if( !(  ( 2 == path.length() ) && ( ':' == path[ 1 ] ) ) ) {
#endif
      struct stat stats;
      if( stat( path.c_str(), &stats ) < 0 ) {
        // The path element didn't exist
  #ifdef _WIN32
        if( _mkdir( path.c_str() ) )
  #else
        if( mkdir( path.c_str(), mode ) )
  #endif
        {
          return false;
        }
      }

  #ifdef _WIN32
      WIN32_FIND_DATA FileData;
      FindFirstFile( path.c_str(), &FileData );
      if( !( FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
  #else
      if( !S_ISDIR( stats.st_mode ) )
  #endif
      {
        return false;
      }
#ifdef _WIN32
    }  // if( !(  ( 2 == path.length() ) && ( ':' == path[ 1 ] ) ) )
#endif

    // Tokenize
    token = strtok( NULL, "/" );
    if( !token ) {
      break;
    }

    path += string( "/" ) + token;
  }

  return true;
}


string sanitizeNodeName( const string& name ) {
  string newName( name );
  replace_all( newName, "|", "_" );
  replace_all( newName, ":", "_" );
  return newName;
}

MString sanitizeNodeName( const MString& name ) {
  string newName( name.asChar() );
  replace_all( newName, "|", "_" );
  replace_all( newName, ":", "_" );
  return MString( newName.c_str() );
}

RtString& getLiquidRibName( const string& name ) {
  static string ribName;
  static RtString tmp;
  ribName = sanitizeNodeName( name );
  tmp = const_cast< RtString >( ribName.c_str() );
  return tmp;
}

/** Standard function to send messages to either the
 *  maya console or the shell for user feedback.
 */
void liquidMessage( const string& msg, liquidVerbosityType type ) {
  if( liqglo_verbosity >= type ) {
    if ( !liquidBin ) {
      MString infoOutput( "[Liquid] " );
      infoOutput += msg.c_str();
      switch( type ) {
        case messageInfo:
          MGlobal::displayInfo( infoOutput );
          break;
        case messageWarning:
          MGlobal::displayWarning( infoOutput );
          break;
        case messageError:
          MGlobal::displayError( infoOutput );
      }
    } else {
      string infoOutput( "[Liquid] " );
      infoOutput += msg;
      switch( type ) {
        case messageWarning:
          cerr << "Warning: ";
          break;
        case messageError:
          cerr << "Error: ";
      }
      cerr << infoOutput << endl << flush;
    }
  }
}
