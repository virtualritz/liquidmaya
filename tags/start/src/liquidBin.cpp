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

/* ______________________________________________________________________
** 
** Liquid Executable
** ______________________________________________________________________
*/

// Standard Headers
#include <iostream.h>
#include <fstream.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>
#include <signal.h>

#ifndef _WIN32
// Dynamic Object Headers
#include <dlfcn.h>
#endif

// Renderman Headers
extern "C" {
	#include <ri.h>
	#include <slo.h>
}

// STL headers
#include <list>
#include <vector>
#include <string>

#ifdef _WIN32
	#include <process.h>
	#include <malloc.h>
#else
	#include <unistd.h>
	#include <stdlib.h>
	#include <alloca.h>
	#include <signal.h>
#endif

// Maya's Headers
#include <maya/MGlobal.h>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MLibrary.h>
#include <maya/MFileIO.h>

#include <liquid.h>
#include <liquidRibTranslator.h>
#include <liquidGetSloInfo.h>
#include <liquidGetAttr.h>
#include <liquidAttachPrefAttribute.h>
#include <liquidPreviewShader.h>
#include <liquidMemory.h>
#include <liquidGlobalHelpers.h>

bool			isRegistered;
extern	float liquidVersion;
extern  bool	liquidBin;

const char* usage = 
"usage: liquid [options] filename ...\n\
please see the liquid documentation for command line parameters. \n";

void signalHandler(int sig)
{
  if (sig == SIGTERM) {
    throw( MString( "Terminated!\n" ) );
  }
  else {
    signal(sig, signalHandler);
  }
}

main(int argc, char **argv)
//
//  Description:
//      Register the command when the plug-in is loaded
//
{
	MStatus status;
  MString command;
  MString UserClassify;
	MString	fileName;

	// initialize the maya library
	status = MLibrary::initialize (argv[0], true );
	if (!status) {
		status.perror("MLibrary::initialize");
		return 1;
	}
	
	// we are now running a "virtual" maya
	
	// maya's argument list
	MArgList myArgs;
	MString mainArg = "-GL";
	myArgs.addArg( mainArg );
	
	liquidBin = true;
	
	    printf("Liquid Expires on: %d/%d/%d \n", expday, expmonth, expyear + 1900 );
	// make sure the beta version expires, this will be the license handling routines also!
	time_t ltime;
	struct tm *today;
	time( &ltime );
	today = localtime( &ltime );
	//int wyear, wmonth, wday;
	isRegistered = true;
	if ( today->tm_year > expyear ) {
		printf("Liquid Expired! contact buzz1@paradise.net.nz or canuck@wetafx.co.nz! ");
		exit(1);
	}
	if ( today->tm_mon > (expmonth - 1) && today->tm_year == expyear ) {
		printf("Liquid Expired! contact buzz1@paradise.net.nz or canuck@wetafx.co.nz! ");
		exit(1);
	}
	if ( today->tm_mday > expday && today->tm_mon == (expmonth - 1) && today->tm_year == expyear ) {
		printf("Liquid Expired! contact buzz1@paradise.net.nz or canuck@wetafx.co.nz! ");
		exit(1);
	}

  for (unsigned i = 0; i <= SIGRTMAX; i++) signal(i, signalHandler);
	
	argc--, argv++;

	if (argc == 0) {
		cerr << usage;
		return(1);
	}

	// scan the command line arguments
	uint i;
	for ( i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help")) {
			cerr << usage;
			return(1);
		} else if ( !strcmp(argv[i], "-mf")  ) {
			i++;
			fileName = argv[i];  // set the fileName
		} else {
			MString newArg = argv[i];
			myArgs.addArg( newArg );
		}
	}

	// check that the filename has been specified and exists	
	if ( fileName == "" ) {
		status.perror("Liquid -> no filename specified!\n" );
		printf( "ALF_EXIT_STATUS 1\n" );
		MLibrary::cleanup( 1 );
		return (1);
	} else if ( !fileExists( fileName ) ) {
		status.perror("Liquid -> file not found!\n");
		printf( "ALF_EXIT_STATUS 1\n" );
		MLibrary::cleanup( 1 );
		return ( 1 );
	}	else {
		// load the file into liquid's virtual maya
		status = MFileIO::open( fileName );
		if ( !status ) {
			MString error = " Error opening file: ";
			error += fileName.asChar();
			status.perror( error );
			printf( "ALF_EXIT_STATUS 1\n" );
			MLibrary::cleanup( 1 );
			return( 1 ) ;
		}

		RibTranslator liquidTrans;
	  for (unsigned i = 0; i <= SIGRTMAX; i++) signal(i, signalHandler);
		liquidTrans.doIt( myArgs );
	}
 
	printf( "ALF_EXIT_STATUS 0\n" );
	MLibrary::cleanup( 0 );
	return (0);
}

