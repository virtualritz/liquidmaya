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

#ifndef liquidRibData_H
#define liquidRibData_H

/* ______________________________________________________________________
** 
** Liquid RibData Header File
** ______________________________________________________________________
*/

#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MStringArray.h>
#include <vector>
#include <liqTokenPointer.h>

class liqRibData {
public:
    virtual ~liqRibData();
    virtual void    write() = 0;
    virtual bool    compare( const liqRibData & other ) const = 0;
    virtual ObjectType type() const = 0;
    virtual void    addAdditionalSurfaceParameters( MObject node );
    std::vector<liqTokenPointer> tokenPointerArray;
    MDagPath	objDagPath;
private:
    void parseVectorAttributes( MFnDependencyNode &nodeFn, MStringArray & strArray, ParameterType pType );
};

#endif
