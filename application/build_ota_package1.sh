#!/bin/bash

#
# such as:
# ./build_ota_package.sh -k kernel.img -r root.img -o ota_package.img
#

source ./logger_info.sh

echo "make ota_package begin ! (format 1)"
app_image_src=""
weight_image_src_1=""
weight_image_src_2=""
weight_image_src_3=""
config_image_src=""
version=""
tag=""
build_project=""
multi_images=()
MODEL_INDEX_FILE="./install/models/model_index.txt"

let app_file_size=0
let weight_file_size_1=0
let weight_file_size_2=0
let weight_file_size_3=0
let config_file_size=0
let image_count=0
let weight_file_count=0
let image_info_size=50
let image_info_offset=0
let image_length_aligned=4096

while getopts o:v:h option; do
    echo $option $OPTARG
    case "$option" in
    o)
        file_dst=${OPTARG}
        ;;
    v)
        version=${OPTARG}
        ;;
    h)
        echo "Usage: .../build_ota_package.sh -v <version> -o <version>-OTA.bin"
        exit 0
        ;;
    esac
done

if [ -z $file_dst ] || [ -z $version ]; then
    logger_info error_info "invalid param!"
    exit 1
fi

if [ -z $KOKS_PROJECT ]; then
    logger_info error_info "KOKS_PROJECT not set"
    exit 1
fi

model_tag=10
[[ $KOKS_PROJECT == @(m21b) ]] && model_tag=8


if [[ $KOKS_PROJECT == @(m53d) ]]; then
    PARTITION_IMAGES=(
        "./tag_image/model.bin,11,0"
        "./tag_image/system.bin,5,0"
        "./tag_image/config.bin,6,0"
        "./tag_image/config_b.bin,7,0"
        "./tag_image/rootfs.bin,3,0"
        "./tag_image/kernel_s0.bin,10,0"
        "./tag_image/kernel_s1.bin,2,0"
        "./tag_image/tag.bin,1,0"
        "./tag_image/boot.bin,0,0"
        "./tag_image/recovery.bin,4,0"
    )
elif [[ $KOKS_PROJECT == @(m21b) ]]; then
    PARTITION_IMAGES=(
        "./tag_image/model.bin,8,0"
        "./tag_image/system.bin,5,0"
        "./tag_image/config.bin,6,0"
        "./tag_image/config_b.bin,7,0"
        "./tag_image/rootfs.bin,3,0"
        "./tag_image/kernel_s1.bin,2,0"
        "./tag_image/tag.bin,1,0"
        "./tag_image/boot.bin,0,0"
        "./tag_image/recovery.bin,4,0"
    )
else
    PARTITION_IMAGES=(
        "./tag_image/model.bin,10,0"
        "./tag_image/system.bin,5,0"
        "./tag_image/config.bin,6,0"
        "./tag_image/config_b.bin,7,0"
        "./tag_image/rootfs.bin,3,0"
        "./tag_image/kernel_s0.bin,9,0"
        "./tag_image/kernel_s1.bin,2,0"
        "./tag_image/tag.bin,1,0"
        "./tag_image/boot.bin,0,0"
        "./tag_image/recovery.bin,4,0"
    )
fi

add_model_images() {
    local file_path="$1"

    if [ ! -f "$file_path" ]; then
        logger_info error_info "文件 '$file_path' 不存在!"
        exit 1
    fi

    local model offset size
    while read -r model offset size _; do
        [ -z "$model" ] && continue
        multi_images+=("./tag_image/${model},${model_tag},${offset}")
    done < "$file_path"
}

# 添加 model_header.bin
multi_images+=("./tag_image/model_header.bin,${model_tag},0")

# 添加模型文件
add_model_images ${MODEL_INDEX_FILE}

# 添加分区文件
multi_images+=("${PARTITION_IMAGES[@]}")

for image_item in "${multi_images[@]}"; do
    IFS=',' read -r image_src image_tag sub_offset<<< "$image_item"
    if [ -f "$image_src" ]; then
        let image_count=$(($image_count + 1))
    fi
done


logger_info normal_info "has input $image_count image!"

if [ $image_count = 0 ]; then
    logger_info error_info "has not found image file"
    exit 1
fi

if [ -f $file_dst ]; then
    rm $file_dst
fi

#############################   ota package layout   ########################################
#  /magic number / + / image count /  + / image info / *n       + / image /
#   0x53563150     +  image count +        0x**             +   ********
#     4 bytes           4 bytes         24 * n bytes       image size bytes
######################################################################################

#############################   image info layout  ########################################
#  / image type / +           / image start /  +  / image size /
#   image type     +             0x********    +    0x********
#   4 bytes                         4bytes          4bytes
######################################################################################

# magic number:0x53454657 ->SEFW
echo -e "0x53454658" | xxd -r | xxd -e | xxd -r >$file_dst

# image count
printf "0: %08x\n" $image_count | xxd -r | xxd -e | xxd -r >>$file_dst

# image info
let image_offset=$image_length_aligned
for image_item in "${multi_images[@]}"; do
    IFS=',' read -r image_src image_tag sub_offset <<< "$image_item"
    if [ -f "$image_src" ]; then
        printf "0: %08x\n" $image_tag | xxd -r | xxd -e | xxd -r >>$file_dst
        printf "0: %08x\n" $((sub_offset)) | xxd -r | xxd -e | xxd -r >>$file_dst
        printf "0: %08x\n" $image_offset | xxd -r | xxd -e | xxd -r >>$file_dst
        let image_size=$(wc -c <${image_src})
        printf "0: %08x\n" $image_size | xxd -r | xxd -e | xxd -r >>$file_dst

        echo "image $image_src, tag $image_tag, sub_offset $sub_offset, offset $image_offset, size $image_size"
        let image_offset=$((image_offset + image_size))
    fi
done

# version
echo "version $version"
echo -n "$version" | xxd -p | xxd -r -p >>$file_dst

# fill blank bytes
truncate -s $image_length_aligned $file_dst

# make ota package
for image_item in "${multi_images[@]}"; do
    IFS=',' read -r image_src image_tag sub_offset <<< "$image_item"
    if [ -f "$image_src" ]; then
        cat $image_src >>$file_dst
    fi
done

MD5VALUE=$(md5sum $file_dst | awk '{print $1}')
echo $MD5VALUE >${file_dst}.md5
echo "make ota_package success !"
