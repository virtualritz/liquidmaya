/*
**
** The contents of this file are subject to the Mozilla Public License Version
** 1.1 (the "License"); you may not use this file except in compliance with
** the License. You may obtain a copy of the License at
** http://www.mozilla.org/MPL/
**
** Software distributed under the License is distributed on an "AS IS" basis,
** WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
** for the specific language governing rights and limitations under the
** License.
**
** The Original Code is the Liquid Rendering Toolkit.
**
** The Initial Developer of the Original Code is Colin Doncaster. Portions
** created by Colin Doncaster are Copyright (C) 2002. All Rights Reserved.
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


#ifdef OSX
  #include <stdlib.h>
#else
  #include <malloc.h>
#endif

#include <sys/types.h>

#ifndef _WIN32
#include <sys/time.h>
#include <sys/stat.h>
#endif

#ifndef _WIN32
// Dynamic Object Headers
#include <dlfcn.h>
#endif

#ifdef _WIN32
#pragma warning(disable:4786)
#endif

// win32 mkdir only has name arg
#ifdef _WIN32
#define MKDIR(_DIR_, _MODE_) (mkdir(_DIR_))
#else
#define MKDIR(_DIR_, _MODE_) (mkdir(_DIR_, _MODE_))
#endif

// Renderman Headers
extern "C" {
#include <ri.h>
}

#ifdef _WIN32
#include <process.h>
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <stdlib.h>
#include <alloca.h>
#endif

#if defined(_WIN32) && !defined(DEFINED_LIQUIDVERSION)
// unix build gets this from the Makefile
/*static const char *LIQUIDVERSION =
#include "liquid.version"
;*/
#define DEFINED_LIQUIDVERSION
#endif

#ifdef _WIN32
#  define RM_CMD "cmd.exe /c del"
#else
#  define RM_CMD "/bin/rm"
#endif

// Maya headers
#include <maya/MAnimControl.h>
#include <maya/MFileIO.h>
#include <maya/MFnLight.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItInstancer.h>
#include <maya/MItSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MSyntax.h>
#include <maya/MDistance.h>

// Liquid headers
#include <liquid.h>
#include <liqRibTranslator.h>
#include <liqGlobalHelpers.h>
#include <liqProcessLauncher.h>
#include <liqRenderer.h>

typedef int RtError;


// this get's set if we are running the commandline version of liquid
bool liquidBin;
int  debugMode;

// Kept global for liquidRigGenData and liquidRibParticleData
FILE        *liqglo_ribFP;
long         liqglo_lframe;
structJob    liqglo_currentJob;
bool         liqglo_doMotion;                         // Motion blur for transformations
bool         liqglo_doDef;                            // Motion blur for deforming objects
bool         liqglo_doCompression;                    // output compressed ribs
bool         liqglo_doBinary;                         // output binary ribs
RtFloat      liqglo_sampleTimes[LIQMAXMOTIONSAMPLES]; // current sample times
liquidlong   liqglo_motionSamples;                    // used to assign more than two motion blur samples!
float        liqglo_shutterTime;
bool         liqglo_doShadows;                        // Kept global for liquidRigLightData
MString      liqglo_sceneName;
bool         liqglo_beautyRibHasCameraName;           // if true, usual behaviour, otherwise, no camera name in beauty rib
bool         liqglo_isShadowPass;                     // true if we are rendering a shadow pass
bool         liqglo_expandShaderArrays;
bool         liqglo_shortShaderNames;                 // true if we don't want to output path names with shaders
bool         liqglo_relativeFileNames;                // true if we only want to output project relative names
MStringArray liqglo_DDimageName;
double       liqglo_FPS;                              // Frame-rate (for particle streak length)
bool         liqglo_outputMeshUVs;                    // true if we are writing uvs for subdivs/polys (in addition to "st")

// Kept global for liquidGlobalHelper
MString      liqglo_projectDir;
MString      liqglo_ribDir;
MString      liqglo_textureDir;
MString      liqglo_shaderPath;               // Shader searchpath
MString      liqglo_texturePath;             // Texture searchpath
MString      liqglo_archivePath;
MString      liqglo_proceduralPath;

// Kept global for liqRibNode.cpp
MStringArray liqglo_preReadArchive;
MStringArray liqglo_preRibBox;
MStringArray liqglo_preReadArchiveShadow;
MStringArray liqglo_preRibBoxShadow;

bool         liqglo_useMtorSubdiv;  // use mtor subdiv attributes
HiderType    liqglo_hider;
RtInt        liqglo_jitter;
MString      liqglo_makeTexture;    // MakeTexture utilite name

// Kept global for raytracing
bool         rt_useRayTracing;
RtFloat      rt_traceBreadthFactor;
RtFloat      rt_traceDepthFactor;
liquidlong   rt_traceMaxDepth;
RtFloat      rt_traceSpecularThreshold;
bool         rt_traceRayContinuation;
liquidlong   rt_traceCacheMemory;
bool         rt_traceDisplacements;
bool         rt_traceSampleMotion;
RtFloat      rt_traceBias;
liquidlong   rt_traceMaxDiffuseDepth;
liquidlong   rt_traceMaxSpecularDepth;

// Additionnal globals for organized people
MString      liqglo_shotName;
MString      liqglo_shotVersion;

#if 0
#ifdef _WIN32
// Hmmmmmmmm what's this ?
int RiNColorSamples;
#endif
// these are little storage variables to keep track of the current graphics state and will eventually be wrapped in
// a specific class
#endif


void liqRibTranslator::freeShaders( void )
{
  LIQDEBUGPRINTF( "-> freeing shader data.\n" );
  std::vector<liqShader>::iterator iter = m_shaders.begin();
  while ( iter != m_shaders.end() ) {
    int k = 0;
    while ( k < iter->numTPV ) {
      iter->freeShader( );
      k++;
    }
    ++iter;
  }
  m_shaders.clear();
  LIQDEBUGPRINTF( "-> finished freeing shader data.\n" );
}

// Hmmmmm should change magic to Liquid
MString liqRibTranslator::magic("##Liquid");

/**
 * Creates a new instance of the translator.
 */
void *liqRibTranslator::creator()
{
  return new liqRibTranslator();
}

// check shaders to see if "string" parameters are expression
// replace expression with calculated values
void liqRibTranslator::scanExpressions( liqShader & currentShader )
{
  for ( int i = 0; i < currentShader.numTPV; i++ ) {
    if ( currentShader.tokenPointerArray[i].getParameterType() == rString )
      processExpression( &currentShader.tokenPointerArray[i] );
    }
}

void liqRibTranslator::scanExpressions( liqRibLightData *light )
{
  if ( light != NULL ) {
    std::vector<liqTokenPointer>::iterator iter = light->tokenPointerArray.begin();
    while ( iter != (light->tokenPointerArray.end()) ) {
    if ( iter->getParameterType() == rString ) {
      liqTokenPointer i = *iter;
      processExpression( &i, light );
    }
     ++iter;
    }
  }
}

void liqRibTranslator::processExpression( liqTokenPointer *token, liqRibLightData *light )
{
  if ( token != NULL ) {
    char *strValue = token->getTokenString();
    LIQDEBUGPRINTF( "-> Expression: " );

    LIQDEBUGPRINTF( token->getTokenName() );

    LIQDEBUGPRINTF( "\n" );
    liqExpression expr( strValue );
    if ( expr.type != exp_None && expr.isValid ) { // we've got expression here
      expr.CalcValue(); // calculate value;
      switch ( expr.type ) {
        case exp_CoordSys:
          LIQDEBUGPRINTF( "-> CoordSys Expression: " );

          LIQDEBUGPRINTF( expr.GetValue().asChar() );

          LIQDEBUGPRINTF( "\n" );
          token->setTokenString( 0, expr.GetValue().asChar(), expr.GetValue().length() );
          break;

        case exp_MakeTexture:
          {
            token->setTokenString( 0, expr.GetValue().asChar(), expr.GetValue().length() );
            if ( !expr.destExists || !expr.destIsNewer ) {
              LIQDEBUGPRINTF( "-> Making Texture: " );

              LIQDEBUGPRINTF( liqglo_makeTexture.asChar() );

              LIQDEBUGPRINTF( "\n" );
              LIQDEBUGPRINTF( "-> MakeTexture Command: " );

              LIQDEBUGPRINTF( expr.GetCmd().asChar() );

              LIQDEBUGPRINTF( "\n" );

              structJob thisJob;
              thisJob.pass = rpMakeTexture;
              thisJob.renderName = liqglo_makeTexture;
              thisJob.ribFileName = expr.GetCmd();
              thisJob.imageName = expr.GetValue(); // destination file name

              std::vector<structJob>::iterator iter = txtList.begin();
              while ( iter != txtList.end() ) {
                if( iter->imageName == thisJob.imageName )
                  break; // already have this job
                ++iter;
              }
              txtList.push_back( thisJob );

            }
          }
          break;

        case exp_ReflectMap:
          LIQDEBUGPRINTF( "-> ReflectMap Expression: ")

          LIQDEBUGPRINTF( expr.GetValue().asChar() );

          LIQDEBUGPRINTF( "\n" );
          token->setTokenString( 0, expr.GetValue().asChar(), expr.GetValue().length() );
          break;

        case exp_Shadow:
        case exp_PointShadow:
          {
            MString shadowName = liqglo_textureDir + light->autoShadowName();
            token->setTokenString( 0, shadowName.asChar(), shadowName.length() );
          }
          break;

        case exp_EnvMap:
        case exp_CubeEnvMap:
        case exp_None:
        default:
          break;
      }
    }
  }
}


liqShader & liqRibTranslator::liqGetShader( MObject shaderObj )
{
  MString rmShaderStr;

  MFnDependencyNode shaderNode( shaderObj );
  MPlug rmanShaderNamePlug = shaderNode.findPlug( MString( "rmanShaderLong" ) );
  rmanShaderNamePlug.getValue( rmShaderStr );

  LIQDEBUGPRINTF( "-> Using Renderman Shader " );

  LIQDEBUGPRINTF( rmShaderStr.asChar() );
  LIQDEBUGPRINTF( "\n" );


  std::vector<liqShader>::iterator iter = m_shaders.begin();
  while ( iter != m_shaders.end() ){
    std::string shaderNodeName = shaderNode.name().asChar();
    if ( iter->name == shaderNodeName ) {
      // Already got it : nothing to do
      return *iter;
    }
    ++iter;
  }
  liqShader currentShader( shaderObj );
  m_shaders.push_back( currentShader );
  return m_shaders.back();
}

MStatus liqRibTranslator::liqShaderParseVectorAttr ( liqShader & currentShader, MFnDependencyNode & shaderNode, const char * argName, ParameterType pType )
{
  MStatus status = MS::kSuccess;
  MPlug triplePlug = shaderNode.findPlug( argName, &status );
  if ( status == MS::kSuccess ) {
    float x, y, z;
    currentShader.tokenPointerArray[currentShader.numTPV].set( argName, pType, false, false, false, 0 );
    triplePlug.child( 0 ).getValue( x );
    triplePlug.child( 1 ).getValue( y );
    triplePlug.child( 2 ).getValue( z );
    currentShader.tokenPointerArray[currentShader.numTPV].setTokenFloat( 0, x, y, z );
    currentShader.numTPV++;
  }
  return status;
}

void liqRibTranslator::printProgress( int stat, long first, long last, long where )
// for printing the progress to the Maya Console or stdout. If alfred is being used it
// will print it in a format that causes the correct formatting for the progress meters
//
// TODO - should be able to set the progress output format somehow to cater for
// different render pipelines - with a user-specifiable string in printf format?
{
  float numFrames  = ( last - first ) + 1;
  float framesDone = where - first;
  float statSize   = ( ( 1 / ( float )numFrames ) / 4 ) * ( float )stat * 100.0;
  float progressf  = ( ( ( float )framesDone / ( float )numFrames ) * 100.0 ) + statSize;
  int progress     = ( int ) progressf;

  if ( liquidBin ) {
    cout << "ALF_PROGRESS " << progress << "%\n" << flush;
  } else {
    MString progressOutput = "Progress: ";
    progressOutput += ( int )progress;
    progressOutput += "%";
    liquidInfo( progressOutput );
  }
}

/**
 * Checks to see if the liquidGlobals are available.
 */
bool liqRibTranslator::liquidInitGlobals()
{
  MStatus status;
  MSelectionList rGlobalList;
  status = rGlobalList.add( "liquidGlobals" );
  if ( rGlobalList.length() > 0 ) {
    status.clear();
    status = rGlobalList.getDependNode( 0, rGlobalObj );
    if ( status == MS::kSuccess ) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
  return false;
}

/**
 * Class constructor.
 */
liqRibTranslator::liqRibTranslator()
{
  m_systemTempDirectory = getenv("TMP");
  if( m_systemTempDirectory.length() == 0 ) {
    m_systemTempDirectory = getenv("TEMP");
    if( m_systemTempDirectory.length() == 0 ) {
      m_systemTempDirectory = getenv( "TEMPDIR");
      if( m_systemTempDirectory.length() == 0 ) {
#ifndef _WIN32
        m_systemTempDirectory = "/tmp";
#else
        m_systemTempDirectory = "%SystemRoot%/Temp";
#endif
      }
    }
  }

  m_rFilterX = 1;
  m_rFilterY = 1;
  m_rFilter = pfBoxFilter;

  liqglo_shortShaderNames = false;
  liqglo_relativeFileNames = false;

  m_showProgress = false;
  m_deferredBlockSize = 1;
  m_deferredGen = false;
  m_rgain = 1.0;
  m_rgamma = 1.0;
  m_outputHeroPass = true;
  m_outputShadowPass = false;
  m_ignoreLights = false;
  m_ignoreSurfaces = false;
  m_ignoreDisplacements = false;
  m_ignoreVolumes = false;
  m_renderAllCurves = false;
  m_renderSelected = false;
  m_exportReadArchive = false;
  useNetRman = false;
  remoteRender = false;
  useRenderScript = true;
  cleanRib = false;
  cleanRenderScript = false;
  liqglo_doBinary = false;
  liqglo_doCompression = false;
  doDof = false;
  launchRender = false;
  liqglo_doMotion = false;          // matrix motion blocks
  liqglo_doDef = false;             // geometry motion blocks
  doCameraMotion = false;           // camera motion blocks
  liqglo_rotateCamera = false;      // rotate the camera 90 degrees around Z axis
  doExtensionPadding = false;       // pad the frame number in the rib file names
  liqglo_doShadows = true;          // render shadows
  m_justRib = false;
  cleanShadows = 0;                 // render shadows
  cleanTextures = 0;                // render shadows
  frameFirst = 1;                   // range
  frameLast = 1;
  frameBy = 1;
  pixelSamples = 1;
  shadingRate = 1.0;
  depth = 1;
  outFormat = "it";
  m_animation = false;
  m_useFrameExt = true;  // Use frame extensions
  outExt = "tif";
  riboutput = "liquid.rib";
  m_renderer = PRMan;
  liqglo_motionSamples = 2;
  liqglo_FPS = 24.0;
  width = 360;
  height = 243;
  aspectRatio = 1.0;
  m_outPadding = 0;
  ignoreFilmGate = true;
  renderAllCameras = true;
  m_lazyCompute = false;
  m_outputShadersInShadows = false;
  m_outputShadersInDeepShadows = false;
  m_outputLightsInDeepShadows = false;
  m_alfredExpand = false;
#ifdef DEBUG
  debugMode = 1;
#else
  debugMode = 0;
#endif
  m_errorMode = 0;
  extension = ".rib";
  bucketSize[0] = 16;
  bucketSize[1] = 16;
  gridSize = 256;
  textureMemory = 2048;
  eyeSplits = 10;
  liqglo_shutterTime = 0.5;
  shutterConfig = OPEN_ON_FRAME;
  m_blurTime = 1.0;
  fullShadowRib = false;
  baseShadowName = "";
  quantValue = 8;
  liqglo_projectDir = m_systemTempDirectory;
  m_pixDir = "rmanpix/";
  m_tmpDir = "rmantmp/";
  m_ribDirG.clear();
  m_texDirG.clear();
  m_pixDirG.clear();
  m_tmpDirG.clear();
  liqglo_preReadArchive.clear();
  liqglo_preRibBox.clear();
  m_alfredTags.clear();
  m_alfredServices.clear();
  m_defGenKey.clear();
  m_defGenService.clear();
  m_preFrameMel.clear();
  m_postFrameMel.clear();
  m_preCommand.clear();
  m_preJobCommand.clear();
  m_postJobCommand.clear();
  m_postFrameCommand.clear();
  m_preFrameCommand.clear();
  m_outputComments = false;
  m_shaderDebug = false;
  // raytracing
  rt_useRayTracing = false;
  rt_traceBreadthFactor = 1.0;
  rt_traceDepthFactor = 1.0;
  rt_traceMaxDepth = 10;
  rt_traceSpecularThreshold = 10.0;
  rt_traceRayContinuation = true;
  rt_traceCacheMemory = 30720;
  rt_traceDisplacements = false;
  rt_traceSampleMotion = false;
  rt_traceBias = 0.05;
  rt_traceMaxDiffuseDepth = 2;
  rt_traceMaxSpecularDepth = 2;
  liqglo_shotName.clear();
  liqglo_shotVersion.clear();
#ifdef AIR
  m_renderCommand = "air";
#elif defined( AQSIS )
  m_renderCommand = "aqsis";
#elif defined( DELIGHT )
  m_renderCommand = "renderdl";
#elif defined( PIXIE )
  m_renderCommand = "rndr";
#elif defined( PRMAN )
  #ifdef _WIN32
  m_renderCommand = "prman";
  #else
  m_renderCommand = "render";
  #endif
#endif
  m_ribgenCommand = "liquid";

  createOutputDirectories = true;

  liqglo_expandShaderArrays = false;

  // Display Driver Defaults
  m_numDisplayDrivers = 0;
  m_DDParams.clear();
  liqglo_DDimageName.clear();
  m_DDimageType.clear();
  m_DDimageMode.clear();
  m_DDparamType.clear();
  m_DDenabled.clear();

  m_minCPU = m_maxCPU = 1;
  m_cropX1 = m_cropY1 = 0.0;
  m_cropX2 = m_cropY2 = 1.0;
  liqglo_isShadowPass = false;

  m_preWorldRIB.clear();
  m_postWorldRIB.clear();

  m_preGeomRIB.clear();

  m_renderScriptFormat = XML;

  liqglo_useMtorSubdiv = false;
  liqglo_hider = htHidden;
  liqglo_jitter = 1;

  liqglo_shaderPath = "&:@:.:~:rmanshader";

  liqglo_texturePath = "&:@:.:~:rmantex";

  liqglo_archivePath = "&:@:.:~:rib";

  liqglo_proceduralPath = "&:@:.:~";



  liqglo_ribDir = "rib";
  liqglo_textureDir = "rmantex";


  MString tmphome( getenv( "LIQUIDHOME" ) );

  if( tmphome != "" ) {
    liqglo_shaderPath += ":" + liquidSanitizePath( tmphome ) + "/shaders";

    liqglo_texturePath += ":" + liquidSanitizePath( tmphome ) + "/rmantex";

    liqglo_archivePath += ":" + liquidSanitizePath( tmphome ) + "/rib";
  }
}


/**
 * Class destructor
 */
liqRibTranslator::~liqRibTranslator()
{
  // this is crashing under Win32
  //#ifdef _WIN32
  // lfree( m_systemTempDirectory );
  //#endif
  LIQDEBUGPRINTF( "-> dumping unfreed memory.\n" );
  if ( debugMode ) ldumpUnfreed();
}

/**
 * Error handling function.
 * This gets called when the RIB library detects an error.
 */
#if defined DELIGHT || ENTROPY || PIXIE || PRMAN
void liqRibTranslatorErrorHandler( RtInt code, RtInt severity, char * message )
#else
void liqRibTranslatorErrorHandler( RtInt code, RtInt severity, const char * message )
#endif
{
  printf( "The renderman library is reporting and error! Code: %d  Severity: %d", code, severity );
  MString error( message );
  throw error;
}

MSyntax liqRibTranslator::syntax()
{
  MSyntax syntax;

  syntax.addFlag("lr",  "launchRender");
  syntax.addFlag("nolr", "noLaunchRender");
  syntax.addFlag("GL",  "useGlobals");
  syntax.addFlag("sel", "selected");
  syntax.addFlag("ra",  "readArchive");
  syntax.addFlag("acv", "allCurves");
  syntax.addFlag("tif", "tiff");
  syntax.addFlag("dof", "dofOn");
  syntax.addFlag("bin", "doBinary");
  syntax.addFlag("sh",  "shadows");
  syntax.addFlag("nsh", "noShadows");
  syntax.addFlag("zip", "doCompression");
  syntax.addFlag("cln", "cleanRib");
  syntax.addFlag("pro", "progress");
  syntax.addFlag("mb",  "motionBlur");
  syntax.addFlag("db",  "deformationBlur");
  syntax.addFlag("d",   "debug");
  syntax.addFlag("net", "netRender");
  syntax.addFlag("fsr", "fullShadowRib");
  syntax.addFlag("rem", "remote");
  syntax.addFlag("rs",  "renderScript");
  syntax.addFlag("nrs", "noRenderScript");
  syntax.addFlag("err", "errHandler");
  syntax.addFlag("sdb", "shaderDebug");
  syntax.addFlag("n",   "sequence", MSyntax::kLong, MSyntax::kLong, MSyntax::kLong);
  syntax.addFlag("m",   "mbSamples", MSyntax::kLong);
  syntax.addFlag("dbs", "defBlock");
  syntax.addFlag("cam", "camera",  MSyntax::kString);
  syntax.addFlag("rcam", "rotateCamera");
  syntax.addFlag("s",   "samples", MSyntax::kLong);
  syntax.addFlag("rnm", "ribName", MSyntax::kString);
  syntax.addFlag("pd",  "projectDir", MSyntax::kString);
  syntax.addFlag("prm", "preFrameMel",  MSyntax::kString);
  syntax.addFlag("pom", "postFrameMel", MSyntax::kString);
  syntax.addFlag("rid", "ribdir", MSyntax::kString);
  syntax.addFlag("txd", "texdir", MSyntax::kString);
  syntax.addFlag("tmd", "tmpdir", MSyntax::kString);
  syntax.addFlag("pid", "picdir", MSyntax::kString);
  syntax.addFlag("pec", "preCommand", MSyntax::kString);
  syntax.addFlag("poc", "postJobCommand",   MSyntax::kString);
  syntax.addFlag("pof", "postFrameCommand", MSyntax::kString);
  syntax.addFlag("prf", "preFrameCommand",  MSyntax::kString);
  syntax.addFlag("rec", "renderCommand",    MSyntax::kString);
  syntax.addFlag("rgc", "ribgenCommand",    MSyntax::kString);
  syntax.addFlag("blt", "blurTime",    MSyntax::kDouble);
  syntax.addFlag("sr",  "shadingRate", MSyntax::kDouble);
  syntax.addFlag("bs",  "bucketSize",  MSyntax::kLong, MSyntax::kLong);
  syntax.addFlag("pf",  "pixelFilter", MSyntax::kLong, MSyntax::kLong, MSyntax::kLong);
  syntax.addFlag("gs",  "gridSize",  MSyntax::kLong);
  syntax.addFlag("txm", "texmem",    MSyntax::kLong);
  syntax.addFlag("es",  "eyeSplits", MSyntax::kLong);
  syntax.addFlag("ar",  "aspect",    MSyntax::kDouble);
  syntax.addFlag("x",   "width",     MSyntax::kLong);
  syntax.addFlag("y",   "height",    MSyntax::kLong);
  syntax.addFlag("ndf", "noDef");
  syntax.addFlag("pad", "padding", MSyntax::kLong);
  syntax.addFlag("rgo", "ribGenOnly");

  return syntax;
}

/**
 * Read the values from the command line and set the internal values.
 */
MStatus liqRibTranslator::liquidDoArgs( MArgList args )
{

  MStatus status;
  MString argValue;

  LIQDEBUGPRINTF( "-> processing arguments\n" );

  // Parse the arguments and set the options.
  if ( args.length() == 0 ) {
    liquidInfo( "Doing nothing, no parameters given." );
    return MS::kFailure;
  }

  // find the activeView for previews;
  m_activeView = M3dView::active3dView();
  width        = m_activeView.portWidth();
  height       = m_activeView.portHeight();

  // get the current project directory
  MString MELCommand = "workspace -q -rd";
  MString MELReturn;
  MGlobal::executeCommand( MELCommand, MELReturn );
  liqglo_projectDir = MELReturn;


  LIQDEBUGPRINTF( "-> using path: " );

  LIQDEBUGPRINTF( liqglo_projectDir.asChar() );

  LIQDEBUGPRINTF( "\n" );


  // get the current scene name
  liqglo_sceneName = liquidTransGetSceneName();

  // setup default animation parameters
  frameFirst = ( int ) MAnimControl::currentTime().as( MTime::uiUnit() );
  frameLast  = ( int ) MAnimControl::currentTime().as( MTime::uiUnit() );
  frameBy    = 1;

  // check to see if the correct project directory was found
  if ( !fileExists( liqglo_projectDir ) ) {
    liqglo_projectDir = m_systemTempDirectory;
    cout <<"Liquid -> trying to set project dir to system tmp :"<<liqglo_projectDir<<endl;
  }
  LIQ_ADD_SLASH_IF_NEEDED( liqglo_projectDir );
  if ( !fileExists( liqglo_projectDir ) ) {
    MGlobal::displayWarning ( "Liquid -> Cannot find Project Directory, " + liqglo_projectDir + ", defaulting to system temp directory!\n" );
    cout <<"Liquid -> Cannot find Project Directory, "<<liqglo_projectDir<<", defaulting to system temp directory!"<<endl;
    liqglo_projectDir = m_systemTempDirectory;
  }

  for (unsigned int i = 0; i < args.length(); i++ ) {
    MString arg = args.asString( i, &status );
    if ((arg == "-lr") || (arg == "-launchRender")) {
      LIQCHECKSTATUS(status, "error in -launchRender parameter");
      launchRender = true;
    } else if ((arg == "-nolr") || (arg == "-noLaunchRender")) {
      LIQCHECKSTATUS(status, "error in -noLaunchRender parameter");
      launchRender = false;
    } else if ((arg == "-GL") || (arg == "-useGlobals")) {
      LIQCHECKSTATUS(status, "error in -useGlobals parameter");
      //load up all the render global parameters!
      if ( liquidInitGlobals() ) liquidReadGlobals();
    } else if ((arg == "-sel") || (arg == "-selected")) {
      LIQCHECKSTATUS(status, "error in -selected parameter");
      m_renderSelected = true;
    } else if ((arg == "-ra") || (arg == "-readArchive")) {
      LIQCHECKSTATUS(status, "error in -readArchive parameter");
      m_exportReadArchive = true;
    } else if ((arg == "-acv") || (arg == "-allCurves")) {
      LIQCHECKSTATUS(status, "error in -allCurves parameter" );
      m_renderAllCurves = true;
    } else if ((arg == "-tif") || (arg == "-tiff")) {
      LIQCHECKSTATUS(status, "error in -tiff parameter");
      outFormat = "tiff";
    } else if ((arg == "-dof") || (arg == "-dofOn")) {
      LIQCHECKSTATUS(status, "error in -dofOn parameter");
      doDof = true;
    } else if ((arg == "-bin") || (arg == "-doBinary")) {
      LIQCHECKSTATUS(status, "error in -doBinary parameter");
      liqglo_doBinary = true;
    } else if ((arg == "-sh") || (arg == "-shadows")) {
      LIQCHECKSTATUS(status, "error in -shadows parameter");
      liqglo_doShadows = true;
    } else if ((arg == "-nsh") || (arg == "-noShadows")) {
      LIQCHECKSTATUS(status, "error in -noShadows parameter");
      liqglo_doShadows = false;
    } else if ((arg == "-zip") || (arg == "-doCompression")) {
      LIQCHECKSTATUS(status, "error in -doCompression parameter");
      liqglo_doCompression = true;
    } else if ((arg == "-cln") || (arg == "-cleanRib")) {
      LIQCHECKSTATUS(status, "error in -cleanRib parameter");
      cleanRib = true;
    } else if ((arg == "-pro") || (arg == "-progress")) {
      LIQCHECKSTATUS(status, "error in -progress parameter");
      m_showProgress = true;
    } else if ((arg == "-mb") || (arg == "-motionBlur")) {
      LIQCHECKSTATUS(status, "error in -motionBlur parameter");
      liqglo_doMotion = true;
    } else if ((arg == "-db") || (arg == "-deformationBlur")) {
      LIQCHECKSTATUS(status, "error in -deformationBlur parameter");
      liqglo_doDef = true;
    } else if ((arg == "-d") || (arg == "-debug")) {
      LIQCHECKSTATUS(status, "error in -debug parameter");
      debugMode = 1;
    } else if ((arg == "-net") || (arg == "-netRender")) {
      LIQCHECKSTATUS(status, "error in -netRender parameter");
      useNetRman = true;
    } else if ((arg == "-fsr") || (arg == "-fullShadowRib")) {
      LIQCHECKSTATUS(status, "error in -fullShadowRib parameter");
      fullShadowRib = true;
    } else if ((arg == "-rem") || (arg == "-remote")) {
      LIQCHECKSTATUS(status, "error in -remote parameter");
      remoteRender = true;
    } else if ((arg == "-rs") || (arg == "-renderScript")) {
      LIQCHECKSTATUS(status, "error in -renderScript parameter");
      useRenderScript = true;
    } else if ((arg == "-nrs") || (arg == "-noRenderScript")) {
      LIQCHECKSTATUS(status, "error in -noRenderScript parameter");
      useRenderScript = false;
    } else if ((arg == "-err") || (arg == "-errHandler")) {
      LIQCHECKSTATUS(status, "error in -errHandler parameter");
      m_errorMode = 1;
    } else if ((arg == "-sdb") || (arg == "-shaderDebug")) {
      LIQCHECKSTATUS(status, "error in -shaderDebug parameter");
      m_shaderDebug = true;
    } else if ((arg == "-n") || (arg == "-sequence")) {
      LIQCHECKSTATUS(status, "error in -sequence parameter");   i++;
      argValue = args.asString( i, &status );
      frameFirst = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -sequence parameter");  i++;
      argValue = args.asString( i, &status );
      frameLast = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -sequence parameter");  i++;
      argValue = args.asString( i, &status );
      frameBy = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -sequence parameter");
      m_animation = true;
    } else if ((arg == "-m") || (arg == "-mbSamples")) {
      LIQCHECKSTATUS(status, "error in -mbSamples parameter");   i++;
      argValue = args.asString( i, &status );
      liqglo_motionSamples = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -mbSamples parameter");
    } else if ((arg == "-dbs") || (arg == "-defBlock")) {
      LIQCHECKSTATUS(status, "error in -defBlock parameter");   i++;
      argValue = args.asString( i, &status );
      m_deferredBlockSize = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -defBlock parameter");
    } else if ((arg == "-cam") || (arg == "-camera")) {
      LIQCHECKSTATUS(status, "error in -camera parameter");   i++;
      renderCamera = args.asString( i, &status );
      LIQCHECKSTATUS(status, "error in -camera parameter");
    } else if ((arg == "-rcam") || (arg == "-rotateCamera")) {
      LIQCHECKSTATUS(status, "error in -rotateCamera parameter");
      liqglo_rotateCamera = true;
    } else if ((arg == "-s") || (arg == "-samples")) {
      LIQCHECKSTATUS(status, "error in -samples parameter");  i++;
      argValue = args.asString( i, &status );
      pixelSamples = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -samples parameter");
    } else if ((arg == "-rnm") || (arg == "-ribName")) {
      LIQCHECKSTATUS(status, "error in -ribName parameter");  i++;
      liqglo_sceneName = args.asString( i, &status );
      LIQCHECKSTATUS(status, "error in -ribName parameter");
    } else if ((arg == "-pd") || (arg == "-projectDir")) {
      LIQCHECKSTATUS(status, "error in -projectDir parameter");  i++;
      MString parsingString = args.asString( i, &status );
      liqglo_projectDir = parseString( parsingString );
      LIQ_ADD_SLASH_IF_NEEDED( liqglo_projectDir );
      if ( !fileExists( liqglo_projectDir ) ) {
        cout << "Liquid -> Cannot find /project dir, defaulting to system temp directory!\n" << flush;
        liqglo_projectDir = m_systemTempDirectory;
      }
      LIQCHECKSTATUS(status, "error in -projectDir parameter");
    } else if ((arg == "-prm") || (arg == "-preFrameMel")) {
      LIQCHECKSTATUS(status, "error in -preFrameMel parameter");  i++;
      MString parsingString = args.asString( i, &status );
      m_preFrameMel = parseString( parsingString );
      LIQCHECKSTATUS(status, "error in -preFrameMel parameter");
    } else if ((arg == "-pom") || (arg == "-postFrameMel")) {
      LIQCHECKSTATUS(status, "error in -postFrameMel parameter");  i++;
      MString parsingString = args.asString( i, &status );
      m_postFrameMel = parseString( parsingString );
      LIQCHECKSTATUS(status, "error in -postFrameMel parameter");
    } else if ((arg == "-rid") || (arg == "-ribdir")) {
      LIQCHECKSTATUS(status, "error in -ribDir parameter");  i++;
      MString parsingString = args.asString( i, &status );
      liqglo_ribDir = parseString( parsingString );
      LIQCHECKSTATUS(status, "error in -ribDir parameter");
    } else if ((arg == "-txd") || (arg == "-texdir")) {
      LIQCHECKSTATUS(status, "error in -texDir parameter");  i++;
      MString parsingString = args.asString( i, &status );
      liqglo_textureDir = parseString( parsingString );
      LIQCHECKSTATUS(status, "error in -texDir parameter");
    } else if ((arg == "-tmd") || (arg == "-tmpdir")) {
      LIQCHECKSTATUS(status, "error in -tmpDir parameter");  i++;
      MString parsingString = args.asString( i, &status );
      m_tmpDir = parseString( parsingString );
      LIQCHECKSTATUS(status, "error in -tmpDir parameter");
    } else if ((arg == "-pid") || (arg == "-picdir")) {
      LIQCHECKSTATUS(status, "error in -picDir parameter");  i++;
      MString parsingString = args.asString( i, &status );
      m_pixDir = parseString( parsingString );
      LIQCHECKSTATUS(status, "error in -picDir parameter");
    } else if ((arg == "-pec") || (arg == "-preCommand")) {
      LIQCHECKSTATUS(status, "error in -preCommand parameter");  i++;
      m_preCommand = args.asString( i, &status );
      LIQCHECKSTATUS(status, "error in -preCommand parameter");
    } else if ((arg == "-poc") || (arg == "-postJobCommand")) {
      LIQCHECKSTATUS(status, "error in -postJobCommand parameter");  i++;
      MString varVal = args.asString( i, &status );
      m_postJobCommand = parseString( varVal );
      LIQCHECKSTATUS(status, "error in -postJobCommand parameter");
    } else if ((arg == "-pof") || (arg == "-postFrameCommand")) {
      LIQCHECKSTATUS(status, "error in -postFrameCommand parameter");  i++;
      m_postFrameCommand = args.asString( i, &status );
      LIQCHECKSTATUS(status, "error in -postFrameCommand parameter");
    } else if ((arg == "-prf") || (arg == "-preFrameCommand")) {
      LIQCHECKSTATUS(status, "error in -preFrameCommand parameter");  i++;
      m_preFrameCommand = args.asString( i, &status );
      LIQCHECKSTATUS(status, "error in -preFrameCommand parameter");
    } else if ((arg == "-rec") || (arg == "-renderCommand")) {
      LIQCHECKSTATUS(status, "error in -renderCommand parameter");  i++;
      m_renderCommand = args.asString( i, &status );
      LIQCHECKSTATUS(status, "error in -renderCommand parameter");
    } else if ((arg == "-rgc") || (arg == "-ribgenCommand")) {
      LIQCHECKSTATUS(status, "error in -ribgenCommand parameter");  i++;
      m_ribgenCommand = args.asString( i, &status );
      LIQCHECKSTATUS(status, "error in -ribgenCommand parameter");
    } else if ((arg == "-blt") || (arg == "-blurTime")) {
      LIQCHECKSTATUS(status, "error in -blurTime parameter");  i++;
      argValue = args.asString( i, &status );
      m_blurTime = argValue.asDouble();
      LIQCHECKSTATUS(status, "error in -blurTime parameter");
    } else if ((arg == "-sr") || (arg == "-shadingRate")) {
      LIQCHECKSTATUS(status, "error in -shadingRate parameter");  i++;
      argValue = args.asString( i, &status );
      shadingRate = argValue.asDouble();
      LIQCHECKSTATUS(status, "error in -shadingRate parameter");
    } else if ((arg == "-bs") || (arg == "-bucketSize")) {
      LIQCHECKSTATUS(status, "error in -bucketSize parameter");  i++;
      argValue = args.asString( i, &status );
      bucketSize[0] = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -bucketSize parameter");  i++;
      argValue = args.asString( i, &status );
      bucketSize[1] = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -bucketSize parameter");
    } else if ((arg == "-pf") || (arg == "-pixelFilter")) {
      LIQCHECKSTATUS(status, "error in -pixelFilter parameter");  i++;
      argValue = args.asString( i, &status );
      m_rFilter = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -pixelFilter parameter");  i++;
      argValue = args.asString( i, &status );
      m_rFilterX = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -pixelFilter parameter");  i++;
      argValue = args.asString( i, &status );
      m_rFilterY = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -pixelFilter parameter");
    } else if ((arg == "-gs") || (arg == "-gridSize")) {
      LIQCHECKSTATUS(status, "error in -gridSize parameter");  i++;
      argValue = args.asString( i, &status );
      gridSize = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -gridSize parameter");
    } else if ((arg == "-txm") || (arg == "-texmem")) {
      LIQCHECKSTATUS(status, "error in -texmem parameter");  i++;
      argValue = args.asString( i, &status );
      textureMemory = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -texmem parameter");
    } else if ((arg == "-es") || (arg == "-eyeSplits")) {
      LIQCHECKSTATUS(status, "error in -eyeSplits parameter");  i++;
      argValue = args.asString( i, &status );
      eyeSplits = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -eyeSplits parameter");
    } else if ((arg == "-ar") || (arg == "-aspect")) {
      LIQCHECKSTATUS(status, "error in -aspect parameter");  i++;
      argValue = args.asString( i, &status );
      aspectRatio = argValue.asDouble();
      LIQCHECKSTATUS(status, "error in -aspect parameter");
    } else if ((arg == "-x") || (arg == "-width")) {
      LIQCHECKSTATUS(status, "error in -width parameter");  i++;
      argValue = args.asString( i, &status );
      width = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -width parameter");
    } else if ((arg == "-y") || (arg == "-height")) {
      LIQCHECKSTATUS(status, "error in -height parameter");  i++;
      argValue = args.asString( i, &status );
      height = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -height parameter");
    } else if ((arg == "-ndf") || (arg == "-noDef")) {
      LIQCHECKSTATUS(status, "error in -noDef parameter");
      m_deferredGen = false;
    } else if ((arg == "-pad") || (arg == "-padding")) {
      LIQCHECKSTATUS(status, "error in -padding parameter");  i++;
      argValue = args.asString( i, &status );
      m_outPadding = argValue.asInt();
      LIQCHECKSTATUS(status, "error in -padding parameter");
    } else if ((arg == "-rgo") || (arg == "-ribGenOnly")) {
      LIQCHECKSTATUS(status, "error in -ribGenOnly parameter");
      m_justRib = true;
    }
  }

  if ( !liqglo_relativeFileNames )
  {
    liqglo_ribDir     = liqglo_projectDir + "rib";
    liqglo_textureDir = liqglo_projectDir + "rmantex";
    m_pixDir          = liqglo_projectDir + "rmanpix";
    m_tmpDir          = liqglo_projectDir + "rmantmp";
  }

  setSearchPaths();

  return MS::kSuccess;
}

/**
 * Read the values from the render globals and set internal values.
 */
void liqRibTranslator::liquidReadGlobals()
{
  MStatus gStatus;
  MPlug gPlug;
  MFnDependencyNode rGlobalNode( rGlobalObj );

  // Display Driver Globals
  {
    gPlug = rGlobalNode.findPlug( "numDD", &gStatus );
    if ( gStatus == MS::kSuccess ) {
      gPlug.getValue( m_numDisplayDrivers );
    }
    gStatus.clear();
    int k = 1;
    while ( k <= m_numDisplayDrivers ) {
      structDDParam parameters;
      MString varName;
      MString varVal;
      bool varValBool=true;

      varName = "dd";
      varName += k;
      varName += "imageName";
      gPlug = rGlobalNode.findPlug( varName, &gStatus );
      if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
      gStatus.clear();
      liqglo_DDimageName.append( varVal );

      varName = "dd";
      varName += k;
      varName += "imageType";
      gPlug = rGlobalNode.findPlug( varName, &gStatus );
      if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
      gStatus.clear();
      m_DDimageType.append( varVal );

      varName = "dd";
      varName += k;
      varName += "imageMode";
      gPlug = rGlobalNode.findPlug( varName, &gStatus );
      if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
      gStatus.clear();
      m_DDimageMode.append( varVal );

      varName = "dd";
      varName += k;
      varName += "paramType";
      gPlug = rGlobalNode.findPlug( varName, &gStatus );
      if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
      gStatus.clear();
      m_DDparamType.append( varVal );

      varName = "dd";
      varName += k;
      varName += "enable";
      gPlug = rGlobalNode.findPlug( varName, &gStatus );
      if ( gStatus == MS::kSuccess ) gPlug.getValue( varValBool );
      gStatus.clear();
      m_DDenabled.append( varValBool );

      varName = "numDD";
      varName += k;
      varName += "Param";
      gPlug = rGlobalNode.findPlug( varName, &gStatus );
      if ( gStatus == MS::kSuccess ) {
        gPlug.getValue( parameters.num );
      } else {
        parameters.num = 0;
      }
      gStatus.clear();

      for ( int p = 1; p <= parameters.num; p++ ) {
        varName = "dd";
        varName += k;
        varName += "Param";
        varName += p;
        varName += "Name";
        gPlug = rGlobalNode.findPlug( varName, &gStatus );
        if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
        gStatus.clear();
        parameters.names.append( varVal );
        varName = "dd";
        varName += k;
        varName += "Param";
        varName += p;
        varName += "Data";
        gPlug = rGlobalNode.findPlug( varName, &gStatus );
        if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
        gStatus.clear();
        parameters.data.append( varVal );
        varName = "dd";
        varName += k;
        varName += "Param";
        varName += p;
        varName += "Type";
        gPlug = rGlobalNode.findPlug( varName, &gStatus );
        liquidlong intVal;
        if ( gStatus == MS::kSuccess ) gPlug.getValue( intVal );
        parameters.type.append( intVal );
        gStatus.clear();
      }
      m_DDParams.push_back( parameters );
      k++;
    }
  }

  // Hmmmmmmmmm duplicated code : bad
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "shotName", &gStatus );
    if ( gStatus == MS::kSuccess ) {
      gPlug.getValue( varVal );
      // no substitution here
      liqglo_shotName = varVal;
    }
    gStatus.clear();
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "shotVersion", &gStatus );
    if ( gStatus == MS::kSuccess ) {
      gPlug.getValue( varVal );
      // no substitution here
      liqglo_shotVersion = varVal;
    }
    gStatus.clear();
  }
  setSearchPaths();
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "ribgenCommand", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    if ( varVal != MString("") ) m_ribgenCommand = varVal;
    gStatus.clear();
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "renderCommand", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    if ( varVal != MString("") ) liquidRenderer.renderCommand = varVal;
    gStatus.clear();
  }
{
    MString varVal;
    liquidRenderer.renderCmdFlags.clear();
    gPlug = rGlobalNode.findPlug( "renderCmdFlags", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    if ( varVal != MString("") ) liquidRenderer.renderCmdFlags = varVal;
    gStatus.clear();
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "makeTexture", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    if ( varVal != MString("") ) liqglo_makeTexture = varVal;
    gStatus.clear();
  }
  { MString varVal;
    int i;
    gPlug = rGlobalNode.findPlug( "bits_features", &gStatus );
    if ( gStatus == MS::kSuccess ) {
      gPlug.getValue( varVal );
      MStringArray tokens;
      varVal.split( ' ', tokens );
      liquidRenderer.supports_BLOBBIES = false;
      liquidRenderer.supports_DOF = false;
      liquidRenderer.supports_EYESPLITS = false;
      liquidRenderer.supports_POINTS = false;
      liquidRenderer.supports_RAYTRACE = false;
      for( i = 0; i<tokens.length() ; i++ ) {
        if ( tokens[i] == "Blobbies") { liquidRenderer.supports_BLOBBIES = true; continue; }
        if ( tokens[i] == "DepthOfField") { liquidRenderer.supports_DOF = true; continue; }
        if ( tokens[i] == "Eyesplits") { liquidRenderer.supports_EYESPLITS = true; continue; }
        if ( tokens[i] == "Points") { liquidRenderer.supports_POINTS = true; continue; }
        if ( tokens[i] == "Raytrace") { liquidRenderer.supports_RAYTRACE = true; continue; }
      }
    }
    gPlug = rGlobalNode.findPlug( "bits_required", &gStatus );
    if ( gStatus == MS::kSuccess ) {
      gPlug.getValue( varVal );
      MStringArray tokens;
      varVal.split( ' ', tokens );
      liquidRenderer.requires__PREF = false;
      liquidRenderer.requires_SWAPPED_UVS = false;
      liquidRenderer.requires_MAKESHADOW = false;

      for( i = 0; i<tokens.length() ; i++ ) {
        if ( tokens[i] == "__Pref") { liquidRenderer.requires__PREF = true; continue; }
        if ( tokens[i] == "Swap_UV") { liquidRenderer.requires_SWAPPED_UVS = true; continue; }
        if ( tokens[i] == "MakeShadow") { liquidRenderer.requires_MAKESHADOW = true; continue; }
      }
    }
  }
  // Deep Shadow Display options
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "dshDisplayName", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    if ( varVal != MString("") ) liquidRenderer.dshDisplayName = varVal;
    gStatus.clear();
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "dshImageMode", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    if ( varVal != MString("") ) liquidRenderer.dshImageMode = varVal;
    gStatus.clear();
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "renderScriptFileName", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    if ( varVal != MString("") ) m_userRenderScriptFileName = varVal;
    gStatus.clear();
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "preCommand", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( m_preCommand );
    gStatus.clear();
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "preJobCommand", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    m_preJobCommand = parseString( varVal );
    gStatus.clear();
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "postJobCommand", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    m_postJobCommand = parseString( varVal );
    gStatus.clear();
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "postFrameCommand", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( m_postFrameCommand );
    gStatus.clear();
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "preFrameCommand", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( m_preFrameCommand );
    gStatus.clear();
  }
  gPlug = rGlobalNode.findPlug( "launchRender", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( launchRender );
  gStatus.clear();
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "renderCamera", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      renderCamera = parseString( varVal );
    }
  }
  gPlug = rGlobalNode.findPlug( "rotateCamera", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_rotateCamera );
  gStatus.clear();
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "ribName", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      liqglo_sceneName = parseString( varVal );
    }
  }
  gPlug = rGlobalNode.findPlug( "beautyRibHasCameraName", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_beautyRibHasCameraName );
  gStatus.clear();
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "pictureDirectory", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_pixDirG = parseString( varVal );
    }
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "textureDirectory", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_texDirG = parseString( varVal );
    }
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "tempDirectory", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_tmpDirG = parseString( varVal );
    }
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "ribDirectory", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_ribDirG = parseString( varVal );
    }
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "alfredTags", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_alfredTags = parseString( varVal );
    }
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "alfredServices", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_alfredServices = parseString( varVal );
    }
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "key", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_defGenKey = parseString( varVal );
    }
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "service", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_defGenService = parseString( varVal );
    }
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "preframeMel", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_preFrameMel = parseString( varVal );
    }
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "postframeMel", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_postFrameMel = parseString( varVal );
    }
  }

  // PIXELFILTER OPTIONS: BEGIN
  gPlug = rGlobalNode.findPlug( "pixelFilter", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_rFilter );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "pixelFilterX", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_rFilterX );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "pixelFilterY", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_rFilterY );
  gStatus.clear();
  // PIXELFILTER OPTIONS: END

  // RENDER OPTIONS:BEGIN
  { int var;
    gPlug = rGlobalNode.findPlug( "hider", &gStatus );
    if ( gStatus == MS::kSuccess )  {
    gPlug.getValue( var );
    liqglo_hider = (enum HiderType) var;
  }
    gStatus.clear();
    gPlug = rGlobalNode.findPlug( "jitter", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_jitter );
    gStatus.clear();

  }
  // RENDER OPTIONS:END

  MStatus cropStatus;
  gPlug = rGlobalNode.findPlug( "cropX1", &cropStatus );
  gStatus.clear();
  if ( cropStatus == MS::kSuccess ) {
    gPlug = rGlobalNode.findPlug( "cropX1", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( m_cropX1 );
    gStatus.clear();
    gPlug = rGlobalNode.findPlug( "cropX2", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( m_cropX2 );
    gStatus.clear();
    gPlug = rGlobalNode.findPlug( "cropY1", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( m_cropY1 );
    gStatus.clear();
    gPlug = rGlobalNode.findPlug( "cropY2", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( m_cropY2 );
    gStatus.clear();
  }

  // RAYTRACING OPTIONS:BEGIN
  gPlug = rGlobalNode.findPlug( "useRayTracing", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( rt_useRayTracing );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "traceMaxDepth", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( rt_traceMaxDepth );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "traceSpecularThreshold", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( rt_traceSpecularThreshold );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "traceBreadthFactor", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( rt_traceBreadthFactor );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "traceDepthFactor", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( rt_traceDepthFactor );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "traceRayContinuation", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( rt_traceRayContinuation );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "traceCacheMemory", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( rt_traceCacheMemory );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "traceDisplacements", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( rt_traceDisplacements );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "traceSampleMotion", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( rt_traceSampleMotion );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "traceBias", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( rt_traceBias );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "traceMaxDiffuseDepth", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( rt_traceMaxDiffuseDepth );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "traceMaxSpecularDepth", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( rt_traceMaxSpecularDepth );
  gStatus.clear();
  // RAYTRACING OPTIONS:END

  gPlug = rGlobalNode.findPlug( "useMtorSubdiv", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_useMtorSubdiv );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "shortShaderNames", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_shortShaderNames );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "relativeFileNames", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_relativeFileNames );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "expandAlfred", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_alfredExpand );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "createOutputDirectories", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( createOutputDirectories );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "minCPU", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_minCPU );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "maxCPU", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_maxCPU );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "showProgress", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_showProgress );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "expandShaderArrays", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_expandShaderArrays );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "outputComments", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_outputComments );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "shaderDebug", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_shaderDebug );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "deferredGen", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_deferredGen );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "deferredBlock", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_deferredBlockSize );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "useRenderScript", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( useRenderScript );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "remoteRender", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( remoteRender );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "renderAllCurves", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_renderAllCurves );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "ignoreLights", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_ignoreLights );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "ignoreSurfaces", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_ignoreSurfaces );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "ignoreDisplacements", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_ignoreDisplacements );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "ignoreVolumes", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_ignoreVolumes );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "outputShadowPass", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_outputShadowPass );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "outputHeroPass", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_outputHeroPass );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "netRManRender", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( useNetRman );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "ignoreShadows", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_doShadows );
  gStatus.clear();
  liqglo_doShadows = !liqglo_doShadows;
  gPlug = rGlobalNode.findPlug( "fullShadowRibs", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( fullShadowRib );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "binaryOutput", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_doBinary );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "lazyCompute", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_lazyCompute );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "outputShadersInShadows", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_outputShadersInShadows );
  gStatus.clear();
  // Moritz: added new options for light/shader output in deep shadows
  gPlug = rGlobalNode.findPlug( "outputShadersInDeepShadows", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_outputShadersInDeepShadows );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "outputLightsInDeepShadows", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_outputLightsInDeepShadows );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "outputMeshUVs", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_outputMeshUVs );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "compressedOutput", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_doCompression );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "exportReadArchive", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_exportReadArchive );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "renderJobName", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( renderJobName );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "doAnimation", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_animation );
  gStatus.clear();
  if ( m_animation ) {
    gPlug = rGlobalNode.findPlug( "startFrame", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( frameFirst );
    gStatus.clear();
    gPlug = rGlobalNode.findPlug( "endFrame", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( frameLast );
    gStatus.clear();
    gPlug = rGlobalNode.findPlug( "frameStep", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( frameBy );
    gStatus.clear();
  }
  gPlug = rGlobalNode.findPlug( "doPadding", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( doExtensionPadding );
  gStatus.clear();
  if ( doExtensionPadding ) {
    gPlug = rGlobalNode.findPlug( "padding", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( m_outPadding );
    gStatus.clear();
  }
  liquidlong gWidth, gHeight;
  gPlug = rGlobalNode.findPlug( "xResolution", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( gWidth );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "yResolution", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( gHeight );
  gStatus.clear();
  if ( gWidth > 0 ) width = gWidth;
  if ( gHeight > 0 ) height = gHeight;
  gPlug = rGlobalNode.findPlug( "pixelAspectRatio", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( aspectRatio );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "transformationBlur", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_doMotion );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "cameraBlur", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( doCameraMotion );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "deformationBlur", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_doDef );
  gStatus.clear();

  gPlug = rGlobalNode.findPlug( "shutterConfig", &gStatus );
  if ( gStatus == MS::kSuccess ) {

    int var;

    gPlug.getValue( var );

    shutterConfig = ( enum shutterConfig ) var;

  }
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "motionBlurSamples", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_motionSamples );
  gStatus.clear();
  if( liqglo_motionSamples > LIQMAXMOTIONSAMPLES )
    liqglo_motionSamples = LIQMAXMOTIONSAMPLES;
  gPlug = rGlobalNode.findPlug( "depthOfField", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( doDof );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "pixelSamples", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( pixelSamples );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "shadingRate", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( shadingRate );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "limitsBucketXSize", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( bucketSize[0] );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "limitsBucketYSize", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( bucketSize[1] );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "limitsGridSize", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( gridSize );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "limitsTextureMemory", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( textureMemory );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "limitsEyeSplits", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( eyeSplits );
  gStatus.clear();

  gPlug = rGlobalNode.findPlug( "cleanRib", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( cleanRib );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "cleanRenderScript", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( cleanRenderScript );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "cleanTex", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( cleanTextures );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "cleanShad", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( cleanShadows );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "justRib", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_justRib );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "imageDepth", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( quantValue );
  gStatus.clear();

  gPlug = rGlobalNode.findPlug( "gain", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_rgain );
  gStatus.clear();
  gPlug = rGlobalNode.findPlug( "gamma", &gStatus );
  if ( gStatus == MS::kSuccess ) gPlug.getValue( m_rgamma );
  gStatus.clear();

  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "imageDriver", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      outFormat = parseString( varVal );
    }
  }

  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "preWorld", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_preWorldRIB = parseString( varVal );
    }
  }

  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "postWorld", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_postWorldRIB = parseString( varVal );
    }
  }

  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "preGeom", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_preGeomRIB = parseString( varVal );
    }
  }

  {
    int var;
    gPlug = rGlobalNode.findPlug( "renderScriptFormat", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( var );
    m_renderScriptFormat = ( enum renderScriptFormat ) var;
    gStatus.clear();
  }

  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "renderScriptCommand", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      m_renderScriptCommand = parseString( varVal );
    }
  }
}

bool liqRibTranslator::verifyOutputDirectories()
{
#ifdef _WIN32
  int dirMode = 0; // dummy arg
#else
  mode_t dirMode;
  dirMode = S_IRWXU|S_IRWXG|S_IRWXO;
#endif

  #define DIR_CREATION_WARNING(type, path) \
    MGlobal::displayWarning( "Liquid -> Had trouble creating " + MString( type ) + " Directory, " + path + ". Defaulting to system temp directory!\n" ); \
    cout <<"WARNING: Liquid -> Had trouble creating "<<type<<" Directory, "<<path<<". Defaulting to system temp directory!"<<endl<<flush

  #define DIR_MISSING_WARNING(type, path) \
    MGlobal::displayWarning( "Liquid -> " + MString( type ) + " Directory, " + path + ", does not exist. Defaulting to system temp directory!\n" ); \
    cout <<"WARNING: Liquid -> "<<type<<" Directory, "<<path<<", does not exist. Defaulting to system temp directory!"<<endl<<flush

  bool problem = false;

  MString tmp_path = LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, liqglo_ribDir, liqglo_projectDir );
  if ( ( access( tmp_path.asChar(), 0 )) == -1 ) {
    if ( createOutputDirectories ) {
      if ( MKDIR( tmp_path.asChar(), dirMode ) != 0 ) {

        DIR_CREATION_WARNING( "RIB", tmp_path );
        liqglo_ribDir = m_systemTempDirectory;
        problem = true;
      }
    } else {
      DIR_MISSING_WARNING( "RIB", tmp_path );
      liqglo_ribDir = m_systemTempDirectory;
      problem = true;
    }
  }

  tmp_path = LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, liqglo_textureDir, liqglo_projectDir );
  if ( ( access( tmp_path.asChar(), 0 ) ) == -1 ) {
    if ( createOutputDirectories ) {
      if ( MKDIR( tmp_path.asChar(), dirMode ) != 0 ) {
        DIR_CREATION_WARNING( "Texture", tmp_path );
        liqglo_textureDir = m_systemTempDirectory;
        problem = true;
      }
    } else {
      DIR_MISSING_WARNING( "Texture", tmp_path );
      liqglo_textureDir = m_systemTempDirectory;
      problem = true;
    }
  }

  if ( m_pixDir.index( '/' ) == 0 ) {
    tmp_path = m_pixDir;
  } else tmp_path = LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, m_pixDir, liqglo_projectDir );
  if ( (access( tmp_path.asChar(), 0 )) == -1 ) {
    if ( createOutputDirectories ) {
      if ( MKDIR( tmp_path.asChar(), dirMode ) != 0 ) {
        DIR_CREATION_WARNING( "Picture", tmp_path );
        m_pixDir = m_systemTempDirectory;
        problem = true;
      }
    } else {
      DIR_MISSING_WARNING( "Picture", tmp_path );
      m_pixDir = m_systemTempDirectory;
      problem = true;
    }
  }

  tmp_path = LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, m_tmpDir, liqglo_projectDir );
  if ( (access( tmp_path.asChar(), 0 )) == -1 ) {
    if ( createOutputDirectories ) {
      if ( MKDIR( tmp_path.asChar(), dirMode ) != 0 ) {
        DIR_CREATION_WARNING( "Temp Files", tmp_path );
        m_tmpDir = m_systemTempDirectory;
        problem = true;
      }
    } else {
      DIR_MISSING_WARNING( "Temp Files", tmp_path );
      m_tmpDir = m_systemTempDirectory;
      problem = true;
    }
  }
  return problem;
}

MString liqRibTranslator::generateRenderScriptName() const
{
  MString renderScriptName;
  renderScriptName = m_tmpDir;
  if ( m_userRenderScriptFileName != MString( "" ) ){
    renderScriptName += m_userRenderScriptFileName;
  } else {
    renderScriptName += liqglo_sceneName;
#ifndef _WIN32
    struct timeval  t_time;
    struct timezone t_zone;
    size_t tempSize = 0;
    char currentHostName[1024];
    gethostname( currentHostName, tempSize );
    liquidlong hashVal = liquidHash( currentHostName );
    gettimeofday( &t_time, &t_zone );
    srandom( t_time.tv_usec + hashVal );
    short alfRand = random();
    renderScriptName += alfRand;
#endif
  }

  if (m_renderScriptFormat == ALFRED) {
    renderScriptName += ".alf";
  }
  if (m_renderScriptFormat == XML) {
    renderScriptName += ".xml";
  }

  return renderScriptName;
}

MString liqRibTranslator::generateTempMayaSceneName() const
{
  MString tempDefname = m_tmpDir;
  tempDefname += liqglo_sceneName;
#ifndef _WIN32
  struct timeval  t_time;
  struct timezone t_zone;
  size_t tempSize = 0;
  char currentHostName[1024];
  gethostname( currentHostName, tempSize );
  liquidlong hashVal = liquidHash( currentHostName );
  gettimeofday( &t_time, &t_zone );
  srandom( t_time.tv_usec + hashVal );
  short defRand = random();
  tempDefname += defRand;
#endif
  MString currentFileType = MFileIO::fileType();
  if ( MString( "mayaAscii" )  == currentFileType ) tempDefname += ".ma";
  if ( MString( "mayaBinary" ) == currentFileType ) tempDefname += ".mb";

  return tempDefname;
}

/**
 * This method actually does the renderman output.
 */
MStatus liqRibTranslator::doIt( const MArgList& args )
{
  MStatus status;
  MString lastRibName;

  status = liquidDoArgs( args );
  if (!status) {
    return MS::kFailure;
  }

  if ( !liquidBin ) liquidInfo("Creating RIB <Press ESC To Cancel> ...");

  // Remember the frame the scene was at so we can restore it later.
  MTime originalTime = MAnimControl::currentTime();

  // Set the frames-per-second global (we'll need this for
  // streak particles)
  //
  MTime oneSecond( 1, MTime::kSeconds );
  liqglo_FPS = oneSecond.as( MTime::uiUnit() );

  // append the progress flag for render job feedback
  if ( useRenderScript ) {
    if ( ( m_renderCommand == MString( "render" ) ) || ( m_renderCommand == MString( "prman" ) ) || ( m_renderCommand == MString( "renderdl" ) ) ) {
      m_renderCommand = m_renderCommand + " -Progress";
    }
  }

  // check to see if the output camera, if specified, is available
  if ( liquidBin && ( renderCamera == "" ) ) {
    printf( "No Render Camera Specified\n" );
    return MS::kFailure;
  }
  if ( renderCamera != "" ) {
    MStatus selectionStatus;
    MSelectionList camList;
    selectionStatus = camList.add( renderCamera );
    if ( selectionStatus != MS::kSuccess ) {
      MGlobal::displayError( "Invalid Render Camera" );
      return MS::kFailure;
    }
  }

  // if directories were set in the globals then set the global variables
  if ( m_ribDirG.length() > 0 ) liqglo_ribDir = m_ribDirG;
  if ( m_texDirG.length() > 0 ) liqglo_textureDir = m_texDirG;
  if ( m_pixDirG.length() > 0 ) m_pixDir = m_pixDirG;
  if ( m_tmpDirG.length() > 0 ) m_tmpDir = m_tmpDirG;

  // make sure the directories end with a slash
  LIQ_ADD_SLASH_IF_NEEDED( liqglo_ribDir );
  LIQ_ADD_SLASH_IF_NEEDED( liqglo_textureDir );
  LIQ_ADD_SLASH_IF_NEEDED( m_pixDir );
  LIQ_ADD_SLASH_IF_NEEDED( m_tmpDir );

  // setup the error handler
#if defined AQSIS || ( _WIN32 && DELIGHT )
#  ifdef _WIN32
  if ( m_errorMode ) RiErrorHandler( (void(__cdecl*)(int,int,char*))liqRibTranslatorErrorHandler );
#  else
  if ( m_errorMode ) RiErrorHandler( (void(*)(int,int,char*))liqRibTranslatorErrorHandler );
#  endif
#else
  if ( m_errorMode ) RiErrorHandler( liqRibTranslatorErrorHandler );
#endif

  // Setup helper variables for alfred
  MString alfredCleanUpCommand;
  if ( remoteRender ) {
    alfredCleanUpCommand = MString( "RemoteCmd" );
  } else {
    alfredCleanUpCommand = MString( "Cmd" );
  }

  MString alfredRemoteTagsAndServices;
  if ( remoteRender || useNetRman ) {
    alfredRemoteTagsAndServices  = MString( "-service { " );
    alfredRemoteTagsAndServices += m_alfredServices.asChar();
    alfredRemoteTagsAndServices += MString( " } -tags { " );
    alfredRemoteTagsAndServices += m_alfredTags.asChar();
    alfredRemoteTagsAndServices += MString( " } " );
  }

  // A seperate one for cleanup as it doens't need a tag!
  MString alfredCleanupRemoteTagsAndServices;
  if ( remoteRender || useNetRman ) {
    alfredCleanupRemoteTagsAndServices  = MString( "-service { " );
    alfredCleanupRemoteTagsAndServices += m_alfredServices.asChar();
    alfredCleanupRemoteTagsAndServices += MString( " } " );
  }

  // exception handling block, this tracks liquid for any possible errors and tries to catch them
  // to avoid crashing
  try {
    m_escHandler.beginComputation();

    // check to see if all the directories we are working with actually exist.
    verifyOutputDirectories();

    if ( ( m_preFrameMel  != "" ) && !fileExists( m_preFrameMel  ) ) {
      cout << "Liquid -> Cannot find pre frame mel script file! Assuming local.\n" << flush;
    }
    if ( ( m_postFrameMel != "" ) && !fileExists( m_postFrameMel ) ) {
      cout << "Liquid -> Cannot find post frame mel script file! Assuming local.\n" << flush;
    }

    // build temp file names
    MString renderScriptName = generateRenderScriptName();
    MString tempDefname    = generateTempMayaSceneName();

    if ( m_deferredGen ) {
      MString currentFileType = MFileIO::fileType();
      MFileIO::exportAll( tempDefname, currentFileType.asChar() );
    }

    if ( !m_deferredGen && m_justRib ) {
      useRenderScript = false;
    }

    liqRenderScript jobScript;
    liqRenderScript::Job preJobInstance;
    preJobInstance.title = "liquid pre-job";
    preJobInstance.isInstance = true;

    if ( useRenderScript ) {
      if ( renderJobName == "" ) {
        renderJobName = liqglo_sceneName;
      }
      jobScript.title = renderJobName.asChar();

      if ( useNetRman ) {
        jobScript.minServers = m_minCPU;
        jobScript.maxServers = m_maxCPU;
      } else {
        jobScript.minServers = 1;
        jobScript.maxServers = 1;
      }

      if ( m_preJobCommand != MString( "" ) ) {
        liqRenderScript::Job preJob;
        preJob.title = "liquid pre-job";
        preJob.commands.push_back( liqRenderScript::Cmd( m_preJobCommand.asChar(), ( remoteRender && !useNetRman ) ) );
        jobScript.addJob( preJob );
      }
    }

    // start looping through the frames

    LIQDEBUGPRINTF( "-> starting to loop through frames\n" );

    int currentBlock = 0;
    for( liqglo_lframe=frameFirst; liqglo_lframe<=frameLast; liqglo_lframe = liqglo_lframe + frameBy ) {
      if ( m_showProgress ) printProgress( 1, frameFirst, frameLast, liqglo_lframe );

      liqRenderScript::Job frameScriptJob;

      m_shadowRibGen = false;
      m_alfShadowRibGen = false;
      liqglo_preReadArchive.clear();
      liqglo_preRibBox.clear();
      liqglo_preReadArchiveShadow.clear();
      liqglo_preRibBoxShadow.clear();

      // make sure all the global strings are parsed for this frame
      MString frameRenderCommand    = parseString( liquidRenderer.renderCommand + " " + liquidRenderer.renderCmdFlags );
      MString frameRibgenCommand    = parseString( m_ribgenCommand );
      MString framePreCommand       = parseString( m_preCommand );
      MString framePreFrameCommand  = parseString( m_preFrameCommand );
      MString framePostFrameCommand = parseString( m_postFrameCommand );

      if ( useRenderScript ) {
        if ( m_deferredGen ) {
          liqRenderScript::Job deferredJob;
          if ( (( liqglo_lframe - frameFirst ) % m_deferredBlockSize ) == 0 ) {
            if ( m_deferredBlockSize == 1 ) {
              currentBlock = liqglo_lframe;
            } else {
              currentBlock++;
            }

            int lastGenFrame = ( liqglo_lframe + ( m_deferredBlockSize - 1 ) );
            if ( lastGenFrame > frameLast ) {
              lastGenFrame = frameLast;
            }
            std::stringstream ribGenExtras;
            ribGenExtras << " -progress -noDef -nop -noalfred -projectDir " << liqglo_projectDir.asChar() << " -ribName " << liqglo_sceneName.asChar() << " -mf " << tempDefname.asChar() << " -n " << liqglo_lframe << " " << lastGenFrame << " " << frameBy;

            std::stringstream titleStream;
            titleStream << liqglo_sceneName.asChar() << "FrameRIBGEN" << currentBlock;
            deferredJob.title = titleStream.str();

            std::stringstream ss;
            ss << framePreCommand.asChar() << " " << frameRibgenCommand.asChar() << ribGenExtras.str();
            liqRenderScript::Cmd cmd(ss.str(), remoteRender);
            cmd.alfredServices = m_defGenService.asChar();
            cmd.alfredTags     = m_defGenKey.asChar();
            if ( m_alfredExpand ) {
              cmd.alfredExpand = true;
            }
            deferredJob.commands.push_back(cmd);
            jobScript.addJob(deferredJob);
          }
        }

        if ( !m_justRib ) {
          std::stringstream titleStream;
          titleStream << liqglo_sceneName.asChar() << "Frame" << liqglo_lframe;
          frameScriptJob.title = titleStream.str();

          if ( m_deferredGen ) {
            std::stringstream ss;
            ss << liqglo_sceneName.asChar() << "FrameRIBGEN" << currentBlock;
            liqRenderScript::Job instanceJob;
            instanceJob.isInstance = true;
            instanceJob.title = ss.str();
            frameScriptJob.childJobs.push_back(instanceJob);
          }
        }
      }

      LIQDEBUGPRINTF( "-> building jobs\n" );
      // Hmmmmmm not really clean ....
      if ( buildJobs() != MS::kSuccess ) break;

      {
        baseShadowName = liqglo_ribDir;
        baseShadowName += liqglo_sceneName;
        /*if ( ( liqglo_DDimageName[0] == "" ) ) {
          baseShadowName += liqglo_sceneName;
        } else {
          int pointIndex = liqglo_DDimageName[0].index( '.' );
          baseShadowName += liqglo_DDimageName[0].substring(0, pointIndex-1).asChar();
        }*/
        baseShadowName += "_SHADOWBODY";
        baseShadowName += LIQ_ANIM_EXT;
        baseShadowName += extension;
        // Hmmmmmmm protect from buffer overflow : got to find a clean way ...
        size_t shadowNameLength = baseShadowName.length() + 1;
        shadowNameLength += 10;
        char *baseShadowRibName;
        baseShadowRibName = (char *)alloca(shadowNameLength);
        sprintf(baseShadowRibName, baseShadowName.asChar(), doExtensionPadding ? m_outPadding : 0, liqglo_lframe );
        baseShadowName = baseShadowRibName;
      }

      if ( !m_deferredGen ) {

        htable = new liqRibHT();

        float sampleinc = ( liqglo_shutterTime * m_blurTime ) / ( liqglo_motionSamples - 1 );
        for ( int msampleOn = 0; msampleOn < liqglo_motionSamples; msampleOn++ ) {
          float subframe;
          switch( shutterConfig ) {
            case OPEN_ON_FRAME:
            default:
              subframe = liqglo_lframe + ( msampleOn * sampleinc );
              break;
            case CENTER_ON_FRAME:
              subframe = ( liqglo_lframe - ( liqglo_shutterTime * m_blurTime * 0.5 ) ) + msampleOn * sampleinc;
              break;
            case CENTER_BETWEEN_FRAMES:
              subframe = liqglo_lframe + ( 0.5 * ( 1 - ( liqglo_shutterTime * m_blurTime ) ) ) + ( msampleOn * sampleinc );
              break;
            case CLOSE_ON_NEXT_FRAME:
              subframe = liqglo_lframe + ( 1 - ( liqglo_shutterTime * m_blurTime ) ) + ( msampleOn * sampleinc );
              break;
          }
          liqglo_sampleTimes[ msampleOn ] = subframe;
        }

        if ( doCameraMotion || liqglo_doMotion || liqglo_doDef ) {
          for ( int msampleOn = 0; msampleOn < liqglo_motionSamples; msampleOn++ ) {
            scanScene( liqglo_sampleTimes[ msampleOn ] , msampleOn );
          }
        } else {
          liqglo_sampleTimes[ 0 ] = liqglo_lframe;
          scanScene( liqglo_lframe, 0 );
        }
        if ( m_showProgress ) printProgress( 2, frameFirst, frameLast, liqglo_lframe );

        std::vector<structJob>::iterator iter = jobList.begin();
        for (; iter != jobList.end(); ++iter ) {
          if ( liqglo_currentJob.isShadow && !liqglo_doShadows ) continue;
          m_currentMatteMode = false;
          liqglo_currentJob = *iter;
          activeCamera = liqglo_currentJob.path;
          if ( liqglo_currentJob.isShadowPass ) {
            liqglo_isShadowPass = true;
          } else {
            liqglo_isShadowPass = false;
          }

          LIQDEBUGPRINTF( "-> setting RiOptions\n" );

          // Rib client file creation options MUST be set before RiBegin
#if defined PRMAN || DELIGHT
          LIQDEBUGPRINTF( "-> setting binary option\n" );
          if ( liqglo_doBinary )
          {
            RtString format = "binary";
            RiOption( "rib", "format", ( RtPointer )&format, RI_NULL);
          }

          LIQDEBUGPRINTF( "-> setting compression option\n" );
          if ( liqglo_doCompression )
          {
            RtString comp = "gzip";
            RiOption( "rib", "compression", &comp, RI_NULL);
          }
#endif // PRMAN || DELIGHT

          // world RiReadArchives and Rib Boxes
          if ( liqglo_currentJob.isShadow && !m_shadowRibGen && !fullShadowRib ) {
#ifndef PRMAN
            LIQDEBUGPRINTF( "-> beginning rib output\n" );
            RiBegin( const_cast<char *>( LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, baseShadowName, liqglo_projectDir ).asChar() ) );
#else
            liqglo_ribFP = fopen( LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, baseShadowName, liqglo_projectDir ).asChar(), "w" );
            if ( liqglo_ribFP ) {
              LIQDEBUGPRINTF( "-> setting pipe option\n" );
              RtInt ribFD = fileno( liqglo_ribFP );
              RiOption( "rib", "pipe", &ribFD, RI_NULL );
            }
            LIQDEBUGPRINTF( "-> beginning rib output\n" );
            RiBegin( RI_NULL );
#endif
            if ( worldPrologue() != MS::kSuccess ) break;
            if( liqglo_currentJob.isShadow && m_outputLightsInDeepShadows ) {
              if ( lightBlock() != MS::kSuccess ) break;
            }
            if ( coordSysBlock() != MS::kSuccess ) break;
            if ( objectBlock() != MS::kSuccess ) break;
            if ( worldEpilogue() != MS::kSuccess ) break;
            RiEnd();
#ifdef PRMAN
            fclose( liqglo_ribFP );
#endif
            liqglo_ribFP = NULL;
            m_shadowRibGen = true;
            m_alfShadowRibGen = true;
          }
#ifndef PRMAN
          RiBegin( const_cast<char *>( LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, liqglo_currentJob.ribFileName, liqglo_projectDir ).asChar() ) );

  #ifdef DELIGHT
          LIQDEBUGPRINTF( "-> setting binary option\n" );
          if ( liqglo_doBinary )
          {
            RtString format = "binary";
            RiOption(( RtToken ) "rib", ( RtToken ) "format", ( RtPointer )&format, RI_NULL);
          } else {
            RtString format = "ascii";
            RiOption(( RtToken ) "rib", ( RtToken ) "format", ( RtPointer )&format, RI_NULL);
          }
  #endif
#else
          liqglo_ribFP = fopen( LIQ_GET_ABS_REL_FILE_NAME(liqglo_relativeFileNames, liqglo_currentJob.ribFileName, liqglo_projectDir ).asChar(), "w" );

          if ( liqglo_ribFP ) {
            RtInt ribFD = fileno( liqglo_ribFP );
            RiOption( ( RtToken )"rib", ( RtToken )"pipe", &ribFD, RI_NULL );
          } else {
            cerr << ( "Error opening rib - writing to stdout.\n" );
          }
          RiBegin( RI_NULL );
#endif
          if ( liqglo_currentJob.isShadow && !fullShadowRib && m_shadowRibGen ) {
            if ( ribPrologue() == MS::kSuccess ) {
              if ( framePrologue( liqglo_lframe ) != MS::kSuccess ) break;
              RiArchiveRecord( RI_COMMENT, "Read Archive Data: \nReadArchive \"%s\"", baseShadowName.asChar() );
              if ( frameEpilogue( liqglo_lframe ) != MS::kSuccess ) break;
              ribEpilogue();
            }
          } else {
            if ( ribPrologue() == MS::kSuccess ) {
              if ( framePrologue( liqglo_lframe ) != MS::kSuccess ) break;
              if ( worldPrologue() != MS::kSuccess ) break;
              if ( lightBlock() != MS::kSuccess ) break;
              if ( coordSysBlock() != MS::kSuccess ) break;
              if ( objectBlock() != MS::kSuccess ) break;
              if ( worldEpilogue() != MS::kSuccess ) break;
              if ( frameEpilogue( liqglo_lframe ) != MS::kSuccess ) break;
              ribEpilogue();
            }
          }

          RiEnd();
#ifdef PRMAN
          fclose( liqglo_ribFP );
#endif
          liqglo_ribFP = NULL;
          if ( m_showProgress ) printProgress( 3, frameFirst, frameLast, liqglo_lframe );
        }

        delete htable;
        freeShaders();
        htable = NULL;
      }

      // set the rib file for the 'view last rib' menu command
      // NOTE: this may be overridden later on in certain code paths
      if ( !m_deferredGen ) {
        lastRibName = LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, liqglo_currentJob.ribFileName.asChar(), liqglo_projectDir );
      }

      // now we re-iterate through the job list to write out the alfred file if we are using it

      if ( useRenderScript && !m_justRib ) {
        bool alf_textures = false;
        bool alf_shadows = false;
        bool alf_refmaps = false;

        // write out make texture pass
        LIQDEBUGPRINTF( "-> Generating job for MakeTexture pass\n");
        std::vector<structJob>::iterator iter = txtList.begin();
        if ( txtList.size() ) {
          alf_textures = true;
          liqRenderScript::Job textureJob;
          std::stringstream ts;
          ts << "Textures." << liqglo_lframe;
          textureJob.title = ts.str();

          while ( iter != txtList.end() ) {
            liqRenderScript::Job textureSubtask;
            std::stringstream ts;
            ts << textureJob.title << " " << iter->imageName.asChar();
            textureSubtask.title = ts.str();
            if ( m_deferredGen ) {

            }
            std::stringstream ss;
            ss << iter->renderName.asChar() << " " << iter->ribFileName.asChar();
            liqRenderScript::Cmd cmd( ss.str(), ( remoteRender && !useNetRman ) );

            if ( m_alfredExpand ) {
              cmd.alfredExpand = true;
            }
            cmd.alfredServices = m_alfredServices.asChar();
            cmd.alfredTags     = m_alfredTags.asChar();
            textureSubtask.commands.push_back( cmd );
            textureSubtask.chaserCommand = ( std::string( "sho \"" ) + liqglo_textureDir.asChar() + " " + iter->imageName.asChar() + "\"" );
            ++iter;
            textureJob.childJobs.push_back( textureSubtask );
          }
          frameScriptJob.childJobs.push_back( textureJob );
        }

        // write out shadows
        if ( liqglo_doShadows ) {
          LIQDEBUGPRINTF( "-> writing out shadow information to alfred file.\n" );
          std::vector<structJob>::iterator iter = shadowList.begin();
          if ( shadowList.size() ) {
            alf_shadows = true;
            liqRenderScript::Job shadowJob;
            std::stringstream ts;
            ts << "Shadows." << liqglo_lframe;
            shadowJob.title = ts.str();
            while ( iter != shadowList.end() ) {
              alf_shadows = true;
              liqRenderScript::Job shadowSubtask;
              shadowSubtask.title = iter->name.asChar();
              if ( alf_textures ) {
                std::stringstream ss;
                ss << "Textures." << liqglo_lframe;
                liqRenderScript::Job instanceJob;
                instanceJob.isInstance = true;
                instanceJob.title = ss.str();
                shadowSubtask.childJobs.push_back(instanceJob);
              }
              if ( m_deferredGen ) {
                std::stringstream ss;
                ss << liqglo_sceneName.asChar() << "FrameRIBGEN" << currentBlock;
                liqRenderScript::Job instanceJob;
                instanceJob.isInstance = true;
                instanceJob.title = ss.str();
                shadowSubtask.childJobs.push_back(instanceJob);
              }
              std::stringstream ss;
              if ( useNetRman ) {
#ifdef _WIN32
                ss << framePreCommand.asChar() << " netrender %H -Progress \"" << iter->ribFileName.asChar() << "\"";
#else
                ss << framePreCommand.asChar() << " netrender %H -Progress " << iter->ribFileName.asChar();
#endif
              } else {
#ifdef _WIN32
                ss << framePreCommand.asChar() << " " << frameRenderCommand.asChar() << " \"" << iter->ribFileName.asChar() << "\"";
#else
                ss << framePreCommand.asChar() << " " << frameRenderCommand.asChar() << " " << iter->ribFileName.asChar();
#endif
              }
              liqRenderScript::Cmd cmd(ss.str(), (remoteRender && !useNetRman));
              if ( m_alfredExpand ) {
                cmd.alfredExpand = true;
              }
              cmd.alfredServices = m_alfredServices.asChar();
              cmd.alfredTags     = m_alfredTags.asChar();
              shadowSubtask.commands.push_back(cmd);

              if (cleanRib)  {
                std::stringstream ss;
#ifdef _WIN32
                ss << framePreCommand.asChar() << " " << RM_CMD << " \"" << iter->ribFileName.asChar() << "\"";
#else
                ss << framePreCommand.asChar() << " " << RM_CMD << " " << iter->ribFileName.asChar();
#endif

                shadowSubtask.cleanupCommands.push_back( liqRenderScript::Cmd( ss.str(), remoteRender ) );
              }
              shadowSubtask.chaserCommand = ( std::string( "sho \"" ) + iter->imageName.asChar() + "\"" );

              ++iter;
              if ( !m_alfShadowRibGen && !fullShadowRib ) m_alfShadowRibGen = true;

              shadowJob.childJobs.push_back( shadowSubtask );
            }
            frameScriptJob.childJobs.push_back( shadowJob );
          }
        }
        LIQDEBUGPRINTF( "-> finished writing out shadow information to render script file.\n" );

        // write out make reflection pass
        if ( refList.size() ) {
          LIQDEBUGPRINTF( "-> Generating job for ReflectionMap pass\n" );
          std::vector<structJob>::iterator iter = refList.begin();

          alf_refmaps = true;
          liqRenderScript::Job reflectJob;
          std::stringstream ts;
          ts << "Reflections." << liqglo_lframe;
          reflectJob.title = ts.str();

          while ( iter != refList.end() ) {
            liqRenderScript::Job reflectSubtask;
            std::stringstream ts;
            ts << reflectJob.title << " " << iter->imageName.asChar();
            reflectSubtask.title = ts.str();
            if ( m_deferredGen ) {

            }
            if ( alf_textures ) {
              std::stringstream ss;
              ss << "Textures." << liqglo_lframe;
              liqRenderScript::Job instanceJob;
              instanceJob.isInstance = true;
              instanceJob.title = ss.str();
              reflectJob.childJobs.push_back( instanceJob );
            }
            if ( alf_shadows ) {
              std::stringstream ss;
              ss << "Shadows." << liqglo_lframe;
              liqRenderScript::Job instanceJob;
              instanceJob.isInstance = true;
              instanceJob.title = ss.str();
              reflectJob.childJobs.push_back( instanceJob );
            }

            std::stringstream ss;
            ss << iter->renderName.asChar() << " " << iter->ribFileName.asChar();
            liqRenderScript::Cmd cmd( ss.str(), (remoteRender && !useNetRman) );

            if ( m_alfredExpand ) {
              cmd.alfredExpand = true;
            }
            cmd.alfredServices = m_alfredServices.asChar();
            cmd.alfredTags     = m_alfredTags.asChar();
            reflectSubtask.commands.push_back( cmd );
            reflectSubtask.chaserCommand = ( std::string( "sho \"" ) + liqglo_textureDir.asChar() + " " + iter->imageName.asChar() + "\"" );
            ++iter;
            reflectJob.childJobs.push_back( reflectSubtask );
          }
          frameScriptJob.childJobs.push_back( reflectJob );
        }

        LIQDEBUGPRINTF( "-> initiating hero pass information.\n" );
        structJob *frameJob = NULL;
        structJob *shadowPassJob = NULL;
        LIQDEBUGPRINTF( "-> setting hero pass.\n" );
        if ( m_outputHeroPass && !m_outputShadowPass ) {
          frameJob = &jobList[jobList.size() - 1];
        } else if ( m_outputShadowPass && m_outputHeroPass ) {
          frameJob = &jobList[jobList.size() - 1];
          shadowPassJob = &jobList[jobList.size() - 2];
        } else if ( m_outputShadowPass && !m_outputHeroPass ) {
          shadowPassJob = &jobList[jobList.size() - 1];
        }
        LIQDEBUGPRINTF( "-> hero pass set.\n" );

        LIQDEBUGPRINTF( "-> writing out pre frame command information to render script file.\n" );
        if ( framePreFrameCommand != MString("") ) {
          liqRenderScript::Cmd cmd(framePreFrameCommand.asChar(), (remoteRender && !useNetRman));
          cmd.alfredServices = m_alfredServices.asChar();
          cmd.alfredTags     = m_alfredTags.asChar();
          frameScriptJob.commands.push_back(cmd);
        }

        if ( m_outputHeroPass ) {
          std::stringstream ss;
          if ( useNetRman ) {
#ifdef _WIN32
            ss << framePreCommand.asChar() << " netrender %H -Progress \"" << frameJob->ribFileName.asChar() << "\"";
#else
            ss << framePreCommand.asChar() << " netrender %H -Progress " << frameJob->ribFileName.asChar();
#endif
          } else {
#ifdef _WIN32
            ss << framePreCommand.asChar() << " " << frameRenderCommand.asChar() << " \"" << frameJob->ribFileName.asChar() << "\"";
#else
            ss << framePreCommand.asChar() << " " << frameRenderCommand.asChar() << " " << frameJob->ribFileName.asChar();
#endif
          }
          liqRenderScript::Cmd cmd(ss.str(), (remoteRender && !useNetRman));
          if ( m_alfredExpand ) {
            cmd.alfredExpand = true;
          }
          cmd.alfredServices = m_alfredServices.asChar();
          cmd.alfredTags     = m_alfredTags.asChar();
          frameScriptJob.commands.push_back(cmd);
        }
        LIQDEBUGPRINTF( "-> finished writing out hero information to alfred file.\n" );

        if ( m_outputShadowPass ) {
          std::stringstream ss;
          if ( useNetRman ) {
#ifdef _WIN32
            ss << framePreCommand.asChar() << " netrender %H -Progress \"" << shadowPassJob->ribFileName.asChar() << "\"";
#else
            ss << framePreCommand.asChar() << " netrender %H -Progress " << shadowPassJob->ribFileName.asChar();
#endif
          } else {
#ifdef _WIN32
            ss << framePreCommand.asChar() << " " << frameRenderCommand.asChar() << " \"" << shadowPassJob->ribFileName.asChar() << "\"";
#else
            ss << framePreCommand.asChar() << " " << frameRenderCommand.asChar() << " " << shadowPassJob->ribFileName.asChar();
#endif
          }
          liqRenderScript::Cmd cmd(ss.str(), (remoteRender && !useNetRman));
          if ( m_alfredExpand ) {
            cmd.alfredExpand = true;
          }
          cmd.alfredServices = m_alfredServices.asChar();
          cmd.alfredTags     = m_alfredTags.asChar();
          frameScriptJob.commands.push_back(cmd);
        }

        if ( cleanRib || ( framePostFrameCommand != MString( "" ) ) ) {
          if ( cleanRib ) {
            std::stringstream ss;
            if ( m_outputHeroPass  ) {
#ifdef _WIN32
              ss << framePreCommand.asChar() << " " << RM_CMD << " \"" << frameJob->ribFileName.asChar() << "\"";
#else
              ss << framePreCommand.asChar() << " " << RM_CMD << " " << frameJob->ribFileName.asChar();
#endif
            }
            if ( m_outputShadowPass) {
#ifdef _WIN32
              ss << framePreCommand.asChar() << " " << RM_CMD << " \"" << shadowPassJob->ribFileName.asChar() << "\"";
#else
              ss << framePreCommand.asChar() << " " << RM_CMD << " " << shadowPassJob->ribFileName.asChar();
#endif
            }
            if ( m_alfShadowRibGen ) {
#ifdef _WIN32
              ss << framePreCommand.asChar() << " " << RM_CMD << " \"" << baseShadowName.asChar() << "\"";
#else
              ss << framePreCommand.asChar() << " " << RM_CMD << " " << baseShadowName.asChar();
#endif
            }
            frameScriptJob.cleanupCommands.push_back(liqRenderScript::Cmd(ss.str(), remoteRender));
          }
          if ( framePostFrameCommand != MString("") ) {
            liqRenderScript::Cmd cmd(framePostFrameCommand.asChar(), (remoteRender && !useNetRman));
            frameScriptJob.cleanupCommands.push_back(cmd);
          }
        }
        if ( m_outputHeroPass ) {
          frameScriptJob.chaserCommand = (std::string( "sho \"" ) + frameJob->imageName.asChar() + "\"" );
        }
        if ( m_outputShadowPass ) {
          frameScriptJob.chaserCommand = (std::string( "sho \"" ) + shadowPassJob->imageName.asChar() + "\"" );
        }
        if ( m_outputShadowPass && !m_outputHeroPass ) {
          lastRibName = LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, shadowPassJob->ribFileName.asChar(), liqglo_projectDir );
        } else {
          lastRibName = LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, frameJob->ribFileName.asChar(), liqglo_projectDir );
        }
      }

      jobScript.addJob( frameScriptJob );

      if ( ( ribStatus != kRibOK ) && !m_deferredGen ) break;
    } // frame for-loop

    if ( useRenderScript ) {
      if ( m_preJobCommand != MString( "" ) ) {
        jobScript.addLeafDependency( preJobInstance );
      }

      // clean up the alfred file in the future
      if ( !m_justRib ) {
        if ( m_deferredGen ) {
          std::stringstream ss;
          ss << RM_CMD << " " << tempDefname.asChar();
          jobScript.cleanupCommands.push_back( liqRenderScript::Cmd( ss.str(), remoteRender ) );
        }
        if ( cleanRenderScript ) {
          std::stringstream ss;
          ss << RM_CMD << " " << renderScriptName.asChar();
          jobScript.cleanupCommands.push_back( liqRenderScript::Cmd( ss.str(), remoteRender ) );
        }
        if ( m_postJobCommand != MString("") ) {
          jobScript.cleanupCommands.push_back( liqRenderScript::Cmd(m_postJobCommand.asChar(), (remoteRender && !useNetRman) ) );
        }
      }
      if ( m_renderScriptFormat == ALFRED ) {
        jobScript.writeALF( LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, renderScriptName, liqglo_projectDir ).asChar() );
      }
      if ( m_renderScriptFormat == XML ) {
        jobScript.writeXML( LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, renderScriptName, liqglo_projectDir ).asChar() );
      }
    }

    LIQDEBUGPRINTF( "-> ending escape handler.\n" );
    m_escHandler.endComputation();

    if ( !liquidBin ) liquidInfo("...Finished Creating RIB.");
    LIQDEBUGPRINTF( "-> clearing job list.\n" );
    jobList.clear();
    jobScript.clear();

    // set the attributes on the liquidGlobals for the last rib file and last alfred script name
    LIQDEBUGPRINTF( "-> setting lastAlfredScript and lastRibFile.\n" );
    MGlobal::executeCommand("if (!attributeExists(\"lastRenderScript\",\"liquidGlobals\")) { addAttr -ln \"lastRenderScript\" -dt \"string\" liquidGlobals; }");
    MFnDependencyNode rGlobalNode( rGlobalObj );
    MPlug nPlug;
    nPlug = rGlobalNode.findPlug( "lastRenderScript" );
    nPlug.setValue( renderScriptName );
    nPlug = rGlobalNode.findPlug( "lastRibFile" );
    nPlug.setValue( lastRibName );

    LIQDEBUGPRINTF( "-> spawning command.\n" );
    if ( launchRender ) {
      if ( useRenderScript ) {
        if ( m_renderScriptCommand == "" ) {
          m_renderScriptCommand = "alfred";
        }
        if ( m_renderScriptFormat == NONE ) {
          MGlobal::displayWarning("No render script format specified to Liquid, and direct render execution not selected" );
        }
#ifdef _WIN32
        // Moritz: Added quotes to render script name as it may contain spaces in bloody Windoze
        // Note: also adding quotes to the path (aka project dir) breaks ShellExecute() -- cost me one hour to trace this!!!
        // Bloody, damn, asinine Windoze!!!
        liqProcessLauncher::execute( m_renderScriptCommand, "\"" + renderScriptName + "\"", liqglo_projectDir, false );
#else
        liqProcessLauncher::execute( m_renderScriptCommand, renderScriptName, liqglo_projectDir, false );
#endif
      } else {
        int exitstat = 0;
        // write out make texture pass
        std::vector<structJob>::iterator iter = txtList.begin();
        while ( iter != txtList.end() ) {
          MGlobal::displayInfo( "Making textures... " );
          cout << "[!] Making textures: " << iter->imageName.asChar() << endl;
#ifdef _WIN32
          liqProcessLauncher::execute( iter->renderName, "\"" + iter->ribFileName + "\"", liqglo_projectDir, true );
#else
          liqProcessLauncher::execute( iter->renderName, iter->ribFileName, liqglo_projectDir, true );
#endif
          ++iter;
        }

        if ( liqglo_doShadows ) {
          MGlobal::displayInfo( "Rendering shadow maps... " );
          cout << endl << "[!] Rendering shadow maps... " << endl;
          std::vector<structJob>::iterator iter = shadowList.begin();
          while ( iter != shadowList.end() ) {
            cout << iter->ribFileName.asChar() << endl;
            // exitstat = _spawnlp( _P_WAIT, liquidRenderer.renderCommand.asChar(), liquidRenderer.renderCommand.asChar(), arg.asChar(), NULL);
            // if ( exitstat ) break;
#ifdef _WIN32
            if( !liqProcessLauncher::execute( liquidRenderer.renderCommand, liquidRenderer.renderCmdFlags + " \"" + iter->ribFileName + "\"", liqglo_projectDir, true ) )
#else
            if( !liqProcessLauncher::execute( liquidRenderer.renderCommand, liquidRenderer.renderCmdFlags + " " + iter->ribFileName, liqglo_projectDir, true ) )
#endif
              break;
            ++iter;
          } // while ( iter != shadowList.end() )
        }
        if ( !exitstat ) {
          MGlobal::displayInfo( "Rendering hero pass... " );
          cout << "[!] Rendering hero pass..." << endl;
          cout << liqglo_currentJob.ribFileName.asChar() << endl << flush;
#ifdef _WIN32
          liqProcessLauncher::execute( liquidRenderer.renderCommand, liquidRenderer.renderCmdFlags + " \"" + liqglo_currentJob.ribFileName + "\"", liqglo_projectDir, false );
#else
          liqProcessLauncher::execute( liquidRenderer.renderCommand, liquidRenderer.renderCmdFlags + " " + liqglo_currentJob.ribFileName, liqglo_projectDir, false );
#endif
        }
      }
    }

    // return to the frame we were at before we ran the animation
    LIQDEBUGPRINTF( "-> setting frame to current frame.\n" );
    MGlobal::viewFrame (originalTime);

    return (ribStatus == kRibOK ? MS::kSuccess : MS::kFailure);

  } catch ( MString errorMessage ) {
    if ( MGlobal::mayaState() != MGlobal::kInteractive ) {
      cerr << errorMessage.asChar() << endl;
    } else {
      MGlobal::displayError( errorMessage );
    }
    if ( NULL != htable ) delete htable;
    freeShaders();
    if ( debugMode ) ldumpUnfreed();
    m_escHandler.endComputation();
    return MS::kFailure;
  } catch ( ... ) {
    cerr << "RIB Export: Unknown exception thrown\n" << endl;
    if ( NULL != htable ) delete htable;
    freeShaders();
    if ( debugMode ) ldumpUnfreed();
    m_escHandler.endComputation();
    return MS::kFailure;
  }
}

/**
 * Calculate the port field of view for the camera.
 */
void liqRibTranslator::portFieldOfView( int port_width, int port_height,
                                        double& horizontal,
                                        double& vertical,
                                        MFnCamera& fnCamera )
{
  double left, right, bottom, top;
  double aspect = (double) port_width / port_height;
  computeViewingFrustum(aspect,left,right,bottom,top,fnCamera);

  double neardb = fnCamera.nearClippingPlane();
  horizontal    = atan( ( ( right - left ) * 0.5 ) / neardb ) * 2.0;
  vertical      = atan( ( ( top - bottom ) * 0.5 ) / neardb ) * 2.0;
}

/**
 * Calculate the viewing frustrum for the camera.
 */
void liqRibTranslator::computeViewingFrustum ( double     window_aspect,
                                               double&    left,
                                               double&    right,
                                               double&    bottom,
                                               double&    top,
                                               MFnCamera& cam )
{
  double film_aspect   = cam.aspectRatio();
  double aperture_x    = cam.horizontalFilmAperture();
  double aperture_y    = cam.verticalFilmAperture();
  double offset_x      = cam.horizontalFilmOffset();
  double offset_y      = cam.verticalFilmOffset();
  double focal_to_near = cam.nearClippingPlane() / (cam.focalLength() * MM_TO_INCH);

  focal_to_near *= cam.cameraScale();

  double scale_x = 1.0;
  double scale_y = 1.0;
  double translate_x = 0.0;
  double translate_y = 0.0;

  switch ( cam.filmFit() ) {
    case MFnCamera::kFillFilmFit:
      if ( window_aspect < film_aspect ) {
        scale_x = window_aspect / film_aspect;
      }
      else {
        scale_y = film_aspect / window_aspect;
      }
      break;
    case MFnCamera::kHorizontalFilmFit:
      scale_y = film_aspect / window_aspect;
      if ( scale_y > 1.0 ) {
        translate_y = cam.filmFitOffset() *
          ( aperture_y - ( aperture_y * scale_y ) ) / 2.0;
      }
      break;
    case MFnCamera::kVerticalFilmFit:
      scale_x = window_aspect / film_aspect;
      if (scale_x > 1.0 ) {
        translate_x = cam.filmFitOffset() *
          ( aperture_x - ( aperture_x * scale_x ) ) / 2.0;
      }
      break;
    case MFnCamera::kOverscanFilmFit:
      if ( window_aspect < film_aspect ) {
        scale_y = film_aspect / window_aspect;
      }
      else {
        scale_x = window_aspect / film_aspect;
      }
      break;
    case MFnCamera::kInvalid:
      break;
  }

  left   = focal_to_near * (-.5 * aperture_x * scale_x + offset_x + translate_x );
  right  = focal_to_near * ( .5 * aperture_x * scale_x + offset_x + translate_x );
  bottom = focal_to_near * (-.5 * aperture_y * scale_y + offset_y + translate_y );
  top    = focal_to_near * ( .5 * aperture_y * scale_y + offset_y + translate_y );
}

/**
 * Get information about the given camera.
 */
void liqRibTranslator::getCameraInfo( MFnCamera& cam )
{
  // Resolution can change if camera film-gate clips image
  // so we must keep camera width/height separate from render
  // globals width/height.
  //
  cam_width  = width;
  cam_height = height;

  // If we are using a film-gate then we may need to
  // adjust the resolution to simulate the 'letter-boxed'
  // effect.
  if ( cam.filmFit() == MFnCamera::kHorizontalFilmFit ) {
    if ( !ignoreFilmGate ) {
      double new_height = cam_width /
        ( cam.horizontalFilmAperture() /
          cam.verticalFilmAperture() );

      if ( new_height < cam_height ) {
        cam_height = ( int )new_height;
      }
    }

    double hfov, vfov;
    portFieldOfView( cam_width, cam_height, hfov, vfov, cam );
    fov_ratio = hfov / vfov;
  }
  else if ( cam.filmFit() == MFnCamera::kVerticalFilmFit ) {
    double new_width = cam_height /
      ( cam.verticalFilmAperture() /
        cam.horizontalFilmAperture() );

    double hfov, vfov;

    // case 1 : film-gate smaller than resolution
    //         film-gate on
    if ( ( new_width < cam_width ) && ( !ignoreFilmGate ) ) {
      cam_width = ( int )new_width;
      fov_ratio = 1.0;
    }

    // case 2 : film-gate smaller than resolution
    //         film-gate off
    else if ( ( new_width < cam_width ) && ( ignoreFilmGate ) ) {
      portFieldOfView( ( int )new_width, cam_height, hfov, vfov, cam );
      fov_ratio = hfov / vfov;
    }

    // case 3 : film-gate larger than resolution
    //         film-gate on
    else if ( !ignoreFilmGate ) {
      portFieldOfView( ( int )new_width, cam_height, hfov, vfov, cam );
      fov_ratio = hfov / vfov;
    }

    // case 4 : film-gate larger than resolution
    //         film-gate off
    else if ( ignoreFilmGate ) {
      portFieldOfView( ( int )new_width, cam_height, hfov, vfov, cam );
      fov_ratio = hfov / vfov;
    }

  }
  else if ( cam.filmFit() == MFnCamera::kOverscanFilmFit ) {
    double new_height = cam_width /
      ( cam.horizontalFilmAperture() /
        cam.verticalFilmAperture() );
    double new_width = cam_height /
      ( cam.verticalFilmAperture() /
        cam.horizontalFilmAperture() );

    if ( new_width < cam_width ) {
      if ( !ignoreFilmGate ) {
        cam_width = ( int ) new_width;
        fov_ratio = 1.0;
      }
      else {
        double hfov, vfov;
        portFieldOfView( ( int )new_width, cam_height, hfov, vfov, cam );
        fov_ratio = hfov / vfov;
      }
    }
    else {
      if ( !ignoreFilmGate )
        cam_height = ( int ) new_height;

      double hfov, vfov;
      portFieldOfView( cam_width, cam_height, hfov, vfov, cam );
      fov_ratio = hfov / vfov;
    }
  }
  else if ( cam.filmFit() == MFnCamera::kFillFilmFit ) {
    double new_width = cam_height /
      ( cam.verticalFilmAperture() /
        cam.horizontalFilmAperture() );
    double hfov, vfov;

    if ( new_width >= cam_width ) {
      portFieldOfView( ( int )new_width, cam_height, hfov, vfov, cam );
      fov_ratio = hfov / vfov;
    }
    else {
      portFieldOfView( cam_width, cam_height, hfov, vfov, cam );
      fov_ratio = hfov / vfov;
    }
  }
}

/**
 * Set up data for the current job.
 */
MStatus liqRibTranslator::buildJobs()
{
  LIQDEBUGPRINTF( "-> beginning to build job list\n" );
  MStatus returnStatus = MS::kSuccess;
  MStatus status;
  MObject cameraNode;
  MDagPath lightPath;
  jobList.clear();
  shadowList.clear();
  structJob thisJob;

  // what we do here is make all of the lights with depth shadows turned on into
  // cameras and add them to the renderable camera list *before* the main camera
  // so all the automatic depth map shadows are complete before the main pass

  if ( liqglo_doShadows ) {
    MItDag dagIterator( MItDag::kDepthFirst, MFn::kLight, &returnStatus );
    for (; !dagIterator.isDone(); dagIterator.next()) {
      if ( !dagIterator.getPath(lightPath) ) continue;
      bool usesDepthMap = false;
      MFnLight fnLightNode( lightPath );
      fnLightNode.findPlug( "useDepthMapShadows" ).getValue( usesDepthMap );
      if ( usesDepthMap && areObjectAndParentsVisible( lightPath ) ) {

        //
        // We have a shadow job, so find out if we need to use deep shadows,
        // and the pixel sample count
        //

        thisJob.deepShadows = false;
        thisJob.shadowPixelSamples = 1;
        thisJob.shadowVolumeInterpretation = 1;

        // Get to our shader node.
        //
        MPlug liquidLightShaderNodeConnection;
        MStatus liquidLightShaderStatus;
        liquidLightShaderNodeConnection = fnLightNode.findPlug( "liquidLightShaderNode", &liquidLightShaderStatus );
        if ( liquidLightShaderStatus == MS::kSuccess && liquidLightShaderNodeConnection.isConnected() )
        {
          MPlugArray liquidLightShaderNodePlugArray;
          liquidLightShaderNodeConnection.connectedTo( liquidLightShaderNodePlugArray, true, true );
          MFnDependencyNode fnLightShaderNode( liquidLightShaderNodePlugArray[0].node() );

          // Now grab the parameters.
          //
          fnLightShaderNode.findPlug( "deepShadows" ).getValue( thisJob.deepShadows );

          // Only use the pixel samples and volume interpretation with deep shadows.
          //
          if ( thisJob.deepShadows )
          {
            fnLightShaderNode.findPlug( "pixelSamples" ).getValue( thisJob.shadowPixelSamples );
            fnLightShaderNode.findPlug( "volumeInterpretation" ).getValue( thisJob.shadowVolumeInterpretation );
          }
        }

        if ( lightPath.hasFn(MFn::kSpotLight) || lightPath.hasFn(MFn::kDirectionalLight) ) {
          thisJob.hasShadowCam = false;
          MPlug liquidLightShaderNodeConnection;
          MStatus liquidLightShaderStatus;
          liquidLightShaderNodeConnection = fnLightNode.findPlug( "liquidLightShaderNode", &liquidLightShaderStatus );
          if ( liquidLightShaderStatus == MS::kSuccess && liquidLightShaderNodeConnection.isConnected() ) {
            MPlugArray liquidLightShaderNodePlugArray;
            liquidLightShaderNodeConnection.connectedTo( liquidLightShaderNodePlugArray, true, true );
            MFnDependencyNode fnLightShaderNode( liquidLightShaderNodePlugArray[0].node() );
            MString shadowCamera;
            fnLightShaderNode.findPlug( "shadowCamera" ).getValue( shadowCamera );
            if ( shadowCamera != "" ) {
              MString parsingString = shadowCamera;
              MString shadowCamera = parseString( parsingString );
              MSelectionList camList;
              MDagPath cameraPath;
              camList.add( shadowCamera );
              camList.getDagPath( 0, cameraPath );
              thisJob.shadowCamPath = cameraPath;
              thisJob.hasShadowCam = true;
            }
          }
          thisJob.path = lightPath;
          thisJob.name = fnLightNode.name();
          thisJob.isShadow = true;
          thisJob.isPoint = false;
          thisJob.isShadowPass = false;

          // check to see if the minmax shadow option is used
          thisJob.isMinMaxShadow = false;
          status.clear();
          MPlug liquidMinMaxShadow = fnLightNode.findPlug( "liquidMinMaxShadow", &status );
          if ( status == MS::kSuccess ) {
            liquidMinMaxShadow.getValue( thisJob.isMinMaxShadow );
          }

          // check to see if the midpoint shadow option is used
          thisJob.isMidPointShadow = false;
          status.clear();
          MPlug liquidMidPointShadow = fnLightNode.findPlug( "useMidDistDmap", &status );
          if ( status == MS::kSuccess ) {
            liquidMidPointShadow.getValue( thisJob.isMidPointShadow );
          }

          bool computeShadow = true;
          if ( m_lazyCompute ) {
            MString baseFileName;
            /* if ( ( liqglo_DDimageName[0] == "" ) ) {
              baseFileName = liqglo_sceneName;
            } else {
              int pointIndex = liqglo_DDimageName[0].index( '.' );
              baseFileName = liqglo_DDimageName[0].substring(0, pointIndex-1).asChar();
            } */
            baseFileName = liqglo_sceneName;
            baseFileName += "_";
            baseFileName += thisJob.name;
            baseFileName += "SHD";

            // TODO - this little block of code below (up to sprintf) is almost identical
            // in a few places just within this file. should be rationalised into a
            // 'name generator' type of function

            MString outFileFmtString;
            outFileFmtString = liqglo_textureDir;
            outFileFmtString += baseFileName;

            size_t outNameLength;

            outNameLength = outFileFmtString.length();

            if( m_animation || m_useFrameExt ) {
              outFileFmtString += LIQ_ANIM_EXT;
              outNameLength += m_outPadding + 1;
            }
            outFileFmtString += ".";
            if( thisJob.deepShadows )
            {
              outFileFmtString += "shd";
            }
            else
            {
              outFileFmtString += "tex";
            }
            outNameLength = outFileFmtString.length();
            // Hmmmmmm protect from buffer overflow ...
            outNameLength = outNameLength + 8; // Space for the null character

            char *outName = (char *)alloca(outNameLength);
            sprintf(outName, outFileFmtString.asChar(), 1, liqglo_lframe);
            MString fileName( outName );
            if ( fileExists( fileName ) ) computeShadow = false;
          }

          if ( computeShadow ) jobList.push_back( thisJob );

          // We have to handle point lights differently as they need 6 shadow maps!
        } else if ( lightPath.hasFn(MFn::kPointLight) ) {
          for ( int dirOn = 0; dirOn < 6; dirOn++ ) {
            thisJob.hasShadowCam = false;
            thisJob.path = lightPath;
            thisJob.name = fnLightNode.name();
            thisJob.isShadow = true;
            thisJob.isShadowPass = false;
            thisJob.isPoint = true;
            //thisJob.pointDir = (enum PointLightDirection)dirOn;

            bool computeShadow = true;
            MString baseFileName;
            /*
            if ( ( liqglo_DDimageName[0] == "" ) ) {
              baseFileName = liqglo_sceneName;
            } else  {
              int pointIndex = liqglo_DDimageName[0].index( '.' );
              baseFileName = liqglo_DDimageName[0].substring(0, pointIndex-1).asChar();
              baseFileName = parse imageName
            }*/
            baseFileName = liqglo_sceneName;
            baseFileName += "_";
            baseFileName += thisJob.name;
            baseFileName += "SHD";

            if ( dirOn == pPX ) { baseFileName += "_PX"; thisJob.pointDir = pPX; } else
              if ( dirOn == pPY ) { baseFileName += "_PY"; thisJob.pointDir = pPY; } else
                if ( dirOn == pPZ ) { baseFileName += "_PZ"; thisJob.pointDir = pPZ; } else
                  if ( dirOn == pNX ) { baseFileName += "_NX"; thisJob.pointDir = pNX; } else
                    if ( dirOn == pNY ) { baseFileName += "_NY"; thisJob.pointDir = pNY; } else
                      if ( dirOn == pNZ ) { baseFileName += "_NZ"; thisJob.pointDir = pNZ; }
            // TODO - this little block of code below (up to sprintf) is almost identical
            // in a few places just within this file. should be rationalised into a
            // 'name generator' type of function
            MString outFileFmtString;
            outFileFmtString = liqglo_textureDir;
            outFileFmtString += baseFileName;

            size_t outNameLength;

            outNameLength = outFileFmtString.length();

            if( m_animation || m_useFrameExt ) {
              outFileFmtString += LIQ_ANIM_EXT;
              outNameLength += m_outPadding + 1;
            }
            outFileFmtString += ".";
            if( thisJob.deepShadows )
            {
              outFileFmtString += "shd";
            }
            else
            {
              outFileFmtString += "tex";
            }
            outNameLength = outFileFmtString.length();
            // Hmmmm protect from buffer overflow ....
            outNameLength = outNameLength + 8; // Space for the null character

            char *outName = (char *)alloca(outNameLength);
            sprintf(outName, outFileFmtString.asChar(), 1, liqglo_lframe);
            MString fileName( outName );
            if ( m_lazyCompute ) {
              if ( fileExists( fileName ) ) computeShadow = false;
            }
            if ( computeShadow ) jobList.push_back( thisJob );
          }
        }
      } // useDepthMap
    } // dagIterator
  } // liqglo_doShadows

  // Determine which cameras to render
  // it will either traverse the dag and find all the renderable cameras or
  // grab the current view and render that as a camera - both get added to the
  // end of the renderable camera array
  MDagPath cameraPath;
  if ( renderCamera != "" ) {
    MSelectionList camList;
    camList.add( renderCamera );
    camList.getDagPath( 0, cameraPath );
    MFnCamera fnCameraNode( cameraPath );
  } else {
    LIQDEBUGPRINTF( "-> getting current view\n" );
    m_activeView.getCamera( cameraPath );
  }
  MFnCamera fnCameraNode( cameraPath );
  thisJob.isPoint = false;
  thisJob.path = cameraPath;
  thisJob.name = fnCameraNode.name();
  thisJob.isShadow = false;
  thisJob.name += "SHADOWPASS";
  thisJob.isShadowPass = true;
  if ( m_outputShadowPass ) jobList.push_back( thisJob );
  thisJob.name = fnCameraNode.name();
  thisJob.isShadowPass = false;
  if ( m_outputHeroPass ) jobList.push_back( thisJob );
  liqglo_shutterTime = fnCameraNode.shutterAngle() * 0.5 / M_PI;

  // If we didn't find a renderable camera then give up
  if ( jobList.size() == 0 ) {
    MString cError("No Renderable Camera Found!\n");
    throw( cError );
    return MS::kFailure;
  }

  // step through the jobs and setup their names
  std::vector<structJob>::iterator iter = jobList.begin();
  while ( iter != jobList.end() ) {
    LIQ_CHECK_CANCEL_REQUEST;

    MString baseFileName;
    baseFileName = liqglo_sceneName;

    MString passname = iter->name;
    int s = passname.length();
    MString pass = iter->name.substring( s-10, s-1 );

    if ( liqglo_beautyRibHasCameraName || iter->isShadow || pass == "SHADOWPASS" ) {
      baseFileName += "_";
      baseFileName += iter->name;
    }

    if ( iter->isShadow ) {
      baseFileName += "SHD";
      if ( iter->isPoint ) {
        if ( iter->pointDir == pPX ) { baseFileName += "_PX"; } else
          if ( iter->pointDir == pPY ) { baseFileName += "_PY"; } else
            if ( iter->pointDir == pPZ ) { baseFileName += "_PZ"; } else
              if ( iter->pointDir == pNX ) { baseFileName += "_NX"; } else
                if ( iter->pointDir == pNY ) { baseFileName += "_NY"; } else
                  if ( iter->pointDir == pNZ ) { baseFileName += "_NZ"; }
      }
    }

    MString fileNameFmtString;

    fileNameFmtString = liqglo_ribDir;
    fileNameFmtString += baseFileName;
    fileNameFmtString += LIQ_ANIM_EXT;
    fileNameFmtString += extension;


    // TODO - this little block of code below (up to sprintf) is almost identical
    // in a few places just within this file. should be rationalised into a
    // 'name generator' type of function

    size_t fileNameLength = fileNameFmtString.length() + 8;
    fileNameLength += 10; // Enough to hold the digits for 1 billion frames
    char *frameFileName = (char *)alloca(fileNameLength);
    sprintf( frameFileName, fileNameFmtString.asChar(), doExtensionPadding ? m_outPadding : 0, liqglo_lframe );
    iter->ribFileName = frameFileName;

    MString outFileFmtString;

    if ( iter->isShadow ) {
      status.clear();
      MString userShadowName;
      MFnDagNode lightNode( iter->path );
      MPlug userShadowNamePlug = lightNode.findPlug( "liquidShadowName", &status );
      if ( status == MS::kSuccess ) {
        MString varVal;
        userShadowNamePlug.getValue( varVal );
        userShadowName = parseString( varVal );
      }
      outFileFmtString = liqglo_textureDir;
      size_t outNameLength;
      if ( userShadowName != MString( "" ) ) {
        outFileFmtString += userShadowName;
        outNameLength = outFileFmtString.length() + 1;
      } else {
        outFileFmtString += baseFileName;

        outNameLength = outFileFmtString.length();

        if( m_animation || m_useFrameExt ) {
          outFileFmtString += LIQ_ANIM_EXT;
          outNameLength += m_outPadding + 1;
        }
        if ( thisJob.deepShadows )
        {
          outFileFmtString += ".shd";
        }
        else
        {
          outFileFmtString += ".tex";
        }
        outNameLength = outFileFmtString.length();
        outNameLength = outNameLength + ( m_outPadding + 1 ); // Space for the null character
      }

      char *outName = (char *)alloca(outNameLength);

      if ( userShadowName != MString( "" ) ) {
        strcpy(outName, outFileFmtString.asChar() );
      } else {
        sprintf(outName, outFileFmtString.asChar(), 1, liqglo_lframe);
      }
      iter->imageName = outName;
      thisJob = *iter;
      if ( iter->isShadow ) shadowList.push_back( thisJob );
    } else {
      outFileFmtString = m_pixDir;
      outFileFmtString += baseFileName;

      size_t outNameLength;

      outNameLength = outFileFmtString.length();

      if( m_animation || m_useFrameExt ) {
        outFileFmtString += LIQ_ANIM_EXT;
        outNameLength += m_outPadding + 1;
      }

      outFileFmtString += ".";
      outFileFmtString += outExt;
      outNameLength = outFileFmtString.length();
      outNameLength = outNameLength + ( m_outPadding + 1 ); // Space for the null character

      char *outName = (char *)alloca(outNameLength);
      sprintf(outName, outFileFmtString.asChar(), m_outPadding, liqglo_lframe);
      /*if ( liqglo_DDimageName[0] == "" ) {
        iter->imageName = outName;
      } else {
        iter->imageName = m_pixDir;
        iter->imageName += parseString( liqglo_DDimageName[0] );
      }*/
      iter->imageName = outName;

    }
    ++iter;
  }

  ribStatus = kRibBegin;
  return MS::kSuccess;
}

/**
 * Write the prologue for the RIB file.
 * This includes all RI options but not the camera transformation.
 */
MStatus liqRibTranslator::ribPrologue()
{
  if ( !m_exportReadArchive ) {
    LIQDEBUGPRINTF( "-> beginning to write prologue\n" );
    // set any rib options
    RiOption( "limits", "bucketsize", ( RtPointer ) &bucketSize, RI_NULL );
    RiOption( "limits", "gridsize", ( RtPointer ) &gridSize, RI_NULL );
    RiOption( "limits", "texturememory", ( RtPointer) &textureMemory, RI_NULL );
    if ( liquidRenderer.supports_EYESPLITS ) {
      RiOption( "limits", "eyesplits", ( RtPointer ) &eyeSplits, RI_NULL );
    }

    RtString list = const_cast< char* > ( liqglo_shaderPath.asChar() );
    RiOption( "searchpath", "shader", &list, RI_NULL );

    MString texturePath = liqglo_texturePath + ":" + liquidSanitizePath( liqglo_textureDir );
    list = const_cast< char* > ( texturePath.asChar() );
    RiOption( "searchpath", "texture", &list, RI_NULL );


    MString archivePath = liqglo_archivePath + ":" + liquidSanitizePath( liqglo_ribDir );

    list = const_cast< char* > ( archivePath.asChar() );

    RiOption( "searchpath", "archive", &list, RI_NULL );


    list = const_cast< char* > ( liqglo_proceduralPath.asChar() );
    RiOption( "searchpath", "procedural", &list, RI_NULL );

    RiOrientation( RI_RH ); // Right-hand coordinates
    if ( liqglo_currentJob.pass == rpShadowMap ) {
      RiPixelSamples( liqglo_currentJob.shadowPixelSamples,
                      liqglo_currentJob.shadowPixelSamples );
      RiShadingRate( 1 );
      // Need to use Box filter for deep shadows.
      RiPixelFilter( RiBoxFilter, 1, 1 );

      RtString option;
      if ( liqglo_currentJob.deepShadows ) {
        option = "deepshadow";
      } else {
        option = "shadow";
      }
      RiOption("user", "uniform string pass", (RtPointer)&option, RI_NULL);
    } else {
      RtString hiderName;
      switch( liqglo_hider ) {
        case htHidden:
    hiderName = "hidden";
    break;
        case htPhoton:
          hiderName = "photon";
    break;
        case htRaytrace:
    hiderName = "raytrace";
    break;
        case htOpenGL:
    hiderName = "OpenGL";
    break;
        case htZbuffer:
    hiderName = "zbuffer";
    break;
  default:
    hiderName = "hidden";
      }
      RiHider( hiderName, "jitter", &liqglo_jitter, RI_NULL );

      RiPixelSamples( pixelSamples, pixelSamples );
      RiShadingRate( shadingRate );
      if ( m_rFilterX > 1 || m_rFilterY > 1 ) {
        switch (m_rFilter) {
        case pfBoxFilter:
          RiPixelFilter( RiBoxFilter, m_rFilterX, m_rFilterY );
          break;
        case pfTriangleFilter:
          RiPixelFilter( RiTriangleFilter, m_rFilterX, m_rFilterY );
          break;
        case pfCatmullRomFilter:
          RiPixelFilter( RiCatmullRomFilter, m_rFilterX, m_rFilterY );
          break;
        case pfGaussianFilter:
          RiPixelFilter( RiGaussianFilter, m_rFilterX, m_rFilterY );
          break;
        case pfSincFilter:
          RiPixelFilter( RiSincFilter, m_rFilterX, m_rFilterY );
          break;
#if defined DELIGHT || PRMAN
        case pfBlackmanHarrisFilter:
          RiArchiveRecord( RI_VERBATIM, "PixelFilter \"blackman-harris\" %g %g\n", m_rFilterX, m_rFilterY);
          break;
        case pfMitchellFilter:
          RiArchiveRecord( RI_VERBATIM, "PixelFilter \"mitchell\" %g %g\n", m_rFilterX, m_rFilterY);
          break;
        case pfSepCatmullRomFilter:
          RiArchiveRecord( RI_VERBATIM, "PixelFilter \"separable-catmull-rom\" %g %g\n", m_rFilterX, m_rFilterY);
          break;
        case pfBesselFilter:
          RiPixelFilter( RiBesselFilter, m_rFilterX, m_rFilterY );
          break;
#endif
#ifdef PRMAN
        case pfLanczosFilter:
          RiPixelFilter( RiLanczosFilter, m_rFilterX, m_rFilterY );
          break;
        case pfDiskFilter:
          RiPixelFilter( RiDiskFilter, m_rFilterX, m_rFilterY );
          break;
#endif
        default:
          RiArchiveRecord( RI_COMMENT, "Unknown Pixel Filter selected" );
          break;
        }
      }
    }

    // RAYTRACING OPTIONS
#if defined DELIGHT || PRMAN
  if ( rt_useRayTracing ) {
    RiArchiveRecord( RI_COMMENT, "Ray Tracing : ON" );
    RiOption( "trace",   "int maxdepth",                ( RtPointer ) &rt_traceMaxDepth,          RI_NULL );
    RiOption( "trace",   "float specularthreshold",     ( RtPointer ) &rt_traceSpecularThreshold, RI_NULL );
    RiOption( "trace",   "int continuationbydefault",   ( RtPointer ) &rt_traceRayContinuation,   RI_NULL );
    RiOption( "limits",  "int geocachememory",          ( RtPointer ) &rt_traceCacheMemory,       RI_NULL );
    RiOption( "user",    "float traceBreadthFactor",    ( RtPointer ) &rt_traceBreadthFactor,     RI_NULL );
    RiOption( "user",    "float traceDepthFactor",      ( RtPointer ) &rt_traceDepthFactor,       RI_NULL );
  } else {
    RiArchiveRecord( RI_COMMENT, "Ray Tracing : OFF" );
  }
#endif

  }
  ribStatus = kRibBegin;
  return MS::kSuccess;
}

/**
 * Write the epilogue for the RIB file.
 */
MStatus liqRibTranslator::ribEpilogue()
{
  if (ribStatus == kRibBegin) ribStatus = kRibOK;
  return (ribStatus == kRibOK ? MS::kSuccess : MS::kFailure);
}

/**
 * Scan the DAG at the given frame number and record information about the scene for writing.
 */
MStatus liqRibTranslator::scanScene(float lframe, int sample )
{

int count =0;

  MTime   mt((double)lframe, MTime::uiUnit());
  if ( MGlobal::viewFrame(mt) == MS::kSuccess) {

    // execute pre-frame command
    if ( m_preFrameMel != "" ) {
      if ( fileExists( m_preFrameMel  ) ) MGlobal::sourceFile( m_preFrameMel );
      else {
        if ( MS::kSuccess == MGlobal::executeCommand( m_preFrameMel, false, false ) ) {
          cout <<"Liquid -> pre-frame script executed successfully."<<endl<<flush;
        } else {
          cout <<"Liquid -> ERROR :pre-frame script failed."<<endl<<flush;
        }
      }
    }

    MStatus returnStatus;
    // Scan the scene for lights
    {
      MItDag dagLightIterator( MItDag::kDepthFirst, MFn::kLight, &returnStatus);

      for (; !dagLightIterator.isDone(); dagLightIterator.next()) {
        LIQ_CHECK_CANCEL_REQUEST;
        MDagPath path;
        MObject currentNode;
        currentNode = dagLightIterator.item();
        MFnDagNode dagNode;
        dagLightIterator.getPath( path );
        if (MS::kSuccess != returnStatus) continue;
        if (!currentNode.hasFn(MFn::kDagNode)) continue;
        returnStatus = dagNode.setObject( currentNode );
        if (MS::kSuccess != returnStatus) continue;

        // if it's a light then insert it into the hash table
        if ( currentNode.hasFn( MFn::kLight ) ) {
          if ( ( sample > 0 ) && isObjectMotionBlur( path )) {
            htable->insert(path, lframe, sample, MRT_Light,count++ );
          } else {
            htable->insert(path, lframe, 0, MRT_Light,count++ );
          }
          continue;
        }
      }
    }

    // Scan the scene for place3DTextures -- for coordinate systems
    {
      MItDag dagLightIterator( MItDag::kDepthFirst, MFn::kPlace3dTexture, &returnStatus);

      for (; !dagLightIterator.isDone(); dagLightIterator.next()) {
        LIQ_CHECK_CANCEL_REQUEST;
        MDagPath path;
        MObject currentNode;
        currentNode = dagLightIterator.item();
        MFnDagNode dagNode;
        dagLightIterator.getPath( path );
        if (MS::kSuccess != returnStatus) continue;
        if (!currentNode.hasFn(MFn::kDagNode)) continue;
        returnStatus = dagNode.setObject( currentNode );
        if (MS::kSuccess != returnStatus) continue;

        // if it's a coordinate system then insert it into the hash table
        if ( currentNode.hasFn( MFn::kPlace3dTexture ) ) {
          if ( ( sample > 0 ) && isObjectMotionBlur( path )) {
            htable->insert(path, lframe, sample, MRT_Coord,count++ );
          } else {
            htable->insert(path, lframe, 0, MRT_Coord,count++ );
          }
          continue;
        }
      }
    }

    if ( !m_renderSelected ) {
      MItDag dagIterator( MItDag::kDepthFirst, MFn::kInvalid, &returnStatus);
      for (; !dagIterator.isDone(); dagIterator.next()) {
        LIQ_CHECK_CANCEL_REQUEST;
        MDagPath path;
        MObject currentNode;
        currentNode = dagIterator.item();
        MFnDagNode   dagNode;
        dagIterator.getPath( path );
        if (MS::kSuccess != returnStatus) continue;
        if (!currentNode.hasFn(MFn::kDagNode)) continue;
        returnStatus = dagNode.setObject( currentNode );
        if (MS::kSuccess != returnStatus) continue;

        // check for a rib generator
        MStatus plugStatus;
        MPlug ribGenPlug = dagNode.findPlug( "liquidRibGen", &plugStatus );
        if ( plugStatus == MS::kSuccess ) {
          // check the node to make sure it's not using the old ribGen assignment method, this is for backwards
          // compatibility.  If it's a kTypedAttribute that it's more than likely going to be a string!
          if ( ribGenPlug.attribute().apiType() == MFn::kTypedAttribute ) {
            MString ribGenNode;
            ribGenPlug.getValue( ribGenNode );
            MSelectionList ribGenList;
            MStatus ribGenAddStatus = ribGenList.add( ribGenNode );
            if ( ribGenAddStatus == MS::kSuccess ) {
              htable->insert( path, lframe, sample, MRT_RibGen,count++ );
            }
          } else {
            if ( ribGenPlug.isConnected() ) {
              htable->insert( path, lframe, sample, MRT_RibGen,count++ );
            }
          }

        }
        if ( currentNode.hasFn(MFn::kNurbsSurface)
             || currentNode.hasFn(MFn::kMesh)
             || currentNode.hasFn(MFn::kParticle)
             || currentNode.hasFn(MFn::kLocator)
             || currentNode.hasFn( MFn::kSubdiv) ) {
          if ( ( sample > 0 ) && isObjectMotionBlur( path )){
            htable->insert(path, lframe, sample, MRT_Unknown,count++ );
          } else {
            htable->insert(path, lframe, 0, MRT_Unknown,count++ );
          }
        }
        if ( currentNode.hasFn(MFn::kNurbsCurve) ) {
          MStatus plugStatus;
          MPlug renderCurvePlug = dagNode.findPlug( "liquidCurve", &plugStatus );
          if ( m_renderAllCurves || ( plugStatus == MS::kSuccess ) ) {
            bool renderCurve = false;
            renderCurvePlug.getValue( renderCurve );
            if( renderCurve ) {
              if ( ( sample > 0 ) && isObjectMotionBlur( path )){
                htable->insert(path, lframe, sample, MRT_Unknown,count++ );
              } else {
                htable->insert(path, lframe, 0, MRT_Unknown,count++ );
              }
            }
          }
        }
      }

      // Now deal with all the particle-instanced objects (where a
      // particle is replaced by an object or group of objects).
      //
      MItInstancer instancerIter;
      while( ! instancerIter.isDone() )
      {
        MDagPath path = instancerIter.path();
        MString instanceStr = (MString)"|INSTANCE_" +
          instancerIter.instancerId() + (MString)"_" +
          instancerIter.particleId() + (MString)"_" +
          instancerIter.pathId();

        MMatrix instanceMatrix = instancerIter.matrix();

        if ( ( sample > 0 ) && isObjectMotionBlur( path )){
          htable->insert( path, lframe, sample, MRT_Unknown,count++,
                          &instanceMatrix, instanceStr, instancerIter.particleId() );
        } else {
          htable->insert( path, lframe, 0, MRT_Unknown,count++,
                          &instanceMatrix, instanceStr, instancerIter.particleId() );
        }
        instancerIter.next();
      }
    } else {
      //find out the current selection for possible selected object output
      MSelectionList currentSelection;
      MGlobal::getActiveSelectionList( currentSelection );
      MItSelectionList selIterator( currentSelection );
      MItDag dagIterator( MItDag::kDepthFirst, MFn::kInvalid, &returnStatus);
      for ( ; !selIterator.isDone(); selIterator.next() )
      {
        MDagPath objectPath;
        selIterator.getDagPath( objectPath );
        dagIterator.reset (objectPath.node(), MItDag::kDepthFirst, MFn::kInvalid );
        for (; !dagIterator.isDone(); dagIterator.next()) {
          LIQ_CHECK_CANCEL_REQUEST;
          MDagPath path;
          MObject currentNode;
          currentNode = dagIterator.item();
          MFnDagNode   dagNode;
          dagIterator.getPath( path );
          if (MS::kSuccess != returnStatus) continue;
          if (!currentNode.hasFn(MFn::kDagNode)) continue;
          returnStatus = dagNode.setObject( currentNode );
          if (MS::kSuccess != returnStatus) continue;

          // check for a rib generator
          MStatus plugStatus;
          MPlug ribGenPlug = dagNode.findPlug( "liquidRibGen", &plugStatus );
          if ( plugStatus == MS::kSuccess ) {
            htable->insert( path, lframe, sample, MRT_RibGen,count++ );
          }
          if ( currentNode.hasFn(MFn::kNurbsSurface)
               || currentNode.hasFn(MFn::kMesh)
               || currentNode.hasFn(MFn::kParticle)
               || currentNode.hasFn(MFn::kLocator) ) {
            if ( ( sample > 0 ) && isObjectMotionBlur( path )){
              htable->insert(path, lframe, sample, MRT_Unknown,count++ );
            } else {
              htable->insert(path, lframe, 0, MRT_Unknown,count++ );
            }
          }
          if ( currentNode.hasFn(MFn::kNurbsCurve) ) {
            MStatus plugStatus;
            MPlug renderCurvePlug = dagNode.findPlug( "liquidCurve", &plugStatus );
            if ( m_renderAllCurves || ( plugStatus == MS::kSuccess ) ) {
              bool renderCurve = false;
              renderCurvePlug.getValue( renderCurve );
              if( renderCurve ) {
                if ( ( sample > 0 ) && isObjectMotionBlur( path )){
                  htable->insert(path, lframe, sample, MRT_Unknown,count++ );
                } else {
                  htable->insert(path, lframe, 0, MRT_Unknown,count++ );
                }
              }
            }
          }
        }
      }

      // Now deal with all the particle-instanced objects (where a
      // particle is replaced by an object or group of objects).
      //
      MItInstancer instancerIter;
      while( ! instancerIter.isDone() )
      {
        MDagPath path = instancerIter.path();
        MString instanceStr = (MString)"|INSTANCE_" +
          instancerIter.instancerId() + (MString)"_" +
          instancerIter.particleId() + (MString)"_" +
          instancerIter.pathId();

        MMatrix instanceMatrix = instancerIter.matrix();

        if ( ( sample > 0 ) && isObjectMotionBlur( path )){
          htable->insert( path, lframe, sample, MRT_Unknown,count++,
                          &instanceMatrix, instanceStr, instancerIter.particleId() );
        } else {
          htable->insert( path, lframe, 0, MRT_Unknown,count++,
                          &instanceMatrix, instanceStr, instancerIter.particleId() );
        }
        instancerIter.next();
      }
    }


    std::vector<structJob>::iterator iter = jobList.begin();
    while ( iter != jobList.end() ) {
      LIQ_CHECK_CANCEL_REQUEST;
      // Get the camera/light info for this job at this frame
      MStatus status;

      if ( !iter->isShadow ) {
        MDagPath path;
        MFnCamera   fnCamera( iter->path );
        iter->gotJobOptions = false;
        status.clear();
        MPlug cPlug = fnCamera.findPlug( MString( "ribOptions" ), &status );
        if ( status == MS::kSuccess ) {
          cPlug.getValue( iter->jobOptions );
          iter->gotJobOptions = true;
        }
        getCameraInfo( fnCamera );
        iter->width = cam_width;
        iter->height = cam_height;
        // Renderman specifies shutter by time open
        // so we need to convert shutterAngle to time.
        // To do this convert shutterAngle to degrees and
        // divide by 360.
        //
        iter->camera[sample].shutter = fnCamera.shutterAngle() * 0.5 / M_PI;
        liqglo_shutterTime = iter->camera[sample].shutter;
        iter->camera[sample].orthoWidth = fnCamera.orthoWidth();
        iter->camera[sample].orthoHeight = fnCamera.orthoWidth() * ((float)cam_height / (float)cam_width);
        iter->camera[sample].motionBlur = fnCamera.isMotionBlur();
        iter->camera[sample].focalLength = fnCamera.focalLength();
        iter->camera[sample].focalDistance = fnCamera.focusDistance();
        iter->camera[sample].fStop = fnCamera.fStop();

        fnCamera.getPath(path);
        MTransformationMatrix xform( path.inclusiveMatrix() );
        double scale[] = { 1, 1, -1 };

        xform.setScale( scale, MSpace::kTransform );

        // philippe : rotate the main camera 90 degrees around Z-axis if necessary
        // ( only in main camera )
        MMatrix camRotMatrix;
        if ( liqglo_rotateCamera == true ) {
          float crm[4][4] = {  {  0.0,  1.0,  0.0,  0.0 },
                               { -1.0,  0.0,  0.0,  0.0 },
                               {  0.0,  0.0,  1.0,  0.0 },
                               {  0.0,  0.0,  0.0,  1.0 }};
          camRotMatrix = crm;
        }
        iter->camera[sample].mat = xform.asMatrixInverse() * camRotMatrix;

        if ( fnCamera.isClippingPlanes() ) {
          iter->camera[sample].neardb    = fnCamera.nearClippingPlane();
          iter->camera[sample].fardb    = fnCamera.farClippingPlane();
        } else {
          iter->camera[sample].neardb    = 0.001;    // TODO: these values are duplicated elsewhere in this file
          iter->camera[sample].fardb    = 250000.0; // TODO: these values are duplicated elsewhere in this file
        }
        iter->camera[sample].isOrtho = fnCamera.isOrtho();

        // The camera's fov may not match the rendered image in Maya
        // if a film-fit is used. 'fov_ratio' is used to account for
        // this.
        //
        iter->camera[sample].hFOV = fnCamera.horizontalFieldOfView()/fov_ratio;
        iter->aspectRatio = aspectRatio;

        // Determine what information to write out (RGB, alpha, zbuffer)
        //
        iter->imageMode.clear();

        bool isOn;
        MPlug boolPlug;
        boolPlug = fnCamera.findPlug( "image" );

        boolPlug.getValue( isOn );
        if ( isOn ) {
          // We are writing RGB info
          //
          iter->imageMode = "rgb";
          iter->format = outFormat;
        }
        boolPlug = fnCamera.findPlug( "mask" );
        boolPlug.getValue( isOn );
        if ( isOn ) {
          // We are writing alpha channel info
          //
          iter->imageMode += "a";
          iter->format = outFormat;
        }
        boolPlug = fnCamera.findPlug( "depth" );
        boolPlug.getValue( isOn );
        if ( isOn ) {
          // We are writing z-buffer info
          //
          iter->imageMode = "z";
          iter->format = "zfile";
        }
      } else { // doing shadow render
        MDagPath path;
        MFnLight   fnLight( iter->path );
        status.clear();

        iter->gotJobOptions = false;
        MPlug cPlug = fnLight.findPlug( MString( "ribOptions" ), &status );
        if ( status == MS::kSuccess ) {
          cPlug.getValue( iter->jobOptions );
          iter->gotJobOptions = true;
        }
        MPlug lightPlug = fnLight.findPlug( "dmapResolution" );
        float dmapSize;
        lightPlug.getValue( dmapSize );
        iter->height = iter->width = (int)dmapSize;
        if ( iter->hasShadowCam ) {
          MFnCamera fnCamera( iter->shadowCamPath );
          fnCamera.getPath(path);
          MTransformationMatrix xform( path.inclusiveMatrix() );
          double scale[] = { 1, 1, -1 };
          xform.setScale( scale, MSpace::kTransform );

          iter->camera[sample].mat         = xform.asMatrixInverse();
          iter->camera[sample].neardb      = fnCamera.nearClippingPlane();
          iter->camera[sample].fardb       = fnCamera.farClippingPlane();
          iter->camera[sample].isOrtho     = fnCamera.isOrtho();
          iter->camera[sample].orthoWidth  = fnCamera.orthoWidth();
          iter->camera[sample].orthoHeight = fnCamera.orthoWidth();
        } else {
          fnLight.getPath(path);
          MTransformationMatrix xform( path.inclusiveMatrix() );
          double scale[] = { 1, 1, -1 };
          xform.setScale( scale, MSpace::kTransform );

          if ( iter->isPoint ) {
            double ninty = M_PI/2;
            if ( iter->pointDir == pPX ) { double rotation[] = { 0, -ninty, 0 }; xform.setRotation( rotation, MTransformationMatrix::kXYZ ); }
            if ( iter->pointDir == pNX ) { double rotation[] = { 0, ninty, 0 }; xform.setRotation( rotation, MTransformationMatrix::kXYZ ); }
            if ( iter->pointDir == pPY ) { double rotation[] = { ninty, 0, 0 }; xform.setRotation( rotation, MTransformationMatrix::kXYZ ); }
            if ( iter->pointDir == pNY ) { double rotation[] = { -ninty, 0, 0 }; xform.setRotation( rotation, MTransformationMatrix::kXYZ ); }
            if ( iter->pointDir == pPZ ) { double rotation[] = { 0, M_PI, 0 }; xform.setRotation( rotation, MTransformationMatrix::kXYZ ); }
            if ( iter->pointDir == pNZ ) { double rotation[] = { 0, 0, 0 }; xform.setRotation( rotation, MTransformationMatrix::kXYZ ); }
          }
          iter->camera[sample].mat = xform.asMatrixInverse();
          iter->camera[sample].neardb   = 0.001;    // TODO: these values are duplicated elsewhere in this file
          iter->camera[sample].fardb    = 250000.0; // TODO: these values are duplicated elsewhere in this file
          if ( fnLight.dagPath().hasFn( MFn::kDirectionalLight ) ) {
            iter->camera[sample].isOrtho = true;
            fnLight.findPlug( "dmapWidthFocus" ).getValue( iter->camera[sample].orthoWidth );
            fnLight.findPlug( "dmapWidthFocus" ).getValue( iter->camera[sample].orthoHeight );
          } else {
            iter->camera[sample].isOrtho = false;
            iter->camera[sample].orthoWidth = 0.0;
          }
        }

        if ( iter->deepShadows )
        {
          iter->camera[sample].shutter = liqglo_shutterTime;
          iter->camera[sample].motionBlur = true;
        }

        else
        {
          iter->camera[sample].shutter = 0;
          iter->camera[sample].motionBlur = false;
        }
        iter->camera[sample].focalLength = 0;
        iter->camera[sample].focalDistance = 0;
        iter->camera[sample].fStop = 0;
        //doCameraMotion = 0;

        iter->aspectRatio = 1.0;

        // The camera's fov may not match the rendered image in Maya
        // if a film-fit is used. 'fov_ratio' is used to account for
        // this.
        //
        if ( iter->hasShadowCam ) {
          MFnCamera fnCamera( iter->shadowCamPath );
          float camFov = fnCamera.horizontalFieldOfView();
          iter->camera[sample].hFOV = camFov;
        } else {
          MStatus coneStatus;
          lightPlug = fnLight.findPlug( "coneAngle", &coneStatus );
          if ( coneStatus == MS::kSuccess ) {
            lightPlug.getValue( iter->camera[sample].hFOV );
          } else {
            iter->camera[sample].hFOV = 95;
          }
        }

        // Determine what information to write out (RGB, alpha, zbuffer)
        //
        iter->imageMode.clear();
        if ( iter->deepShadows )
        {
          iter->imageMode += "deepopacity";
          iter->format = "deepshad";
        }
        else
        {
          iter->imageMode += "z";
          iter->format = "shadow";
        }
      }

      ++iter;
    }

    // post-frame script execution
    if ( m_postFrameMel != "" ) {
      if ( fileExists( m_postFrameMel  ) ) MGlobal::sourceFile( m_postFrameMel );
      else {
        if ( MS::kSuccess == MGlobal::executeCommand( m_postFrameMel, false, false ) ) {
          cout <<"Liquid -> post-frame script executed successfully."<<endl<<flush;
        } else {
          cout <<"Liquid -> ERROR :post-frame script failed."<<endl<<flush;
        }
      }
    }

    return MS::kSuccess;
  }
  return MS::kFailure;
}

/**
 * This method takes care of the blocking together of objects and their children in the DAG.
 * This method compares two DAG paths and figures out how many attribute levels to push and/or pop.
 * The intention seems clear but this method is not currently used -- anyone cares to comment? --Moritz.
 */
void liqRibTranslator::doAttributeBlocking( const MDagPath & newPath, const MDagPath & previousPath )
{
  int newDepth = newPath.length();
  int prevDepth = 0;
  MFnDagNode dagFn( newPath );
  MDagPath npath = newPath;
  MDagPath ppath = previousPath;

  if ( previousPath.isValid() ) {
    // Recursive base case
    // If the paths are the same, then we don't have to write
    // any start/end attribute blocks.  So, just return
    //
    if ( newPath == previousPath ) return;

    prevDepth = previousPath.length();

    // End attribute block if necessary
    //
    if ( newDepth <= prevDepth ) {
      // Write an attribute block end
      //
      RiAttributeEnd();
      attributeDepth--;
      if ( prevDepth > 1 ) {
        ppath.pop();
      }
    }
  }
  if ( ( newDepth >= prevDepth ) && ( newDepth > 1 ) ){
    npath.pop();
  }
  // Recurse and process parents
  //
  if ( ( prevDepth > 1 ) || ( newDepth > 1 ) ) {
    // Recurse
    //
    doAttributeBlocking( npath, ppath );
  }

  // Write open for new attribute block if necessary
  //
  if ( newDepth >= prevDepth ) {
    MString name = dagFn.name();

    RiAttributeBegin();
    RtString ribname = const_cast < char*>( name.asChar() );
    RiAttribute( "identifier", "name", &ribname, RI_NULL );

/*    if( ribNode->grouping.membership != "" ) {
      RtString members = const_cast<char*>( ribNode->grouping.membership.asChar() );
      RiAttribute( "grouping", "membership",&members, RI_NULL );
    }*/

    RiAttribute( "grouping", "membership", &name, RI_NULL );

    if ( newPath.hasFn( MFn::kTransform ) ) {
      // We have a transform, so write out the info
      //
      RtMatrix ribMatrix;
      MObject transform = newPath.node();
      MFnTransform transFn( transform );
      MTransformationMatrix localTransformMatrix = transFn.transformation();
      MMatrix localMatrix = localTransformMatrix.asMatrix();
      localMatrix.get( ribMatrix );
      RiConcatTransform( ribMatrix );
    }
    attributeDepth++;
  }
}

/**
 * Write out the frame prologue.
 * This includes all pass-dependant options like shading interpolation, hider,
 * display driver and the camera transformation.
 */
MStatus liqRibTranslator::framePrologue(long lframe)
{
  LIQDEBUGPRINTF( "-> Beginning Frame Prologue\n" );
  ribStatus = kRibFrame;

  if ( !m_exportReadArchive ) {

    RiFrameBegin( lframe );

    if ( !liqglo_currentJob.isShadow ) {
      // Smooth Shading
      RiShadingInterpolation( "smooth" );
      // Quantization
      if ( quantValue != 0 ) {
        int whiteValue = (int) pow( 2.0, quantValue ) - 1;
        RiQuantize( RI_RGBA, whiteValue, 0, whiteValue, 0.5 );
      } else {
        RiQuantize( RI_RGBA, 0, 0, 0, 0 );
      }
      if ( m_rgain != 1.0 || m_rgamma != 1.0 ) {
        RiExposure( m_rgain, m_rgamma );
      }
    }

    if ( liqglo_currentJob.isShadow &&
         ( !liqglo_currentJob.deepShadows ||
           liqglo_currentJob.shadowPixelSamples == 1 ) )
    {
      RtInt zero = 0;
      RiHider( "hidden", "jitter", &zero, RI_NULL );
    }

    if ( liqglo_currentJob.isMidPointShadow && !liqglo_currentJob.deepShadows ) {
      RtString midPoint = "midpoint";
      RiHider( "hidden", "depthfilter", &midPoint, RI_NULL );
    }

    LIQDEBUGPRINTF( "-> Setting Display Options\n" );

    if ( liqglo_currentJob.isShadow ) {
      MString relativeShadowName = LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, liqglo_currentJob.imageName, liqglo_projectDir );
      if ( !liqglo_currentJob.isMinMaxShadow ) {
        if ( liqglo_currentJob.deepShadows )
        {
          // RiDeclare( "volumeinterpretation", "string" );
          RtString viContinuous = "continuous";
          RtString viDiscrete   = "discrete";

#ifdef DELIGHT
          RiDisplay( const_cast<char *>( relativeShadowName.asChar()),
                     const_cast<char *>( liqglo_currentJob.format.asChar() ),
                     (RtToken)liqglo_currentJob.imageMode.asChar(),
                     "volumeinterpretation",
                     ( liqglo_currentJob.shadowVolumeInterpretation == 2 ? &viContinuous : &viDiscrete ),
                     RI_NULL );
#else
          // Deep shadows cannot be the primary output driver in PRMan & co.
          // We need to create a null output zfile first, and use the deep
          // shadows as a secondary output.
          //
          RiDisplay( "null", "null", "z", RI_NULL );
          MString deepFileImageName = "+" + relativeShadowName;
          RiDisplay( const_cast<char *>( deepFileImageName.asChar() ),
                     const_cast<char *>( liqglo_currentJob.format.asChar() ),
                     (RtToken)liqglo_currentJob.imageMode.asChar(),
                     "volumeinterpretation",
                     ( liqglo_currentJob.shadowVolumeInterpretation == 2 ? &viContinuous : &viDiscrete ),
                     RI_NULL );
#endif
        }
        else
        {
          RiDisplay( const_cast<char *>( relativeShadowName.asChar() ),
                     const_cast<char *>( liqglo_currentJob.format.asChar() ),
                     (RtToken)liqglo_currentJob.imageMode.asChar(),
                     RI_NULL );
        }
      } else {
        RiArchiveRecord( RI_COMMENT, "Display Driver:" );
        RtInt minmax = 1;
        RiDisplay( const_cast<char *>( relativeShadowName.asChar() ),
                   const_cast<char *>(liqglo_currentJob.format.asChar()),
                   (RtToken)liqglo_currentJob.imageMode.asChar(),
                   "minmax",
                   &minmax,
                   RI_NULL );
      }
    } else {
      if ( ( m_cropX1 != 0.0 ) || ( m_cropY1 != 0.0 ) || ( m_cropX2 != 1.0 ) || ( m_cropY2 != 1.0 ) ) {
        // philippe : handle the rotated camera case
        if ( liqglo_rotateCamera == true ) {
          RiCropWindow( m_cropY2, m_cropY1, 1 - m_cropX1, 1 - m_cropX2 );
        } else {
          RiCropWindow( m_cropX1, m_cropX2, m_cropY1, m_cropY2 );
        }
      };

      int k = 0;
      while ( k < m_numDisplayDrivers ) {
        MString parameterString;
        MString imageName;
        MString formatType;
        MString imageMode;
        // first check if additional display is enabled
        if ((k != 0) && !m_DDenabled[k]) {
          k++;
          continue;
        }
        // handle the output type, declaring new types in RIB as necessary
        if ( m_DDimageType[k] == "" ) {
          formatType = liqglo_currentJob.format;
        } else {
          formatType = parseString( m_DDimageType[k] );
        }
        if ( m_DDimageMode[k] == "" ) {
          imageMode = liqglo_currentJob.imageMode;
        } else {
          imageMode = parseString( m_DDimageMode[k] );
        }
        if ( m_DDparamType[k] != "" && m_DDparamType[k] != "rgbaz" && m_DDparamType[k] != "rgba" && m_DDparamType[k] != "rgb" && m_DDparamType[k] != "z" ) {
          MString pType = "varying ";
          pType += m_DDparamType[k];
          RiDeclare( (RtToken)imageMode.asChar(),(RtToken)pType.asChar() );
        }
        for ( int p = 0; p < m_DDParams[k].num; p++ ) {
          parameterString += "\""; parameterString += m_DDParams[k].names[p].asChar(); parameterString += "\" ";
          if ( m_DDParams[k].type[p] == 1 ) {
            //RiDeclare( (RtToken)m_DDParams[k].names[p].asChar(), "string" );
            parameterString += " [ string \""; parameterString += m_DDParams[k].data[p].asChar(); parameterString += "\"] ";
          } else if ( m_DDParams[k].type[p] == 2 ) {
            //RiDeclare( (RtToken)m_DDParams[k].names[p].asChar(), "float" );
            parameterString += " [ float "; parameterString += m_DDParams[k].data[p].asChar(); parameterString += "] ";
          }
        }
        // setup the output name
        imageName = m_pixDir;
        if ( liqglo_DDimageName[k] == "" ) {
          if ( k == 0 ) {
            imageName = liqglo_currentJob.imageName;
          } else {
            imageName = liqglo_sceneName + "_" + imageMode;
            // TODO - this little block of code below (up to sprintf) is almost identical
            // in a few places just within this file. should be rationalised into a
            // 'name generator' type of function
            MString outFileFmtString;
            outFileFmtString = m_pixDir;
            outFileFmtString += imageName;
            size_t outNameLength = outFileFmtString.length();
            if( m_animation || m_useFrameExt ) {
              outFileFmtString += LIQ_ANIM_EXT;
              outNameLength += m_outPadding + 1;
            }
            outFileFmtString += ".";
            outFileFmtString += outExt;
            outNameLength = outFileFmtString.length();
            // Hmmmmmm protect from buffer overflow ...
            outNameLength = outNameLength + 8; // Space for the null character
            char *outName = (char *)alloca(outNameLength);
            sprintf(outName, outFileFmtString.asChar(), 1, liqglo_lframe);
            imageName = outName;
          }
        } else {
          imageName += parseString( liqglo_DDimageName[k] );
        }
        if ( k > 0 ) {
          imageName = "+" + imageName;
        }
        // handle relative paths
        imageName = LIQ_GET_ABS_REL_FILE_NAME( liqglo_relativeFileNames, imageName, liqglo_projectDir );
        // output the display call
        RiArchiveRecord( RI_COMMENT, "Display Driver %d:", k );
        RiArchiveRecord( RI_VERBATIM, "Display \"%s\" \"%s\" \"%s\" %s\n", const_cast<char *>( imageName.asChar() ), formatType.asChar(), imageMode.asChar(), parameterString.asChar() );
        k++;
      }
    }

    LIQDEBUGPRINTF( "-> Setting Resolution\n" );

    if ( liqglo_currentJob.isShadow == false && liqglo_rotateCamera  == true ) {
      // philippe : Rotated Camera Case
      RiFormat( liqglo_currentJob.height, liqglo_currentJob.width, liqglo_currentJob.aspectRatio );
    } else {
      RiFormat( liqglo_currentJob.width, liqglo_currentJob.height, liqglo_currentJob.aspectRatio );
    }

    if ( liqglo_currentJob.camera[0].isOrtho ) {
      MDistance unitHelper;
      MDistance unit( 1, unitHelper.uiUnit() );
      RtFloat frameWidth, frameHeight;
      // the whole frame width has to be scaled according to the UI Unit
      frameWidth  = liqglo_currentJob.camera[0].orthoWidth  * 5 * unit.asMeters();
      frameHeight = liqglo_currentJob.camera[0].orthoHeight * 5 * unit.asMeters();
      RiProjection( "orthographic", RI_NULL );
      RiScreenWindow( -frameWidth, frameWidth, -frameHeight, frameHeight );
    } else {
      RtFloat fieldOfView = liqglo_currentJob.camera[0].hFOV * 180.0 / M_PI;
      if ( liqglo_currentJob.isShadow && liqglo_currentJob.isPoint ) {
        fieldOfView = liqglo_currentJob.camera[0].hFOV;
      }
      RiProjection( "perspective", RI_FOV, &fieldOfView, RI_NULL );
    }

    RiClipping( liqglo_currentJob.camera[0].neardb, liqglo_currentJob.camera[0].fardb );
    if ( doDof && !liqglo_currentJob.isShadow ) {
      RiDepthOfField( liqglo_currentJob.camera[0].fStop, liqglo_currentJob.camera[0].focalLength, liqglo_currentJob.camera[0].focalDistance );
    }
    // Set up for camera motion blur
    /* doCameraMotion = liqglo_currentJob.camera[0].motionBlur && liqglo_doMotion; */
    if ( doCameraMotion || liqglo_doMotion || liqglo_doDef ) {
      switch( shutterConfig ) {
        case OPEN_ON_FRAME:
        default:
          RiShutter( lframe, lframe + liqglo_currentJob.camera[0].shutter );
          break;
        case CENTER_ON_FRAME:
          RiShutter( ( lframe - ( liqglo_currentJob.camera[0].shutter * 0.5 ) ), ( lframe + ( liqglo_currentJob.camera[0].shutter * 0.5 ) ) );
          break;
        case CENTER_BETWEEN_FRAMES:
          RiShutter( lframe + ( 0.5 * ( 1 - liqglo_currentJob.camera[0].shutter ) ), lframe + liqglo_currentJob.camera[0].shutter + ( 0.5 * ( 1 - liqglo_currentJob.camera[0].shutter ) ) );
          break;
        case CLOSE_ON_NEXT_FRAME:
          RiShutter( lframe + ( 1 - liqglo_currentJob.camera[0].shutter ), lframe + 1 );
          break;
      }
    } else {
      RiShutter( lframe, lframe );
    }

    if ( liqglo_currentJob.gotJobOptions ) {
      RiArchiveRecord( RI_COMMENT, "jobOptions: \n%s", liqglo_currentJob.jobOptions.asChar() );
    }
    if ( ( liqglo_preRibBox.length() > 0 ) && !liqglo_currentJob.isShadow ) {
      unsigned ii;
      for ( ii = 0; ii < liqglo_preRibBox.length(); ii++ ) {
        RiArchiveRecord( RI_COMMENT, "Additional Rib:\n%s", liqglo_preRibBox[ii].asChar() );
      }
    }
    if ( ( liqglo_preReadArchive.length() > 0 ) && !liqglo_currentJob.isShadow ) {
      unsigned ii;
      for ( ii = 0; ii < liqglo_preReadArchive.length(); ii++ ) {
        RiArchiveRecord( RI_COMMENT, "Read Archive Data: \nReadArchive \"%s\"", liqglo_preReadArchive[ii].asChar() );
      }
    }
    if ( ( liqglo_preRibBoxShadow.length() > 0 ) && !liqglo_currentJob.isShadow ) {
      unsigned ii;
      for ( ii = 0; ii < liqglo_preRibBoxShadow.length(); ii++ ) {
        RiArchiveRecord( RI_COMMENT, "Additional Rib:\n%s", liqglo_preRibBoxShadow[ii].asChar() );
      }
    }
    if ( ( liqglo_preReadArchiveShadow.length() > 0 ) && liqglo_currentJob.isShadow ) {
      unsigned ii;
      for ( ii = 0; ii < liqglo_preReadArchiveShadow.length(); ii++ ) {
        RiArchiveRecord( RI_COMMENT, "Read Archive Data: \nReadArchive \"%s\"", liqglo_preReadArchiveShadow[ii].asChar() );
      }
    }

    if ( doCameraMotion && ( !liqglo_currentJob.isShadow || liqglo_currentJob.deepShadows) ) {
      /*RiMotionBegin( 2, ( lframe - ( liqglo_currentJob.camera[0].shutter * m_blurTime * 0.5  )), ( lframe + ( liqglo_currentJob.camera[0].shutter * m_blurTime * 0.5  )) );*/
      // Moritz: replaced RiMotionBegin call with ..V version to allow for more than five motion samples
      RiMotionBeginV( liqglo_motionSamples, liqglo_sampleTimes );
    }
    RtMatrix cameraMatrix;
    liqglo_currentJob.camera[0].mat.get( cameraMatrix );
    RiTransform( cameraMatrix );
    if ( doCameraMotion && ( !liqglo_currentJob.isShadow || liqglo_currentJob.deepShadows ) ) {
      int mm = 1;
      while ( mm < liqglo_motionSamples ) {
        liqglo_currentJob.camera[mm].mat.get( cameraMatrix );
        RiTransform( cameraMatrix );
        ++mm;
      }
      RiMotionEnd();
    }
  }
  return MS::kSuccess;
}

/**
 * Write out the frame epilogue.
 * This basically calls RiFrameEnd().
 */
MStatus liqRibTranslator::frameEpilogue( long )
{
  if (ribStatus == kRibFrame) {
    ribStatus = kRibBegin;
    if ( !m_exportReadArchive ) {
      RiFrameEnd();
    }
  }
  return (ribStatus == kRibBegin ? MS::kSuccess : MS::kFailure);
}

/**
 * Write out the body of the frame.
 * This is a dump of the DAG to RIB with flattened transforms (MtoR-style).
 */
MStatus liqRibTranslator::objectBlock()
{
  MStatus returnStatus = MS::kSuccess;
  MStatus status;
  attributeDepth = 0;

  if ( m_ignoreSurfaces ) {
    RiSurface( "matte", RI_NULL );
  }

  // Moritz: Added Pre-Geometry RIB for insertion right before any primitives
  if ( m_preGeomRIB != "" ) {
    RiArchiveRecord( RI_COMMENT,  " Pre-Geometry RIB from liquid globals");
    RiArchiveRecord( RI_VERBATIM, ( char* ) m_preGeomRIB.asChar() );
    RiArchiveRecord( RI_VERBATIM, "\n");
  }

  MMatrix matrix;
  MDagPath path;
  MFnDagNode dagFn;

  for ( RNMAP::iterator rniter = htable->RibNodeMap.begin(); rniter != htable->RibNodeMap.end(); rniter++ ) {
    LIQ_CHECK_CANCEL_REQUEST;

    liqRibNode * ribNode = (*rniter).second;
    path = ribNode->path();

    if ( ( NULL == ribNode ) || ( ribNode->object(0)->type == MRT_Light ) ) continue;
    if ( ribNode->object(0)->type == MRT_Coord ) continue;
    if ( ( !liqglo_currentJob.isShadow ) && ( ribNode->object(0)->ignore ) ) continue;
    if ( ( liqglo_currentJob.isShadow ) && ( ribNode->object(0)->ignoreShadow ) ) continue;

    if ( m_outputComments ) RiArchiveRecord( RI_COMMENT, "Name: %s", ribNode->name.asChar(), RI_NULL );

    RiAttributeBegin();
    RtString name = const_cast< char* >( ribNode->name.asChar() );
    RiAttribute( "identifier", "name", &name, RI_NULL );

    if( ribNode->grouping.membership != "" ) {
      RtString members = const_cast< char* >( ribNode->grouping.membership.asChar() );
      RiAttribute( "grouping", "membership", &members, RI_NULL );
    }

    // If this is a matte object, then turn that on if it isn't currently set
    if ( ribNode->shading.matte == -1){
      // Respect the maya shading node setting
    if ( ribNode->matteMode ) {
      if ( !m_currentMatteMode ) RiMatte( RI_TRUE );
      m_currentMatteMode = true;
    } else {
      if ( m_currentMatteMode ) RiMatte( RI_FALSE );
      m_currentMatteMode = false;
    }
    } else {
      // The dag had a liqMatte property, use that (overriding the maya shader)
      if ( ribNode->shading.matte > 0 ) {	// Slightly nasty test to verify we've actually got a value here
        if ( !m_currentMatteMode ) RiMatte( RI_TRUE );
        m_currentMatteMode = true;
      } else {
        if ( m_currentMatteMode ) RiMatte( RI_FALSE );
        m_currentMatteMode = false;
      }
    }
    // If this is a single sided object, then turn that on (RMan default is Sides 2)
    if ( !ribNode->doubleSided ) {
      RiSides( 1 );
    }

    LIQDEBUGPRINTF( "-> object name: " );

    LIQDEBUGPRINTF( ribNode->name.asChar() );

    LIQDEBUGPRINTF( "\n" );

    MObject object;

    // Moritz: only write out light linking if we're not in a shadow pass
    if ( !liqglo_currentJob.isShadow || liqglo_currentJob.deepShadows && m_outputLightsInDeepShadows && !m_ignoreLights ) {
      MObjectArray ignoredLights;
    //ribNode->getIgnoredLights( ribNode->assignedShadingGroup.object(), ignoredLights );
      ribNode->getIgnoredLights( ignoredLights );

      for ( unsigned i=0; i<ignoredLights.length(); i++ )
      {
        MFnDagNode lightFnDag( ignoredLights[i] );
        MString nodeName = lightFnDag.fullPathName();

        if ( NULL != htable ) {
          //RibNode * ln = htable->find( light, MRT_Light );
          MDagPath nodeDagPath;
          lightFnDag.getPath( nodeDagPath );
          liqRibNode * ln = htable->find( lightFnDag.fullPathName(), nodeDagPath, MRT_Light );
          if ( NULL != ln ) {
            RiIlluminate( ln->object(0)->lightHandle(), RI_FALSE );
          }
        }
      }
    }

    if ( liqglo_doMotion && ribNode->motion.transformationBlur && ( ribNode->object( 1 ) != NULL ) && ( ribNode->object(0)->type != MRT_Locator ) &&
         ( !liqglo_currentJob.isShadow || liqglo_currentJob.deepShadows ) )
    {
      LIQDEBUGPRINTF( "-> writing matrix motion blur data\n" );
      // Moritz: replaced RiMotionBegin call with ..V version to allow for more than five motion samples
      RiMotionBeginV( liqglo_motionSamples, liqglo_sampleTimes );
    }
    RtMatrix ribMatrix;
    matrix = ribNode->object( 0 )->matrix( path.instanceNumber() );
    matrix.get( ribMatrix );
    RiTransform( ribMatrix );

    // Output the world matrices for the motionblur
    // This will override the current transformation setting
    if ( liqglo_doMotion && ribNode->motion.transformationBlur && ( ribNode->object( 1 ) != NULL ) && ( ribNode->object( 0 )->type != MRT_Locator ) &&
         ( !liqglo_currentJob.isShadow || liqglo_currentJob.deepShadows ) )
    {
      path = ribNode->path();
      unsigned mm = 1;
      RtMatrix ribMatrix;
      while ( mm < liqglo_motionSamples ) {
        matrix = ribNode->object( mm )->matrix( path.instanceNumber() );
        matrix.get( ribMatrix );
        RiTransform( ribMatrix );
        ++mm;
      }
      RiMotionEnd();
    }

    MFnDagNode fnDagNode( path );
    bool hasSurfaceShader = false;
    bool hasDisplacementShader = false;
    bool hasVolumeShader = false;

    MPlug rmanShaderPlug;
    // Check for surface shader
    status.clear();
    rmanShaderPlug = fnDagNode.findPlug( MString( "liquidSurfaceShaderNode" ), &status );
    if ( ( status != MS::kSuccess ) || ( !rmanShaderPlug.isConnected() ) ) { status.clear(); rmanShaderPlug = ribNode->assignedShadingGroup.findPlug( MString( "liquidSurfaceShaderNode" ), &status ); }
    if ( ( status != MS::kSuccess ) || ( !rmanShaderPlug.isConnected() ) ) { status.clear(); rmanShaderPlug = ribNode->assignedShader.findPlug( MString( "liquidSurfaceShaderNode" ), &status ); }
    if ( status == MS::kSuccess && !rmanShaderPlug.isNull() ) {
      if ( rmanShaderPlug.isConnected() ) {
        MPlugArray rmShaderNodeArray;
        rmanShaderPlug.connectedTo( rmShaderNodeArray, true, true );
        MObject rmShaderNodeObj;
        rmShaderNodeObj = rmShaderNodeArray[0].node();
        ribNode->assignedShader.setObject( rmShaderNodeObj );
        hasSurfaceShader = true;
      }
    }
    // Check for displacement shader
    status.clear();
    rmanShaderPlug = fnDagNode.findPlug( MString( "liquidDispShaderNode" ), &status );
    if ( ( status != MS::kSuccess ) || ( !rmanShaderPlug.isConnected() ) ) { status.clear(); rmanShaderPlug = ribNode->assignedShadingGroup.findPlug( MString( "liquidDispShaderNode" ), &status ); }
    if ( ( status != MS::kSuccess ) || ( !rmanShaderPlug.isConnected() ) ) { status.clear(); rmanShaderPlug = ribNode->assignedDisp.findPlug( MString( "liquidDispShaderNode" ), &status ); }
    if ( ( status == MS::kSuccess ) && !rmanShaderPlug.isNull() && !m_ignoreDisplacements ) {
      if ( rmanShaderPlug.isConnected() ) {
        MPlugArray rmShaderNodeArray;
        rmanShaderPlug.connectedTo( rmShaderNodeArray, true, true );
        MObject rmShaderNodeObj;
        rmShaderNodeObj = rmShaderNodeArray[0].node();
        ribNode->assignedDisp.setObject( rmShaderNodeObj );
        hasDisplacementShader = true;
      }
    }
    // Check for volume shader
    status.clear();
    rmanShaderPlug = fnDagNode.findPlug( MString( "liquidVolumeShaderNode" ), &status );
    if ( ( status != MS::kSuccess ) || ( !rmanShaderPlug.isConnected() ) ) { status.clear(); rmanShaderPlug = ribNode->assignedShadingGroup.findPlug( MString( "liquidVolumeShaderNode" ), &status ); }
    if ( ( status != MS::kSuccess ) || ( !rmanShaderPlug.isConnected() ) ) { status.clear(); rmanShaderPlug = ribNode->assignedVolume.findPlug( MString( "liquidVolumeShaderNode" ), &status ); }
    if ( ( status == MS::kSuccess ) && !rmanShaderPlug.isNull() && !m_ignoreVolumes ) {
      if ( rmanShaderPlug.isConnected() ) {
        MPlugArray rmShaderNodeArray;
        rmanShaderPlug.connectedTo( rmShaderNodeArray, true, true );
        MObject rmShaderNodeObj;
        rmShaderNodeObj = rmShaderNodeArray[0].node();
        ribNode->assignedVolume.setObject( rmShaderNodeObj );
        hasVolumeShader = true;
      }
    }

    float surfaceDisplacementBounds = 0.0;
    status.clear();
    if ( !ribNode->assignedShader.object().isNull() ) {
      MPlug sDBPlug = ribNode->assignedShader.findPlug( MString( "displacementBound" ), &status );
      if ( status == MS::kSuccess ) sDBPlug.getValue( surfaceDisplacementBounds );
    }
    float dispDisplacementBounds = 0.0;
    status.clear();
    if ( !ribNode->assignedDisp.object().isNull() ) {
      MPlug dDBPlug = ribNode->assignedDisp.findPlug( MString( "displacementBound" ), &status );
      if ( status == MS::kSuccess ) dDBPlug.getValue( dispDisplacementBounds );
    }
    if ( ( dispDisplacementBounds != 0.0 ) && ( dispDisplacementBounds > surfaceDisplacementBounds ) ) {
      RtString coordsys = "shader";
      RiAttribute( "displacementbound", (RtToken) "sphere", &dispDisplacementBounds, "coordinatesystem", &coordsys, RI_NULL );
    } else if ( ( surfaceDisplacementBounds != 0.0 ) ) {
      RtString coordsys = "shader";
      RiAttribute( "displacementbound", (RtToken) "sphere", &surfaceDisplacementBounds, "coordinatesystem", &coordsys, RI_NULL );
    }

    bool writeShaders = true;

    if ( liqglo_currentJob.isShadow && ( !m_outputShadersInShadows ||
         ( liqglo_currentJob.deepShadows && !m_outputShadersInDeepShadows ) ) )
      writeShaders = false;

    if ( writeShaders ) {
      if ( hasVolumeShader && !m_ignoreVolumes ) {

        liqShader & currentShader = liqGetShader( ribNode->assignedVolume.object());

        if( !currentShader.hasErrors ) {
          RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * currentShader.numTPV );
          RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * currentShader.numTPV );
          assignTokenArrays( currentShader.numTPV, currentShader.tokenPointerArray, tokenArray, pointerArray );
          char *shaderFileName;
          LIQ_GET_SHADER_FILE_NAME( shaderFileName, liqglo_shortShaderNames, currentShader );
          RiAtmosphereV ( shaderFileName, currentShader.numTPV, tokenArray, pointerArray );
        }
      }

      // RtFloat currentNodeShadingRate = shadingRate;

      LIQDEBUGPRINTF( "-> writing node attributes" );
      //MGlobal::displayInfo( "writing node attributes" );

      if( ribNode->shading.shadingRate != shadingRate )
        RiShadingRate ( ribNode->shading.shadingRate );

      if( !ribNode->shading.diceRasterOrient ) {
        RtInt off = 0;
        RiAttribute( "dice", (RtToken) "rasterorient", &off, RI_NULL );
      }

      if( ribNode->trace.sampleMotion ) {
        RtInt on = 1;
        RiAttribute( "trace", (RtToken) "samplemotion", &on, RI_NULL );
      }

      if( ribNode->trace.displacements ) {
        RtInt on = 1;
        RiAttribute( "trace", (RtToken) "displacements", &on, RI_NULL );
      }

      if( ribNode->trace.bias != 0.01f ) {
        RtFloat bias = ribNode->trace.bias;
        RiAttribute( "trace", (RtToken) "bias", &bias, RI_NULL );
      }

      if( ribNode->trace.maxDiffuseDepth != 1 ) {
        RtInt mddepth = ribNode->trace.maxDiffuseDepth;
        RiAttribute( "trace", (RtToken) "maxdiffusedepth", &mddepth, RI_NULL );
      }

      if( ribNode->trace.maxSpecularDepth != 2 ) {
        RtInt msdepth = ribNode->trace.maxSpecularDepth;
        RiAttribute( "trace", (RtToken) "maxspeculardepth", &msdepth, RI_NULL );
      }

      if( !ribNode->visibility.camera ) {
        RtInt off = 0;
        RiAttribute( "visibility", (RtToken) "camera", &off, RI_NULL );
      }

      // old-style raytracing visibility support
      // philippe: if raytracing is off in the globals, trace visibility is turned off for all objects, transmission is set to TRANSPARENT for all objects

      if( rt_useRayTracing && ribNode->visibility.trace ) {
        RtInt on = 1;
        RiAttribute( "visibility", (RtToken) "trace", &on, RI_NULL );
      }

      if( rt_useRayTracing && ribNode->visibility.transmission != liqRibNode::visibility::TRANSMISSION_TRANSPARENT ) {
        RtString trans;
        switch( ribNode->visibility.transmission ) {
          case liqRibNode::visibility::TRANSMISSION_OPAQUE:
            trans = "opaque";
            break;
          case liqRibNode::visibility::TRANSMISSION_OS:
            trans = "Os";
            break;
          case liqRibNode::visibility::TRANSMISSION_SHADER:
          default:
            trans = "shader";
        }
        RiAttribute( "visibility", (RtToken) "string transmission", &trans, RI_NULL );
      }

      // philippe : prman 12.5 visibility support

      if( rt_useRayTracing && ribNode->visibility.diffuse ) {
        RtInt on = 1;
        RiAttribute( "visibility", (RtToken) "int diffuse", &on, RI_NULL );
      }

      if( rt_useRayTracing && ribNode->visibility.specular ) {
        RtInt on = 1;
        RiAttribute( "visibility", (RtToken) "int specular", &on, RI_NULL );
      }

      if( rt_useRayTracing && ribNode->visibility.newtransmission ) {
        RtInt on = 1;
        RiAttribute( "visibility", (RtToken) "int transmission", &on, RI_NULL );
      }

      if( rt_useRayTracing && ribNode->visibility.camera ) {
        RtInt on = 1;
        RiAttribute( "visibility", (RtToken) "int camera", &on, RI_NULL );
      }

      if( rt_useRayTracing && ribNode->hitmode.diffuse != liqRibNode::hitmode::DIFFUSE_HITMODE_PRIMITIVE ) {
        RtString mode;
        switch( ribNode->hitmode.diffuse ) {
          case liqRibNode::hitmode::DIFFUSE_HITMODE_SHADER:
            mode = "shader";
            break;
          case liqRibNode::hitmode::DIFFUSE_HITMODE_PRIMITIVE:
          default:
            mode = "primitive";
        }
        RiAttribute( "shade", (RtToken) "string diffusehitmode", &mode, RI_NULL );
      }

      if( rt_useRayTracing && ribNode->hitmode.specular != liqRibNode::hitmode::SPECULAR_HITMODE_SHADER ) {
        RtString mode;
        switch( ribNode->hitmode.specular ) {
          case liqRibNode::hitmode::SPECULAR_HITMODE_PRIMITIVE:
            mode = "primitive";
            break;
          case liqRibNode::hitmode::SPECULAR_HITMODE_SHADER:
          default:
            mode = "shader";
        }
        RiAttribute( "shade", (RtToken) "string specularhitmode", &mode, RI_NULL );
      }

      if( rt_useRayTracing && ribNode->hitmode.transmission != liqRibNode::hitmode::TRANSMISSION_HITMODE_SHADER ) {
        RtString mode;
        switch( ribNode->hitmode.transmission ) {
          case liqRibNode::hitmode::TRANSMISSION_HITMODE_PRIMITIVE:
            mode = "primitive";
            break;
          case liqRibNode::hitmode::TRANSMISSION_HITMODE_SHADER:
          default:
            mode = "shader";
        }
        RiAttribute( "shade", (RtToken) "string transmissionhitmode", &mode, RI_NULL );
      }

      if( ribNode->hitmode.camera != liqRibNode::hitmode::CAMERA_HITMODE_SHADER ) {
        RtString mode;
        switch( ribNode->hitmode.camera ) {
          case liqRibNode::hitmode::CAMERA_HITMODE_PRIMITIVE:
            mode = "primitive";
            break;
          case liqRibNode::hitmode::CAMERA_HITMODE_SHADER:
          default:
            mode = "shader";
        }
        RiAttribute( "shade", (RtToken) "string camerahitmode", &mode, RI_NULL );
      }


      if( ribNode->irradiance.shadingRate != 1.0f ) {
        RtFloat rate = ribNode->irradiance.shadingRate;
        RiAttribute( "irradiance", (RtToken) "shadingrate", &rate, RI_NULL );
      }

      if( ribNode->irradiance.nSamples != 64 ) {
        RtInt samples = ribNode->irradiance.nSamples;
        RiAttribute( "irradiance", (RtToken) "nsamples", &samples, RI_NULL );
      }

      if( ribNode->irradiance.maxError != 0.5f ) {
        RtFloat merror = ribNode->irradiance.maxError;
        RiAttribute( "irradiance", (RtToken) "float maxerror", &merror, RI_NULL );
      }

      if( ribNode->irradiance.maxPixelDist != 30.0f ) {
        RtFloat mpd = ribNode->irradiance.maxPixelDist;
        RiAttribute( "irradiance", (RtToken) "float maxpixeldist", &mpd, RI_NULL );
      }

      if( ribNode->irradiance.handle != "" ) {
        RtString handle = const_cast< char* >( ribNode->irradiance.handle.asChar() );
        RiAttribute( "irradiance", (RtToken) "handle", &handle, RI_NULL );
      }

      if( ribNode->irradiance.fileMode != liqRibNode::irradiance::FILEMODE_NONE ) {
        RtString mode;
        switch( ribNode->irradiance.fileMode ) {
          case liqRibNode::irradiance::FILEMODE_READ:
            mode = "r";
            break;
          case liqRibNode::irradiance::FILEMODE_WRITE:
            mode = "w";
            break;
          case liqRibNode::irradiance::FILEMODE_READ_WRITE:
          default:
            mode = "rw";
        }
        RiAttribute( "irradiance", (RtToken) "filemode", &mode, RI_NULL );
      }

      if( ribNode->photon.globalMap != "" ) {
        RtString map = const_cast< char* >( ribNode->photon.globalMap.asChar() );
        RiAttribute( "photon", (RtToken) "globalmap", &map, RI_NULL );
      }

      if( ribNode->photon.causticMap != "" ) {
        RtString map = const_cast< char* >( ribNode->photon.causticMap.asChar() );
        RiAttribute( "photon", (RtToken) "causticmap", &map, RI_NULL );
      }

      if( ribNode->photon.shadingModel != liqRibNode::photon::SHADINGMODEL_MATTE ) {
        RtString model;
        switch( ribNode->photon.shadingModel  ) {
          case liqRibNode::photon::SHADINGMODEL_GLASS:
            model = "glass";
            break;
          case liqRibNode::photon::SHADINGMODEL_WATER:
            model = "water";
            break;
          case liqRibNode::photon::SHADINGMODEL_CHROME:
            model = "chrome";
            break;
          case liqRibNode::photon::SHADINGMODEL_TRANSPARENT:
            model = "chrome";
            break;
          case liqRibNode::photon::SHADINGMODEL_MATTE:
          default:
            model = "matte";
        }
        RiAttribute( "photon", (RtToken) "shadingmodel", &model, RI_NULL );
      }

      if( ribNode->photon.estimator != 100 ) {
        RtInt estimator = ribNode->photon.estimator;
        RiAttribute( "photon", (RtToken) "estimator", &estimator, RI_NULL );
      }

      if( ribNode->motion.deformationBlur || ribNode->motion.transformationBlur &&
          ribNode->motion.factor != 2.0f ) {
        RiGeometricApproximation( "motionfactor", ribNode->motion.factor );
      }

      if ( hasSurfaceShader && !m_ignoreSurfaces ) {

        liqShader & currentShader = liqGetShader( ribNode->assignedShader.object());

        RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * currentShader.numTPV );
        RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * currentShader.numTPV );

        assignTokenArrays( currentShader.numTPV, currentShader.tokenPointerArray, tokenArray, pointerArray );

        // Output color overrides or color

        if (ribNode->shading.color.r != -1.0) {
          RtColor rColor;
          rColor[0] = ribNode->shading.color[0];
          rColor[1] = ribNode->shading.color[1];
          rColor[2] = ribNode->shading.color[2];
          RiColor( rColor );
        } else {
          RiColor( currentShader.rmColor );
        }

        if (ribNode->shading.opacity.r != -1.0) {
          RtColor rOpacity;
          rOpacity[0] = ribNode->shading.opacity[0];
          rOpacity[1] = ribNode->shading.opacity[1];
          rOpacity[2] = ribNode->shading.opacity[2];
          RiOpacity( rOpacity );
      } else {
        RiOpacity( currentShader.rmOpacity );
      }
        char *shaderFileName;
        LIQ_GET_SHADER_FILE_NAME(shaderFileName, liqglo_shortShaderNames, currentShader );
        RiSurfaceV ( shaderFileName, currentShader.numTPV, tokenArray, pointerArray );
      } else {
        RtColor rColor,rOpacity;
        if ( m_shaderDebug ) {
          rColor[0] = 1;
          rColor[1] = 0;
          rColor[2] = 0;
          RiColor( rColor );
          rOpacity[0] = 1;
          rOpacity[1] = 1;
          rOpacity[1] = 1;
          RiOpacity( rOpacity );
        } else {
          if (ribNode->shading.color.r != -1.0) {
            rColor[0] = ribNode->shading.color[0];
            rColor[1] = ribNode->shading.color[1];
            rColor[2] = ribNode->shading.color[2];
            RiColor( rColor );
        } else if ( ( ribNode->color.r != -1.0 ) ) {
          rColor[0] = ribNode->color[0];
          rColor[1] = ribNode->color[1];
          rColor[2] = ribNode->color[2];
          RiColor( rColor );
        }

      if (ribNode->shading.opacity.r != -1.0) {
        rOpacity[0] = ribNode->shading.opacity[0];
        rOpacity[1] = ribNode->shading.opacity[1];
        rOpacity[2] = ribNode->shading.opacity[2];
        RiOpacity( rOpacity );
      } else if ( ( ribNode->opacity.r != -1.0 ) ) {
        rOpacity[0] = ribNode->opacity[0];
        rOpacity[1] = ribNode->opacity[1];
        rOpacity[2] = ribNode->opacity[2];
        RiOpacity( rOpacity );
      }
        }

        if ( !m_ignoreSurfaces ) {
          MObject shadingGroup = ribNode->assignedShadingGroup.object();
          MObject shader = ribNode->findShader( shadingGroup );
          if ( m_shaderDebug ) {
            RiSurface( "constant", RI_NULL );
          } else if ( shader.apiType() == MFn::kLambert ) {
            RiSurface( "matte", RI_NULL );
          } else if ( shader.apiType() == MFn::kPhong ) {
            RiSurface( "plastic", RI_NULL );
          } else {
            RiSurface( "plastic", RI_NULL );
          }
        }
      }
    }

    if ( hasDisplacementShader && !m_ignoreDisplacements ) {

      liqShader & currentShader = liqGetShader( ribNode->assignedDisp.object() );

      if ( !currentShader.hasErrors ) {
        RtToken *tokenArray = ( RtToken *)alloca( sizeof( RtToken ) * currentShader.numTPV );
        RtPointer *pointerArray = ( RtPointer *)alloca( sizeof( RtPointer ) * currentShader.numTPV );

        assignTokenArrays( currentShader.numTPV, currentShader.tokenPointerArray, tokenArray, pointerArray );

        char *shaderFileName;
        LIQ_GET_SHADER_FILE_NAME(shaderFileName, liqglo_shortShaderNames, currentShader );
        RiDisplacementV ( shaderFileName, currentShader.numTPV, tokenArray, pointerArray );
      }
    }

    if ( ribNode->rib.box != "" && ribNode->rib.box != "-" ) {
      RiArchiveRecord( RI_COMMENT, " RIB Box:\n%s", ribNode->rib.box.asChar() );
    }
    if ( ribNode->rib.readArchive != "" && ribNode->rib.readArchive != "-" ) {
      RiArchiveRecord( RI_COMMENT, " Read Archive Data: \nReadArchive \"%s\"", ribNode->rib.readArchive.asChar() );
    }
    if ( ribNode->rib.delayedReadArchive != "" && ribNode->rib.delayedReadArchive != "-" ) {
      RiArchiveRecord( RI_COMMENT, " Delayed Read Archive Data: \nProcedural \"DelayedReadArchive\" [ \"%s\" ] [ %f %f %f %f %f %f ]", ribNode->rib.delayedReadArchive.asChar(), ribNode->bound[0],ribNode->bound[3],ribNode->bound[1],ribNode->bound[4],ribNode->bound[2],ribNode->bound[5] );
    }

    if ( !ribNode->ignoreShapes ) {

      // check to see if we are writing a curve to set the proper basis
      if ( ribNode->object(0)->type == MRT_NuCurve ) {
        RiBasis( RiBSplineBasis, 1, RiBSplineBasis, 1 );
      }

      if( liqglo_doDef && ribNode->motion.deformationBlur && ( ribNode->object(1) != NULL ) && ( ribNode->object(0)->type != MRT_RibGen )
          && ( ribNode->object(0)->type != MRT_Locator ) && ( !liqglo_currentJob.isShadow || liqglo_currentJob.deepShadows ) ) {
        // Moritz: replaced RiMotionBegin call with ..V version to allow for more than five motion samples
        RiMotionBeginV( liqglo_motionSamples, liqglo_sampleTimes );
      }

      ribNode->object(0)->writeObject();
      if ( liqglo_doDef && ribNode->motion.deformationBlur && ( ribNode->object(1) != NULL ) && ( ribNode->object(0)->type != MRT_RibGen )
           && ( ribNode->object(0)->type != MRT_Locator ) && ( !liqglo_currentJob.isShadow || liqglo_currentJob.deepShadows ) ) {
        LIQDEBUGPRINTF( "-> writing deformation blur data\n" );
        int msampleOn = 1;
        while ( msampleOn < liqglo_motionSamples ) {
          ribNode->object(msampleOn)->writeObject();
          ++msampleOn;
        }
        RiMotionEnd();
      }

    } else RiArchiveRecord( RI_COMMENT, " Shapes Ignored !!" );

    RiAttributeEnd();
  }

  while ( attributeDepth > 0 ) {
    RiAttributeEnd();
    attributeDepth--;
  }

  return returnStatus;
}

/**
 * Write the world prologue.
 * This includes the pre- and post-world begin RIB boxes and the definition of
 * any default coordinate systems.
 */
MStatus liqRibTranslator::worldPrologue()
{
  MStatus returnStatus = MS::kSuccess;

  LIQDEBUGPRINTF( "-> Writing world prologue.\n" );

  // if this is a readArchive that we are exporting then ingore this header information
  if ( !m_exportReadArchive ) {

    // put in pre-worldbegin statements
    if (m_preWorldRIB != "") {
      RiArchiveRecord(RI_COMMENT,  " Pre-WorldBegin RIB from liquid globals");
      RiArchiveRecord(RI_VERBATIM, (char*) m_preWorldRIB.asChar());
      RiArchiveRecord(RI_VERBATIM, "\n");
    }

    RiWorldBegin();

    // put in post-worldbegin statements
    if (m_postWorldRIB != "") {
      RiArchiveRecord(RI_COMMENT,  " Post-WorldBegin RIB from liquid globals");
      RiArchiveRecord(RI_VERBATIM, (char*) m_postWorldRIB.asChar());
      RiArchiveRecord(RI_VERBATIM, "\n");
    }

    RiTransformBegin();
    RiCoordinateSystem( "worldspace" );
    RiTransformEnd();
  }

  return returnStatus;
}

/**
 * Write the world prologue.
 * This basically calls RiWorldEnd().
 */
MStatus liqRibTranslator::worldEpilogue()
{
  MStatus returnStatus = MS::kSuccess;

  LIQDEBUGPRINTF( "-> Writing world epilogue.\n" );

  // If this is a readArchive that we are exporting there's no world block
  if ( !m_exportReadArchive ) {
    RiWorldEnd();
  }

  return returnStatus;
}

/**
 * Write all coordinate systems.
 * This writes all user-defined coordinate systems as well as those required
 * for environment/reflection map lookup and texture projection.
 */
MStatus liqRibTranslator::coordSysBlock()
{
  MStatus returnStatus = MS::kSuccess;

  LIQDEBUGPRINTF( "-> Writing coordinate systems.\n" );

  RNMAP::iterator rniter;

  for ( rniter = htable->RibNodeMap.begin(); rniter != htable->RibNodeMap.end(); rniter++ ) {

    LIQ_CHECK_CANCEL_REQUEST;

    liqRibNode * ribNode = (*rniter).second;

    if ( ribNode->object(0)->ignore || ribNode->object(0)->type != MRT_Coord ) continue;

    if ( m_outputComments )
      RiArchiveRecord( RI_COMMENT, "Name: %s", ribNode->name.asChar(), RI_NULL );

    RiAttributeBegin();
    RtString name = const_cast< char* >( ribNode->name.asChar() );
    RiAttribute( "identifier", "name", &name, RI_NULL );

    RtMatrix ribMatrix;
    MMatrix matrix;
    MDagPath path;

    matrix = ribNode->object(0)->matrix( path.instanceNumber() );
    matrix.get( ribMatrix );
    RiTransform( ribMatrix );

    ribNode->object(0)->writeObject();
    ribNode->object(0)->written = 1;

    RiAttributeEnd();
  }

  return returnStatus;
}

/**
 * Write all lights.
 * This writes all lightsoure shaders with their attributes and switches them
 * on afterwards.
 */
MStatus liqRibTranslator::lightBlock()
{
  MStatus returnStatus = MS::kSuccess;

  LIQDEBUGPRINTF( "-> Writing lights.\n" );

  // If this is a readArchive that we are exporting then ingore this header information
  if ( !m_exportReadArchive ) {
    RNMAP::iterator rniter;

    int nbLight = 0;

    for ( rniter = htable->RibNodeMap.begin(); rniter != htable->RibNodeMap.end(); rniter++ ) {

      LIQ_CHECK_CANCEL_REQUEST;
      liqRibNode * ribNode = (*rniter).second;

      if ( ribNode->object(0)->ignore || ribNode->object(0)->type != MRT_Light ) continue;

      // We need to enclose lights in attribute blocks because of the
      // new added attribute support
      RiAttributeBegin();

      // All this stuff below should be handled by a new attribute class
      RtString name = const_cast< char* >( ribNode->name.asChar() );
      RiAttribute( "identifier", "name", &name, RI_NULL );

      if( ribNode->trace.sampleMotion ) {
        RtInt on = 1;
        RiAttribute( "trace", (RtToken) "samplemotion", &on, RI_NULL );
      }

      if( ribNode->trace.displacements ) {
        RtInt on = 1;
        RiAttribute( "trace", (RtToken) "displacements", &on, RI_NULL );
      }

      if( ribNode->trace.bias != 0.01f ) {
        RtFloat bias = ribNode->trace.bias;
        RiAttribute( "trace", (RtToken) "bias", &bias, RI_NULL );
      }

      if( ribNode->trace.maxDiffuseDepth != 1 ) {
        RtInt mddepth = ribNode->trace.maxDiffuseDepth;
        RiAttribute( "trace", (RtToken) "maxdiffusedepth", &mddepth, RI_NULL );
      }

      if( ribNode->trace.maxSpecularDepth != 2 ) {
        RtInt msdepth = ribNode->trace.maxSpecularDepth;
        RiAttribute( "trace", (RtToken) "maxspeculardepth", &msdepth, RI_NULL );
      }

      ribNode->object(0)->writeObject();
      ribNode->object(0)->written = 1;

      // The next line pops the light...
      RiAttributeEnd();
      // ...so we have to switch it on again explicitly
      RiIlluminate( ribNode->object(0)->lightHandle(), 1 );

      nbLight++;
    }
  }

  return returnStatus;
}

void liqRibTranslator::setSearchPaths()
{
  liqglo_shaderPath = "&:@:.:~:rmanshader";
  liqglo_texturePath = "&:@:.:~:rmantex";
  liqglo_archivePath = "&:@:.:~:rib";
  liqglo_proceduralPath = "&:@:.:~";

  MString tmphome( getenv( "LIQUIDHOME" ) );

  if( tmphome != "" ) {
    liqglo_shaderPath += ":" + liquidSanitizePath( tmphome ) + "/shaders";
    liqglo_texturePath += ":" + liquidSanitizePath( tmphome ) + "/rmantex";
    liqglo_archivePath += ":" + liquidSanitizePath( tmphome ) + "/rib";
  }

  MStatus gStatus;
  MPlug gPlug;
  MFnDependencyNode rGlobalNode( rGlobalObj );

  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "shaderPath", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      tmphome = liquidSanitizePath( tmphome ) + "/shaders";
      if ( varVal.index( ':' ) == 0 )
        liqglo_shaderPath += ":" + removeEscapes( parseString( varVal ) );
      else if ( varVal.rindex( ':' ) == 0 )
        liqglo_shaderPath = removeEscapes( parseString( varVal ) ) + liqglo_shaderPath;
      else
        liqglo_shaderPath = removeEscapes( parseString( varVal ) ) + ":" + tmphome;
    }
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "texturePath", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      if ( varVal.index( ':' ) == 0 )
        liqglo_texturePath += ":" + removeEscapes( parseString( varVal ) );
      else if ( varVal.rindex( ':' ) == 0 )
        liqglo_texturePath = removeEscapes( parseString( varVal ) ) + liqglo_texturePath;
      else
        liqglo_texturePath = removeEscapes( parseString( varVal ) );
    }
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "archivePath", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      if ( varVal.index( ':' ) == 0 )
        liqglo_archivePath += ":" + removeEscapes( parseString( varVal ) );
      else if ( varVal.rindex( ':' ) == 0 )
        liqglo_archivePath = removeEscapes( parseString( varVal ) ) + liqglo_archivePath;
      else
        liqglo_archivePath = removeEscapes( parseString( varVal ) );
    }
  }
  {
    MString varVal;
    gPlug = rGlobalNode.findPlug( "proceduralPath", &gStatus );
    if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
    gStatus.clear();
    if ( varVal != "" ) {
      if ( varVal.index( ':' ) == 0 )
        liqglo_proceduralPath += ":" + removeEscapes( parseString( varVal ) );
      else if ( varVal.rindex( ':' ) == 0 )
        liqglo_proceduralPath = removeEscapes( parseString( varVal ) ) + liqglo_proceduralPath;
      else
        liqglo_proceduralPath = removeEscapes( parseString( varVal ) );
    }
  }
}
