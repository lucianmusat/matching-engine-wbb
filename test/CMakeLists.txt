cmake_minimum_required(VERSION 3.22)
project(MatchingEngineTest)

enable_testing()

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
               ${CMAKE_SOURCE_DIR}/include/matching_engine.hpp
               ${CMAKE_SOURCE_DIR}/src/matching_engine.cpp
               ${CMAKE_SOURCE_DIR}/include/order.hpp
               ${CMAKE_SOURCE_DIR}/src/order.cpp
               test.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} PRIVATE Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
include(CTest)
include(Catch)

# Add a last build step to run the unittests
set(UNIT_TEST ${PROJECT_NAME})
add_custom_command(
        TARGET ${UNIT_TEST}
        COMMENT "Run tests"
        POST_BUILD
        COMMAND ${UNIT_TEST}
)