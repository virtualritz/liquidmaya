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
** Liquid Rib Translator Source 
** ______________________________________________________________________
*/ 

// Standard Headers
#include <iostream.h>
#include <fstream.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>
#ifdef UNIX
#include <sys/time.h>
#include <sys/stat.h>
#endif

#ifndef _WIN32
// Dynamic Object Headers
#include <dlfcn.h>
#include <libgen.h>
#endif

// Renderman Headers
extern "C" {
#include <ri.h>
#include <slo.h>
	/* Rib Stream Defines */
	/* Commented out for Win32 as there is conflicts with Maya's drand on Win32 - go figure */
#ifndef _WIN32
#include <target.h>
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

#include <liquid.h>
#include <liquidRibObj.h>
#include <liquidRibNode.h>
#include <liquidGlobalHelpers.h>
#include <liquidRibData.h>
#include <liquidRibSurfaceData.h>
#include <liquidRibNuCurveData.h>
#include <liquidRibMeshData.h>
#include <liquidRibParticleData.h>
#include <liquidRibLocatorData.h>
#include <liquidRibLightData.h>
#include <liquidRibCoordData.h>
#include <liquidRibHT.h>
#include <liquidRibTranslator.h>
#include <liquidGetSloInfo.h>
#include <liquidRIBStatus.h>
#include <liquidRIBGen.h>
#include <liquidRibGenData.h>
#include <liquidMemory.h>

typedef int RtError;

// this get's set if we are running the commandline version of liquid
bool liquidBin;

FILE			*ribFP;
MString		mayaVersion;

float liquidVersion = float(1.3);

MStatus tempStatus;

bool		  shortShaderNames;		// true if we don't want to output path names with shaders
bool		  doCompression;	      // output compressed ribs
bool		  doBinary;	      // output binary ribs
RendererType  renderer;		// which renderer are we using
char			*userhomedir;	// the user's home directory
char			*licensedir;	// the license file directory
bool			isShadowPass;	// true if we are rendering a shadow pass
int				debugMode;		// true if in debug mode
int				errorMode;
M3dView			activeView;
MString			projectDir;
MString			ribDir;
MString			pixDir;
MString			texDir;
MString			tmpDir;
MString			ribDirG;
MString			pixDirG;
MString			texDirG;
MString			tmpDirG;
MString			animExt;
MString			sceneName;
structJob		currentJob;
bool			animation;
bool			useFrameExt;
bool			shadowRibGen;
bool			alfShadowRibGen;
long			lframe;
float 		shutterTime;
double			blurTime;
liquidlong			outPadding;
MComputation escHandler;
float			rgain, rgamma;
MString		cancelFeedback = "Liquid -> RibGen Cancelled!\n";
bool		  doMotion;           		// Motion blur for transformations
bool		  doDef;              		// Motion blur for deforming objects
bool			justRib;
RtFloat   sampleTimes[5];					// current sample times
long      msampleOn;
liquidlong		  motionSamples;   				// used to assign more than two motion blur samples!
liquidlong      minCPU;
liquidlong			maxCPU;

double    cropX1, cropX2, cropY1, cropY2;


#ifdef _WIN32
int RiNColorSamples;
#endif

// these are little storage variables to keep track of the current graphics state and will eventually be wrapped in
// a specific class

bool			showProgress;
bool 			currentMatteMode;
char			*currentSurfaceShader;
char			*currentDisplacementShader;
bool      normalizeNurbsUV;
MString		lastRibName;
bool			renderSelected;
bool			exportReadArchive;
bool 			renderAllCurves;
bool			ignoreLights;
bool			ignoreSurfaces;
bool			ignoreDisplacements;
bool			ignoreVolumes;
bool 			outputShadowPass;
bool			outputHeroPass;	
bool			deferredGen;
bool			lazyCompute;
bool			outputShadersInShadows;
liquidlong			deferredBlockSize;
bool			outputComments;
bool			shaderDebug;
bool 			expandShaderArrays;
bool			doShadows;
bool			alfredExpand;
MSelectionList currentSelection;

long 			currentLiquidJobNumber;

/* BMRT PARAMS: BEGIN */
bool			useBMRT;
bool			BMRTusePrmanDisp;
bool			BMRTusePrmanSpec;
liquidlong			BMRTDStep;
liquidlong			RadSteps;
liquidlong			RadMinPatchSamples;
/* BMRT PARAMS: END */

MString		alfredTags;
MString		alfredServices;

MString		defGenKey;
MString		defGenService;

MString		preFrameMel;
MString		postFrameMel;

MStringArray preReadArchive;
MStringArray preRibBox;
MStringArray preReadArchiveShadow;
MStringArray preRibBoxShadow;

MString		renderCommand;
MString		ribgenCommand;
MString		preCommand;
MString		postJobCommand;
MString   preFrameCommand;
MString		postFrameCommand;
MString		shaderPath;
MString   userAlfredFileName;

/* Display Driver Variables */
struct structDDParam {
	liquidlong num;
	MStringArray names;
	MStringArray data;
	MIntArray type;
};

std::vector<structDDParam> DDParams;	

liquidlong		numDisplayDrivers;
MStringArray DDimageName;
MStringArray DDimageType;
MStringArray DDimageMode;
MStringArray DDparamType;

liquidlong rFilter;
float rFilterX, rFilterY;

extern int debugMode;

std::vector<shaderStruct> shaders;	

void freeShaders() 
{
	if ( debugMode ) { printf( "-> freeing shader data.\n" ); }
	std::vector<shaderStruct>::iterator iter = shaders.begin();
	while ( iter != shaders.end() )
	{
		int k = 0;
		while ( k < iter->numTPV ) {
			if ( iter->tokenPointerArray[k].tokenFloats != NULL ) {
				lfree( iter->tokenPointerArray[k].tokenFloats );
				iter->tokenPointerArray[k].tokenFloats = NULL;
			}
			if ( iter->tokenPointerArray[k].tokenString != NULL ) {
				lfree( iter->tokenPointerArray[k].tokenString );
				iter->tokenPointerArray[k].tokenString = NULL;
			}
			k++;
		}
		++iter;
	}
	shaders.clear();
	if ( debugMode ) { printf("-> finished freeing shader data.\n" ); }
}

MString RibTranslator::magic("##RenderMan");

void *RibTranslator::creator()
//
//  Description:
//      Create a new instance of the translator
//
{
    return new RibTranslator();
}

void RibTranslator::printProgress( int stat )
// Description:
// Member function for printing the progress to the 
// Maya Console or stdout.  If alfred is being used 
// it will print it in a format that causes the correct
// formatting for the progress meters
{ 
	float numFrames = ( frameLast - frameFirst ) + 1;
	float framesDone = lframe - frameFirst;
	float statSize = ( ( 1 / (float)numFrames ) / 4 ) * (float)stat * 100.0;
	float progressf = (( (float)framesDone / (float)numFrames ) * 100.0 ) + statSize;
	int progress = progressf;
	
	if ( !liquidBin ) {
		MString progressOutput = "Progress: ";
		progressOutput += (int)progress;
		progressOutput += "%";
		liquidInfo( progressOutput );
	} else {
		cout << "ALF_PROGRESS " << progress << "%\n" << flush;
	}
}					

bool RibTranslator::liquidInitGlobals()
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

RibTranslator::RibTranslator()
//
//  Description:
//      Class constructor
//
{
#ifdef _WIN32
	systemTempDirectory = getenv("TEMP");
#else
	systemTempDirectory = (char *)lmalloc( sizeof( char ) * 2048 );
	sprintf( systemTempDirectory, "/tmp/" );
#endif
	rFilterX = 1;
	rFilterY = 1;
	rFilter = fBoxFilter;
	
	currentLiquidJobNumber = 0;
	
	/* BMRT PARAMS: BEGIN */
	useBMRT = false;
	BMRTusePrmanDisp = true;
	BMRTusePrmanSpec = false;
	BMRTDStep = 0;
	RadSteps = 0;
	RadMinPatchSamples = 1;
	/* BMRT PARAMS: END */
	
	shortShaderNames = false;
	showProgress = false;
	deferredBlockSize = 1;
	deferredGen = false;
	isShadowPass = false;
	rgain = 1.0;
	rgamma = 1.0;
	outputHeroPass = true;
	outputShadowPass = false;
	ignoreLights = false;
	ignoreSurfaces = false;
	ignoreDisplacements = false;
	ignoreVolumes = false;
	renderAllCurves = false;
	renderSelected = false;
	exportReadArchive = false;
	normalizeNurbsUV = true;
	useNetRman = false;
	remoteRender = false;
    useAlfred = false;
    cleanRib = false;
	cleanAlf = false;
    doBinary = false;
    doCompression = false;
    doDof = false;
    outputpreview = 0;
    doMotion = false;		// matrix motion blocks
    doDef = false;			// geometry motion blocks
    doCameraMotion = false;		// camera motion blocks
    doExtensionPadding = 0; // pad the frame number in the rib file names
    doShadows = true;			// render shadows
		justRib = false;
    cleanShadows = 0;			// render shadows
    cleanTextures = 0;			// render shadows
    frameFirst = 1;			// range
    frameLast = 1;
    frameBy = 1;
    outPadding = 0;
    pixelSamples = 1;
    shadingRate = 1.0;
    depth = 1;
    outFormat = "it";
    animation = false;
    useFrameExt = true;		// Use frame extensions
    outExt = "tif";
    animExt = ".%0*d";
    riboutput = "liquid.rib";
    renderer = PRMan;
    motionSamples = 2;
    width = 360;
    height = 243;
    aspectRatio = 1.0;
    outPadding = 0;
    ignoreFilmGate = true;
    renderAllCameras = true;
	lazyCompute = false;
	outputShadersInShadows = false;
	alfredExpand = false;
    debugMode = 0;
    errorMode = 0;
    extension = ".rib";
    bucketSize[0] = 16;
	bucketSize[1] = 16;
    gridSize = 256;
    textureMemory = 2048;
    eyeSplits = 10;
	shutterTime = 0.5;
	blurTime = 1.0;
	fullShadowRib = false;
	baseShadowName = "";
	quantValue = 8;
    projectDir = systemTempDirectory;
    ribDir = "rib/";
    texDir = "rmantex/";
    pixDir = "rmanpix/";
	tmpDir = "rmantmp/";
	ribDirG.clear();
	texDirG.clear();
	pixDirG.clear();
	tmpDirG.clear();
	preReadArchive.clear();
	preRibBox.clear();
	alfredTags.clear();
	alfredServices.clear();
	defGenKey.clear();
	defGenService.clear();
	preFrameMel.clear();
	postFrameMel.clear();
	preCommand.clear();
	postJobCommand.clear();
	postFrameCommand.clear();
	preFrameCommand.clear();
	outputComments = false;
	shaderDebug = false;
	mayaVersion = MGlobal::mayaVersion();
#ifdef _WIN32
	renderCommand = "prman";
#else
	renderCommand = "render";
#endif
	ribgenCommand = "liquid";
	shaderPath = "&:.:~";
	currentSelection.clear();
	createOutputDirectories = true;
	
	expandShaderArrays = false;
	
	/* Display Driver Defaults */
	numDisplayDrivers = 0;
	DDParams.clear();
	DDimageName.clear();
	DDimageType.clear();
	DDimageMode.clear();
	DDparamType.clear();
	
	minCPU = maxCPU = 1;
	cropX1 = cropY1 = 0.0;
	cropX2 = cropY2 = 1.0;
	
}   

RibTranslator::~RibTranslator()
//  Description:
//      Class destructor
{
	lfree( systemTempDirectory );
	if ( debugMode ) { printf("-> dumping unfreed memory.\n" ); }
	if ( debugMode ) ldumpUnfreed();
}   

void RibTranslatorErrorHandler( RtInt code, RtInt severity, char * message )
//  Description:
//      Error handling function.  This gets called when the RIB library
//      detects an error.  
{
    printf( "The renderman library is reporting and error!" );
    MString error( message );
    throw error;
}

MStatus RibTranslator::liquidDoArgs( MArgList args ) 
{
//	Description:
//		Read the values from the command line and set the internal values

	int i;
	MStatus status;
	MString argValue;

	for ( i = 0; i < args.length(); i++ ) {
		if ( MString( "-p" ) == args.asString( i, &status ) )  {
			PERRORfail(status, "error in -p parameter");
			outputpreview = 1;
		} else if ( MString( "-nop" ) == args.asString( i, &status ) )  {
			PERRORfail(status, "error in -p parameter");
			outputpreview = 0;
		} else if ( MString( "-GL" ) == args.asString( i, &status ) )  {
			PERRORfail(status, "error in -GL parameter");
			//load up all the render global parameters!
			if ( liquidInitGlobals() ) liquidReadGlobals();
		} else if ( MString( "-sel" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -sel parameter");
			renderSelected = true;
		} else if ( MString( "-ra" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -ra parameter");
			exportReadArchive = true;
		} else if ( MString( "-allCurves" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -allCurves parameter" );
			renderAllCurves = true;
		} else if ( MString( "-tiff" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -tiff parameter");
			outFormat = "tiff";
		} else if ( MString( "-dofOn" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -dofOn parameter");
			doDof = true;
		} else if ( MString( "-doBinary" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -doBinary parameter");
			doBinary = true;
		} else if ( MString( "-shadows" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -doShadows parameter");
			doShadows = true;
		} else if ( MString( "-noshadows" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -noshadows parameter");
			doShadows = false;
		} else if ( MString( "-doCompression" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -doCompression parameter");
			doCompression = true;
		} else if ( MString( "-cleanRib" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -cleanRib parameter");
			cleanRib = true;
		} else if ( MString( "-progress" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -progress parameter");
			showProgress = true;
		} else if ( MString( "-mb" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -mb parameter");
			doMotion = true;
		} else if ( MString( "-db" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -db parameter");
			doDef = true;
		} else if ( MString( "-d" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -d parameter");
			debugMode = 1;
		} else if ( MString( "-netRman" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -netRman parameter");
			useNetRman = true;
		} else if ( MString( "-fullShadowRib" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -fullShadowRib parameter");
			fullShadowRib = true;
		} else if ( MString( "-compFor" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -compFor parameter");
			cout << "Compiled For: " << COMPFOR << "\n";
		} else if ( MString( "-remote" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -remote parameter");
			remoteRender = true;
		} else if ( MString( "-alfred" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -alfred parameter");
			useAlfred = true;
		} else if ( MString( "-noalfred" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -noalfred parameter");
			useAlfred = false;
		} else if ( MString( "-err" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -err parameter");
			errorMode = 1;
		} else if ( MString( "-shaderDB" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -shaderDB parameter");
			shaderDebug = true;
		} else if ( MString( "-n" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -n parameter");   i++;
			
			argValue = args.asString( i, &status );
			frameFirst = argValue.asInt();
			
			PERRORfail(status, "error in -n parameter");  i++;
			argValue = args.asString( i, &status );
			frameLast = argValue.asInt();
			
			PERRORfail(status, "error in -n parameter");  i++;
			argValue = args.asString( i, &status );
			frameBy = argValue.asInt();
			
			PERRORfail(status, "error in -n parameter");
			animation = true;
		} else if ( MString( "-m" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -m parameter");   i++;
			argValue = args.asString( i, &status );
			motionSamples = argValue.asInt();
			PERRORfail(status, "error in -m parameter");
		} else if ( MString( "-defBlock" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -defBlock parameter");   i++;
			argValue = args.asString( i, &status );
			deferredBlockSize = argValue.asInt();
			PERRORfail(status, "error in -defBlock parameter");
		} else if ( MString( "-cam" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -cam parameter");   i++;
			renderCamera = args.asString( i, &status );
			PERRORfail(status, "error in -cam parameter");
		} else if ( MString( "-s" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -s parameter");  i++;
			argValue = args.asString( i, &status );
			pixelSamples = argValue.asInt();
			PERRORfail(status, "error in -s parameter");
		} else if ( MString( "-ribName" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -ribName parameter");  i++;
			sceneName = args.asString( i, &status );
			PERRORfail(status, "error in -ribName parameter");
		} else if ( MString( "-od" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -od parameter");  i++;
			MString parsingString = args.asString( i, &status );
			projectDir = parseString( parsingString );
			if ( projectDir.substring( projectDir.length() - 1, projectDir.length() - 1 ) != "/" ) projectDir += "/";
			if ( !fileExists( projectDir ) ) { cout << "Liquid -> Cannot find /project dir, defaulting to system temp directory!\n" << flush;
			projectDir = systemTempDirectory; }
			ribDir = projectDir + "rib/";
			texDir = projectDir + "rmantex/";
			pixDir = projectDir + "rmanpix/";
			tmpDir = projectDir + "rmantmp/";
			PERRORfail(status, "error in -od parameter");
		} else if ( MString( "-preFrameMel" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -preFrameMel parameter");  i++;
			MString parsingString = args.asString( i, &status );
			preFrameMel = parseString( parsingString );
			PERRORfail(status, "error in -preFrameMel parameter");
		} else if ( MString( "-postFrameMel" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -postFrameMel parameter");  i++;
			MString parsingString = args.asString( i, &status );
			postFrameMel = parseString( parsingString );
			PERRORfail(status, "error in -postFrameMel parameter");
		} else if ( MString( "-ribdir" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -ribDir parameter");  i++;
			MString parsingString = args.asString( i, &status );
			ribDir = parseString( parsingString );
			PERRORfail(status, "error in -ribDir parameter");
		} else if ( MString( "-texdir" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -texDir parameter");  i++;
			MString parsingString = args.asString( i, &status );
			texDir = parseString( parsingString );
			PERRORfail(status, "error in -texDir parameter");
		} else if ( MString( "-tmpdir" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -tmpDir parameter");  i++;
			MString parsingString = args.asString( i, &status );
			tmpDir = parseString( parsingString );
			PERRORfail(status, "error in -tmpDir parameter");
		} else if ( MString( "-picdir" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -picDir parameter");  i++;
			MString parsingString = args.asString( i, &status );
			pixDir = parseString( parsingString );
			PERRORfail(status, "error in -picDir parameter");
		} else if ( MString( "-preCommand" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -preCommand parameter");  i++;
			preCommand = args.asString( i, &status );
			PERRORfail(status, "error in -preCommand parameter");
		} else if ( MString( "-postJobCommand" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -postJobCommand parameter");  i++;
			MString varVal = args.asString( i, &status );
			postJobCommand = parseString( varVal );
			PERRORfail(status, "error in -postJobCommand parameter");
		} else if ( MString( "-postFrameCommand" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -postFrameCommand parameter");  i++;
			postFrameCommand = args.asString( i, &status );
			PERRORfail(status, "error in -postFrameCommand parameter");
		} else if ( MString( "-preFrameCommand" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -preFrameCommand parameter");  i++;
			preFrameCommand = args.asString( i, &status );
			PERRORfail(status, "error in -preFrameCommand parameter");
		} else if ( MString( "-renderCommand" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -renderCommand parameter");  i++;
			renderCommand = args.asString( i, &status );
			PERRORfail(status, "error in -renderCommand parameter");
		} else if ( MString( "-ribgenCommand" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -ribgenCommand parameter");  i++;
			ribgenCommand = args.asString( i, &status );
			PERRORfail(status, "error in -ribgenCommand parameter");
		} else if ( MString( "-blurTime" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -blurTime parameter");  i++;
			argValue = args.asString( i, &status );
			blurTime = argValue.asDouble();
			PERRORfail(status, "error in -blurTime parameter");
		} else if ( MString( "-sr" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -sr parameter");  i++;
			argValue = args.asString( i, &status );
			shadingRate = argValue.asDouble();
			PERRORfail(status, "error in -sr parameter");
		} else if ( MString( "-bs" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -bs parameter");  i++;
			argValue = args.asString( i, &status );
			bucketSize[0] = argValue.asInt();
			PERRORfail(status, "error in -bs parameter");  i++;
			argValue = args.asString( i, &status );
			bucketSize[1] = argValue.asInt();
			PERRORfail(status, "error in -bs parameter");
		} else if ( MString( "-ps" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -ps parameter");  i++;
			argValue = args.asString( i, &status );
			rFilter = argValue.asInt();
			PERRORfail(status, "error in -ps parameter");  i++;
			argValue = args.asString( i, &status );
			rFilterX = argValue.asInt();
			PERRORfail(status, "error in -ps parameter");  i++;
			argValue = args.asString( i, &status );
			rFilterY = argValue.asInt();
			PERRORfail(status, "error in -ps parameter");
		} else if ( MString( "-gs" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -gs parameter");  i++;
			argValue = args.asString( i, &status );
			gridSize = argValue.asInt();
			PERRORfail(status, "error in -gs parameter");
		} else if ( MString( "-texmem" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -texmem parameter");  i++;
			argValue = args.asString( i, &status );
			textureMemory = argValue.asInt();
			PERRORfail(status, "error in -texmem parameter");
		} else if ( MString( "-es" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -es parameter");  i++;
			argValue = args.asString( i, &status );
			eyeSplits = argValue.asInt();
			PERRORfail(status, "error in -es parameter");
		} else if ( MString( "-aspect" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -aspect parameter");  i++;
			argValue = args.asString( i, &status );
			aspectRatio = argValue.asDouble();
			PERRORfail(status, "error in -aspect parameter");
		} else if ( MString( "-x" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -x parameter");  i++;
			argValue = args.asString( i, &status );
			width = argValue.asInt();
			PERRORfail(status, "error in -x parameter");
		} else if ( MString( "-y" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -y parameter");  i++;
			argValue = args.asString( i, &status );
			height = argValue.asInt();
			PERRORfail(status, "error in -y parameter");
		} else if ( MString( "-noDef" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -noDef parameter");  
			deferredGen = false;
		} else if ( MString( "-pad" ) == args.asString( i, &status ) ) {
			PERRORfail(status, "error in -pad parameter");  i++;
			argValue = args.asString( i, &status );
			outPadding = argValue.asInt();
			PERRORfail(status, "error in -pad parameter");
		} 
	}
	return MS::kSuccess;
}

void RibTranslator::liquidReadGlobals() 
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
			gPlug.getValue( numDisplayDrivers );
		}
		gStatus.clear();
		int k = 1;
		while ( k <= numDisplayDrivers ) {
			structDDParam parameters;
			MString varName;
			MString varVal;
			varName = "dd";
			varName += k;
			varName += "imageName";
			gPlug = rGlobalNode.findPlug( varName, &gStatus );
			if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
			gStatus.clear();
			DDimageName.append( varVal );
			varName = "dd";
			varName += k;
			varName += "imageType";
			gPlug = rGlobalNode.findPlug( varName, &gStatus );
			if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
			gStatus.clear();
			DDimageType.append( varVal );
			varName = "dd";
			varName += k;
			varName += "imageMode";
			gPlug = rGlobalNode.findPlug( varName, &gStatus ); 
			if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
			gStatus.clear();
			DDimageMode.append( varVal );
			varName = "dd";
			varName += k;
			varName += "paramType";
			gPlug = rGlobalNode.findPlug( varName, &gStatus );
			if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
			gStatus.clear();
			DDparamType.append( varVal );
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
			DDParams.push_back( parameters );
			k++;
		}
	}
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "shaderPath", &gStatus );
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		gStatus.clear();
		if ( varVal != "" ) {
			shaderPath += ":";
			shaderPath += parseString( varVal );
		}
	}
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "ribgenCommand", &gStatus );
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		if ( varVal != MString("") ) ribgenCommand = varVal;
		gStatus.clear();
	}
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "renderCommand", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		if ( varVal != MString("") ) renderCommand = varVal;
		gStatus.clear();
	}
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "alfredFileName", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		if ( varVal != MString("") ) userAlfredFileName = varVal;
		gStatus.clear();
	}
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "preCommand", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( preCommand );
		gStatus.clear();
	}
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "postJobCommand", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		postJobCommand = parseString( varVal );
		gStatus.clear();
	}
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "postFrameCommand", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( postFrameCommand );
		gStatus.clear();
	}
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "preFrameCommand", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( preFrameCommand );
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
			sceneName = parseString( varVal );
		}
	}
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "pictureDirectory", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		gStatus.clear();
		if ( varVal != "" ) {
			pixDirG = parseString( varVal );
		}
	}	
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "textureDirectory", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		gStatus.clear();
		if ( varVal != "" ) {
			texDirG = parseString( varVal );
		}
	}	
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "tempDirectory", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		gStatus.clear();
		if ( varVal != "" ) {
			tmpDirG = parseString( varVal );
		}
	}	
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "ribDirectory", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		gStatus.clear();
		if ( varVal != "" ) {
			ribDirG = parseString( varVal );
		}
	}
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "alfredTags", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		gStatus.clear();
		if ( varVal != "" ) {
			alfredTags = parseString( varVal );
		}
	}	
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "alfredServices", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		gStatus.clear();
		if ( varVal != "" ) {
			alfredServices = parseString( varVal );
		}
	}	
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "key", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		gStatus.clear();
		if ( varVal != "" ) {
			defGenKey = parseString( varVal );
		}
	}	
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "service", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		gStatus.clear();
		if ( varVal != "" ) {
			defGenService = parseString( varVal );
		}
	}	
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "preframeMel", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		gStatus.clear();
		if ( varVal != "" ) {
			preFrameMel = parseString( varVal );
		}
	}	
	{ 
		MString varVal;
		gPlug = rGlobalNode.findPlug( "postframeMel", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( varVal );
		gStatus.clear();
		if ( varVal != "" ) {
			postFrameMel = parseString( varVal );
		}
	}	
				
	/* PIXELFILTER OPTIONS: BEGIN */
	gPlug = rGlobalNode.findPlug( "PixelFilter", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( rFilter );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "PixelFilterX", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( rFilterX );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "PixelFilterY", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( rFilterY );
	gStatus.clear();
	/* PIXELFILTER OPTIONS: END */
	
	/* BMRT OPTIONS:BEGIN */
	gPlug = rGlobalNode.findPlug( "BMRTAttrs", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( useBMRT );
	gStatus.clear();
	if ( useBMRT ) {
		renderCommand = "rendrib ";
	}
	gPlug = rGlobalNode.findPlug( "BMRTDStep", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( BMRTDStep );
	gStatus.clear();
	if ( BMRTDStep > 0 ) {
		renderCommand += "-d ";
		renderCommand += (int)BMRTDStep;
		renderCommand += " ";
	}
	gPlug = rGlobalNode.findPlug( "RadSteps", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( RadSteps );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "RadMinPatchSamples", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( RadMinPatchSamples );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "BMRTusePrmanSpec", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( BMRTusePrmanSpec );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "BMRTusePrmanDisp", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( BMRTusePrmanSpec );
	gStatus.clear();
	/* BMRT OPTIONS:END */
		
	MStatus cropStatus;
	gPlug = rGlobalNode.findPlug( "cropX1", &cropStatus );
	gStatus.clear();
	if ( cropStatus == MS::kSuccess ) {
		gPlug = rGlobalNode.findPlug( "cropX1", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( cropX1 );
		gStatus.clear();
		gPlug = rGlobalNode.findPlug( "cropX2", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( cropX2 );
		gStatus.clear();
		gPlug = rGlobalNode.findPlug( "cropY1", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( cropY1 );
		gStatus.clear();
		gPlug = rGlobalNode.findPlug( "cropY2", &gStatus ); 
		if ( gStatus == MS::kSuccess ) gPlug.getValue( cropY2 );
		gStatus.clear();
	}
				
	gPlug = rGlobalNode.findPlug( "shortShaderNames", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( shortShaderNames );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "expandAlfred", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( alfredExpand );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "createOutputDirectories", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( createOutputDirectories );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "minCPU", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( minCPU );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "maxCPU", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( maxCPU );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "showProgress", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( showProgress );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "expandShaderArrays", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( expandShaderArrays );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "outputComments", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( outputComments );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "shaderDebug", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( shaderDebug );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "deferredGen", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( deferredGen );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "deferredBlock", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( deferredBlockSize );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "ignoreAlfred", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( useAlfred );
	gStatus.clear();
	useAlfred = !useAlfred;
	gPlug = rGlobalNode.findPlug( "remoteRender", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( remoteRender );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "renderAllCurves", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( renderAllCurves );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "ignoreLights", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( ignoreLights );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "ignoreSurfaces", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( ignoreSurfaces );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "ignoreDisplacements", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( ignoreDisplacements );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "ignoreVolumes", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( ignoreVolumes );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "outputShadowPass", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( outputShadowPass );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "outputHeroPass", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( outputHeroPass );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "netRManRender", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( useNetRman );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "ignoreShadows", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( doShadows );
	gStatus.clear();
	doShadows = !doShadows;
	gPlug = rGlobalNode.findPlug( "fullShadowRibs", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( fullShadowRib );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "binaryOutput", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( doBinary );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "lazyCompute", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( lazyCompute );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "outputShadersInShadows", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( outputShadersInShadows );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "compressedOutput", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( doCompression );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "exportReadArchive", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( exportReadArchive );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "alfredJobName", &gStatus );
	if ( gStatus == MS::kSuccess ) gPlug.getValue( alfredJobName );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "doAnimation", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( animation );
	gStatus.clear();
	if ( animation ) {
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
		if ( gStatus == MS::kSuccess ) gPlug.getValue( outPadding );
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
	if ( gStatus == MS::kSuccess ) gPlug.getValue( doMotion );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "transformationBlur", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( doCameraMotion );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "deformationBlur", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( doDef );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "motionBlurSamples", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( motionSamples );
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
	if ( gStatus == MS::kSuccess ) gPlug.getValue( justRib );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "imageDepth", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( quantValue );
	gStatus.clear();
	
	gPlug = rGlobalNode.findPlug( "gain", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( rgain );
	gStatus.clear();
	gPlug = rGlobalNode.findPlug( "gamma", &gStatus ); 
	if ( gStatus == MS::kSuccess ) gPlug.getValue( rgamma );
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

MStatus RibTranslator::doIt( const MArgList& args )
//  Description:
//      This method actually does the renderman output 
{
	MStatus status;
	
	// Parse the arguments and set the options.
	if ( args.length() == 0 ) {
		liquidInfo( "Doing nothing, no parameters given." );
		return MS::kFailure;
	}

	if ( !liquidBin ) liquidInfo("Creating Rib <Press ESC To Cancel> ...");
	
	// find the activeView for previews;
	activeView = M3dView::active3dView();
	width = activeView.portWidth();
	height = activeView.portHeight();
	
	//find out the current selection for possible selected object output
	MGlobal::getActiveSelectionList( currentSelection );
	
	// get the current project directory
	MString MELCommand = "workspace -q -rd";
	MString MELReturn;
	MGlobal::executeCommand( MELCommand, MELReturn );
	projectDir = MELReturn ;
	
	if ( debugMode ) { printf("-> using path: %s\n", projectDir.asChar() ); }
	
	// get the current scene name
	sceneName = liquidTransGetSceneName();
	
	// Remember the frame the scene was at so we can restore it later.
	MTime currentFrame = MAnimControl::currentTime();
	
	// setup default animation parameters
	frameFirst = MAnimControl::currentTime().as( MTime::uiUnit() );
	frameLast = MAnimControl::currentTime().as( MTime::uiUnit() );
	frameBy = 1;
	
	// check to see if the correct project directory was found
	if ( !fileExists( projectDir ) ) projectDir = systemTempDirectory;
	if ( projectDir.substring( projectDir.length() - 1, projectDir.length() - 1 ) != "/" ) projectDir += "/";
	if ( !fileExists( projectDir ) ) { cout << "Liquid -> Cannot find /project dir, defaulting to system temp directory!\n" << flush;
	projectDir = systemTempDirectory; }
	ribDir = projectDir + "rib/";
	texDir = projectDir + "rmantex/";
	pixDir = projectDir + "rmanpix/";
	tmpDir = projectDir + "rmantmp/";

	liquidDoArgs( args );

	// append the progress flag for alfred feedback
	if ( useAlfred ) {
		if ( ( renderCommand == MString( "render" ) ) || ( renderCommand == MString( "prman" ) ) ) renderCommand = renderCommand + " -Progress";
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
	if ( ribDirG.length() > 0 ) ribDir = ribDirG;
	if ( texDirG.length() > 0 ) texDir = texDirG;
	if ( pixDirG.length() > 0 ) pixDir = pixDirG;
	if ( tmpDirG.length() > 0 ) tmpDir = tmpDirG;
	
	// make sure the directories end with a slash
	if ( ribDir.substring( ribDir.length() - 1, ribDir.length() - 1 ) != "/" ) ribDir += "/";
	if ( texDir.substring( texDir.length() - 1, texDir.length() - 1 ) != "/" ) texDir += "/"; 
	if ( pixDir.substring( pixDir.length() - 1, pixDir.length() - 1 ) != "/" ) pixDir += "/";
	if ( tmpDir.substring( tmpDir.length() - 1, tmpDir.length() - 1 ) != "/" ) tmpDir += "/"; 
	
	// setup the error handler
	if ( errorMode ) RiErrorHandler( RibTranslatorErrorHandler );
	
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
		alfredRemoteTagsAndServices += alfredServices.asChar();
		alfredRemoteTagsAndServices += MString( " } -tags { " );
		alfredRemoteTagsAndServices += alfredTags.asChar();
		alfredRemoteTagsAndServices += MString( " } " );
	}

	/*  A seperate one for cleanup as it doens't need a tag! */
	MString alfredCleanupRemoteTagsAndServices;
	if ( remoteRender || useNetRman ) {
		alfredCleanupRemoteTagsAndServices = MString( "-service { " );
		alfredCleanupRemoteTagsAndServices += alfredServices.asChar();
		alfredCleanupRemoteTagsAndServices += MString( " } " );
	}
		
	// exception handling block, this tracks liquid for any possible errors and tries to catch them
	// to avoid crashing
	try {
		escHandler.beginComputation();
		
#ifndef _WIN32
		mode_t dirMode;
		dirMode = S_IRWXU|S_IRWXG|S_IRWXO;
#endif
		// check to see if all the directories we are working with actually exist. 	
		if ( ( access( ribDir.asChar(), 0 )) == -1 ) {
			if ( createOutputDirectories ) {
					if ( mkdir( ribDir.asChar(), dirMode ) != 0 ) {
						printf( "Liquid -> had trouble creating rib dir, defaulting to system temp directory!\n" );
						ribDir = systemTempDirectory; 
					}
				} else {
					printf( "Liquid -> Cannot find /rib dir, defaulting to system temp directory!\n" );
					ribDir = systemTempDirectory; 
				}
			}
			if ( (access( texDir.asChar(), 0 )) == -1 ) { 
				if ( createOutputDirectories ) {
						if ( mkdir( texDir.asChar(), dirMode ) != 0 ) {
							printf( "Liquid -> had trouble creating tex dir, defaulting to system temp directory!\n" );
							texDir = systemTempDirectory; 
						}
					} else {
						printf( "Liquid -> Cannot find /tex dir, defaulting to system temp directory!\n" );
						texDir = systemTempDirectory; 
					}
				}
				if ( (access( pixDir.asChar(), 0 )) == -1 ) { 
					if ( createOutputDirectories ) {
							if ( mkdir( pixDir.asChar(), dirMode ) != 0 ) {
								printf( "Liquid -> had trouble creating pix dir, defaulting to system temp directory!\n" );
								pixDir = systemTempDirectory; 
							}
						} else {
							printf( "Liquid -> Cannot find /pix dir, defaulting to system temp directory!\n" );
							pixDir = systemTempDirectory; 
						}
					}
					if ( (access( tmpDir.asChar(), 0 )) == -1 ) { 
						if ( createOutputDirectories ) {
								if ( mkdir( tmpDir.asChar(), dirMode ) != 0 ) {
									printf( "Liquid -> had trouble creating tmp dir, defaulting to system temp directory!\n" );
									tmpDir = systemTempDirectory; 
								}
							} else {
								printf( "Liquid -> Cannot find /tmp dir, defaulting to system temp directory!\n" );
								tmpDir = systemTempDirectory; 
							}
						}
						if ( !fileExists( preFrameMel ) && ( preFrameMel != "" ) ) {
							cout << "Liquid -> Cannot find pre frame mel script! Assuming local.\n" << flush;
						}
						if ( !fileExists( postFrameMel ) && ( postFrameMel != "" ) ) { 
							cout << "Liquid -> Cannot find post frame mel script! Assuming local.\n" << flush;
						}
						
						//BMRT didn't support deformation blur at the time of this writing
						if (renderer == BMRT) {
							doDef = false;
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
						MString tempAlfname = tmpDir;
						if ( userAlfredFileName != MString( "" ) ){
							tempAlfname += userAlfredFileName;
						} else {
							tempAlfname += sceneName;
#ifndef _WIN32
							gettimeofday( &t_time, &t_zone );
							srandom( t_time.tv_usec + hashVal );
							short alfRand = random();
							tempAlfname += alfRand;
#endif
						}
						tempAlfname += ".alf";
						
						alfredFileName = (char *)alloca( sizeof( char ) * tempAlfname.length() );
						sprintf( alfredFileName , tempAlfname.asChar() );
						
						// build deferred temp maya file name
						MString tempDefname = tmpDir;
						tempDefname += sceneName;
#ifndef _WIN32
						gettimeofday( &t_time, &t_zone );
						srandom( t_time.tv_usec + hashVal );
						short defRand = random();
						tempDefname += defRand;
#endif
						
						MString currentFileType = MFileIO::fileType();
						if ( MString( "mayaAscii" ) == currentFileType ) tempDefname += ".ma";
						if ( MString( "mayaBinary" ) == currentFileType ) tempDefname += ".mb";
						if ( deferredGen ) {
							MFileIO::exportAll( tempDefname, currentFileType.asChar() );
						}
						
						if ( !deferredGen && justRib ) useAlfred = false;
						
						if ( useAlfred ) {
							alfFile.open( alfredFileName );
							
							//write the little header information alfred needs
							alfFile << "##AlfredToDo 3.0" << "\n";
							
							//write the main job info
							if ( alfredJobName == "" ) { alfredJobName = sceneName; }
							alfFile << "Job -title {" << alfredJobName.asChar() << "(liquid job)} -comment {#Created By Liquid " << VERSION << "} "
								<< "-service " << "{}" << " " 
								<< "-tags " << "{}" << " "; 
							if ( useNetRman ) {
								alfFile << "-atleast " << minCPU << " " 
									<< "-atmost " << maxCPU << " "; 
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
						for( lframe=frameFirst; lframe<=frameLast; lframe = lframe + frameBy ) {
							if ( showProgress ) printProgress( 1 );
							
							shadowRibGen = false;
							alfShadowRibGen = false;
							preReadArchive.clear();
							preRibBox.clear();
							preReadArchiveShadow.clear();
							preRibBoxShadow.clear();
							
							/* make sure all the global strings are parsed for this frame */
							MString frameRenderCommand = parseString( renderCommand );
							MString frameRibgenCommand = parseString( ribgenCommand );
							MString framePreCommand = parseString( preCommand );
							MString framePreFrameCommand = parseString( preFrameCommand );
							MString framePostFrameCommand = parseString( postFrameCommand );
							
							if ( useAlfred ) {
								if ( deferredGen ) {
									if ( (( lframe - frameFirst ) % deferredBlockSize ) == 0 ) {
										if ( deferredBlockSize == 1 ) {
											currentBlock = lframe;
										} else { 
											currentBlock++;
										}
										alfFile << "	Task -title {" << sceneName.asChar() << "FrameRIBGEN" << currentBlock << "} -subtasks {\n";
										alfFile << "			} -cmds { \n";
										if ( remoteRender ) {
											alfFile << "			      RemoteCmd { " << framePreCommand.asChar() << "  "; 
											alfFile << frameRibgenCommand.asChar(); 
										} else {
											alfFile << "			      Cmd { " << framePreCommand.asChar() << "  "; 
											alfFile << frameRibgenCommand.asChar(); 
										}
										int lastGenFrame = ( lframe + ( deferredBlockSize - 1 ) );
										if ( lastGenFrame > frameLast ) lastGenFrame = frameLast;
										alfFile << " -progress -noDef -nop -noalfred -od " << projectDir.asChar() << " -ribName " << sceneName.asChar() << " -mf " << tempDefname.asChar() << " -n " << lframe << " " << lastGenFrame << " " << frameBy << " } ";
										if ( alfredExpand ) {
											alfFile	<< "-expand 1 ";
										}
										alfFile << "-service { " << defGenService.asChar() << " } ";
										alfFile	<< "-tags { " << defGenKey.asChar() << " }\n";
										alfFile << "			}\n";
									}
								}
								if ( !justRib ) {
									alfFile << "	Task -title {" << sceneName.asChar() << "Frame" << lframe << "} -subtasks {\n";
									
									if ( deferredGen ) {
										alfFile << "		Instance {" << sceneName.asChar() << "FrameRIBGEN" << currentBlock << "}\n";
									}
								
									alfFile << flush;
								}
							}
							
							if ( buildJobs() != MS::kSuccess ) break;
							
							{
								baseShadowName = ribDir;
								if ( ( DDimageName[0] == "" ) ) {
									baseShadowName += sceneName; 
								} else {
									char *mydot = ".";
									int pointIndex = DDimageName[0].index( *mydot );
									baseShadowName += DDimageName[0].substring(0, pointIndex-1).asChar();
								}
								baseShadowName += "_SHADOWBODY";
								baseShadowName += animExt;
								baseShadowName += extension;
								size_t shadowNameLength = baseShadowName.length() + 1;
								shadowNameLength += 10;
								char *baseShadowRibName;
								baseShadowRibName = (char *)alloca(shadowNameLength);
								sprintf(baseShadowRibName, baseShadowName.asChar(), doExtensionPadding ? outPadding : 0, lframe);
								baseShadowName = baseShadowRibName;
							}
							
							if ( !deferredGen ) {
								
								htable = new RibHT();
								
								float sampleinc = ( shutterTime * blurTime ) / ( motionSamples - 1 );
								for ( msampleOn = 0; msampleOn < motionSamples; msampleOn++ ) {
									float subframe = ( lframe - ( shutterTime * blurTime * 0.5 ) ) + msampleOn * sampleinc;
									sampleTimes[ msampleOn ] = subframe;
								}
								
								if ( doMotion || doDef ) {
									for ( msampleOn = 0; msampleOn < motionSamples; msampleOn++ ) {
										scanScene( sampleTimes[ msampleOn ] , msampleOn );
									}
								} else {
									sampleTimes[ 0 ] = lframe;
									scanScene( lframe, 0 );
								}
								if ( showProgress ) printProgress( 2 );
								
								std::vector<structJob>::iterator iter = jobList.begin();
								for (; iter != jobList.end(); ++iter ) {
									if ( currentJob.isShadow && !doShadows ) continue;
									currentMatteMode = false;
									currentSurfaceShader = NULL;
									currentDisplacementShader = NULL;
									currentJob = *iter;
									activeCamera = currentJob.path;
									if ( currentJob.isShadowPass ) {
										isShadowPass = true;
										ignoreSurfaces = true;
									} else {
										isShadowPass = false;
									}
									
									if ( debugMode ) { printf("-> setting RiOptions\n"); }
									
									// Rib client file creation options MUST be done before RiBegin
									if ( debugMode ) { printf("-> setting binary option\n"); }
									if ( doBinary )
									{
										RtString format = "binary"; 
										RiOption(( RtToken ) "rib", ( RtToken ) "format", ( RtPointer )&format, RI_NULL);
									} else {
										RtString format = "ascii";
										RiOption(( RtToken ) "rib", ( RtToken ) "format", ( RtPointer )&format, RI_NULL);
									}
									
									if ( debugMode ) { printf("-> setting compression option\n"); }
									if ( doCompression )
									{
										RiOption(( RtToken ) "rib", ( RtToken ) "compression", "gzip", RI_NULL);
									} else {
										RiOption(( RtToken ) "rib", ( RtToken ) "compression", "none", RI_NULL);
									}
									
									// world RiReadArchives and Rib Boxes
									
									if ( currentJob.isShadow && !shadowRibGen && !fullShadowRib ) {
										ribFP = fopen( baseShadowName.asChar(), "w" );
										if ( ribFP ) {
											if ( debugMode ) { printf("-> setting pipe option\n"); }
											RtInt ribFD = fileno( ribFP );
											RiOption( ( RtToken )"rib", ( RtToken )"pipe", &ribFD, RI_NULL );
										}
										if ( debugMode ) { printf("-> beginning rib output\n"); }
										RiBegin(RI_NULL);
										if (   frameBody() != MS::kSuccess ) break;
										RiEnd();
										fclose( ribFP );
										shadowRibGen = true;
										alfShadowRibGen = true;
									}	
									
									ribFP = fopen( currentJob.ribFileName.asChar(), "w" );
									if ( ribFP ) {
										RtInt ribFD = fileno( ribFP );
										RiOption( ( RtToken )"rib", ( RtToken )"pipe", &ribFD, RI_NULL );
									} else {
										cerr << ( "Error opening rib - writing to stdout.\n" );
									}
									
									RiBegin( RI_NULL );
									
									if ( currentJob.isShadow && !fullShadowRib && shadowRibGen ) {
										if ( ribPrologue() == MS::kSuccess ) {
											if ( framePrologue( lframe ) != MS::kSuccess ) break;
											RiArchiveRecord( RI_COMMENT, "Read Archive Data: \nReadArchive \"%s\"", baseShadowName.asChar() );
											if ( frameEpilogue( lframe ) != MS::kSuccess ) break;
											ribEpilogue();
										}
									} else { 
										if ( ribPrologue() == MS::kSuccess ) {
											if ( framePrologue( lframe ) != MS::kSuccess ) break;
											if ( frameBody() != MS::kSuccess ) break;
											if ( frameEpilogue( lframe ) != MS::kSuccess ) break;
											ribEpilogue();
										}			
									}
									
									RiEnd();
									fclose( ribFP );
									if ( showProgress ) printProgress( 3 );
				}
				
				delete htable;
				freeShaders();
				htable = NULL; 
			}
			
			// now we re-iterate through the job list to write out the alfred file if we are using it
			
			// write out shadows
			
			if ( useAlfred && !justRib ) {
				if ( doShadows ) {
					if ( debugMode ) { printf("-> writing out shadow information to alfred file.\n" ); }
					std::vector<structJob>::iterator iter = shadowList.begin();
					while ( iter != shadowList.end() ) {
						alfFile << "	      Task -title {" << iter->name.asChar() << "} -subtasks {\n";
						if ( deferredGen ) {
							alfFile << "		Instance {" << sceneName.asChar() << "FrameRIBGEN" << currentBlock << "}\n";
						}
						alfFile << "			} -cmds { \n";
						if ( useNetRman ) {
							alfFile << "			      Cmd { " << framePreCommand.asChar() << " netrender %H -Progress " << iter->ribFileName.asChar() << "} ";
						} else if ( remoteRender ) {
								alfFile << "			      RemoteCmd { " << framePreCommand.asChar() << "  " << frameRenderCommand.asChar() << " " << iter->ribFileName.asChar() << "} ";
						} else {
							alfFile << "			      Cmd { " << framePreCommand.asChar() << "  " << frameRenderCommand.asChar() << " " << iter->ribFileName.asChar() << "} ";
						}
						if ( alfredExpand ) {
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
						if ( !alfShadowRibGen && !fullShadowRib ) alfShadowRibGen = true;
					}
				}
				if ( debugMode ) { printf("-> finished writing out shadow information to alfred file.\n" ); }
				
				if ( debugMode ) { printf("-> initiating hero pass information.\n" ); }
				structJob *frameJob;
				structJob *shadowPassJob;
				if ( debugMode ) { printf("-> setting hero pass.\n" ); }
				if ( outputHeroPass && !outputShadowPass ) {
					frameJob = &jobList[ jobList.size() - 1 ];
				} else if ( outputShadowPass && outputHeroPass ) {
					frameJob = &jobList[ jobList.size() - 1 ];
					shadowPassJob = &jobList[ jobList.size() - 2 ];
				} else if ( outputShadowPass && !outputHeroPass ) {
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

				if ( outputHeroPass ) {
					if ( useNetRman ) {
						alfFile << "			      Cmd { " << framePreCommand.asChar() << "  netrender %H -Progress " << frameJob->ribFileName.asChar() << "} ";
					} else if ( remoteRender ) {
						alfFile << "			      RemoteCmd { " << framePreCommand.asChar() << "  " << frameRenderCommand.asChar() << " " << frameJob->ribFileName.asChar() << "} ";
					} else {
						alfFile << "			      Cmd { " << framePreCommand.asChar() << "  " << frameRenderCommand.asChar() << " " << frameJob->ribFileName.asChar() << "} ";
					}
					if ( alfredExpand ) {
						alfFile	<< "-expand 1 ";
					}
					alfFile << alfredRemoteTagsAndServices.asChar() << "\n";

				}
				if ( debugMode ) { printf("-> finished writing out hero information to alfred file.\n" ); }
				alfFile << flush;
				
				if ( outputShadowPass ) {
					if ( useNetRman ) {
						alfFile << "			      Cmd { " << framePreCommand.asChar() << "  netrender %H -Progress " << shadowPassJob->ribFileName.asChar() << "} ";
					} else if ( remoteRender ) {
						alfFile << "			      RemoteCmd { " << framePreCommand.asChar() << "  " << frameRenderCommand.asChar() << " " << shadowPassJob->ribFileName.asChar() << "} ";
					} else {
						alfFile << "			      Cmd { " << framePreCommand.asChar() << "  " << frameRenderCommand.asChar() << " " << shadowPassJob->ribFileName.asChar() << "} ";
					}
					if ( alfredExpand ) {
						alfFile	<< "-expand 1 ";
					}
					alfFile << alfredRemoteTagsAndServices.asChar() << "\n";

				}		
				alfFile << flush;
				
				if (cleanRib || (framePostFrameCommand != MString("")) ) { 
					alfFile << "	} -cleanup {\n";
					
					if (cleanRib) { 
						
						if ( outputHeroPass ) alfFile << "	      " << alfredCleanUpCommand.asChar() << " { " << framePreCommand.asChar() << "  /bin/rm " << frameJob->ribFileName.asChar() << "} " << alfredCleanupRemoteTagsAndServices.asChar() << "\n"; 
						if ( outputShadowPass ) alfFile << "	      " << alfredCleanUpCommand.asChar() << " { " << framePreCommand.asChar() << "  /bin/rm " << shadowPassJob->ribFileName.asChar() << "} " << alfredCleanupRemoteTagsAndServices.asChar() << "\n"; 
						if ( alfShadowRibGen ) alfFile << "	      " << alfredCleanUpCommand.asChar() << " { " << framePreCommand.asChar() << "  /bin/rm " << baseShadowName.asChar() << "} " << alfredCleanupRemoteTagsAndServices.asChar() << "\n";
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
				if ( outputHeroPass ) alfFile << "				sho \"" << frameJob->imageName.asChar() << "\"\n";
				if ( outputShadowPass ) alfFile << "				sho \"" << shadowPassJob->imageName.asChar() << "\"\n";
				alfFile << "	}\n";
				if ( outputShadowPass && !outputHeroPass ) {
					lastRibName = shadowPassJob->ribFileName.asChar();
				} else {
					lastRibName = frameJob->ribFileName.asChar();
				}
			}
			
			if ( ( ribStatus != kRibOK ) && !deferredGen ) break;
		}
		
		if ( useAlfred ) {
			// clean up the alfred file in the future
			if ( !justRib ) {
				alfFile << "} -cleanup { \n";
				if ( deferredGen ) {
					alfFile << "" << alfredCleanUpCommand.asChar() << " { /bin/rm " << tempDefname.asChar() << "} " << alfredCleanupRemoteTagsAndServices.asChar() << "\n";
				}
				if ( cleanAlf ) {
					alfFile << "" << alfredCleanUpCommand.asChar() << " { /bin/rm " << alfredFileName << "} " << alfredCleanupRemoteTagsAndServices.asChar() << "\n";
				}
				if ( postJobCommand != MString("") ) {
					if ( useNetRman ) {
						alfFile << "				  Cmd { " << postJobCommand.asChar() << "}\n";
					} else if ( remoteRender ) {
						alfFile << "				  RemoteCmd { " << postJobCommand.asChar() << "}\n";
					} else {
						alfFile << "				  Cmd { " << postJobCommand.asChar() << "}\n";
					}
				}
			}
			alfFile << "}\n";
			alfFile.close();
		}
		
		if ( debugMode ) { printf("-> ending escape handler.\n" ); }
		escHandler.endComputation();
		
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
		if ( useAlfred && ( outputpreview == 1 )) {
#ifdef _LINUX
			if (fork() == 0) execlp( "alfred", "alfred", alfredFileName, NULL);
#endif
#ifdef _IRIX
			pcreatelp( "alfred", "alfred", alfredFileName, NULL); 
#endif
		} else if ( outputpreview == 1 ) {	   
			// if we are previewing the scene spawn the preview
#ifdef _LINUX
			if (vfork() == 0) execlp( renderCommand.asChar(), renderCommand.asChar(), currentJob.ribFileName.asChar(), NULL);
#endif
#ifdef _IRIX
			pcreatelp( renderCommand.asChar(), renderCommand.asChar(), currentJob.ribFileName.asChar(), NULL);
#endif
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
		escHandler.endComputation();
		return MS::kFailure;
	} catch ( ... ) {
		cerr << "RIB Export: Unknown exception thrown\n" << endl;
		if ( NULL != htable ) delete htable;
		freeShaders();
		if ( debugMode ) ldumpUnfreed();
		escHandler.endComputation();
		return MS::kFailure;
	} 
}

void RibTranslator::portFieldOfView( int port_width, int port_height,
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

void RibTranslator::computeViewingFrustum ( double     window_aspect,
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

void RibTranslator::getCameraInfo( MFnCamera& cam )
//
//  Description:
//      Get information about the given camera
//
{
	// Resoultion can change if camera film-gate clips image
	// so we must keep camera width/height separate from render
	// globals width/height.
	//
	cam_width = width;
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
				cam_height = new_height;
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
			cam_width = new_width;
			fov_ratio = 1.0;		
		}
		
		// case 2 : film-gate smaller than resolution
		//	        film-gate off
		else if ( (new_width < cam_width) && (ignoreFilmGate) ) {
			portFieldOfView( new_width, cam_height, hfov, vfov, cam );
			fov_ratio = hfov/vfov;		
		}
		
		// case 3 : film-gate larger than resolution
		//	        film-gate on
		else if ( !ignoreFilmGate ) {
			portFieldOfView( new_width, cam_height, hfov, vfov, cam );
			fov_ratio = hfov/vfov;		
		}
		
		// case 4 : film-gate larger than resolution
		//	        film-gate off
		else if ( ignoreFilmGate ) {
			portFieldOfView( new_width, cam_height, hfov, vfov, cam );
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
				cam_width = new_width;
				fov_ratio = 1.0;
			}
			else {
				double hfov, vfov;
				portFieldOfView( new_width, cam_height, hfov, vfov, cam );
				fov_ratio = hfov/vfov;		
			}
		}
		else {
			if ( !ignoreFilmGate )
				cam_height = new_height;
			
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
			portFieldOfView( new_width, cam_height, hfov, vfov, cam );
			fov_ratio = hfov/vfov;
		}
		else {						
			portFieldOfView( cam_width, cam_height, hfov, vfov, cam );
			fov_ratio = hfov/vfov;
		}
	}
}

MStatus RibTranslator::buildJobs()
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
	
    if ( doShadows ) {
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
					if ( lazyCompute ) {
						MString baseFileName;
						if ( ( DDimageName[0] == "" ) ) {
							baseFileName = sceneName; 
						} else {
							char *mydot = ".";
							int pointIndex = DDimageName[0].index( *mydot );
							baseFileName = DDimageName[0].substring(0, pointIndex-1).asChar();
						} 
						baseFileName += "_";
						baseFileName += thisJob.name;
						baseFileName += "SHD";
						
						MString outFileFmtString;
						outFileFmtString = texDir;
						outFileFmtString += baseFileName;
						
						size_t outNameLength;
						
						outNameLength = outFileFmtString.length();
						
						if (animation || useFrameExt) {
							outFileFmtString += animExt;
							outNameLength += outPadding + 1;
						}
						outFileFmtString += ".";
						outFileFmtString += "tex";
						outNameLength = outFileFmtString.length();
						outNameLength = outNameLength + 8; // Space for the null character
						
						char *outName = (char *)alloca(outNameLength);
						sprintf(outName, outFileFmtString.asChar(), 1, lframe);
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
						if ( ( DDimageName[0] == "" ) ) {
							baseFileName = sceneName; 
						} else {
							char *mydot = ".";
							int pointIndex = DDimageName[0].index( *mydot );
							baseFileName = DDimageName[0].substring(0, pointIndex-1).asChar();
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
											outFileFmtString = texDir;
											outFileFmtString += baseFileName;
											
											size_t outNameLength;
											
											outNameLength = outFileFmtString.length();
											
											if (animation || useFrameExt) {
												outFileFmtString += animExt;
												outNameLength += outPadding + 1;
											}
											outFileFmtString += ".";
											outFileFmtString += "tex";
											outNameLength = outFileFmtString.length();
											outNameLength = outNameLength + 8; // Space for the null character
											
											char *outName = (char *)alloca(outNameLength);
											sprintf(outName, outFileFmtString.asChar(), 1, lframe);
											MString fileName( outName );
											if ( lazyCompute ) {
												if ( fileExists( fileName ) ) computeShadow = false;
											}
											if ( computeShadow ) jobList.push_back( thisJob );
					}
				}
			} // useDepthMap
		} // dagIterator
	} // doShadows
	
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
		activeView.getCamera( cameraPath );
    }
	MFnCamera fnCameraNode( cameraPath );
	thisJob.isPoint = false;
	thisJob.path = cameraPath;
	thisJob.name = fnCameraNode.name();
	thisJob.isShadow = false;
	thisJob.name += "SHADOWPASS";
	thisJob.isShadowPass = true;
	if ( outputShadowPass ) jobList.push_back( thisJob );
	thisJob.name = fnCameraNode.name();
	thisJob.isShadowPass = false;
	if ( outputHeroPass ) jobList.push_back( thisJob );
	shutterTime = fnCameraNode.shutterAngle() * 0.5 / M_PI;
	
    // If we didn't find a renderable camera then give up
    if ( jobList.size() == 0 ) {
		MString cError("No Renderable Camera Found!\n");
		throw( cError );
		return MS::kFailure; 
    }
	
	// step through the jobs and setup their names
	std::vector<structJob>::iterator iter = jobList.begin();
	while ( iter != jobList.end() ) {
		if ( escHandler.isInterruptRequested() ) throw( cancelFeedback );
		
		MString baseFileName;
		if ( ( DDimageName[0] == "" ) ) {
			baseFileName = sceneName; 
		} else {
			char *mydot = ".";
			int pointIndex = DDimageName[0].index( *mydot );
			baseFileName = DDimageName[0].substring(0, pointIndex-1).asChar();
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
		fileNameFmtString = ribDir;
		fileNameFmtString += baseFileName;
		fileNameFmtString += animExt;
		fileNameFmtString += extension;
		
		size_t fileNameLength = fileNameFmtString.length() + 8;		 
		fileNameLength += 10; // Enough to hold the digits for 1 billion frames
		char *frameFileName = (char *)alloca(fileNameLength);
		sprintf(frameFileName, fileNameFmtString.asChar(), doExtensionPadding ? outPadding : 0, lframe);
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
			outFileFmtString = texDir;
			size_t outNameLength;
			if ( userShadowName != MString( "" ) ) {
				outFileFmtString += userShadowName;
				outNameLength = outFileFmtString.length();
			} else {
				outFileFmtString += baseFileName;
			
				outNameLength = outFileFmtString.length();
		
				if (animation || useFrameExt) {
					outFileFmtString += animExt;
					outNameLength += outPadding + 1;
				}
				outFileFmtString += ".tex";
				outNameLength = outFileFmtString.length();
				outNameLength = outNameLength + ( outPadding + 1 ); // Space for the null character
			}
			
			char *outName = (char *)alloca(outNameLength);
			
			if ( userShadowName != MString( "" ) ) {
				sprintf(outName, outFileFmtString.asChar() );
			} else {
				sprintf(outName, outFileFmtString.asChar(), 1, lframe);
			}
			iter->imageName = outName;
			thisJob = *iter;
			if ( iter->isShadow ) shadowList.push_back( thisJob );
		} else {
			outFileFmtString = pixDir;
			outFileFmtString += baseFileName;
			
			size_t outNameLength;
		
			outNameLength = outFileFmtString.length();
		
			if (animation || useFrameExt) {
				outFileFmtString += animExt;
				outNameLength += outPadding + 1;
			}
			
			outFileFmtString += ".";
			outFileFmtString += outExt;
			outNameLength = outFileFmtString.length();
			outNameLength = outNameLength + ( outPadding + 1 ); // Space for the null character
		
			char *outName = (char *)alloca(outNameLength);
			sprintf(outName, outFileFmtString.asChar(), outPadding, lframe);
			if ( DDimageName[0] == "" ) {
				iter->imageName = outName;
			} else {
				iter->imageName = pixDir; 
				iter->imageName += parseString( DDimageName[0] );
			}
		
		}
		++iter;
	}
	
	ribStatus = kRibBegin;   
	return MS::kSuccess;
}

MStatus RibTranslator::ribPrologue()
//
//  Description:
//      Write the prologue for the RIB file
//
{
	if ( !exportReadArchive ) {
		if ( debugMode ) { printf("-> beginning to write prologue\n"); }
		MStatus returnStatus = MS::kSuccess;
		MStatus status;
		
		// set any rib options		
		RiOption( ( RtToken )"limits", ( RtToken )"bucketsize", (RtPointer)&bucketSize, RI_NULL );
		RiOption( "limits", "gridsize", (RtPointer)&gridSize, RI_NULL );
		RiOption( "limits", "texturememory", (RtPointer)&textureMemory, RI_NULL );
		RiOption( "limits", "eyesplits", (RtPointer)&eyeSplits, RI_NULL );
		RiArchiveRecord( RI_COMMENT, "Shader Path\nOption \"searchpath\" \"shader\" [\"%s\"]\n", shaderPath.asChar() );

		/* BMRT OPTIONS: BEGIN */
		if ( useBMRT ) {
			RiOption( "radiosity", "integer steps", (RtPointer)&RadSteps, RI_NULL );
			RiOption( "radiosity", "integer minpatchsamples", (RtPointer)&RadMinPatchSamples, RI_NULL );
			if ( BMRTusePrmanSpec ) {
				RtInt prmanSpec = 1;
				RiOption( "render", "integer prmanspecular", &prmanSpec, RI_NULL );
			}
			if ( BMRTusePrmanDisp ) {
				RtInt prmanDisp = 1;
				RiOption( "render", "integer useprmandspy", &prmanDisp, RI_NULL );
			}							
		}
		/* BMRT OPTIONS: END */
		
		
		RiOrientation( RI_RH );       // Right-hand coordinates
		if ( currentJob.isShadow ) {
			RiPixelSamples( 1, 1 );
			RiShadingRate( 1 );
		} else {
			RiPixelSamples( pixelSamples, pixelSamples );
			RiShadingRate( shadingRate );
			if ( rFilterX > 1 || rFilterY > 1 ) {
				if ( rFilter == fBoxFilter ) {
					RiPixelFilter( RiBoxFilter, rFilterX, rFilterY );
				} else if ( rFilter == fTriangleFilter ) {
					RiPixelFilter( RiTriangleFilter, rFilterX, rFilterY );
				} else if ( rFilter == fCatmullRomFilter ) {
					RiPixelFilter( RiCatmullRomFilter, rFilterX, rFilterY );
				} else if ( rFilter == fGaussianFilter ) {
					RiPixelFilter( RiGaussianFilter, rFilterX, rFilterY );
				} else if ( rFilter == fSincFilter ) {
					RiPixelFilter( RiSincFilter, rFilterX, rFilterY );
				}
			}
		}
	}
    ribStatus = kRibBegin;   
    return MS::kSuccess;
}

MStatus RibTranslator::ribEpilogue()
//
//  Description:
//      Write the epilogue for the RIB file
//
{
    if (ribStatus == kRibBegin) ribStatus = kRibOK;
    return (ribStatus == kRibOK ? MS::kSuccess : MS::kFailure);
}

MStatus RibTranslator::scanScene(float lframe, int sample )
//
//  Description:
//      Scan the DAG at the given frame number and record information
//      about the scene for writing
//
{    
    MTime   mt((double)lframe);
    if ( MGlobal::viewFrame(mt) == MS::kSuccess) {
		
		if ( preFrameMel != "" ) MGlobal::sourceFile( preFrameMel );
		
		MStatus returnStatus;

		/* Scan the scene for place3DTextures - for coordinate systems */
		{
			MItDag dagLightIterator( MItDag::kDepthFirst, MFn::kPlace3dTexture, &returnStatus);
			
			for (; !dagLightIterator.isDone(); dagLightIterator.next()) {
				if ( escHandler.isInterruptRequested() ) throw( cancelFeedback );
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
				if ( escHandler.isInterruptRequested() ) throw( cancelFeedback );
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
		
		if ( !renderSelected ) {
			MItDag dagIterator( MItDag::kDepthFirst, MFn::kInvalid, &returnStatus);
			for (; !dagIterator.isDone(); dagIterator.next()) {
				if ( escHandler.isInterruptRequested() ) throw( cancelFeedback );
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
					if ( renderAllCurves || ( plugStatus == MS::kSuccess ) ) {
						if ( ( sample > 0 ) && isObjectMotionBlur( path )){
							htable->insert(path, lframe, sample, MRT_Unknown );
						} else {
							htable->insert(path, lframe, 0, MRT_Unknown );
						}
					}
				}
			}
		} else {
			MItSelectionList selIterator( currentSelection );
			MItDag dagIterator( MItDag::kDepthFirst, MFn::kInvalid, &returnStatus);
			for ( ; !selIterator.isDone(); selIterator.next() ) 
			{
				MDagPath objectPath;
				selIterator.getDagPath( objectPath );
				dagIterator.reset (objectPath.node(), MItDag::kDepthFirst, MFn::kInvalid );
				for (; !dagIterator.isDone(); dagIterator.next()) {
					if ( escHandler.isInterruptRequested() ) throw( cancelFeedback );
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
						if ( renderAllCurves || ( plugStatus == MS::kSuccess ) ) {
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
			if ( escHandler.isInterruptRequested() ) throw( cancelFeedback );
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
				shutterTime = iter->camera[sample].shutter;
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
				iter->height = iter->width = dmapSize;
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
	
	if ( postFrameMel != "" ) MGlobal::sourceFile( postFrameMel );
	
	return MS::kSuccess;
  }
  return MS::kFailure;
}

void RibTranslator::doAttributeBlocking( const MDagPath & newPath, const MDagPath & previousPath )
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

MStatus RibTranslator::framePrologue(long lframe)
//  
//  Description:
//  	Write out the frame prologue.
//  
{
	if ( debugMode ) printf( "-> Beginning Frame Prologue\n" );
    ribStatus = kRibFrame;
	
	if ( !exportReadArchive ) {
		
		RiFrameBegin( lframe );
		
		if ( !currentJob.isShadow ) {
			// Smooth Shading
			RiShadingInterpolation( "smooth" );
			// Quantization
			if ( quantValue != 0 ) {
				int whiteValue = pow( 2.0, (int)quantValue ) - 1;
				RiQuantize( RI_RGBA, whiteValue, 0, whiteValue, 0.5 );
			} else {
				RiQuantize( RI_RGBA, 0, 0, 0, 0 );
			}				
			if ( rgain != 1.0 || rgamma != 1.0 ) {
				RiExposure( rgain, rgamma );
			}
		}
		
		if ( debugMode ) printf( "-> Setting Display Options\n" );
		
		if ( currentJob.isShadow ) {
			char *outName = (char *)alloca( currentJob.imageName.length()+1);
			sprintf(outName, currentJob.imageName.asChar());
			char *outFormatType = (char *)alloca(20);
			sprintf(outFormatType, currentJob.format.asChar());
			if ( !currentJob.isMinMaxShadow ) {
				RiDisplay( outName, outFormatType, (RtToken)currentJob.imageMode.asChar(), RI_NULL ); 
			} else {
				RiArchiveRecord( RI_COMMENT, "Display Driver: \nDisplay \"%s\" \"%s\" \"%s\" \"minmax\" [ 1 ]", outName, outFormatType, (RtToken)currentJob.imageMode.asChar() );
			}
		} else {
			if ( ( cropX1 != 0.0 ) || ( cropY1 != 0.0 ) || ( cropX2 != 1.0 ) || ( cropY2 != 1.0 ) ) {
				RiCropWindow( cropX1, cropX2, cropY1, cropY2 );
			};
			
			int k = 0;
			while ( k < numDisplayDrivers ) {
				MString parameterString;
				MString imageName;
				MString formatType;
				MString imageMode;
				if ( k > 0 ) imageName = "+"; 
				imageName += pixDir;
				if ( DDimageName[k] == "" ) {
					imageName = currentJob.imageName;
				} else { 
					imageName += parseString( DDimageName[k] );
				}
				if ( DDimageType[k] == "" ) {
					formatType = currentJob.format;
				} else { 
					formatType = parseString( DDimageType[k] );
				}
				if ( DDimageMode[k] == "" ) {
					imageMode = currentJob.imageMode;
				} else { 
					imageMode = parseString( DDimageMode[k] );
				}
				if ( DDparamType[k] != "" && DDparamType[k] != "rgbaz" && DDparamType[k] != "rgba" && DDparamType[k] != "rgb" && DDparamType[k] != "z" ) {
					MString pType = "varying ";
					pType += DDparamType[k];
					RiDeclare( (RtToken)imageMode.asChar(),(RtToken)pType.asChar() ); 
				} 
				for ( int p = 0; p < DDParams[k].num; p++ ) {
					parameterString += "\""; parameterString += DDParams[k].names[p].asChar(); parameterString += "\" ";
					if ( DDParams[k].type[p] == 1 ) {
						RiDeclare( (RtToken)DDParams[k].names[p].asChar(), "string" );
						parameterString += " [\""; parameterString += DDParams[k].data[p].asChar(); parameterString += "\"] ";
					} else if ( DDParams[k].type[p] == 2 ) {
						
						/* BUGFIX: Monday 6th August fixed bug where a float display driver parameter wasn't declared */
						
						RiDeclare( (RtToken)DDParams[k].names[p].asChar(), "float" );
						parameterString += " ["; parameterString += DDParams[k].data[p].asChar(); parameterString += "] ";
					}
				}
				RiArchiveRecord( RI_COMMENT, "Display Driver %d: \nDisplay \"%s\" \"%s\" \"%s\" %s", k, imageName.asChar(), formatType.asChar(), imageMode.asChar(), parameterString.asChar() );
				k++; 
			}
		}
		
		if ( debugMode ) printf( "-> Setting Resolution\n" );
		
		RiFormat( currentJob.width, currentJob.height, currentJob.aspectRatio );
		
		if ( currentJob.camera[0].isOrtho ) {
			RtFloat frameWidth, frameHeight;
			frameWidth = currentJob.camera[0].orthoWidth * 0.5; // the whole frame width has to be divided in half!
			frameHeight = currentJob.camera[0].orthoHeight * 0.5; // the whole frame height has to be divided in half!
			RiProjection( "orthographic", RI_NULL );
			RiScreenWindow( -frameWidth, frameWidth, -frameHeight, frameHeight );
		} else {
			RtFloat fieldOfView = currentJob.camera[0].hFOV * 180.0 / M_PI;
			if ( currentJob.isShadow && currentJob.isPoint ) {
				fieldOfView = currentJob.camera[0].hFOV;
			}
			RiProjection( "perspective", RI_FOV, &fieldOfView, RI_NULL );
		}
		
		RiClipping( currentJob.camera[0].neardb, currentJob.camera[0].fardb );
		if ( doDof && !currentJob.isShadow ) {
			RiDepthOfField( currentJob.camera[0].fStop, currentJob.camera[0].focalLength, currentJob.camera[0].focalDistance );
		}
		// Set up for camera motion blur
		//
		/* doCameraMotion = currentJob.camera[0].motionBlur && doMotion; */
		if ( doMotion || doDef ) {
			RiShutter( ( lframe - ( currentJob.camera[0].shutter * 0.5 ) ), ( lframe + ( currentJob.camera[0].shutter * 0.5 ) ) );
		} else {
			RiShutter( lframe, lframe );
		} 
		
		if ( currentJob.gotJobOptions ) {
			RiArchiveRecord( RI_COMMENT, "jobOptions: \n%s", currentJob.jobOptions.asChar() );
		}
		if ( ( preRibBox.length() > 0 ) && !currentJob.isShadow ) {
			int ii;
			for ( ii = 0; ii < preRibBox.length(); ii++ ) {
				RiArchiveRecord( RI_COMMENT, "Additional Rib:\n%s", preRibBox[ii].asChar() );
			}
		}
		if ( ( preReadArchive.length() > 0 ) && !currentJob.isShadow ) {
			int ii;
			for ( ii = 0; ii < preReadArchive.length(); ii++ ) {
				RiArchiveRecord( RI_COMMENT, "Read Archive Data: \nReadArchive \"%s\"", preReadArchive[ii].asChar() );
			}
		}
		if ( ( preRibBoxShadow.length() > 0 ) && !currentJob.isShadow ) {
			int ii;
			for ( ii = 0; ii < preRibBox.length(); ii++ ) {
				RiArchiveRecord( RI_COMMENT, "Additional Rib:\n%s", preRibBoxShadow[ii].asChar() );
			}
		}
		if ( ( preReadArchiveShadow.length() > 0 ) && currentJob.isShadow ) {
			int ii;
			for ( ii = 0; ii < preReadArchive.length(); ii++ ) {
				RiArchiveRecord( RI_COMMENT, "Read Archive Data: \nReadArchive \"%s\"", preReadArchiveShadow[ii].asChar() );
			}
		}
		
		if ( doCameraMotion && ( !currentJob.isShadow )) {
			/*RiMotionBegin( 2, ( lframe - ( currentJob.camera[0].shutter * blurTime * 0.5  )), ( lframe + ( currentJob.camera[0].shutter * blurTime * 0.5  )) );*/
			RiMotionBegin( motionSamples, sampleTimes[0], sampleTimes[1] , sampleTimes[2], sampleTimes[3], sampleTimes[4] );
		}
		RtMatrix cameraMatrix;
		currentJob.camera[0].mat.get( cameraMatrix );
		RiTransform( cameraMatrix );
		if ( doCameraMotion && ( !currentJob.isShadow ) ) {	
			int mm = 1;
			while ( mm < motionSamples ) {
				currentJob.camera[mm].mat.get( cameraMatrix );
				RiTransform( cameraMatrix );
				mm++;
			}
			RiMotionEnd();
		}
	}
    return MS::kSuccess;
}

MStatus RibTranslator::frameEpilogue( long )
//  
//  Description:
//  	Write out the frame epilogue.
//  
{
    if (ribStatus == kRibFrame) {
		ribStatus = kRibBegin;
        if ( !exportReadArchive ) {
			RiFrameEnd();
		}
    }
    return (ribStatus == kRibBegin ? MS::kSuccess : MS::kFailure);
}

MStatus RibTranslator::frameBody()
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
	if ( !exportReadArchive ) {
		RiWorldBegin();
		RiTransformBegin();
		RiCoordinateSystem( "worldspace" );
		RiTransformEnd();
		
		if ( !currentJob.isShadow && !ignoreLights ) {
			
			int nbLight = 0;
			
			for ( rniter = htable->RibNodeMap.begin(); rniter != htable->RibNodeMap.end(); rniter++ ) {
				if ( escHandler.isInterruptRequested() ) throw( cancelFeedback );
				RibNode *	rn = (*rniter).second;
				if (rn->object(0)->ignore || rn->object(0)->type != MRT_Light) continue;
				rn->object(0)->writeObject();
				rn->object(0)->written = 1;
				nbLight++;
			}
			
		}
	}
	
    if ( ignoreSurfaces ) {
        RiSurface( "matte", RI_NULL );
    }
    
    MMatrix matrix;
    MDagPath path;
    MFnDagNode dagFn;
	
	
	for ( rniter = htable->RibNodeMap.begin(); rniter != htable->RibNodeMap.end(); rniter++ ) {
		if ( escHandler.isInterruptRequested() ) throw( cancelFeedback );
		
		RibNode * ribNode = (*rniter).second;
		path = ribNode->path();
		
		if ( ( NULL == ribNode ) || ( ribNode->object(0)->type == MRT_Light ) ) continue;
		if ( ( !currentJob.isShadow ) && ( ribNode->object(0)->ignore ) ) continue;
		if ( ( currentJob.isShadow ) && ( ribNode->object(0)->ignoreShadow ) ) continue;
		
		char *namePtr = ( char * )alloca( sizeof( char ) * ribNode->name.length()+1);
		sprintf( namePtr, ribNode->name.asChar() );
		//const char * namePtr = ribNode->name;
		if ( outputComments ) RiArchiveRecord( RI_COMMENT, "Name: %s", ribNode->name.asChar(), RI_NULL );
		RiAttributeBegin();
		RiAttribute( "identifier", "name", &namePtr, RI_NULL );
		
		// If this is a matte object, then turn that on if it isn't currently set 
		if ( ribNode->matteMode ) {
			if ( !currentMatteMode ) RiMatte(RI_TRUE);
			currentMatteMode = true;
		} else {
			if ( currentMatteMode ) RiMatte(RI_FALSE);
			currentMatteMode = false;
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
				RibNode * ln = htable->find( lightFnDag.fullPathName(), nodeDagPath, MRT_Light );						
				if ( NULL != ln ) {
					RiIlluminate( ln->object(0)->lightHandle(), RI_FALSE );
				}
			}				
		}
		
		// If there is matrix motion blur, open a new motion block, the 5th element in the object will always 
		// be there if matrix blur will occur!
		if ( doMotion && ribNode->doMotion && ( ribNode->object(1) != NULL ) && ( ribNode->object(0)->type != MRT_Locator ) 
			&& ( !currentJob.isShadow ) ) {    
			if ( debugMode ) { printf("-> writing matrix motion blur data\n"); }
			RiMotionBegin( motionSamples, sampleTimes[0], sampleTimes[1] , sampleTimes[2], sampleTimes[3], sampleTimes[4] );
		}
		RtMatrix ribMatrix;
		matrix = ribNode->object(0)->matrix( path.instanceNumber() );
		matrix.get( ribMatrix );
		RiTransform( ribMatrix );
		
		// Output the world matrices for the motionblur
		// This will override the current transformation setting
		//
		if ( doMotion && ribNode->doMotion && ( ribNode->object(1) != NULL ) && ( ribNode->object(0)->type != MRT_Locator ) 
			&& ( !currentJob.isShadow ) ) {    
			path = ribNode->path();
			int mm = 1;
			RtMatrix ribMatrix;
			while ( mm < motionSamples ) {
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
		if ( ( status == MS::kSuccess ) && !rmanShaderPlug.isNull() && !ignoreDisplacements ) {
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
		if ( ( status == MS::kSuccess ) && !rmanShaderPlug.isNull() && !ignoreVolumes ) {
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
		
		if ( currentJob.isShadow && !outputShadersInShadows ) writeShaders = false;
		
		if ( writeShaders  ) {
			if ( hasVolumeShader && !ignoreVolumes ) {
				
				shaderStruct currentShader;
				currentShader = liquidGetShader( ribNode->assignedVolume.object());
			
				if ( !currentShader.hasErrors ) {				
					RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * currentShader.numTPV );
					RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * currentShader.numTPV );

					char *assignedRManShader = (char *)alloca(currentShader.file.size() + 1);
					sprintf(assignedRManShader, currentShader.file.c_str() );
					
					assignTokenArrays( currentShader.numTPV, currentShader.tokenPointerArray, tokenArray, pointerArray );
					RiAtmosphereV ( assignedRManShader, currentShader.numTPV, tokenArray, pointerArray );
				}
			} 
			
			if ( hasSurfaceShader && !ignoreSurfaces ) {
				
				shaderStruct currentShader;
				currentShader = liquidGetShader( ribNode->assignedShader.object());

				RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * currentShader.numTPV );
				RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * currentShader.numTPV );
					
				assignTokenArrays( currentShader.numTPV, currentShader.tokenPointerArray, tokenArray, pointerArray );
				RiColor( currentShader.rmColor );
				RiOpacity( currentShader.rmOpacity );

				char *assignedRManShader = (char *)lmalloc(currentShader.file.size() + 1);
				sprintf(assignedRManShader, currentShader.file.c_str() );

					
#ifndef _WIN32
				if ( shortShaderNames ) {
					MString shaderString = basename( assignedRManShader );
					lfree( assignedRManShader ); assignedRManShader = NULL;
					assignedRManShader = (char *)lmalloc(shaderString.length()+1);
					sprintf(assignedRManShader, shaderString.asChar());
				}
#endif
				if ( ribNode->nodeShadingRateSet && ( ribNode->nodeShadingRate != currentNodeShadingRate ) ) {
					RiShadingRate ( ribNode->nodeShadingRate );
					currentNodeShadingRate = ribNode->nodeShadingRate;
				} else if ( currentShader.hasShadingRate ) {
					RiShadingRate ( currentShader.shadingRate );
					currentNodeShadingRate = currentShader.shadingRate;
				}

				RiSurfaceV ( assignedRManShader, currentShader.numTPV, tokenArray, pointerArray );

			} else {
				RtColor rColor;
				if ( shaderDebug ) {
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
				
				if ( !ignoreSurfaces ) {
					MObject shadingGroup = ribNode->assignedShadingGroup.object();
					MObject shader = ribNode->findShader( shadingGroup );
					if ( shaderDebug ) {
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
		
		if ( hasDisplacementShader && !ignoreDisplacements ) {
			
			shaderStruct currentShader;
			currentShader = liquidGetShader( ribNode->assignedDisp.object() );
			
			if ( !currentShader.hasErrors ) {
				RtToken *tokenArray = (RtToken *)alloca( sizeof(RtToken) * currentShader.numTPV );
				RtPointer *pointerArray = (RtPointer *)alloca( sizeof(RtPointer) * currentShader.numTPV );
				
				assignTokenArrays( currentShader.numTPV, currentShader.tokenPointerArray, tokenArray, pointerArray );
				
				char *assignedRManShader = (char *)lmalloc(currentShader.file.size() + 1);
				sprintf(assignedRManShader, currentShader.file.c_str() );

#ifndef _WIN32
				if ( shortShaderNames ) {
					MString shaderString = basename( assignedRManShader );
					lfree( assignedRManShader ); assignedRManShader = NULL;
					assignedRManShader = (char *)lmalloc(shaderString.length()+1);
					sprintf(assignedRManShader, shaderString.asChar());
				}
#endif
				if ( ribNode->nodeShadingRateSet && ( ribNode->nodeShadingRate != currentNodeShadingRate ) ) {
					RiShadingRate ( ribNode->nodeShadingRate );
					currentNodeShadingRate = ribNode->nodeShadingRate;
				} else if ( currentShader.hasShadingRate ) {
					RiShadingRate ( currentShader.shadingRate );
					currentNodeShadingRate = currentShader.shadingRate;
				}
				RiDisplacementV ( assignedRManShader, currentShader.numTPV, tokenArray, pointerArray );
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
		if (doDef && ribNode->doDef && ( ribNode->object(1) != NULL ) && ( ribNode->object(0)->type != MRT_RibGen )
			&& ( ribNode->object(0)->type != MRT_Locator ) && ( !currentJob.isShadow ) ) {
			RiMotionBegin( motionSamples, sampleTimes[0] , sampleTimes[1] , sampleTimes[2], sampleTimes[3], sampleTimes[4] );
		}        
		
		ribNode->object(0)->writeObject();
		if ( doDef && ribNode->doDef && ( ribNode->object(1) != NULL ) && ( ribNode->object(0)->type != MRT_RibGen )
			&& ( ribNode->object(0)->type != MRT_Locator ) && ( !currentJob.isShadow ) ) {
			if ( debugMode ) { printf("-> writing deformation blur data\n"); }
			int msampleOn = 1;
			while ( msampleOn < motionSamples ) {
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
	
    if ( !exportReadArchive ) {
		RiWorldEnd();
	}
	return returnStatus;
}
