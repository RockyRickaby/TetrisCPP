cmake_minimum_required(VERSION 4.0)
project(TetrisCPP VERSION 0 LANGUAGES CXX)

set(TARGET_MAIN main)
set(PROJECT_SOURCES
    src/main.cpp
    src/tetris/tetris_game.cpp
    src/tetris/tetris_menu.cpp
    src/tetris/tetris_pieces.cpp
    src/tetris/tetris_kicks.cpp
    src/tetris/tetris_utils.cpp
    src/tetris/tetris_bags.cpp
    src/tetris/tetris_input.cpp
    src/tetris/tetris_state_machines/tetris_game_sm.cpp
    src/engine/tengine.cpp
    src/engine/state_machine.cpp
    src/engine/text/text_renderer.cpp
    src/engine/text/fonts.cpp
    src/engine/sprites/sprites.cpp
)
# Manually set the path to the SDL3 library on Windows systems
set(WIN_SDL3_PATH "C:\\SDL3")

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT WIN32)
    add_executable(${TARGET_MAIN})
else()
    add_executable(${TARGET_MAIN} WIN32)
endif()
target_sources(${TARGET_MAIN} PRIVATE ${PROJECT_SOURCES})
    
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendored/SDL/CMakeLists.txt")
    set(VENDORED_SDL_EXISTS ON)
else()
    message(STATUS "SDL submodule NOT found. Assuming a package is available")
    set(VENDORED_SDL_EXISTS OFF)
endif()
# use vendored version by default if it exists unless defined otherwise. avoid problems if the package isn't available
option(VENDORED_SDL "Use vendored SDL library" ${VENDORED_SDL_EXISTS})

if(VENDORED_SDL)
    message(STATUS "Using vendored SDL")
    if(NOT VENDORED_SDL_EXISTS)
        # git submodule update --init --recursive
        message(FATAL_ERROR "Vendored SDL not found. Grab a copy of the source code and save it to vendored/SDL or fetch the submodule via Git (run 'git submodule update --init --recursive')")
    endif()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
else()
    message(STATUS "Using packaged SDL")
    # TODO - TEST THIS LATER
    if (WIN32)
        list(APPEND CMAKE_PREFIX_PATH ${WIN_SDL3_PATH})
        find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared) # REQUIRED = outputs error; QUIET = does nothing. useful for downloading it dynamically
        # target_include_directories(${TARGET_MAIN} PRIVATE ${WIN_SDL3_PATH}/include)
        # target_include_directories(${TARGET_MAIN} PRIVATE ${WIN_SDL3_PATH}/lib)
    else()
        find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared) # REQUIRED = outputs error; QUIET = does nothing. useful for downloading it dynamically
    endif()
endif()

set_target_properties(${TARGET_MAIN} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    # OUTPUT_NAME "TPP"
)

target_compile_options(${TARGET_MAIN} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -Wpedantic -fdiagnostics-color=always
    >
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# target_compile_features(${TARGET_MAIN} PUBLIC ${needed_features})
target_link_libraries(${TARGET_MAIN} PRIVATE SDL3::SDL3)

if(WIN32)
    add_custom_command(TARGET ${TARGET_MAIN} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${TARGET_MAIN}>
            $<TARGET_FILE_DIR:${TARGET_MAIN}>
        COMMAND_EXPAND_LISTS
    )
endif()

add_custom_command(TARGET ${TARGET_MAIN} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${TARGET_MAIN}>/assets
    COMMENT "Copying assets to output directory"
)