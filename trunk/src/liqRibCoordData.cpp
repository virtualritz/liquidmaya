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
#include <liqRibCoordData.h>
#include <liqMemory.h>

extern int debugMode;

liqRibCoordData::liqRibCoordData( MObject coord )
//
//  Description:
//      create a RIB compatible representation of a Maya polygon mesh
//

{
	LIQDEBUGPRINTF( "-> creating coord\n" );
	MFnDependencyNode fnNode( coord );
	this->name = fnNode.name();
}

liqRibCoordData::~liqRibCoordData()
//
//  Description:
//      class destructor
//
{
	LIQDEBUGPRINTF( "-> killing coord\n" );
}

void liqRibCoordData::write()
//
//  Description:
//      Write the RIB for this mesh
//
{
	LIQDEBUGPRINTF( "-> writing coord" );
	char *coordName;
	coordName = (char *)lmalloc( name.length()  +1);
	strcpy(coordName, name.asChar());
	RiCoordinateSystem( coordName );
	lfree( coordName );
}

bool liqRibCoordData::compare( const liqRibData & otherObj ) const
//
//  Description:
//      Compare this mesh to the other for the purpose of determining
//      if its animated
//
{
	LIQDEBUGPRINTF( "-> comparing coord\n" );
	if ( otherObj.type() != MRT_Coord ) return false;
    return true;
}

ObjectType liqRibCoordData::type() const
//
//  Description:
//      return the geometry type
//
{
	LIQDEBUGPRINTF( "-> returning coord type\n" );
 	return MRT_Coord;
}
