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

#ifndef liqRibMayaSubdivisionData_H
#define liqRibMayaSubdivisionData_H

/* ______________________________________________________________________
**
** Liquid Rib Maya Subdivision Data Header File
** ______________________________________________________________________
*/

#include <maya/MUint64Array.h>
#include <liqRibData.h>

#include <boost/shared_array.hpp>

using namespace boost;

class liqRibMayaSubdivisionData : public liqRibData {
public: // Methods
  liqRibMayaSubdivisionData( MObject mesh );

  virtual void       write();
  virtual bool       compare( const liqRibData & other ) const;
  virtual ObjectType type() const;

private: // Data
  int findIndex( MUint64 id, MUint64Array& arr );

  RtInt     npolys;
  vector< RtInt > nverts;
  shared_array< RtInt > verts;
  vector< RtInt > polyvertsIds;
  const RtFloat* vertexParam;
  const RtFloat* stTexCordParam;
/*   const RtFloat * normalParam; */
/*   const RtFloat * polyuvParam; */

  MIntArray creases;
  MIntArray corners;

  unsigned  totalNumOfVertices;
  unsigned  numtexCords;
  unsigned  textureindex;
  MString   name;
  RtMatrix  transformationMatrix;

  //  bool interpBoundary; // this is always on for maya subd's
  bool hasCreases;
  bool hasCorners;
};

#endif
