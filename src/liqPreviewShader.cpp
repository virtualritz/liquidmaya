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

/* liquid command to export a shader ball with the selected shader */

// Renderman Headers
extern "C" {
#include <ri.h>
#ifdef PRMAN
#include <slo.h>
#endif
}
#ifdef _WIN32
#include <process.h>
#include <malloc.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

// Maya's Headers
#include <maya/MFn.h>
#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MPxCommand.h>
#include <maya/MCommandResult.h>
#include <maya/MStringArray.h>
#include <maya/MArgList.h>
#include <maya/MSyntax.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MPlug.h>
#include <maya/MDoubleArray.h>
#include <maya/MGlobal.h>

#include <liquid.h>
#include <liqRenderer.h>
#include <liqShader.h>
#include <liqPreviewShader.h>
#include <liqMemory.h>
#include <liqGlobalHelpers.h>
#include <liqIOStream.h>

extern int debugMode;
extern liqRenderer liquidRenderer;


/**
 *  Creates a new instance of the command plug-in.
 *
 */
void* liqPreviewShader::creator()
{
    return new liqPreviewShader();
}

MSyntax liqPreviewShader::syntax()
{
  MSyntax syn;

  syn.addFlag("s","shader");
  syn.addFlag("r","renderer");
  syn.addFlag("dd","displayDriver");
  syn.addFlag("dn","displayName");
  syn.addFlag("ds","displaySize");
  syn.addFlag("sshn", "shortShaderNames");
  syn.addFlag("nbp","noBackPlane");
  syn.addFlag("os","objectSize");
  syn.addFlag("sr","shadingRate");
  syn.addFlag("p","pipe");
  syn.addFlag("t","type");

  syn.addFlag("sph", "sphere");
  syn.addFlag("tor", "torus");
  syn.addFlag("cyl", "cylinder");
  syn.addFlag("cub", "cube");
  syn.addFlag("pla", "plane");
  syn.addFlag("tea", "teapot");
  syn.addFlag("cst", "custom");

  return syn;
}


/**
 *  Class destructor.
 *
 */
liqPreviewShader::~liqPreviewShader()
{
}


typedef struct liqPreviewShoptions
{
  const char *shaderNodeName;
  const char *displayDriver;
  const char *displayName;
  const char *renderCommand;
  const char *backPlaneShader;
  bool    shortShaderName, backPlane, usePipe;
  int     displaySize;
  int     primitiveType;
  double  objectScale;
  double  shadingRate;
  MString customRibFile;
  bool    fullShaderPath;
  MString type;
} liqPreviewShoptions;

int liquidOutputPreviewShader( const char *fileName, liqPreviewShoptions *options );

#ifndef _WIN32
void liquidNewPreview( liqPreviewShoptions *options )
{
  int val = 0;
  int ret;
  if( !options->shaderNodeName || !options->displayDriver || !options->renderCommand )
  {
    cerr << "Invalid options for shader preview" << endl;
    pthread_exit( ( void * )&val);
  }

  fflush(NULL);
  // Open a pipe to a command
  FILE *fp = popen( options->renderCommand, "w");
  if( !fp )
  {
    char str[256];
    sprintf( str, "Opening pipe to %s ", options->renderCommand );
    perror( str );
    pthread_exit( ( void * )&val);
  }
  val = fork();
  if( val == -1 ) // Parent with error
  {
    perror( "Fork for RIB output" );
    pthread_exit( ( void * )&val);
  }
  else if( val == 0 ) // Child
  {
    int fd = fileno(fp);
    // Redirect stdout to pipe
    // Warning : messages should be sent to sdterr until stdout is restored
    ret = dup2( fd, 1 );
    if( ret < 0 )
    {
      perror( "Pipe redirect failed " );
      pthread_exit( ( void * )&val);
    }
    // And output RIB stdout
    cout << "# Outputing " << options->shaderNodeName << endl;
    liquidOutputPreviewShader( NULL, options );
    _exit(0);
  }
  cerr << "Waiting for process" << val << " to finish " << endl;
  ret = waitpid( val, NULL, 0 );
  cerr << "Waiting for " << options->renderCommand << " to finish" << endl;
  pclose( fp );   	// Wait until render finish
  val = 1;	// Set a "all is ok" returned value
  //cout << "Stdout is still open" << endl;
  LIQDEBUGPRINTF("-> Thread preview is done.\n" );
  pthread_exit( ( void * )&val);
}
#endif // ifndef _WIN32


MStatus	liqPreviewShader::doIt( const MArgList& args )
{
#if defined( PRMAN ) || defined( ENTROPY ) || defined( AQSIS ) || defined( DELIGHT ) || defined( PIXIE )
  MStatus status;
  int i;
  liqPreviewShoptions preview;
  preview.shortShaderName = false;
  preview.usePipe = false;
  preview.backPlane = true;
  preview.displaySize = 128;
  preview.primitiveType = SPHERE;
  preview.objectScale = 1.0;
  preview.shadingRate = 1.0;
  preview.customRibFile = "";
  preview.fullShaderPath = false;
  preview.type = "";

  MString displayDriver( "framebuffer" );
  MString displayName( "liqPreviewShader" );
  MString shaderNodeName;

  liquidRenderer.setRenderer();
  MString renderCommand( liquidRenderer.renderPreview );

  for ( i = 0; i < args.length(); i++ ) {
    MString arg = args.asString( i, &status );

    if ( ( arg == "-tea" ) || ( arg == "-teapot" ) )  {
      preview.primitiveType = TEAPOT;
    } else if ( ( arg == "-cube" ) || ( arg == "-box" ) ) {
      preview.primitiveType = CUBE;
    } else if ( arg == "-plane" )  {
      preview.primitiveType = PLANE;
    } else if ( arg == "-torus" )  {
      preview.primitiveType = TORUS;
    } else if ( arg == "-cylinder" ) {
      preview.primitiveType = CYLINDER;
    } else if ( arg == "-custom" )  {
      preview.primitiveType = CUSTOM;
      i++;
      preview.customRibFile = args.asString( i, &status );
    } else if ( ( arg == "-s" ) || ( arg == "-shader" ) ) {
      i++;
      shaderNodeName = args.asString( i, &status );
    } else if ( ( arg == "-r" ) || ( arg == "-renderer" ) ) {
      i++;
      renderCommand = args.asString( i, &status );
    } else if ( ( arg == "-dd" ) || ( arg == "-displayDriver" ) )  {
      i++;
      displayDriver = args.asString( i, &status );
    } else if ( ( arg == "-dn" ) || ( arg == "-displayName" ) ) {
      i++;
      displayName = args.asString( i, &status );
    } else if ( ( arg == "-ds" ) || ( arg == "-displaySize" ) ) {
      i++;
      MString argValue = args.asString( i, &status );
      preview.displaySize = argValue.asInt();
    } else if ( ( arg == "-sshn" ) || ( arg == "-shortShaderNames" ) ) {
      preview.shortShaderName = true;
    } else if ( ( arg == "-p" ) || ( arg == "-pipe" ) ) {
      preview.usePipe = true;
    } else if ( ( arg == "-nbp" ) || ( arg == "-noBackPlane" ) ) {
      preview.backPlane = false;
    } else if ( ( arg == "-os" ) || ( arg == "-objectSize" ) ) {
      i++;
      MString argValue = args.asString( i, &status );
      preview.objectScale = argValue.asDouble();
    } else if ( ( arg == "-sr" ) || ( arg == "-shadingRate" ) ) {
      i++;
      MString argValue = args.asString( i, &status );
      preview.shadingRate = argValue.asDouble();
    } else if ( ( arg == "-t" ) || ( arg == "-type" ) ) {
      i++;
      preview.type = args.asString( i, &status );
    }
  }

  // Check values
  if( shaderNodeName == "" )
  {
    cerr << "Need a shader name for previews" << endl;
    return MS::kFailure;
  }
  if( renderCommand == "" )
  {
    cerr << "Need a render command for previews" << endl;
    return MS::kFailure;
  }

  MString last3letters( shaderNodeName.substring( shaderNodeName.length()-3, shaderNodeName.length()-1 ) );

  if ( last3letters == liquidRenderer.shaderExtension ) preview.fullShaderPath = true;
  //cout <<"[liquid] last3letters = "<<last3letters<<endl;

  preview.shaderNodeName = shaderNodeName.asChar();
  preview.renderCommand = renderCommand.asChar();
  preview.displayDriver = displayDriver.asChar();
  preview.displayName = displayName.asChar();

  char *tempString = getenv( "LIQUIDHOME" );
  MString tempBackPlaneShader( tempString );

  if( tempBackPlaneShader == "" )
    tempBackPlaneShader = "null";
  else
    tempBackPlaneShader += "/shaders/liquidchecker";

  tempBackPlaneShader = liquidSanitizePath( tempBackPlaneShader );

  preview.backPlaneShader = tempBackPlaneShader.asChar();


#ifndef _WIN32 // Pipes don't work right under bloody Windoze
  if( preview.usePipe ) {

    LIQDEBUGPRINTF( "-> Creating thread preview.\n" );
    pthread_t prevthread;
    if( pthread_create( & prevthread, NULL, (void *(*)(void *)) liquidNewPreview, ( void * ) &preview ) ) {
      perror( "Thread create" );
      return MS::kFailure;
    }
    void * threadreturn;
    // Wait for end of rendering thread
    // must do so to make sure local variables always exist for the renderer
    pthread_join( prevthread, &threadreturn  );
    LIQDEBUGPRINTF( "-> End of thread preview.\n" );
  } else {
#endif

#ifdef _WIN32
    // Bad, better use global from liqRibTranslator
    tempString = getenv("TEMP");
    if( !tempString ) {
      tempString = getenv("TMP");
      if( !tempString ) {
        MGlobal::displayError( "Cannot write preview RIB. Please define either 'TMP' or 'TEMP' environment variables and restart Maya.");
        return MS::kFailure;
      }
    }

    MString tempRibName( tempString );
    LIQ_ADD_SLASH_IF_NEEDED( tempRibName );
#else
    MString tempRibName;
#endif
    char name[L_tmpnam];
    tmpnam( name );
    tempRibName += MString( name ) + ".liqTmp.rib";

    liquidOutputPreviewShader( tempRibName.asChar(), &preview );
#ifdef _WIN32
    _spawnlp( _P_WAIT, preview.renderCommand, preview.renderCommand, tempRibName.asChar(), NULL );
	FILE* fid = fopen( MString( displayName + ".done").asChar(), "w" );
	if(fid != 0)
		fclose(fid);

#else
    system( ( MString( preview.renderCommand ) + " " +  tempRibName + ";touch " + displayName + ".done&" ).asChar() );
  }
#endif
  return MS::kSuccess;
#else // PRMAN || ENTROPY || AQSIS || DELIGHT || PIXIE
  return MS::kFailure;
#endif
};

/**
 * Writes preview RIB into fileName for a shader
 * If fileName is RI_NULL : output to stdout
 * returns 1 on success
 */
int liquidOutputPreviewShader( const char *fileName, liqPreviewShoptions *options )
{
  // Get the Pathes in globals node
  MObject globalObjNode;
  MString liquidShaderPath = "",liquidTexturePath = "",liquidProceduralPath = "";
  MStatus status;
  MSelectionList globalList;
  status = globalList.add( "liquidGlobals" );
  if ( globalList.length() > 0 ) {
    status.clear();
    status = globalList.getDependNode( 0, globalObjNode );
    if ( status == MS::kSuccess ) {
	  MPlug shaderPlug;
	  MFnDependencyNode globalNode( globalObjNode );
      status.clear();
	  shaderPlug = globalNode.findPlug( "shaderPath", &status );
	  if ( status == MStatus::kSuccess )
        shaderPlug.getValue( liquidShaderPath );
      status.clear();
	  shaderPlug = globalNode.findPlug( "texturePath", &status );
	  if ( status == MStatus::kSuccess )
        shaderPlug.getValue( liquidTexturePath );
      status.clear();
	  shaderPlug = globalNode.findPlug( "proceduralPath", &status );
	  if ( status == MStatus::kSuccess )
        shaderPlug.getValue( liquidProceduralPath );
    }
  }

  if( fileName )
    RiBegin( const_cast<char *>(fileName) );
  else
    RiBegin( NULL );

  char* liquidPath = getenv("LIQUIDHOME");
  MString shaderPath = "&:@:.:~:" + MString(liquidPath) + "/shaders" + ":" + liquidShaderPath;
  RtString list = const_cast< char* > ( shaderPath.asChar() );
  RiOption( "searchpath", "shader", &list, RI_NULL );
  RtString texPath = const_cast< char* > ( liquidTexturePath.asChar() );
  if( texPath[0] != '\0' )
	  RiOption ("searchpath","texture", &texPath, NULL);
  RtString procPath = const_cast< char* > ( liquidProceduralPath.asChar() );
  if( procPath[0] != '\0' )
	  RiOption ("searchpath","procedural", &procPath, NULL);

  RiFrameBegin( 1 );
  RiShadingRate( ( options->shadingRate ) );
  RiPixelSamples( 3, 3 );
  RiPixelFilter( RiCatmullRomFilter, 3.0, 3.0 );
  RiFormat( (RtInt) options->displaySize, (RtInt) options->displaySize, 1 );
  if( options->backPlane )
  RiDisplay( const_cast<char *>( options->displayName ), const_cast<char *>( options->displayDriver ), RI_RGB, RI_NULL );
  else // Alpha might be useful
  RiDisplay( const_cast<char *>( options->displayName ), const_cast<char *>( options->displayDriver ), RI_RGBA, RI_NULL );
  RtFloat fov = 22.5;
  RiProjection( "perspective", "fov", &fov, RI_NULL );
  RiTranslate( 0, 0, 2.75 );
  RiWorldBegin();
  RiReverseOrientation();
  RiTransformBegin();
  RiRotate( -90.0, 1.0, 0.0, 0.0 );
  RiCoordinateSystem( "_environment" );
  RiTransformEnd();
  RtLightHandle ambientLightH, directionalLightH;
  RtFloat intensity;
  intensity = 0.05f;
  ambientLightH = RiLightSource( "ambientlight", "intensity", &intensity, RI_NULL );
  intensity = 0.9f;
  RtPoint from;
  RtPoint to;
  from[0] = -1.0; from[1] = 1.5; from[2] = -1.0;
  to[0] = 0.0; to[1] = 0.0; to[2] = 0.0;
  directionalLightH = RiLightSource( "distantlight", "intensity", &intensity, "from", &from, "to", &to, RI_NULL );
  intensity = 0.2f;
  from[0] = 1.3; from[1] = -1.2; from[2] = -1.0;
  directionalLightH = RiLightSource( "distantlight", "intensity", &intensity, "from", &from, "to", &to, RI_NULL );

  RiAttributeBegin();

  //cout <<"[liquid]  preview !"<<endl;

  char *shaderFileName;
  liqShader currentShader;
  MObject	shaderObj;

  if ( options->fullShaderPath ) {

    // a full shader path was specified
    //cout <<"[liquid] got full path for preview !"<<endl;

    //shaderFileName = const_cast<char*>(options->shaderNodeName);

    MString tmp(options->shaderNodeName);
    currentShader.file = tmp.substring( 0, tmp.length()-5 ).asChar();
    if ( options->type == "surface" ) currentShader.shader_type = SHADER_TYPE_SURFACE;
    else if ( options->type == "displacement" ) currentShader.shader_type = SHADER_TYPE_DISPLACEMENT;

    //cout <<"[liquid]   options->shaderNodeName = "<<options->shaderNodeName<<endl;
    //cout <<"[liquid]   options->type = "<<options->type<<endl;

  } else {

    // a shader node was specified

    MSelectionList shaderNameList;
    shaderNameList.add( options->shaderNodeName );

    shaderNameList.getDependNode( 0, shaderObj );
    if( shaderObj == MObject::kNullObj )
    {
      cerr << "Can't find node for " << options->shaderNodeName << endl;
      RiEnd();
      return 0;
    }
    currentShader = liqShader( shaderObj );

  }

  MFnDependencyNode assignedShader( shaderObj );

  RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * currentShader.numTPV );
  RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * currentShader.numTPV );

  assignTokenArrays( currentShader.numTPV, currentShader.tokenPointerArray, tokenArray, pointerArray );

  float displacementBounds = 0.0;
  MPlug tmpPlug;
  tmpPlug = assignedShader.findPlug( "displacementBound", &status );
  if ( status == MS::kSuccess )
    tmpPlug.getValue( displacementBounds );

  if ( displacementBounds > 0.0 ) {
    RtString coordsys = "shader";
    RiAttribute( "displacementbound", "sphere", &displacementBounds, "coordinatesystem", &coordsys, RI_NULL );
  }

  LIQ_GET_SHADER_FILE_NAME(shaderFileName, options->shortShaderName, currentShader );

  MString shadingSpace;
  tmpPlug = assignedShader.findPlug( "shaderSpace", &status );
  if ( status == MS::kSuccess ) tmpPlug.getValue( shadingSpace );
  if ( shadingSpace != "" ) {
    RiTransformBegin();
    RiCoordSysTransform( (char*) shadingSpace.asChar() );
  }



  RiTransformBegin();
  // Rotate shader space to make the preview more interesting
  RiRotate( 60.0, 1.0, 0.0, 0.0 );
  RiRotate( 60.0, 0.0, 1.0, 0.0 );
  RtFloat scale = 1 / options->objectScale;
  RiScale( scale, scale, scale );

  if ( currentShader.shader_type == SHADER_TYPE_SURFACE ) {
    RiColor( currentShader.rmColor );
    RiOpacity( currentShader.rmOpacity );
    if ( options->fullShaderPath )
      RiSurface( shaderFileName, RI_NULL );
    else
      RiSurfaceV( shaderFileName, currentShader.numTPV, tokenArray, pointerArray );
  } else if ( currentShader.shader_type == SHADER_TYPE_DISPLACEMENT ) {
    if ( options->fullShaderPath )
      RiDisplacement( shaderFileName, RI_NULL );
    else
      RiDisplacementV( shaderFileName, currentShader.numTPV, tokenArray, pointerArray );
  }

  RiTransformEnd();
  if ( shadingSpace != "" ) RiTransformEnd();

  switch( options->primitiveType ) {

    case CYLINDER: {
      RiReverseOrientation();
      RiScale( 0.95, 0.95, 0.95 );
      RiRotate( 60.0, 1.0, 0.0, 0.0 );
      RiTranslate( 0.0, 0.0, -0.05 );
      RiCylinder( 0.5, -0.3, 0.3, 360.0, RI_NULL );
      RiTranslate( 0.0, 0.0, 0.3 );
      RiTorus( 0.485, 0.015, 0.0, 90.0, 360.0, RI_NULL );
      RiDisk( 0.015, 0.485, 360.0, RI_NULL );
      RiTranslate( 0.0, 0.0, -0.6 );
      RiTorus( 0.485, 0.015, 270.0, 360.0, 360.0 );
      RiReverseOrientation();
      RiDisk( -0.015, 0.485, 360.0, RI_NULL );
      break;
    }
    case TORUS: {
      RiRotate( 45.0, 1.0, 0.0, 0.0 );
      RiTranslate( 0.0, 0.0, -0.05 );
      RiReverseOrientation();
      RiTorus( 0.3, 0.2, 0.0, 360.0, 360.0, RI_NULL );
      break;
    }
    case PLANE: {
      RiScale( 0.5, 0.5, 0.5 );
      RiReverseOrientation();
      static RtPoint plane[4] = {
        { -1.0,  1.0,  0.0 },
        {  1.0,  1.0,  0.0 },
        { -1.0, -1.0,  0.0 },
        {  1.0, -1.0,  0.0 }
      };
      RiPatch( RI_BILINEAR, RI_P, (RtPointer) plane, RI_NULL );
      break;
    }
    case TEAPOT: {
      RiTranslate( 0.06, -0.18, 0.0 );
      RiRotate( -120, 1.0, 0.0, 0.0 );
      RiRotate( 130.0, 0.0, 0.0, 1.0 );
      RiScale( 0.2, 0.2, 0.2 );
      RiGeometry( "teapot", RI_NULL );
      break;
    }
    case CUBE: {
      /* Lovely cube with rounded corners and edges */
      RiScale( 0.35, 0.35, 0.35 );
      RiRotate( 60.0, 1.0, 0.0, 0.0 );
      RiRotate( 60.0, 0.0, 0.0, 1.0 );

      RiTranslate( 0.11, 0.0, -0.08 );

      RiReverseOrientation();

      static RtPoint top[ 4 ] = { { -0.95, 0.95, -1.0 }, { 0.95, 0.95, -1.0 }, { -0.95, -0.95, -1.0 },  { 0.95, -0.95, -1.0 } };
      RiPatch( RI_BILINEAR, RI_P, ( RtPointer ) top, RI_NULL );

      static RtPoint bottom[ 4 ] = { { 0.95, 0.95, 1.0 }, { -0.95, 0.95, 1.0 }, { 0.95, -0.95, 1.0 }, { -0.95, -0.95, 1.0 } };
      RiPatch( RI_BILINEAR, RI_P, ( RtPointer ) bottom, RI_NULL );

      static RtPoint right[ 4 ] = { { -0.95, -1.0, -0.95 }, { 0.95, -1.0, -0.95 }, { -0.95, -1.0, 0.95 }, { 0.95, -1.0, 0.95 } };
      RiPatch( RI_BILINEAR, RI_P, ( RtPointer ) right, RI_NULL );

      static RtPoint left[ 4 ] = { { 0.95, 1.0, -0.95 }, { -0.95, 1.0, -0.95 }, { 0.95, 1.0, 0.95 }, { -0.95, 1.0, 0.95 } };
      RiPatch( RI_BILINEAR, RI_P, ( RtPointer ) left, RI_NULL );

      static RtPoint front[ 4 ] = { {-1.0, 0.95, -0.95 }, { -1.0, -0.95, -0.95 }, { -1.0, 0.95, 0.95 }, { -1.0, -0.95, 0.95 } };
      RiPatch( RI_BILINEAR, RI_P, ( RtPointer ) front, RI_NULL );

      static RtPoint back[ 4 ] = { { 1.0, -0.95, -0.95 }, { 1.0, 0.95, -0.95 }, { 1.0, -0.95, 0.95 }, { 1.0, 0.95, 0.95 } };
      RiPatch( RI_BILINEAR, RI_P, ( RtPointer ) back, RI_NULL );

      RiTransformBegin();
      RiTranslate( 0.95, 0.95, 0.0 );
      RiCylinder( 0.05, -0.95, 0.95, 90, RI_NULL );
      RiTransformEnd();

      RiTransformBegin();
      RiTranslate( 0.95, -0.95, 0.0 );
      RiRotate( -90.0, 0.0, 0.0, 1.0 );
      RiCylinder( 0.05, -0.95, 0.95, 90, RI_NULL );
      RiTransformEnd();

      RiTransformBegin();
      RiTranslate( -0.95, 0.95, 0.0 );
      RiRotate( 90.0, 0.0, 0.0, 1.0 );
      RiCylinder( 0.05, -0.95, 0.95, 90, RI_NULL );
      RiTransformEnd();

      RiTransformBegin();
      RiTranslate( -0.95, -0.95, 0.0 );
      RiRotate( 180.0, 0.0, 0.0, 1.0 );
      RiCylinder( 0.05, -0.95, 0.95, 90, RI_NULL );
      RiTransformEnd();

      RiTransformBegin();
      RiTranslate( 0.0, 0.0, 0.95 );

      RiTransformBegin();

      RiTransformBegin();
      RiTranslate( 0.95, 0.95, 0.0 );
      RiSphere( 0.05, 0.0, 0.05, 90, RI_NULL );
      RiTransformEnd();

      RiTransformBegin();
      RiTranslate( 0.95, -0.95, 0.0 );
      RiRotate( -90.0, 0.0, 0.0, 1.0 );
      RiSphere( 0.05, 0.0, 0.05, 90, RI_NULL );
      RiTransformEnd();

      RiRotate( 180.0, 0.0, 0.0, 1.0 );

      RiTransformBegin();
      RiTranslate( 0.95, 0.95, 0.0 );
      RiSphere( 0.05, 0.0, 0.05, 90, RI_NULL );
      RiTransformEnd();

      RiTransformBegin();
      RiTranslate( 0.95, -0.95, 0.0 );
      RiRotate( -90.0, 0.0, 0.0, 1.0 );
      RiSphere( 0.05, 0.0, 0.05, 90, RI_NULL );
      RiTransformEnd();

      RiTransformEnd();

      RiRotate( 90.0, 1.0, 0.0, 0.0 );

      RiTransformBegin();
      RiTranslate( 0.95, 0.0,  0.0 );
      RiCylinder( 0.05, -0.95, 0.95, 90, RI_NULL );
      RiTransformEnd();

      RiTransformBegin();
      RiTranslate( -0.95, 0.0,  0.0 );
      RiRotate( 90.0, 0.0, 0.0, 1.0 );
      RiCylinder( 0.05, -0.95, 0.95, 90, RI_NULL );
      RiTransformEnd();

      RiRotate( 90.0, 0.0, 1.0, 0.0 );

      RiTransformBegin();
      RiTranslate( 0.95, 0.0,  0.0 );
      RiCylinder( 0.05, -0.95, 0.95, 90, RI_NULL );
      RiTransformEnd();

      RiTransformBegin();
      RiTranslate( -0.95, 0.0,  0.0 );
      RiRotate( 90.0, 0.0, 0.0, 1.0 );
      RiCylinder( 0.05, -0.95, 0.95, 90, RI_NULL );
      RiTransformEnd();

      RiTransformEnd();

      RiTransformBegin();
      RiTranslate( 0.0, 0.0, -0.95 );

      RiTransformBegin();

      RiTransformBegin();
      RiTranslate( 0.95, 0.95, 0.0 );
      RiSphere( 0.05, -0.05, 0.0, 90, RI_NULL );
      RiTransformEnd();

      RiTransformBegin();
      RiTranslate( 0.95, -0.95, 0.0 );
      RiRotate( -90.0, 0.0, 0.0, 1.0 );
      RiSphere( 0.05, -0.05, 0.0, 90, RI_NULL );
      RiTransformEnd();

      RiRotate( 180.0, 0.0, 0.0, 1.0 );

      RiTransformBegin();
      RiTranslate( 0.95, 0.95, 0.0 );
      RiSphere( 0.05, -0.05, 0.0, 90, RI_NULL );
      RiTransformEnd();

      RiTransformBegin();
      RiTranslate( 0.95, -0.95, 0.0 );
      RiRotate( -90.0, 0.0, 0.0, 1.0 );
      RiSphere( 0.05, -0.05, 0.0, 90, RI_NULL );
      RiTransformEnd();

      RiTransformEnd();

      RiRotate( 90.0, 1.0, 0.0, 0.0 );

      RiTransformBegin();
      RiTranslate( -0.95, 0.0,  0.0 );
      RiRotate( 180.0, 0.0, 0.0, 1.0 );
      RiCylinder( 0.05, -0.95, 0.95, 90, RI_NULL );
      RiTransformEnd();

      RiTransformBegin();
      RiTranslate( 0.95, 0.0,  0.0 );
      RiRotate( -90.0, 0.0, 0.0, 1.0 );
      RiCylinder( 0.05, -0.95, 0.95, 90, RI_NULL );
      RiTransformEnd();

      RiRotate( 90.0, 0.0, 1.0, 0.0 );

      RiTransformBegin();
      RiTranslate( 0.95, 0.0,  0.0 );
      RiRotate( -90.0, 0.0, 0.0, 1.0 );
      RiCylinder( 0.05, -0.95, 0.95, 90, RI_NULL );
      RiTransformEnd();

      RiTransformBegin();
      RiTranslate( -0.95, 0.0,  0.0 );
      RiRotate( 180.0, 0.0, 0.0, 1.0 );
      RiCylinder( 0.05, -0.95, 0.95, 90, RI_NULL );
      RiTransformEnd();

      RiTransformEnd();

      break;
    }
    case CUSTOM: {
      cout <<"custom : "<<options->customRibFile<<endl;
      if ( fileExists( options->customRibFile ) ) {
        RiArchiveRecord( RI_VERBATIM, "ReadArchive \"%s\"\n", options->customRibFile.asChar() );
      }
      break;
    }
    case SPHERE:
    default: {
      RiRotate( 60.0, 1.0, 0.0, 0.0 );
      RiReverseOrientation();
      RiSphere( 0.5, -0.5, 0.5, 360.0, RI_NULL );
      break;
    }
  }

  RiAttributeEnd();

  /*
   * Backplane
   */
  if( options->backPlane ) {
    RiAttributeBegin();
    RiScale( 0.91, 0.91, 0.91 );
    RiSurface( ( RtToken ) options->backPlaneShader, RI_NULL );
    RtInt visible = 1;
    RtString transmission = "transparent";

    RiAttribute( "visibility", ( RtToken ) "camera", &visible, ( RtToken ) "trace", &visible, ( RtToken ) "transmission", ( RtPointer ) &transmission, RI_NULL );
    static RtPoint backplane[4] = {
      { -1.0,  1.0,  2.0 },
      {  1.0,  1.0,  2.0 },
      { -1.0, -1.0,  2.0 },
      {  1.0, -1.0,  2.0 }
    };
    RiPatch( RI_BILINEAR, RI_P, (RtPointer) backplane, RI_NULL );
    RiAttributeEnd();
  }

  RiWorldEnd();
  RiFrameEnd();
  RiEnd();
  fflush( NULL );

  LIQDEBUGPRINTF("-> Shader Preview RIB output done.\n" );

  return 1;
}




