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

extern int debugMode;


const char * liqTokenPointer::StringDetailType[] = {
  "uniform",
  "varying",
  "vertex",
  "constant",
  "facevarying",
  "facevertex"
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

liqTokenPointer::liqTokenPointer( const liqTokenPointer &src )
{
  LIQDEBUGPRINTF("-> copy constructing additional ribdata: " );
  LIQDEBUGPRINTF(src.m_tokenName);
  LIQDEBUGPRINTF("\n" );

  m_tokenFloats   = NULL;
  m_tokenString   = NULL;
  m_isArray       = false;
  m_isUArray      = false;
  m_isNurbs       = false;
  m_isString      = false;
  m_isFull        = false;
  m_arraySize     = 0;
  m_uArraySize    = 0;
  m_eltSize       = 0;
  m_tokenSize     = 0;
  m_stringSize    = 0;

  if( src.m_isUArray )
    // Moritz: this is the baddy: src.m_arraySize wasn't divided by m_uArraySize!
    //         m_uArraySize actually is only used in the set() method to calculate
    //         m_arraySize and in getRiDeclare()
    // Todo:   Find a less Italian (non-spaghetticode) solution to this.
    set( src.m_tokenName, src.m_pType, src.m_isNurbs, src.m_arraySize / src.m_uArraySize, src.m_uArraySize );
  else
    set( src.m_tokenName, src.m_pType, src.m_isNurbs, src.m_arraySize );
  m_dType = src.m_dType;

  if( m_pType != rString ) {
    setTokenFloats( src.m_tokenFloats );
  } else {
    if( src.m_tokenString ) {
      if ( m_arraySize ) {
        for ( unsigned int i=0; i<m_arraySize; i++) {
          setTokenString( i, src.m_tokenString[i], strlen( src.m_tokenString[i]) );
        }
      } else {
        setTokenString( 0, src.m_tokenString[0], strlen( src.m_tokenString[0]) );
      }
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
  if( src.m_isUArray )
    set( src.m_tokenName, src.m_pType, src.m_isNurbs, src.m_arraySize / src.m_uArraySize, src.m_uArraySize );
  else
    set( src.m_tokenName, src.m_pType, src.m_isNurbs, src.m_arraySize );
  m_dType = src.m_dType;
  if( m_pType != rString ) {
    setTokenFloats( src.m_tokenFloats );
  } else {
    if( src.m_tokenString ) {
      if ( m_arraySize ) {
        for ( unsigned int i=0; i<m_arraySize; i++) {
          setTokenString( i, src.m_tokenString[i], strlen( src.m_tokenString[i]) );
        }
      } else {
        setTokenString( 0, src.m_tokenString[0], strlen( src.m_tokenString[0]) );
      }
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
  resetTokenString();

};

void liqTokenPointer::reset()
{
  if( m_tokenFloats ) { lfree( m_tokenFloats ); m_tokenFloats = NULL; }
  if( m_tokenString ) {
    resetTokenString();
  }
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

int liqTokenPointer::set( const char * name, ParameterType ptype, bool asNurbs )
{
  return set( name, ptype, asNurbs, 0, 0 );
}

int liqTokenPointer::set( const char * name, ParameterType ptype, bool asNurbs, unsigned int arraySize )
{
  return set( name, ptype, asNurbs, arraySize, 0 );
}

int liqTokenPointer::set( const char * name, ParameterType ptype, bool asNurbs, bool asArray, bool asUArray, unsigned int arraySize )
{
  // philippe : passing arraySize when asUArray is true fixed the float array export problem
  // TO DO : replace occurences of this function with non-obsolete ones
  //return set( name, ptype, asNurbs, arraySize, asUArray ? 2 : 0 );

  return set( name, ptype, asNurbs, asArray? arraySize : 1, asUArray ? arraySize : 0  );
}

int liqTokenPointer::set( const char * name, ParameterType ptype, bool asNurbs, unsigned int arraySize, unsigned int uArraySize )
{
  setTokenName( name );
  m_pType = ptype;
  m_isNurbs = asNurbs;
  if( m_pType != rString ) {

    resetTokenString();

    // define element size based on parameter type
    switch( m_pType ) {
    case rFloat :
      m_eltSize = 1;
      break;
    case rPoint :
      if( m_isNurbs ) {
        m_eltSize = 4;
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
    case rMatrix:
      m_eltSize = 16;
      break;
    }

    // check how much we need if we have an array
    m_isArray = arraySize != 0;
    unsigned long neededSize;
    if( m_isArray ) {
      m_arraySize = arraySize;
      m_isUArray = uArraySize != 0;
      if( m_isUArray ) {
        m_uArraySize = uArraySize;
        m_arraySize *= m_uArraySize;
      }
      neededSize = m_arraySize * m_eltSize * sizeof( RtFloat);
    } else {
      m_arraySize = 0;
      neededSize = m_eltSize * sizeof( RtFloat);
    }

    // allocate whatever we need
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

    LIQDEBUGPRINTF( "Needed %ld got %ld\n", neededSize, m_tokenSize );
  } else {
    // STRINGS ARE A SPECIAL CASE
    // Space is now allocated upfront

    // free mem
    if( m_tokenFloats ) {
      lfree( m_tokenFloats );
      m_tokenFloats = NULL;
    }
    resetTokenString();

    m_isUArray    = false;
    m_isArray = arraySize != 0;

    if ( m_isArray ) {

      // init array size
      m_arraySize = arraySize;
      m_tokenSize   = 0;
      // init pointer array
      m_tokenString = ( char ** ) lcalloc( m_arraySize , sizeof( char * ) );

    } else {

      m_arraySize   = 0; // useless
      m_tokenSize   = 0;
      m_tokenString = ( char ** ) lcalloc( 1, sizeof( char * ) );
    }
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

ParameterType liqTokenPointer::getParameterType( void )
{
  return m_pType;
}

void liqTokenPointer::setTokenFloat( unsigned int i, RtFloat val )
{
#ifdef DEBUG
  unsigned int max = m_tokenSize / sizeof( RtFloat);
  if( i >= max ) {
    LIQDEBUGPRINTF( "setTokeFloat out of bounds, max: %d, asked: %d\n", max, i );
  }
#endif
  m_tokenFloats[i] = val;
}

void liqTokenPointer::setTokenFloat( unsigned int i, unsigned int uIndex, RtFloat val )
{
#ifdef DEBUG
  unsigned int max = m_tokenSize / sizeof( RtFloat);
  if( i * m_uArraySize + uIndex >= max ) {
    LIQDEBUGPRINTF( "setTokenFloat out of bounds, max: %d, asked: %d\n", max, i * m_uArraySize + uIndex );
  }
#endif
  setTokenFloat( i * m_uArraySize + uIndex, val );
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


char * liqTokenPointer::getTokenString( void )
{
  return m_tokenString[0];
}

void liqTokenPointer::setTokenString( unsigned int i, const char *str, unsigned int length )
{
  m_tokenString[i] = ( char * ) lmalloc( ( length + 1 ) * sizeof( char ) );
  if( m_tokenString[i] ) {
    strcpy( m_tokenString[i], str );
    m_stringSize += length + 1;
  }
}

void liqTokenPointer::resetTokenString( void )
{
  if ( m_tokenString ) {
    unsigned int tokensize = (m_arraySize > 0)? m_arraySize:1;
    for ( unsigned int ii = 0; ii<tokensize; ii++ ) {
      if ( m_tokenString[ii] ) lfree( m_tokenString[ii] );
    }
    lfree( m_tokenString );
    m_tokenString = NULL;
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

char * liqTokenPointer::getDetailedTokenName( void )
{
  // Hmmmm should we handle token without name ?
#ifdef PRMAN
  // Philippe : in PRMAN, declaring P as a vertex point is not necessary and it make riCurves generation fail.
  // so when the token is P, we just skip the type declaration.
  if ( strcmp(m_tokenName, "P\0") == 0 ) {
    m_detailedTokenName[0] = '\0';
  } else {
    getRiDeclare( m_detailedTokenName );
    strcat( m_detailedTokenName, " " );
  }
#else
  getRiDeclare( m_detailedTokenName );
  strcat( m_detailedTokenName, " " );
#endif
  strcat( m_detailedTokenName, m_tokenName );
  return m_detailedTokenName;
}

RtPointer liqTokenPointer::getRtPointer( void )
{
  if( m_pType == rString ) {
    return ( RtPointer ) m_tokenString;
  } else {
    return ( RtPointer ) m_tokenFloats;
  }
}

void liqTokenPointer::getRiDeclare( char *declare  )
{

  switch ( m_pType ) {
  case rString:
    if ( m_isArray ) {
      sprintf( declare, "%s string[%d]", StringDetailType[m_dType], m_arraySize );
    } else {
      sprintf( declare, "%s string", StringDetailType[m_dType]  );
    }
    break;
  case rMatrix:
    sprintf( declare, "%s matrix", StringDetailType[m_dType]  );
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
      if( m_isUArray ) {
        sprintf( declare, "%s point[%d]", StringDetailType[m_dType], m_uArraySize );
      } else {
        sprintf( declare, "%s point", StringDetailType[m_dType] );
      }
    }
    break;
  case rVector:
    if( m_isUArray ) {
      sprintf( declare, "%s vector[%d]", StringDetailType[m_dType], m_uArraySize );
    } else {
      sprintf( declare, "%s vector", StringDetailType[m_dType] );
    }
    break;
  case rNormal:
    if( m_isUArray ) {
      sprintf( declare, "%s normal[%d]", StringDetailType[m_dType], m_uArraySize );
    } else {
      sprintf( declare, "%s normal", StringDetailType[m_dType] );
    }
    break;
  case rColor:
    if( m_isUArray ) {
      sprintf( declare, "%s color[%d]", StringDetailType[m_dType], m_uArraySize );
    } else {
      sprintf( declare, "%s color", StringDetailType[m_dType] );
    }
    break;
  }
}

bool liqTokenPointer::isBasicST( void )
{
  // Not st or, if it is, face varying
  if( m_tokenName[0] != 's' || m_tokenName[1] != 't' || m_tokenName[2] != '\0' || m_dType == rFaceVarying || m_dType == rFaceVertex ) {
    return false;
  }
  return true;
}

