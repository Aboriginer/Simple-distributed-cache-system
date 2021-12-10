cmake_minimum_required(VERSION 3.0)

PROJECT(Clint)

INCLUDE_DIRECTORIES(include)

AUX_SOURCE_DIRECTORY(src DIR_SRCS)

SET(CMAKE_CXX_STANDARD 11)

add_executable(client ${DIR_SRCS})