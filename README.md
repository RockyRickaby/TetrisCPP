



# Tetris in C++

https://github.com/user-attachments/assets/2b11ee85-d743-4686-a7be-775e4c1a33bd

Yet another personal project of mine: a (overengineered) Tetris implementation (again)! The first time I made a Tetris clone it was rather... bad. I like to believe I have improved since then.

This project uses SDL3 and SDL3_Mixer as dependencies and can be built using CMake on Linux with Clang/GCC or on Windows with MSVC (MinGW and MacOS builds have not been tested).

The 3D assets actually used in the game were made by me. The 2D block texture was also made by me. The bitmap font was taken from the game Joust (NES), and the sounds/music were taken from Tengen Tetris (NES) and from Tetris by Nintendo R&D (NES). All rights reserved to their respective owners.

## Controls

- **Arrow keys**: move the piece
    - **Up key**: hard-drops the piece
    - **Down key**: soft-drops it (i.e. the piece will drop faster)
    - **Left and right keys**: moves the piece to the left and to the right
- **Spacebar**: hold the piece (i.e. save it for later)
    - Press it again to swap the held piece with the current one
- **ESC**: pauses the game
- **D key**: rotates the piece counter-clockwise
- **F key**: rotates the piece clockwise
- **Q key**: exits the game
- **Scroll wheel**: increase/decrease the game's volume (default: ~25%)

## Dependencies

If you're on a Linux distribution, you can install SDL3 and SDL3_Mixer through your package manager as long as it is up to date (reference point is Fedora 44,
and SDL3_Mixer requires SDL 3.4.0). If your distribution doesn't provide that (or in case you're on Windows), you can pull the submodules recursively (`git clone --recursive <url>` or `git submodule update --init --recursive` after cloning the repository normally) and compile everything together. Alternatively, the dependencies can be manually downloaded and added to the `vendored` directory (the folder names should
be `SDL` and `SDL_mixer`, case-sensitive). The dependencies might be detected and found automatically in either case (only if a `CMakeLists.txt` file is found in their respective folders).

## Building and Running

This project is built using CMake. Assuming all the dependnecies are installed, you can build it like this:

```sh
$ cmake -B build 
$ cmake --build build
```

The following flags can be set in order to enable/disable looking up for dependencies inside the `vendored` directory:

- **VENDORED_SDL** (Example: `cmake -B build -DVENDORED_SDL=OFF`)
- **VENDORED_SDL_MIXER** (Example: `cmake -B build -DVENDORED_SDL_MIXER=ON`)

I generally prefer to generate the project like this:

```sh
$ cmake -B build -G "Ninja" -DCMAKE_CXX_COMPILER=clang++
```

To run the app, run the executable inside the build folder (which will likely be inside of another folder according to the `CMAKE_BUILD_TYPE`).

```sh
# Example, -DCMAKE_BUILD_TYPE=Debug
$ ./build/Debug/main
```
