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
** Liquid Rib Hash Table Source
** ______________________________________________________________________
*/


#ifndef _WIN32
// Dynamic Object Headers
#include <dlfcn.h>
#endif

#ifdef _WIN32
#pragma warning(disable:4786)
#endif

// Standard Headers
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <vector>

// Renderman Headers
extern "C" {
#include <ri.h>
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
#include <maya/MFnDagNode.h>
#include <maya/MColor.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>

#include <liquid.h>
#include <liqRibNode.h>
#include <liqRibHT.h>
#include <liqMemory.h>

extern int debugMode;


liqRibHT::liqRibHT()
//
//  Description:
//      Class constructor.
//
{
  LIQDEBUGPRINTF( "-> creating hash table\n" );
  RibNodeMap.clear();
}

liqRibHT::~liqRibHT()
//
//  Description:
//      Class destructor.
//
{
  LIQDEBUGPRINTF( "-> killing hash table\n" );
  RNMAP::iterator iter;
  for ( iter = RibNodeMap.begin(); iter != RibNodeMap.end(); iter++ )
  {
    liqRibNode * node;
    node = (*iter).second;
    delete node;
  }
  RibNodeMap.clear();
  if ( debugMode ) {
    printf("-> finished killing hash table\n");
  }
}

ulong liqRibHT::hash(const char *str)
//
//  Description:
//      hash function for strings
//
{
  LIQDEBUGPRINTF( "-> hashing\n" );
  ulong hc = 0;

  while(*str) {
    //hc = hc * 13 + *str * 27;   // old hash function
    hc = hc + *str;   // change this to a better hash func
    str++;
  }

  return (ulong)hc;
}

int liqRibHT::insert( MDagPath &path, double /*lframe*/, int sample,
                      ObjectType objType,
                      MMatrix *matrix,
                      const MString instanceStr,
                      int particleId )
//  Description:
//      insert a new node into the hash table.
{
  LIQDEBUGPRINTF( "-> inserting node into hash table\n" );
  MFnDagNode  fnDagNode( path );
  MStatus    returnStatus;

  MString nodeName = fnDagNode.fullPathName(&returnStatus);
  if ( objType == MRT_RibGen ) nodeName += "RIBGEN";

  const char * name = nodeName.asChar();

  ulong hc = hash( name );
  LIQDEBUGPRINTF( "-> hashed node name: " );
  LIQDEBUGPRINTF( name );
  LIQDEBUGPRINTF( " size: %ld\n", hc );

  liqRibNode * node;
  /*node = find( path.node(), objType );*/
  node = find( nodeName, path, objType );
  liqRibNode *     newNode = NULL;
  liqRibNode *    instance = NULL;

  // If "node" is non-null then there's already a hash table entry at
  // this point
  //
  liqRibNode * tail = NULL;
  if ( NULL != node ) {
    while(node) {
      tail = node;

      // Break if we've already dealt with this DAG path
      // (and instance string - by default this is "", which
      //  will match for most objects - allowing us to deal
      //  with particle-instancing).
      //
      if ( path        == node->path() &&
           instanceStr == node->getInstanceStr() )
      {
        break;
      }

      if ( path.node() == node->path().node() ) {
        // We have found another instance of the object we are object
        // looking for
        instance = node;
      }
      node = node->next;
    }
    if ( ( NULL == node ) && ( NULL != instance ) ) {
      // We have not found a node with a matching path, but we have found
      // one with a matching object, so we need to insert a new instance
      newNode = new liqRibNode( instance, instanceStr );
    }
  }
  if ( NULL == newNode ) {
    // We have to make a new node
    if (node == NULL) {
      node = new liqRibNode(NULL, instanceStr);
      if ( NULL != tail ) {
        assert( NULL == tail->next );
        tail->next = node;
        RibNodeMap.insert( RNMAP::value_type( hc, node ) );
      } else {
        RibNodeMap.insert( RNMAP::value_type( hc, node ) );
      }
    }
  } else {
    assert(NULL==node);
    // Append new instance node onto tail of linked list
    node = newNode;
    if ( NULL != tail ) {
      assert( NULL == tail->next );
      tail->next = node;
      RibNodeMap.insert( RNMAP::value_type( hc, node ) );
    } else {
      RibNodeMap.insert( RNMAP::value_type( hc, node ) );
    }
  }

  node->set( path, sample, objType, particleId );

  // If we were given a specific matrix to use (this only
  // happens when we've got particle-instancing.
  //
  if ( matrix != NULL )
  {
    // Calculate the inclusive matrix for the instanced
    // object (as a product of the original object's
    // exclusive matrix - see Maya docs - and the given
    // matrix).
    //
    MMatrix inclusiveMatrix = path.exclusiveMatrix() * (*matrix);
    node->object( sample )->setMatrix( 0, inclusiveMatrix );
  }

  // We can NOT support deformation blur on particle instancing,
  // since there's no way to guarantee that the objects are
  // topologically identical over the sample range.
  //
  if ( instanceStr != "" )
  {
    node->motion.deformationBlur = false;
  }

  LIQDEBUGPRINTF( "-> finished inserting node into hash table\n" );
  return 0;
}

liqRibNode* liqRibHT::find( MString nodeName, MDagPath path, ObjectType
                            /*objType = MRT_Unknown*/ )
//  Description: find the hash table entry for the given object
{
  LIQDEBUGPRINTF( "-> finding node in hash table using object, %s\n", nodeName.asChar() );
  liqRibNode * result = NULL;

  const char * name = nodeName.asChar();

  ulong hc = hash( name );
  LIQDEBUGPRINTF( "-> Done\n"  );

  RNMAP::iterator iter = RibNodeMap.find( hc );
  while ( ( (*iter).first == hc ) && ( iter != RibNodeMap.end() ) ) {
    if ( (*iter).second->path() == path ) {
      result = (*iter).second;
      iter = RibNodeMap.end();
    } else {
      iter++;
    }
  }

  LIQDEBUGPRINTF( "-> finished finding node in hash table using object\n" );
  return result;
}
