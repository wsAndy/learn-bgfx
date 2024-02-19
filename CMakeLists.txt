cmake_minimum_required( VERSION 3.5 )

project( EmptyDemo )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})


include(cmake/big2.cmake)

add_subdirectory(examples)