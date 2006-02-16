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

#ifndef liqRibObject_H
#define liqRibObject_H

/* ______________________________________________________________________
**
** Liquid Rib Object Header File
** ______________________________________________________________________
*/

#include <liqRibData.h>

class liqRibObj {
public:
    liqRibObj( const MDagPath &, ObjectType objType );
    ~liqRibObj();

    AnimType compareMatrix(const liqRibObj *, int instance);
    AnimType compareBody(const liqRibObj *);
    void     writeObject(); // write geometry directly

    int    type;
    int    written;
    bool   ignore;
    bool   ignoreShadow;
    bool   ignoreShapes;
    char **lightSources;

    MMatrix matrix( int instance ) const;
    void    setMatrix( int instance, MMatrix matrix );

    void ref();
    void unref();

    RtObjectHandle handle() const;
    RtLightHandle  lightHandle() const;
    void setHandle( RtObjectHandle handle );

private:
    MMatrix       *instanceMatrices; // Matrices for all instances of this object
    RtObjectHandle objectHandle;     // Handle used by RenderMan to refer to defined geometry
    int            referenceCount;   // Object's reference count
    liqRibData    *data;             // Geometry or light data
};

#endif
