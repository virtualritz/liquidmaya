README.txt

Liquid Rendering Toolkit

Note from the original developer, Colin Doncaster.
--------------------------------------------------

	Liquid has been in development for over three and a half years now, starting off as a small tool that was used at my own company in Toronto and then evolving into a production tool able to handle large complex scenes.  I've had a lot of input from people on how it should and shouldn't work, what needs to get added and where the bugs were.  Over the first year I made a decision that I was going to develop Liquid as a production tool that was meant to be used by Technical Directors to get their job done, not a rendering interface for visually building shaders.  
	Because the Renderman interface has been pushed by Pixar as a standard their are various different renderers out there that support it, with slightly different options and attributes and quiet different compiled shader formats.  Because of this it tends to get quiet ugly, you'll understand once you start to take a look at the liquidGetSloInfo source.  Add to the confusion different platforms, different means of compiling Maya plugins on those platforms and other inconsistencies you'll understand why I've just provided somewhat generic makefiles.
	Some people who have know me and the amount of time I've spent with the source think I am crazy for giving it away.  Maybe I am.  Over the last while I've been wrestling with idea of marketing the plug-in, the source, or coming up with an closed development group for it.  All were interesting ideas but I really wanted people to use it, after all there really is only two or three companies using it.  More importantly what I would like to see is people actually contributing back changes, bug fixes and additions they make.  Lastly, I want to make it accessible - for all the studios that want to start using higher end software but don't want to pay the extreme costs for it ( MTOR is more expensive than Maya these days! ).  
	
	Thanks - Colin Doncaster
	colin@nomadicmonkey.com

Compiling
---------

    	Liquid has been succesfully compiled against Aqsis, Entropy and PRman libraries on linux.
	To compile : 
	    - Go in a shell into the src directory. 
	    - Set the below environment variables as desired.
		LIQRMAN : which library to compile against. Can be aqsis ( default if not set ), entropy or prman.
		MAYA_LOCATION : specify where is the main directory of maya ( /usr/aw/maya if not set )
		AW_LOCATION : specify where various versions of maya are installed ( /usr/aw if not set )
	    - Type :
		make debug to get a version of Liquid compiled with debugging flags ( use MAYA_LOCATION )
		make release to get a version of Liquid compiled with release flags ( use MAYA_LOCATION )
		make newversion to get a set of version of Liquid compiled agaisnt different version of maya installed ( use AW_LOCATION )

Thanks
------
( in no particular order )

	Liz Vezina, My Folks, Kris Howald, Berj Bannayan, Jeff Hameluck, Dan Lemmon, Ken McGaugh, Mark Tait, James Cunningham, Wayne Stables, Guy Williams, Joe Letteri, John Shiels, Daniel Kramer, Jamie McCarter, Julian Mann, Cory Bedwell, Seth Lippman, Matt Hightower, Randy Goux, Greg Butler, Markus Manninen, Larry Gritz, Shai Hinitz and anyone else I forgot.  

License
----------
The contents of this file are subject to the Mozilla Public License Version 1.1 (the 
"License"); you may not use this file except in compliance with the License. You may 
obtain a copy of the License at http://www.mozilla.org/MPL/ 
 
Software distributed under the License is distributed on an "AS IS" basis, WITHOUT 
WARRANTY OF ANY KIND, either express or implied. See the License for the specific 
language governing rights and limitations under the License. 

The Original Code is the Liquid Rendering Toolkit.  
The Initial Developer of the Original Code is Colin Doncaster. Portions created by 
Colin Doncaster are Copyright (C) 2002. All Rights Reserved. 
 
Contributor(s): Berj Bannayan. 

The RenderMan (R) Interface Procedures and Protocol are:
Copyright 1988, 1989, Pixar
All Rights Reserved

RenderMan (R) is a registered trademark of Pixar




