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

#include <liqRibHT.h>
#include <liqShader.h>
#include <maya/MPxCommand.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnCamera.h>
#include <maya/MArgList.h>

class liqRibTranslator : public MPxCommand {
public:
	liqRibTranslator();
	~liqRibTranslator();
	static void * creator();
	static MSyntax syntax();

	MStatus doIt(const MArgList& args );

private: // Methods

    MObject	rGlobalObj;

    MStatus scanScene(float, int );

    void portFieldOfView( int width, int height, double& horizontal, double& vertical, MFnCamera& fnCamera );
    void computeViewingFrustum (	double window_aspect, double& left, double& right, double& bottom, double& top, MFnCamera& cam );
    void getCameraInfo( MFnCamera &cam );

    // rib output functions
    MStatus liquidDoArgs( MArgList args );
    bool liquidInitGlobals();
    void liquidReadGlobals();
    bool verifyOutputDirectories();

    MStatus		buildJobs();
    MStatus		ribPrologue();
    MStatus		ribEpilogue();
    MStatus		framePrologue( long );
    MStatus		frameBody();
    MStatus		frameEpilogue( long );
    void	    doAttributeBlocking( const MDagPath & newPath,  const MDagPath & previousPath );
    void	    printProgress ( int stat, long first, long last, long where );

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
    //
    std::vector<structJob>	jobList;
    std::vector<structJob>	shadowList;

    MDagPathArray shadowLightArray;
    MDagPath	  activeCamera;
#ifndef _WIN32
    const char *m_systemTempDirectory;
    static const char *m_default_tmp_dir;
#else
    char *m_systemTempDirectory;
#endif
    liquidlong		  frameFirst;
    liquidlong		  frameLast;
    liquidlong		  frameBy;
    liquidlong		  width, height, depth;

    bool		    useNetRman;
    bool    	    	    fullShadowRib;
    bool    	    	    remoteRender;
    bool		    cleanRib;	      // clean the rib files up
    bool		    doDof;              // do camera depth of field
    bool		    doCameraMotion;     // Motion blur for moving cameras
    bool		    cleanShadows;
    bool		    cleanTextures;
    bool   		    cleanAlf;
    bool		    useAlfred;
    bool		    doExtensionPadding;
    liquidlong		    pixelSamples;
    float		  shadingRate;
    liquidlong		    bucketSize[2];
    liquidlong		    gridSize;
    liquidlong		    textureMemory;
    liquidlong		    eyeSplits;
    bool		    renderAllCameras;   // Render all cameras, or only active ones
    bool		    ignoreFilmGate;
    double		  fov_ratio;
    int			    cam_width, cam_height;
    float		    aspectRatio;
    liquidlong	    	    quantValue;
    MString 	    	    renderCamera;
    MString 	    	    baseShadowName;    
    MString 	    	    alfredJobName;
    bool    	    	    createOutputDirectories;
	
    static MString magic;
	
    // Data used to construct output file names
    //
    MString 	    outFormat;
    MString 	    outFormatString;
    MString 	    outExt;
    liquidlong	    outFormatControl;
    MString 	    extension;
    MString 	    imageName;
	
    // Data used for choosing output method
    MString 	    riboutput;
    bool    	    outputpreview;
	
    // Hash table for scene
    //
    liqRibHT   	    *htable;
    
    // Depth in attribute blocking
    //
    int         attributeDepth;

private :
    // Old global values
    
    RendererType    	    m_renderer;		// which renderer are we using
    int				m_errorMode;
    M3dView			m_activeView;
    MString			m_pixDir;
    MString			m_tmpDir;
    MString			m_ribDirG;
    MString			m_pixDirG;
    MString			m_texDirG;
    MString			m_tmpDirG;
    bool			m_animation;
    bool			m_useFrameExt;
    bool			m_shadowRibGen;
    bool			m_alfShadowRibGen;
    double		    	m_blurTime;
    liquidlong			m_outPadding;
    MComputation    	    	m_escHandler;
    float		    m_rgain, m_rgamma;
    bool		    m_justRib;
    liquidlong      	    	m_minCPU;
    liquidlong			m_maxCPU;

    double    	    	m_cropX1, m_cropX2, m_cropY1, m_cropY2;


#ifdef _WIN32
    int RiNColorSamples;
#endif

    // these are little storage variables to keep track of the current graphics state and will eventually be wrapped in
    // a specific class

    bool			m_showProgress;
    bool 			m_currentMatteMode;
    bool			m_renderSelected;
    bool			m_exportReadArchive;
    bool 			m_renderAllCurves;
    bool			m_ignoreLights;
    bool			m_ignoreSurfaces;
    bool			m_ignoreDisplacements;
    bool			m_ignoreVolumes;
    bool 			m_outputShadowPass;
    bool			m_outputHeroPass;	
    bool			m_deferredGen;
    bool			m_lazyCompute;
    bool			m_outputShadersInShadows;
    liquidlong			m_deferredBlockSize;
    bool			m_outputComments;
    bool			m_shaderDebug;
    bool			m_alfredExpand;

    long 		    	m_currentLiquidJobNumber;

    /* BMRT PARAMS: BEGIN */
    bool			m_BMRTusePrmanDisp;
    bool			m_BMRTusePrmanSpec;
    liquidlong			m_BMRTDStep;
    liquidlong			m_RadSteps;
    liquidlong			m_RadMinPatchSamples;
    /* BMRT PARAMS: END */

    MString		m_alfredTags;
    MString		m_alfredServices;

    MString		m_defGenKey;
    MString		m_defGenService;

    MString		m_preFrameMel;
    MString		m_postFrameMel;

    MString		m_renderCommand;
    MString		m_ribgenCommand;
    MString		m_preCommand;
    MString		m_postJobCommand;
    MString   	    	m_preFrameCommand;
    MString		m_postFrameCommand;
    MString		m_shaderPath;
    MString   	    	m_userAlfredFileName;

    /* Display Driver Variables */
    typedef struct structDDParam {
	    liquidlong num;
	    MStringArray names;
	    MStringArray data;
	    MIntArray type;
    } structDDParam;

    std::vector<structDDParam> m_DDParams;	

    liquidlong		m_numDisplayDrivers;
    MStringArray m_DDimageType;
    MStringArray m_DDimageMode;
    MStringArray m_DDparamType;

    liquidlong m_rFilter;
    float m_rFilterX, m_rFilterY;


    std::vector<liqShader> m_shaders;	


    liqShader & liqGetShader( MObject shaderObj );
    MStatus liqShaderParseVectorAttr ( liqShader & currentShader, MFnDependencyNode & shaderNode, const char * argName, ParameterType pType );
    void freeShaders( void );

    
};

#endif
