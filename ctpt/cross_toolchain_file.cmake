

set(CCPFREFIX arm-linux-gnueabihf-)
# where to find the compiler
find_program(CROSS_TOOL_FULL ${CCPFREFIX}gcc )

if(NOT CROSS_TOOL_FULL)
  message(STATUS "relative path" )

else()
  message(STATUS "cross tool auto found" )
  # get path
  get_filename_component(CROSS_TOOL_FULL_PATH ${CROSS_TOOL_FULL} DIRECTORY CACHE )
endif()

# setup compiler
include (CMakeForceCompiler)

# linux os
set(CMAKE_SYSTEM_NAME Linux)

CMAKE_FORCE_C_COMPILER(${CROSS_TOOL_FULL_PATH}/${CCPFREFIX}gcc GNU)
CMAKE_FORCE_CXX_COMPILER (${CROSS_TOOL_FULL_PATH}/${CCPFREFIX}g++ GNU)
#CMAKE_FORCE_ASM_COMPILER (${CROSS_TOOL_FULL_PATH}/arm-none-eabi-g++ GNU)
set(CMAKE_SYSTEM_PROCESSOR arm)

#set(CMAKE_SYSTEM_INCLUDE_PATH /include )
#set(CMAKE_SYSTEM_LIBRARY_PATH /lib )
#set(CMAKE_SYSTEM_PROGRAM_PATH /bin )


function(cmake_gui_list var vdefault ...)
  set(${var} ${vdefault} CACHE TYPE STRING)
    set( thelist ${ARGV} )
    list(REMOVE_AT thelist 0 )
  set_property(CACHE ${var} PROPERTY STRINGS ${thelist} )
endfunction()

# build type select
cmake_gui_list(CMAKE_BUILD_TYPE Debug Release RelWithDebInfo MinSizeRel)



