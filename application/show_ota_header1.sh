#!/bin/bash

#
# OTA 包头部信息解析脚本 v2 (仅读取头部，不拆分镜像)
# 用法: ./show_ota_header1.sh <ota_package.bin> [project_name]
#   project_name 可选: default, m21b, m53d (不指定则显示所有可能映射)
#

OTA_FILE="$1"
HEADER_SIZE=4096
PROJECT="${2:-auto}"   # auto 表示自动显示所有可能映射

if [ -z "$OTA_FILE" ]; then
    echo "用法: $0 <ota_package.bin> [project_name]"
    echo "  project_name: default, m21b, m53d, auto"
    exit 1
fi

if [ ! -f "$OTA_FILE" ]; then
    echo "错误: 文件 '$OTA_FILE' 不存在!"
    exit 1
fi

###############################################################################
# Tag → 分区名称映射表 (来自 build_ota_package1.sh 中的 PARTITION_IMAGES)
###############################################################################
# 格式: declare -A TAG_MAP_<project>=( [tag]="name" )
declare -A TAG_MAP_DEFAULT=(
    [0]="boot.bin"
    [1]="tag.bin"
    [2]="kernel_s1.bin"
    [3]="rootfs.bin"
    [4]="recovery.bin"
    [5]="system.bin"
    [6]="config.bin"
    [7]="config_b.bin"
    [9]="kernel_s0.bin"
    [10]="model.bin / model_header.bin"
)

declare -A TAG_MAP_M21B=(
    [0]="boot.bin"
    [1]="tag.bin"
    [2]="kernel_s1.bin"
    [3]="rootfs.bin"
    [4]="recovery.bin"
    [5]="system.bin"
    [6]="config.bin"
    [7]="config_b.bin"
    [8]="model.bin / model_header.bin"
)

declare -A TAG_MAP_M53D=(
    [0]="boot.bin"
    [1]="tag.bin"
    [2]="kernel_s1.bin"
    [3]="rootfs.bin"
    [4]="recovery.bin"
    [5]="system.bin"
    [6]="config.bin"
    [7]="config_b.bin"
    [10]="kernel_s0.bin"
    [11]="model.bin / model_header.bin"
)

# 根据 tag 和项目查名称
get_partition_name() {
    local tag=$1
    local proj=$2
    local name=""
    case "$proj" in
        default) name="${TAG_MAP_DEFAULT[$tag]:-}" ;;
        m21b)    name="${TAG_MAP_M21B[$tag]:-}" ;;
        m53d)    name="${TAG_MAP_M53D[$tag]:-}" ;;
    esac
    echo "${name:-(未知)}"
}

# 获取一个 tag 在所有项目中的可能名称(去重)
get_all_names() {
    local tag=$1
    local names=()
    local n1="${TAG_MAP_DEFAULT[$tag]:-}"
    local n2="${TAG_MAP_M21B[$tag]:-}"
    local n3="${TAG_MAP_M53D[$tag]:-}"
    [[ -n "$n1" ]] && names+=("$n1")
    [[ -n "$n2" && "$n2" != "$n1" ]] && names+=("$n2")
    [[ -n "$n3" && "$n3" != "$n1" && "$n3" != "$n2" ]] && names+=("$n3")
    if [ ${#names[@]} -eq 0 ]; then
        echo "(未知)"
    else
        # 用 | 连接
        local IFS="|"
        echo "${names[*]}"
    fi
}

###############################################################################
# 开始解析
###############################################################################
FILE_SIZE=$(wc -c < "$OTA_FILE")
echo "=========================================="
echo "  OTA 包头部信息解析"
echo "  文件: $OTA_FILE"
echo "  文件大小: $FILE_SIZE 字节"
if [ "$PROJECT" != "auto" ]; then
    echo "  项目模式: $PROJECT"
else
    echo "  项目模式: auto (显示所有可能映射)"
fi
echo "=========================================="

# ---------- 1. 魔数 ----------
MAGIC_HEX=$(xxd -l 4 -e -g 4 "$OTA_FILE" 2>/dev/null | head -1 | awk '{print $2}')
echo ""
echo "--- [1] 魔数 (Magic Number) ---"
echo "  hex: 0x${MAGIC_HEX}"

if [ "$MAGIC_HEX" != "53454658" ]; then
    echo "  警告: 魔数不匹配! 期望 0x53454658 (SEFX)"
fi

# ---------- 2. 镜像数量 ----------
COUNT_HEX=$(xxd -s 4 -l 4 -e -g 4 "$OTA_FILE" 2>/dev/null | head -1 | awk '{print $2}')
IMAGE_COUNT=$((16#${COUNT_HEX}))
echo ""
echo "--- [2] 镜像数量 (Image Count) ---"
echo "  数量: ${IMAGE_COUNT}"

if [ "$IMAGE_COUNT" -eq 0 ]; then
    echo "  警告: 镜像数量为 0!"
elif [ "$IMAGE_COUNT" -gt 256 ]; then
    echo "  警告: 镜像数量异常，可能文件损坏!"
fi

# ---------- 3. 镜像信息列表 ----------
INFO_START=8
INFO_ENTRY_SIZE=16

echo ""
echo "--- [3] 镜像信息列表 (Image Info) ---"
if [ "$PROJECT" = "auto" ]; then
    printf "  %-4s %-6s %-10s %-12s %-12s %s\n" \
        "No." "Tag" "SubOffset" "DataOffset" "DataSize" "分区名称(可能)"
    printf "  %-4s %-6s %-10s %-12s %-12s %s\n" \
        "---" "---" "---------" "----------" "--------" "----------------"
else
    printf "  %-4s %-6s %-10s %-12s %-12s %s\n" \
        "No." "Tag" "SubOffset" "DataOffset" "DataSize" "分区名称"
    printf "  %-4s %-6s %-10s %-12s %-12s %s\n" \
        "---" "---" "---------" "----------" "--------" "----------"
fi

TOTAL_DATA_SIZE=0
for i in $(seq 0 $((IMAGE_COUNT - 1))); do
    OFF=$((INFO_START + i * INFO_ENTRY_SIZE))

    TAG_HEX=$(xxd -s $OFF -l 4 -e -g 4 "$OTA_FILE" 2>/dev/null | head -1 | awk '{print $2}')
    TAG=$((16#${TAG_HEX}))

    SUB_OFF_HEX=$(xxd -s $((OFF + 4)) -l 4 -e -g 4 "$OTA_FILE" 2>/dev/null | head -1 | awk '{print $2}')
    SUB_OFF=$((16#${SUB_OFF_HEX}))

    IMG_OFF_HEX=$(xxd -s $((OFF + 8)) -l 4 -e -g 4 "$OTA_FILE" 2>/dev/null | head -1 | awk '{print $2}')
    IMG_OFF=$((16#${IMG_OFF_HEX}))

    IMG_SIZE_HEX=$(xxd -s $((OFF + 12)) -l 4 -e -g 4 "$OTA_FILE" 2>/dev/null | head -1 | awk '{print $2}')
    IMG_SIZE=$((16#${IMG_SIZE_HEX}))

    TOTAL_DATA_SIZE=$((TOTAL_DATA_SIZE + IMG_SIZE))

    if [ "$PROJECT" = "auto" ]; then
        NAME=$(get_all_names $TAG)
    else
        NAME=$(get_partition_name $TAG "$PROJECT")
    fi

    printf "  %-4d %-6d %-10d %-12d %-12d %s\n" \
        $((i + 1)) $TAG $SUB_OFF $IMG_OFF $IMG_SIZE "$NAME"
done

echo ""
echo "  镜像数据总大小: $TOTAL_DATA_SIZE 字节"

# ---------- 4. 版本字符串 ----------
VERSION_OFFSET=$((INFO_START + IMAGE_COUNT * INFO_ENTRY_SIZE))
VERSION_MAX_LEN=$((HEADER_SIZE - VERSION_OFFSET))

echo ""
echo "--- [4] 版本信息 (Version) ---"
if [ "$VERSION_MAX_LEN" -gt 0 ]; then
    VERSION=$(dd if="$OTA_FILE" bs=1 skip=$VERSION_OFFSET count=$VERSION_MAX_LEN 2>/dev/null | tr -d '\0')
    if [ -z "$VERSION" ]; then
        echo "  (空)"
    else
        echo "  ${VERSION}"
    fi
else
    echo "  (头部空间不足)"
fi

# ---------- 5. 完整性校验 ----------
EXPECTED_SIZE=$((HEADER_SIZE + TOTAL_DATA_SIZE))
echo ""
echo "--- [5] 完整性校验 ---"
echo "  头部固定:    ${HEADER_SIZE} 字节"
echo "  数据区合计:  ${TOTAL_DATA_SIZE} 字节"
echo "  期望总大小:  ${EXPECTED_SIZE} 字节"
echo "  实际总大小:  ${FILE_SIZE} 字节"
if [ "$FILE_SIZE" -eq "$EXPECTED_SIZE" ]; then
    echo "  结果:        [OK] 大小匹配"
elif [ "$FILE_SIZE" -gt "$EXPECTED_SIZE" ]; then
    echo "  结果:        [WARN] 文件比预期大 $((FILE_SIZE - EXPECTED_SIZE)) 字节"
else
    echo "  结果:        [FAIL] 文件比预期小 $((EXPECTED_SIZE - FILE_SIZE)) 字节"
fi

# ---------- 6. 分区映射参考表 ----------
echo ""
echo "--- [6] PARTITION_IMAGES tag 映射参考表 ---"
echo "  (来自 build_ota_package1.sh)"
echo ""
printf "  %-6s %-25s %-25s %-25s\n" "Tag" "default" "m21b" "m53d"
printf "  %-6s %-25s %-25s %-25s\n" "---" "-------" "----" "----"
for tag in 0 1 2 3 4 5 6 7 8 9 10 11; do
    d="${TAG_MAP_DEFAULT[$tag]:-(无)}"
    m="${TAG_MAP_M21B[$tag]:-(无)}"
    s="${TAG_MAP_M53D[$tag]:-(无)}"
    printf "  %-6s %-25s %-25s %-25s\n" "$tag" "$d" "$m" "$s"
done

echo ""
echo "=========================================="
echo "  头部信息读取完成"
echo "=========================================="