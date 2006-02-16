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
** Contributor(s): Philippe Leprince.
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
** Liquid Displacement Shader Node Source
** ______________________________________________________________________
*/

#include <liquid.h>
#include <liqDisplacementNode.h>
#include <liqNodeSwatch.h>

#include <maya/MGlobal.h>
#include <maya/MCommandResult.h>
#include <maya/MIOStream.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MSwatchRenderBase.h>
#include <maya/MSwatchRenderRegister.h>
#include <maya/MImage.h>
#include <maya/MFnDependencyNode.h>


#include <liqIOStream.h>

// static data
MTypeId liqDisplacementNode::id( 0x00103512 );

// Attributes
MObject liqDisplacementNode::aRmanShader;
MObject liqDisplacementNode::aRmanShaderLong;
MObject liqDisplacementNode::aRmanShaderLif;
MObject liqDisplacementNode::aPreviewPrimitive;
MObject liqDisplacementNode::aPreviewObjectSize;
MObject liqDisplacementNode::aPreviewShadingRate;
MObject liqDisplacementNode::aPreviewBackplane;

MObject liqDisplacementNode::aShaderSpace;
MObject liqDisplacementNode::aDisplacementBound;
MObject liqDisplacementNode::aOutputInShadow;
MObject liqDisplacementNode::aRefreshPreview;

MObject liqDisplacementNode::aDisplacement;
MObject liqDisplacementNode::aOutColor;

#define MAKE_INPUT(attr)		\
    CHECK_MSTATUS(attr.setKeyable(true)); 		\
    CHECK_MSTATUS(attr.setStorable(true));		\
    CHECK_MSTATUS(attr.setReadable(true)); 		\
    CHECK_MSTATUS(attr.setWritable(true));

#define MAKE_NONKEYABLE_INPUT(attr)		\
    CHECK_MSTATUS(attr.setKeyable(false)); 		\
    CHECK_MSTATUS(attr.setStorable(true));		\
    CHECK_MSTATUS(attr.setReadable(true)); 		\
    CHECK_MSTATUS(attr.setWritable(true));

#define MAKE_OUTPUT(attr)		\
    CHECK_MSTATUS(attr.setKeyable(false)); 		\
    CHECK_MSTATUS(attr.setStorable(false));		\
    CHECK_MSTATUS(attr.setReadable(true)); 		\
    CHECK_MSTATUS(attr.setWritable(false));

void liqDisplacementNode::postConstructor( )
{
	setMPSafe(true);

  // init swatch
  if ( swatchInit != true ) {
    MObject obj = MPxNode::thisMObject();
    renderSwatch = new liqNodeSwatch( obj, obj, 128 );
    swatchInit = true;
  }

  MGlobal::executeCommandOnIdle( "liquidCheckGlobals()", false );
}

liqDisplacementNode::liqDisplacementNode()
{
  swatchInit = false;
  renderSwatch = NULL;
}

liqDisplacementNode::~liqDisplacementNode()
{
  if (renderSwatch != NULL) delete renderSwatch;
}

void* liqDisplacementNode::creator()
{
    return new liqDisplacementNode();
}

MStatus liqDisplacementNode::initialize()
{
  MFnTypedAttribute   tAttr;
  MFnNumericAttribute nAttr;
  MFnEnumAttribute    eAttr;
  MStatus status;

  // Create input attributes

	aRmanShader = tAttr.create( MString("rmanShader"), MString("rms"), MFnData::kString, aRmanShader, &status );
	MAKE_INPUT(tAttr);

  aRmanShaderLong = tAttr.create( MString("rmanShaderLong"), MString("rml"), MFnData::kString, aRmanShaderLong, &status );
	MAKE_INPUT(tAttr);

	aRmanShaderLif = tAttr.create(  MString("rmanShaderLif"),  MString("lif"), MFnData::kString, aRmanShaderLif, &status );
	MAKE_INPUT(tAttr);

  aPreviewPrimitive = eAttr.create( "previewPrimitive", "pvp", 5, &status );
  eAttr.addField( "Sphere",   0 );
  eAttr.addField( "Cube",     1 );
  eAttr.addField( "Cylinder", 2 );
  eAttr.addField( "Torus",    3 );
  eAttr.addField( "Plane",    4 );
  eAttr.addField( "Teapot",   5 );
  MAKE_NONKEYABLE_INPUT(eAttr);
  CHECK_MSTATUS(eAttr.setConnectable(false));

  aPreviewObjectSize = nAttr.create("previewObjectSize", "pos", MFnNumericData::kDouble, 1.0, &status);
  MAKE_NONKEYABLE_INPUT(nAttr);
  CHECK_MSTATUS(nAttr.setConnectable(false));

  aPreviewShadingRate = nAttr.create("previewShadingRate", "psr", MFnNumericData::kDouble, 1.0, &status);
  MAKE_NONKEYABLE_INPUT(nAttr);
  CHECK_MSTATUS(nAttr.setConnectable(false));

  aPreviewBackplane = nAttr.create("previewBackplane", "pbp", MFnNumericData::kBoolean, true, &status);
  MAKE_NONKEYABLE_INPUT(nAttr);
  CHECK_MSTATUS(nAttr.setConnectable(false));

  aShaderSpace = tAttr.create( MString("shaderSpace"), MString("ssp"), MFnData::kString, aShaderSpace, &status );
	MAKE_INPUT(tAttr);

  aDisplacementBound = nAttr.create("displacementBound", "db", MFnNumericData::kDouble, 0.0, &status);
  MAKE_INPUT(nAttr);
  aOutputInShadow = nAttr.create("outputInShadow", "ois",  MFnNumericData::kBoolean, 0.0, &status);
  MAKE_NONKEYABLE_INPUT(nAttr);

  // refreshPreview must be true to allow refresh
  aRefreshPreview = nAttr.create("refreshPreview", "rfp",  MFnNumericData::kBoolean, 0.0, &status);
  MAKE_NONKEYABLE_INPUT(nAttr);
  CHECK_MSTATUS(nAttr.setHidden(true));

	// Create output attributes
  aDisplacement = nAttr.create("displacement", "d", MFnNumericData::kDouble, 0.0, &status);
	MAKE_OUTPUT(nAttr);
  aOutColor = nAttr.createColor("outColor", "oc");
	MAKE_OUTPUT(nAttr);

  CHECK_MSTATUS(addAttribute(aRmanShader));
  CHECK_MSTATUS(addAttribute(aRmanShaderLong));
  CHECK_MSTATUS(addAttribute(aRmanShaderLif));
  CHECK_MSTATUS(addAttribute(aPreviewPrimitive));
  CHECK_MSTATUS(addAttribute(aPreviewObjectSize));
  CHECK_MSTATUS(addAttribute(aPreviewShadingRate));
  CHECK_MSTATUS(addAttribute(aPreviewBackplane));
  CHECK_MSTATUS(addAttribute(aShaderSpace));
  CHECK_MSTATUS(addAttribute(aDisplacementBound));
  CHECK_MSTATUS(addAttribute(aOutputInShadow));
  CHECK_MSTATUS(addAttribute(aRefreshPreview));

  CHECK_MSTATUS(addAttribute(aDisplacement));
  CHECK_MSTATUS(addAttribute(aOutColor));

  CHECK_MSTATUS(attributeAffects(aRmanShaderLong, aDisplacement));
  CHECK_MSTATUS(attributeAffects(aRmanShaderLif, aDisplacement));

  return MS::kSuccess;
}

//MStatus liqDisplacementNode::compute( const MPlug& plug, MDataBlock& block )
//{
//  //cout <<"compute !"<<endl;
//
//	// outColor or individual R, G, B channel
//  if((plug != aDisplacement) && (plug.parent() != aDisplacement))
//  return MS::kUnknownParameter;
//
//  return MS::kSuccess;
//}



