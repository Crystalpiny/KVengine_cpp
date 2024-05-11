cmake_minimum_required(VERSION 3.26)
project(KVengine VERSION 1.0.0 DESCRIPTION "A Key-Value Engine using Skip List" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置 Boost 库的路径
set(BOOST_ROOT "C:/SoftWare/boost_1_84_0")
set(Boost_USE_STATIC_LIBS ON) # 启用静态库
set(Boost_USE_MULTITHREADED ON) # 启用多线程
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost REQUIRED) # 查找 Boost 库

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) # 添加 Boost 头文件的搜索路径
endif()

# 添加 progressbar 库的头文件搜索路径
include_directories(include/progressbar)
# 添加 RapidJSON 库的头文件搜索路径
include_directories(include/rapidjson)
# 添加 ConfigUpdater 库的头文件搜索路径
include_directories(ConfigUpdater)

set(HEADERS
        skiplist.h
        ThreadPool.h
        benchmark.h
        ctpl_stl.h
        ConfigUpdater/ConfigUpdater.h
        JsonTest.h
        logMod.h
)

set(SOURCES
        main.cpp
        benchmark.cpp
        ConfigUpdater/ConfigUpdater.cpp
        JsonTest.cpp
)

add_executable(KVengine ${SOURCES} ${HEADERS})

if(Boost_FOUND)
    target_link_libraries(KVengine ${Boost_LIBRARIES}) # 链接 Boost 库
endif()