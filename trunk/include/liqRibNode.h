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

#ifndef liqRibNode_H
#define liqRibNode_H

/* ______________________________________________________________________
**
** Liquid Rib Node Header File
** ______________________________________________________________________
*/

#include <liqRibData.h>
#include <liqRibObj.h>
#include <maya/MColor.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagPath.h>
#include <maya/MObjectArray.h>


//enum transmissions {TRANS, OPAQUE, OS, SHADER};


class liqRibNode {
  public:


    liqRibNode( liqRibNode * instanceOfNode = NULL,
                const MString instanceOfNodeStr = "" );
    ~liqRibNode();

    void set( const MDagPath &, int, ObjectType objType, int particleId = -1 );


    liqRibNode *       next;
    MString            name;

    AnimType           matXForm;
    AnimType           bodyXForm;

    liqRibObj *        object( unsigned );
    liqRibObj *        no;

    MDagPath &         path();

    MColor             color;
    MColor             opacity;
    bool               matteMode;
    bool               doubleSided;
    MString            shaderName;
    MString            dispName;
    MString            volumeName;
    MFnDependencyNode  assignedShadingGroup;
    MFnDependencyNode  assignedShader;
    MFnDependencyNode  assignedDisp;
    MFnDependencyNode  assignedVolume;

    MObject  findShadingGroup( const MDagPath& path );
    MObject  findShader( MObject& group );
    MObject  findDisp( MObject& group );
    MObject  findVolume( MObject& group );
    void     getIgnoredLights( MObject& group, MObjectArray& lights );
    void     getIgnoredLights( MObjectArray& lights );
    bool     getColor( MObject& shader, MColor& color );
    bool     getOpacity( MObject& shader, MColor& opacity );
    bool     getMatteMode( MObject& shader );
    bool     hasRibGen();
    void     doRibGen();
    RtBound  bound;
    bool     doDef;    /* Used for per-object deformation blur */
    bool     doMotion;  /* Used for per-object transformation blur */

    bool     belongsToCsgOperation;

    MString  getInstanceStr() { return instanceStr; };
    bool     hasNObjects( unsigned n );
    bool     colorOverridden() { return overrideColor; };


    struct shading {
      float    shadingRate;
      bool     diceRasterOrient;
      MColor   color;
      MColor   opacity;
      int      matte;

    } shading;


    struct trace {
      bool      sampleMotion;
      bool      displacements;
      float     bias;

      int     maxDiffuseDepth;

      int     maxSpecularDepth;
    } trace;



    struct visibility {
      bool      camera;
      bool      trace;
      bool      diffuse;
      bool      specular;
      bool      photon;
      bool      midpoint;
      bool      newtransmission;
      enum {
        TRANSMISSION_TRANSPARENT = 0,
        TRANSMISSION_OPAQUE      = 1,
        TRANSMISSION_OS          = 2,
        TRANSMISSION_SHADER      = 3
      } transmission;
    } visibility;

    struct hitmode {
      enum {
        CAMERA_HITMODE_PRIMITIVE  = 0,
        CAMERA_HITMODE_SHADER     = 1
      } camera;
      enum {
        DIFFUSE_HITMODE_PRIMITIVE  = 0,
        DIFFUSE_HITMODE_SHADER     = 1
      } diffuse;
      enum {
        SPECULAR_HITMODE_PRIMITIVE  = 0,
        SPECULAR_HITMODE_SHADER     = 1
      } specular;
      enum {
        TRANSMISSION_HITMODE_PRIMITIVE  = 0,
        TRANSMISSION_HITMODE_SHADER     = 1
      } transmission;
    } hitmode;

    struct irradiance {
      float     shadingRate;
      int       nSamples;
      float     maxError;
      float     maxPixelDist;
      MString   handle;
      enum {
        FILEMODE_NONE = 0,
        FILEMODE_READ = 1,
        FILEMODE_WRITE = 2,
        FILEMODE_READ_WRITE = 3
      } fileMode;
    } irradiance;

    struct photon {
      MString   globalMap;
      MString   causticMap;
      enum {
        SHADINGMODEL_MATTE = 0,
        SHADINGMODEL_GLASS = 1,
        SHADINGMODEL_WATER = 2,
        SHADINGMODEL_CHROME = 3,
        SHADINGMODEL_TRANSPARENT = 4
      } shadingModel;
      int estimator;
    } photon;

    struct motion {
      bool    transformationBlur;
      bool    deformationBlur;
      int     samples;
      float   factor;
    } motion;

    struct rib {
      MString box;
      MString generator;
      MString readArchive;
      MString delayedReadArchive;
    } rib;

    struct grouping {
      MString membership;
    } grouping;

    struct delight {
      struct {
        MString groupName;
        MColor  scattering;
        MColor  absorption;
        float  refraction;
        float  scale;
        float  shadingRate;
      } subSurface;
    } delight;

    struct subdivMesh {
      bool  render;
      bool  interpBounday;
      bool  edgeCreasing;
    } subdivMesh;

    struct curve {
      bool  render;
      float constantwidth;
    } curve;

    bool    instanceInheritPPColor;
    bool    invisible;
    bool    ignoreShapes;

private:

    MDagPath    DagPath;
    liqRibObj  *objects[LIQMAXMOTIONSAMPLES];
    liqRibNode *instance;
    MString     instanceStr;
    MString     ribGenName;
    bool        hasRibGenAttr;
    bool        overrideColor;
};

#endif
