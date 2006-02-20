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
** Liquid Surface Shader Node Source
** ______________________________________________________________________
*/

#include <liquid.h>
#include <liqSurfaceNode.h>
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
MTypeId liqSurfaceNode::id( 0x00103511 );

// Attributes
MObject liqSurfaceNode::aRmanShader;
MObject liqSurfaceNode::aRmanShaderLong;
MObject liqSurfaceNode::aRmanShaderLif;
MObject liqSurfaceNode::aPreviewPrimitive;
MObject liqSurfaceNode::aColor;
MObject liqSurfaceNode::aOpacity;
MObject liqSurfaceNode::aShaderSpace;
MObject liqSurfaceNode::aDisplacementBound;
MObject liqSurfaceNode::aOutputInShadow;
MObject liqSurfaceNode::aResolution;
MObject liqSurfaceNode::aRefreshPreview;
MObject liqSurfaceNode::aPreviewObjectSize;
MObject liqSurfaceNode::aPreviewShadingRate;
MObject liqSurfaceNode::aPreviewBackplane;

MObject liqSurfaceNode::aOutColor;
MObject liqSurfaceNode::aOutTransparency;

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

void liqSurfaceNode::postConstructor( )
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

liqSurfaceNode::liqSurfaceNode()
{
  swatchInit = false;
  renderSwatch = NULL;
}

liqSurfaceNode::~liqSurfaceNode()
{
  if (renderSwatch != NULL) delete renderSwatch;
}

void* liqSurfaceNode::creator()
{
    return new liqSurfaceNode();
}

MStatus liqSurfaceNode::initialize()
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

  aPreviewPrimitive = eAttr.create( "previewPrimitive", "pvp", 6, &status );
  eAttr.addField( "Sphere",   0 );
  eAttr.addField( "Cube",     1 );
  eAttr.addField( "Cylinder", 2 );
  eAttr.addField( "Torus",    3 );
  eAttr.addField( "Plane",    4 );
  eAttr.addField( "Teapot",   5 );
  eAttr.addField( "(globals)",6 );
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

  aColor = nAttr.createColor("color", "cs");
  nAttr.setDefault( 1.0, 1.0, 1.0 );
  MAKE_INPUT(nAttr);
  aOpacity = nAttr.createColor("opacity", "os");
  nAttr.setDefault( 1.0, 1.0, 1.0 );
  MAKE_INPUT(nAttr);
  aShaderSpace = tAttr.create( MString("shaderSpace"), MString("ssp"), MFnData::kString, aShaderSpace, &status );
	MAKE_INPUT(tAttr);
  aDisplacementBound = nAttr.create("displacementBound", "db", MFnNumericData::kDouble, 0.0, &status);
  MAKE_INPUT(nAttr);
  aOutputInShadow = nAttr.create("outputInShadow", "ois",  MFnNumericData::kBoolean, 0.0, &status);
  MAKE_NONKEYABLE_INPUT(nAttr);

  // resolution attribute for maya's hardware renderer
  aResolution = nAttr.create("resolution", "res",  MFnNumericData::kInt, 16, &status);
  CHECK_MSTATUS(nAttr.setStorable( true ));
  CHECK_MSTATUS(nAttr.setReadable( true ));
  CHECK_MSTATUS(nAttr.setWritable( true ));
  CHECK_MSTATUS(nAttr.setHidden( true ));

  // refreshPreview must be true to allow refresh
  aRefreshPreview = nAttr.create("refreshPreview", "rfp",  MFnNumericData::kBoolean, 0.0, &status);
  MAKE_NONKEYABLE_INPUT(nAttr);
  CHECK_MSTATUS(nAttr.setHidden(true));


	// Create output attributes
  aOutColor = nAttr.createColor("outColor", "oc");
	MAKE_OUTPUT(nAttr);
  aOutTransparency = nAttr.createColor("outTransparency", "ot");
	MAKE_OUTPUT(nAttr);

  CHECK_MSTATUS(addAttribute(aRmanShader));
  CHECK_MSTATUS(addAttribute(aRmanShaderLong));
  CHECK_MSTATUS(addAttribute(aRmanShaderLif));
  CHECK_MSTATUS(addAttribute(aPreviewPrimitive));
  CHECK_MSTATUS(addAttribute(aPreviewObjectSize));
  CHECK_MSTATUS(addAttribute(aPreviewShadingRate));
  CHECK_MSTATUS(addAttribute(aPreviewBackplane));
  CHECK_MSTATUS(addAttribute(aColor));
  CHECK_MSTATUS(addAttribute(aOpacity));
  CHECK_MSTATUS(addAttribute(aShaderSpace));
  CHECK_MSTATUS(addAttribute(aDisplacementBound));
  CHECK_MSTATUS(addAttribute(aOutputInShadow));
  CHECK_MSTATUS(addAttribute(aResolution));
  CHECK_MSTATUS(addAttribute(aRefreshPreview));
  CHECK_MSTATUS(addAttribute(aOutColor));
  CHECK_MSTATUS(addAttribute(aOutTransparency));

  CHECK_MSTATUS(attributeAffects( aColor,          aOutColor ));
  CHECK_MSTATUS(attributeAffects( aOpacity,        aOutColor ));

  return MS::kSuccess;
}

MStatus liqSurfaceNode::compute( const MPlug& plug, MDataBlock& block )
{
	// outColor or individual R, G, B channel
  if((plug != aOutColor) && (plug.parent() != aOutColor))
  return MS::kUnknownParameter;

  // init shader
  MStatus status;
  MFloatVector& cColor  = block.inputValue(aColor).asFloatVector();
  MFloatVector& cTrans  = block.inputValue(aOpacity).asFloatVector();

  MFloatVector resultColor( cColor );
  MFloatVector resultTrans( cTrans );
  resultTrans[0] = ( 1 - resultTrans[0] );
  resultTrans[1] = ( 1 - resultTrans[1] );
  resultTrans[2] = ( 1 - resultTrans[2] );
  cout <<resultTrans[0]<<endl;

  // set ouput color attribute
	MDataHandle outColorHandle = block.outputValue( aOutColor );
	MFloatVector& outColor = outColorHandle.asFloatVector();
	outColor = resultColor;
	outColorHandle.setClean();

  MDataHandle outTransHandle = block.outputValue( aOutTransparency );
	MFloatVector& outTrans = outTransHandle.asFloatVector();
	outTrans = resultTrans;
	outTransHandle.setClean();


  return MS::kSuccess;
}




