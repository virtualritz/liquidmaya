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

//#include <liquid.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif
extern "C" {
#include <ri.h>
}
#include <liqTokenPointer.h>
#include <liqMemory.h>
#include <liquid.h>
#include <iostream.h>

extern int debugMode;



const char * liqTokenPointer::StringDetailType[] = {
  "uniform",
  "varying",
  "vertex",
  "constant",
  "facevarying"
};


liqTokenPointer::liqTokenPointer() 
{
  m_pType        = rFloat;
  m_tokenName[0] = '\0';
  m_tokenFloats  = NULL;
  m_tokenString  = NULL;
  m_isArray      = false;
  m_isUArray     = false;
  m_isNurbs      = false;
  m_isString     = false;
  m_isFull       = false;
  m_arraySize    = 0;
  m_uArraySize   = 0;
  m_eltSize      = 0;
  m_tokenSize    = 0;
  m_stringSize   = 0;
}

liqTokenPointer::liqTokenPointer( const liqTokenPointer &src)
{
  LIQDEBUGPRINTF("-> copy constructing additional ribdata: " );
  LIQDEBUGPRINTF(src.m_tokenName);
  LIQDEBUGPRINTF("\n" );

  m_tokenFloats = NULL;
  m_tokenString = NULL;
  m_isArray     = false;
  m_isUArray    = false;
  m_isNurbs     = false;
  m_isString    = false;
  m_isFull      = false;
  m_arraySize   = 0;
  m_uArraySize  = 0;
  m_eltSize     = 0;
  m_tokenSize   = 0;
  m_stringSize  = 0;
  set( src.m_tokenName, src.m_pType, src.m_isNurbs, src.m_isArray, src.m_isUArray, src.m_arraySize );
  m_dType = src.m_dType;
  if( m_pType != rString ) {
    setTokenFloats( src.m_tokenFloats );
  } else {
    if( src.m_tokenString ) {
      setTokenString( src.m_tokenString, strlen( src.m_tokenString) );
    }
  }

  LIQDEBUGPRINTF("-> done copy constructing additional ribdata: " );
  LIQDEBUGPRINTF(src.m_tokenName);
  LIQDEBUGPRINTF("\n" );
}    

liqTokenPointer & liqTokenPointer::operator=( const liqTokenPointer &src)
{
  LIQDEBUGPRINTF("-> copying additional ribdata: " );
  LIQDEBUGPRINTF(src.m_tokenName);
  LIQDEBUGPRINTF("\n" );

  reset();
  set( src.m_tokenName, src.m_pType, src.m_isNurbs, src.m_isArray, src.m_isUArray, src.m_arraySize );
  m_dType = src.m_dType;
  if( m_pType != rString ) {
    setTokenFloats( src.m_tokenFloats );
  } else {
    if( src.m_tokenString ) {
      setTokenString( src.m_tokenString, strlen( src.m_tokenString) );
    }
  }

  LIQDEBUGPRINTF("-> done copying additional ribdata: " );
  LIQDEBUGPRINTF(src.m_tokenName);
  LIQDEBUGPRINTF("\n" );
  return *this;
}

liqTokenPointer::~liqTokenPointer() 
{
  LIQDEBUGPRINTF("-> freeing additional ribdata: " );
  LIQDEBUGPRINTF(m_tokenName);
  LIQDEBUGPRINTF("\n" );

  if( m_tokenFloats ) { lfree( m_tokenFloats ); m_tokenFloats = NULL; }
  if( m_tokenString ) { lfree( m_tokenString ); m_tokenString = NULL; }
};

void liqTokenPointer::reset()
{
  if( m_tokenFloats ) { lfree( m_tokenFloats ); m_tokenFloats = NULL; }
  if( m_tokenString ) { lfree( m_tokenString ); m_tokenString = NULL; }
  m_isArray      = false;
  m_isUArray     = false;
  m_isNurbs      = false;
  m_isString     = false;
  m_isFull       = false;
  m_arraySize    = 0;
  m_uArraySize   = 0;
  m_eltSize      = 0;
  m_tokenSize    = 0;
  m_stringSize   = 0;
  m_pType        = rFloat;
  m_tokenName[0] = '\0';
}

int liqTokenPointer::set( const char * name, ParameterType ptype, bool asNurbs, bool asArray, bool asUArray, unsigned int arraySize )
{
  setTokenName( name );
  m_pType = ptype;
  m_isNurbs = asNurbs;
  if( m_pType != rString ) {
    if( m_tokenString ) {
      lfree( m_tokenString );
    }
    m_tokenString = NULL;
    switch( m_pType )
    {
    case rFloat : 
      m_eltSize = 1; 
      break;
    case rPoint : 
      if( m_isNurbs ) {
        m_eltSize = 4 ;
      } else {
        m_eltSize = 3;
      }
      break;
    case rColor :
    case rNormal :
    case rVector :
      m_eltSize = 3; 
      break;
    case rHpoint :
      m_eltSize = 0;
      break;
    case rString : // Useless but prevent warning at compile time
      m_eltSize = 0;
      break;
    }
    m_isArray = asArray;
    m_isUArray = asUArray;
    unsigned long neededSize;
    if( m_isArray || m_isUArray ) {
      m_arraySize = arraySize;
      neededSize = m_arraySize * m_eltSize * sizeof( RtFloat);
    } else {
      m_arraySize = 0;
      neededSize = m_eltSize * sizeof( RtFloat);
    }
    if( m_tokenFloats ) {
      // Check if we already got enough space
      if( m_tokenSize < neededSize ) {
        lfree( m_tokenFloats );
        m_tokenSize = 0;
        m_tokenFloats = ( RtFloat * ) lmalloc( neededSize );
        if( ! m_tokenFloats ) {
          printf("Error : liqTokenPointer out of memory for %ld bytes\n", neededSize );
          return 0;
        }
        m_tokenSize = neededSize;
      }
    } else if( neededSize ) {
      m_tokenFloats = ( RtFloat * ) lmalloc( neededSize );
      if( ! m_tokenFloats ) {
        printf("Error : liqTokenPointer out of memory for %ld bytes\n", neededSize );
        return 0;
      }
      m_tokenSize = neededSize;
    }
    if( debugMode ) printf( "Needed %ld got %ld\n", neededSize, m_tokenSize );
  } else {
    // Space will be allocated when string will be set
    m_isArray     = false;  // Do not handle array of strings
    m_isUArray    = false;
    m_arraySize   = 0;
    if( m_tokenFloats ) {
      lfree( m_tokenFloats );
    }
    m_tokenFloats = NULL;
    m_tokenSize   = 0;
  }
  return 1;
} 

int liqTokenPointer::adjustArraySize( unsigned int size )
{
  if( m_arraySize != size )
  {
    // Only augment allocated memory if needed, do not reduce it
    unsigned long neededSize = size * m_eltSize * sizeof( RtFloat);
    if( m_tokenSize < neededSize ) {
      RtFloat * tmp = ( RtFloat * ) realloc( m_tokenFloats, neededSize );
      // Hmmmmm should get a way to report error message to caller
      if( !tmp )
        return 0;
      m_tokenFloats = tmp; 
      m_tokenSize = neededSize;
    }
    m_arraySize = size;
  }
  return m_arraySize;
}

void liqTokenPointer::setDetailType( DetailType dType )
{
  m_dType = dType;
}

DetailType liqTokenPointer::getDetailType( void )
{
  return m_dType;
}

void liqTokenPointer::setTokenFloat( unsigned int i, RtFloat val )
{
#ifdef DEBUG
  unsigned int max = m_tokenSize / sizeof( RtFloat);
  if( i >= max ) {
    cout << "Error : out of space for tokens max : " << max << " index " << i << endl;
  }
#endif
  m_tokenFloats[i] = val;
}

void liqTokenPointer::setTokenFloat( unsigned int i, RtFloat x, RtFloat y , RtFloat z )
{
  m_tokenFloats[3 * i + 0] = x;
  m_tokenFloats[3 * i + 1] = y;
  m_tokenFloats[3 * i + 2] = z;
}

void liqTokenPointer::setTokenFloats( const RtFloat * vals )
{
  if( m_isArray || m_isUArray ) {
    memcpy( m_tokenFloats, vals, m_arraySize * m_eltSize * sizeof( RtFloat) );
  } else {
    memcpy( m_tokenFloats, vals, m_eltSize * sizeof( RtFloat) );
  }
}


const RtFloat * liqTokenPointer::getTokenFloatArray( )
{
  return m_tokenFloats;
}

void liqTokenPointer::setTokenFloat( unsigned int i, RtFloat x, RtFloat y , RtFloat z, RtFloat w )
{
  m_tokenFloats[4 * i + 0] = x;
  m_tokenFloats[4 * i + 1] = y;
  m_tokenFloats[4 * i + 2] = z;
  m_tokenFloats[4 * i + 3] = w;
}

void liqTokenPointer::setTokenString( const char *str, unsigned int length )
{
  if( m_tokenString ) lfree( m_tokenString );
  m_tokenString = NULL;
  m_stringSize = 0;
  if( length > 0 )
  {
    m_tokenString = ( char * ) lmalloc( ( length + 1 ) * sizeof( char ) ); // Need space for '\0' 
    if( m_tokenString ) 
    {
      strcpy( m_tokenString, str );
      m_stringSize = length + 1;
    }
  }
}

void liqTokenPointer::setTokenName( const char * name )
{
  // Hmmm we should check name length here
  strcpy( m_tokenName, name );
}

char * liqTokenPointer::getTokenName( void )
{
  // Hmmmm should we handle token without name ?
  return m_tokenName;
}


RtPointer liqTokenPointer::getRtPointer( void )
{
  if( m_pType == rString ) return ( RtPointer ) &m_tokenString;
  else return ( RtPointer ) m_tokenFloats;
}

void liqTokenPointer::getRiDeclare( char *declare  )
{
  switch ( m_pType ) {
  case rString:
    sprintf( declare, "%s string", StringDetailType[m_dType]  );
    break;
  case rFloat:
    if( m_isUArray ) {
      sprintf( declare, "%s float[%d]", StringDetailType[m_dType], m_uArraySize );
    } else {
      sprintf( declare, "%s float", StringDetailType[m_dType] );
    }
    break;
  case rHpoint:
    // Hmmmmm not sure I do right thing here
  case rPoint:
    if( m_isNurbs ) {
      if ( m_isArray ) {
        sprintf( declare, "%s hpoint", StringDetailType[m_dType] );
      } else {
        sprintf( declare, "%s point", StringDetailType[m_dType] );
      }
    } else {
      sprintf( declare, "%s point", StringDetailType[m_dType] );
    }
    break;
  case rVector:
    sprintf( declare, "%s vector", StringDetailType[m_dType] );
    break;
  case rNormal:
    sprintf( declare, "%s normal", StringDetailType[m_dType] );
    break;
  case rColor:
    sprintf( declare, "%s color", StringDetailType[m_dType] );
    break;
  }
}

bool liqTokenPointer::isBasicST( void )
{
  // Not st or, if it is, face varying
  if( m_tokenName[0] != 's' || m_tokenName[1] != 't' || m_tokenName[2] != '\0' || m_dType == rFaceVarying ) {
    return false;
  }
  return true;
}

