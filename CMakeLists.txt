cmake_minimum_required(VERSION 4.0)
project(TetrisCPP VERSION 0 LANGUAGES CXX C) # CXX is this project, C is SDL3

set(TARGET_MAIN main)
set(PROJECT_SOURCES
    src/main.cpp
    src/appstate.cpp
    src/tetris/tetris_game.cpp
    src/tetris/tetris_menu.cpp
    src/tetris/tetris_pieces.cpp
    src/tetris/tetris_piece3d.cpp
    src/tetris/tetris_kicks.cpp
    src/tetris/tetris_utils.cpp
    src/tetris/tetris_bags.cpp
    src/tetris/tetris_input.cpp
    src/tetris/tetris_scoreboard.cpp
    src/tetris/tetris_state_machines/tetris_game_sm.cpp
    src/engine/tengine.cpp
    src/engine/texture.cpp
    src/engine/sound/audio.cpp
    src/engine/shapes/shapes.cpp
    src/engine/silly3D/silly_3D.cpp
    src/engine/state_machine.cpp
    src/engine/text/text_renderer.cpp
    src/engine/text/fonts.cpp
    src/engine/sprites/sprites.cpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

include(Dependencies.cmake)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT WIN32)
    add_executable(${TARGET_MAIN})
else()
    add_executable(${TARGET_MAIN} WIN32)
endif()
target_sources(${TARGET_MAIN} PRIVATE ${PROJECT_SOURCES})

set_target_properties(${TARGET_MAIN} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    # OUTPUT_NAME "TPP"
)

# TODO - this could be simplified with a simple if-else. would make this easier to read
target_compile_options(${TARGET_MAIN} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall -Wextra -Wpedantic -fdiagnostics-color=always
    >
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /MP>
)

if(WIN32)
    add_custom_command(TARGET ${TARGET_MAIN} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${TARGET_MAIN}>
            $<TARGET_FILE_DIR:${TARGET_MAIN}>
        COMMAND_EXPAND_LISTS
    )
else()
endif()

target_link_libraries(${TARGET_MAIN} PRIVATE SDL3_mixer::SDL3_mixer SDL3::SDL3)

add_custom_command(TARGET ${TARGET_MAIN} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${TARGET_MAIN}>/assets
    COMMENT "Copying assets to output directory"
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT "${EXECUTABLE_NAME}")