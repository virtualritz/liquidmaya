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


#include <liqRenderer.h>

#include <liqEntropyRenderer.h>
#include <liqPrmanRenderer.h>
#include <liqAqsisRenderer.h>
#include <liqDelightRenderer.h>


const liqRenderer & liquidRenderer()
{
  // first thing we should do is setup our renderer

  // TODO: got to make this much better in the future -- get the renderer
  // and version from the globals UI
#if defined(ENTROPY)
  static liqRenderer *renderer = new liqEntropyRenderer("3.1");
#elif defined(PRMAN)
  static liqRenderer *renderer = new liqPrmanRenderer("3.9");
#elif defined(AQSIS)
  static liqRenderer *renderer = new liqAqsisRenderer("0.7.4");
#elif defined(DELIGHT)
  static liqRenderer *renderer = new liqDelightRenderer("1.0.0");
#else
  ERROR: unknown renderer
#endif

  return *renderer;
}
