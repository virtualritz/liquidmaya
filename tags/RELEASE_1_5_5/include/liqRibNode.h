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

#ifndef liqRibNode_H
#define liqRibNode_H

/* ______________________________________________________________________
** 
** Liquid Rib Node Header File
** ______________________________________________________________________
*/

#include <liqRibData.h>
#include <liqRibObj.h>
#include <maya/MColor.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagPath.h>
#include <maya/MObjectArray.h>

class liqRibNode {   
public:
    liqRibNode( liqRibNode * instanceOfNode = NULL );
    ~liqRibNode();
			
    void set( const MDagPath &, int, ObjectType objType );
			   
    liqRibNode *	    next;
    MString name;
	
    AnimType        matXForm;
    AnimType        bodyXForm;
	
    liqRibObj *	    object(int);
    liqRibObj *	    no;
    
    MDagPath &      path();
     
    MColor		color;
    bool    	    	matteMode;
    bool    	    	doubleSided;
    MString 	    	shaderName;
    MString 	    	dispName;
    MString 	    	volumeName;
    MFnDependencyNode 	assignedShadingGroup;
    MFnDependencyNode	assignedShader;
    MFnDependencyNode	assignedDisp;
    MFnDependencyNode	assignedVolume;
    MObject 	    	findShadingGroup( const MDagPath& path );
    MObject 		findShader( MObject& group );
    MObject 		findDisp( MObject& group );
    MObject 	    	findVolume( MObject& group );
    void    	    	getIgnoredLights( MObject& group, MObjectArray& lights );
    void    	    	getIgnoredLights( MObjectArray& lights );
    bool    	    	getColor( MObject& shader, MColor& color );
    bool    	    	getMatteMode( MObject& shader );
    bool    	    	hasRibGen();
    void    	    	doRibGen();
    MString 	    	ribBoxString;
    bool    	    	isRibBox;
    MString		archiveString;
    bool  		isArchive;
    MString		delayedArchiveString;
    bool  		isDelayedArchive;
    RtBound 	    	bound;
    bool    	    	doDef;		/* Used for per-object deformation blur */
    bool    	    	doMotion;	/* Used for per-object transformation blur */
    float   	    	nodeShadingRate;
    bool    	    	nodeShadingRateSet;
		   
private:
        
    MDagPath	    DagPath;
    liqRibObj *  objects[5];
    liqRibNode * instance;
    MString 	    ribGenName;
    bool    	    hasRibGenAttr;
};

#endif
