#!/bin/bash

###############################################################################
# @Copyright: (c) 2025 Sunvision :  All rights reserved.
###############################################################################

set -e
CUR_PATH=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
INSTALL_PATH=${CUR_PATH}/install

source $CUR_PATH/logger_info.sh

help() {
    echo "Usage: build.sh [OPTIONS]"
    echo "   -p <PROJECT_NAME>  : Project name."
    echo "   -d                 : Debug build"
    echo "   -i                 : Image pick build type"
    echo "   -u <format>        : Upgrade build (build ota package, format 0 or 1. See tag_image/tag.md)"
    echo "   -l                 : Local or overseas build. Default is local."
    echo "   -h                 : Help"
}

check_build() {
    if [ -z $KOKS_PROJECT ]; then
        logger_info error_info "Missing build project. Use -p <PROJECT_NAME> to specify."
        exit 1
    fi
}

strip() {
    TOOL_CHAIN_DIR=/opt/Ingenic/T33/mips-gcc540-glibc222-r3.3.7.mxu2.cve2/bin
    STRIP=${TOOL_CHAIN_DIR}/mips-linux-uclibc-gnu-strip
    ${STRIP} ${INSTALL_PATH}/bin/koks
    ${STRIP} ${INSTALL_PATH}/bin/rota

    local lib_path_list=(${INSTALL_PATH}/libs/*)
    for res in ${lib_path_list[@]}; do
        STRIPFILES=$(find ${res} -type f -exec file {} \; 2>/dev/null | grep "not stripped" | awk -F ':' '{print $1}')
        for target_file in ${STRIPFILES}; do
            ${STRIP} -g --strip-unneeded ${target_file}
        done
    done

}

get_version() {
    SYS_VERSION=${INSTALL_PATH}/include/sys_version.h
    if [ -f ${SYS_VERSION} ]; then
        version=$(grep SENSELOCK_V3X_VERSION $SYS_VERSION | awk '{print $3}' | sed 's/"//g')
        echo $version
    else
        echo "V0.0.0"
    fi
}

kaadas_ota_repack() {
    local base_dir="$1"
    local subdir="$2"

    # 重命名
    logger_info normal_info "repack kaadas ota"
    local new_name="${subdir:0:6}UVC\$15_OTA_${subdir:6}"

    local old_path="$base_dir/$subdir"
    local new_path="$base_dir/$new_name"

    mv "$old_path/${subdir}.bin" "$old_path/${new_name}.bin"
    mv "$old_path/${subdir}.bin.md5" "$old_path/${new_name}.bin.md5"
    mv "$old_path" "$new_path"

    # 打包
    local tar_name=${new_name}.tar
    local tar_file="$base_dir/${tar_name}"

    tar -cf "$tar_file" -C "$base_dir" "$(basename "$new_path")"

    logger_info normal_info "pack ota ${tar_name} success"
}

build_ota() {
    logger_info normal_info "Start to pack ota, format $BUILD_OTA_FORMAT"
    local package_name=$(get_version)
    local ota_dir=${INSTALL_PATH}/ota
    local package_dir=${ota_dir}/${package_name}
    local out_file=${package_dir}/${package_name}.bin
    local tar_name=${package_name}.tar

    rm -rf ${package_dir}
    mkdir -p ${package_dir}
    echo "Packing ota package to ${out_file} ..."

    if [ "$BUILD_OTA_FORMAT" = "0" ]; then
        ./build_ota_package.sh -v $(get_version) -o ${out_file}
    elif [ "$BUILD_OTA_FORMAT" = "1" ]; then
        ./build_ota_package1.sh -v $(get_version) -o ${out_file}
    else
        logger_info error_info "Unexpected ota format ${BUILD_OTA_FORMAT}"
        exit 1
    fi

    cd $ota_dir
    tar -cvf ${tar_name} ${package_name}/
    cd -

    if [[ "$package_name" = K59MM_* ]] || [[ "$package_name" = P31UT_* ]] || [[ "$package_name" = P31DP_* ]] || [[ "$package_name" = K3D0A_* ]]; then
        rm -f $ota_dir/$tar_name
        kaadas_ota_repack $ota_dir $package_name
    else
        logger_info normal_info "pack ota ${tar_name} success"
    fi
}

BUILD_TYPE=Release
KOKS_PROJECT=
BUILD_IMAGE_PICK=OFF
BUILD_OTA_FORMAT=
BUILD_OVERSEAS=OFF

while getopts p:diu:lh option; do
    case "$option" in
    p)
        KOKS_PROJECT=$(echo "$OPTARG" | tr '[:upper:]' '[:lower:]' | tr -d ['-_'])
        ;;
    d)
        BUILD_TYPE=Debug
        ;;
    l)
        BUILD_OVERSEAS=ON
        ;;
    i)
        BUILD_IMAGE_PICK=ON
        echo "Enable image pick build option"
        ;;
    u)
        BUILD_OTA_FORMAT=$OPTARG
        ;;
    h)
        help
        exit 1
        ;;
    esac
done

export KOKS_PROJECT=${KOKS_PROJECT}

if [ -n "$BUILD_OTA_FORMAT" ]; then
    build_ota
    exit 0
fi

logger_info normal_info "BUILD_TYPE=${BUILD_TYPE}"
logger_info normal_info "KOKS_PROJECT=${KOKS_PROJECT}"

check_build

rm -rf build
rm -rf install
mkdir build
cd build

if [[ $KOKS_PROJECT == @(m31|m32|m33|m53a|m24cp) ]]; then
    if [ $BUILD_IMAGE_PICK == "ON" ]; then
        cmake -DKOKS_PROJECT=$KOKS_PROJECT -DBUILD_WITH_SDK_V2=ON -DBUILD_PALM=ON -DBUILD_IMAGE_PICK=ON -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
    else
        cmake -DKOKS_PROJECT=$KOKS_PROJECT -DBUILD_WITH_SDK_V2=ON -DBUILD_PALM=ON -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
    fi
elif [[ $KOKS_PROJECT == @(m25p) ]]; then
    cmake -DKOKS_PROJECT=$KOKS_PROJECT -DBUILD_WITH_SDK_V2=ON -DBUILD_PALM=ON -DBUILD_OVERSEAS=ON -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
elif [[ $KOKS_PROJECT == @(m31a) ]]; then
    cmake -DKOKS_PROJECT=$KOKS_PROJECT -DBUILD_WITH_SDK_V2=ON -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
elif [[ $KOKS_PROJECT == @(m53d) ]]; then
    cmake -DKOKS_PROJECT=$KOKS_PROJECT -DBUILD_ZBAR=ON -DBUILD_AIC_WIFI=ON -DBUILD_CMCC_OVD=ON -DBUILD_AAC=ON -DBUILD_CMCC_VOIP=ON -DBUILD_WSSERVER=ON -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
elif [[ $KOKS_PROJECT == @(m35) ]]; then
    cmake -DKOKS_PROJECT=$KOKS_PROJECT -DBUILD_WITH_SDK_V2=ON -DBUILD_PALM=OFF -DBUILD_IMAGE_PICK=OFF -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
else
    cmake -DKOKS_PROJECT=$KOKS_PROJECT -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
fi

make -j$(nproc)
make install
strip
cd -
