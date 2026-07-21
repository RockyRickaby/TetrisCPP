cmake_minimum_required(VERSION 4.0)
project(TetrisCPP VERSION 0 LANGUAGES CXX)

set(TARGET_MAIN main)
set(SDL_TESTS OFF)
set(SDL_EXAMPLES OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(${TARGET_MAIN}
    src/main.cpp
    src/tetris_utils.cpp
    src/pieces.cpp
    src/base.cpp
    src/kicks.cpp
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendored/SDL/CMakeLists.txt")
    set(VENDORED_SDL_EXISTS ON)
else()
    message(WARNING "SDL submodule NOT found")
    set(VENDORED_SDL_EXISTS OFF)
endif()
# use vendored version by default if it exists unless defined otherwise. avoid problems if the package isn't available
option(VENDORED_SDL "Use vendored SDL library" ${VENDORED_SDL_EXISTS})

if(VENDORED_SDL)
    message(STATUS "Using vendored SDL")
    if(NOT VENDORED_SDL_EXISTS)
        # find_package(Git REQUIRED)
        message(FATAL_ERROR "Vendored SDL not found. Grab a copy of the source code and save it to vendored/SDL or fetch the submodule via Git (run 'git submodule update --init --recursive')")
        # message(STATUS "Vendored SDL not found. Fetching submodule via Git")
        # execute_process(
        #     COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        #     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        #     RESULT_VARIABLE GIT_SUBMODULE_RESULT
        # )
    endif()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
else()
    message(STATUS "Using packaged SDL")
    find_package(SDL3 REQUIRED CONFIG) # REQUIRED = outputs error; QUIET = does nothing. useful for downloading it dynamically
endif()

target_compile_options(${TARGET_MAIN} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -Wpedantic -fdiagnostics-color=always
    >
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)
# target_compile_features(${TARGET_MAIN} PUBLIC ${needed_features})
target_link_libraries(${TARGET_MAIN} PRIVATE SDL3::SDL3)

# set_property(TARGET ${TARGET_MAIN} PROPERTY WIN32_EXECUTABLE TRUE)