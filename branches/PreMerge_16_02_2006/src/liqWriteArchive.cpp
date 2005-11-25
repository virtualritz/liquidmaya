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

#include <maya/MArgList.h>
#include <maya/MSyntax.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MMatrix.h>
#include <maya/MFnDagNode.h>
#include <maya/MArgParser.h>

#include <liqIOStream.h>

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
  if (!isObjectVisible(objDagPath)) {
    return;
  }

  if (debug) { cout << "liquidWriteArchive: writing object: " << objDagPath.fullPathName().asChar() << endl; }
  if (objDagPath.node().hasFn(MFn::kShape) || objDagPath.node().hasFn(MFn::kPlace3dTexture)) {
    // we're looking at a shape node, so write out the geometry to the RIB
    outputObjectName(objDagPath);

    liqRibNode ribNode;
    ribNode.set(objDagPath, 0, MRT_Unknown);

    if ( ribNode.rib.box != "" && ribNode.rib.box != "-" ) {
      RiArchiveRecord( RI_COMMENT, "Additional RIB:\n%s", ribNode.rib.box.asChar() );
    }
    if ( ribNode.rib.readArchive != "" && ribNode.rib.readArchive != "-" ) {
      RiArchiveRecord( RI_COMMENT, "Read Archive Data: \nReadArchive \"%s\"", ribNode.rib.readArchive.asChar() );
    }
    if ( ribNode.rib.delayedReadArchive != "" && ribNode.rib.delayedReadArchive != "-" ) {
      RiArchiveRecord( RI_COMMENT, "Delayed Read Archive Data: \nProcedural \"DelayedReadArchive\" [ \"%s\" ] [ %f %f %f %f %f %f ]", ribNode.rib.delayedReadArchive.asChar(), ribNode.bound[0], ribNode.bound[3], ribNode.bound[1], ribNode.bound[4], ribNode.bound[2], ribNode.bound[5]);
    }

    // If it's a curve we should write the basis function
    if ( ribNode.object(0)->type == MRT_NuCurve ) {
      RiBasis( RiBSplineBasis, 1, RiBSplineBasis, 1 );
    }

    if (!ribNode.object(0)->ignore) {
      ribNode.object(0)->writeObject();
    }
  } else {
    // we're looking at a transform node
    bool wroteTransform = false;
    if (writeTransform && (objDagPath.apiType() == MFn::kTransform)) {
      if (debug) { cout << "liquidWriteArchive: writing transform: " << objDagPath.fullPathName().asChar() << endl; }
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
    if (debug) { cout << "liquidWriteArchive: object " << objDagPath.fullPathName().asChar() << "has " << nChildren << " children" << endl; }
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
  if (debug) { cout << "liquidWriteArchive: finished writing object: " << objDagPath.fullPathName().asChar() << endl; }
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
  RiArchiveRecord(RI_VERBATIM, "\n");
  outputIndentation();
  RtString ribname = const_cast< char* >( name.asChar() );
  RiAttribute( "identifier", "name", &ribname, RI_NULL );
}
