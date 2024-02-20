cmake_minimum_required( VERSION 3.5 )

project( EmptyDemo )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17") 

if(WIN32)
else()
add_compile_options(-Wno-c++98-compat -Wno-c++98-compat-pedantic)
endif()

set(ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include(cmake/big2.cmake)

add_subdirectory(examples)