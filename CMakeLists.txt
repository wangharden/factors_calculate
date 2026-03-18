cmake_minimum_required(VERSION 3.10)
project(limit_up_factor_demo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(limit_up_factor_demo main.cpp
        factor.h
        factor001.cpp
        factor_collection.h)
