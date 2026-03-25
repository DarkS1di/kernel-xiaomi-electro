#!/bin/bash
set -e

rm -f out/arch/arm64/boot/dtbo.img
find out/arch/arm64/boot/dts/vendor/qcom/ -name "*.dtb" -type f -delete
find out/arch/arm64/boot/dts/vendor/qcom/ -name "*.dtbo" -type f -delete

./scripts/config --file out/.config --enable CONFIG_BOARD_XIAOMI
./scripts/config --file out/.config --disable CONFIG_BOARD_CITRUS
./scripts/config --file out/.config --enable CONFIG_BOARD_LIME

export ARCH=arm64

PATH=$PWD/toolchain/clang/bin:$PATH

make O=out CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 LLVM_IAS=1 olddefconfig
make O=out CROSS_COMPILE=aarch64-linux-gnu- LLVM=1 LLVM_IAS=1 dtbo.img -j$(nproc)

mkdir -p out/arch/arm64/boot/lime/
mv out/arch/arm64/boot/dtbo.img out/arch/arm64/boot/lime/dtbo.img
