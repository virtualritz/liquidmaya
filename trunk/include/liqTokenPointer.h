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
#ifndef liqTokenPointer_H
#define liqTokenPointer_H
#include <ri.h>

// token/pointer pairs structure

enum ParameterType {
    rFloat  = 0,
    rPoint  = 1,
    rVector = 2,
    rNormal = 3,
    rColor  = 4,
    rString = 5,
    rHpoint = 6
};

enum DetailType {
    rUniform	    = 0,
    rVarying	    = 1,
    rVertex 	    = 2,
    rConstant	    = 3,
    rFaceVarying    = 4
};

class liqTokenPointer
{
public:
    liqTokenPointer();
    liqTokenPointer(const liqTokenPointer &src);
    liqTokenPointer & operator=( const liqTokenPointer &src);
    ~liqTokenPointer();
    void    	    setTokenName( const char * name );
    char *  	    getTokenName( void );
    int     	    set( const char * name, ParameterType ptype, bool asNurbs, bool asArray, bool asUArray, unsigned int arraySize );
    int     	    adjustArraySize( unsigned int size );
    void    	    setDetailType( DetailType dType );
    DetailType	    	getDetailType( void );
    void    	    setTokenFloat( unsigned int i, RtFloat val );
    void    	    setTokenFloat( unsigned int i, RtFloat x, RtFloat y , RtFloat z );
    void    	    setTokenFloat( unsigned int i, RtFloat x, RtFloat y , RtFloat z, RtFloat w );
    void    	    setTokenFloats( const RtFloat * floatVals );
    const RtFloat * getTokenFloatArray( );
    void    	    setTokenString( const char *str, unsigned int length );
    RtPointer 	    	getRtPointer( void );
    void    	    getRiDeclare( char * declare );
    bool    	    	isBasicST( void );
    void    	    reset( void );
private:
    RtFloat *   m_tokenFloats;
    char * m_tokenString;
    ParameterType m_pType;
    DetailType m_dType;
    char m_tokenName[256];
    unsigned int m_arraySize;
    unsigned int  m_uArraySize;
    unsigned int m_eltSize;
    bool m_isArray;
    bool m_isUArray;
    bool m_isNurbs;
    bool m_isString;
    bool m_isFull;
    static const char * StringDetailType[];
    long m_stringSize;
    long m_tokenSize;
};


#endif //liquidTokenPointer_H
