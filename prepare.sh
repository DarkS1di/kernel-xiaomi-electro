#!/bin/bash
set -e

KERNEL_NAME="ElectroX"
DATE=$(date +"%y.%m")

DEFCONFIG="arch/arm64/configs/vendor/chime_defconfig"

REPO_PACKAGING_TOOLS="https://github.com/NoxS1d-Dev/kernel-packaging.git"
REPO_KERNELSU_PATCHES="https://github.com/NoxS1d-Dev/ksu-kernel-4.19.git"

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
VARIANT_DISPLAY=""
UNKNOWN_ARGS=()

if [[ "$KERNEL_VARIANT" == "ksu" ]]; then
    KSU=true
    VARIANT_DISPLAY="KernelSU"
elif [[ "$KERNEL_VARIANT" == "default" ]]; then
    DEFAULT=true
    VARIANT_DISPLAY="Default"
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
echo ""

echo "Checkout Packaging Tools"
rm -rf toolchain/packaging
git clone --depth=1 -b kernel-packaging $REPO_PACKAGING_TOOLS toolchain/packaging
echo ""

echo "Removing Appended Configs in $DEFCONFIG"
if grep -q "# KernelSU" "$DEFCONFIG"; then
    echo "Appended configs found"
    sed -i '/# KernelSU/,$d' "$DEFCONFIG"
    sed -i '$d' "$DEFCONFIG"
else
    echo "Appended configs not found"
fi
echo ""

if [ "$KSU" = true ]; then
    echo "Checkout KernelSU Patches"
    rm -rf toolchain/kernelsu
    git clone --depth=1 -b main $REPO_KERNELSU_PATCHES toolchain/kernelsu
    echo ""

    echo "Apply KernelSU Patches"
    patch -p1 --verbose < toolchain/kernelsu/kernel_patches/add_ksu_in_kernel-4.19.patch
    echo ""

    if [ "$KSU_EXTRAS" = true ]; then
        echo "Apply KernelSU Extras Patches"
        patch -p1 --verbose < toolchain/kernelsu/kernel_patches/extras/add_ksu_extras_in_kernel-4.19.patch
        echo ""
    fi

    echo "Apply Configs"
    cat <<EOF >> "$DEFCONFIG"

#
# KernelSU
#
CONFIG_KSU=y
$(if [ "$KSU_EXTRAS" = true ]; then echo "CONFIG_KSU_EXTRAS=y"; else echo "# CONFIG_KSU_EXTRAS is not set"; fi)
CONFIG_KSU_KPROBES_KSUD=y
# CONFIG_KSU_TAMPER_SYSCALL_TABLE is not set
CONFIG_KSU_FEATURE_SULOG=y
CONFIG_KSU_FEATURE_ADBROOT=y
# CONFIG_KSU_DEBUG is not set
# CONFIG_KSU_THRONE_TRACKER_ALWAYS_THREADED is not set
CONFIG_KSU_LSM_SECURITY_HOOKS=y

EOF

    echo "Verifying $DEFCONFIG:"
    tail -n 20 "$DEFCONFIG"
    echo ""

    echo "Setup KernelSU"
    curl -LSs "https://raw.githubusercontent.com/backslashxx/KernelSU/master/kernel/setup.sh" | bash -
    echo ""
fi

echo "Generate Kernel Name"
CIP_VERSION=$(cat localversion-cip | tr -d '\n\r')
ST_VERSION=$(cat localversion-st | tr -d '\n\r')
FULL_KERNEL_NAME=${KERNEL_NAME}-${DATE}.${PATCH_VERSION}-${KERNEL_VARIANT}
echo "${FULL_KERNEL_NAME}${CIP_VERSION}${ST_VERSION}"
echo ""

if [ -n "$GITHUB_ENV" ]; then
    echo "FULL_KERNEL_NAME=${FULL_KERNEL_NAME}${CIP_VERSION}${ST_VERSION}" >> $GITHUB_ENV
fi

echo "Modify localversion File"
echo "-${FULL_KERNEL_NAME}" > localversion
echo "Verifying localversion:"
cat localversion
echo ""

echo "Configure AnyKernel3 version"
sed -i "s/CIP.*/CIP       : $(echo "$CIP_VERSION" | tr -dc '0-9')/g" toolchain/packaging/AnyKernel3/version
sed -i "s/ST.*/ST        : $(echo "$ST_VERSION" | tr -dc '0-9')/g" toolchain/packaging/AnyKernel3/version
sed -i "s/ElectroX.*/ElectroX  : ${DATE}.${PATCH_VERSION}/g" toolchain/packaging/AnyKernel3/version
sed -i "s/Variant.*/Variant   : ${VARIANT_DISPLAY}/g" toolchain/packaging/AnyKernel3/version
echo ""

echo "Stage Packaging Directory"
rm -rf AnyKernel3
cp -r toolchain/packaging/AnyKernel3 .
echo ""

echo "Preparation Complete."
