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
** The RenderMan (R) Interface Procedures and Protocol are:
** Copyright 1988, 1989, Pixar
** All Rights Reserved
**
**
** RenderMan (R) is a registered trademark of Pixar
*/

#include "liqProcessLauncher.h"

#include <maya/MString.h>


/* ______________________________________________________________________
** 
** Linux implementation of liqProcessLauncher::execute()
** ______________________________________________________________________
*/ 
#if defined(LINUX)

#include <sys/types.h>
#include <unistd.h>

bool liqProcessLauncher::execute(const MString &command, const MString &arguments)
{
	int res = vfork();
  if (res == -1) {
		return false;
	} else if (res == 0) {
	  execlp(command.asChar(), command.asChar(), arguments.asChar(), NULL);
	}
	return true;
}
#endif // LINUX


/* ______________________________________________________________________
** 
** Irix implementation of liqProcessLauncher::execute()
** ______________________________________________________________________
*/ 
#if defined(IRIX)

#include <sys/types.h>
#include <unistd.h>

bool liqProcessLauncher::execute(const MString &command, const MString &arguments)
{
	pcreatelp(command.asChar(), command.asChar(), arguments.asChar(), NULL);
	return true;
}
#endif // IRIX


/* ______________________________________________________________________
** 
** Win32 implementation of liqProcessLauncher::execute()
** ______________________________________________________________________
*/ 
#if defined(_WIN32)

#include <windows.h>

bool liqProcessLauncher::execute(const MString &command, const MString &arguments)
{
	ShellExecute(NULL, NULL, command.asChar(), arguments.asChar(), NULL, SW_SHOWNORMAL);
	return true;
}
#endif // _WIN32
