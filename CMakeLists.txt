cmake_minimum_required(VERSION 3.3)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

add_executable(gm-xg-gs
  sources/main.cc)
