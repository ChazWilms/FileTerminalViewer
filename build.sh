#!/bin/bash
export PATH="/mingw64/bin:/c/Program Files/CMake/bin:$PATH"

PROJ="/c/Users/Chaz/Desktop/csProjects/FileTerminalViewer"
BUILD="$PROJ/build"

mkdir -p "$BUILD"
cd "$BUILD"

cmake "$PROJ" -G "MinGW Makefiles" \
  -DCMAKE_C_COMPILER=/mingw64/bin/gcc.exe \
  -DCMAKE_CXX_COMPILER=/mingw64/bin/g++.exe \
  -DCMAKE_MAKE_PROGRAM=/mingw64/bin/mingw32-make.exe \
  && mingw32-make -j4
