/*
**
** The contents of this file are subject to the Mozilla Public License Version
** 1.1 (the "License"); you may not use this file except in compliance with
** the License.  You may obtain a copy of the License at
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

#ifndef liqRenderer_H
#define liqRenderer_H

#include <maya/MString.h>


class liqRenderer {

public:
  /*enum e_renderer   { REN_PRMAN, REN_ENTROPY, REN_AQSIS, REN_DELIGHT };
  enum e_capability { BLOBBIES, POINTS, EYESPLITS };

  enum e_requirement	{
    SWAPPED_UVS,  // transpose u & v direction on NURBS
    __PREF        // use __Pref instead of Pref
  };*/

  liqRenderer()
  : renderName( "PRMan" ),
#ifdef _WIN32
    renderCommand( "prman" ),
    renderPreview( "prman" ),
#else
    renderCommand( "render" ),
    renderPreview( "render" ),
#endif
  renderCmdFlags( "" ),

  supports_BLOBBIES( true ),
  supports_POINTS( true ),
  supports_EYESPLITS( true ),
  supports_RAYTRACE( true ),
  supports_DOF( true ),

  requires_SWAPPED_UVS( true ),
  requires__PREF( true ),
  requires_MAKESHADOW( false ),

  dshDisplayName( "deepshad" ), // PRman default
  dshImageMode( "deepopacity" )
  {}

  virtual ~liqRenderer()
  {
    // nothing else needed
  }

  MString renderName;
  MString renderCommand;
  MString renderPreview;
  MString renderCmdFlags;

  bool supports_BLOBBIES;
  bool supports_POINTS;
  bool supports_EYESPLITS;
  bool supports_RAYTRACE;
  bool supports_DOF;

  // renderer requirement
  bool requires_SWAPPED_UVS; // transpose u & v direction on NURBS
  bool requires__PREF;       // use __Pref instead of Pref
  bool requires_MAKESHADOW;  // requires MakeShadow to convert zfile to shadow

  // Deep Shadow Display
  MString dshDisplayName;
  MString dshImageMode;
};

// Singleton copy of liqRenderer object
// const liqRenderer & liquidRenderer();

extern liqRenderer liquidRenderer;


#endif
