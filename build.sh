#!/bin/bash
set -e

if [ ! -e "toolchain/clang" ]; then
    echo "Error: clang not found!"
    echo "Make clang avaliable at $(pwd)/toolchain/clang"
    exit 1
fi

if [ ! -e "toolchain/packaging" ]; then
    echo "Error: packaging not found!"
    echo "Make packaging available at $(pwd)/toolchain/packaging"
    exit 1
fi

export KBUILD_BUILD_USER="NoxS1d"
export KBUILD_BUILD_HOST="GitHub Actions"
export ARCH="arm64"

PATH=$PWD/toolchain/clang/bin:$PATH

rm -rf out
make O=out CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 LLVM_IAS=1 -j$(nproc) vendor/chime_defconfig
make O=out CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 LLVM_IAS=1 -j$(nproc)
