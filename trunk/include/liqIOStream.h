#ifndef liqIOStream_H
#define liqIOStream_H

// This file is intended to be the only std io header file included by any Liquid 
// files. This is to get around problems between Maya versions, platforms and
// compilers, some of which try and help by auto-including or auto-namespacing,
// and others do not

//#define MAYA_API_VERSION 600

#if MAYA_API_VERSION < 500
  #include <iostream>
    using std::cout;
    using std::cerr;
    using std::endl;
    using std::flush;
#else
  #include <maya/MIOStream.h>
#endif


#endif // liqIOStream_H
