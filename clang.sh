#!/bin/bash
set -e

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

URL="https://github.com/ZyCromerZ/Clang/releases/download/20.0.0git-20250129-release/Clang-20.0.0git-20250129.tar.gz"

echo "Downloading ZyC Clang 20.0.0git..."
wget -q "$URL" -O clang.tar.gz

echo "Extracting..."
tar -zxf clang.tar.gz

rm clang.tar.gz

echo "Done. Verifying version:"
./bin/clang --version
