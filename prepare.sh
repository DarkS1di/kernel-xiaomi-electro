#!/bin/bash
set -e

print_usage() {
    cat << EOF
Usage: ./prepare.sh [KERNEL_VARIANT] [PATCH_VERSION] [FLAGS...]

KERNEL_VARIANT:
  default       Vanilla
  ksu           KernelSU

PATCH_VERSION:
  Any string/number to define the patch level

FLAGS:
  extras        Enable KernelSU Extras
EOF
}

if [ $# -lt 2 ] || [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]]; then
    print_usage
    exit 1
fi

KERNEL_VARIANT=$1
PATCH_VERSION=$2

shift 2 

DEFAULT=false
KSU=false
KSU_EXTRAS=false
SUFFIX=""
UNKNOWN_ARGS=()

if [[ "$KERNEL_VARIANT" == "ksu" ]]; then
    KSU=true
    SUFFIX="ksu"
elif [[ "$KERNEL_VARIANT" == "default" ]]; then
    DEFAULT=true
    SUFFIX="default"
else
    echo "ERROR: Unknown first argument."
    echo "Run './prepare.sh --help' for a list of valid first argument."
    exit 1
fi

for arg in "$@"; do
    case $arg in
        extras)
            KSU_EXTRAS=true
            ;;
        *)
            UNKNOWN_ARGS+=("$arg")
            ;;
    esac
done

if [ ${#UNKNOWN_ARGS[@]} -gt 0 ]; then
    echo "ERROR: Unknown flags."
    echo "Run './prepare.sh --help' for a list of valid flags."
    exit 1
fi

if [ "$KSU_EXTRAS" = true ] && [ "$KSU" = false ]; then
    echo "ERROR: The 'extras' flag is only valid when the base variant is 'ksu'."
    exit 1
fi

ACTIVE_FLAGS=()
if [ "$KSU_EXTRAS" = true ]; then
    ACTIVE_FLAGS+=("extras")
fi

if [ ${#ACTIVE_FLAGS[@]} -gt 0 ]; then
    echo "Variant: $KERNEL_VARIANT, Patch: $PATCH_VERSION, Flags: ${ACTIVE_FLAGS[*]}."
else
    echo "Variant: $KERNEL_VARIANT, Patch: $PATCH_VERSION."
fi

echo "Preparing Build Environment."

if [ "$KSU" = true ]; then
    echo "Checkout KernelSU Patches"
    git clone --depth=1 -b main https://github.com/NoxS1d-Dev/ksu-kernel-4.19.git toolchain/kernelsu

    echo "Apply KernelSU Patches"
    cp ./toolchain/kernelsu/kernel_patches/add_ksu_in_kernel-4.19.patch ./
    patch -p1 --verbose < add_ksu_in_kernel-4.19.patch
    echo "KernelSU Patches Applied Successfully."

    echo "Setup KernelSU"
    curl -LSs "https://raw.githubusercontent.com/backslashxx/KernelSU/master/kernel/setup.sh" | bash -

    if [ "$KSU_EXTRAS" = true ]; then
        echo "Apply KernelSU Extras Patches"
        cp ./toolchain/kernelsu/kernel_patches/extras/add_ksu_extras_in_kernel-4.19.patch ./
        patch -p1 --verbose < add_ksu_extras_in_kernel-4.19.patch
        echo "KernelSU Extras Patches Applied Successfully."
    fi

elif [ "$DEFAULT" = true ]; then
    :
fi

echo "Preparation Complete."
