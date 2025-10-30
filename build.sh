#/bin/bash
set -e

if [ ! -e "packaging/packaging/pack.sh" ]; then
    echo "Error: pack.sh not found!"
    echo "Make pack.sh available at packaging/packaging/pack.sh"
    exit 1
fi

if [ ! -e "toolchain" ]; then
    echo "Error: toolchain not found!"
    echo "Make toolchain avaliable at $(pwd)/toolchain"
    exit 1
fi

export KBUILD_BUILD_USER=darks1di
export KBUILD_BUILD_HOST=github_actions
export ARCH=arm64
PATH=$PWD/toolchain/bin:$PATH

rm -rf out
make O=out CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 -j$(nproc) vendor/chime_defconfig
make O=out CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 -j$(nproc)
