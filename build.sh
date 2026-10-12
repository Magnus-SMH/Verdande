#!/bin/bash

#Remember to grant permission before execution of the shell
#chmod +x script.sh

#Please read the script before executing :)

#Running build.sh from the project root
SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
cd "$SCRIPT_DIR" || exit

#Compiler flags
_FLAGS="-Wall -Wextra -Werror -Wshadow -Wconversion -Wpedantic \
        -fno-exceptions -lc -fno-stack-protector \
        -march=znver4 -mtune=znver4"
INFO="--- znver4 cpu architecture hardcoded"
_CPPFLAGS="-std=c++20 -fno-rtti -nodefaultlibs" #c++ 20 for designated initialisers

D_FLAGS="-O0 -g $_FLAGS $_CPPFLAGS"
R_FLAGS="-O3 $_FLAGS $_CPPFLAGS"

#File structure
_SRC="verdande"
SRC="$_SRC/verdande.cpp"
_APP="app"
APP="$_APP/linux_app.cpp"
LIB="libverdande.so"
EXE="appverdande"

#Directories
XDG_XML_PATH="/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml"
XDG_DIR="$_APP/xdg-shell"

BUILD_DIR="./build"
BUILD_DIR_XDG="$BUILD_DIR/xdg-shell-client-protocol.o"
PREV_FLAGS_FILE="$BUILD_DIR/.prev_flags"

mkdir -p "$BUILD_DIR"
mkdir -p "$XDG_DIR"

COMPILE_ONLY=0
GCC_RETURN=0

MODE=${1}
if [ "$MODE" = "--hot-reload" ] || [ "$MODE" = "-h" ]; then
    #Read previous flags to avoid hotreloading with a diffrent set of compiler flags
    PREV_FLAGS=$(cat $PREV_FLAGS_FILE)
    
    #Recompiling the SO with the flags used in the previous full compilation
    if [ "$PREV_FLAGS" = "$D_FLAGS" ] || [ "$PREV_FLAGS" = "$R_FLAGS" ]; then
        echo "--- Hot-reloading shared object"
        g++ $PREV_FLAGS -fPIC -shared $SRC -o "$BUILD_DIR/$LIB-temp" \
            || { GCC_RETURN=$?; GCC_RETURNER="Shared Object"; }
    
        if [ $GCC_RETURN -ne 0 ]; then
            echo -e "\n--- Failed hot-reload compilation of $GCC_RETURNER ($GCC_RETURN)"
        else
            mv "$BUILD_DIR/$LIB-temp" "$BUILD_DIR/$LIB"
            echo "--- Hot-reload complete"
        fi
    else
        echo "--- Did not find compilation flags from previous full compilation"
    fi
    exit 0
else
    if [ "$MODE" != "--build-only" ] && [ "$MODE" != "-b" ]; then
        clear
    fi
    echo "==============================================="
    
    #XDG client Protocol
    if [[ ! -f "$XDG_DIR/xdg-shell-client-protocol.h" || ! -f "$XDG_DIR/xdg-shell-client-protocol.c" ]]; then
        echo "--- Scanning and compiling xdg shell client protocol"
        wayland-scanner private-code  < "$XDG_XML_PATH" > "$XDG_DIR/xdg-shell-client-protocol.c"
        wayland-scanner client-header < "$XDG_XML_PATH" > "$XDG_DIR/xdg-shell-client-protocol.h"
        gcc -O3 $_FLAGS -c "$XDG_DIR/xdg-shell-client-protocol.c" -o $BUILD_DIR_XDG
        fi

    #Fresh Full Compilation
    if [ "$MODE" = "--release" ] || [ "$MODE" = "-r" ]; then
        FLAGS="$R_FLAGS"
        MODE_NAME="Release"
        echo $INFO
    elif [ $# -eq 0 ] || [ "$MODE" = "--debug" ] || [ "$MODE" = "-d" ]; then
        FLAGS="$D_FLAGS"
        MODE_NAME="Debug"
        echo $INFO
    elif [ "$MODE" = "--build-only" ] || [ "$MODE" = "-b" ]; then
        FLAGS="$D_FLAGS"
        MODE_NAME="Build-only"
        COMPILE_ONLY=1
    else
        echo -e "Unknown arg: $MODE"
        echo "--- Zero args will default to full debug compilation"
        echo "--- Currently only supports 1 or less args, case sensitive"
        echo "Options:"
        echo "  --release    | -r   Full compilation, using release build flags"
        echo "  --debug      | -d   Full compilation, using debug   build flags"
        echo "  --hot-reload | -h   Recompilation of just the shared object"
        echo "  --build-only | -b   Full compilation, using debug, dosent run"
        exit 1
    fi
    
    #Timed Compilation, with failed comp handeling
    echo "--- Full Compilation in $MODE_NAME-mode"
    START=$(date +%s%N)
    #SO; placed in the same dir as exe.
    g++ $FLAGS -fPIC -shared $SRC -o "$BUILD_DIR/$LIB" \
        || { GCC_RETURN=$?; GCC_RETURNER="Shared Object"; }
    
    if [ $GCC_RETURN -eq 0 ]; then
        #App; looking for .so in the same dir as exe, static linked to wayland, passing .so path as a build variable
        g++ $FLAGS $APP -o "$BUILD_DIR/$EXE" -Wl,-rpath,'$ORIGIN' -ldl \
            -lwayland-client -DSO_PATH="\"$BUILD_DIR/$LIB\"" $BUILD_DIR_XDG \
            || { GCC_RETURN=$?; GCC_RETURNER="Application"; }
    fi
    END=$(date +%s%N)
    
    #Runs the EXE if both compilations succeded
    if [ $GCC_RETURN -eq 0 ]; then
        #Stores compiler flags
        echo "$FLAGS" > $PREV_FLAGS_FILE 

        TIME_DIFF=$(( ($END - $START) / 1000000))
        SIZE_KB=$(du -sk "$BUILD_DIR" | cut -f1)
        echo "--- Build successful ($TIME_DIFF ms, $SIZE_KB kB)"
        if [ $COMPILE_ONLY -eq 0 ]; then
            echo -e "--- Running...\n"
            "$BUILD_DIR/$EXE"
            RUN_ERROR=$?
            if [ $RUN_ERROR -ne 0 ]; then
                echo -e "\n--- Runtime Error ($RUN_ERROR)"
            else
                echo
            fi
        fi
    else
        echo -e "\n--- Failed compilation. $GCC_RETURNER ($GCC_RETURN)"
    fi
    echo -e "--- Exiting script"
    exit 0
fi


