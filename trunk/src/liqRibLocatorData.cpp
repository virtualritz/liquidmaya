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
** Liquid Rib Locator Data Source 
** ______________________________________________________________________
*/

// Standard Headers
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>

#ifndef _WIN32
// Dynamic Object Headers
#include <dlfcn.h>
#endif

// Renderman Headers
extern "C" {
#include <ri.h>
}

#ifdef _WIN32
#include <process.h>
#include <malloc.h>
#else
#include <unistd.h>
#include <stdlib.h>
#include <alloca.h>
#endif

// Maya's Headers
#include <maya/MPoint.h>
#include <maya/MBoundingBox.h>
#include <maya/MPlug.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MStringArray.h>

#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liqRibLocatorData.h>
#include <liqMemory.h>

extern int debugMode;

liqRibLocatorData::liqRibLocatorData( MObject /*locator*/ )
//
//  Description:
//      create a RIB compatible representation of a Maya polygon mesh
//

{
  if ( debugMode ) { printf("-> creating locator\n"); }
}

liqRibLocatorData::~liqRibLocatorData()
//
//  Description:
//      class destructor
//
{
  if ( debugMode ) { printf("-> killing locator\n"); }
}

void liqRibLocatorData::write()
//
//  Description:
//      Write the RIB for this mesh
//
{
  if ( debugMode ) { printf("-> writing locator"); }
}

bool liqRibLocatorData::compare( const liqRibData & otherObj ) const
//
//  Description:
//      Compare this mesh to the other for the purpose of determining
//      if its animated
//
{
  if ( debugMode ) { printf("-> comparing locator\n"); }
  if ( otherObj.type() != MRT_Locator ) return false;
  return true;
}

ObjectType liqRibLocatorData::type() const
//
//  Description:
//      return the geometry type
//
{
  if ( debugMode ) { printf("-> returning locator type\n"); }
  return MRT_Locator;
}
