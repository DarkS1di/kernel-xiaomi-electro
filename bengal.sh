#!/bin/bash
set -e

mkdir -p out/arch/arm64/boot/bengal/
mv out/arch/arm64/boot/dtb out/arch/arm64/boot/bengal/dtb
mv out/arch/arm64/boot/dtbo.img out/arch/arm64/boot/bengal/dtbo.img
