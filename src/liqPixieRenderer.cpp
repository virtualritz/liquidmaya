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
** The RenderMan (R) Interface Procedures and Protocol are: Copyright 1988,
** 1989, Pixar All Rights Reserved
**
**
** RenderMan (R) is a registered trademark of Pixar
*/

#include <liqPixieRenderer.h>

bool
liqPixieRenderer::supports(e_capability capability) const
{
    bool supported = false;

    switch(capability)
    {
	case BLOBBIES:
	    supported = true;
	    break;

	case POINTS:
	    supported = true;
	    break;

	case EYESPLITS:
	    supported = true;
	    break;

	// no default case. let the compiler catch it if we're missing
	// something
    }

    return supported;
}

bool
liqPixieRenderer::requires(e_requirement requirement) const
{
    bool required = false;

    switch(requirement)
    {
	case SWAPPED_UVS:
	    required = true;
	    break;
	
	// no default case. let the compiler catch it if we're missing
	// something
    }

    return required;
}
