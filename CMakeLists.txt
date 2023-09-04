cmake_minimum_required(VERSION 3.26)
project(KVengine)

set(CMAKE_CXX_STANDARD 11)

add_executable(KVengine skiplist.h ThreadPool.h main.cpp)
