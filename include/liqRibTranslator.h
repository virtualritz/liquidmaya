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

#ifndef liqRibTranslator_H
#define liqRibTranslator_H

/* ______________________________________________________________________
**
** Liquid Rib Translator Header File
** ______________________________________________________________________
*/

#include <liquid.h>
#include <liqRenderer.h>
#include <liqRibHT.h>
#include <liqShader.h>
#include <liqRenderScript.h>
#include <liqRibLightData.h>
#include <liqExpression.h>

#include <maya/MPxCommand.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnCamera.h>
#include <maya/MArgList.h>
#include <maya/MFloatArray.h>


class liqRibTranslator : public MPxCommand {
public:
  liqRibTranslator();
  ~liqRibTranslator();
  static void * creator();
  static MSyntax syntax();

  MStatus doIt(const MArgList& args );

private: // Methods
  MObject rGlobalObj;

  MStatus scanScene(float, int );

  void portFieldOfView( int width, int height, double& horizontal, double& vertical, MFnCamera& fnCamera );
  void computeViewingFrustum( double window_aspect, double& left, double& right, double& bottom, double& top, MFnCamera& cam );
  void getCameraInfo( MFnCamera &cam );
  void setSearchPaths();
  void setOutDirs();

  // rib output functions
  MStatus liquidDoArgs( MArgList args );
  bool liquidInitGlobals();
  void liquidReadGlobals();
  bool verifyOutputDirectories();

  MStatus buildJobs();
  MStatus ribPrologue();
  MStatus ribEpilogue();
  MStatus framePrologue( long );
  MStatus worldPrologue();
  MStatus lightBlock();
  MStatus coordSysBlock();
  MStatus objectBlock();
  MStatus worldEpilogue();
  MStatus frameEpilogue( long );
  void doAttributeBlocking( const MDagPath & newPath,  const MDagPath & previousPath );
  void printProgress ( int stat, long first, long last, long where );

  MString generateRenderScriptName()  const;
  MString generateTempMayaSceneName() const;
  MString generateFileName( fileGenMode mode, const structJob& job );
  MString generateShadowArchiveName( bool renderAllFrames, long renderAtframe, MString geometrySet );
  static bool renderFrameSort( const structJob& a, const structJob& b );

private: // Data
  enum MRibStatus {
    kRibOK,
    kRibBegin,
    kRibFrame,
    kRibWorld,
    kRibError
  };
  MRibStatus ribStatus;

  // Render Globals and RIB Export Options
  std::vector<structJob>  jobList;
  std::vector<structJob>  shadowList;

  std::vector<structJob>  refList;  // reflections list
  std::vector<structJob>  envList;  // environments list
  std::vector<structJob>  txtList;  // make textures list

  MDagPathArray shadowLightArray;
  MDagPath activeCamera;

  const MString m_default_tmp_dir;
  MString m_systemTempDirectory;

  liquidlong frameFirst;
  liquidlong frameLast;
  liquidlong frameBy;
  liquidlong width, height, depth;

  // alfred stuff
  bool useRenderScript;
  bool cleanRenderScript;
  bool m_alfShadowRibGen;
  bool m_alfredExpand;
  MString renderJobName;
  MString m_alfredTags;
  MString m_alfredServices;
  MString m_userRenderScriptFileName;
  MString m_renderScriptCommand;
  enum renderScriptFormat {
    NONE   = 0,
    ALFRED = 1,
    XML    = 2 }
  m_renderScriptFormat;

  bool useNetRman;
  bool fullShadowRib;
  bool remoteRender;
  bool cleanRib;              // clean the rib files up

  bool doDof;                 // do camera depth of field
  bool doCameraMotion;        // Motion blur for moving cameras
  bool liqglo_rotateCamera;   // rotates the camera for sideways renderings
  enum shutterConfig {
    OPEN_ON_FRAME         = 0,
    CENTER_ON_FRAME       = 1,
    CENTER_BETWEEN_FRAMES = 2,
    CLOSE_ON_NEXT_FRAME   = 3
  } shutterConfig;

  bool cleanShadows;
  bool cleanTextures;
  liquidlong pixelSamples;
  float shadingRate;
  liquidlong bucketSize[2];
  liquidlong gridSize;
  liquidlong textureMemory;
  liquidlong eyeSplits;
  bool renderAllCameras;   // Render all cameras, or only active ones
  bool ignoreFilmGate;
  double fov_ratio;
  int cam_width, cam_height;
  float aspectRatio;
  liquidlong quantValue;
  MString renderCamera;
  MString baseShadowName;
  bool createOutputDirectories;

  static MString magic;

  // Data used to construct output file names
  MString outFormat;
  MString outFormatString;
  MString outExt;
  liquidlong outFormatControl;
  MString extension;
  MString imageName;

  // Data used for choosing output method
  MString riboutput;
  bool launchRender;

  // Hash table for scene
  liqRibHT *htable;

  // Depth in attribute blocking
  int attributeDepth;

private :

  // Old global values
  int m_errorMode;
  M3dView m_activeView;
  MString m_pixDir;
  MString m_tmpDir;
  MString m_ribDirG;
  MString m_texDirG;
  MString m_tmpDirG;
  bool m_animation;
  bool m_useFrameExt;
  bool m_shadowRibGen;
  double m_blurTime;
  MComputation m_escHandler;
  float m_rgain, m_rgamma;
  bool m_justRib;
  liquidlong m_minCPU;
  liquidlong m_maxCPU;

  double m_cropX1, m_cropX2, m_cropY1, m_cropY2;

#ifdef _WIN32
  int RiNColorSamples;
#endif

  // these are little storage variables to keep track of the current graphics state and will eventually be wrapped in
  // a specific class
  struct {
    struct {
      bool showProgress;
      bool outputDetailedComments;
      bool shaderDebugging;
    } feedback;

    struct {
      bool noShadowRibs;
      bool fullShadowRibs;
      bool lazyCompute;
    } shadowMaps;

    struct {
      bool opacityThreshold;
      bool outputAllShaders;
    } depthShadows;

    struct {
      bool outputAllShaders;
      bool outputLights;
    } deepShadows;

    struct {
      bool readArchiveable;

      struct {
        bool allCurves;
        bool meshUVs;
      } output;

      struct {
        bool noSurfaces;
        bool noLights;
        bool noDisplacements;
        bool noVolumes;
        bool expandArrays;
      } shaders;

      struct {
        bool projectRelative;
        bool shaders;
      } paths;

      struct {
        bool binary;
        bool gZip;
      } format;

      struct {
        MString preWorld;
        MString postWorld;
        MString preObject;
      } box;

    } rib;

  } globals;


  bool m_showProgress;
  bool m_currentMatteMode;
  bool m_renderSelected;
  bool m_exportReadArchive;
  bool m_renderAllCurves;
  bool m_ignoreLights;
  bool m_ignoreSurfaces;
  bool m_ignoreDisplacements;
  bool m_ignoreVolumes;
  bool m_outputShadowPass;
  bool m_outputHeroPass;
  bool m_deferredGen;
  bool m_lazyCompute;
  bool m_outputShadersInShadows;
  bool m_outputShadersInDeepShadows;
  bool m_outputLightsInDeepShadows;
  liquidlong m_deferredBlockSize;
  bool m_outputComments;
  bool m_shaderDebug;

  long m_currentLiquidJobNumber;

  MString m_defGenKey;
  MString m_defGenService;

  MString m_preFrameMel;
  MString m_postFrameMel;

  MString m_renderCommand;
  MString m_ribgenCommand;
  MString m_preCommand;

  MString m_preJobCommand;
  MString m_postJobCommand;
  MString m_preFrameCommand;
  MString m_postFrameCommand;

  MString m_shaderPath;

  MString m_preFrameRIB;
  MString m_preWorldRIB;
  MString m_postWorldRIB;

  MString m_preGeomRIB;

  // Display Driver Variables
  typedef struct structDDParam {
    liquidlong    num;
    MStringArray  names;
    MStringArray  data;
    MIntArray     type;
  } structDDParam;

  bool          m_ignoreAOVDisplays;

  typedef struct structDisplay {
    MString         name;
    MString         type;
    MString         mode;
    bool            enabled;
    bool            doQuantize;
    int             bitDepth;
    float           dither;
    bool            doFilter;
    MString         filter;
    float           filterX;
    float           filterY;
    structDDParam   xtraParams;
  } structDisplay;
  std::vector<structDisplay> m_displays;

  typedef struct structChannel {
    MString     name;
    int         type;
    bool        quantize;
    int         bitDepth;
    float       dither;
    bool        filter;
    int         pixelFilter;
    float       pixelFilterX;
    float       pixelFilterY;
  } structChannel;
  std::vector<structChannel> m_channels;

  MStringArray  m_pixelFilterNames;
  liquidlong    m_rFilter;
  float         m_rFilterX, m_rFilterY;

  bool          m_renderView;
  bool          m_renderViewCrop;
  bool          m_renderViewLocal;
  liquidlong    m_renderViewPort;
  liquidlong    m_renderViewTimeOut;

  int           m_statistics;

  std::vector<liqShader> m_shaders;

  liqShader & liqGetShader( MObject shaderObj );
  MStatus liqShaderParseVectorAttr ( liqShader & currentShader, MFnDependencyNode & shaderNode, const char * argName, ParameterType pType );
  void freeShaders( void );

  void scanExpressions( liqShader & currentShader );
  void liqRibTranslator::scanExpressions( liqRibLightData *light );
  void processExpression( liqTokenPointer *token, liqRibLightData *light = NULL );
};

#endif
