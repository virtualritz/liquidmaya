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

// Renderman Headers
extern "C" {
  #include <ri.h>
}

#include <maya/MFnDependencyNode.h>

#include <liquid.h>
#include <liqRibCoordData.h>

extern int debugMode;


/**
 * Create a RIB compatible representation of a Maya coordinate system.
 */
liqRibCoordData::liqRibCoordData( MObject coord )
{
  if ( debugMode ) { printf("-> creating coord\n"); }
  MFnDependencyNode fnNode( coord );
  this->name = fnNode.name();
}

/**
 * Class destructor
 */
liqRibCoordData::~liqRibCoordData()
{
  if ( debugMode ) { printf("-> killing coord\n"); }
}

/**
 * Write the RIB for this coordinate system
 */
void liqRibCoordData::write()
{
  if ( debugMode ) { printf("-> writing coord"); }
  RiCoordinateSystem( const_cast<char *> ( name.asChar() ) );
}

/**
 * Compare this coordinate system to otherObj.
 * The purpose is usually to determe if the coordinate system is animated.
 */
bool liqRibCoordData::compare( const liqRibData & otherObj ) const
{
  if ( debugMode ) { printf("-> comparing coord\n"); }
  if ( otherObj.type() != MRT_Coord ) return false;
    return true;
}

/**
 * Return the geometry type.
 */
ObjectType liqRibCoordData::type() const
{
  if ( debugMode ) { printf("-> returning coord type\n"); }
  return MRT_Coord;
}
