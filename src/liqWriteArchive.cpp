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
#include <liqRibObj.h>

#include <maya/MArgList.h>
#include <maya/MSyntax.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MMatrix.h>
#include <maya/MFnDagNode.h>
#include <maya/MArgParser.h>

#include <ri.h>

// RI_VERBATIM is in the current RenderMan spec but 
// some RIB libraries don't know about it
#ifndef RI_VERBATIM
  #define RI_VERBATIM "verbatim"
#endif


void* liqWriteArchive::creator()
{
  return new liqWriteArchive();
}

MSyntax liqWriteArchive::syntax()
{
  MSyntax syn;

  syn.addArg(MSyntax::kString); // object name
  syn.addArg(MSyntax::kString); // RIB output filename

  syn.addFlag("rt", "rootTransform");
  syn.addFlag("ct", "childTransforms");
  syn.addFlag("d",  "debug");

  return syn;
}


MStatus liqWriteArchive::doIt(const MArgList& args)
{
  MStatus status;
  MArgParser argParser(syntax(), args);
    
  MString tempStr;
  status = argParser.getCommandArgument(0, tempStr);
  if (!status) {
    MGlobal::displayError("error parsing object name argument");
    return MS::kFailure;
  }
  objectNames.append(tempStr);

  status = argParser.getCommandArgument(1, outputFilename);
  if (!status) {
    MGlobal::displayError("error parsing rib filename argument");
    return MS::kFailure;
  }

  outputRootTransform = false;
  int flagIndex = args.flagIndex("rt", "rootTransform");
  if (flagIndex != MArgList::kInvalidArgIndex) {
    outputRootTransform = true;
  }

  outputChildTransforms = true;
  flagIndex = args.flagIndex("ct", "childTransforms");
  if (flagIndex != MArgList::kInvalidArgIndex) {
    outputChildTransforms = true;
  }
  
  debug = false;
  flagIndex = args.flagIndex("d", "debug");
  if (flagIndex != MArgList::kInvalidArgIndex) {
    debug = true;
  }

  return redoIt();
}

MStatus liqWriteArchive::redoIt()
{
  try {
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
  } catch (...) {
    MGlobal::displayError("Caught exception in liqWriteArchive::redoIt()");
    return MS::kFailure;
  }

  return MS::kSuccess;
}

void liqWriteArchive::writeObjectToRib(const MDagPath &objDagPath, bool writeTransform)
{
  if (debug) { cout << "liquidWriteArchive: writing object: " << objDagPath.fullPathName() << endl; }
  if (objDagPath.node().hasFn(MFn::kShape)) {
    // we're looking at a shape node, so write out the geometry to the RIB
    outputObjectName(objDagPath);
    if (objDagPath.hasFn(MFn::kMesh)) {
      if (debug) { cout << "liquidWriteArchive: writing mesh shape: " << objDagPath.fullPathName() << endl; }
      liqRibObj ribObj(objDagPath, MRT_Mesh);
      ribObj.writeObject();
      if (debug) { cout << "liquidWriteArchive: finished writing mesh shape" << endl; }
    } else if (objDagPath.hasFn(MFn::kNurbsSurface)) {
      if (debug) { cout << "liquidWriteArchive: writing nurbs shape: " << objDagPath.fullPathName() << endl; }
      liqRibObj ribObj(objDagPath, MRT_Nurbs);
      ribObj.writeObject();
      if (debug) { cout << "liquidWriteArchive: finished writing nurbs shape" << endl; }
    } else {
      MGlobal::displayWarning("skipping unknown geometry type '" + MString(objDagPath.node().apiTypeStr()) + "'in liquidWriteArchive");
    }
  } else {
    // we're looking at a transform node
    bool wroteTransform = false;
    if (writeTransform && (objDagPath.apiType() == MFn::kTransform)) {
      if (debug) { cout << "liquidWriteArchive: writing transform: " << objDagPath.fullPathName() << endl; }
      // push the transform onto the RIB stack
      outputObjectName(objDagPath);
      MFnDagNode mfnDag(objDagPath);
      MMatrix tm = mfnDag.transformationMatrix();
      if (true) { // (!tm.isEquivalent(MMatrix::identity)) {
        RtMatrix riTM;
        tm.get(riTM);
        wroteTransform = true;
        outputIndentation();
        RiAttributeBegin();
        indentLevel++;
        outputIndentation();
        RiConcatTransform(riTM);
      }
    }
    // go through all the children of this node and deal with each of them
    int nChildren = objDagPath.childCount();
    if (debug) { cout << "liquidWriteArchive: object " << objDagPath.fullPathName() << "has " << nChildren << " children" << endl; }
    for(int i=0; i<nChildren; ++i) {
      if (debug) { cout << "liquidWriteArchive: writing child number " << i << endl; }
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
  if (debug) { cout << "liquidWriteArchive: finished writing object: " << objDagPath.fullPathName() << endl; }
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
