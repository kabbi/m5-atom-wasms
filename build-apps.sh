#!/bin/bash

CURR_DIR=$PWD
OUT_DIR=${PWD}/wasm
WASM_APPS=${PWD}/wasm

cd ${WASM_APPS}

for i in `ls *.c`
do
APP_SRC="$i"
OUT_FILE=${i%.*}.wasm

# use WAMR SDK to build out the .wasm binary
/opt/wasi-sdk/bin/clang     \
        -O3 -z stack-size=512 -Wl,--initial-memory=65536 \
        --sysroot=${WAMR_DIR}/wamr-sdk/app/libc-builtin-sysroot  \
        -Wl,--strip-all,--no-entry -nostdlib \
        -Wl,--export=on_init \
        -Wl,--export=on_timer \
        -Wl,--export=on_button \
        -Wl,--export=on_destroy \
        -Wl,--allow-undefined \
        -o ${OUT_DIR}/${OUT_FILE} ${APP_SRC}

if [ -f ${OUT_DIR}/${OUT_FILE} ]; then
        echo "build ${OUT_FILE} success"
else
        echo "build ${OUT_FILE} fail"
fi
done
