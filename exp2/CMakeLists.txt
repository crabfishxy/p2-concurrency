cmake_minimum_required(VERSION 2.8.9)

project(exp2)

## toggle this to use vtune itt notify instrumentation
SET(USE_VTUNE "1") 
SET(USE_INIT_ELEMENTS "0")

# -- apply to all configurations --- #
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -g -Wall -Wextra -Werror -pthread -O2")
#SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
SET(CMAKE_C_COMPILER "gcc")

include_directories(
  ./
  /usr/include/glib-2.0
  /usr/lib/x86_64-linux-gnu/glib-2.0/include
)

set(COMMON_LIB
    dl
    )
  
if (USE_VTUNE)
    include_directories(
            /data/intel/vtune_profiler/include   # for vtune instrumentation
    )
    
    ADD_LIBRARY(ittnotify STATIC IMPORTED)
    SET_TARGET_PROPERTIES(ittnotify PROPERTIES IMPORTED_LOCATION /data/intel/vtune_profiler/lib64/libittnotify.a)
    SET (COMMON_LIB ittnotify ${COMMON_LIB})
    
    add_definitions(-DUSE_VTUNE)    
endif()

if (USE_INIT_ELEMENTS)
  add_definitions(-DUSE_INIT_ELEMENTS)
endif()

add_definitions(-DDEBUG)

file(GLOB HEADERS
"./*.h"
)

file(GLOB COMMON_SOURCES
  ${HEADERS}
  measure.c 
  common.c)

# ------------------------------------- #
# all executables  
# ------------------------------------- #

# biglock
add_executable(hashtable
  hashtable.c
  ${COMMON_SOURCES}
)

TARGET_LINK_LIBRARIES(hashtable
        ${COMMON_LIB}
        )      

# ------------------------------------- #
add_executable(hashtable-m
  hashtable.c
  ${COMMON_SOURCES}
)
set_target_properties(hashtable-m PROPERTIES COMPILE_FLAGS "-DUSE_MULTITABLES" )
TARGET_LINK_LIBRARIES(hashtable-m
        ${COMMON_LIB}
        )        


# ------------------------------------- #
#add_executable(list9
#  list9.c
#  ${COMMON_SOURCES}
#)

#TARGET_LINK_LIBRARIES(list9
#        ${COMMON_LIB}
#        )        
# ------------------------------------- #
