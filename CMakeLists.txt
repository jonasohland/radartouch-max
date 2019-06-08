cmake_minimum_required(VERSION 3.4)

if (${CMAKE_GENERATOR} MATCHES "Xcode")
 		if (${XCODE_VERSION} VERSION_LESS 9)
  			message(STATUS "Xcode 9 or higher is required. Please install from the Mac App Store.")
			return ()
		endif ()
endif ()


include(${CMAKE_CURRENT_SOURCE_DIR}/lib/min-api/script/min-package.cmake)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/liboh)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/min-api)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/shared)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/source/radartouch)