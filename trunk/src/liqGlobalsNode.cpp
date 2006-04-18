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
** Contributor(s): Philippe Leprince.
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
** Liquid Globals Node Source
** ______________________________________________________________________
*/

#include <liquid.h>
#include <liqGlobalsNode.h>

#include <maya/MGlobal.h>
#include <maya/MCommandResult.h>
#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MSwatchRenderBase.h>
#include <maya/MSwatchRenderRegister.h>
#include <maya/MImage.h>
#include <maya/MFnDependencyNode.h>


#include <liqIOStream.h>

// static data
MTypeId liqGlobalsNode::id( 0x00103516 );

// Attributes
MObject liqGlobalsNode::aLaunchRender;

MObject liqGlobalsNode::aRenderCamera;
MObject liqGlobalsNode::aRotateCamera;

MObject liqGlobalsNode::aIgnoreAOVDisplays;
MObject liqGlobalsNode::aDdImageName;
MObject liqGlobalsNode::aDdImageType;
MObject liqGlobalsNode::aDdImageMode;
MObject liqGlobalsNode::aDdParamType;
MObject liqGlobalsNode::aDdEnable;
MObject liqGlobalsNode::aDdQuantizeEnabled;
MObject liqGlobalsNode::aDdBitDepth;
MObject liqGlobalsNode::aDdDither;
MObject liqGlobalsNode::aDdFilterEnabled;
MObject liqGlobalsNode::aDdPixelFilter;
MObject liqGlobalsNode::aDdPixelFilterX;
MObject liqGlobalsNode::aDdPixelFilterY;
MObject liqGlobalsNode::aDdXtraParamNames;
MObject liqGlobalsNode::aDdXtraParamTypes;
MObject liqGlobalsNode::aDdXtraParamDatas;
MObject liqGlobalsNode::aNumDD;
MObject liqGlobalsNode::aNumDDParam;

MObject liqGlobalsNode::aChannelName;
MObject liqGlobalsNode::aChannelType;
MObject liqGlobalsNode::aChannelQuantize;
MObject liqGlobalsNode::aChannelBitDepth;
MObject liqGlobalsNode::aChannelDither;
MObject liqGlobalsNode::aChannelFilter;
MObject liqGlobalsNode::aChannelPixelFilter;
MObject liqGlobalsNode::aChannelPixelFilterX;
MObject liqGlobalsNode::aChannelPixelFilterY;

MObject liqGlobalsNode::aCreateOutputDirectories;
MObject liqGlobalsNode::aExpandShaderArrays;

MObject liqGlobalsNode::aShaderPath;
MObject liqGlobalsNode::aTexturePath;
MObject liqGlobalsNode::aArchivePath;
MObject liqGlobalsNode::aProceduralPath;

MObject liqGlobalsNode::aRibName;
MObject liqGlobalsNode::aBeautyRibHasCameraName;

MObject liqGlobalsNode::aPictureDirectory;
MObject liqGlobalsNode::aTextureDirectory;
MObject liqGlobalsNode::aRibDirectory;
MObject liqGlobalsNode::aShaderDirectory;
MObject liqGlobalsNode::aTempDirectory;

MObject liqGlobalsNode::aDeferredGen;
MObject liqGlobalsNode::aDeferredBlock;
MObject liqGlobalsNode::aPreframeMel;
MObject liqGlobalsNode::aPostframeMel;
MObject liqGlobalsNode::aUseRenderScript;
MObject liqGlobalsNode::aRemoteRender;
MObject liqGlobalsNode::aNetRManRender;
MObject liqGlobalsNode::aMinCPU;
MObject liqGlobalsNode::aMaxCPU;
MObject liqGlobalsNode::aIgnoreShadows;
MObject liqGlobalsNode::aShapeOnlyInShadowNames;
MObject liqGlobalsNode::aFullShadowRibs;
MObject liqGlobalsNode::aBinaryOutput;
MObject liqGlobalsNode::aCompressedOutput;
MObject liqGlobalsNode::aRenderAllCurves;
MObject liqGlobalsNode::aOutputMeshUVs;
MObject liqGlobalsNode::aIgnoreSurfaces;
MObject liqGlobalsNode::aIgnoreDisplacements;
MObject liqGlobalsNode::aIgnoreLights;
MObject liqGlobalsNode::aIgnoreVolumes;
MObject liqGlobalsNode::aOutputShadersInShadows;
MObject liqGlobalsNode::aOutputShadersInDeepShadows;
MObject liqGlobalsNode::aOutputLightsInDeepShadows;

MObject liqGlobalsNode::aOutputShadowPass;
MObject liqGlobalsNode::aOutputHeroPass;
MObject liqGlobalsNode::aOutputComments;
MObject liqGlobalsNode::aShaderDebug;
MObject liqGlobalsNode::aShowProgress;
MObject liqGlobalsNode::aDoAnimation;
MObject liqGlobalsNode::aStartFrame;
MObject liqGlobalsNode::aEndFrame;
MObject liqGlobalsNode::aFrameStep;
MObject liqGlobalsNode::aDoPadding;
MObject liqGlobalsNode::aPadding;
MObject liqGlobalsNode::aNumProcs;
MObject liqGlobalsNode::aGain;
MObject liqGlobalsNode::aGamma;
MObject liqGlobalsNode::aXResolution;
MObject liqGlobalsNode::aYResolution;
MObject liqGlobalsNode::aPixelAspectRatio;
MObject liqGlobalsNode::aImageDriver;

MObject liqGlobalsNode::aCameraBlur;
MObject liqGlobalsNode::aTransformationBlur;
MObject liqGlobalsNode::aDeformationBlur;
MObject liqGlobalsNode::aShutterConfig;
MObject liqGlobalsNode::aMotionBlurSamples;
MObject liqGlobalsNode::aMotionFactor;
MObject liqGlobalsNode::aDepthOfField;

MObject liqGlobalsNode::aPixelSamples;
MObject liqGlobalsNode::aShadingRate;

MObject liqGlobalsNode::aLimitsOThreshold;
MObject liqGlobalsNode::aLimitsZThreshold;
MObject liqGlobalsNode::aLimitsBucketXSize;
MObject liqGlobalsNode::aLimitsBucketYSize;
MObject liqGlobalsNode::aLimitsGridSize;
MObject liqGlobalsNode::aLimitsTextureMemory;
MObject liqGlobalsNode::aLimitsEyeSplits;
MObject liqGlobalsNode::aLimitsGPrimSplits;

MObject liqGlobalsNode::aCleanRib;
MObject liqGlobalsNode::aCleanTex;
MObject liqGlobalsNode::aCleanShad;
MObject liqGlobalsNode::aCleanRenderScript;
MObject liqGlobalsNode::aJustRib;
MObject liqGlobalsNode::aAlfredTags;
MObject liqGlobalsNode::aAlfredServices;
MObject liqGlobalsNode::aRenderCommand;
MObject liqGlobalsNode::aRibgenCommand;

MObject liqGlobalsNode::aPreviewer;
MObject liqGlobalsNode::aPreCommand;
MObject liqGlobalsNode::aPostFrameCommand;
MObject liqGlobalsNode::aPreFrameCommand;
MObject liqGlobalsNode::aPreJobCommand;
MObject liqGlobalsNode::aPostJobCommand;
MObject liqGlobalsNode::aKey;
MObject liqGlobalsNode::aService;
MObject liqGlobalsNode::aLastRenderScript;
MObject liqGlobalsNode::aLastRibFile;
MObject liqGlobalsNode::aSimpleGlobalsWindow;
MObject liqGlobalsNode::aLazyCompute;
MObject liqGlobalsNode::aCropX1;
MObject liqGlobalsNode::aCropX2;
MObject liqGlobalsNode::aCropY1;
MObject liqGlobalsNode::aCropY2;
MObject liqGlobalsNode::aExportReadArchive;
MObject liqGlobalsNode::aRenderJobName;
MObject liqGlobalsNode::aShortShaderNames;

MObject liqGlobalsNode::aRelativeFileNames;

MObject liqGlobalsNode::aExpandAlfred;

MObject liqGlobalsNode::aPreFrameBegin;
MObject liqGlobalsNode::aPreWorld;
MObject liqGlobalsNode::aPostWorld;
MObject liqGlobalsNode::aPreGeom;

MObject liqGlobalsNode::aRenderScriptFormat;
MObject liqGlobalsNode::aRenderScriptCommand;

MObject liqGlobalsNode::aFluidShaderBrowserDefaultPath;
MObject liqGlobalsNode::aPreviewType;
MObject liqGlobalsNode::aPreviewRenderer;
MObject liqGlobalsNode::aPreviewSize;
MObject liqGlobalsNode::aPreviewPrimitive;
MObject liqGlobalsNode::aPreviewDisplayDriver;
MObject liqGlobalsNode::aPreviewConnectionType;
MObject liqGlobalsNode::aRenderViewLocal;
MObject liqGlobalsNode::aRenderViewPort;
MObject liqGlobalsNode::aRenderViewTimeOut;

MObject liqGlobalsNode::aUseRayTracing;
MObject liqGlobalsNode::aTraceBreadthFactor;
MObject liqGlobalsNode::aTraceDepthFactor;
MObject liqGlobalsNode::aTraceMaxDepth;
MObject liqGlobalsNode::aTraceSpecularThreshold;
MObject liqGlobalsNode::aTraceRayContinuation;
MObject liqGlobalsNode::aTraceCacheMemory;
MObject liqGlobalsNode::aTraceDisplacements;
MObject liqGlobalsNode::aTraceBias;
MObject liqGlobalsNode::aTraceSampleMotion;
MObject liqGlobalsNode::aTraceMaxSpecularDepth;
MObject liqGlobalsNode::aTraceMaxDiffuseDepth;

MObject liqGlobalsNode::aIrradianceMaxError;
MObject liqGlobalsNode::aIrradianceMaxPixelDist;
MObject liqGlobalsNode::aIrradianceHandle;
MObject liqGlobalsNode::aIrradianceFileMode;

MObject liqGlobalsNode::aUseMtorSubdiv;
MObject liqGlobalsNode::aHider;
MObject liqGlobalsNode::aJitter;
MObject liqGlobalsNode::aHiddenOcclusionBound;
MObject liqGlobalsNode::aHiddenMpCache;
MObject liqGlobalsNode::aHiddenMpMemory;
MObject liqGlobalsNode::aHiddenMpCacheDir;
MObject liqGlobalsNode::aHiddenSampleMotion;
MObject liqGlobalsNode::aHiddenSubPixel;
MObject liqGlobalsNode::aHiddenExtremeMotionDof;
MObject liqGlobalsNode::aHiddenMaxVPDepth;

MObject liqGlobalsNode::aDepthMaskZFile;
MObject liqGlobalsNode::aDepthMaskReverseSign;
MObject liqGlobalsNode::aDepthMaskDepthBias;

MObject liqGlobalsNode::aRenderCmdFlags;
MObject liqGlobalsNode::aShaderInfo;
MObject liqGlobalsNode::aShaderComp;
MObject liqGlobalsNode::aShaderExt;
MObject liqGlobalsNode::aMakeTexture;
MObject liqGlobalsNode::aViewTexture;

MObject liqGlobalsNode::aBits_hiders;
MObject liqGlobalsNode::aBits_hiders_0;
MObject liqGlobalsNode::aBits_hiders_1;
MObject liqGlobalsNode::aBits_hiders_2;
MObject liqGlobalsNode::aBits_hiders_3;
MObject liqGlobalsNode::aBits_hiders_4;
MObject liqGlobalsNode::aBits_hiders_5;

MObject liqGlobalsNode::aBits_filters;
MObject liqGlobalsNode::aBits_filters_0;
MObject liqGlobalsNode::aBits_filters_1;
MObject liqGlobalsNode::aBits_filters_2;
MObject liqGlobalsNode::aBits_filters_3;
MObject liqGlobalsNode::aBits_filters_4;
MObject liqGlobalsNode::aBits_filters_5;
MObject liqGlobalsNode::aBits_filters_6;
MObject liqGlobalsNode::aBits_filters_7;
MObject liqGlobalsNode::aBits_filters_8;
MObject liqGlobalsNode::aBits_filters_9;
MObject liqGlobalsNode::aBits_filters_10;

MObject liqGlobalsNode::aBits_features;
MObject liqGlobalsNode::aBits_features_0;
MObject liqGlobalsNode::aBits_features_1;
MObject liqGlobalsNode::aBits_features_2;
MObject liqGlobalsNode::aBits_features_3;
MObject liqGlobalsNode::aBits_features_4;
MObject liqGlobalsNode::aBits_features_5;
MObject liqGlobalsNode::aBits_features_6;

MObject liqGlobalsNode::aBits_required;
MObject liqGlobalsNode::aBits_required_0;
MObject liqGlobalsNode::aBits_required_1;
MObject liqGlobalsNode::aBits_required_2;

MObject liqGlobalsNode::aDshDisplayName;
MObject liqGlobalsNode::aDshImageMode;

MObject liqGlobalsNode::aShotName;
MObject liqGlobalsNode::aShotVersion;

MObject liqGlobalsNode::aStatistics;




#define CREATE_BOOL(attr, obj, name, shortName, default)    \
    obj = attr.create( name, shortName, MFnNumericData::kBoolean, default, &status); \
    CHECK_MSTATUS(attr.setKeyable(true));     \
    CHECK_MSTATUS(attr.setStorable(true));    \
    CHECK_MSTATUS(attr.setReadable(true));    \
    CHECK_MSTATUS(attr.setWritable(true));    \
    CHECK_MSTATUS(attr.setHidden(true));      \
    CHECK_MSTATUS(addAttribute(obj));

#define CREATE_MULTI_BOOL(attr, obj, name, shortName, default)    \
    obj = attr.create( name, shortName, MFnNumericData::kBoolean, default, &status); \
    CHECK_MSTATUS(attr.setArray(true));       \
    CHECK_MSTATUS(attr.setKeyable(true));     \
    CHECK_MSTATUS(attr.setStorable(true));    \
    CHECK_MSTATUS(attr.setReadable(true));    \
    CHECK_MSTATUS(attr.setWritable(true));    \
    CHECK_MSTATUS(attr.setHidden(true));      \
    CHECK_MSTATUS(addAttribute(obj));

#define CREATE_INT(attr, obj, name, shortName, default)    \
    obj = attr.create( name, shortName, MFnNumericData::kInt, default, &status); \
    CHECK_MSTATUS(attr.setKeyable(true));     \
    CHECK_MSTATUS(attr.setStorable(true));    \
    CHECK_MSTATUS(attr.setReadable(true));    \
    CHECK_MSTATUS(attr.setWritable(true));    \
    CHECK_MSTATUS(attr.setHidden(true));      \
    CHECK_MSTATUS(addAttribute(obj));

#define CREATE_MULTI_INT(attr, obj, name, shortName, default)    \
    obj = attr.create( name, shortName, MFnNumericData::kInt, default, &status); \
    CHECK_MSTATUS(attr.setArray(true));       \
    CHECK_MSTATUS(attr.setKeyable(true));     \
    CHECK_MSTATUS(attr.setStorable(true));    \
    CHECK_MSTATUS(attr.setReadable(true));    \
    CHECK_MSTATUS(attr.setWritable(true));    \
    CHECK_MSTATUS(attr.setHidden(true));      \
    CHECK_MSTATUS(addAttribute(obj));

#define CREATE_LONG(attr, obj, name, shortName, default)    \
    obj = attr.create( name, shortName, MFnNumericData::kLong, default, &status); \
    CHECK_MSTATUS(attr.setKeyable(true));     \
    CHECK_MSTATUS(attr.setStorable(true));    \
    CHECK_MSTATUS(attr.setReadable(true));    \
    CHECK_MSTATUS(attr.setWritable(true));    \
    CHECK_MSTATUS(attr.setHidden(true));      \
    CHECK_MSTATUS(addAttribute(obj));

#define CREATE_MULTI_LONG(attr, obj, name, shortName, default)    \
    obj = attr.create( name, shortName, MFnNumericData::kLong, default, &status); \
    CHECK_MSTATUS(attr.setArray(true));       \
    CHECK_MSTATUS(attr.setKeyable(true));     \
    CHECK_MSTATUS(attr.setStorable(true));    \
    CHECK_MSTATUS(attr.setReadable(true));    \
    CHECK_MSTATUS(attr.setWritable(true));    \
    CHECK_MSTATUS(attr.setHidden(true));      \
    CHECK_MSTATUS(addAttribute(obj));

#define CREATE_FLOAT(attr, obj, name, shortName, default)    \
    obj = attr.create( name, shortName, MFnNumericData::kFloat, default, &status); \
    CHECK_MSTATUS(attr.setKeyable(true));     \
    CHECK_MSTATUS(attr.setStorable(true));    \
    CHECK_MSTATUS(attr.setReadable(true));    \
    CHECK_MSTATUS(attr.setWritable(true));    \
    CHECK_MSTATUS(attr.setHidden(true));      \
    CHECK_MSTATUS(addAttribute(obj));

#define CREATE_MULTI_FLOAT(attr, obj, name, shortName, default)    \
    obj = attr.create( name, shortName, MFnNumericData::kFloat, default, &status); \
    CHECK_MSTATUS(attr.setArray(true));       \
    CHECK_MSTATUS(attr.setKeyable(true));     \
    CHECK_MSTATUS(attr.setStorable(true));    \
    CHECK_MSTATUS(attr.setReadable(true));    \
    CHECK_MSTATUS(attr.setWritable(true));    \
    CHECK_MSTATUS(attr.setHidden(true));      \
    CHECK_MSTATUS(addAttribute(obj));

#define CREATE_STRING(attr, obj, name, shortName, default)    \
    obj = attr.create( name, shortName, MFnData::kString, obj, &status); \
    CHECK_MSTATUS(attr.setKeyable(true));     \
    CHECK_MSTATUS(attr.setStorable(true));    \
    CHECK_MSTATUS(attr.setReadable(true));    \
    CHECK_MSTATUS(attr.setWritable(true));    \
    CHECK_MSTATUS(attr.setHidden(true));      \
    CHECK_MSTATUS(addAttribute(obj));

#define CREATE_MULTI_STRING(attr, obj, name, shortName, default)    \
    obj = attr.create( name, shortName, MFnData::kString, obj, &status); \
    CHECK_MSTATUS(attr.setArray(true));       \
    CHECK_MSTATUS(attr.setKeyable(true));     \
    CHECK_MSTATUS(attr.setStorable(true));    \
    CHECK_MSTATUS(attr.setReadable(true));    \
    CHECK_MSTATUS(attr.setWritable(true));    \
    CHECK_MSTATUS(attr.setHidden(true));      \
    CHECK_MSTATUS(addAttribute(obj));

#define CREATE_COMP(attr, obj, name, shortName)    \
    obj = attr.create( name, shortName, &status); \
    CHECK_MSTATUS(attr.setStorable(true));    \
    CHECK_MSTATUS(attr.setReadable(true));    \
    CHECK_MSTATUS(attr.setWritable(true));    \
    CHECK_MSTATUS(attr.setHidden(true));      \
    CHECK_MSTATUS(addAttribute(obj));

#define CREATE_MULTI_STR_ARRAY(attr, obj, name, shortName)    \
    obj = attr.create( name, shortName, MFnData::kStringArray, obj, &status); \
    CHECK_MSTATUS(attr.setArray(true));       \
    CHECK_MSTATUS(attr.setKeyable(true));     \
    CHECK_MSTATUS(attr.setStorable(true));    \
    CHECK_MSTATUS(attr.setReadable(true));    \
    CHECK_MSTATUS(attr.setWritable(true));    \
    CHECK_MSTATUS(attr.setHidden(true));      \
    CHECK_MSTATUS(addAttribute(obj));

#define CREATE_MULTI_INT_ARRAY(attr, obj, name, shortName)    \
    obj = attr.create( name, shortName, MFnData::kIntArray, obj, &status); \
    CHECK_MSTATUS(attr.setArray(true));       \
    CHECK_MSTATUS(attr.setKeyable(true));     \
    CHECK_MSTATUS(attr.setStorable(true));    \
    CHECK_MSTATUS(attr.setReadable(true));    \
    CHECK_MSTATUS(attr.setWritable(true));    \
    CHECK_MSTATUS(attr.setHidden(true));      \
    CHECK_MSTATUS(addAttribute(obj));



liqGlobalsNode::liqGlobalsNode()
{
}

liqGlobalsNode::~liqGlobalsNode()
{
}

void* liqGlobalsNode::creator()
{
    return new liqGlobalsNode();
}

MStatus liqGlobalsNode::initialize()
{
  MFnTypedAttribute     tAttr;
  MFnNumericAttribute   nAttr;
  MFnCompoundAttribute  cAttr;

  MStatus status;

  // Create input attributes
            CREATE_BOOL( nAttr,  aLaunchRender,               "launchRender",                 "lr",     1     );
          CREATE_STRING( tAttr,  aRenderCamera,               "renderCamera",                 "rc",     ""    );
            CREATE_BOOL( nAttr,  aRotateCamera,               "rotateCamera",                 "roc",    0     );
            CREATE_BOOL( nAttr,  aIgnoreAOVDisplays,          "ignoreAOVDisplays",            "iaov",   0     );

    CREATE_MULTI_STRING( tAttr,  aDdImageName,                "ddImageName",                  "ddin",   ""    );
    CREATE_MULTI_STRING( tAttr,  aDdImageMode,                "ddImageMode",                  "ddim",   ""    );
    CREATE_MULTI_STRING( tAttr,  aDdImageType,                "ddImageType",                  "ddit",   ""    );
    CREATE_MULTI_STRING( tAttr,  aDdParamType,                "ddParamType",                  "ddpt",   ""    );
      CREATE_MULTI_BOOL( nAttr,  aDdEnable,                   "ddEnable",                     "dde",    1     );
      CREATE_MULTI_BOOL( nAttr,  aDdQuantizeEnabled,          "ddQuantizeEnabled",            "ddqe",   0     );
       CREATE_MULTI_INT( nAttr,  aDdBitDepth,                 "ddBitDepth",                   "ddbd",   0     );
     CREATE_MULTI_FLOAT( nAttr,  aDdDither,                   "ddDither",                     "ddd",    0     );
      CREATE_MULTI_BOOL( nAttr,  aDdFilterEnabled,            "ddFilterEnabled",              "ddfe",   0     );
       CREATE_MULTI_INT( nAttr,  aDdPixelFilter,              "ddPixelFilter",                "ddpf",   0     );
     CREATE_MULTI_FLOAT( nAttr,  aDdPixelFilterX,             "ddPixelFilterX",               "ddpfx",  2.0   );
     CREATE_MULTI_FLOAT( nAttr,  aDdPixelFilterY,             "ddPixelFilterY",               "ddpfy",  2.0   );
 CREATE_MULTI_STR_ARRAY( tAttr,  aDdXtraParamNames,           "ddXtraParamNames",             "ddxpn"         );
 CREATE_MULTI_INT_ARRAY( tAttr,  aDdXtraParamTypes,           "ddXtraParamTypes",             "ddxpt"         );
 CREATE_MULTI_STR_ARRAY( tAttr,  aDdXtraParamDatas,           "ddXtraParamDatas",             "ddxpd"         );

  CREATE_MULTI_STRING( tAttr,  aChannelName,                "channelName",                  "dcn",    ""    );
     CREATE_MULTI_INT( nAttr,  aChannelType,                "channelType",                  "dct",    0     );
    CREATE_MULTI_BOOL( nAttr,  aChannelQuantize,            "channelQuantize",              "dcq",    0     );
     CREATE_MULTI_INT( nAttr,  aChannelBitDepth,            "channelBitDepth",              "dcbd",   8     );
   CREATE_MULTI_FLOAT( nAttr,  aChannelDither,              "channelDither",                "dcd",    0.5   );
    CREATE_MULTI_BOOL( nAttr,  aChannelFilter,              "channelFilter",                "dcf",    0     );
     CREATE_MULTI_INT( nAttr,  aChannelPixelFilter,         "channelPixelFilter",           "dcpf",   0     );
   CREATE_MULTI_FLOAT( nAttr,  aChannelPixelFilterX,        "channelPixelFilterX",          "dcfx",   0.0   );
   CREATE_MULTI_FLOAT( nAttr,  aChannelPixelFilterY,        "channelPixelFilterY",          "dcfy",   0.0   );

          CREATE_BOOL( nAttr,  aCreateOutputDirectories,    "createOutputDirectories",      "cod",    1     );
          CREATE_BOOL( nAttr,  aExpandShaderArrays,         "expandShaderArrays",           "esa",    1     );

        CREATE_STRING( tAttr,  aShaderPath,                 "shaderPath",                   "spth",   ""    );
        CREATE_STRING( tAttr,  aTexturePath,                "texturePath",                  "tpth",   ""    );
        CREATE_STRING( tAttr,  aArchivePath,                "archivePath",                  "apth",   ""    );
        CREATE_STRING( tAttr,  aProceduralPath,             "proceduralPath",               "ppth",   ""    );

        CREATE_STRING( tAttr,  aRibName,                    "ribName",                      "ribn",   ""    );
          CREATE_BOOL( nAttr,  aBeautyRibHasCameraName,     "beautyRibHasCameraName",       "bhcn",   1     );

        CREATE_STRING( tAttr,  aPictureDirectory,           "pictureDirectory",             "picd",   ""    );
        CREATE_STRING( tAttr,  aTextureDirectory,           "textureDirectory",             "texd",   ""    );
        CREATE_STRING( tAttr,  aRibDirectory,               "ribDirectory",                 "ribd",   ""    );
        CREATE_STRING( tAttr,  aShaderDirectory,            "shaderDirectory",              "shdd",   ""    );
        CREATE_STRING( tAttr,  aTempDirectory,              "tempDirectory",                "tmpd",   ""    );

          CREATE_BOOL( nAttr,  aDeferredGen,                "deferredGen",                  "defg",   0     );
           CREATE_INT( nAttr,  aDeferredBlock,              "deferredBlock",                "defb",   1     );
        CREATE_STRING( tAttr,  aPreframeMel,                "preframeMel",                  "prfm",   ""    );
        CREATE_STRING( tAttr,  aPostframeMel,               "postframeMel",                 "pofm",   ""    );
          CREATE_BOOL( nAttr,  aUseRenderScript,            "useRenderScript",              "urs",    0     );
          CREATE_BOOL( nAttr,  aRemoteRender,               "remoteRender",                 "rr",     0     );
          CREATE_BOOL( nAttr,  aNetRManRender,              "netRManRender",                "nrr",    0     );
           CREATE_INT( nAttr,  aMinCPU,                     "minCPU",                       "min",    1     );
           CREATE_INT( nAttr,  aMaxCPU,                     "maxCPU",                       "max",    1     );
          CREATE_BOOL( nAttr,  aIgnoreShadows,              "ignoreShadows",                "ish",    0     );
          CREATE_BOOL( nAttr,  aShapeOnlyInShadowNames,     "shapeOnlyInShadowNames",       "sosn",   0     );
          CREATE_BOOL( nAttr,  aFullShadowRibs,             "fullShadowRibs",               "fsr",    0     );
          CREATE_BOOL( nAttr,  aBinaryOutput,               "binaryOutput",                 "bin",    0     );
          CREATE_BOOL( nAttr,  aCompressedOutput,           "compressedOutput",             "comp",   0     );
          CREATE_BOOL( nAttr,  aRenderAllCurves,            "renderAllCurves",              "rac",    0     );
          CREATE_BOOL( nAttr,  aOutputMeshUVs,              "outputMeshUVs",                "muv",    0     );
          CREATE_BOOL( nAttr,  aIgnoreSurfaces,             "ignoreSurfaces",               "isrf",   0     );
          CREATE_BOOL( nAttr,  aIgnoreDisplacements,        "ignoreDisplacements",          "idsp",   0     );
          CREATE_BOOL( nAttr,  aIgnoreLights,               "ignoreLights",                 "ilgt",   0     );
          CREATE_BOOL( nAttr,  aIgnoreVolumes,              "ignoreVolumes",                "ivol",   0     );
          CREATE_BOOL( nAttr,  aOutputShadersInShadows,     "outputShadersInShadows",       "osis",   0     );
          CREATE_BOOL( nAttr,  aOutputShadersInDeepShadows, "outputShadersInDeepShadows",   "osids",  0     );
          CREATE_BOOL( nAttr,  aOutputLightsInDeepShadows,  "outputLightsInDeepShadows",    "olids",  0     );

          CREATE_BOOL( nAttr,  aOutputShadowPass,           "outputShadowPass",             "osp",    0     );
          CREATE_BOOL( nAttr,  aOutputHeroPass,             "outputHeroPass",               "ohp",    1     );
          CREATE_BOOL( nAttr,  aOutputComments,             "outputComments",               "oc",     0     );
          CREATE_BOOL( nAttr,  aShaderDebug,                "shaderDebug",                  "sdbg",   0     );
          CREATE_BOOL( nAttr,  aShowProgress,               "showProgress",                 "prog",   0     );
          CREATE_BOOL( nAttr,  aDoAnimation,                "doAnimation",                  "anim",   0     );
           CREATE_INT( nAttr,  aStartFrame,                 "startFrame",                   "sf",     1     );
           CREATE_INT( nAttr,  aEndFrame,                   "endFrame",                     "ef",     100   );
           CREATE_INT( nAttr,  aFrameStep,                  "frameStep",                    "fs",     1     );
          CREATE_BOOL( nAttr,  aDoPadding,                  "doPadding",                    "dpad",   0     );
           CREATE_INT( nAttr,  aPadding,                    "padding",                      "pad",    4     );
           CREATE_INT( nAttr,  aNumProcs,                   "numProcs",                     "np",     0     );
         CREATE_FLOAT( nAttr,  aGain,                       "gain",                         "gn",     1.0   );
         CREATE_FLOAT( nAttr,  aGamma,                      "gamma",                        "gm",     1.0   );
           CREATE_INT( nAttr,  aXResolution,                "xResolution",                  "xres",   640   );
           CREATE_INT( nAttr,  aYResolution,                "yResolution",                  "yres",   480   );
         CREATE_FLOAT( nAttr,  aPixelAspectRatio,           "pixelAspectRatio",             "par",    1.0   );

          CREATE_BOOL( nAttr,  aCameraBlur,                 "cameraBlur",                   "cb",     0     );
          CREATE_BOOL( nAttr,  aTransformationBlur,         "transformationBlur",           "tb",     0     );
          CREATE_BOOL( nAttr,  aDeformationBlur,            "deformationBlur",              "db",     0     );
           CREATE_INT( nAttr,  aShutterConfig,              "shutterConfig",                "shc",    0     );
           CREATE_INT( nAttr,  aMotionBlurSamples,          "motionBlurSamples",            "mbs",    2     );
         CREATE_FLOAT( nAttr,  aMotionFactor,               "motionFactor",                 "mf",     1.0   );
          CREATE_BOOL( nAttr,  aDepthOfField,               "depthOfField",                 "dof",    0     );

           CREATE_INT( nAttr,  aPixelSamples,               "pixelSamples",                 "ps",     4     );
         CREATE_FLOAT( nAttr,  aShadingRate,                "shadingRate",                  "sr",     1.0   );

         CREATE_FLOAT( nAttr,  aLimitsOThreshold,           "limitsOThreshold",             "lot",    0.99  );
         CREATE_FLOAT( nAttr,  aLimitsZThreshold,           "limitsZThreshold",             "lzt",    1.0   );
           CREATE_INT( nAttr,  aLimitsBucketXSize,          "limitsBucketXSize",            "lbsx",   16    );
           CREATE_INT( nAttr,  aLimitsBucketYSize,          "limitsBucketYSize",            "lbsy",   16    );
           CREATE_INT( nAttr,  aLimitsGridSize,             "limitsGridSize",               "lgs",    256   );
          CREATE_LONG( nAttr,  aLimitsTextureMemory,        "limitsTextureMemory",          "ltm",    65536 );
           CREATE_INT( nAttr,  aLimitsEyeSplits,            "limitsEyeSplits",              "les",    10    );
           CREATE_INT( nAttr,  aLimitsGPrimSplits,          "limitsGPrimSplits",            "lges",   4    );

          CREATE_BOOL( nAttr,  aCleanRib,                   "cleanRib",                     "clr",    0     );
          CREATE_BOOL( nAttr,  aCleanTex,                   "cleanTex",                     "clt",    0     );
          CREATE_BOOL( nAttr,  aCleanShad,                  "cleanShad",                    "cls",    0     );
          CREATE_BOOL( nAttr,  aCleanRenderScript,          "cleanRenderScript",            "clrs",   0     );
          CREATE_BOOL( nAttr,  aJustRib,                    "justRib",                      "jr",     0     );

        CREATE_STRING( tAttr,  aAlfredTags,                 "alfredTags",                   "alft",   ""    );
        CREATE_STRING( tAttr,  aAlfredServices,             "alfredServices",               "alfs",   ""    );
        CREATE_STRING( tAttr,  aRenderCommand,              "renderCommand",                "rdc",    ""    );
        CREATE_STRING( tAttr,  aRibgenCommand,              "ribgenCommand",                "rgc",    ""    );

        CREATE_STRING( tAttr,  aPreviewer,                  "previewer",                    "prv",    ""    );
        CREATE_STRING( tAttr,  aPreCommand,                 "preCommand",                   "prc",    ""    );
        CREATE_STRING( tAttr,  aPostFrameCommand,           "postFrameCommand",             "pofc",   ""    );
        CREATE_STRING( tAttr,  aPreFrameCommand,            "preFrameCommand",              "prfc",   ""    );
        CREATE_STRING( tAttr,  aPreJobCommand,              "preJobCommand",                "prjc",   ""    );
        CREATE_STRING( tAttr,  aPostJobCommand,             "postJobCommand",               "pojc",   ""    );
        CREATE_STRING( tAttr,  aKey,                        "key",                          "k",      ""    );
        CREATE_STRING( tAttr,  aService,                    "service",                      "srv",    ""    );
        CREATE_STRING( tAttr,  aLastRenderScript,           "lastRenderScript",             "lrs",    ""    );
        CREATE_STRING( tAttr,  aLastRibFile,                "lastRibFile",                  "lrf",    ""    );

          CREATE_BOOL( nAttr,  aSimpleGlobalsWindow,        "simpleGlobalsWindow",          "sgw",    0     );
          CREATE_BOOL( nAttr,  aLazyCompute,                "lazyCompute",                  "lc",     0     );
         CREATE_FLOAT( nAttr,  aCropX1,                     "cropX1",                       "cx1",    0.0   );
         CREATE_FLOAT( nAttr,  aCropX2,                     "cropX2",                       "cx2",    1.0   );
         CREATE_FLOAT( nAttr,  aCropY1,                     "cropY1",                       "cy1",    0.0   );
         CREATE_FLOAT( nAttr,  aCropY2,                     "cropY2",                       "cy2",    1.0   );
          CREATE_BOOL( nAttr,  aExportReadArchive,          "exportReadArchive",            "era",    0     );
        CREATE_STRING( tAttr,  aRenderJobName,              "renderJobName",                "rjn",    ""    );
          CREATE_BOOL( nAttr,  aShortShaderNames,           "shortShaderNames",             "ssn",    0     );

          CREATE_BOOL( nAttr,  aRelativeFileNames,          "relativeFileNames",            "rfn",    0     );
          CREATE_BOOL( nAttr,  aExpandAlfred,               "expandAlfred",                 "ea",     0     );

        CREATE_STRING( tAttr,  aPreFrameBegin,              "preFrameBegin",                "prfb",   ""    );
        CREATE_STRING( tAttr,  aPreWorld,                   "preWorld",                     "prw",    ""    );
        CREATE_STRING( tAttr,  aPostWorld,                  "postWorld",                    "pow",    ""    );
        CREATE_STRING( tAttr,  aPreGeom,                    "preGeom",                      "prg",    ""    );

           CREATE_INT( nAttr,  aRenderScriptFormat,         "renderScriptFormat",           "rsf",    2     );
        CREATE_STRING( tAttr,  aRenderScriptCommand,        "renderScriptCommand",          "rsc",    ""    );

        CREATE_STRING( tAttr,  aFluidShaderBrowserDefaultPath,  "fluidShaderBrowserDefaultPath",  "fsbdp",  ""  );

           CREATE_INT( nAttr,  aPreviewType,                "previewType",                  "prt",    0     );
        CREATE_STRING( tAttr,  aPreviewRenderer,            "previewRenderer",              "prr",    ""    );
           CREATE_INT( nAttr,  aPreviewSize,                "previewSize",                  "prs",    128   );
           CREATE_INT( nAttr,  aPreviewPrimitive,           "previewPrimitive",             "prp",    0     );
        CREATE_STRING( tAttr,  aPreviewDisplayDriver,       "previewDisplayDriver",         "prdd",   ""    );
           CREATE_INT( nAttr,  aPreviewConnectionType,      "previewConnectionType",        "prct",   0     );
          CREATE_BOOL( nAttr,  aRenderViewLocal,            "renderViewLocal",              "rvl",    1     );
          CREATE_LONG( nAttr,  aRenderViewPort,             "renderViewPort",               "rvp",    6667  );
           CREATE_INT( nAttr,  aRenderViewTimeOut,          "renderViewTimeOut",            "rvto",   20    );

          CREATE_BOOL( nAttr,  aUseRayTracing,              "useRayTracing",                "ray",    0     );
         CREATE_FLOAT( nAttr,  aTraceBreadthFactor,         "traceBreadthFactor",           "trbf",    1.0   );
         CREATE_FLOAT( nAttr,  aTraceDepthFactor,           "traceDepthFactor",             "trdf",    1.0   );
           CREATE_INT( nAttr,  aTraceMaxDepth,              "traceMaxDepth",                "trmd",    10    );
         CREATE_FLOAT( nAttr,  aTraceSpecularThreshold,     "traceSpecularThreshold",       "trst",    10.0  );
          CREATE_BOOL( nAttr,  aTraceRayContinuation,       "traceRayContinuation",         "trrc",    1     );
          CREATE_LONG( nAttr,  aTraceCacheMemory,           "traceCacheMemory",             "trcm",    30720 );
          CREATE_BOOL( nAttr,  aTraceDisplacements,         "traceDisplacements",           "trd",     0     );
         CREATE_FLOAT( nAttr,  aTraceBias,                  "traceBias",                    "trb",     0.05  );
          CREATE_BOOL( nAttr,  aTraceSampleMotion,          "traceSampleMotion",            "tsm",    0     );
           CREATE_INT( nAttr,  aTraceMaxSpecularDepth,      "traceMaxSpecularDepth",        "trmsd",   2     );
           CREATE_INT( nAttr,  aTraceMaxDiffuseDepth,       "traceMaxDiffuseDepth",         "trmdd",   2     );

         CREATE_FLOAT( nAttr,  aIrradianceMaxError,         "irradianceMaxError",           "ime",    -1.0  );
         CREATE_FLOAT( nAttr,  aIrradianceMaxPixelDist,     "irradianceMaxPixelDist",       "impd",   -1.0  );
        CREATE_STRING( tAttr,  aIrradianceHandle,           "irradianceHandle",             "ih",     ""    );
           CREATE_INT( nAttr,  aIrradianceFileMode,         "irradianceFileMode",           "ifm",    0     );

          CREATE_BOOL( nAttr,  aUseMtorSubdiv,              "useMtorSubdiv",                "ums",    0     );

           CREATE_INT( nAttr,  aHider,                      "hider",                        "h",      0     );
           // "hidden" hider advanced options - PRMAN ONLY
           CREATE_INT( nAttr,  aJitter,                     "jitter",                       "j",      0     );
         CREATE_FLOAT( nAttr,  aHiddenOcclusionBound,       "hiddenOcclusionBound",         "hob",    0.0   );
          CREATE_BOOL( nAttr,  aHiddenMpCache,              "hiddenMpCache",                "hmpc",   1     );
           CREATE_INT( nAttr,  aHiddenMpMemory,             "hiddenMpMemory",               "hmpm",   6144  );
        CREATE_STRING( tAttr,  aHiddenMpCacheDir,           "hiddenMpCacheDir",             "hmcd",   ""   );
          CREATE_BOOL( nAttr,  aHiddenSampleMotion,         "hiddenSampleMotion",           "hsm",    1     );
           CREATE_INT( nAttr,  aHiddenSubPixel,             "hiddenSubPixel",               "hsp",    1     );
          CREATE_BOOL( nAttr,  aHiddenExtremeMotionDof,     "hiddenExtremeMotionDof",       "hemd",   0     );
           CREATE_INT( nAttr,  aHiddenMaxVPDepth,           "hiddenMaxVPDepth",             "hmvd",  -1     );

        CREATE_STRING( tAttr,  aDepthMaskZFile,             "depthMaskZFile",               "dmzf",   ""    );
          CREATE_BOOL( nAttr,  aDepthMaskReverseSign,       "depthMaskReverseSign",         "dmrs",   0     );
         CREATE_FLOAT( nAttr,  aDepthMaskDepthBias,         "depthMaskDepthBias",           "dmdb",   0.01  );


        CREATE_STRING( tAttr,  aRenderCmdFlags,             "renderCmdFlags",               "rcf",    ""    );
        CREATE_STRING( tAttr,  aShaderInfo,                 "shaderInfo",                   "shi",    ""    );
        CREATE_STRING( tAttr,  aShaderComp,                 "shaderComp",                   "shcp",   ""    );
        CREATE_STRING( tAttr,  aShaderExt,                  "shaderExt",                    "she",    ""    );
        CREATE_STRING( tAttr,  aMakeTexture,                "makeTexture",                  "mtx",    ""    );
        CREATE_STRING( tAttr,  aViewTexture,                "viewTexture",                  "vtx",    ""    );

        CREATE_STRING( tAttr,  aDshDisplayName,             "dshDisplayName",               "dsdn",   ""    );
        CREATE_STRING( tAttr,  aDshImageMode,               "dshImageMode",                 "dsim",   ""    );

           CREATE_INT( nAttr,  aStatistics,                 "statistics",                   "st",     0     );



  CREATE_COMP( cAttr, aBits_hiders, "bits_hiders", "bhid" );
    CREATE_BOOL( nAttr, aBits_hiders_0, "Hidden", "Hidden", 1 );
    CHECK_MSTATUS( cAttr.addChild( aBits_hiders_0 ) );
    CREATE_BOOL( nAttr, aBits_hiders_1, "Photon", "Photon", 0 );
    CHECK_MSTATUS( cAttr.addChild( aBits_hiders_1 ) );
    CREATE_BOOL( nAttr, aBits_hiders_2, "ZBuffer", "ZBuffer", 1 );
    CHECK_MSTATUS( cAttr.addChild( aBits_hiders_2 ) );
    CREATE_BOOL( nAttr, aBits_hiders_3, "Raytrace", "Raytrace", 0 );
    CHECK_MSTATUS( cAttr.addChild( aBits_hiders_3 ) );
    CREATE_BOOL( nAttr, aBits_hiders_4, "OpenGL", "OpenGL", 0 );
    CHECK_MSTATUS( cAttr.addChild( aBits_hiders_4 ) );
    CREATE_BOOL( nAttr, aBits_hiders_5, "DepthMask", "DepthMask", 0 );
    CHECK_MSTATUS( cAttr.addChild( aBits_hiders_5 ) );

    CREATE_COMP( cAttr, aBits_filters, "bits_filters", "bfil" );
      CREATE_BOOL( nAttr, aBits_filters_0, "Box", "Box", 1 );
      CHECK_MSTATUS( cAttr.addChild( aBits_filters_0 ) );
      CREATE_BOOL( nAttr, aBits_filters_1, "Triangle", "Triangle", 1 );
      CHECK_MSTATUS( cAttr.addChild( aBits_filters_1 ) );
      CREATE_BOOL( nAttr, aBits_filters_2, "Catmull_Rom", "Catmull_Rom", 1 );
      CHECK_MSTATUS( cAttr.addChild( aBits_filters_2 ) );
      CREATE_BOOL( nAttr, aBits_filters_3, "Gaussian", "Gaussian", 1 );
      CHECK_MSTATUS( cAttr.addChild( aBits_filters_3 ) );
      CREATE_BOOL( nAttr, aBits_filters_4, "Sinc", "Sinc", 1 );
      CHECK_MSTATUS( cAttr.addChild( aBits_filters_4 ) );
      CREATE_BOOL( nAttr, aBits_filters_5, "Blackman_Harris", "Blackman_Harris", 0 );
      CHECK_MSTATUS( cAttr.addChild( aBits_filters_5 ) );
      CREATE_BOOL( nAttr, aBits_filters_6, "Mitchell", "Mitchell", 0 );
      CHECK_MSTATUS( cAttr.addChild( aBits_filters_6 ) );
      CREATE_BOOL( nAttr, aBits_filters_7, "SeparableCatmull_Rom", "SeparableCatmull_Rom", 0 );
      CHECK_MSTATUS( cAttr.addChild( aBits_filters_7 ) );
      CREATE_BOOL( nAttr, aBits_filters_8, "Lanczos", "Lanczos", 0 );
      CHECK_MSTATUS( cAttr.addChild( aBits_filters_8 ) );
      CREATE_BOOL( nAttr, aBits_filters_9, "Bessel", "Bessel", 0 );
      CHECK_MSTATUS( cAttr.addChild( aBits_filters_9 ) );
      CREATE_BOOL( nAttr, aBits_filters_10, "Disk", "Disk", 0 );
      CHECK_MSTATUS( cAttr.addChild( aBits_filters_10 ) );

  CREATE_COMP( cAttr, aBits_features, "bits_features", "bfea" );
      CREATE_BOOL( nAttr, aBits_features_0, "Blobbies", "Blobbies", 0 );
      CHECK_MSTATUS( cAttr.addChild( aBits_features_0 ) );
      CREATE_BOOL( nAttr, aBits_features_1, "Points", "Points", 0 );
      CHECK_MSTATUS( cAttr.addChild( aBits_features_1 ) );
      CREATE_BOOL( nAttr, aBits_features_2, "Eyesplits", "Eyesplits", 1 );
      CHECK_MSTATUS( cAttr.addChild( aBits_features_2 ) );
      CREATE_BOOL( nAttr, aBits_features_3, "Raytracing", "Raytracing", 0 );
      CHECK_MSTATUS( cAttr.addChild( aBits_features_3 ) );
      CREATE_BOOL( nAttr, aBits_features_4, "DepthOfField", "DepthOfField", 1 );
      CHECK_MSTATUS( cAttr.addChild( aBits_features_4 ) );
      CREATE_BOOL( nAttr, aBits_features_5, "AdvancedVisibility", "AdvancedVisibility", 0 );
      CHECK_MSTATUS( cAttr.addChild( aBits_features_5 ) );
      CREATE_BOOL( nAttr, aBits_features_6, "DisplayChannels", "DisplayChannels", 0 );
      CHECK_MSTATUS( cAttr.addChild( aBits_features_6 ) );

  CREATE_COMP( cAttr, aBits_required, "bits_required", "breq" );
    CREATE_BOOL( nAttr, aBits_required_0, "Swap_UV", "Swap_UV", 0 );
    CHECK_MSTATUS( cAttr.addChild( aBits_required_0 ) );
    CREATE_BOOL( nAttr, aBits_required_1, "__Pref", "__Pref", 0 );
    CHECK_MSTATUS( cAttr.addChild( aBits_required_1 ) );
    CREATE_BOOL( nAttr, aBits_required_2, "MakeShadow", "MakeShadow", 0 );
    CHECK_MSTATUS( cAttr.addChild( aBits_required_2 ) );

        CREATE_STRING( tAttr,  aShotName,                   "shotName",                     "sn",     ""    );
        CREATE_STRING( tAttr,  aShotVersion,                "shotVersion",                  "sv",     ""    );


  return MS::kSuccess;
}





