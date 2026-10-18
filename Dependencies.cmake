# Manually set one possible path to the SDL3 library on Windows systems
# edit these to correspond to the actual SDL directory on your computer
set(WIN_SDL3_CPATH "C:/SDL/SDL3")
set(WIN_SDL3_mixer_CPATH "C:/SDL/SDL3_mixer")

if (WIN32)
    list(APPEND CMAKE_PREFIX_PATH ${WIN_SDL3_CPATH} ${WIN_SDL3_mixer_CPATH})
endif()



if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendored/SDL/CMakeLists.txt")
    set(VENDORED_SDL_EXISTS ON)
else()
    message(STATUS "SDL submodule NOT found.")
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
    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
else()
    message(STATUS "Using packaged SDL")
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared) # REQUIRED = outputs error; QUIET = does nothing. useful for downloading it dynamically
endif()




# try linking against sdl_mixer
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendored/SDL_mixer/CMakeLists.txt")
    set(VENDORED_SDL_mixer_EXISTS ON)
else()
    message(STATUS "SDL_mixer submodule NOT found.")
    set(VENDORED_SDL_mixer_EXISTS OFF)
endif()

option(VENDORED_SDL_MIXER "Use vendored SDL_mixer library" ${VENDORED_SDL_mixer_EXISTS})

set(SDLMIXER_MIDI_NATIVE OFF)
set(SDLMIXER_GME OFF)
set(SDLMIXER_WAVPACK OFF)     
set(SDLMIXER_MOD OFF)
set(SDLMIXER_MOD_XMP OFF)
set(SDLMIXER_OPUS OFF)
set(SDLMIXER_MP3_MPG123 OFF)
set(SDLMIXER_FLAC_LIBFLAC OFF)
set(SDLMIXER_VORBIS_VORBISFILE ON)

if(VENDORED_SDL_MIXER)
    message(STATUS "Using vendored SDL_mixer")
    if(NOT VENDORED_SDL_mixer_EXISTS)
        # git submodule update --init --recursive
        message(FATAL_ERROR "Vendored SDL_mixer not found. Grab a copy of the source code and save it to vendored/SDL_mixer or fetch the submodule via Git (run 'git submodule update --init --recursive')")
    endif()
    add_subdirectory(vendored/SDL_mixer EXCLUDE_FROM_ALL)
else()
    message(STATUS "Using packaged SDL_mixer")
    find_package(SDL3_mixer REQUIRED CONFIG REQUIRED COMPONENTS SDL3_mixer-shared) # REQUIRED = outputs error; QUIET = does nothing. useful for downloading it dynamically
endif()