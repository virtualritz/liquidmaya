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

#ifndef liqRibLightData_H
#define liqRibLightData_H

/* ______________________________________________________________________
**
** Liquid Rib Light Data Header File
** ______________________________________________________________________
*/

#include <liqRibData.h>

class liqRibLightData : public liqRibData {
public:

  liqRibLightData( const MDagPath & light );
  virtual ~liqRibLightData();

  virtual void       write();
  virtual bool       compare( const liqRibData & other ) const;
  virtual ObjectType type() const;

  RtLightHandle lightHandle() const;
  bool          rmanLight;
  MString       assignedRManShader;

  MString       autoShadowName( MString suffix = "" ) const;

private:
  LightType     lightType;
  RtFloat       color[3];
  RtFloat       decay;
  RtFloat       intensity, coneAngle, penumbraAngle, dropOff;

  RtFloat       barnDoors;
  RtFloat       leftBarnDoor;
  RtFloat       rightBarnDoor;
  RtFloat       topBarnDoor;
  RtFloat       bottomBarnDoor;

  RtFloat       nonDiffuse;
  RtFloat       nonSpecular;
  //RtPoint       from, to;
  RtMatrix      transformationMatrix;

  RtLightHandle handle;
  bool          usingShadow;
  bool          deepShadows;
  bool          rayTraced;
  RtInt         raySamples;
  bool          excludeFromRib;
  MString       userShadowName;
  MString       lightName;

  MString       shadowName;
  MString       shadowNamePx;
  MString       shadowNameNx;
  MString       shadowNamePy;
  MString       shadowNameNy;
  MString       shadowNamePz;
  MString       shadowNameNz;
  RtFloat       shadowBias;
  RtFloat       shadowFilterSize;
  RtFloat       shadowSamples;
  RtColor       shadowColor;
};


#endif
