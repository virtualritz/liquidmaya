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

/* ______________________________________________________________________
**
** Liquid Rib Node Source
** ______________________________________________________________________
*/


// RenderMan headers
extern "C" {
#include <ri.h>
}

// Maya headers
#include <maya/MPlug.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnBlinnShader.h>
#include <maya/MFnPhongShader.h>
#include <maya/MPlugArray.h>
#include <maya/MObjectArray.h>
#include <maya/MFnSet.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDagNode.h>
#include <maya/MBoundingBox.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnParticleSystem.h>
#include <maya/MVectorArray.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MPointArray.h>

// Liquid headers
#include <liquid.h>
#include <liqGlobalHelpers.h>
#include <liqRibNode.h>

// Standard/Boost headers
#include <list>
#include <boost/scoped_array.hpp>


#ifdef _WIN32
#undef min
#undef max
#endif

extern int debugMode;

extern MStringArray liqglo_preReadArchive;
extern MStringArray liqglo_preRibBox;
extern MStringArray liqglo_preReadArchiveShadow;
extern MStringArray liqglo_preRibBoxShadow;
extern MString      liqglo_currentNodeName;
extern MString      liqglo_currentNodeShortName;


/**
 * Class constructor.
 */
liqRibNode::liqRibNode( liqRibNodePtr instanceOfNode,
                        const MString instanceOfNodeStr )
  :   matXForm( MRX_Const ),
      bodyXForm( MRX_Const ),
      instance( instanceOfNode ),
      instanceStr( instanceOfNodeStr ),
      overrideColor( false )
{
  LIQDEBUGPRINTF( "-> creating rib node\n");
  objects.resize( LIQMAXMOTIONSAMPLES );
  //for( unsigned i = 0; i < LIQMAXMOTIONSAMPLES; i++ )
  //  objects[ i ] = NULL;

  name.clear();
  mayaMatteMode             = false;

  shading.shadingRate       = -1.0f;
  shading.diceRasterOrient  = true;
  shading.color.r           = -1.0;
  shading.opacity.r         = -1.0;
  shading.matte             = false;
  shading.doubleShaded      = false;

  trace.displacements       = false;
  trace.sampleMotion        = false;
  trace.bias                = 0.01f;
  trace.maxDiffuseDepth     = 1;
  trace.maxSpecularDepth    = 2;

  visibility.camera         = true;
  // philippe: pre-prman 12.5 style
  visibility.trace          = false;
  visibility.transmission   = visibility::TRANSMISSION_TRANSPARENT;
  // philippe: prman 12.5 style
  visibility.diffuse        = false;
  visibility.specular       = false;
  visibility.newtransmission = false;
  visibility.midpoint       = true;
  visibility.photon         = false;

  hitmode.diffuse           = hitmode::DIFFUSE_HITMODE_PRIMITIVE;
  hitmode.specular          = hitmode::SPECULAR_HITMODE_SHADER;
  hitmode.transmission      = hitmode::TRANSMISSION_HITMODE_SHADER;
  hitmode.camera            = hitmode::CAMERA_HITMODE_SHADER;

  irradiance.shadingRate    = 1.0f;
  irradiance.nSamples       = 64;
  irradiance.maxError       = 0.5f;
  irradiance.maxPixelDist   = 30.0f;
  irradiance.handle         = "";
  irradiance.fileMode       = irradiance::FILEMODE_NONE;

  photon.globalMap          = "";
  photon.causticMap         = "";
  photon.shadingModel       = photon::SHADINGMODEL_MATTE;
  photon.estimator          = 100;

  motion.transformationBlur = true;;
  motion.deformationBlur    = true;
  motion.samples            = 2;
  motion.factor             = 1.0;

  rib.box                   = "";
  rib.generator             = "";
  rib.readArchive           = "";
  rib.delayedReadArchive    = "";

  delightSSS.doScatter = false;
  delightSSS.shadingRate = 1.0;
  delightSSS.groupName = "";
  delightSSS.scattering.r = 0.0;
  delightSSS.scattering.g = 0.0;
  delightSSS.scattering.b = 0.0;
  delightSSS.absorption.r = 0.0;
  delightSSS.absorption.g = 0.0;
  delightSSS.absorption.b = 0.0;
  delightSSS.refraction = 0.0;
  delightSSS.scale = 1.0;

  invisible                 = false;
  ignoreShapes              = false;
}

/**
 * Class destructor.
 */
liqRibNode::~liqRibNode()
{
  LIQDEBUGPRINTF( "-> killing rib node %s\n", name.asChar() );

  for( unsigned i( 0 ); i < LIQMAXMOTIONSAMPLES; i++ ) {
    if( objects[ i ] ) {
      LIQDEBUGPRINTF( "-> killing %d. ref\n", i );
      objects[ i ]->unref();
      //objects[ i ] = NULL;
    }
  }
  LIQDEBUGPRINTF( "-> killing no obj\n" );
  //name.clear();
  //irradiance.handle.clear();
  //photon.globalMap.clear();
  //photon.causticMap.clear();
  //rib.box.clear();
  //rib.generator.clear();
  //rib.readArchive.clear();
  //rib.delayedReadArchive.clear();

  LIQDEBUGPRINTF( "-> finished killing rib node.\n" );
}

/**
 * Get the object referred to by this node.
 * This returns the surface, mesh, light, etc. this node points to.
 */
liqRibObjPtr liqRibNode::object( unsigned interval )
{
  return objects[ interval ];
}

/**
 * Set this node with the given path.
 * If this node already refers to the given object, then it is assumed that the
 * path represents the object at the next frame.
 * This method also scans the dag upwards and thereby sets any attributes
 * Liquid knows that have non-default values and sets them for to this node.
 */
void liqRibNode::set( const MDagPath &path, int sample, ObjectType objType, int particleId )
{
  LIQDEBUGPRINTF( "-> setting rib node\n");
  DagPath = path;
#if 0
  int instanceNum = path.instanceNumber();
#endif
  MStatus status;
  MFnDagNode fnNode( path );
  MPlug nPlug;
  status.clear();

  MSelectionList hierarchy; // needed to find objectSets later below
  MDagPath dagSearcher( path );

  liqglo_currentNodeName      = path.fullPathName();
  liqglo_currentNodeShortName = path.partialPathName();

  do { // while( dagSearcher.length() > 0 )
    dagSearcher.pop(); // Go upwards (should be a transform node)

    hierarchy.add( dagSearcher, MObject::kNullObj, true );

    if( dagSearcher.apiType( &status ) == MFn::kTransform ) {
      MFnDagNode nodePeeker( dagSearcher );

      // Shading. group ----------------------------------------------------------
      if( !invisible ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "template" ), &status );
        if( status == MS::kSuccess ) {
          nPlug.getValue( invisible );
          if( invisible )
            break; // Exit do..while loop -- IF OBJECT ATTRIBUTES NEED TO BE PARSED FOR INVISIBLE OBJECTS TOO IN THE FUTURE -- REMOVE THIS LINE!
        } else {
          status.clear();
          nPlug = nodePeeker.findPlug( MString( "liqInvisible" ), &status );
          if( status == MS::kSuccess ) {
            nPlug.getValue( invisible );
            if( invisible )
              break; // Exit do..while loop -- IF OBJECT ATTRIBUTES NEED TO BE PARSED FOR INVISIBLE OBJECTS TOO IN THE FUTURE -- REMOVE THIS LINE!
          }
        }
      }

      if( shading.shadingRate == -1.0f ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqShadingRate" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( shading.shadingRate );
      }

      if( shading.diceRasterOrient == true ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqDiceRasterOrient" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( shading.diceRasterOrient );
      }

      if(shading.color.r == -1.0f) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqColor"), &status );
        if( status == MS::kSuccess) {
          MPlug tmpPlug;
          tmpPlug = nPlug.child(0,&status);
          if(status == MS::kSuccess) tmpPlug.getValue( shading.color.r );
          tmpPlug = nPlug.child(1,&status);
          if(status == MS::kSuccess) tmpPlug.getValue( shading.color.g );
          tmpPlug = nPlug.child(2,&status);
          if(status == MS::kSuccess) tmpPlug.getValue( shading.color.b );
        }
      }

      if(shading.opacity.r == -1.0f) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqOpacity"), &status );
        if( status == MS::kSuccess) {
        MPlug tmpPlug;
        tmpPlug = nPlug.child(0,&status);
        if(status == MS::kSuccess) tmpPlug.getValue( shading.opacity.r );
        tmpPlug = nPlug.child(1,&status);
        if(status == MS::kSuccess) tmpPlug.getValue( shading.opacity.g );
        tmpPlug = nPlug.child(2,&status);
        if(status == MS::kSuccess) tmpPlug.getValue( shading.opacity.b );
        }
      }

      status.clear();
      nPlug = nodePeeker.findPlug( MString( "liqMatte" ), &status );
      if( status == MS::kSuccess) {
        nPlug.getValue( shading.matte );
      }

 		status.clear();
		nPlug = nodePeeker.findPlug( MString( "liqDoubleShaded" ), &status );
		if( status == MS::kSuccess )
		  nPlug.getValue( shading.doubleShaded );


      // trace group ----------------------------------------------------------
      if( trace.sampleMotion == false ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqTraceSampleMotion" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( trace.sampleMotion );
      }

      if( trace.displacements == false ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqTraceDisplacements" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( trace.displacements );
      }

      if( trace.bias == 0.01f ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqTraceBias" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( trace.bias );
      }

      if( trace.maxDiffuseDepth == 1 ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqMaxDiffuseDepth" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( trace.maxDiffuseDepth );
      }

      if( trace.maxSpecularDepth == 2 ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqMaxSpecularDepth" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( trace.maxSpecularDepth );
      }

      // visibility group -----------------------------------------------------

      if( visibility.camera == true ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqVisibilityCamera" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( visibility.camera );
      }

      // philippe : deprecated in prman 12.5
      if( visibility.trace == false ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqVisibilityTrace" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( visibility.trace );
      }

      if( visibility.transmission == visibility::TRANSMISSION_TRANSPARENT ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqVisibilityTransmission" ), &status );
        if( status == MS::kSuccess ) {
		  int tmpInt;
          nPlug.getValue( tmpInt  );
		  visibility.transmission = ( visibility::Transmission )tmpInt;
		}
      }

      // philippe : new visibility attributes in prman 12.5

      if( visibility.diffuse == false ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqVisibilityDiffuse" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( visibility.diffuse );
      }

      if( visibility.specular == false ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqVisibilitySpecular" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( visibility.specular );
      }

      if( visibility.newtransmission == false ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqVisibilityNewTransmission" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( visibility.newtransmission );
      }

      if( visibility.photon == false ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqVisibilityPhoton" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( visibility.photon );
      }
      // philippe : new shading hit-mode attributes in prman 12.5

      if( hitmode.camera == hitmode::CAMERA_HITMODE_SHADER ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqHitModeCamera" ), &status );
        if( status == MS::kSuccess ) {
		  int tmpInt;
          nPlug.getValue( tmpInt  );
		  hitmode.camera = ( hitmode::Camera ) tmpInt;
		}
      }

      if( hitmode.diffuse == hitmode::DIFFUSE_HITMODE_PRIMITIVE ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqHitModeDiffuse" ), &status );
        if( status == MS::kSuccess ) {
		  int tmpInt;
          nPlug.getValue( tmpInt  );
		  hitmode.diffuse = ( hitmode::Diffuse ) tmpInt;
		}
      }

      if( hitmode.specular == hitmode::SPECULAR_HITMODE_SHADER ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqHitModeSpecular" ), &status );
        if( status == MS::kSuccess ) {
		  int tmpInt;
          nPlug.getValue( tmpInt );
		   hitmode.specular= ( hitmode::Specular ) tmpInt;
		}
      }

      if( hitmode.transmission == hitmode::TRANSMISSION_HITMODE_SHADER ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqHitModeTransmission" ), &status );
        if( status == MS::kSuccess ) {
		  int tmpInt;
          nPlug.getValue( tmpInt );
		  hitmode.transmission = ( hitmode::Transmission )tmpInt;
		}
      }

      // irradiance group -----------------------------------------------------

      if( irradiance.shadingRate == 1.0f ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqIrradianceShadingRate" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( irradiance.shadingRate );
      }

      if( irradiance.nSamples == 64 ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqIrradianceNSamples" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( irradiance.nSamples );
      }

      if( irradiance.maxError == 0.5f ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqIrradianceMaxError" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( irradiance.maxError );
      }

      if( irradiance.maxPixelDist == 30.0f ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqIrradianceMaxPixelDist" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( irradiance.maxPixelDist );
      }

      if( irradiance.handle == "" ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqIrradianceHandle" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( irradiance.handle );
      }

      if( irradiance.fileMode == irradiance::FILEMODE_NONE ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqIrradianceFileMode" ), &status );
        if( status == MS::kSuccess ) {
		  int tmpInt;
          nPlug.getValue( tmpInt );
		  irradiance.fileMode = ( irradiance::FileMode )tmpInt;
		}
      }

      // photon group ---------------------------------------------------------

      if( photon.globalMap == "" ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqPhotonGlobalMap" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( photon.globalMap );
      }

      if( photon.causticMap == "" ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqPhotonCausticMap" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( photon.causticMap );
      }

      if( photon.shadingModel == photon::SHADINGMODEL_MATTE ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqPhotonShadingModel" ), &status );
        if( status == MS::kSuccess ) {
		  int tmpInt;
          nPlug.getValue( tmpInt );
		  photon.shadingModel = ( photon::ShadingModel ) tmpInt;
		}
      }

      if( photon.estimator == 100 ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqPhotonEstimator" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( photon.estimator );
      }

      // Motion blur group ---------------------------------------------------------
      // DOES NOT SEEM TO OVERRIDE GLOBALS
      if( motion.transformationBlur == true ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqTransformationBlur" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( motion.transformationBlur );
      }
      if( motion.deformationBlur == true ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqDeformationBlur" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( motion.deformationBlur );
      }
      if( motion.samples == 2 ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqMotionSamples" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( motion.samples );
      }

      if( motion.factor == 1.0f ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqMotionFactor" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( motion.factor );
      }

		// 3Delight sss group ---------------------------------------------------------
		nPlug = nodePeeker.findPlug( MString( "liqDelightSSShadingRate" ), &status );
		if( status == MS::kSuccess )
		{
			nPlug.getValue( delightSSS.shadingRate );
			delightSSS.doScatter = true;
		}

		nPlug = nodePeeker.findPlug( MString( "liqDelightSSGroupName" ), &status );
		if( status == MS::kSuccess )
			nPlug.getValue( delightSSS.groupName );

		nPlug = nodePeeker.findPlug( MString( "SSScattering1" ), &status );
		if( status == MS::kSuccess )
			nPlug.getValue( delightSSS.scattering.r );
		nPlug = nodePeeker.findPlug( MString( "SSScattering2" ), &status );
		if( status == MS::kSuccess )
			nPlug.getValue( delightSSS.scattering.g );
		nPlug = nodePeeker.findPlug( MString( "SSScattering3" ), &status );
		if( status == MS::kSuccess )
			nPlug.getValue( delightSSS.scattering.b );

		nPlug = nodePeeker.findPlug( MString( "SSAbsorption1" ), &status );
		if( status == MS::kSuccess )
			nPlug.getValue( delightSSS.absorption.r );
		nPlug = nodePeeker.findPlug( MString( "SSAbsorption2" ), &status );
		if( status == MS::kSuccess )
			nPlug.getValue( delightSSS.absorption.g );
		nPlug = nodePeeker.findPlug( MString( "SSAbsorption3" ), &status );
		if( status == MS::kSuccess )
			nPlug.getValue( delightSSS.absorption.b );

		nPlug = nodePeeker.findPlug( MString( "liqDelightSSRefraction" ), &status );
		if( status == MS::kSuccess )
			nPlug.getValue( delightSSS.refraction );

		nPlug = nodePeeker.findPlug( MString( "liqDelightSSScale" ), &status );
		if( status == MS::kSuccess )
			nPlug.getValue( delightSSS.scale );

      // RIB group ---------------------------------------------------------

      if( rib.box == "" ) {
        status.clear();
        MString ribBoxValue;
        nPlug = nodePeeker.findPlug( MString( "liqRIBBox" ), &status );
        if( status == MS::kSuccess ) {
          nPlug.getValue( ribBoxValue );
          if( ribBoxValue.substring(0,2) == "*H*" ) {
            MString parseThis = ribBoxValue.substring(3, ribBoxValue.length() - 1 );
            liqglo_preRibBox.append( parseString( parseThis ) );
          } else if( ribBoxValue.substring(0,3) == "*SH*" ) {
            MString parseThis = ribBoxValue.substring(3, ribBoxValue.length() - 1 );
            liqglo_preRibBoxShadow.append( parseString( parseThis ) );
          }
        }
        rib.box = (ribBoxValue == "")? "-" : parseString(ribBoxValue);
      }

      if( rib.readArchive == "" ) {
        status.clear();
        MString archiveValue;
        nPlug = nodePeeker.findPlug( MString( "liqRIBReadArchive" ), &status );
        if( status == MS::kSuccess ) {
          nPlug.getValue( archiveValue );
          if( archiveValue.substring(0,2) == "*H*" ) {
            MString parseThis = archiveValue.substring(3, archiveValue.length() - 1 );
            liqglo_preReadArchive.append( parseString( parseThis ) );
          } else if( archiveValue.substring(0,3) == "*SH*" ) {
            MString parseThis = archiveValue.substring(3, archiveValue.length() - 1 );
            liqglo_preReadArchiveShadow.append( parseString( parseThis ) );
          }
        }
        rib.readArchive = (archiveValue == "")? "-" : parseString(archiveValue);
      }

      if( rib.delayedReadArchive == "" ) {
        status.clear();
        MString delayedArchiveString, delayedArchiveValue;
        nPlug = nodePeeker.findPlug( MString( "liqRIBDelayedReadArchive" ), &status );
        if( status == MS::kSuccess ) {
          nPlug.getValue( delayedArchiveValue );
          delayedArchiveString = parseString( delayedArchiveValue );

          MStatus Dstatus;
          MPlug delayedPlug = fnNode.findPlug( MString( "ribDelayedArchiveBBox" ), &Dstatus );
          if( ( Dstatus == MS::kSuccess ) && ( delayedPlug.isConnected() ) ) {
            MPlugArray delayedNodeArray;
            delayedPlug.connectedTo( delayedNodeArray, true, true );
            MObject delayedNodeObj;
            delayedNodeObj = delayedNodeArray[0].node();
            MFnDagNode delayedfnNode( delayedNodeObj );

            MBoundingBox bounding = delayedfnNode.boundingBox();
            MPoint bMin = bounding.min();
            MPoint bMax = bounding.max();
            bound[0] = bMin.x;
            bound[1] = bMin.y;
            bound[2] = bMin.z;
            bound[3] = bMax.x;
            bound[4] = bMax.y;
            bound[5] = bMax.z;
          } else {
            // here, we are going to calculate the bounding box of the gprim
            //

            // get the dagPath
            MDagPath fullPath( nodePeeker.dagPath() );
            fullPath.extendToShape();
            /* cout <<"  + "<<fullPath.fullPathName()<<endl; */

            // get the full transform
            MMatrix currentMatrix( fullPath.inclusiveMatrixInverse() );
            /* cout <<"  + got matrix "<<currentMatrix<<endl; */

            // get the bounding box
            MFnDagNode shapeNode( fullPath );
            MBoundingBox bounding = shapeNode.boundingBox();
            /* cout <<"  + got bbox "<<endl; */

            // retrieve the bounding box expansion attribute
            MPlug expandBBoxPlug = nodePeeker.findPlug( MString( "liqRIBDelayedReadArchiveBBoxScale" ), &Dstatus );
            if( Dstatus == MS::kSuccess ) {
              /* cout <<"  + found scale attr"<<endl; */
              double expansion;
              expandBBoxPlug.getValue( expansion );
              if( expansion != 1.0 ) {
                /* cout <<"  + expansion = "<<expansion<<endl; */
                MTransformationMatrix bboxScale;
                double exp[3] = {expansion, expansion, expansion};
                bboxScale.setScale( exp, MSpace::kTransform );
                bounding.transformUsing( bboxScale.asMatrix() );
              }
            }

            // transform it to account for flattened transforms
            //bounding.transformUsing( currentMatrix );
            MPoint bMin = bounding.min() ;
            MPoint bMax = bounding.max() ;
            bound[0] = bMin.x;
            bound[1] = bMin.y;
            bound[2] = bMin.z;
            bound[3] = bMax.x;
            bound[4] = bMax.y;
            bound[5] = bMax.z;
          }
        }
        rib.delayedReadArchive = ( delayedArchiveString == "" )? "-" : delayedArchiveString;
      }

      if( ignoreShapes == false ) {
        status.clear();
        nPlug = nodePeeker.findPlug( MString( "liqIgnoreShapes" ), &status );
        if( status == MS::kSuccess )
          nPlug.getValue( ignoreShapes );
      }

      MFnDependencyNode nodeFn( nodePeeker );

      // find the attributes
      MStringArray floatAttributesFound  = findAttributesByPrefix( "rmanF", nodeFn );
      MStringArray pointAttributesFound  = findAttributesByPrefix( "rmanP", nodeFn );
      MStringArray vectorAttributesFound = findAttributesByPrefix( "rmanV", nodeFn );
      MStringArray normalAttributesFound = findAttributesByPrefix( "rmanN", nodeFn );
      MStringArray colorAttributesFound  = findAttributesByPrefix( "rmanC", nodeFn );
      MStringArray stringAttributesFound = findAttributesByPrefix( "rmanS", nodeFn );

      if( floatAttributesFound.length() > 0 ) {
        for ( unsigned i( 0 ); i < floatAttributesFound.length(); i++ ) {
          liqTokenPointer tokenPointerPair;
          MString cutString( floatAttributesFound[i].substring( 5, floatAttributesFound[i].length() ) );
          MPlug fPlug( nodeFn.findPlug( floatAttributesFound[i] ) );
          MObject plugObj;
          status = fPlug.getValue( plugObj );
          if( plugObj.apiType() == MFn::kDoubleArrayData ) {
            MFnDoubleArrayData fnDoubleArrayData( plugObj );
            const MDoubleArray& doubleArrayData( fnDoubleArrayData.array( &status ) );
            tokenPointerPair.set( cutString.asChar(), rFloat, doubleArrayData.length() );
            for( unsigned kk( 0 ); kk < doubleArrayData.length(); kk++ ) {
              tokenPointerPair.setTokenFloat( kk, doubleArrayData[kk] );
            }
          } else {

            if( fPlug.isArray() ) {

              unsigned nbElts( fPlug.evaluateNumElements() );
              float floatValue;
              tokenPointerPair.set( cutString.asChar(),
                                    rFloat,
                                    false,
                                    true, // philippe :passed as uArray, otherwise it will think it is a single float
                                    nbElts );
              MPlug elementPlug;
              for( unsigned kk( 0 ); kk < nbElts; kk++ ) {
                elementPlug = fPlug.elementByPhysicalIndex(kk);
                elementPlug.getValue( floatValue );
                tokenPointerPair.setTokenFloat( kk, floatValue );
              }
            } else {

              float floatValue;
              tokenPointerPair.set( cutString.asChar(), rFloat );
              fPlug.getValue( floatValue );
              tokenPointerPair.setTokenFloat( 0, floatValue );
            }
          }
          if( tokenPointerMap.end() == tokenPointerMap.find( tokenPointerPair.getDetailedTokenName() ) ) {
            tokenPointerMap[ tokenPointerPair.getDetailedTokenName() ] = tokenPointerPair;
          }
        }
      }

      if( pointAttributesFound.length() > 0 ) {
        for ( unsigned i( 0 ); i < pointAttributesFound.length(); i++ ) {
          liqTokenPointer tokenPointerPair;
          MString cutString( pointAttributesFound[i].substring( 5, pointAttributesFound[i].length() ) );
          MPlug pPlug( nodeFn.findPlug( pointAttributesFound[i] ) );
          MObject plugObj;
          status = pPlug.getValue( plugObj );
          if( plugObj.apiType() == MFn::kPointArrayData ) {
            MFnPointArrayData  fnPointArrayData( plugObj );
            MPointArray pointArrayData = fnPointArrayData.array( &status );
            tokenPointerPair.set( cutString.asChar(), rPoint, pointArrayData.length() );
            for ( unsigned kk( 0 ); kk < pointArrayData.length(); kk++ ) {
              tokenPointerPair.setTokenFloat( kk, pointArrayData[kk].x, pointArrayData[kk].y, pointArrayData[kk].z );
            }
          } else {
            // Hmmmm float ? double ?
            float x, y, z;
            tokenPointerPair.set( cutString.asChar(), rPoint );
            // Hmmm should check as for arrays if we are in nurbs mode : 4 values
            pPlug.child(0).getValue( x );
            pPlug.child(1).getValue( y );
            pPlug.child(2).getValue( z );
            tokenPointerPair.setTokenFloat( 0, x, y, z );
          }
          if( tokenPointerMap.end() == tokenPointerMap.find( tokenPointerPair.getDetailedTokenName() ) ) {
            tokenPointerMap[ tokenPointerPair.getDetailedTokenName() ] = tokenPointerPair;
          }
        }
      }
      parseVectorAttributes( nodeFn, vectorAttributesFound, rVector );
      parseVectorAttributes( nodeFn, normalAttributesFound, rNormal );
      parseVectorAttributes( nodeFn, colorAttributesFound,  rColor  );

      if( stringAttributesFound.length() > 0 ) {
        for ( unsigned i( 0 ); i < stringAttributesFound.length(); i++ ) {
          liqTokenPointer tokenPointerPair;
          MString cutString( stringAttributesFound[i].substring( 5, stringAttributesFound[i].length() ) );
          MPlug sPlug( nodeFn.findPlug( stringAttributesFound[i] ) );
          MObject plugObj;
          status = sPlug.getValue( plugObj );
          tokenPointerPair.set( cutString.asChar(), rString );
          MString stringVal;
          sPlug.getValue( stringVal );
          tokenPointerPair.setTokenString( 0, stringVal.asChar() );
          if( tokenPointerMap.end() == tokenPointerMap.find( tokenPointerPair.getDetailedTokenName() ) ) {
            tokenPointerMap[ tokenPointerPair.getDetailedTokenName() ] = tokenPointerPair;
          }
        }
      }
    } // if( dagSearcher.apiType( &status ) == MFn::kTransform )
  } while( dagSearcher.length() > 0 );

	// Raytracing Sets membership handling
	if( grouping.membership.empty() )
	{
		MObjectArray setArray;
		MGlobal::getAssociatedSets( hierarchy, setArray );
		for( unsigned i( 0 ); i < setArray.length(); i++ )
		{
			MFnDependencyNode depNodeFn( setArray[ i ] );
			status.clear();
			MPlug plug = depNodeFn.findPlug( "liqTraceSet", &status );
			if( status == MS::kSuccess )
			{
				bool value = false;
				plug.getValue( value );

				if( value )
				{
					status.clear();
					grouping.membership += "," + string( depNodeFn.name( &status ).asChar() );
				}
			}
		}
		if( !grouping.membership.empty() )
			grouping.membership = grouping.membership.substr( 1 );
        status.clear();
	}

  // Get the object's color
  if( objType != MRT_Shader ) {
    MObject shadingGroup = findShadingGroup( path, objType );
    if( shadingGroup != MObject::kNullObj ) {
      assignedShadingGroup.setObject( shadingGroup );
      MObject surfaceShader = findShader( shadingGroup );
      assignedShader.setObject( surfaceShader );
      assignedDisp.setObject( findDisp( shadingGroup ) );
      assignedVolume.setObject( findVolume( shadingGroup ) );
      if( ( surfaceShader == MObject::kNullObj ) || !getColor( surfaceShader, color ) ) {
        // This is how we specify that the color was not found.
        color.r = -1.0;
      }
      if( ( surfaceShader == MObject::kNullObj ) || !getOpacity( surfaceShader, opacity ) ) {
        // This is how we specify that the opacity was not found.
        //
        opacity.r = -1.0;
      }
      mayaMatteMode = getMatteMode( surfaceShader );
    } else {
      color.r = -1.0;
      opacity.r = -1.0;
    }
    doubleSided = isObjectTwoSided( path );
    reversedNormals = isObjectReversed( path );
  }

  // Check to see if the object should have its color overridden
  // (if possible).
  //
  nPlug = fnNode.findPlug( MString( "useParticleColorWhenInstanced" ), &status );
  if( status == MS::kSuccess ) {
    bool override;
    nPlug.getValue( override );
    if( override && particleId != -1 ) {
      // Traverse upwards, looking for some connection between this
      // geometry hierarchy and a particle instancer node.
      //
      MFnDagNode dagNode( path.node() );
      bool foundInstancerNode = false;
      while( true ) {
        // The instancer is always connected to the "matrix" attribute.
        //
        MPlug matrixPlug = dagNode.findPlug( MString( "matrix" ), &status );
        if( status != MS::kSuccess )
        {
          break;
        }

        // If the matrix plug is connected, iterate over the connections
        // to see if one of them is an instancer.
        //
        if( matrixPlug.isConnected() ) {
          MPlugArray connections;
          matrixPlug.connectedTo( connections, false, true );
          for( unsigned i( 0 ); i < connections.length(); i++ )
          {
            MObject obj = connections[i].node();
            if( obj.hasFn( MFn::kInstancer ) )
            {
              dagNode.setObject( obj );
              foundInstancerNode = true;
              break;
            }

          }
        }

        // If we've found an instancer or we're at the top of the
        // hierarchy, break.
        //
        if( foundInstancerNode || dagNode.parentCount() == 0 )
        {
          break;
        }
        dagNode.setObject( dagNode.parent( 0 ) );
      }

      // If we've got an instancer, find the associated particle system.
      //
      if( foundInstancerNode )
      {
        // Find out what particles we're replacing.
        //
        MPlug inputPointsPlug = dagNode.findPlug( "inputPoints", &status );
        if( status == MS::kSuccess )
        {
          // Find the array of connected plugs.
          //
          MPlugArray sourcePlugArray;
          inputPointsPlug.connectedTo( sourcePlugArray, true, false, &status );

          // There SHOULD always be a connected plug,
          // but this is a safety check.
          //
          if( sourcePlugArray.length() > 0 )
          {
            MObject sourceObject = sourcePlugArray[0].node();

            // Another sanity check: make sure the source is
            // actually a particle system.
            //
            if( sourceObject.hasFn( MFn::kParticle ) )
            {
              MFnParticleSystem particles( sourceObject );

              // Proceed with color overrides if the rgbPP exists.
              //
              if( particles.hasRgb() )
              {
                MVectorArray rgbPP;
                particles.rgb( rgbPP );

                // Find the ID's
                //
                MPlug idPlug = particles.findPlug( "id", &status );
                if( status == MS::kSuccess )
                {
                  MObject idObject;
                  idPlug.getValue( idObject );
                  MFnDoubleArrayData idArray( idObject, &status );

                  // Look for an id that matches.
                  //
                  for ( int i = 0; i < idArray.length(); i++ )
                  {
                    // If a match is found, grab the color.
                    //
                    if( static_cast<int>(idArray[i]) == particleId )
                    {
                      color.r = rgbPP[i].x;
                      color.g = rgbPP[i].y;
                      color.b = rgbPP[i].z;
                      overrideColor = true;
                      break;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // Create a new RIB object for the given path

  //
  LIQDEBUGPRINTF( "-> creating rib object for given path\n");

  MObject obj( path.node() );
  liqRibObjPtr no( new liqRibObj( path, objType ) );
  LIQDEBUGPRINTF( "-> creating rib object for reference\n");
  no->ref();

  LIQDEBUGPRINTF( "-> getting objects name\n");
  name = path.fullPathName();

  if( name == "" ) {
    LIQDEBUGPRINTF( "-> name unknown -- searching for name\n");
    MDagPath parentPath = path;
    parentPath.pop();

    name = parentPath.fullPathName( &status );
    LIQDEBUGPRINTF( "-> found name\n");
  }

  if( objType == MRT_RibGen ) {
    name += "RIBGEN";
  }

  LIQDEBUGPRINTF( "-> inserting object into ribnode's obj sample table\n" );
  if( !objects[ sample ] ) {
    objects[ sample ] = no;
  } else {
	objects[ sample ]->unref();
    objects[ sample ] = no;
  }

  LIQDEBUGPRINTF( "-> done creating rib object for given path\n");
}


/**
 * Return the path in the DAG to the instance that this node represents.
 */
MDagPath & liqRibNode::path()
{
  return DagPath;
}


/**
 * Find the shading group assigned to the given object.
 */
MObject liqRibNode::findShadingGroup( const MDagPath& path, ObjectType type )
{
	LIQDEBUGPRINTF( "-> finding rib node shading group\n");

	// Alf: the case of a custom shading group assigned to the shape directly
	MStatus status;
	MFnDagNode fnDagNode( path.transform() );
	MPlug rmanSGPlug( fnDagNode.findPlug( MString( "liquidCustomShadingGroup" ), &status ) );
	if( status==MS::kSuccess && rmanSGPlug.isConnected() )
	{
		MPlugArray rmSGArray;
		rmanSGPlug.connectedTo( rmSGArray, true, true );
		MObject rmSGObj( rmSGArray[0].node() );
		MFnDependencyNode SGDepNode( rmSGObj );
		if( SGDepNode.typeName() == "shadingEngine" )
			return rmSGObj;
	}

	// Alf: Paint effects shading group
	fnDagNode.setObject( path.node() );
	if( path.hasFn( MFn::kPfxGeometry ) )
	{
		if( type == MRT_PfxTube )
			rmanSGPlug = fnDagNode.findPlug( MString( "liquidTubeShadingGroup" ), &status );
		if( type == MRT_PfxLeaf )
			rmanSGPlug = fnDagNode.findPlug( MString( "liquidLeafShadingGroup" ), &status );
		if( type == MRT_PfxPetal )
			rmanSGPlug = fnDagNode.findPlug( MString( "liquidPetalShadingGroup" ), &status );
		if( status == MS::kSuccess && rmanSGPlug.isConnected() )
		{
			MPlugArray rmSGArray;
			rmanSGPlug.connectedTo( rmSGArray, true, true );
			MObject rmSGObj( rmSGArray[0].node() );
			MFnDependencyNode SGDepNode( rmSGObj );
			if( SGDepNode.typeName() == "shadingEngine" )
				return rmSGObj;
		}
	}
	MSelectionList objects;
	objects.add( path );
	MObjectArray setArray;

	// Get all of the sets that this object belongs to
	//
	MGlobal::getAssociatedSets( objects, setArray );
	MObject mobj;

	// Look for a set that is a "shading group"
	//
	for ( unsigned i( 0 ); i<setArray.length(); i++ )
	{
		mobj = setArray[i];
		MFnSet fnSet( mobj );
		MStatus stat;
		if( MFnSet::kRenderableOnly == fnSet.restriction(&stat) )
			return mobj;
	}
	return MObject::kNullObj;
}


/**
 * Find the shading node for the given shading group.
 */
MObject liqRibNode::findShader( MObject& group )
{
  LIQDEBUGPRINTF( "-> finding shader for rib node shading group\n");
  MFnDependencyNode fnNode( group );
  MPlug shaderPlug = fnNode.findPlug( "surfaceShader" );

  if( !shaderPlug.isNull() ) {
    MPlugArray connectedPlugs;
    bool asSrc = false;
    bool asDst = true;
    shaderPlug.connectedTo( connectedPlugs, asDst, asSrc );
    if( connectedPlugs.length() != 1 ) {
      //cerr << "Error getting shader\n";
    }
    else
      return connectedPlugs[0].node();
  }

  return MObject::kNullObj;
}


/**
 * Find the displacement node for the given shading group
 */
MObject liqRibNode::findDisp( MObject& group )
{
  LIQDEBUGPRINTF( "-> finding shader for rib node shading group\n");
  MFnDependencyNode fnNode( group );
  MPlug shaderPlug = fnNode.findPlug( "displacementShader" );

  if( !shaderPlug.isNull() ) {
    MPlugArray connectedPlugs;
    bool asSrc = false;
    bool asDst = true;
    shaderPlug.connectedTo( connectedPlugs, asDst, asSrc );
    if( connectedPlugs.length() != 1 ) {
      //cerr << "Error getting shader\n";
    }
    else
      return connectedPlugs[0].node();
  }

  return MObject::kNullObj;
}


/**
 * Find the volume shading node for the given shading group.
 */
MObject liqRibNode::findVolume( MObject& group )
{
  LIQDEBUGPRINTF( "-> finding shader for rib node shading group\n");
  MFnDependencyNode fnNode( group );
  MPlug shaderPlug = fnNode.findPlug( "volumeShader" );

  if( !shaderPlug.isNull() ) {
    MPlugArray connectedPlugs;
    bool asSrc = false;
    bool asDst = true;
    shaderPlug.connectedTo( connectedPlugs, asDst, asSrc );
    if( connectedPlugs.length() != 1 ) {
      //cerr << "Error getting shader\n";
    }
    else
      return connectedPlugs[0].node();
  }

  return MObject::kNullObj;
}

// Get the list of all included/excluded lights for this node
// using liquid light sets
void liqRibNode::getSetLights( MObjectArray& linkLights )
{
	MStatus status;

	// get associated sets
	MSelectionList hierarchy;
	MDagPath dagSearcher( DagPath );
	do
	{ 
		dagSearcher.pop();
		hierarchy.add( dagSearcher, MObject::kNullObj, true );
	}
	while( dagSearcher.length() > 0 );
	MObjectArray oaSets;
	MGlobal::getAssociatedSets( hierarchy, oaSets );

	// get linked lights
	MObjectArray oaLights;
	for( unsigned i( 0 ); i < oaSets.length(); i++ )
	{
		MFnDependencyNode depNodeFn( oaSets[ i ] );
		MPlug lightLinkPlug = depNodeFn.findPlug( "liqLinkedLights", &status );
		if( status == MS::kSuccess )
		{
			int numElements = lightLinkPlug.evaluateNumElements();
			for( int k( 0 ); k < numElements; k++ )
			{
				MPlug elementPlug  = lightLinkPlug.elementByPhysicalIndex( k );
				MPlugArray connectedPlugs;
				if( !elementPlug.isConnected() )
					continue;
				elementPlug.connectedTo( connectedPlugs, true, false, &status );
				oaLights.append( connectedPlugs[0].node() );
			}
		}
	}
	// remove duplicate lights
	MSelectionList selList;
	for( unsigned i( 0 ); i < oaLights.length(); i++ )
	{
		selList.add( oaLights[i], 1 );
	}

	for( unsigned i( 0 ); i < selList.length(); i++ )
	{
		MObject node;
		selList.getDependNode( i, node );
		linkLights.append( node );
	}
}

// Get the list of all included/excluded lights for this node
void liqRibNode::getLinkLights( MObjectArray& linkLights, bool exclusive )
{
	MStatus status;
	LIQDEBUGPRINTF( "-> getting linked lights\n");
	MFnDependencyNode fnNode( DagPath.node() );
	MPlug msgPlug = fnNode.findPlug( "message", &status );

	if( status != MS::kSuccess )
		return;
	
	MPlugArray llPlugs;
	msgPlug.connectedTo(llPlugs, true, true);

	for ( unsigned i=0; i<llPlugs.length(); i++ )
	{
		MPlug llPlug = llPlugs[i];
		MObject llPlugAttr = llPlug.attribute();
		MFnAttribute llPlugAttrFn(llPlugAttr);

		if( llPlugAttrFn.name() == MString( "objectIgnored" ) && exclusive )
		{
			MPlug llParentPlug = llPlug.parent(&status);
			int numChildren  = llParentPlug.numChildren();

			for (int k=0; k<numChildren; k++)
			{
				MPlug   childPlug  = llParentPlug.child(k);
				MObject llChildAttr = childPlug.attribute();
				MFnAttribute llChildAttrFn(llChildAttr);

				if(llChildAttrFn.name() == MString( "lightIgnored" ))
				{
					MPlugArray connectedPlugs;
					childPlug.connectedTo(connectedPlugs,true,true);
					MFnDependencyNode conP( connectedPlugs[0].node() );
					if( connectedPlugs[0].node().hasFn( MFn::kSet ) )
					{
						MStatus setStatus;
						MFnDependencyNode listSetNode( connectedPlugs[0].node() );
						MPlug setPlug = fnNode.findPlug( "dagSetMembers", &setStatus );
						if( setStatus == MS::kSuccess )
						{
							MPlugArray setConnectedPlugs;
							setPlug.connectedTo(setConnectedPlugs,true,true);
							linkLights.append( setConnectedPlugs[0].node() );
						}
					}
					else
						linkLights.append( connectedPlugs[0].node() );
				}
			}
		}
		if( llPlugAttrFn.name() == MString( "object" ) && !exclusive )
		{
			MPlug llParentPlug = llPlug.parent(&status);
			int numChildren  = llParentPlug.numChildren();

			for (int k=0; k<numChildren; k++)
			{
				MPlug   childPlug  = llParentPlug.child(k);
				MObject llChildAttr = childPlug.attribute();
				MFnAttribute llChildAttrFn(llChildAttr);

				if(llChildAttrFn.name() == MString( "light" ))
				{
					MPlugArray connectedPlugs;
					childPlug.connectedTo(connectedPlugs,true,true);
					MFnDependencyNode conP( connectedPlugs[0].node() );
	//				MGlobal::displayInfo( MString( "connectedPlugs: " ) + conP.name() );
					if( connectedPlugs[0].node().hasFn( MFn::kSet ) )
					{
						MStatus setStatus;
						MFnDependencyNode listSetNode( connectedPlugs[0].node() );
						MPlug setPlug = fnNode.findPlug( "dagSetMembers", &setStatus );
						if( setStatus == MS::kSuccess )
						{
							MPlugArray setConnectedPlugs;
							setPlug.connectedTo(setConnectedPlugs,true,true);
							linkLights.append( setConnectedPlugs[0].node() );
						}
					}
					else
						linkLights.append( connectedPlugs[0].node() );
				}
			}
		}
	}
	// so that liquid works with illuminate by default
}


/**
 * Get the color & opacity of the given shading node.
 */
bool liqRibNode::getColor( MObject& shader, MColor& color )
{
  LIQDEBUGPRINTF( "-> getting a shader color\n");
  MStatus stat = MS::kSuccess;
  switch ( shader.apiType() )
  {
  case MFn::kLambert :
  {
    MFnLambertShader fnShader( shader );
    color = fnShader.color();
    break;
  }
  case MFn::kBlinn :
  {
    MFnBlinnShader fnShader( shader );
    color = fnShader.color();
    break;
  }
  case MFn::kPhong :
  {
    MFnPhongShader fnShader( shader );
    color = fnShader.color();
    break;
  }
  default:
  {
    MFnDependencyNode fnNode( shader );
    MPlug colorPlug = fnNode.findPlug( "outColor" );
    MPlug tmpPlug;
  tmpPlug = colorPlug.child(0,&stat);
  if(stat == MS::kSuccess) tmpPlug.getValue( color.r );
  tmpPlug = colorPlug.child(1,&stat);
  if(stat == MS::kSuccess) tmpPlug.getValue( color.g );
  tmpPlug = colorPlug.child(2,&stat);
  if(stat == MS::kSuccess) tmpPlug.getValue( color.b );
    return false;
  }
  }
  return true;
}


bool liqRibNode::getOpacity( MObject& shader, MColor& opacity )
{
  LIQDEBUGPRINTF( "-> getting a shader color\n");
  MStatus stat = MS::kSuccess;
  switch ( shader.apiType() )
  {
  case MFn::kLambert :
  {
    MFnLambertShader fnShader( shader );
    opacity = fnShader.transparency();
    opacity.r = 1. - opacity.r;
    opacity.g = 1. - opacity.g;
    opacity.b = 1. - opacity.b;
    break;
  }
  case MFn::kBlinn :
  {
    MFnBlinnShader fnShader( shader );
    color = fnShader.transparency();
    opacity.r = 1. - opacity.r;
    opacity.g = 1. - opacity.g;
    opacity.b = 1. - opacity.b;
    break;
  }
  case MFn::kPhong :
  {
    MFnPhongShader fnShader( shader );
    opacity = fnShader.transparency();
    opacity.r = 1. - opacity.r;
    opacity.g = 1. - opacity.g;
    opacity.b = 1. - opacity.b;
    break;
  }
  default:
  {
    MFnDependencyNode fnNode( shader );
    MPlug colorPlug = fnNode.findPlug( "outTransparency" );
    MPlug tmpPlug;
    tmpPlug = colorPlug.child(0,&stat);
    if(stat == MS::kSuccess) tmpPlug.getValue( opacity.r );
    tmpPlug = colorPlug.child(1,&stat);
    if(stat == MS::kSuccess) tmpPlug.getValue( opacity.g );
    tmpPlug = colorPlug.child(2,&stat);
    if(stat == MS::kSuccess) tmpPlug.getValue( opacity.b );
    opacity.r = 1. - opacity.r;
    opacity.g = 1. - opacity.g;
    opacity.b = 1. - opacity.b;
    return false;
  }
  }
  return true;
}


/**
 * Check to see if this is a matte object.
 * if a regular maya shader with a matteOpacityMode attribute is attached,
 * and the value of the attribute is 0 ( Black Hole ) then we return true.
 */
bool liqRibNode::getMatteMode( MObject& shader )
{
  MObject matteModeObj;
  short matteModeInt;
  MStatus myStatus;
  LIQDEBUGPRINTF( "-> getting matte mode\n");
  if( !shader.isNull() ) {
    MFnDependencyNode fnNode( shader );
    MPlug mattePlug = fnNode.findPlug( "matteOpacityMode", &myStatus );
    if( myStatus == MS::kSuccess ) {
      mattePlug.getValue( matteModeInt );
      LIQDEBUGPRINTF(  "-> matte mode: %d \n", matteModeInt );
      if( matteModeInt == 0 ) {
        return true;
      }
    }
  }
  return false;
}


/**
 * Checks if this node has at least n objects.
 */
bool liqRibNode::hasNObjects( unsigned n )
{
  for( int i = 0; i < n; i++ ) {
    if( !objects[ i ] ) {
      return false;
    }
  }
  return true;
}

void liqRibNode::parseVectorAttributes( const MFnDependencyNode& nodeFn, const MStringArray& strArray, const ParameterType& pType ) {
  MStatus status;
  if( strArray.length() > 0 ) {
    for ( unsigned i( 0 ); i < strArray.length(); i++ ) {
      liqTokenPointer tokenPointerPair;
      MString cutString( strArray[i].substring( 5, strArray[i].length() ) );
      MPlug vPlug( nodeFn.findPlug( strArray[i] ) );
      MObject plugObj;
      status = vPlug.getValue( plugObj );
      if( plugObj.apiType() == MFn::kVectorArrayData ) {
        MFnVectorArrayData  fnVectorArrayData( plugObj );
        MVectorArray vectorArrayData = fnVectorArrayData.array( &status );
        tokenPointerPair.set( cutString.asChar(), pType, vectorArrayData.length() );
        for ( unsigned kk( 0 ); kk < vectorArrayData.length(); kk++ ) {
          tokenPointerPair.setTokenFloat( kk, vectorArrayData[kk].x, vectorArrayData[kk].y, vectorArrayData[kk].z );
        }

        // store it all
        if( tokenPointerMap.end() == tokenPointerMap.find( tokenPointerPair.getDetailedTokenName() ) ) {
          tokenPointerMap[ tokenPointerPair.getDetailedTokenName() ] = tokenPointerPair;
        }

      } else {
        // Hmmmm float ? double ?
        float x, y, z;
        tokenPointerPair.set( cutString.asChar(), pType );

        vPlug.child(0).getValue( x );
        vPlug.child(1).getValue( y );
        vPlug.child(2).getValue( z );

/*		// temporay solution to convert the rim tint to vector value
		if( vPlug.partialName() == "rmanCglobalRimTint" )
		{
			vPlug.getValue( x );
			y = x;
			z = x;
			MGlobal::displayInfo( MString ( "rmanCglobalRimTint converted to vector value: " ) + vPlug.partialName() );
		}
*/
		//if( plugObj.apiType() == MFn::kDoubleArrayData )

        tokenPointerPair.setTokenFloat( 0, x, y, z );
        if( tokenPointerMap.end() == tokenPointerMap.find( tokenPointerPair.getDetailedTokenName() ) ) {
          tokenPointerMap[ tokenPointerPair.getDetailedTokenName() ] = tokenPointerPair;
        }
      }
    }
  }
}


void liqRibNode::writeUserAttributes() {
  unsigned numTokens( tokenPointerMap.size() );
  if( numTokens ) {
    scoped_array< RtToken > tokenArray( new RtToken[ numTokens ] );
    scoped_array< RtPointer > pointerArray( new RtPointer[ numTokens ] );
    // Can't use assignTokenArraysV() since we're dealing with std::map
    unsigned i( 0 );
    for ( map< string, liqTokenPointer >::const_iterator iter( tokenPointerMap.begin() ); iter != tokenPointerMap.end(); iter++, i++ ) {
      tokenArray[ i ] = const_cast< RtString >( const_cast< liqTokenPointer* >( &( iter->second ) )->getDetailedTokenName().c_str() );
      pointerArray[ i ] = const_cast< liqTokenPointer* >( &( iter->second ) )->getRtPointer();
    }

    RiAttributeV( "user", numTokens, tokenArray.get(), pointerArray.get() );
  }
}
