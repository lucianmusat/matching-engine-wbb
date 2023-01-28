cmake_minimum_required(VERSION 3.22)
project(MatchingEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.0.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(${PROJECT_NAME}
                include/main.hpp
                src/main.cpp
                src/matching_engine.cpp
                src/order.cpp
                include/string_utils.hpp
                include/matching_engine.hpp
                test/test.cpp
            )

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(MatchingEngine PRIVATE Catch2::Catch2WithMain)

include(Catch)