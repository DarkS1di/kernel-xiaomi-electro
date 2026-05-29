#!/bin/bash
set -e

CLANG="ZyC Clang 23.0.0"

URL="https://github.com/ZyCromerZ/Clang/releases/download/23.0.0git-20260130-release/Clang-23.0.0git-20260130.tar.gz"

if [ ! -e "toolchain" ]; then
    echo "mkdir toolchain"
    mkdir toolchain
elif [ ! -d "toolchain" ]; then
    echo "$(pwd)/toolchain is not a directory"
    exit 1
fi

echo "Setting up ZyC Clang in $(pwd)/toolchain/clang"
cd toolchain

if [ -d "clang" ]; then
    echo "Removing old clang directory..."
    rm -rf clang
fi

mkdir clang
cd clang

echo "Downloading $CLANG..."
wget -q "$URL" -O clang.tar.gz

echo "Extracting..."
tar -zxf clang.tar.gz

rm clang.tar.gz

echo "Done. Verifying version:"
./bin/clang --version
