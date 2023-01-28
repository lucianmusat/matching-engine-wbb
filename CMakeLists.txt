cmake_minimum_required(VERSION 3.22)
project(MatchingEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

add_executable(${PROJECT_NAME}
                include/main.hpp
                src/main.cpp
                src/matching_engine.cpp
                src/order.cpp
                include/string_utils.hpp
                include/matching_engine.hpp
            )

target_include_directories(${PROJECT_NAME} PUBLIC include)

add_subdirectory(test)
