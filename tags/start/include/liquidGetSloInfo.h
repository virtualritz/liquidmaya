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

#ifndef liquidGetSloInfo_H
#define liquidGetSloInfo_H

/* ______________________________________________________________________
** 
** Liquid Get .slo Info Header File
** ______________________________________________________________________
*/

#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MIntArray.h>
#include <maya/MPxCommand.h>

#include <vector>

typedef enum {
    SHADER_TYPE_UNKNOWN,
    SHADER_TYPE_POINT,
    SHADER_TYPE_COLOR,
    SHADER_TYPE_SCALAR,
    SHADER_TYPE_STRING,
/* The following types are primarily used for shaders */
    SHADER_TYPE_SURFACE,
    SHADER_TYPE_LIGHT,
    SHADER_TYPE_DISPLACEMENT,
    SHADER_TYPE_VOLUME,
    SHADER_TYPE_TRANSFORMATION,
    SHADER_TYPE_IMAGER,
/* The following are variable types added since RISpec 3.1 */
    SHADER_TYPE_VECTOR,
    SHADER_TYPE_NORMAL,
    SHADER_TYPE_MATRIX
} SHADER_TYPE;

typedef enum {
    SHADER_DETAIL_UNKNOWN,
    SHADER_DETAIL_VARYING,
    SHADER_DETAIL_UNIFORM
} SHADER_DETAIL;

class liquidGetSloInfo : public MPxCommand {
public:
	liquidGetSloInfo() {};
	virtual ~liquidGetSloInfo();
	static  void*	creator();
	int		setShader( MString shaderName );
	void	resetIt();
	int		nargs() { return numParam; }
	MString getName() { return shaderName; }
	SHADER_TYPE getType() { return shaderType; }
	int		getNumParam() { return numParam; }
	MString getTypeStr();
	MString getArgName( int num ) { return argName[ num ]; }
	SHADER_TYPE getArgType( int num ) { return argType[ num ]; }
	MString getArgTypeStr( int num );
	SHADER_DETAIL getArgDetail( int num ) { return argDetail[ num ]; }
	MString getArgDetailStr( int num );
	MString getArgStringDefault( int num, int entry );
	float   getArgFloatDefault( int num, int entry );
	int     getArgArraySize( int num ) { return argArraySize[ num ]; }
			    
	MStatus	    doIt(const MArgList& args );
private:
	unsigned numParam;
	SHADER_TYPE shaderType;
	MString shaderName;
	std::vector<MString> argName;
	std::vector<SHADER_TYPE> argType;
	std::vector<SHADER_DETAIL> argDetail;
	std::vector<int> argArraySize;
	std::vector<void*> argDefault;
};

#endif
