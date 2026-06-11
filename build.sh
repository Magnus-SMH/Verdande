#!/bin/bash

#Remember to grant permission before execution of the shell
#sudo chmod +x script.sh

BUILD_DIR="./build"
PREV_FLAGS_FILE="$BUILD_DIR/.prev_flags"

mkdir -p "$BUILD_DIR"

_FLAGS="-std=c++23 -Wall -Wextra -Werror -Wshadow -Wconversion -Wpedantic \
        -fno-rtti -fno-exceptions -nodefaultlibs -lc -fno-stack-protector \
        -lwayland-client \
        -march=znver4 -mtune=znver4"

D_FLAGS="-O0 -g $_FLAGS"
R_FLAGS="-O3 $_FLAGS"

SRC="Verdande/Verdande.cpp"
APP="main.cpp"
LIB="libVerdande.so"
EXE="Verdande"

GCC_RETURN=0

MODE=${1}
if [ "$MODE" = "--hot-reload" ] || [ "$MODE" = "-h" ]; then
    #Read previous flags to avoid hotreloading with a diffrent flags
    PREV_FLAGS=$(cat $PREV_FLAGS_FILE)
    
    #Recompiling with the flags used in the previous full compilation
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
        g++ $FLAGS -DSO_PATH="\"$BUILD_DIR/$LIB\"" $APP -o "$BUILD_DIR/$EXE" -Wl,-rpath,'$ORIGIN' -ldl || { GCC_RETURN=$?; GCC_RETURNER="Application"; }
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


