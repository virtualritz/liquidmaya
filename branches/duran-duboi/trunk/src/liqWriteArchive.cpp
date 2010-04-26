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
**
**
** The RenderMan (R) Interface Procedures and Protocol are:
** Copyright 1988, 1989, Pixar
** All Rights Reserved
**
**
** RenderMan (R) is a registered trademark of Pixar
*/

#include <liquid.h>
#include <liqWriteArchive.h>
#include <liqRibNode.h>
#include <liqRibObj.h>
#include <liqGlobalHelpers.h>
#include <liqIOStream.h>

#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>

#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>
#include <maya/MSelectionList.h>
#include <maya/MMatrix.h>
#include <maya/MFnDagNode.h>
#include <maya/MArgParser.h>
#include <maya/MArgDatabase.h>
#include <maya/MFnSet.h>
#include <maya/MPlug.h>

#include <ri.h>


// RI_VERBATIM is in the current RenderMan spec but
// some RIB libraries don't know about it
#ifndef RI_VERBATIM
  #define RI_VERBATIM "verbatim"
#endif


MSyntax liqWriteArchive::m_syntax;


liqWriteArchive::liqWriteArchive() : m_indentLevel(0), m_outputFilename("/tmp/tmprib.rib"), m_exportTransform(1)
{
}


liqWriteArchive::~liqWriteArchive()
{
}


void* liqWriteArchive::creator()
{
	return new liqWriteArchive();
}


MSyntax liqWriteArchive::syntax()
{
	MSyntax &syn = liqWriteArchive::m_syntax;

	syn.useSelectionAsDefault(true);
	syn.setObjectType(MSyntax::kStringObjects, 0);

	syn.addFlag("b",  "binary");
	syn.addFlag("d",  "debug");
	syn.addFlag("o",  "output", MSyntax::kString);
	return syn;
}


MStatus liqWriteArchive::doIt(const MArgList& args)
{
	MStatus status;
	MArgParser argParser(syntax(), args);
	int flagIndex;

	// flag debug
	m_debug = false;
	flagIndex = args.flagIndex("d", "debug");
	if(flagIndex != MArgList::kInvalidArgIndex)
	{
		m_debug = true;
	}
	// flag binary
	m_binaryRib = false;
	flagIndex = args.flagIndex("b", "binary");
	if(flagIndex != MArgList::kInvalidArgIndex)
	{
		m_binaryRib = true;
	}
	// flag output
	flagIndex = args.flagIndex("o", "output");
	if(flagIndex == MArgList::kInvalidArgIndex)
	{
		displayError("[liqWriteArchive::doIt] Must provide the output rib : liquidWriteArchive -o /mon/rib.rib");
		return MS::kInvalidParameter;
	}
	status = argParser.getFlagArgument("output", flagIndex, m_outputFilename);


	// get objetcs
	int i;
	MStringArray listToBeExported;
	status = argParser.getObjects( listToBeExported );
	if(status!=MS::kSuccess)
	{
		displayError("[liqWriteArchive::doIt] Must provide the output rib : liquidWriteArchive -o /mon/rib.rib");
		return MS::kInvalidParameter;
	}
	//listToBeExported = stringArrayRemoveDuplicates(listToBeExported);
	
	
	// if provided list is empty, get selection
	if(listToBeExported.length()==0)
	{
		MSelectionList list;
		MGlobal::getActiveSelectionList(list);
		list.getSelectionStrings(listToBeExported);
	}
	m_objectNames = listToBeExported;
	if(m_debug)
	{
		printf("[liqWriteArchive::doIt] exporting :\n");
		for(i=0; i<listToBeExported.length();i++)
		{
			printf("    '%d' : '%s' \n", i, listToBeExported[i].asChar() );
		}
		printf("et c'est tout\n");
	}

	return redoIt();
}


MStringArray liqWriteArchive::stringArrayRemoveDuplicates(MStringArray src)
{
	unsigned int i;
	unsigned int j;
	MStringArray dst;
	for(i=0; i<src.length(); i++)
	{
		bool yetIn = 0;
		for(j=0; j<dst.length(); j++)
		{
			if( src[i]==dst[j] )
			{
				yetIn = 1;
			}
		}
		if(!yetIn)
		{
			dst.append(src[i]);
		}
	}
	return dst;
}


MStatus liqWriteArchive::redoIt()
{
	try
	{
		MStatus status;
		unsigned int i;
		unsigned int j;
		std::vector<MDagPath> objDb;
		std::vector<MObject> setsDn;
		// building an array with the MDagPaths to export
		for(i=0; i<m_objectNames.length(); i++)
		{
			// get a handle on the named object
			MSelectionList selList;
			selList.add(m_objectNames[i]);
			MDagPath objDagPath;
			status = selList.getDagPath(0, objDagPath);
			if(!status)
			{
				MObject depNode;
				status = selList.getDependNode(0, depNode);

				if(!status)
				{
					MGlobal::displayWarning("[liqWriteArchive::redoIt] Error retrieving object " + m_objectNames[i]);
				}
				else
				{
					MFnDependencyNode fnDepNode(depNode);
					MString type = fnDepNode.typeName();
					printf("OBJ %s : type=%s \n", fnDepNode.name().asChar(), type.asChar());
					if(type=="objectSet")
					{
						setsDn.push_back(depNode);
					}
				}
			}
			else
			{
				objDb.push_back(objDagPath);
			}
		}
		if( !objDb.size() && !setsDn.size() )
		{
			MGlobal::displayError("[liqWriteArchive::redoIt] no objetcs to export");
			return MS::kFailure;
		}

		// test that the output file is writable
		FILE *f = fopen( m_outputFilename.asChar(), "w" );
		if(!f)
		{
			MGlobal::displayError( "Error writing to output file " + m_outputFilename + ". Check file permissions there" );
			return MS::kFailure;
		}
		fclose(f);

		// binary or ascii
#if defined( PRMAN ) || defined( DELIGHT )
		RtString format[ 1 ] = { "ascii" };
		if ( m_binaryRib )
		{
			format[ 0 ] = "binary";
		}
		RiOption( "rib", "format", ( RtPointer )&format, RI_NULL);
#endif
		// write the RIB file
		if(m_debug)
		{
			cout << "liquidWriteArchive: writing on file : " << m_outputFilename.asChar() << endl;
		}
		RiBegin( const_cast< RtToken >( m_outputFilename.asChar() ) );

		for(i=0; i<objDb.size(); i++)
		{
			printf("EXPORT OBJECT  %s \n", m_objectNames[i].asChar());
			writeObjectToRib(objDb[i], m_exportTransform);
		}
		for(i=0; i<setsDn.size(); i++)
		{
			MFnSet fnSet(setsDn[i], &status);
			if(!status)
			{
				MGlobal::displayWarning("[liqWriteArchive::redoIt] Error init fnSet on object " + m_objectNames[i]);
				continue;
			}
			printf("EXPORT SET  %s \n", m_objectNames[i].asChar());
			
			MSelectionList memberList;
			fnSet.getMembers(memberList, true);
			MDagPath objDagPath;
			for(j=0; j<memberList.length(); j++)
			{
				status = memberList.getDagPath(j, objDagPath);
				printf("    - EXPORT OBJECT  %s \n", objDagPath.fullPathName().asChar());
				writeObjectToRib(objDagPath, m_exportTransform);
			}
		}
	
		RiEnd();
	}
	catch (...)
	{
		MGlobal::displayError("Caught exception in liqWriteArchive::redoIt()");
		return MS::kFailure;
	}
	return MS::kSuccess;
}


void liqWriteArchive::writeObjectToRib(const MDagPath &objDagPath, bool writeTransform)
{
	if (!isObjectVisible(objDagPath))
	{
		return;
	}
	if(m_debug)
	{
		cout << "liquidWriteArchive: writing object: " << objDagPath.fullPathName().asChar() << endl;
	}
	
	if (objDagPath.node().hasFn(MFn::kShape) || MFnDagNode( objDagPath ).typeName() == "liquidCoorSys")
	{
		// we're looking at a shape node, so write out the geometry to the RIB
		outputObjectName(objDagPath);

		liqRibNode ribNode;
		ribNode.set(objDagPath, 0, MRT_Unknown);


		// don't write out clipping planes
		if ( ribNode.object(0)->type == MRT_ClipPlane )
		{
			return;
		}
		if ( ribNode.rib.box != "" && ribNode.rib.box != "-" )
		{
			RiArchiveRecord( RI_COMMENT, "Additional RIB:\n%s", ribNode.rib.box.asChar() );
		}
		if ( ribNode.rib.readArchive != "" && ribNode.rib.readArchive != "-" )
		{
			// the following test prevents a really nasty infinite loop !!
			if ( ribNode.rib.readArchive != m_outputFilename )
			{
				RiArchiveRecord( RI_COMMENT, "Read Archive Data: \nReadArchive \"%s\"", ribNode.rib.readArchive.asChar() );
			}
		}
		if ( ribNode.rib.delayedReadArchive != "" && ribNode.rib.delayedReadArchive != "-" )
		{
			// the following test prevents a really nasty infinite loop !!
			if ( ribNode.rib.delayedReadArchive != m_outputFilename )
			{
				RiArchiveRecord( RI_COMMENT, "Delayed Read Archive Data: \nProcedural \"DelayedReadArchive\" [ \"%s\" ] [ %f %f %f %f %f %f ]", ribNode.rib.delayedReadArchive.asChar(), ribNode.bound[0], ribNode.bound[3], ribNode.bound[1], ribNode.bound[4], ribNode.bound[2], ribNode.bound[5]);
			}
		}
		// If it's a curve we should write the basis function
		if ( ribNode.object(0)->type == MRT_NuCurve )
		{
			RiBasis( RiBSplineBasis, 1, RiBSplineBasis, 1 );
		}
		if ( !ribNode.object(0)->ignore )
		{

			// SURFACE
			{
				// liqGetShader : construit les shaders une seul fois meme si utilisé bcp
				MStatus status;
				MStatus gStatus;
				MObject assignedShader = ribNode.assignedShader.object();
				//MFnDependencyNode shaderNode( assignedShader );
				//MPlug rmanShaderNamePlug = shaderNode.findPlug( MString( "rmanShaderLong" ) );
				//rmanShaderNamePlug.getValue( rmShaderStr );
				liqShader currentShader( assignedShader );

				scoped_array< RtToken > tokenArray( new RtToken[ currentShader.tokenPointerArray.size() ] );
				scoped_array< RtPointer > pointerArray( new RtPointer[ currentShader.tokenPointerArray.size() ] );
				assignTokenArrays( currentShader.tokenPointerArray.size(), &currentShader.tokenPointerArray[ 0 ], tokenArray.get(), pointerArray.get() );

				//////////////////////////////////////////////
				// get global liquidGlobals.shortShaderNames ..... FAUDRAIT ETRE INDEPENDANT !!
				//
				//	comme ca    : Surface "db_lambertRed" "uniform float Kd" [1]
				//	ou comme ca : Surface "/prod/tools/renderman/duran-duboi/pixar/compiledShaders/db_lambertRed" "uniform float Kd" [1]
				//
				bool liqglo_shortShaderNames;                 // true if we don't want to output path names with shaders
				{
					MSelectionList rGlobalList;
					MObject rGlobalObj;
					status = rGlobalList.add( "liquidGlobals" );
					status = rGlobalList.getDependNode( 0, rGlobalObj );
					MFnDependencyNode rGlobalNode( rGlobalObj );
					MPlug gPlug = rGlobalNode.findPlug( "shortShaderNames", &gStatus );
					if( gStatus == MS::kSuccess )
						gPlug.getValue( liqglo_shortShaderNames );
					gStatus.clear();
				}
				char* shaderFileName;
				LIQ_GET_SHADER_FILE_NAME( shaderFileName, liqglo_shortShaderNames, currentShader );

				// check shader space transformation
				if( currentShader.shaderSpace != "" )
				{
					RiTransformBegin();
					RiCoordSysTransform( ( RtString )currentShader.shaderSpace.asChar() );
				}
				// output shader
				// its one less as the tokenPointerArray has a preset size of 1 not 0
				int shaderParamCount = currentShader.tokenPointerArray.size() - 1;
				RiSurfaceV ( shaderFileName, shaderParamCount, tokenArray.get(), pointerArray.get() );
				if( currentShader.shaderSpace != "" )
					RiTransformEnd();
			}



			// DISPLACE
			{
				bool liqglo_shortShaderNames = 1;

				MObject assignedDisplace = ribNode.assignedDisp.object();
				liqShader currentShader(assignedDisplace);
				scoped_array< RtToken > tokenArray( new RtToken[ currentShader.tokenPointerArray.size() ] );
				scoped_array< RtPointer > pointerArray( new RtPointer[ currentShader.tokenPointerArray.size() ] );
				assignTokenArrays( currentShader.tokenPointerArray.size(), &currentShader.tokenPointerArray[ 0 ], tokenArray.get(), pointerArray.get() );
				char *shaderFileName;
				LIQ_GET_SHADER_FILE_NAME(shaderFileName, liqglo_shortShaderNames, currentShader );
				// check shader space transformation
				if( currentShader.shaderSpace != "" )
				{
					RiTransformBegin();
					RiCoordSysTransform( ( RtString )currentShader.shaderSpace.asChar() );
				}
				// output shader
				int shaderParamCount = currentShader.tokenPointerArray.size() - 1;
				RiDisplacementV ( shaderFileName, shaderParamCount, tokenArray.get(), pointerArray.get() );
				if( currentShader.shaderSpace != "" )
					RiTransformEnd();
			}




			ribNode.object(0)->writeObject();
		}
	}
	else
	{
		// we're looking for a transform node
		bool wroteTransform = false;
		if (writeTransform && (objDagPath.apiType() == MFn::kTransform))
		{
			if (m_debug)
			{
				cout << "liquidWriteArchive: writing transform: " << objDagPath.fullPathName().asChar() << endl;
			}
			// push the transform onto the RIB stack
			outputObjectName(objDagPath);
			MFnDagNode mfnDag(objDagPath);
			MMatrix tm = mfnDag.transformationMatrix();
			if (true)   // (!tm.isEquivalent(MMatrix::identity)) {
			{
				RtMatrix riTM;
				tm.get(riTM);
				wroteTransform = true;
				outputIndentation();
				RiAttributeBegin();
				m_indentLevel++;
				outputIndentation();
				RiConcatTransform(riTM);
			}
		}
		// go through all the children of this node and deal with each of them
		int nChildren = objDagPath.childCount();
		if(m_debug)
		{
			cout << "liquidWriteArchive: object " << objDagPath.fullPathName().asChar() << "has " << nChildren << " children" << endl;
		}
		for(int i=0; i<nChildren; ++i)
		{
			if(m_debug)
			{
				cout << "liquidWriteArchive: writing child number " << i << endl;
			}
			MDagPath childDagNode;
			MStatus stat = MDagPath::getAPathTo(objDagPath.child(i), childDagNode);
			if (stat)
			{
				writeObjectToRib(childDagNode, m_exportTransform);
			}
			else
			{
				MGlobal::displayWarning("error getting a dag path to child node of object " + objDagPath.fullPathName());
			}
		}
		if (wroteTransform)
		{
			m_indentLevel--;
			outputIndentation();
			RiAttributeEnd();
		}
	}
	if(m_debug)
	{
		cout << "liquidWriteArchive: finished writing object: " << objDagPath.fullPathName().asChar() << endl;
	}
}


void liqWriteArchive::outputIndentation()
{
	for(unsigned int i=0; i<m_indentLevel; ++i)
	{
		RiArchiveRecord(RI_VERBATIM, "\t");
	}
}


void liqWriteArchive::outputObjectName(const MDagPath &objDagPath)
{
	MString name = sanitizeNodeName( objDagPath.fullPathName() );
	RiArchiveRecord(RI_VERBATIM, "\n");
	outputIndentation();
	RtString ribname = const_cast< char* >( name.asChar() );
	RiAttribute( "identifier", "name", &ribname, RI_NULL );
}
