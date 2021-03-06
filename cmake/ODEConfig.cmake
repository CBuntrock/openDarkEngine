#
# Try to find ODE headers on both Windows and Linux systems
# 
# The following values will get defined:
#	ODE_FOUND - True if ODE was found
#	ODE_INCLUDE_DIR - Include headers for ODE usage
#
# $Id$

SET(ODE_FOUND)
MARK_AS_ADVANCED(ODE_FOUND)
MARK_AS_ADVANCED(ODE_LIBRARIES)
MARK_AS_ADVANCED(ODE_INCLUDE_DIR)
MARK_AS_ADVANCED(ODE_PATH)

# For win32 builds. This can be filled by hand. Standard path is dependencies directory next to opde binary dir (out of source build target directory)
SET(ODE_PATH "${OPDE_BINARY_DIR}/../dependencies/ode/" CACHE STRING "For Win32 include directories and the installer. Can be filled to the path of the ODE")

IF (UNIX)
    # Let's look for ODE
    SET(ODE_INC_SEARCH_PATH
        /usr/include/
        /usr/local/include/
     )
	 
	SET(ODE_LIB_SEARCH_PATH
        /lib
        /usr/lib
        /usr/lib32
        /usr/lib64
        /usr/local/lib
        /usr/local/lib32
        /usr/local/lib64
     )
	 
	SET(ODE_LIBNAMES ode)

    # The search begins
    FIND_PATH(ODE_INCLUDE_DIR ode/ode.h ${ODE_INC_SEARCH_PATH})

    IF(NOT ODE_INCLUDE_DIR)
        MESSAGE("Warning: ode/ode.h not found under ${ODE_INC_SEARCH_PATH}")
        SET(ODE_FOUND 0)
    ENDIF(NOT ODE_INCLUDE_DIR)
	
	FIND_LIBRARY(ODE_LIBRARIES NAME ${ODE_LIBNAMES} PATHS ${ODE_LIB_SEARCH_PATH})
	
	IF(NOT ODE_LIBRARIES)
        MESSAGE("Warning: ode library not found under ${ODE_INC_SEARCH_PATH}")
        SET(ODE_FOUND 0)
    ENDIF(NOT ODE_LIBRARIES)
    
    # 
    
ENDIF(UNIX)

IF(WIN32)
    # TODO: Ogre SDK also includes ODE, and it might actually suffice
    
    SET(ODE_INC_SEARCH_PATH
        $ENV{ODE_PATH}/include
	${ODE_PATH}/include
    )
	
	SET(ODE_LIB_SEARCH_PATH
        $ENV{ODE_PATH}/lib/ReleaseSingleLib
	${ODE_PATH}/lib/ReleaseSingleLib
    )
    
	SET(ODE_LIBNAMES ode_single)
	
    # The search begins
    FIND_PATH(ODE_INCLUDE_DIR ode/ode.h ${ODE_INC_SEARCH_PATH})

    IF(NOT ODE_INCLUDE_DIR)
        MESSAGE("Warning: ode/ode.h not found under ${ODE_INC_SEARCH_PATH}")
        SET(ODE_FOUND 0)
    ENDIF(NOT ODE_INCLUDE_DIR)
	
	FIND_LIBRARY(ODE_LIBRARIES NAME ${ODE_LIBNAMES} PATHS ${ODE_LIB_SEARCH_PATH})
	
	IF(NOT ODE_LIBRARIES)
        MESSAGE("Warning: ode library not found under ${ODE_INC_SEARCH_PATH}")
        SET(ODE_FOUND 0)
    ENDIF(NOT ODE_LIBRARIES)
    
ENDIF(WIN32)
