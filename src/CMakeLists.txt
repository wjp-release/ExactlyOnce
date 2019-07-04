cmake_minimum_required (VERSION 3.1)

project (ExactlyOnce)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "-fPIC -std=c++11 -Wall")

include_directories(./)

aux_source_directory(./utest utest_src)

add_executable(utest ${utest_src})