cmake_minimum_required(VERSION 3.10)
project(limit_up_factor_demo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(limit_up_factor_demo main.cpp
        factor.h
        factor_result.h
        cross_section_utils.h
        factor01_031_state.h
        factor01_031_stream.h
        factor01_031_extract.h
        factor032_040_extract.h
        trade_vwap_utils.h
        factor_collection.h)
