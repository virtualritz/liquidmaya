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

#ifndef liquidRibTranslator_H
#define liquidRibTranslator_H

/* ______________________________________________________________________
** 
** Liquid Rib Translator Header File
** ______________________________________________________________________
*/

#include <liquidRibHT.h>
#include <maya/MPxCommand.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnCamera.h>
#include <maya/MArgList.h>

class RibTranslator : public MPxCommand {
public:
	RibTranslator();
	~RibTranslator();
	static void *	    creator();
	
	MStatus	    doIt(const MArgList& args );
	
private: // Methods  
	
	MObject	rGlobalObj;
	
	MStatus		scanScene(float, int );
	
	void 		portFieldOfView(		int width, int height,
		double& horizontal,
		double& vertical,
		MFnCamera& fnCamera );
	
	void 		computeViewingFrustum (	double window_aspect,
		double& left,
		double& right,
		double& bottom,
		double& top,
		MFnCamera& cam );
	void		getCameraInfo( MFnCamera &cam );		
	// rib output functions
	
	MStatus liquidDoArgs( MArgList args );
	bool liquidInitGlobals();
	void liquidReadGlobals();
	
	MStatus		buildJobs();
	MStatus		ribPrologue();
	MStatus		ribEpilogue();
	MStatus		framePrologue( long );
	MStatus		frameBody();
	MStatus		frameEpilogue( long );
    void        doAttributeBlocking( const MDagPath & newPath,  
		const MDagPath & previousPath );
	void 			printProgress ( int stat );
	
private: // Data
	enum MRibStatus {
		kRibOK,
			kRibBegin, 
			kRibFrame, 
			kRibWorld, 
			kRibError
	};
	MRibStatus	  ribStatus;
	
    // Render Globals and RIB Export Options
    //
	std::vector<structJob>	jobList;
	std::vector<structJob>	shadowList;
	
	MDagPathArray shadowLightArray;
	MDagPath	  activeCamera;
	
	char *systemTempDirectory;
	
	liquidlong		  frameFirst;
	liquidlong		  frameLast;
	liquidlong		  frameBy;
	liquidlong		  width, height, depth;
	
	bool		  useNetRman;
	bool			fullShadowRib;
	bool			remoteRender;
	bool		  cleanRib;	      // clean the rib files up 
	bool		  doDof;              // do camera depth of field 
	bool		  doCameraMotion;     // Motion blur for moving cameras
	bool		  cleanShadows;
	bool		  cleanTextures;
	bool   		cleanAlf;
	bool		  useAlfred;
	bool		  doExtensionPadding;
	liquidlong		  pixelSamples;
	float		  shadingRate;
	liquidlong		  bucketSize[2];
	liquidlong		  gridSize;
	liquidlong		  textureMemory;
	liquidlong		  eyeSplits;
	bool		  renderAllCameras;   // Render all cameras, or only active ones
	bool		  ignoreFilmGate;
	double		  fov_ratio;
	int			  cam_width, cam_height;
	float		  aspectRatio;
	liquidlong 			quantValue;
	MString			renderCamera;
	MString			baseShadowName;    
	MString			alfredJobName;
	bool			createOutputDirectories;
	
	static MString magic;
	
    // Data used to construct output file names
    //
	MString 	  outFormat;
	MString 	  outFormatString;
	MString 	  outExt;
	liquidlong 		  outFormatControl;
	MString		  extension;
	MString 	  imageName;
	
	// Data used for choosing output method
	MString       riboutput;
	bool			  outputpreview;
	
    // Hash table for scene
    //
	RibHT		*htable;
    
    // Depth in attribute blocking
    //
    int         attributeDepth;
    
};

#endif
