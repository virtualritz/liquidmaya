# - Try to find MAYA
# Once done this will define
#
#  Maya_FOUND - system has Maya
#  Maya_INCLUDE_DIR - the Maya include directory
#  Maya_LIBRARIES - Link this to use Maya

# Copyright (c) 2009, Moritz Moeller, <cmake@virtualritz.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

IF( Maya_INCLUDE_DIR AND Maya_LIBRARIES )
  SET( Maya_FIND_QUIETLY TRUE )
ENDIF( Maya_INCLUDE_DIR AND Maya_LIBRARIES )

if(APPLE)
	set(MAYA_INCLUDE ${MAYA_LOCATION}/devkit/include) 
	set(MAYA_LIB ${MAYA_LOCATION}/Maya.app/Contents/MacOS)
else(APPLE)
	set(MAYA_INCLUDE ${MAYA_LOCATION}/include) 
	set(MAYA_LIB ${MAYA_LOCATION}/lib)	
endif(APPLE)

FIND_PATH( Maya_INCLUDE_DIR maya/MFn.h ${MAYA_INCLUDE} )

FIND_LIBRARY( Maya_LIB_Foundation NAMES Foundation PATHS ${MAYA_LIB} NO_DEFAULT_PATH)
FIND_LIBRARY( Maya_LIB_OpenMaya NAMES OpenMaya PATHS ${MAYA_LIB} )
FIND_LIBRARY( Maya_LIB_OpenMayaAnim NAMES OpenMayaAnim PATHS ${MAYA_LIB} )
FIND_LIBRARY( Maya_LIB_OpenMayaFX NAMES OpenMayaFX PATHS ${MAYA_LIB} )
FIND_LIBRARY( Maya_LIB_OpenMayaRender NAMES OpenMayaRender PATHS ${MAYA_LIB} )
FIND_LIBRARY( Maya_LIB_OpenMayaUI NAMES OpenMayaUI PATHS ${MAYA_LIB} )

set( Maya_LIBRARIES ${Maya_LIB_Foundation} ${Maya_LIB_OpenMaya} ${Maya_LIB_OpenMayaAnim} ${Maya_LIB_OpenMayaFX} ${Maya_LIB_OpenMayaRender} ${Maya_LIB_OpenMayaUI} )

# handle the QUIETLY and REQUIRED arguments and set Maya_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( Maya DEFAULT_MSG Maya_LIBRARIES Maya_INCLUDE_DIR )

MARK_AS_ADVANCED( Maya_INCLUDE_DIR Maya_LIBRARIES )

