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
#include <liquidRibObj.h>

#include <maya/MArgList.h>
#include <maya/MSyntax.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MMatrix.h>

#include <ri.h>


void* liqWriteArchive::creator()
{
	return new liqWriteArchive();
}

MSyntax liqWriteArchive::syntax()
{
	MSyntax syn;

	syn.addArg(MSyntax::kString); // object name
	syn.addArg(MSyntax::kString); // RIB output filename

	syn.addFlag("rt", "rootTransform",   MSyntax::kBoolean);
	syn.addFlag("ct", "childTransforms", MSyntax::kBoolean);

	return syn;
}


MStatus liqWriteArchive::doIt(const MArgList& args)
{
	MStatus status;

	MString objName = args.asString(0, &status);
	if (!status) {
		MGlobal::displayError("error parsing object name argument");
		return MS::kFailure;
	}
	objectNames.append(objName);

	outputFilename = args.asString(1, &status);
	if (!status) {
		MGlobal::displayError("error parsing rib filename argument");
		return MS::kFailure;
	}

	outputRootTransform = false;
	int flagIndex = args.flagIndex("rt", "rootTransform");
	if (flagIndex != MArgList::kInvalidArgIndex) {
		outputRootTransform   = args.asBool(flagIndex + 1, &status);
		if (!status) {
		MGlobal::displayError("error parsing rootTransform flag's bool argument");
		}
	}

	outputChildTransforms = true;
	flagIndex = args.flagIndex("ct", "childTransforms");
	if (flagIndex != MArgList::kInvalidArgIndex) {
		outputChildTransforms = args.asBool(flagIndex + 1, &status);
		if (!status) {
		MGlobal::displayError("error parsing childTransform flag's bool argument");
		}
	}

	return redoIt();
}

MStatus liqWriteArchive::redoIt()
{
	MString objName = objectNames[0]; // TEMP - just handling one object at the moment

	// get a handle on the named object
	MSelectionList selList;
	selList.add(objName);
	MDagPath objDagPath;
	MStatus status = selList.getDagPath(0, objDagPath);
	if (!status) {
		MGlobal::displayError("Error retrieving object " + objName);
		return MS::kFailure;
	}

	// test that the output file is writable
	FILE *f = fopen(outputFilename.asChar(), "w");
	if (!f) {
		MGlobal::displayError("Error writing to output file " + outputFilename + ". Check file permissions there");
		return MS::kFailure;
	}
	fclose(f);

	// write the RIB file
	RiBegin(const_cast<char*>(outputFilename.asChar()));
	
	writeObjectToRib(objDagPath, outputRootTransform);
	
	RiEnd();

	return MS::kSuccess;
}

void liqWriteArchive::writeObjectToRib(const MDagPath &objDagPath, bool writeTransform)
{
	if (objDagPath.node().hasFn(MFn::kShape)) {
		// we're looking at a shape node, so write out the geometry to the RIB
		outputObjectName(objDagPath);
		if (objDagPath.hasFn(MFn::kMesh)) {
			liquidRibObj ribObj(objDagPath, MRT_Mesh);
			ribObj.writeObject();
		} else if (objDagPath.hasFn(MFn::kNurbsSurface)) {
			liquidRibObj ribObj(objDagPath, MRT_Nurbs);
			ribObj.writeObject();
		} else {
			MGlobal::displayWarning("skipping unknown geometry type in liquidWriteArchive");
		}
	} else {
		// we're looking at a transform node
		bool wroteTransform = false;
		if (writeTransform && (objDagPath.apiType() == MFn::kTransform)) {
			// push the transform onto the RIB stack
			outputObjectName(objDagPath);
			MMatrix tm = objDagPath.inclusiveMatrix();
			if (true) { //(!tm.isEquivalent(MMatrix::identity)) {
				RtMatrix riTM;
				tm.get(riTM);
				wroteTransform = true;
				outputIndentation();
				RiAttributeBegin();
				indentLevel++;
				outputIndentation();
				RiTransform(riTM);
			}
		}
		// go through all the children of this node and deal with each of them
		int nChildren = objDagPath.childCount();
		for(int i=0; i<nChildren; ++i) {
			MDagPath childDagNode;
			MStatus stat = MDagPath::getAPathTo(objDagPath.child(i), childDagNode);
			if (stat) {
				writeObjectToRib(childDagNode, outputChildTransforms);
			} else {
				MGlobal::displayWarning("error getting a dag path to child node of object " + objDagPath.fullPathName());
			}
		}
		if (wroteTransform) {
			indentLevel--;
			outputIndentation();
			RiAttributeEnd();
		}
	}
}

void liqWriteArchive::outputIndentation()
{
	for(unsigned int i=0; i<indentLevel; ++i) {
		RiArchiveRecord(RI_VERBATIM, "\t");
	}
}

void liqWriteArchive::outputObjectName(const MDagPath &objDagPath)
{
	MString name = objDagPath.fullPathName();
	const char *namePtr = name.asChar();
	RiArchiveRecord(RI_VERBATIM, "\n");
	outputIndentation();
	RiAttribute("identifier", "name", &namePtr, RI_NULL);
}
