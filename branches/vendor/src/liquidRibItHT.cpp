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
** Liquid Rib Hash Table Source 
** ______________________________________________________________________
*/

// Standard Headers
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>

#ifndef _WIN32
// Dynamic Object Headers
#include <dlfcn.h>
#endif

// Renderman Headers
extern "C" {
	#include <ri.h>
	#include <slo.h>
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
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MColor.h>

#include <liquid.h>
#include <liquidRibItHT.h>
#include <liquidMemory.h>

extern int debugMode;

RibItHT::RibItHT(RibHT *iht)
//
//  Description:
//      Class constructor
//
{
	if ( debugMode ) { printf("-> creating hash table iterator\n"); }
    ht = iht;
    reset();
}

void RibItHT::reset()
//
//  Description:
//      Reset to the beginning of the hash table
//
{
    if ( debugMode ) { printf("-> reseting hash table iterator\n"); }
    for (curIndex=0; curIndex<MR_HASHSIZE; ++curIndex) {
		current = ht->table[curIndex];
		if (current != NULL) break;
    }
    done = (current == NULL);
}

RibNode *RibItHT::next()
//
//  Description:
//      Advance to the next entry in the table and return a pointer to it.
//
{
    if ( debugMode ) { printf("-> advancing through hash table\n"); }
    RibNode * rn = current;
    if (current) {
		if (current->next) current = current->next;
		else {
			current = NULL;
			for (++curIndex; curIndex<MR_HASHSIZE; ++curIndex) {
				current = ht->table[curIndex];
				if (current != NULL) break;
			}
			done = (current == NULL);
		}
    }
    return rn;
}
