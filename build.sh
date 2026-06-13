#!/bin/bash

#Remember to grant permission before execution of the shell
#chmod +x script.sh

#Please read the script before executing :)

SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
cd "$SCRIPT_DIR" || exit

XDG_XML_PATH="/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml"
XDG_DIR="app/xdg-shell"
mkdir -p "$XDG_DIR"

BUILD_DIR="./build"
BUILD_DIR_XDG="$BUILD_DIR/xdg-shell-client-protocol.o"
PREV_FLAGS_FILE="$BUILD_DIR/.prev_flags"

mkdir -p "$BUILD_DIR"

_FLAGS="-Wall -Wextra -Werror -Wshadow -Wconversion -Wpedantic \
        -fno-exceptions -lc -fno-stack-protector \
        -lwayland-client \
        -march=znver4 -mtune=znver4"
#c++ 20 for designated initialisers
_CPPFLAGS="-std=c++20 -fno-rtti -nodefaultlibs"

D_FLAGS="-O0 -g $_FLAGS $_CPPFLAGS"
R_FLAGS="-O3 $_FLAGS $_CPPFLAGS"

SRC="Verdande/Verdande.cpp"
APP="app/main.cpp"
LIB="libVerdande.so"
EXE="Verdande"

GCC_RETURN=0

MODE=${1}
if [ "$MODE" = "--hot-reload" ] || [ "$MODE" = "-h" ]; then
    #Read previous flags to avoid hotreloading with a diffrent flags
    PREV_FLAGS=$(cat $PREV_FLAGS_FILE)
    
    #Recompiling the SO with the flags used in the previous full compilation
    if [ "$PREV_FLAGS" = "$D_FLAGS" ] || [ "$PREV_FLAGS" = "$R_FLAGS" ]; then
        echo "--- Hot-reloading shared object" 
        g++ $PREV_FLAGS -fPIC -shared $SRC -o "$BUILD_DIR/$LIB" || { GCC_RETURN=$?; GCC_RETURNER="Shared Object"; }
    
        if [ $GCC_RETURN -ne 0 ]; then
            echo -e "\n--- Failed hot-reload compilation of $GCC_RETURNER ($GCC_RETURN)"
        else
            echo "--- Hot-reload complete"
        fi
    else
        echo "--- Did not find compilation flags from previous full compilation"
    fi
    exit 0
else
    clear 
    INFO="--- znver4 cpu architecture hardcoded"

    #XDG client Protocol
    if [[ ! -f "$XDG_DIR/xdg-shell-client-protocol.h" || ! -f "$XDG_DIR/xdg-shell-client-protocol.c" ]]; then   
        wayland-scanner private-code  < "$XDG_XML_PATH" > "$XDG_DIR/xdg-shell-client-protocol.c"
        wayland-scanner client-header < "$XDG_XML_PATH" > "$XDG_DIR/xdg-shell-client-protocol.h"
        gcc -O3 $_FLAGS -c "$XDG_DIR/xdg-shell-client-protocol.c" -o $BUILD_DIR_XDG
        echo "--- Scanning and compiling xdg shell client protocol"
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
    else
        echo -e "Unknown arg: $MODE"
        echo "--- Zero args will default to full debug compilation"
        echo "--- Currently only supports 1 or less args, case sensitive"
        echo "Options:"
        echo "  --release    | -r   Full compilation, using release build flags"
        echo "  --debug      | -d   Full compilation, using debug   build flags"
        echo "  --hot-reload | -h   Recompilation of just the shared object"
        exit 1
    fi
    
    #Timed Compilation
    echo "--- Full Compilation in $MODE_NAME-mode"
    START=$(date +%s%N)
    g++ $FLAGS -fPIC -shared $SRC -o "$BUILD_DIR/$LIB" || { GCC_RETURN=$?; GCC_RETURNER="Shared Object"; }
    
    if [ $GCC_RETURN -eq 0 ]; then 
        g++ $FLAGS -DSO_PATH="\"$BUILD_DIR/$LIB\"" $APP $BUILD_DIR_XDG -o "$BUILD_DIR/$EXE" -Wl,-rpath,'$ORIGIN' -ldl || { GCC_RETURN=$?; GCC_RETURNER="Application"; }
    fi
    END=$(date +%s%N)
    
    #Runs the EXE if both compilations succeded
    if [ $GCC_RETURN -eq 0 ]; then
        echo "$FLAGS" > $PREV_FLAGS_FILE 

        TIME_DIFF=$(( ($END - $START) / 1000000))
        SIZE_KB=$(du -sk "$BUILD_DIR" | cut -f1)
        echo "--- Build successful ($TIME_DIFF ms, $SIZE_KB kB)"
        echo -e "--- Running...\n"
        "$BUILD_DIR/$EXE"
        RUN_ERROR=$?
        if [ $RUN_ERROR -ne 0 ]; then
            echo -e "\n--- Runtime Error ($RUN_ERROR)"
        else
            echo
        fi
    else
        echo -e "\n--- Failed compilation. $GCC_RETURNER ($GCC_RETURN)"
    fi
    echo -e "--- Exiting script"
    exit 0
fi


