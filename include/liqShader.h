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
#ifndef liqShader_H_
#define liqShader_H_

#include <string>
#include <vector>

#include <maya/MColor.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagPath.h>

#include <liqTokenPointer.h>
#include <liquidGetSloInfo.h>
#define MR_SURFPARAMSIZE 1024


#ifndef _WIN32
#include <libgen.h>
#define LIQ_GET_SHADER_FILE_NAME(a, b, c) if( b ) a = basename( const_cast<char *>(c.file.c_str())); else a = const_cast<char *>(c.file.c_str());
#else
#define LIQ_GET_SHADER_FILE_NAME(a, b, c) a = const_cast<char *>(c.file.c_str());
#endif



class liqShader 
{
public :
    liqShader();
    liqShader( const liqShader & src );
    liqShader & operator=( const liqShader & src );
    liqShader ( MObject shaderObj );
    MStatus liqShaderParseVectorAttr ( MFnDependencyNode & shaderNode, const char * argName, ParameterType pType );
    ~liqShader();
    void freeShader( void );
    int numTPV;
    liqTokenPointer	tokenPointerArray[MR_SURFPARAMSIZE];
    std::string name;
    std::string file;
    RtColor rmColor;
    RtColor rmOpacity;
    bool hasShadingRate;
    RtFloat shadingRate;
    bool hasDisplacementBound;
    RtFloat displacementBound;
    bool hasErrors;
    SHADER_TYPE shader_type;
};
#endif // liqShader_H_
