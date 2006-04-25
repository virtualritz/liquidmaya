//-
// ==========================================================================
// Copyright (C) 1995 - 2005 Alias Systems Corp. and/or its licensors.  All
// rights reserved.
//
// The coded instructions, statements, computer programs, and/or related
// material (collectively the "Data") in these files are provided by Alias
// Systems Corp. ("Alias") and/or its licensors for the exclusive use of the
// Customer (as defined in the Alias Software License Agreement that
// accompanies this Alias software). Such Customer has the right to use,
// modify, and incorporate the Data into other products and to distribute such
// products for use by end-users.
//
// THE DATA IS PROVIDED "AS IS".  ALIAS HEREBY DISCLAIMS ALL WARRANTIES
// RELATING TO THE DATA, INCLUDING, WITHOUT LIMITATION, ANY AND ALL EXPRESS OR
// IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE. IN NO EVENT SHALL ALIAS BE LIABLE FOR ANY DAMAGES
// WHATSOEVER, WHETHER DIRECT, INDIRECT, SPECIAL, OR PUNITIVE, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, OR IN EQUITY,
// ARISING OUT OF ACCESS TO, USE OF, OR RELIANCE UPON THE DATA.
// ==========================================================================
//+

#include <maya/MPxLocatorNode.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MVector.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MColor.h>
#include <maya/M3dView.h>
#include <maya/MFnEnumAttribute.h>

#include <liqCoordSysNode.h>
#include <liqMayaNodeIds.h>

#if defined(OSMac_MachO_)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <liqIOStream.h>

MTypeId liqCoordSysNode::id( liqCoordSysNodeId );
MObject liqCoordSysNode::aType;

#define MAKE_INPUT(attr)		\
    CHECK_MSTATUS(attr.setKeyable(true)); 		\
    CHECK_MSTATUS(attr.setStorable(true));		\
    CHECK_MSTATUS(attr.setReadable(true)); 		\
    CHECK_MSTATUS(attr.setWritable(true));

#define MAKE_NONKEYABLE_INPUT(attr)		\
    CHECK_MSTATUS(attr.setKeyable(false));  \
    CHECK_MSTATUS(attr.setStorable(true));  \
    CHECK_MSTATUS(attr.setReadable(true));  \
    CHECK_MSTATUS(attr.setWritable(true));



void* liqCoordSysNode::creator()
{
  return new liqCoordSysNode;
}


MStatus liqCoordSysNode::initialize()
{
  MFnEnumAttribute  eAttr;
  MStatus           status;

  aType = eAttr.create( "type", "t", 0, &status );
  eAttr.addField( "Card",           0 );
  eAttr.addField( "Sphere",         1 );
  eAttr.addField( "Cylinder",       2 );
  eAttr.addField( "Cube",           3 );
  eAttr.addField( "Deep Card",      4 );
  eAttr.addField( "Clipping Plane", 5 );
  MAKE_INPUT(eAttr);
  CHECK_MSTATUS(eAttr.setConnectable(false));

  CHECK_MSTATUS( addAttribute( aType ) );

  return MS::kSuccess;
}



MStatus liqCoordSysNode::compute( const MPlug& /*plug*/, MDataBlock& /*data*/ )
{
  return MS::kUnknownParameter;
}

void liqCoordSysNode::draw(  M3dView & view, const MDagPath & /*path*/,
                             M3dView::DisplayStyle style,
                             M3dView::DisplayStatus status )
{
  // Get the type
  //
  MObject thisNode = thisMObject();
  MPlug typePlug( thisNode, aType );

  CHECK_MSTATUS(typePlug.getValue( coordType ));

  view.beginGL();

  // Draw the arrows
  //

  glPushAttrib( GL_ALL_ATTRIB_BITS );

  glBegin( GL_LINES );

    glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
    glVertex3f(  0.00f,  0.00f,  0.00f );
    glVertex3f(  0.50f,  0.00f,  0.00f );
    glVertex3f(  0.50f,  0.00f,  0.00f );
    glVertex3f(  0.40f, -0.05f,  0.00f );
    glVertex3f(  0.40f, -0.05f,  0.00f );
    glVertex3f(  0.40f,  0.00f,  0.00f );

    glColor4f( 0.0f, 1.0f, 0.0f, 1.0f );
    glVertex3f(  0.00f,  0.00f,  0.00f  );
    glVertex3f(  0.00f,  0.50f,  0.00f  );
    glVertex3f(  0.00f,  0.50f,  0.00f  );
    glVertex3f(  0.05f,  0.40f,  0.00f  );
    glVertex3f(  0.05f,  0.40f,  0.00f  );
    glVertex3f(  0.00f,  0.40f,  0.00f  );

    glColor4f( 0.0f, 0.0f, 1.0f, 1.0f );
    glVertex3f(  0.00f,  0.00f,  0.00f   );
    glVertex3f(  0.00f,  0.00f,  0.50f   );
    glVertex3f(  0.00f,  0.00f,  0.50f   );
    glVertex3f(  0.00f, -0.05f,  0.40f   );
    glVertex3f(  0.00f, -0.05f,  0.40f   );
    glVertex3f(  0.00f,  0.00f,  0.40f   );

  glEnd();

  glPopAttrib();


  // draw the warious primitives
  //

  switch( coordType ) {

    case 0:
      // PLANE
      glPushAttrib( GL_ALL_ATTRIB_BITS );
      glBegin( GL_LINES );
        glVertex3f( -0.50f,  0.50f,  0.00f );
        glVertex3f(  0.50f,  0.50f,  0.00f );
        glVertex3f(  0.50f,  0.50f,  0.00f );
        glVertex3f(  0.50f, -0.50f,  0.00f );
        glVertex3f(  0.50f, -0.50f,  0.00f );
        glVertex3f( -0.50f, -0.50f,  0.00f );
        glVertex3f( -0.50f, -0.50f,  0.00f );
        glVertex3f( -0.50f,  0.50f,  0.00f );
      glEnd();
      glPopAttrib();
      break;

    case 1:
      // SPHERE
      {
        glRotatef( -90.0f, 1.0f, 0.0f, 0.0f );
        GLUquadricObj* pQuadric = gluNewQuadric();
        gluQuadricDrawStyle( pQuadric, GLU_LINE);
        gluSphere( pQuadric, 0.5f , 10, 6);
        gluDeleteQuadric(pQuadric);
      }
      break;

    case 2:
      // CYLINDER
      {
        glTranslatef( 0.0f, -0.5f, 0.0f );
        glRotatef( -90.0f, 1.0f, 0.0f, 0.0f );
        GLUquadricObj* pQuadric = gluNewQuadric();
        gluQuadricDrawStyle( pQuadric, GLU_LINE);
        gluCylinder( pQuadric, 0.5f , 0.5f, 1.0f, 10, 2);
        gluDeleteQuadric(pQuadric);
      }
      break;

    case 3:
      // CUBE
      glBegin( GL_LINES );

        glVertex3f( -0.50f,  0.50f, -0.50f );
        glVertex3f(  0.50f,  0.50f, -0.50f );
        glVertex3f(  0.50f,  0.50f, -0.50f );
        glVertex3f(  0.50f,  0.50f,  0.50f );
        glVertex3f(  0.50f,  0.50f,  0.50f );
        glVertex3f( -0.50f,  0.50f,  0.50f );
        glVertex3f( -0.50f,  0.50f,  0.50f );
        glVertex3f( -0.50f,  0.50f, -0.50f );

        glVertex3f( -0.50f, -0.50f, -0.50f );
        glVertex3f(  0.50f, -0.50f, -0.50f );
        glVertex3f(  0.50f, -0.50f, -0.50f );
        glVertex3f(  0.50f, -0.50f,  0.50f );
        glVertex3f(  0.50f, -0.50f,  0.50f );
        glVertex3f( -0.50f, -0.50f,  0.50f );
        glVertex3f( -0.50f, -0.50f,  0.50f );
        glVertex3f( -0.50f, -0.50f, -0.50f );

        glVertex3f( -0.50f,  0.50f, -0.50f );
        glVertex3f( -0.50f, -0.50f, -0.50f );
        glVertex3f(  0.50f,  0.50f, -0.50f );
        glVertex3f(  0.50f, -0.50f, -0.50f );
        glVertex3f(  0.50f,  0.50f,  0.50f );
        glVertex3f(  0.50f, -0.50f,  0.50f );
        glVertex3f( -0.50f,  0.50f,  0.50f );
        glVertex3f( -0.50f, -0.50f,  0.50f );

      glEnd();
      break;

    case 4:
      // DEEP PLANE
      glBegin( GL_LINES );
        glVertex3f( -0.50f,  0.50f,  0.00f );
        glVertex3f(  0.50f,  0.50f,  0.00f );
        glVertex3f(  0.50f,  0.50f,  0.00f );
        glVertex3f(  0.50f, -0.50f,  0.00f );
        glVertex3f(  0.50f, -0.50f,  0.00f );
        glVertex3f( -0.50f, -0.50f,  0.00f );
        glVertex3f( -0.50f, -0.50f,  0.00f );
        glVertex3f( -0.50f,  0.50f,  0.00f );
      glEnd();
      glPushAttrib( GL_ALL_ATTRIB_BITS );
      glEnable( GL_LINE_STIPPLE);
      glLineStipple(1,0xff);
      glBegin( GL_LINES );
        glVertex3f( -0.50f,  0.50f,  1.00f );
        glVertex3f(  0.50f,  0.50f,  1.00f );
        glVertex3f(  0.50f,  0.50f,  1.00f );
        glVertex3f(  0.50f, -0.50f,  1.00f );
        glVertex3f(  0.50f, -0.50f,  1.00f );
        glVertex3f( -0.50f, -0.50f,  1.00f );
        glVertex3f( -0.50f, -0.50f,  1.00f );
        glVertex3f( -0.50f,  0.50f,  1.00f );
      glEnd();
      glPopAttrib();
      break;

    case 5:
      // CLIPPING PLANE
      glPushAttrib( GL_ALL_ATTRIB_BITS );

      glEnable( GL_LINE_STIPPLE);
      glLineStipple(1,0xff);

      glBegin( GL_LINES );

        glVertex3f( -1.0f,  1.0f,  0.0f );
        glVertex3f(  1.0f,  1.0f,  0.0f );
        glVertex3f(  1.0f,  1.0f,  0.0f );
        glVertex3f(  1.0f, -1.0f,  0.0f );
        glVertex3f(  1.0f, -1.0f,  0.0f );
        glVertex3f( -1.0f, -1.0f,  0.0f );
        glVertex3f( -1.0f, -1.0f,  0.0f );
        glVertex3f( -1.0f,  1.0f,  0.0f );

        glVertex3f( -1.0f,  1.0f,  0.0f );
        glVertex3f(  1.0f, -1.0f,  0.0f );
        glVertex3f(  1.0f,  1.0f,  0.0f );
        glVertex3f( -1.0f, -1.0f,  0.0f );

      glEnd();

      glPopAttrib();
      break;
  }



  view.endGL();
}


bool liqCoordSysNode::isBounded() const
{
  return true;
}


MBoundingBox liqCoordSysNode::boundingBox() const
{
  MPoint corner1;
  MPoint corner2;

  switch( coordType ) {

    case 0:
      corner1 = MPoint( -0.5, -0.5,  0.0 );
      corner2 = MPoint(  0.5,  0.5,  0.5 );
      break;

    case 4:
      corner1 = MPoint( -0.5, -0.5,  0.0 );
      corner2 = MPoint(  0.5,  0.5,  1.0 );
      break;

    case 5:
      corner1 = MPoint( -1.0, -1.0,  0.0 );
      corner2 = MPoint(  1.0,  1.0,  0.5 );
      break;

    default:
      corner1 = MPoint( -0.5, -0.5, -0.5 );
      corner2 = MPoint(  0.5,  0.5,  0.5 );
      break;
  }

  return MBoundingBox( corner1, corner2 );
}



