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

#ifndef liquid_H
#define liquid_H

/* ______________________________________________________________________
** 
** Liquid Header File
** ______________________________________________________________________
*/

#include <maya/M3dView.h>
#include <maya/MComputation.h>
#include <maya/MString.h>
#include <maya/MMatrix.h>
#include <maya/MDagPath.h>
#include <math.h>
#include <string>

#include <malloc.h>

#include <liqMemory.h>

////////////////////////
// Macros and Defines //
////////////////////////
#ifndef debugMode
extern int debugMode;
#endif

#ifndef LIQDEBUGPRINTF
#define LIQDEBUGPRINTF(msg) if( debugMode ) printf((msg));
#endif

#ifndef LIQCHECKSTATUS
#define LIQCHECKSTATUS(stat,msg) \
  if (!(stat)) { \
    stat.perror((msg)); \
    return (stat); \
  }
#endif

// Set up a textcoord type for poly uv export routine
//typedef RtFloat textcoords[2];
// this has to be added to make up for differences between linux and irix maya

/* between Maya 3.0 /4.0/Linux/Other Platforms some functions changed their input type from long to int so
a stand-in type called liquidlong was created to get around the problem */
#ifdef LINUX
typedef int liquidlong;
#else
#if MAYA_API_VERSION > 300
typedef int liquidlong;
#else
typedef long liquidlong;
#endif
#endif

// Equivalence test for floats.  Equality tests are dangerous for floating      
// point values 
//

#define FLOAT_EPSILON 0.0001
inline bool equiv( float val1, float val2 )
{
  return ( fabsf( val1 - val2 ) < FLOAT_EPSILON );
}

// Specifies how the start/end frame is set
//
#define USE_TIMESLIDER 1
#ifndef  MM_TO_INCH
#define MM_TO_INCH 0.03937
#endif 


///////////
// Enums //
///////////
enum ObjectType {
  MRT_Unknown         = 0, 
  MRT_Nurbs           = 1, 
  MRT_Mesh            = 2, 
  MRT_Light           = 3,
  MRT_Weirdo          = 4,
  MRT_NuCurve         = 5,
  MRT_Particles       = 6,
  MRT_Locator         = 7,
  MRT_RibGen          = 8,
  MRT_Shader          = 9,
  MRT_Coord           = 10,
  MRT_Subdivision     = 11,
  MRT_MayaSubdivision = 12,
  MRT_Custom          = 13
};

enum LightType {
  MRLT_Unknown  = 0, 
  MRLT_Ambient  = 1, 
  MRLT_Distant  = 2, 
  MRLT_Point	  = 3, 
  MRLT_Spot	    = 4, 
  MRLT_Rman	    = 5, 
  MRLT_Area	    = 6 
};

enum AnimType {
  MRX_Const         = 0, 
  MRX_Animated      = 1, 
  MRX_Incompatible  = 2
};

enum RendererType {
  PRMan   = 0,
  BMRT    = 1,
  RDC     = 2
};

enum PointLightDirection {
  pPX     = 0,
  pPY     = 1,
  pPZ     = 2,
  pNX     = 3,
  pNY     = 4,
  pNZ     = 5
};


enum PixelFilerType {
  fBoxFilter            = 0,
  fTriangleFilter       = 1,
  fCatmullRomFilter     = 2,
  fGaussianFilter       = 3,
  fSincFilter           = 4,
  fBlackmanHarrisFilter = 5,
  fMitchellFilter       = 6,
  fSepCatmullRomFilter  = 7
};

struct structCamera {
  MMatrix mat;
  double  neardb;
  double  fardb;
  double  hFOV;
  int     isOrtho;
  double  orthoWidth;
  double  orthoHeight;
  MString name;
  bool    motionBlur;
  double  shutter;
  double  fStop;
  double  focalDistance;
  double  focalLength;
};

struct structJob {
  int      width, height;
  float    aspectRatio;
  MString  name;
  MString  imageMode;
  MString  format;
  MString  renderName;
  MString  ribFileName;
  MString  imageName;
  bool     isShadow;
  bool     isMinMaxShadow;
  bool     isMidPointShadow;
  bool     hasShadowCam;
  bool     isShadowPass;
  bool     isPoint;
  PointLightDirection pointDir;
  structCamera camera[5];
  MDagPath path;
  MDagPath shadowCamPath;
  MString  jobOptions;
  bool     gotJobOptions;
};


#endif
