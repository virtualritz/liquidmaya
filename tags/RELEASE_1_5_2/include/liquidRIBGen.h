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

#ifndef _H_liquidRIBGen
#define _H_liquidRIBGen

/* ______________________________________________________________________
** 
** Liquid RibGen Header File
** ______________________________________________________________________
*/

#include <ri.h>
#include <liquidRIBStatus.h>
#include "zlib.h"
class liquidRIBGen;

extern "C" liquidRIBGen    *RIBGenCreate();
extern "C" void       RIBGenDestroy( liquidRIBGen * );
extern "C" void RiFlush( void );

class liquidRIBGen
{
public:
//    virtual int SetArgs( int n, RtToken tokens[], RtPointer values[] ) = 0;
//    virtual void Bound( liquidRIBStatus *, RtBound b ) = 0;
    virtual int GenRIB( liquidRIBStatus * ) = 0;
		int _GenRIB( liquidRIBStatus * ribStatus ) {
		int returnValue = 0;
		returnValue = GenRIB( ribStatus );
		return returnValue;
	}
};

#endif
