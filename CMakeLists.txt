cmake_minimum_required (VERSION 3.0)

project(TextureGen)

set (INCLUDE_DIR
thirdparty
thirdparty/volk
thirdparty/Vulkan-Headers/include
.
)

include_directories(${INCLUDE_DIR})

add_subdirectory(thirdparty/volk)


add_executable(PackShaders pack_shaders.cpp)
add_custom_target(Run_PackShaders
COMMAND PackShaders
WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
COMMENT "Running PackShaders in ${CMAKE_CURRENT_SOURCE_DIR}"
SOURCES pack_shaders.cpp
)


set (SOURCE
context.cpp
renderer.cpp
api.cpp
)

set (HEADER
context.h
renderer.h
api.h
)

if (WIN32) 
set (DEFINES  ${DEFINES}
-D"_CRT_SECURE_NO_DEPRECATE"  
-D"_SCL_SECURE_NO_DEPRECATE" 
-D"_CRT_SECURE_NO_WARNINGS"
)
else()
add_definitions(-std=c++14)
add_compile_options(-fPIC)
endif()

add_definitions(${DEFINES})
add_library(TextureGen SHARED ${SOURCE} ${HEADER})
target_link_libraries(TextureGen volk)

add_dependencies(Run_PackShaders PackShaders)
add_dependencies(TextureGen Run_PackShaders)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX  ../python CACHE PATH "Install path" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

if (WIN32) 
install(TARGETS TextureGen RUNTIME DESTINATION .)
else()
install(TARGETS TextureGen DESTINATION .)
endif()
