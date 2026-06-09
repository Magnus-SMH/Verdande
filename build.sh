#!/bin/bash
set -e

BUILD_DIR="./build"
mkdir -p "$BUILD_DIR"

_FLAGS="-std=c++23 -Wall -Wextra -Werror -Wshadow -Wconversion -Wpedantic \
        -fno-rtti -fno-exceptions -nodefaultlibs -lc -fno-stack-protector" 
D_FLAGS="-O0 -g $_FLAGS"
R_FLAGS="-O3 $_FLAGS"

SRC="Verdande/Verdande.cpp"
APP="main.cpp"
LIB="libVerdande.so"
EXE="Verdande"

MODE=${1:-debug}
[ "$MODE" = "release" ] && FLAGS="$R_FLAGS" || FLAGS="$D_FLAGS"

echo "Compiling $SRC in $MODE-mode"

g++ $FLAGS -fPIC -shared $SRC -o "$BUILD_DIR/$LIB"

g++ $FLAGS $APP -o "$BUILD_DIR/$EXE" -Wl,-rpath,'$ORIGIN' -ldl

echo "Build successful"

"$BUILD_DIR/$EXE"
