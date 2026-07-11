cmake_minimum_required(VERSION 4.0)
project(TetrisCPP VERSION 0 LANGUAGES CXX)

set(TARGET_MAIN main)
set(SDL_TESTS OFF)
set(SDL_EXAMPLES OFF)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(${TARGET_MAIN}
    src/main.cpp
)

option(VENDORED_SDL "Use vendored SDL library" OFF)
if(VENDORED_SDL)
    if(NOT SDL3_FOUND AND NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendored/SDL/CMakeLists.txt")
        message(STATUS "SDL submodule missing. Fetching submodule via Git")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMODULE_RESULT
        )
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
        add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
    endif()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
else()
    find_package(SDL3 REQUIRED) # REQUIRED = outputs error; QUIET = does nothing. useful for downloading it dynamically
endif()

target_compile_options(${TARGET_MAIN} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -Wpedantic
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
    >
)
# target_compile_features(${TARGET_MAIN} PUBLIC ${needed_features})
target_link_libraries(${TARGET_MAIN} PRIVATE SDL3::SDL3)

# set_property(TARGET ${TARGET_MAIN} PROPERTY WIN32_EXECUTABLE TRUE)