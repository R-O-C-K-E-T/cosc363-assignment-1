cmake_minimum_required(VERSION 3.0.0)
project(Assignment VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# OpenGL
set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)

# GLM
set(glm_DIR glm/cmake/glm)
find_package(glm REQUIRED)

# GLFW
set( GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only" )
set( GLFW_INSTALL OFF CACHE BOOL  "GLFW lib only" )

add_subdirectory(glfw)

if( MSVC )
    SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup" )
endif()

# Add source files
file(GLOB_RECURSE SOURCE_FILES 
	${CMAKE_SOURCE_DIR}/src/*.cpp)
	
# Add header files
file(GLOB_RECURSE HEADER_FILES 
	${CMAKE_SOURCE_DIR}/src/*.h)

include_directories(include)

#add_link_options(-Wl,-no-as-needed -lprofiler)

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(Assignment ${OPENGL_LIBRARIES} glfw glm::glm ${GLUT_LIBRARIES})

# Random windows stuff
if( MSVC )
    if(${CMAKE_VERSION} VERSION_LESS "3.6.0") 
        message( "\n\t[ WARNING ]\n\n\tCMake version lower than 3.6.\n\n\t - Please update CMake and rerun; OR\n\t - Manually set 'GLFW-CMake-starter' as StartUp Project in Visual Studio.\n" )
    else()
        set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Assignment )
    endif()
endif()