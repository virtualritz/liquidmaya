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

/* ______________________________________________________________________
** 
** Liquid Rib Translator Source 
** ______________________________________________________________________
*/ 
// Error macro: if not successful, print error message and return
// the MStatus instance containing the error code.
// Assumes that "stat" contains the error value

// Standard Headers
#include <iostream.h>
#include <fstream.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <malloc.h>
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
#define M_PI 3.1415926535897932384626433832795
#endif

#ifdef _WIN32
#pragma warning(disable:4786)
#endif

// win32 mkdir only has name arg
#ifdef WIN32
#define MKDIR(_DIR_, _MODE_) (mkdir(_DIR_))
#else
#define MKDIR(_DIR_, _MODE_) (mkdir(_DIR_, _MODE_))
#endif

// Renderman Headers
extern "C" {
#include <ri.h>
	/* Rib Stream Defines */
	/* Commented out for Win32 as there is conflicts with Maya's drand on Win32 - go figure */
#ifndef _WIN32
#if 0
// Hmmmmmm do not compile
#include <target.h>
#endif
#endif
}

// STL headers
#include <list>
#include <vector>
#include <string>
#include <map>

#ifdef _WIN32
#include <process.h>
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#include <stdlib.h>
#include <alloca.h>
#endif

#ifdef _WIN32
// unix build gets this from the Makefile
static const char * LIQUIDVERSION = 
#include "liquid.version"
;
#endif

// Maya's Headers
#include <maya/MComputation.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDag.h>
#include <maya/MFnCamera.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnNurbsSurface.h>
#include <maya/MFnTransform.h>
#include <maya/MFnMesh.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFloatArray.h>
#include <maya/MIntArray.h>
#include <maya/MPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MPoint.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MString.h>
#include <maya/MTime.h>
#include <maya/MVector.h>
#include <maya/MFloatVector.h>
#include <maya/MArgList.h>
#include <maya/MColor.h>
#include <maya/MPxCommand.h>
#include <maya/MPxNode.h>
#include <maya/MGlobal.h>
#include <maya/M3dView.h>
#include <maya/MItSurfaceCV.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MSelectionList.h>
#include <maya/MAnimControl.h>
#include <maya/MItCurveCV.h>
#include <maya/MFnLight.h>
#include <maya/MFnNonAmbientLight.h>
#include <maya/MFnNonExtendedLight.h>
#include <maya/MFnAmbientLight.h>
#include <maya/MFnDirectionalLight.h>
#include <maya/MFnLightDataAttribute.h>
#include <maya/MFnPointLight.h>
#include <maya/MFnSpotLight.h>
#include <maya/MFnSet.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnBlinnShader.h>
#include <maya/MFnPhongShader.h>
#include <maya/MCommandResult.h>
#include <maya/MPxNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MTypeId.h>
#include <maya/MFn.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnStringData.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MBoundingBox.h>
#include <maya/MItSelectionList.h>
#include <maya/MFileIO.h>
#include <maya/MQuaternion.h>
#include <maya/MSyntax.h>

#include <liquid.h>
#include <liqRibNode.h>
#include <liqGlobalHelpers.h>
#include <liqRibData.h>
#include <liqRibCoordData.h>
#include <liqRibHT.h>
#include <liqRibTranslator.h>
#include <liqGetSloInfo.h>
#include <liqRibGenData.h>
#include <liqMemory.h>
#include <liqProcessLauncher.h>
#include <liqEntropyRenderer.h>
#include <liqPrmanRenderer.h>
#include <liqAqsisRenderer.h>
#include <liqDelightRenderer.h>

typedef int RtError;

// this get's set if we are running the commandline version of liquid
bool liquidBin;
int debugMode;

#define LIQ_CANCEL_FEEDBACK_MESSAGE MString( "Liquid -> RibGen Cancelled!\n" )
#define LIQ_CHECK_CANCEL_REQUEST    if ( m_escHandler.isInterruptRequested() ) throw( LIQ_CANCEL_FEEDBACK_MESSAGE )
#define LIQ_ADD_SLASH_IF_NEEDED(a) if ( a.asChar()[a.length() - 1] != '/' ) a += "/"
#define LIQ_ANIM_EXT MString( ".%0*d");

#ifndef _WIN32
const char *liqRibTranslator::m_default_tmp_dir = "/tmp";
#endif

// Kept global for liquidRigGenData and liquidRibParticleData
FILE	    	*liqglo_ribFP;
long	    liqglo_lframe;
structJob   	liqglo_currentJob;
bool		  liqglo_doMotion;           		// Motion blur for transformations
bool		  liqglo_doDef;              		// Motion blur for deforming objects
bool	    	liqglo_doCompression;	      	    	// output compressed ribs
bool	    	liqglo_doBinary;	      	    	// output binary ribs
RtFloat     	    liqglo_sampleTimes[5];  	    	    // current sample times
liquidlong  	    liqglo_motionSamples;   	    	// used to assign more than two motion blur samples!
float 		    liqglo_shutterTime;
// Kept global for liquidRigLightData
bool	    	    liqglo_doShadows;
MString     	    	liqglo_sceneName;
MString			liqglo_texDir;
bool		    liqglo_isShadowPass;	// true if we are rendering a shadow pass
bool	    	    liqglo_expandShaderArrays;
bool	    	    liqglo_useBMRT;
bool	    	    liqglo_shortShaderNames;		// true if we don't want to output path names with shaders
MStringArray 	    	liqglo_DDimageName;

// Kept global for liquidGlobalHelper

MString			liqglo_ribDir;
MString			liqglo_projectDir;

// Kept global for liqRibNode.cpp
MStringArray liqglo_preReadArchive;
MStringArray liqglo_preRibBox;
MStringArray liqglo_preReadArchiveShadow;
MStringArray liqglo_preRibBoxShadow;

// our global renderer object
liqRenderer *liqglo_renderer = NULL;

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
    if ( debugMode ) { printf( "-> freeing shader data.\n" ); }
    std::vector<liqShader>::iterator iter = m_shaders.begin();
    while ( iter != m_shaders.end() )
    {
	int k = 0;
	while ( k < iter->numTPV ) {
	    iter->freeShader( );
    	    k++;
	}
	++iter;
    }
    m_shaders.clear();
    if ( debugMode ) { printf("-> finished freeing shader data.\n" ); }
}
// Hmmmmm should change magic to Liquid
MString liqRibTranslator::magic("##RenderMan");

void *liqRibTranslator::creator()
//
//  Description:
//      Create a new instance of the translator
//
{
    return new liqRibTranslator();
}


liqShader & liqRibTranslator::liqGetShader( MObject shaderObj )
{
    MString rmShaderStr;

    MFnDependencyNode shaderNode( shaderObj );
    MPlug rmanShaderNamePlug = shaderNode.findPlug( MString( "rmanShaderLong" ) );
    rmanShaderNamePlug.getValue( rmShaderStr );

    if ( debugMode ) { printf("-> Using Renderman Shader %s. \n", rmShaderStr.asChar() ) ;}

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
    	currentShader.tokenPointerArray[ currentShader.numTPV ].set( argName, pType, false, false, false, 0 );
	triplePlug.child(0).getValue( x );
	triplePlug.child(1).getValue( y );
	triplePlug.child(2).getValue( z );
	currentShader.tokenPointerArray[ currentShader.numTPV ].setTokenFloat( 0, x, y, z );
	currentShader.numTPV++;
    }
    return status;
}




void liqRibTranslator::printProgress( int stat, long first, long last, long where )
// Description:
// Member function for printing the progress to the 
// Maya Console or stdout.  If alfred is being used 
// it will print it in a format that causes the correct
// formatting for the progress meters
{ 
	float numFrames = ( last - first ) + 1;
	float framesDone = where - first;
	float statSize = ( ( 1 / (float)numFrames ) / 4 ) * (float)stat * 100.0;
	float progressf = (( (float)framesDone / (float)numFrames ) * 100.0 ) + statSize;
	int progress = ( int ) progressf;
	
	if ( !liquidBin ) {
		MString progressOutput = "Progress: ";
		progressOutput += (int)progress;
		progressOutput += "%";
		liquidInfo( progressOutput );
	} else {
		cout << "ALF_PROGRESS " << progress << "%\n" << flush;
	}
}					

bool liqRibTranslator::liquidInitGlobals()
// Description:
// checks to see if the liquidGlobals are available
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

liqRibTranslator::liqRibTranslator()
//
//  Description:
//      Class constructor
//
{
#ifdef _WIN32
	m_systemTempDirectory = getenv("TEMP");
#else
	m_systemTempDirectory = getenv( "TEMPDIR");
    	// Env var not found
	if( !m_systemTempDirectory )
	    m_systemTempDirectory = m_default_tmp_dir;
#endif
    m_rFilterX = 1;
    m_rFilterY = 1;
    m_rFilter = fBoxFilter;


    /* BMRT PARAMS: BEGIN */
    liqglo_useBMRT = false;
    m_BMRTusePrmanDisp = true;
    m_BMRTusePrmanSpec = false;
    m_BMRTDStep = 0;
    m_RadSteps = 0;
    m_RadMinPatchSamples = 1;
    /* BMRT PARAMS: END */
	
    liqglo_shortShaderNames = false;
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
    useAlfred = false;
    cleanRib = false;
	cleanAlf = false;
    liqglo_doBinary = false;
    liqglo_doCompression = false;
    doDof = false;
    outputpreview = 0;
    liqglo_doMotion = false;		// matrix motion blocks
    liqglo_doDef = false;			// geometry motion blocks
    doCameraMotion = false;		// camera motion blocks
    doExtensionPadding = 0; // pad the frame number in the rib file names
    liqglo_doShadows = true;			// render shadows
    m_justRib = false;
    cleanShadows = 0;			// render shadows
    cleanTextures = 0;			// render shadows
    frameFirst = 1;			// range
    frameLast = 1;
    frameBy = 1;
    pixelSamples = 1;
    shadingRate = 1.0;
    depth = 1;
    outFormat = "it";
    m_animation = false;
    m_useFrameExt = true;		// Use frame extensions
    outExt = "tif";
    riboutput = "liquid.rib";
    m_renderer = PRMan;
    liqglo_motionSamples = 2;
    width = 360;
    height = 243;
    aspectRatio = 1.0;
    m_outPadding = 0;
    ignoreFilmGate = true;
    renderAllCameras = true;
    m_lazyCompute = false;
    m_outputShadersInShadows = false;
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
    m_blurTime = 1.0;
	fullShadowRib = false;
	baseShadowName = "";
	quantValue = 8;
    liqglo_projectDir = m_systemTempDirectory;
    liqglo_ribDir = "rib/";
    liqglo_texDir = "rmantex/";
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
    m_postJobCommand.clear();
    m_postFrameCommand.clear();
    m_preFrameCommand.clear();
    m_outputComments = false;
    m_shaderDebug = false;
#ifdef _WIN32
    m_renderCommand = "prman";
#else
    m_renderCommand = "render";
#endif
    m_ribgenCommand = "liquid";
    m_shaderPath = "&:.:~";
	createOutputDirectories = true;
	
    liqglo_expandShaderArrays = false;   
	
    /* Display Driver Defaults */
    m_numDisplayDrivers = 0;
    m_DDParams.clear();
    liqglo_DDimageName.clear();
    m_DDimageType.clear();
    m_DDimageMode.clear();
    m_DDparamType.clear();
	
	m_minCPU = m_maxCPU = 1;
	m_cropX1 = m_cropY1 = 0.0;
	m_cropX2 = m_cropY2 = 1.0;
    liqglo_isShadowPass = false;
}   

liqRibTranslator::~liqRibTranslator()
//  Description:
//      Class destructor
{
// this is crashing under Win32
//#ifdef _WIN32
//	lfree( m_systemTempDirectory );
//#endif
//	if ( debugMode ) { printf("-> dumping unfreed memory.\n" ); }
//	if ( debugMode ) ldumpUnfreed();

    delete liqglo_renderer;
}   

#if defined ENTROPY || PRMAN
void liqRibTranslatorErrorHandler( RtInt code, RtInt severity, char * message )
#else
void liqRibTranslatorErrorHandler( RtInt code, RtInt severity, const char * message )
#endif
//  Description:
//  Error handling function.  This gets called when the RIB library detects an error.  
{
    printf( "The renderman library is reporting and error! Code: %d  Severity: %d", code, severity );
    MString error( message );
    throw error;
}

MSyntax liqRibTranslator::syntax()
{
	MSyntax syntax;

	syntax.addFlag("p",   "preview");
	syntax.addFlag("nop", "noPreview");
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
	syntax.addFlag("nrm", "netRman");
	syntax.addFlag("fsr", "fullShadowRib");
	syntax.addFlag("rem", "remote");
	syntax.addFlag("alf", "alfred");
	syntax.addFlag("nal", "noAlfred");
	syntax.addFlag("err", "errHandler");
	syntax.addFlag("sdb", "shaderDB");
	syntax.addFlag("n",   "animation", MSyntax::kLong, MSyntax::kLong, MSyntax::kLong);
	syntax.addFlag("m",   "mbSamples", MSyntax::kLong);
	syntax.addFlag("dbs", "defBlock");
	syntax.addFlag("cam", "camera",  MSyntax::kString);
	syntax.addFlag("s",   "samples", MSyntax::kLong);
	syntax.addFlag("rnm", "ribName", MSyntax::kString);
	syntax.addFlag("od",  "projDir", MSyntax::kString);
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
	syntax.addFlag("ps",  "pixelFilter", MSyntax::kLong, MSyntax::kLong, MSyntax::kLong);
	syntax.addFlag("gs",  "gridSize",  MSyntax::kLong);
	syntax.addFlag("txm", "texmem",    MSyntax::kLong);
	syntax.addFlag("es",  "eyeSplits", MSyntax::kLong);
	syntax.addFlag("ar",  "aspect",    MSyntax::kDouble);
	syntax.addFlag("x",   "width",     MSyntax::kLong);
	syntax.addFlag("y",   "height",    MSyntax::kLong);
	syntax.addFlag("ndf", "noDef");
	syntax.addFlag("pad", "padding", MSyntax::kLong);

	return syntax;
}

MStatus liqRibTranslator::liquidDoArgs( MArgList args ) 
{
//	Description:
//		Read the values from the command line and set the internal values

	int i;
	MStatus status;
	MString argValue;

	if ( debugMode ) { printf("-> processing arguments\n"); }

	// Parse the arguments and set the options.
	if ( args.length() == 0 ) {
		liquidInfo( "Doing nothing, no parameters given." );
		return MS::kFailure;
	}

	// find the activeView for previews;
	m_activeView = M3dView::active3dView();
	width  = m_activeView.portWidth();
	height = m_activeView.portHeight();

	// get the current project directory
	MString MELCommand = "workspace -q -rd";
	MString MELReturn;
	MGlobal::executeCommand( MELCommand, MELReturn );
	liqglo_projectDir = MELReturn ;

	if ( debugMode ) { printf("-> using path: %s\n", liqglo_projectDir.asChar() ); }

	// get the current scene name
	liqglo_sceneName = liquidTransGetSceneName();

	// setup default animation parameters
	frameFirst = (int) MAnimControl::currentTime().as( MTime::uiUnit() );
	frameLast  = (int) MAnimControl::currentTime().as( MTime::uiUnit() );
	frameBy    = 1;

	// check to see if the correct project directory was found
	if ( !fileExists( liqglo_projectDir ) ) liqglo_projectDir = m_systemTempDirectory;
	LIQ_ADD_SLASH_IF_NEEDED( liqglo_projectDir );
	if ( !fileExists( liqglo_projectDir ) ) {
	    cout << "Liquid -> Cannot find /project dir, defaulting to system temp directory!\n" << flush;
	    liqglo_projectDir = m_systemTempDirectory;
	}
	liqglo_ribDir = liqglo_projectDir + "rib/";
	liqglo_texDir = liqglo_projectDir + "rmantex/";
	m_pixDir = liqglo_projectDir + "rmanpix/";
	m_tmpDir = liqglo_projectDir + "rmantmp/";

	for ( i = 0; i < args.length(); i++ ) {
		if ( MString( "-p" ) == args.asString( i, &status ) )  {
			LIQCHECKSTATUS(status, "error in -p parameter");
			outputpreview = 1;
		} else if ( MString( "-nop" ) == args.asString( i, &status ) )  {
			LIQCHECKSTATUS(status, "error in -p parameter");
			outputpreview = 0;
		} else if ( MString( "-GL" ) == args.asString( i, &status ) )  {
			LIQCHECKSTATUS(status, "error in -GL parameter");
			//load up all the render global parameters!
			if ( liquidInitGlobals() ) liquidReadGlobals();
		} else if ( MString( "-sel" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -sel parameter");
			m_renderSelected = true;
		} else if ( MString( "-ra" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -ra parameter");
			m_exportReadArchive = true;
		} else if ( MString( "-allCurves" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -allCurves parameter" );
			m_renderAllCurves = true;
		} else if ( MString( "-tiff" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -tiff parameter");
			outFormat = "tiff";
		} else if ( MString( "-dofOn" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -dofOn parameter");
			doDof = true;
		} else if ( MString( "-doBinary" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -doBinary parameter");
			liqglo_doBinary = true;
		} else if ( MString( "-shadows" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -doShadows parameter");
			liqglo_doShadows = true;
		} else if ( MString( "-noshadows" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -noshadows parameter");
			liqglo_doShadows = false;
		} else if ( MString( "-doCompression" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -doCompression parameter");
			liqglo_doCompression = true;
		} else if ( MString( "-cleanRib" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -cleanRib parameter");
			cleanRib = true;
		} else if ( MString( "-progress" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -progress parameter");
			m_showProgress = true;
		} else if ( MString( "-mb" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -mb parameter");
			liqglo_doMotion = true;
		} else if ( MString( "-db" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -db parameter");
			liqglo_doDef = true;
		} else if ( MString( "-d" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -d parameter");
			debugMode = 1;
		} else if ( MString( "-netRman" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -netRman parameter");
			useNetRman = true;
		} else if ( MString( "-fullShadowRib" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -fullShadowRib parameter");
			fullShadowRib = true;
		} else if ( MString( "-remote" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -remote parameter");
			remoteRender = true;
		} else if ( MString( "-alfred" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -alfred parameter");
			useAlfred = true;
		} else if ( MString( "-noalfred" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -noalfred parameter");
			useAlfred = false;
		} else if ( MString( "-err" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -err parameter");
			m_errorMode = 1;
		} else if ( MString( "-shaderDB" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -shaderDB parameter");
			m_shaderDebug = true;
		} else if ( MString( "-n" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -n parameter");   i++;
			
			argValue = args.asString( i, &status );
			frameFirst = argValue.asInt();
			
			LIQCHECKSTATUS(status, "error in -n parameter");  i++;
			argValue = args.asString( i, &status );
			frameLast = argValue.asInt();
			
			LIQCHECKSTATUS(status, "error in -n parameter");  i++;
			argValue = args.asString( i, &status );
			frameBy = argValue.asInt();
			
			LIQCHECKSTATUS(status, "error in -n parameter");
			m_animation = true;
		} else if ( MString( "-m" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -m parameter");   i++;
			argValue = args.asString( i, &status );
			liqglo_motionSamples = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -m parameter");
		} else if ( MString( "-defBlock" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -defBlock parameter");   i++;
			argValue = args.asString( i, &status );
			m_deferredBlockSize = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -defBlock parameter");
		} else if ( MString( "-cam" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -cam parameter");   i++;
			renderCamera = args.asString( i, &status );
			LIQCHECKSTATUS(status, "error in -cam parameter");
		} else if ( MString( "-s" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -s parameter");  i++;
			argValue = args.asString( i, &status );
			pixelSamples = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -s parameter");
		} else if ( MString( "-ribName" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -ribName parameter");  i++;
			liqglo_sceneName = args.asString( i, &status );
			LIQCHECKSTATUS(status, "error in -ribName parameter");
		} else if ( MString( "-od" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -od parameter");  i++;
			MString parsingString = args.asString( i, &status );
			liqglo_projectDir = parseString( parsingString );
			LIQ_ADD_SLASH_IF_NEEDED( liqglo_projectDir );
			if ( !fileExists( liqglo_projectDir ) ) { 
			    cout << "Liquid -> Cannot find /project dir, defaulting to system temp directory!\n" << flush;
			    liqglo_projectDir = m_systemTempDirectory; 
			}
			liqglo_ribDir = liqglo_projectDir + "rib/";
			liqglo_texDir = liqglo_projectDir + "rmantex/";
			m_pixDir = liqglo_projectDir + "rmanpix/";
			m_tmpDir = liqglo_projectDir + "rmantmp/";
			LIQCHECKSTATUS(status, "error in -od parameter");
		} else if ( MString( "-preFrameMel" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -preFrameMel parameter");  i++;
			MString parsingString = args.asString( i, &status );
			m_preFrameMel = parseString( parsingString );
			LIQCHECKSTATUS(status, "error in -preFrameMel parameter");
		} else if ( MString( "-postFrameMel" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -postFrameMel parameter");  i++;
			MString parsingString = args.asString( i, &status );
			m_postFrameMel = parseString( parsingString );
			LIQCHECKSTATUS(status, "error in -postFrameMel parameter");
		} else if ( MString( "-ribdir" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -ribDir parameter");  i++;
			MString parsingString = args.asString( i, &status );
			liqglo_ribDir = parseString( parsingString );
			LIQCHECKSTATUS(status, "error in -ribDir parameter");
		} else if ( MString( "-texdir" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -texDir parameter");  i++;
			MString parsingString = args.asString( i, &status );
			liqglo_texDir = parseString( parsingString );
			LIQCHECKSTATUS(status, "error in -texDir parameter");
		} else if ( MString( "-tmpdir" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -tmpDir parameter");  i++;
			MString parsingString = args.asString( i, &status );
			m_tmpDir = parseString( parsingString );
			LIQCHECKSTATUS(status, "error in -tmpDir parameter");
		} else if ( MString( "-picdir" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -picDir parameter");  i++;
			MString parsingString = args.asString( i, &status );
			m_pixDir = parseString( parsingString );
			LIQCHECKSTATUS(status, "error in -picDir parameter");
		} else if ( MString( "-preCommand" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -preCommand parameter");  i++;
			m_preCommand = args.asString( i, &status );
			LIQCHECKSTATUS(status, "error in -preCommand parameter");
		} else if ( MString( "-postJobCommand" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -postJobCommand parameter");  i++;
			MString varVal = args.asString( i, &status );
			m_postJobCommand = parseString( varVal );
			LIQCHECKSTATUS(status, "error in -postJobCommand parameter");
		} else if ( MString( "-postFrameCommand" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -postFrameCommand parameter");  i++;
			m_postFrameCommand = args.asString( i, &status );
			LIQCHECKSTATUS(status, "error in -postFrameCommand parameter");
		} else if ( MString( "-preFrameCommand" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -preFrameCommand parameter");  i++;
			m_preFrameCommand = args.asString( i, &status );
			LIQCHECKSTATUS(status, "error in -preFrameCommand parameter");
		} else if ( MString( "-renderCommand" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -renderCommand parameter");  i++;
			m_renderCommand = args.asString( i, &status );
			LIQCHECKSTATUS(status, "error in -renderCommand parameter");
		} else if ( MString( "-ribgenCommand" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -ribgenCommand parameter");  i++;
			m_ribgenCommand = args.asString( i, &status );
			LIQCHECKSTATUS(status, "error in -ribgenCommand parameter");
		} else if ( MString( "-blurTime" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -blurTime parameter");  i++;
			argValue = args.asString( i, &status );
			m_blurTime = argValue.asDouble();
			LIQCHECKSTATUS(status, "error in -blurTime parameter");
		} else if ( MString( "-sr" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -sr parameter");  i++;
			argValue = args.asString( i, &status );
			shadingRate = argValue.asDouble();
			LIQCHECKSTATUS(status, "error in -sr parameter");
		} else if ( MString( "-bs" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -bs parameter");  i++;
			argValue = args.asString( i, &status );
			bucketSize[0] = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -bs parameter");  i++;
			argValue = args.asString( i, &status );
			bucketSize[1] = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -bs parameter");
		} else if ( MString( "-ps" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -ps parameter");  i++;
			argValue = args.asString( i, &status );
			m_rFilter = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -ps parameter");  i++;
			argValue = args.asString( i, &status );
			m_rFilterX = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -ps parameter");  i++;
			argValue = args.asString( i, &status );
			m_rFilterY = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -ps parameter");
		} else if ( MString( "-gs" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -gs parameter");  i++;
			argValue = args.asString( i, &status );
			gridSize = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -gs parameter");
		} else if ( MString( "-texmem" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -texmem parameter");  i++;
			argValue = args.asString( i, &status );
			textureMemory = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -texmem parameter");
		} else if ( MString( "-es" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -es parameter");  i++;
			argValue = args.asString( i, &status );
			eyeSplits = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -es parameter");
		} else if ( MString( "-aspect" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -aspect parameter");  i++;
			argValue = args.asString( i, &status );
			aspectRatio = argValue.asDouble();
			LIQCHECKSTATUS(status, "error in -aspect parameter");
		} else if ( MString( "-x" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -x parameter");  i++;
			argValue = args.asString( i, &status );
			width = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -x parameter");
		} else if ( MString( "-y" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -y parameter");  i++;
			argValue = args.asString( i, &status );
			height = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -y parameter");
		} else if ( MString( "-noDef" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -noDef parameter");  
			m_deferredGen = false;
		} else if ( MString( "-pad" ) == args.asString( i, &status ) ) {
			LIQCHECKSTATUS(status, "error in -pad parameter");  i++;
			argValue = args.asString( i, &status );
			m_outPadding = argValue.asInt();
			LIQCHECKSTATUS(status, "error in -pad parameter");
		} 
	}
	return MS::kSuccess;
}

void liqRibTranslator::liquidReadGlobals() 
{
//	Description:
//		Read the values from the render globals and set internal values
	MStatus gStatus;
	MPlug gPlug;
	MFnDependencyNode rGlobalNode( rGlobalObj );
	/* Display Driver Globals */
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
		gPlug = rGlobalNode.findPlug( "shaderPath", &gStatus );
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		gStatus.clear();
		if ( varVal != "" ) {
			m_shaderPath += ":";
			m_shaderPath += parseString( varVal );
		}
	}
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
		if ( varVal != MString("") ) m_renderCommand = varVal;
		gStatus.clear();
	}
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "alfredFileName", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		if ( varVal != MString("") ) m_userAlfredFileName = varVal;
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
	gPlug = rGlobalNode.findPlug( "previewMode", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( outputpreview );
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
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "ribName", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		gStatus.clear();
		if ( varVal != "" ) {
			liqglo_sceneName = parseString( varVal );
		}
	}
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
				
	/* PIXELFILTER OPTIONS: BEGIN */
	gPlug = rGlobalNode.findPlug( "PixelFilter", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( m_rFilter );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "PixelFilterX", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( m_rFilterX );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "PixelFilterY", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( m_rFilterY );
	gStatus.clear();
	/* PIXELFILTER OPTIONS: END */
	
	/* BMRT OPTIONS:BEGIN */
	gPlug = rGlobalNode.findPlug( "BMRTAttrs", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_useBMRT );
	gStatus.clear();
	if ( liqglo_useBMRT ) {
		m_renderCommand = "rendrib ";
	}
	gPlug = rGlobalNode.findPlug( "BMRTDStep", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( m_BMRTDStep );
	gStatus.clear();
	if ( m_BMRTDStep > 0 ) {
		m_renderCommand += "-d ";
		m_renderCommand += (int)m_BMRTDStep;
		m_renderCommand += " ";
	}
	gPlug = rGlobalNode.findPlug( "RadSteps", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( m_RadSteps );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "RadMinPatchSamples", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( m_RadMinPatchSamples );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "BMRTusePrmanSpec", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( m_BMRTusePrmanSpec );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "BMRTusePrmanDisp", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( m_BMRTusePrmanDisp );
	gStatus.clear();
	/* BMRT OPTIONS:END */
		
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
				
	gPlug = rGlobalNode.findPlug( "shortShaderNames", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_shortShaderNames );
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
	gPlug = rGlobalNode.findPlug( "ignoreAlfred", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( useAlfred );
	gStatus.clear();
	useAlfred = !useAlfred;
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
	gPlug = rGlobalNode.findPlug( "compressedOutput", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_doCompression );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "exportReadArchive", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( m_exportReadArchive );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "alfredJobName", &gStatus );
	if ( gStatus == MS::kSuccess ) gPlug.getValue( alfredJobName );
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
	gPlug = rGlobalNode.findPlug( "transformationBlur", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( doCameraMotion );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "deformationBlur", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_doDef );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "motionBlurSamples", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( liqglo_motionSamples );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "depthOfField", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( doDof );
	gStatus.clear();
		
	gPlug = rGlobalNode.findPlug( "pixelSamples", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( pixelSamples );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "shadingRate", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( shadingRate );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "bucketXSize", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( bucketSize[0] );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "bucketYSize", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( bucketSize[1] );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "gridSize", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( gridSize );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "textureMemory", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( textureMemory );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "eyeSplits", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( eyeSplits );
	gStatus.clear();
				
	gPlug = rGlobalNode.findPlug( "cleanRib", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( cleanRib );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "cleanAlf", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( cleanAlf );
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
}

bool liqRibTranslator::verifyOutputDirectories()
{
#ifdef _WIN32
	int dirMode = 0; // dummy arg
#else
	mode_t dirMode;
	dirMode = S_IRWXU|S_IRWXG|S_IRWXO;
#endif

	bool problem = false;
	if ( ( access( liqglo_ribDir.asChar(), 0 )) == -1 ) {
		if ( createOutputDirectories ) {
			if ( MKDIR( liqglo_ribDir.asChar(), dirMode ) != 0 ) {
				printf( "Liquid -> had trouble creating rib dir, defaulting to system temp directory!\n" );
				liqglo_ribDir = m_systemTempDirectory;
				problem = true;
			}
		} else {
			printf( "Liquid -> Cannot find /rib dir, defaulting to system temp directory!\n" );
			liqglo_ribDir = m_systemTempDirectory;
			problem = true;
		}
	}
	if ( (access( liqglo_texDir.asChar(), 0 )) == -1 ) {
		if ( createOutputDirectories ) {
			if ( MKDIR( liqglo_texDir.asChar(), dirMode ) != 0 ) {
				printf( "Liquid -> had trouble creating tex dir, defaulting to system temp directory!\n" );
				liqglo_texDir = m_systemTempDirectory;
				problem = true;
			}
		} else {
			printf( "Liquid -> Cannot find /tex dir, defaulting to system temp directory!\n" );
			liqglo_texDir = m_systemTempDirectory;
			problem = true;
		}
	}
	if ( (access( m_pixDir.asChar(), 0 )) == -1 ) {
		if ( createOutputDirectories ) {
			if ( MKDIR( m_pixDir.asChar(), dirMode ) != 0 ) {
				printf( "Liquid -> had trouble creating pix dir, defaulting to system temp directory!\n" );
				m_pixDir = m_systemTempDirectory;
				problem = true;
			}
		} else {
			printf( "Liquid -> Cannot find /pix dir, defaulting to system temp directory!\n" );
			m_pixDir = m_systemTempDirectory;
			problem = true;
		}
	}
	if ( (access( m_tmpDir.asChar(), 0 )) == -1 ) {
		if ( createOutputDirectories ) {
			if ( MKDIR( m_tmpDir.asChar(), dirMode ) != 0 ) {
				printf( "Liquid -> had trouble creating tmp dir, defaulting to system temp directory!\n" );
				m_tmpDir = m_systemTempDirectory;
				problem = true;
			}
		} else {
			printf( "Liquid -> Cannot find /tmp dir, defaulting to system temp directory!\n" );
			m_tmpDir = m_systemTempDirectory;
			problem = true;
		}
	}
	return problem;
}

MStatus liqRibTranslator::doIt( const MArgList& args )
//  Description:
//      This method actually does the renderman output
{
    // first thing we should do is setup our renderer
    
    // TODO: got to make this much better in the future -- get the renderer
    // and version from the globals UI
#ifdef ENTROPY
    liqglo_renderer = new liqEntropyRenderer("3.1");
#endif

#ifdef PRMAN
    liqglo_renderer = new liqPrmanRenderer("3.9");
#endif

#ifdef AQSIS
    liqglo_renderer = new liqAqsisRenderer("0.7.4");
#endif

#ifdef DELIGHT
    liqglo_renderer = new liqDelightRenderer("1.0.0");
#endif

    MStatus status;
    MString lastRibName;

	status = liquidDoArgs( args );
	if (!status) {
		return MS::kFailure;	
	}

	if ( !liquidBin ) liquidInfo("Creating Rib <Press ESC To Cancel> ...");

	// Remember the frame the scene was at so we can restore it later.
	MTime currentFrame = MAnimControl::currentTime();

	// append the progress flag for alfred feedback
	if ( useAlfred ) {
		if ( ( m_renderCommand == MString( "render" ) ) || ( m_renderCommand == MString( "prman" ) ) ) {
			m_renderCommand = m_renderCommand + " -Progress";
		}
	}

	// check to see if the output camera, if specified, is available
	if ( liquidBin && ( renderCamera == "" ) ) {
		printf( "No Render Camera Specified" );
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
	if ( m_texDirG.length() > 0 ) liqglo_texDir = m_texDirG;
	if ( m_pixDirG.length() > 0 ) m_pixDir = m_pixDirG;
	if ( m_tmpDirG.length() > 0 ) m_tmpDir = m_tmpDirG;

	// make sure the directories end with a slash
	LIQ_ADD_SLASH_IF_NEEDED( liqglo_ribDir );
	LIQ_ADD_SLASH_IF_NEEDED( liqglo_texDir );
	LIQ_ADD_SLASH_IF_NEEDED( m_pixDir );
	LIQ_ADD_SLASH_IF_NEEDED( m_tmpDir );

	// setup the error handler
	if ( m_errorMode ) RiErrorHandler( liqRibTranslatorErrorHandler );

	// Setup helper variables for alfred
	MString alfredCleanUpCommand;
	if ( remoteRender ) {
		alfredCleanUpCommand = MString( "RemoteCmd" );
	} else {
		alfredCleanUpCommand = MString( "Cmd" );
	}

	MString alfredRemoteTagsAndServices;
	if ( remoteRender || useNetRman ) {
		alfredRemoteTagsAndServices = MString( "-service { " );
		alfredRemoteTagsAndServices += m_alfredServices.asChar();
		alfredRemoteTagsAndServices += MString( " } -tags { " );
		alfredRemoteTagsAndServices += m_alfredTags.asChar();
		alfredRemoteTagsAndServices += MString( " } " );
	}

	/*  A seperate one for cleanup as it doens't need a tag! */
	MString alfredCleanupRemoteTagsAndServices;
	if ( remoteRender || useNetRman ) {
		alfredCleanupRemoteTagsAndServices = MString( "-service { " );
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
			cout << "Liquid -> Cannot find pre frame mel script! Assuming local.\n" << flush;
		}
		if ( ( m_postFrameMel != "" ) && !fileExists( m_postFrameMel ) ) {
			cout << "Liquid -> Cannot find post frame mel script! Assuming local.\n" << flush;
		}

		//BMRT didn't support deformation blur at the time of this writing
		if (m_renderer == BMRT) {
			liqglo_doDef = false;
		}

		char *alfredFileName;
		ofstream alfFile;
#ifndef _WIN32
		struct timeval t_time;
		struct timezone t_zone;
		size_t tempSize = 0;

		char currentHostName[1024];
		gethostname( currentHostName, tempSize );
		liquidlong hashVal = liquidHash( currentHostName );
#endif
		// build alfred file name
		MString tempAlfname = m_tmpDir;
		if ( m_userAlfredFileName != MString( "" ) ){
			tempAlfname += m_userAlfredFileName;
		} else {
			tempAlfname += liqglo_sceneName;
#ifndef _WIN32
			gettimeofday( &t_time, &t_zone );
			srandom( t_time.tv_usec + hashVal );
			short alfRand = random();
			tempAlfname += alfRand;
#endif
		}
		tempAlfname += ".alf";

		alfredFileName = (char *)alloca( sizeof( char ) * tempAlfname.length() +1);
		strcpy( alfredFileName , tempAlfname.asChar() );

		// build deferred temp maya file name
		MString tempDefname = m_tmpDir;
		tempDefname += liqglo_sceneName;
#ifndef _WIN32
		gettimeofday( &t_time, &t_zone );
		srandom( t_time.tv_usec + hashVal );
		short defRand = random();
		tempDefname += defRand;
#endif

		MString currentFileType = MFileIO::fileType();
		if ( MString( "mayaAscii" ) == currentFileType ) tempDefname += ".ma";
		if ( MString( "mayaBinary" ) == currentFileType ) tempDefname += ".mb";
		if ( m_deferredGen ) {
			MFileIO::exportAll( tempDefname, currentFileType.asChar() );
		}

		if ( !m_deferredGen && m_justRib ) useAlfred = false;

		if ( useAlfred ) {
			alfFile.open( alfredFileName );

			//write the little header information alfred needs
			alfFile << "##AlfredToDo 3.0" << "\n";

			//write the main job info
			if ( alfredJobName == "" ) { alfredJobName = liqglo_sceneName; }
			alfFile << "Job -title {" << alfredJobName.asChar() 
				<< "(liquid job)} -comment {#Created By Liquid " << LIQUIDVERSION << "} "
				<< "-service " << "{}" << " "
				<< "-tags " << "{}" << " ";
			if ( useNetRman ) {
				alfFile << "-atleast " << m_minCPU << " " << "-atmost " << m_maxCPU << " ";
			} else {
				alfFile << "-atleast " << "1" << " "
					<< "-atmost " << "1" << " ";
			}
			alfFile << "-init " << "{" << " "
				<< "\n";

			alfFile << "} -subtasks {" << "\n";
			alfFile << flush;
		}

		// start looping through the frames
		int currentBlock = 0;
		for( liqglo_lframe=frameFirst; liqglo_lframe<=frameLast; liqglo_lframe = liqglo_lframe + frameBy ) {
			if ( m_showProgress ) printProgress( 1, frameFirst, frameLast, liqglo_lframe );

			m_shadowRibGen = false;
			m_alfShadowRibGen = false;
			liqglo_preReadArchive.clear();
			liqglo_preRibBox.clear();
			liqglo_preReadArchiveShadow.clear();
			liqglo_preRibBoxShadow.clear();

			// make sure all the global strings are parsed for this frame
			MString frameRenderCommand = parseString( m_renderCommand );
			MString frameRibgenCommand = parseString( m_ribgenCommand );
			MString framePreCommand = parseString( m_preCommand );
			MString framePreFrameCommand = parseString( m_preFrameCommand );
			MString framePostFrameCommand = parseString( m_postFrameCommand );

			if ( useAlfred ) {
				if ( m_deferredGen ) {
					if ( (( liqglo_lframe - frameFirst ) % m_deferredBlockSize ) == 0 ) {
						if ( m_deferredBlockSize == 1 ) {
							currentBlock = liqglo_lframe;
						} else {
							currentBlock++;
						}
						alfFile << "	Task -title {" << liqglo_sceneName.asChar() << "FrameRIBGEN" << currentBlock << "} -subtasks {\n";
						alfFile << "			} -cmds { \n";
						if ( remoteRender ) {
							alfFile << "			      RemoteCmd { " << framePreCommand.asChar() << "  ";
							alfFile << frameRibgenCommand.asChar();
						} else {
							alfFile << "			      Cmd { " << framePreCommand.asChar() << "  ";
							alfFile << frameRibgenCommand.asChar();
						}
						int lastGenFrame = ( liqglo_lframe + ( m_deferredBlockSize - 1 ) );
						if ( lastGenFrame > frameLast ) lastGenFrame = frameLast;
						alfFile << " -progress -noDef -nop -noalfred -od " << liqglo_projectDir.asChar() << " -ribName " << liqglo_sceneName.asChar() << " -mf " << tempDefname.asChar() << " -n " << liqglo_lframe << " " << lastGenFrame << " " << frameBy << " } ";
						if ( m_alfredExpand ) {
							alfFile	<< "-expand 1 ";
						}
						alfFile << "-service { " << m_defGenService.asChar() << " } ";
						alfFile	<< "-tags { " << m_defGenKey.asChar() << " }\n";
						alfFile << "			}\n";
					}
				}
				if ( !m_justRib ) {
					alfFile << "	Task -title {" << liqglo_sceneName.asChar() << "Frame" << liqglo_lframe << "} -subtasks {\n";

					if ( m_deferredGen ) {
						alfFile << "		Instance {" << liqglo_sceneName.asChar() << "FrameRIBGEN" << currentBlock << "}\n";
					}

					alfFile << flush;
				}
			}
			// Hmmmmmm not really clean ....
			if ( buildJobs() != MS::kSuccess ) break;

			{
				baseShadowName = liqglo_ribDir;
				if ( ( liqglo_DDimageName[0] == "" ) ) {
					baseShadowName += liqglo_sceneName;
				} else {
					int pointIndex = liqglo_DDimageName[0].index( '.' );
					baseShadowName += liqglo_DDimageName[0].substring(0, pointIndex-1).asChar();
				}
				baseShadowName += "_SHADOWBODY";
				baseShadowName += LIQ_ANIM_EXT;
				baseShadowName += extension;
				// Hmmmmmmm protect from buffer overflow : got to find a clean way ...
				size_t shadowNameLength = baseShadowName.length() + 1;
				shadowNameLength += 10;
				char *baseShadowRibName;
				baseShadowRibName = (char *)alloca(shadowNameLength);
				sprintf(baseShadowRibName, baseShadowName.asChar(), doExtensionPadding ? m_outPadding : 0, liqglo_lframe);
				baseShadowName = baseShadowRibName;
			}

			if ( !m_deferredGen ) {

				htable = new liqRibHT();

				float sampleinc = ( liqglo_shutterTime * m_blurTime ) / ( liqglo_motionSamples - 1 );
				for ( int msampleOn = 0; msampleOn < liqglo_motionSamples; msampleOn++ ) {
					float subframe = ( liqglo_lframe - ( liqglo_shutterTime * m_blurTime * 0.5 ) ) + msampleOn * sampleinc;
					liqglo_sampleTimes[ msampleOn ] = subframe;
				}

				if ( liqglo_doMotion || liqglo_doDef ) {
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
						m_ignoreSurfaces = true;
						liqglo_isShadowPass = true;
					} else {
						liqglo_isShadowPass = false;
					}

					if ( debugMode ) { printf("-> setting RiOptions\n"); }

					// Rib client file creation options MUST be done before RiBegin
					if ( debugMode ) { printf("-> setting binary option\n"); }
					if ( liqglo_doBinary )
					{
						RtString format = "binary";
						RiOption(( RtToken ) "rib", ( RtToken ) "format", ( RtPointer )&format, RI_NULL);
					} else {
						RtString format = "ascii";
						RiOption(( RtToken ) "rib", ( RtToken ) "format", ( RtPointer )&format, RI_NULL);
					}

					if ( debugMode ) { printf("-> setting compression option\n"); }
					if ( liqglo_doCompression )
					{
							RtString comp = "gzip";
						RiOption(( RtToken ) "rib", ( RtToken ) "compression", &comp, RI_NULL);
					} else {
							RtString comp = "none";
						RiOption(( RtToken ) "rib", ( RtToken ) "compression", &comp, RI_NULL);
					}

					// world RiReadArchives and Rib Boxes
					if ( liqglo_currentJob.isShadow && !m_shadowRibGen && !fullShadowRib ) {
#ifndef	PRMAN
						if ( debugMode ) { printf("-> beginning rib output\n"); }
						RiBegin( const_cast<char *>( baseShadowName.asChar()));
#else
						liqglo_ribFP = fopen( baseShadowName.asChar(), "w" );
						if ( liqglo_ribFP ) {
							if ( debugMode ) { printf("-> setting pipe option\n"); }
							RtInt ribFD = fileno( liqglo_ribFP );
							RiOption( ( RtToken )"rib", ( RtToken )"pipe", &ribFD, RI_NULL );
						}
						if ( debugMode ) { printf("-> beginning rib output\n"); }
						RiBegin(RI_NULL);
#endif
						if (   frameBody() != MS::kSuccess ) break;
						RiEnd();
#ifdef PRMAN
						fclose( liqglo_ribFP );
#endif
						liqglo_ribFP = NULL;
						m_shadowRibGen = true;
						m_alfShadowRibGen = true;
					}
#ifndef	PRMAN
					RiBegin( const_cast<char *>( liqglo_currentJob.ribFileName.asChar() ) );
#else
					liqglo_ribFP = fopen( liqglo_currentJob.ribFileName.asChar(), "w" );
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
							if ( frameBody() != MS::kSuccess ) break;
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

			// now we re-iterate through the job list to write out the alfred file if we are using it

			// write out shadows

			if ( useAlfred && !m_justRib ) {
				if ( liqglo_doShadows ) {
					if ( debugMode ) { printf("-> writing out shadow information to alfred file.\n" ); }
					std::vector<structJob>::iterator iter = shadowList.begin();
					while ( iter != shadowList.end() ) {
						alfFile << "	      Task -title {" << iter->name.asChar() << "} -subtasks {\n";
						if ( m_deferredGen ) {
							alfFile << "		Instance {" << liqglo_sceneName.asChar() << "FrameRIBGEN" << currentBlock << "}\n";
						}
						alfFile << "			} -cmds { \n";
						if ( useNetRman ) {
							alfFile << "			      Cmd { " << framePreCommand.asChar() << " netrender %H -Progress " << iter->ribFileName.asChar() << "} ";
						} else if ( remoteRender ) {
								alfFile << "			      RemoteCmd { " << framePreCommand.asChar() << "  " << frameRenderCommand.asChar() << " " << iter->ribFileName.asChar() << "} ";
						} else {
							alfFile << "			      Cmd { " << framePreCommand.asChar() << "  " << frameRenderCommand.asChar() << " " << iter->ribFileName.asChar() << "} ";
						}
						if ( m_alfredExpand ) {
							alfFile	<< "-expand 1 ";
						}
						alfFile << alfredRemoteTagsAndServices.asChar() << "\n";


						if (cleanRib)  {
							alfFile << "			} -cleanup {\n";
							alfFile << "			      " << alfredCleanUpCommand.asChar() << " { " << framePreCommand.asChar();
							alfFile << "  /bin/rm ";
							alfFile << iter->ribFileName.asChar() << "} " << alfredCleanupRemoteTagsAndServices.asChar() << "\n";
						}
						alfFile << "			} -chaser {\n";
						alfFile << "						sho \"" << iter->imageName.asChar() << "\"\n";
						alfFile << "			}\n";

						++iter;
						if ( !m_alfShadowRibGen && !fullShadowRib ) m_alfShadowRibGen = true;
					}
				}
				if ( debugMode ) { printf("-> finished writing out shadow information to alfred file.\n" ); }

				if ( debugMode ) { printf("-> initiating hero pass information.\n" ); }
				structJob *frameJob = NULL;
				structJob *shadowPassJob = NULL;
				if ( debugMode ) { printf("-> setting hero pass.\n" ); }
				if ( m_outputHeroPass && !m_outputShadowPass ) {
					frameJob = &jobList[ jobList.size() - 1 ];
				} else if ( m_outputShadowPass && m_outputHeroPass ) {
					frameJob = &jobList[ jobList.size() - 1 ];
					shadowPassJob = &jobList[ jobList.size() - 2 ];
				} else if ( m_outputShadowPass && !m_outputHeroPass ) {
					shadowPassJob = &jobList[ jobList.size() - 1 ];
				}
				if ( debugMode ) { printf("-> hero pass set.\n" ); }

				alfFile << "	} -cmds {\n";

				if ( debugMode ) { printf("-> writing out pre frame command information to alfred file.\n" ); }
				if ( framePreFrameCommand != MString("") ) {
					if ( useNetRman ) {
						alfFile << "				  Cmd { " << framePreFrameCommand.asChar() << "} " << alfredRemoteTagsAndServices.asChar() << "\n";
					} else if ( remoteRender ) {
						alfFile << "				  RemoteCmd { " << framePreFrameCommand.asChar() << "} " << alfredRemoteTagsAndServices.asChar() << "\n";
					} else {
						alfFile << "				  Cmd { " << framePreFrameCommand.asChar() << "} " << alfredRemoteTagsAndServices.asChar() << "\n";
					}
				}

				if ( m_outputHeroPass ) {
					if ( useNetRman ) {
						alfFile << "			      Cmd { " << framePreCommand.asChar() << "  netrender %H -Progress " << frameJob->ribFileName.asChar() << "} ";
					} else if ( remoteRender ) {
						alfFile << "			      RemoteCmd { " << framePreCommand.asChar() << "  " << frameRenderCommand.asChar() << " " << frameJob->ribFileName.asChar() << "} ";
					} else {
						alfFile << "			      Cmd { " << framePreCommand.asChar() << "  " << frameRenderCommand.asChar() << " " << frameJob->ribFileName.asChar() << "} ";
					}
					if ( m_alfredExpand ) {
						alfFile	<< "-expand 1 ";
					}
					alfFile << alfredRemoteTagsAndServices.asChar() << "\n";

				}
				if ( debugMode ) { printf("-> finished writing out hero information to alfred file.\n" ); }
				alfFile << flush;

				if ( m_outputShadowPass ) {
					if ( useNetRman ) {
						alfFile << "			      Cmd { " << framePreCommand.asChar() << "  netrender %H -Progress " << shadowPassJob->ribFileName.asChar() << "} ";
					} else if ( remoteRender ) {
						alfFile << "			      RemoteCmd { " << framePreCommand.asChar() << "  " << frameRenderCommand.asChar() << " " << shadowPassJob->ribFileName.asChar() << "} ";
					} else {
						alfFile << "			      Cmd { " << framePreCommand.asChar() << "  " << frameRenderCommand.asChar() << " " << shadowPassJob->ribFileName.asChar() << "} ";
					}
					if ( m_alfredExpand ) {
						alfFile	<< "-expand 1 ";
					}
					alfFile << alfredRemoteTagsAndServices.asChar() << "\n";

				}
				alfFile << flush;

				if (cleanRib || (framePostFrameCommand != MString("")) ) {
					alfFile << "	} -cleanup {\n";

					if (cleanRib) {

						if ( m_outputHeroPass ) alfFile << "	      " << alfredCleanUpCommand.asChar() << " { " << framePreCommand.asChar() << "  /bin/rm " << frameJob->ribFileName.asChar() << "} " << alfredCleanupRemoteTagsAndServices.asChar() << "\n";
						if ( m_outputShadowPass ) alfFile << "	      " << alfredCleanUpCommand.asChar() << " { " << framePreCommand.asChar() << "  /bin/rm " << shadowPassJob->ribFileName.asChar() << "} " << alfredCleanupRemoteTagsAndServices.asChar() << "\n";
						if ( m_alfShadowRibGen ) alfFile << "	      " << alfredCleanUpCommand.asChar() << " { " << framePreCommand.asChar() << "  /bin/rm " << baseShadowName.asChar() << "} " << alfredCleanupRemoteTagsAndServices.asChar() << "\n";
					}
					if ( framePostFrameCommand != MString("") ) {
						if ( useNetRman ) {
							alfFile << "				  Cmd { " << framePostFrameCommand.asChar() << "} " << alfredRemoteTagsAndServices.asChar() << "\n";
						} else if ( remoteRender ) {
							alfFile << "				  RemoteCmd { " << framePostFrameCommand.asChar() << "} " << alfredRemoteTagsAndServices.asChar() << "\n";
						} else {
							alfFile << "				  Cmd { " << framePostFrameCommand.asChar() << "} " << alfredRemoteTagsAndServices.asChar() << "\n";
						}
					}

				}
				alfFile << "	} -chaser {\n";
				if ( m_outputHeroPass ) alfFile << "				sho \"" << frameJob->imageName.asChar() << "\"\n";
				if ( m_outputShadowPass ) alfFile << "				sho \"" << shadowPassJob->imageName.asChar() << "\"\n";
				alfFile << "	}\n";
				if ( m_outputShadowPass && !m_outputHeroPass ) {
					lastRibName = shadowPassJob->ribFileName.asChar();
				} else {
					lastRibName = frameJob->ribFileName.asChar();
				}
			}

			if ( ( ribStatus != kRibOK ) && !m_deferredGen ) break;
		}

		if ( useAlfred ) {
			// clean up the alfred file in the future
			if ( !m_justRib ) {
				alfFile << "} -cleanup { \n";
				if ( m_deferredGen ) {
					alfFile << "" << alfredCleanUpCommand.asChar() << " { /bin/rm " << tempDefname.asChar() << "} " << alfredCleanupRemoteTagsAndServices.asChar() << "\n";
				}
				if ( cleanAlf ) {
					alfFile << "" << alfredCleanUpCommand.asChar() << " { /bin/rm " << alfredFileName << "} " << alfredCleanupRemoteTagsAndServices.asChar() << "\n";
				}
				if ( m_postJobCommand != MString("") ) {
					if ( useNetRman ) {
						alfFile << "				  Cmd { " << m_postJobCommand.asChar() << "}\n";
					} else if ( remoteRender ) {
						alfFile << "				  RemoteCmd { " << m_postJobCommand.asChar() << "}\n";
					} else {
						alfFile << "				  Cmd { " << m_postJobCommand.asChar() << "}\n";
					}
				}
			}
			alfFile << "}\n";
			alfFile.close();
		}

		if ( debugMode ) { printf("-> ending escape handler.\n" ); }
		m_escHandler.endComputation();

		if ( !liquidBin ) liquidInfo("...Finished Creating Rib.");
		if ( debugMode ) { printf("-> clearing job list.\n" ); }
		jobList.clear();

		// set the attributes on the liquidGlobals for the last rib file and last alfred script name
		if ( debugMode ) { printf("-> setting lastAlfredScript and lastRibFile.\n" ); }
		MFnDependencyNode rGlobalNode( rGlobalObj );
		MPlug nPlug;
		nPlug = rGlobalNode.findPlug( "lastAlfredScript" );
		nPlug.setValue( tempAlfname );
		nPlug = rGlobalNode.findPlug( "lastRibFile" );
		nPlug.setValue( lastRibName );

		if ( debugMode ) { printf("-> spawning command.\n" ); }

		if ( outputpreview ) {
			if ( useAlfred ) {
				liqProcessLauncher::execute( "alfred", alfredFileName );
			} else {
				liqProcessLauncher::execute( m_renderCommand, liqglo_currentJob.ribFileName );
			}
		}

		if ( debugMode ) { printf("-> setting frame to current frame.\n" ); }
		// Return to the frame we were at before we ran the animation
		MGlobal::viewFrame (currentFrame);

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

void liqRibTranslator::portFieldOfView( int port_width, int port_height,
									double& horizontal,
									double& vertical,
									MFnCamera& fnCamera )
									//
									//  Description:
									//      Calculate the port field of view for the camera
									//
{
    double left, right, bottom, top;
    double aspect = (double) port_width / port_height;
    computeViewingFrustum(aspect,left,right,bottom,top,fnCamera);
	
    double neardb = fnCamera.nearClippingPlane();
    horizontal = atan(((right - left) * 0.5) / neardb) * 2.0;
    vertical = atan(((top - bottom) * 0.5) / neardb) * 2.0;
}

void liqRibTranslator::computeViewingFrustum ( double     window_aspect,
										   double&    left,
										   double&    right,
										   double&    bottom,
										   double&    top,
										   MFnCamera&	cam )
										   //
										   //  Description:
										   //      Calculate the viewing frustrum for the camera
										   //
{
    double film_aspect = cam.aspectRatio();
    double aperture_x = cam.horizontalFilmAperture();
    double aperture_y = cam.verticalFilmAperture();
    double offset_x = cam.horizontalFilmOffset();
    double offset_y = cam.verticalFilmOffset();
    double focal_to_near = cam.nearClippingPlane() /
		(cam.focalLength() * MM_TO_INCH);
	
    focal_to_near *= cam.cameraScale();
	
    double scale_x = 1.0;
    double scale_y = 1.0;
    double translate_x = 0.0;
    double translate_y = 0.0;
	
	
	switch (cam.filmFit()) {
    case MFnCamera::kFillFilmFit:
        if (window_aspect < film_aspect) {
            scale_x = window_aspect / film_aspect;
        }
        else {
            scale_y = film_aspect / window_aspect;
        }
		
		break;
		
	case MFnCamera::kHorizontalFilmFit:
        scale_y = film_aspect / window_aspect;
		
		if (scale_y > 1.0) {
            translate_y = cam.filmFitOffset() *
                (aperture_y - (aperture_y * scale_y)) / 2.0;
        }
		
		break;
		
	case MFnCamera::kVerticalFilmFit:
        scale_x = window_aspect / film_aspect;
		
		
		if (scale_x > 1.0) {
            translate_x = cam.filmFitOffset() *
                (aperture_x - (aperture_x * scale_x)) / 2.0;
        }
		
		break;
		
	case MFnCamera::kOverscanFilmFit:
        if (window_aspect < film_aspect) {
            scale_y = film_aspect / window_aspect;
        }
        else {
            scale_x = window_aspect / film_aspect;
        }
		
		break;
    }
    
	
    left   = focal_to_near * (-.5*aperture_x*scale_x+offset_x+translate_x);
    right  = focal_to_near * ( .5*aperture_x*scale_x+offset_x+translate_x);
    bottom = focal_to_near * (-.5*aperture_y*scale_y+offset_y+translate_y);
    top    = focal_to_near * ( .5*aperture_y*scale_y+offset_y+translate_y);
}

void liqRibTranslator::getCameraInfo( MFnCamera& cam )
//
//  Description:
//      Get information about the given camera
//
{
	// Resoultion can change if camera film-gate clips image
	// so we must keep camera width/height separate from render
	// globals width/height.
	//
	cam_width  = width;
	cam_height = height;
	
	// If we are using a film-gate then we may need to
	// adjust the resolution to simulate the 'letter-boxed'
	// effect.
	//
	if ( cam.filmFit() == MFnCamera::kHorizontalFilmFit ) {
		if ( !ignoreFilmGate ) {
			double new_height = cam_width /
				(cam.horizontalFilmAperture() /
				cam.verticalFilmAperture());
			
			if ( new_height < cam_height ) {
				cam_height = (int)new_height;
			}
		}
		
		double hfov, vfov;
		portFieldOfView( cam_width, cam_height, hfov, vfov, cam );
		fov_ratio = hfov/vfov;
	}
	else if ( cam.filmFit() == MFnCamera::kVerticalFilmFit ) {
		double new_width = cam_height /
			(cam.verticalFilmAperture() /
			cam.horizontalFilmAperture());
		
		double hfov, vfov;
		
		// case 1 : film-gate smaller than resolution
		//	        film-gate on
		if ( (new_width < cam_width) && (!ignoreFilmGate) ) {
			cam_width = (int)new_width;
			fov_ratio = 1.0;		
		}
		
		// case 2 : film-gate smaller than resolution
		//	        film-gate off
		else if ( (new_width < cam_width) && (ignoreFilmGate) ) {
			portFieldOfView( (int)new_width, cam_height, hfov, vfov, cam );
			fov_ratio = hfov/vfov;
		}

		// case 3 : film-gate larger than resolution
		//	        film-gate on
		else if ( !ignoreFilmGate ) {
			portFieldOfView( (int)new_width, cam_height, hfov, vfov, cam );
			fov_ratio = hfov/vfov;
		}

		// case 4 : film-gate larger than resolution
		//	        film-gate off
		else if ( ignoreFilmGate ) {
			portFieldOfView( (int)new_width, cam_height, hfov, vfov, cam );
			fov_ratio = hfov/vfov;		
		}
		
	}
	else if ( cam.filmFit() == MFnCamera::kOverscanFilmFit ) {
		double new_height = cam_width /
			(cam.horizontalFilmAperture() /
			cam.verticalFilmAperture());
		double new_width = cam_height /
			(cam.verticalFilmAperture() /
			cam.horizontalFilmAperture());
		
		if ( new_width < cam_width ) {
			if ( !ignoreFilmGate ) {
				cam_width = (int) new_width;
				fov_ratio = 1.0;
			}
			else {
				double hfov, vfov;
				portFieldOfView( (int)new_width, cam_height, hfov, vfov, cam );
				fov_ratio = hfov/vfov;		
			}
		}
		else {
			if ( !ignoreFilmGate )
				cam_height = (int) new_height;
			
			double hfov, vfov;
			portFieldOfView( cam_width, cam_height, hfov, vfov, cam );
			fov_ratio = hfov/vfov;
		}		
	}
	else if ( cam.filmFit() == MFnCamera::kFillFilmFit ) {
		double new_width = cam_height /
			(cam.verticalFilmAperture() /
			cam.horizontalFilmAperture());				
		double hfov, vfov;
		
		if ( new_width >= cam_width ) {
			portFieldOfView( (int)new_width, cam_height, hfov, vfov, cam );
			fov_ratio = hfov/vfov;
		}
		else {						
			portFieldOfView( cam_width, cam_height, hfov, vfov, cam );
			fov_ratio = hfov/vfov;
		}
	}
}

MStatus liqRibTranslator::buildJobs()
//
//  Description:
//      Write the prologue for the RIB file
//
{
    if ( debugMode ) { printf("-> beginning to build job list\n"); }
    MStatus returnStatus = MS::kSuccess;
    MStatus status;
    MObject cameraNode;
    MDagPath lightPath;
    jobList.clear();
    shadowList.clear();
    structJob	thisJob;
	
    // what we do here is make all of the lights with depth shadows turned on into
    // cameras and add them to the renderable camera list *before* the main camera
    // so all the automatic depth map shadows are complete before the main pass
	
    if ( liqglo_doShadows ) {
		MItDag dagIterator( MItDag::kDepthFirst, MFn::kLight, &returnStatus );
		for (; !dagIterator.isDone(); dagIterator.next()) {
			bool		usesDepthMap;
			if ( !dagIterator.getPath(lightPath) ) continue;
			usesDepthMap = false;
			MFnLight fnLightNode( lightPath );
			fnLightNode.findPlug( "useDepthMapShadows" ).getValue( usesDepthMap );
			if ( usesDepthMap && areObjectAndParentsVisible( lightPath ) ) {
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
					
					/* now we check to see if the minmax option is used */
					thisJob.isMinMaxShadow = false;
					status.clear();
					MPlug liquidMinMaxShadow = fnLightNode.findPlug( "liquidMinMaxShadow", &status );
					if ( status == MS::kSuccess ) {
						liquidMinMaxShadow.getValue( thisJob.isMinMaxShadow );
					}
					
					bool computeShadow = true;
					if ( m_lazyCompute ) {
						MString baseFileName;
						if ( ( liqglo_DDimageName[0] == "" ) ) {
							baseFileName = liqglo_sceneName; 
						} else {
							int pointIndex = liqglo_DDimageName[0].index( '.' );
							baseFileName = liqglo_DDimageName[0].substring(0, pointIndex-1).asChar();
						} 
						baseFileName += "_";
						baseFileName += thisJob.name;
						baseFileName += "SHD";
						
						MString outFileFmtString;
						outFileFmtString = liqglo_texDir;
						outFileFmtString += baseFileName;
						
						size_t outNameLength;
						
						outNameLength = outFileFmtString.length();
						
						if (m_animation || m_useFrameExt) {
							outFileFmtString += LIQ_ANIM_EXT;
							outNameLength += m_outPadding + 1;
						}
						outFileFmtString += ".";
						outFileFmtString += "tex";
						outNameLength = outFileFmtString.length();
						// Hmmmmmm protect from buffer overflow ...
						outNameLength = outNameLength + 8; // Space for the null character
						
						char *outName = (char *)alloca(outNameLength);
						sprintf(outName, outFileFmtString.asChar(), 1, liqglo_lframe);
						MString fileName( outName );
						if ( fileExists( fileName ) ) computeShadow = false;
					}
					
					if ( computeShadow ) jobList.push_back( thisJob );
					
					/* We have to handle point lights differently as they need 6 shadow maps! */					
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
						if ( ( liqglo_DDimageName[0] == "" ) ) {
							baseFileName = liqglo_sceneName; 
						} else {
							int pointIndex = liqglo_DDimageName[0].index( '.' );
							baseFileName = liqglo_DDimageName[0].substring(0, pointIndex-1).asChar();
						} 
						baseFileName += "_";
						baseFileName += thisJob.name;
						baseFileName += "SHD";
						
						if ( dirOn == pPX ) { baseFileName += "_PX"; thisJob.pointDir = pPX; } else
							if ( dirOn == pPY ) { baseFileName += "_PY"; thisJob.pointDir = pPY; } else
								if ( dirOn == pPZ ) { baseFileName += "_PZ"; thisJob.pointDir = pPZ; } else
									if ( dirOn == pNX ) { baseFileName += "_NX"; thisJob.pointDir = pNX; } else
										if ( dirOn == pNY ) { baseFileName += "_NY"; thisJob.pointDir = pNY; } else
											if ( dirOn == pNZ ) { baseFileName += "_NZ"; thisJob.pointDir = pNZ; } 
											
											MString outFileFmtString;
											outFileFmtString = liqglo_texDir;
											outFileFmtString += baseFileName;
											
											size_t outNameLength;
											
											outNameLength = outFileFmtString.length();
											
											if (m_animation || m_useFrameExt) {
												outFileFmtString += LIQ_ANIM_EXT;
												outNameLength += m_outPadding + 1;
											}
											outFileFmtString += ".";
											outFileFmtString += "tex";
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
		if ( debugMode ) { printf("-> getting current view\n"); }
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
		if ( ( liqglo_DDimageName[0] == "" ) ) {
			baseFileName = liqglo_sceneName; 
		} else {
			int pointIndex = liqglo_DDimageName[0].index( '.' );
			baseFileName = liqglo_DDimageName[0].substring(0, pointIndex-1).asChar();
		}
		baseFileName += "_";
		baseFileName += iter->name;
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
		
		size_t fileNameLength = fileNameFmtString.length() + 8;		 
		fileNameLength += 10; // Enough to hold the digits for 1 billion frames
		char *frameFileName = (char *)alloca(fileNameLength);
		sprintf(frameFileName, fileNameFmtString.asChar(), doExtensionPadding ? m_outPadding : 0, liqglo_lframe);
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
			outFileFmtString = liqglo_texDir;
			size_t outNameLength;
			if ( userShadowName != MString( "" ) ) {
				outFileFmtString += userShadowName;
				outNameLength = outFileFmtString.length() + 1;
			} else {
				outFileFmtString += baseFileName;
			
				outNameLength = outFileFmtString.length();
		
				if (m_animation || m_useFrameExt) {
					outFileFmtString += LIQ_ANIM_EXT;
					outNameLength += m_outPadding + 1;
				}
				outFileFmtString += ".tex";
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
		
			if (m_animation || m_useFrameExt) {
				outFileFmtString += LIQ_ANIM_EXT;
				outNameLength += m_outPadding + 1;
			}
			
			outFileFmtString += ".";
			outFileFmtString += outExt;
			outNameLength = outFileFmtString.length();
			outNameLength = outNameLength + ( m_outPadding + 1 ); // Space for the null character
		
			char *outName = (char *)alloca(outNameLength);
			sprintf(outName, outFileFmtString.asChar(), m_outPadding, liqglo_lframe);
			if ( liqglo_DDimageName[0] == "" ) {
				iter->imageName = outName;
			} else {
				iter->imageName = m_pixDir; 
				iter->imageName += parseString( liqglo_DDimageName[0] );
			}
		
		}
		++iter;
	}
	
	ribStatus = kRibBegin;   
	return MS::kSuccess;
}

MStatus liqRibTranslator::ribPrologue()
//
//  Description:
//      Write the prologue for the RIB file
//
{
	if ( !m_exportReadArchive ) {
		if ( debugMode ) { printf("-> beginning to write prologue\n"); }
		// set any rib options		
		RiOption( "limits", "bucketsize", (RtPointer)&bucketSize, RI_NULL );
		RiOption( "limits", "gridsize", (RtPointer)&gridSize, RI_NULL );
		RiOption( "limits", "texturememory", (RtPointer)&textureMemory, RI_NULL );
		if (liqglo_renderer->supports(liqRenderer::EYESPLITS)) {
		    RiOption( "limits",
			      "eyesplits",
			      (RtPointer)&eyeSplits,
			      RI_NULL );
		}

		RiArchiveRecord( RI_COMMENT, "Shader Path\nOption \"searchpath\" \"shader\" [\"%s\"]\n", m_shaderPath.asChar() );

		/* BMRT OPTIONS: BEGIN */
		if ( liqglo_useBMRT ) {
			RiOption( "radiosity", "integer steps", (RtPointer)&m_RadSteps, RI_NULL );
			RiOption( "radiosity", "integer minpatchsamples", (RtPointer)&m_RadMinPatchSamples, RI_NULL );
			if ( m_BMRTusePrmanSpec ) {
				RtInt prmanSpec = 1;
				RiOption( "render", "integer prmanspecular", &prmanSpec, RI_NULL );
			}
			if ( m_BMRTusePrmanDisp ) {
				RtInt prmanDisp = 1;
				RiOption( "render", "integer useprmandspy", &prmanDisp, RI_NULL );
			}
		}
		/* BMRT OPTIONS: END */


		RiOrientation( RI_RH );       // Right-hand coordinates
		if ( liqglo_currentJob.isShadow ) {
			RiPixelSamples( 1, 1 );
			RiShadingRate( 1 );
		} else {
			RiPixelSamples( pixelSamples, pixelSamples );
			RiShadingRate( shadingRate );
			if ( m_rFilterX > 1 || m_rFilterY > 1 ) {
				if ( m_rFilter == fBoxFilter ) {
					RiPixelFilter( RiBoxFilter, m_rFilterX, m_rFilterY );
				} else if ( m_rFilter == fTriangleFilter ) {
					RiPixelFilter( RiTriangleFilter, m_rFilterX, m_rFilterY );
				} else if ( m_rFilter == fCatmullRomFilter ) {
					RiPixelFilter( RiCatmullRomFilter, m_rFilterX, m_rFilterY );
				} else if ( m_rFilter == fGaussianFilter ) {
					RiPixelFilter( RiGaussianFilter, m_rFilterX, m_rFilterY );
				} else if ( m_rFilter == fSincFilter ) {
					RiPixelFilter( RiSincFilter, m_rFilterX, m_rFilterY );
				}
			}
		}
	}
    ribStatus = kRibBegin;
    return MS::kSuccess;
}

MStatus liqRibTranslator::ribEpilogue()
//
//  Description:
//      Write the epilogue for the RIB file
//
{
    if (ribStatus == kRibBegin) ribStatus = kRibOK;
    return (ribStatus == kRibOK ? MS::kSuccess : MS::kFailure);
}

MStatus liqRibTranslator::scanScene(float lframe, int sample )
//
//  Description:
//      Scan the DAG at the given frame number and record information
//      about the scene for writing
//
{    
    MTime   mt((double)lframe);
    if ( MGlobal::viewFrame(mt) == MS::kSuccess) {
		
		if ( m_preFrameMel != "" ) MGlobal::sourceFile( m_preFrameMel );
		
		MStatus returnStatus;

		/* Scan the scene for place3DTextures - for coordinate systems */
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
				
				// if it's a light then insert it into the hash table
				if (	currentNode.hasFn( MFn::kPlace3dTexture ) ) {
					if ( ( sample > 0 ) && isObjectMotionBlur( path )) {
						htable->insert(path, lframe, sample, MRT_Coord );
					} else {
						htable->insert(path, lframe, 0, MRT_Coord );
					}
					continue;
				}
			}
		}

		/* Scan the scene for lights */
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
				if (	currentNode.hasFn( MFn::kLight ) ) {
					if ( ( sample > 0 ) && isObjectMotionBlur( path )) {
						htable->insert(path, lframe, sample, MRT_Light );
					} else {
						htable->insert(path, lframe, 0, MRT_Light );
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
				MFnDagNode	  dagNode;
				dagIterator.getPath( path ); 
				if (MS::kSuccess != returnStatus) continue;
				if (!currentNode.hasFn(MFn::kDagNode)) continue;
				returnStatus = dagNode.setObject( currentNode );
				if (MS::kSuccess != returnStatus) continue;
				
				// check for a rib generator 
				MStatus plugStatus;
				MPlug ribGenPlug = dagNode.findPlug( "liquidRibGen", &plugStatus );
				if ( plugStatus == MS::kSuccess ) {
				/* check the node to make sure it's not using the old ribGen assignment method, this is for backwards
					compatibility.  If it's a kTypedAttribute that it's more than likely going to be a string! */
					if ( ribGenPlug.attribute().apiType() == MFn::kTypedAttribute ) {
						MString ribGenNode;
						ribGenPlug.getValue( ribGenNode );
						MSelectionList ribGenList;
						MStatus ribGenAddStatus = ribGenList.add( ribGenNode );
						if ( ribGenAddStatus == MS::kSuccess ) {
							htable->insert( path, lframe, sample, MRT_RibGen );
						}
					} else {
						if ( ribGenPlug.isConnected() ) {
							htable->insert( path, lframe, sample, MRT_RibGen );
						}
					}
					
				}			
				if (	currentNode.hasFn(MFn::kNurbsSurface)
					|| currentNode.hasFn(MFn::kMesh)
					|| currentNode.hasFn(MFn::kParticle)
					|| currentNode.hasFn(MFn::kLocator) ) {
					if ( ( sample > 0 ) && isObjectMotionBlur( path )){
						htable->insert(path, lframe, sample, MRT_Unknown );
					} else {
						htable->insert(path, lframe, 0, MRT_Unknown );
					}
				}
				if (	currentNode.hasFn(MFn::kNurbsCurve) ) {
					MStatus plugStatus;
					MPlug renderCurvePlug = dagNode.findPlug( "renderCurve", &plugStatus );
					if ( m_renderAllCurves || ( plugStatus == MS::kSuccess ) ) {
						if ( ( sample > 0 ) && isObjectMotionBlur( path )){
							htable->insert(path, lframe, sample, MRT_Unknown );
						} else {
							htable->insert(path, lframe, 0, MRT_Unknown );
						}
					}
				}
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
					MFnDagNode	  dagNode;
					dagIterator.getPath( path ); 
					if (MS::kSuccess != returnStatus) continue;
					if (!currentNode.hasFn(MFn::kDagNode)) continue;
					returnStatus = dagNode.setObject( currentNode );
					if (MS::kSuccess != returnStatus) continue;
					
					// check for a rib generator 
					MStatus plugStatus;
					MPlug ribGenPlug = dagNode.findPlug( "liquidRibGen", &plugStatus );
					if ( plugStatus == MS::kSuccess ) {
						htable->insert( path, lframe, sample, MRT_RibGen );
					}			
					if (	currentNode.hasFn(MFn::kNurbsSurface)
						|| currentNode.hasFn(MFn::kMesh)
						|| currentNode.hasFn(MFn::kParticle)
						|| currentNode.hasFn(MFn::kLocator) ) {
						if ( ( sample > 0 ) && isObjectMotionBlur( path )){
							htable->insert(path, lframe, sample, MRT_Unknown );
						} else {
							htable->insert(path, lframe, 0, MRT_Unknown );
						}
					}
					if (	currentNode.hasFn(MFn::kNurbsCurve) ) {
						MStatus plugStatus;
						MPlug renderCurvePlug = dagNode.findPlug( "renderCurve", &plugStatus );
						if ( m_renderAllCurves || ( plugStatus == MS::kSuccess ) ) {
							if ( ( sample > 0 ) && isObjectMotionBlur( path )){
								htable->insert(path, lframe, sample, MRT_Unknown );
							} else {
								htable->insert(path, lframe, 0, MRT_Unknown );
							}
						}
					}
				}
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
				/*	      if ( iter->camera[0].motionBlur ) {
				doCameraMotion = 1;
				} else {
				doCameraMotion = 0;
			}*/
				fnCamera.getPath(path);
				MTransformationMatrix xform( path.inclusiveMatrix() );
				double scale[] = { 1, 1, -1 };
				//xform.addScale( scale, MSpace::kTransform );
				xform.setScale( scale, MSpace::kTransform );
				iter->camera[sample].mat = xform.asMatrixInverse();
				
				if ( fnCamera.isClippingPlanes() ) {
					iter->camera[sample].neardb    = fnCamera.nearClippingPlane();
					iter->camera[sample].fardb	   = fnCamera.farClippingPlane();
				} else {
					iter->camera[sample].neardb    = 0.001;
					iter->camera[sample].fardb	   = 10000.0;
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
			} else {
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
					iter->camera[sample].mat = xform.asMatrixInverse();
					iter->camera[sample].neardb    = fnCamera.nearClippingPlane();
					iter->camera[sample].fardb	   = fnCamera.farClippingPlane();
					iter->camera[sample].isOrtho = fnCamera.isOrtho();
					iter->camera[sample].orthoWidth = fnCamera.orthoWidth();
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
						if ( iter->pointDir == pPZ ) { double rotation[] = { 0, 0, 0 }; xform.setRotation( rotation, MTransformationMatrix::kXYZ ); }
						if ( iter->pointDir == pNZ ) { double rotation[] = { 0, M_PI, 0 }; xform.setRotation( rotation, MTransformationMatrix::kXYZ ); }
					} 
					iter->camera[sample].mat = xform.asMatrixInverse();
					iter->camera[sample].neardb    = 0.0001;
					iter->camera[sample].fardb	   = 10000.0;
					if ( fnLight.dagPath().hasFn( MFn::kDirectionalLight ) ) {
						iter->camera[sample].isOrtho = true;
						fnLight.findPlug( "dmapWidthFocus" ).getValue( iter->camera[sample].orthoWidth );
						fnLight.findPlug( "dmapWidthFocus" ).getValue( iter->camera[sample].orthoHeight );
					} else {
						iter->camera[sample].isOrtho = false;
						iter->camera[sample].orthoWidth = 0.0;
					}						
				}
				
				iter->camera[sample].shutter = 0;
				iter->camera[sample].motionBlur = false;
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
				iter->imageMode += "z";
				iter->format = "shadow";
			}
			
			++iter;
	}
	
	if ( m_postFrameMel != "" ) MGlobal::sourceFile( m_postFrameMel );
	
	return MS::kSuccess;
  }
  return MS::kFailure;
}

void liqRibTranslator::doAttributeBlocking( const MDagPath & newPath, const MDagPath & previousPath )
// 
//  Description:
//      This method takes care of the blocking together of objects and
//      their children in the DAG.  This method compares two DAG paths and
//      figures out how many attribute levels to push and/or pop
//
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
        const char * namePtr = name.asChar();
        RiAttributeBegin();
        RiAttribute( "identifier", "name", &namePtr, RI_NULL );
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

MStatus liqRibTranslator::framePrologue(long lframe)
//  
//  Description:
//  	Write out the frame prologue.
//  
{
	if ( debugMode ) printf( "-> Beginning Frame Prologue\n" );
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
		
		if ( debugMode ) printf( "-> Setting Display Options\n" );
		
		if ( liqglo_currentJob.isShadow ) {
			if ( !liqglo_currentJob.isMinMaxShadow ) {
				RiDisplay( const_cast<char *>(liqglo_currentJob.imageName.asChar()), const_cast<char *>(liqglo_currentJob.format.asChar()), (RtToken)liqglo_currentJob.imageMode.asChar(), RI_NULL ); 
			} else {
				RiArchiveRecord( RI_COMMENT, "Display Driver: \nDisplay \"%s\" \"%s\" \"%s\" \"minmax\" [ 1 ]", const_cast<char *>(liqglo_currentJob.imageName.asChar()), const_cast<char *>(liqglo_currentJob.format.asChar()), (RtToken)liqglo_currentJob.imageMode.asChar() );
			}
		} else {
			if ( ( m_cropX1 != 0.0 ) || ( m_cropY1 != 0.0 ) || ( m_cropX2 != 1.0 ) || ( m_cropY2 != 1.0 ) ) {
				RiCropWindow( m_cropX1, m_cropX2, m_cropY1, m_cropY2 );
			};
			
			int k = 0;
			while ( k < m_numDisplayDrivers ) {
				MString parameterString;
				MString imageName;
				MString formatType;
				MString imageMode;
				if ( k > 0 ) imageName = "+"; 
				imageName += m_pixDir;
				if ( liqglo_DDimageName[k] == "" ) {
					imageName = liqglo_currentJob.imageName;
				} else { 
					imageName += parseString( liqglo_DDimageName[k] );
				}
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
						RiDeclare( (RtToken)m_DDParams[k].names[p].asChar(), "string" );
						parameterString += " [\""; parameterString += m_DDParams[k].data[p].asChar(); parameterString += "\"] ";
					} else if ( m_DDParams[k].type[p] == 2 ) {
						
						/* BUGFIX: Monday 6th August fixed bug where a float display driver parameter wasn't declared */
						
						RiDeclare( (RtToken)m_DDParams[k].names[p].asChar(), "float" );
						parameterString += " ["; parameterString += m_DDParams[k].data[p].asChar(); parameterString += "] ";
					}
				}
				RiArchiveRecord( RI_COMMENT, "Display Driver %d: \nDisplay \"%s\" \"%s\" \"%s\" %s", k, imageName.asChar(), formatType.asChar(), imageMode.asChar(), parameterString.asChar() );
				k++; 
			}
		}
		
		if ( debugMode ) printf( "-> Setting Resolution\n" );
		
		RiFormat( liqglo_currentJob.width, liqglo_currentJob.height, liqglo_currentJob.aspectRatio );
		
		if ( liqglo_currentJob.camera[0].isOrtho ) {
			RtFloat frameWidth, frameHeight;
			frameWidth = liqglo_currentJob.camera[0].orthoWidth * 0.5; // the whole frame width has to be divided in half!
			frameHeight = liqglo_currentJob.camera[0].orthoHeight * 0.5; // the whole frame height has to be divided in half!
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
		//
		/* doCameraMotion = liqglo_currentJob.camera[0].motionBlur && liqglo_doMotion; */
		if ( liqglo_doMotion || liqglo_doDef ) {
			RiShutter( ( lframe - ( liqglo_currentJob.camera[0].shutter * 0.5 ) ), ( lframe + ( liqglo_currentJob.camera[0].shutter * 0.5 ) ) );
		} else {
			RiShutter( lframe, lframe );
		} 
		
		if ( liqglo_currentJob.gotJobOptions ) {
			RiArchiveRecord( RI_COMMENT, "jobOptions: \n%s", liqglo_currentJob.jobOptions.asChar() );
		}
		if ( ( liqglo_preRibBox.length() > 0 ) && !liqglo_currentJob.isShadow ) {
			int ii;
			for ( ii = 0; ii < liqglo_preRibBox.length(); ii++ ) {
				RiArchiveRecord( RI_COMMENT, "Additional Rib:\n%s", liqglo_preRibBox[ii].asChar() );
			}
		}
		if ( ( liqglo_preReadArchive.length() > 0 ) && !liqglo_currentJob.isShadow ) {
			int ii;
			for ( ii = 0; ii < liqglo_preReadArchive.length(); ii++ ) {
				RiArchiveRecord( RI_COMMENT, "Read Archive Data: \nReadArchive \"%s\"", liqglo_preReadArchive[ii].asChar() );
			}
		}
		if ( ( liqglo_preRibBoxShadow.length() > 0 ) && !liqglo_currentJob.isShadow ) {
			int ii;
			for ( ii = 0; ii < liqglo_preRibBoxShadow.length(); ii++ ) {
				RiArchiveRecord( RI_COMMENT, "Additional Rib:\n%s", liqglo_preRibBoxShadow[ii].asChar() );
			}
		}
		if ( ( liqglo_preReadArchiveShadow.length() > 0 ) && liqglo_currentJob.isShadow ) {
			int ii;
			for ( ii = 0; ii < liqglo_preReadArchiveShadow.length(); ii++ ) {
				RiArchiveRecord( RI_COMMENT, "Read Archive Data: \nReadArchive \"%s\"", liqglo_preReadArchiveShadow[ii].asChar() );
			}
		}
		
		if ( doCameraMotion && ( !liqglo_currentJob.isShadow )) {
			/*RiMotionBegin( 2, ( lframe - ( liqglo_currentJob.camera[0].shutter * m_blurTime * 0.5  )), ( lframe + ( liqglo_currentJob.camera[0].shutter * m_blurTime * 0.5  )) );*/
			RiMotionBegin( liqglo_motionSamples, liqglo_sampleTimes[0], liqglo_sampleTimes[1] , liqglo_sampleTimes[2], liqglo_sampleTimes[3], liqglo_sampleTimes[4] );
		}
		RtMatrix cameraMatrix;
		liqglo_currentJob.camera[0].mat.get( cameraMatrix );
		RiTransform( cameraMatrix );
		if ( doCameraMotion && ( !liqglo_currentJob.isShadow ) ) {	
			int mm = 1;
			while ( mm < liqglo_motionSamples ) {
				liqglo_currentJob.camera[mm].mat.get( cameraMatrix );
				RiTransform( cameraMatrix );
				mm++;
			}
			RiMotionEnd();
		}
	}
    return MS::kSuccess;
}

MStatus liqRibTranslator::frameEpilogue( long )
//  
//  Description:
//  	Write out the frame epilogue.
//  
{
    if (ribStatus == kRibFrame) {
		ribStatus = kRibBegin;
        if ( !m_exportReadArchive ) {
			RiFrameEnd();
		}
    }
    return (ribStatus == kRibBegin ? MS::kSuccess : MS::kFailure);
}

MStatus liqRibTranslator::frameBody()
//  
//  Description:
//  	Write out the body of the frame.  This includes a dump of the DAG
//  
{
    MStatus returnStatus = MS::kSuccess;
	MStatus status;
	attributeDepth = 0;
    
	RNMAP::iterator rniter;
	
	/* if this is a readArchive that we are exporting than ingore this header information */
	if ( !m_exportReadArchive ) {
		RiWorldBegin();
		RiTransformBegin();
		RiCoordinateSystem( "worldspace" );
		RiTransformEnd();
		
		if ( !liqglo_currentJob.isShadow && !m_ignoreLights ) {
			
			int nbLight = 0;
			
			for ( rniter = htable->RibNodeMap.begin(); rniter != htable->RibNodeMap.end(); rniter++ ) {
			    LIQ_CHECK_CANCEL_REQUEST;
				liqRibNode *	rn = (*rniter).second;
				if (rn->object(0)->ignore || rn->object(0)->type != MRT_Light) continue;
				rn->object(0)->writeObject();
				rn->object(0)->written = 1;
				nbLight++;
			}
			
		}
	}
	
    if ( m_ignoreSurfaces ) {
        RiSurface( "matte", RI_NULL );
    }
    
    MMatrix matrix;
    MDagPath path;
    MFnDagNode dagFn;
	
	
	for ( rniter = htable->RibNodeMap.begin(); rniter != htable->RibNodeMap.end(); rniter++ ) {
	    LIQ_CHECK_CANCEL_REQUEST;
		
		liqRibNode * ribNode = (*rniter).second;
		path = ribNode->path();
		
		if ( ( NULL == ribNode ) || ( ribNode->object(0)->type == MRT_Light ) ) continue;
		if ( ( !liqglo_currentJob.isShadow ) && ( ribNode->object(0)->ignore ) ) continue;
		if ( ( liqglo_currentJob.isShadow ) && ( ribNode->object(0)->ignoreShadow ) ) continue;
		
		char *namePtr = ( char * )alloca( sizeof( char ) * ribNode->name.length()+1);
		strcpy( namePtr, ribNode->name.asChar() );
		//const char * namePtr = ribNode->name;
		if ( m_outputComments ) RiArchiveRecord( RI_COMMENT, "Name: %s", ribNode->name.asChar(), RI_NULL );
		RiAttributeBegin();
		RiAttribute( "identifier", "name", &namePtr, RI_NULL );
		
		// If this is a matte object, then turn that on if it isn't currently set 
		if ( ribNode->matteMode ) {
			if ( !m_currentMatteMode ) RiMatte(RI_TRUE);
			m_currentMatteMode = true;
		} else {
			if ( m_currentMatteMode ) RiMatte(RI_FALSE);
			m_currentMatteMode = false;
		}
		// If this is a double sided object, then turn that on if it isn't currently set
		if ( ribNode->doubleSided ) {
			RiSides(2);
		} else {
			RiSides(1);
		}
		
		if ( debugMode ) { printf("-> Object name %s.\n", ribNode->name.asChar()); }
        
		MObject object;
		
		MObjectArray ignoredLights;
//		ribNode->getIgnoredLights( ribNode->assignedShadingGroup.object(), ignoredLights );
		ribNode->getIgnoredLights( ignoredLights );
		
		for ( int i=0; i<ignoredLights.length(); i++ )
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
		
		// If there is matrix motion blur, open a new motion block, the 5th element in the object will always 
		// be there if matrix blur will occur!
		if ( liqglo_doMotion && ribNode->doMotion && ( ribNode->object(1) != NULL ) && ( ribNode->object(0)->type != MRT_Locator ) 
			&& ( !liqglo_currentJob.isShadow ) ) {    
			if ( debugMode ) { printf("-> writing matrix motion blur data\n"); }
			RiMotionBegin( liqglo_motionSamples, liqglo_sampleTimes[0], liqglo_sampleTimes[1] , liqglo_sampleTimes[2], liqglo_sampleTimes[3], liqglo_sampleTimes[4] );
		}
		RtMatrix ribMatrix;
		matrix = ribNode->object(0)->matrix( path.instanceNumber() );
		matrix.get( ribMatrix );
		RiTransform( ribMatrix );
		
		// Output the world matrices for the motionblur
		// This will override the current transformation setting
		//
		if ( liqglo_doMotion && ribNode->doMotion && ( ribNode->object(1) != NULL ) && ( ribNode->object(0)->type != MRT_Locator ) 
			&& ( !liqglo_currentJob.isShadow ) ) {    
			path = ribNode->path();
			int mm = 1;
			RtMatrix ribMatrix;
			while ( mm < liqglo_motionSamples ) {
				matrix = ribNode->object(mm)->matrix( path.instanceNumber() );
				matrix.get( ribMatrix );
				RiTransform( ribMatrix );
				mm++;
			}
            RiMotionEnd();
		}
		
		MFnDagNode fnDagNode( path );
		bool hasSurfaceShader = false;
		bool hasDisplacementShader = false;
		bool hasVolumeShader = false;
		
		MPlug rmanShaderPlug;
		/* Check for surface shader */
		status.clear();
		rmanShaderPlug = fnDagNode.findPlug( MString( "liquidSurfaceShaderNode" ), &status );
		if ( ( status != MS::kSuccess ) || ( !rmanShaderPlug.isConnected() ) ) {	status.clear(); rmanShaderPlug = ribNode->assignedShadingGroup.findPlug( MString( "liquidSurfaceShaderNode" ), &status ); }
		if ( ( status != MS::kSuccess ) || ( !rmanShaderPlug.isConnected() ) ) {	status.clear(); rmanShaderPlug = ribNode->assignedShader.findPlug( MString( "liquidSurfaceShaderNode" ), &status ); }
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
		/* Check for displacement shader */
		status.clear();
		rmanShaderPlug = fnDagNode.findPlug( MString( "liquidDispShaderNode" ), &status );
		if ( ( status != MS::kSuccess ) || ( !rmanShaderPlug.isConnected() ) )	{ status.clear(); rmanShaderPlug = ribNode->assignedShadingGroup.findPlug( MString( "liquidDispShaderNode" ), &status ); }
		if ( ( status != MS::kSuccess ) || ( !rmanShaderPlug.isConnected() ) )	{ status.clear(); rmanShaderPlug = ribNode->assignedDisp.findPlug( MString( "liquidDispShaderNode" ), &status ); }
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
		/* Check for volume shader */
		status.clear();
		rmanShaderPlug = fnDagNode.findPlug( MString( "liquidVolumeShaderNode" ), &status );
		if ( ( status != MS::kSuccess ) || ( !rmanShaderPlug.isConnected() ) )	{ status.clear(); rmanShaderPlug = ribNode->assignedShadingGroup.findPlug( MString( "liquidVolumeShaderNode" ), &status ); }
		if ( ( status != MS::kSuccess ) || ( !rmanShaderPlug.isConnected() ) )	{ status.clear(); rmanShaderPlug = ribNode->assignedVolume.findPlug( MString( "liquidVolumeShaderNode" ), &status ); }
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
		if ( ( dispDisplacementBounds != 0.0 )  && ( dispDisplacementBounds > surfaceDisplacementBounds ) ) {
			RiAttribute( "displacementbound", "sphere", &dispDisplacementBounds, RI_NULL );
		} else if ( ( surfaceDisplacementBounds != 0.0 )  ) {
			RiAttribute( "displacementbound", "sphere", &surfaceDisplacementBounds, RI_NULL );
		}
		
		RtFloat currentNodeShadingRate = shadingRate;
		
		bool writeShaders = true;
		
		if ( liqglo_currentJob.isShadow && !m_outputShadersInShadows ) writeShaders = false;
		
		if ( writeShaders  ) {
			if ( hasVolumeShader && !m_ignoreVolumes ) {
				
				liqShader & currentShader = liqGetShader( ribNode->assignedVolume.object());
			
				if ( !currentShader.hasErrors ) {				
					RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * currentShader.numTPV );
					RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * currentShader.numTPV );					
					assignTokenArrays( currentShader.numTPV, currentShader.tokenPointerArray, tokenArray, pointerArray );
					char *shaderFileName;
					LIQ_GET_SHADER_FILE_NAME(shaderFileName, liqglo_shortShaderNames, currentShader );
					RiAtmosphereV ( shaderFileName, currentShader.numTPV, tokenArray, pointerArray );
				}
			} 
			
			if ( hasSurfaceShader && !m_ignoreSurfaces ) {
				
				liqShader & currentShader = liqGetShader( ribNode->assignedShader.object());

				RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * currentShader.numTPV );
				RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * currentShader.numTPV );
					
				assignTokenArrays( currentShader.numTPV, currentShader.tokenPointerArray, tokenArray, pointerArray );
				RiColor( currentShader.rmColor );
				RiOpacity( currentShader.rmOpacity );


				if ( ribNode->nodeShadingRateSet && ( ribNode->nodeShadingRate != currentNodeShadingRate ) ) {
					RiShadingRate ( ribNode->nodeShadingRate );
					currentNodeShadingRate = ribNode->nodeShadingRate;
				} else if ( currentShader.hasShadingRate ) {
					RiShadingRate ( currentShader.shadingRate );
					currentNodeShadingRate = currentShader.shadingRate;
				}
    	    	    	    	char *shaderFileName;
    	    	    	    	LIQ_GET_SHADER_FILE_NAME(shaderFileName, liqglo_shortShaderNames, currentShader );
    	    	    	    	RiSurfaceV ( shaderFileName, currentShader.numTPV, tokenArray, pointerArray );
			} else {
				RtColor rColor;
				if ( m_shaderDebug ) {
					rColor[0] = 1;
					rColor[1] = 0;
					rColor[2] = 0;
					RiColor( rColor );
				} else if ( ( ribNode->color.r != -1.0 ) ) {
					rColor[0] = ribNode->color[0];
					rColor[1] = ribNode->color[1];
					rColor[2] = ribNode->color[2];
					RiColor( rColor );
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
				RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * currentShader.numTPV );
				RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * currentShader.numTPV );
				
				assignTokenArrays( currentShader.numTPV, currentShader.tokenPointerArray, tokenArray, pointerArray );
				
				if ( ribNode->nodeShadingRateSet && ( ribNode->nodeShadingRate != currentNodeShadingRate ) ) {
					RiShadingRate ( ribNode->nodeShadingRate );
					currentNodeShadingRate = ribNode->nodeShadingRate;
				} else if ( currentShader.hasShadingRate ) {
					RiShadingRate ( currentShader.shadingRate );
					currentNodeShadingRate = currentShader.shadingRate;
				}
				char *shaderFileName;
    	    	    	    	LIQ_GET_SHADER_FILE_NAME(shaderFileName, liqglo_shortShaderNames, currentShader );
				RiDisplacementV ( shaderFileName, currentShader.numTPV, tokenArray, pointerArray );
			}
		}
		
		if ( ribNode->isRibBox ) {
			RiArchiveRecord( RI_COMMENT, "Additional Rib:\n%s", ribNode->ribBoxString.asChar() );
		}
		if ( ribNode->isArchive ) {
			RiArchiveRecord( RI_COMMENT, "Read Archive Data: \nReadArchive \"%s\"", ribNode->archiveString.asChar() );
		}
		if ( ribNode->isDelayedArchive ) {
			RiArchiveRecord( RI_COMMENT, "Delayed Read Archive Data: \nProcedural \"DelayedReadArchive\" [ \"%s\" ] [ %f %f %f %f %f %f ]", ribNode->delayedArchiveString.asChar(), ribNode->bound[0],ribNode->bound[3],ribNode->bound[1],ribNode->bound[4],ribNode->bound[2],ribNode->bound[5] );
		}
		
		// check to see if we are writing a curve to set the proper basis
		if ( ribNode->object(0)->type == MRT_NuCurve ) {
			RiBasis( RiBSplineBasis, 1, RiBSplineBasis, 1 );
		}
		
		// If there is deformation motion blur, open a new motion block, the 5th element in the object will always 
		// be there if deformation blur will occur!
		if (liqglo_doDef && ribNode->doDef && ( ribNode->object(1) != NULL ) && ( ribNode->object(0)->type != MRT_RibGen )
			&& ( ribNode->object(0)->type != MRT_Locator ) && ( !liqglo_currentJob.isShadow ) ) {
			RiMotionBegin( liqglo_motionSamples, liqglo_sampleTimes[0] , liqglo_sampleTimes[1] , liqglo_sampleTimes[2], liqglo_sampleTimes[3], liqglo_sampleTimes[4] );
		}        
		
		ribNode->object(0)->writeObject();
		if ( liqglo_doDef && ribNode->doDef && ( ribNode->object(1) != NULL ) && ( ribNode->object(0)->type != MRT_RibGen )
			&& ( ribNode->object(0)->type != MRT_Locator ) && ( !liqglo_currentJob.isShadow ) ) {
			if ( debugMode ) { printf("-> writing deformation blur data\n"); }
			int msampleOn = 1;
			while ( msampleOn < liqglo_motionSamples ) {
				ribNode->object(msampleOn)->writeObject();
				msampleOn++;
			}
			RiMotionEnd();
		}

		RiAttributeEnd();
    }
	
	
	
    while ( attributeDepth > 0 ) {
		RiAttributeEnd();  
        attributeDepth--;
    }
	
    if ( !m_exportReadArchive ) {
		RiWorldEnd();
	}
	return returnStatus;
}
