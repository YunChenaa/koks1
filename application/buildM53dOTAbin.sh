#!/bin/bash
# ============================================================
# pack_ota.sh — 将 AIC / Lock / T33 文件打包成 OTA bin 文件
#               通过 -AIC / -LOCK / -T33 标志按需指定文件
#
# 用法: ./pack_ota.sh [-AIC <file>] [-LOCK <file>] [-T33 <file>] [-v <version>] [-o <output>]
# 示例:
#       ./pack_ota.sh -AIC AIC_v1.0.bin -LOCK lock_v2.1.bin -T33 t33.bin
#       ./pack_ota.sh -AIC AIC.bin -T33 t33.bin
#       ./pack_ota.sh -LOCK lock.bin
# ============================================================
# 注意事项:
# 1. 输出文件默认名为 ota_package.bin，可通过 -o 指定其他
# 2. 输出文件结构:
#    +--------------------+
#    | OTAFileHeader (72B) |
#    | OTAFileHeader (72B) |
#    | OTAFileHeader (72B) |
#    +--------------------+
#    | AIC 文件内容        |
#    | Lock 文件内容       |
#    | T33 文件内容        |
# 3. OTAFileHeader 结构:
#    +--------------------+
#    | file_size (4B)     |
#    | offset (4B)        |
#    | version (32B)      |
#    | md5 (32B)          |
#    +--------------------+ 
# 4. version 直接取文件名（去掉 .bin 后缀）
# 5. 如果某个文件未指定，则对应的 OTAFileHeader 中 file_size=0, offset=0, version="", md5=0
# 6. 输出文件总大小 = 216 + AIC_size + Lock_size + T33_size
# 7. 注意升级AIC时文件名直接加上时间戳(2606251646)即可, 例如: AIC_2606251646.bin
# 8. 注意强制升级Lock时文件名直接加上时间戳(2606251646)即可, 例如: Lock_2606251646.bin
#    正常升级Lock时文件名直接用版本号, 例如: DQ0.104.A01_20260529.bin
# 9. 升级T33时注意：内部版本号（如V1.0.1）可与文件名末尾版本号不同，但文件名前缀（如DZ_LO_FM）必须与内部版本号前缀一致，否则升级会失败。，



set -euo pipefail

# -------------------- 默认值 --------------------
AIC_FILE=""
LOCK_FILE=""
T33_FILE=""
VERSION="101"
OUTPUT=""

# -------------------- 用法 --------------------
usage() {
    echo "用法: $0 [-AIC <file>] [-LOCK <file>] [-T33 <file>] [-v <version>] [-o <output>]"
    echo ""
    echo "选项:"
    echo "  -AIC  <file>     指定 AIC  固件文件"
    echo "  -LOCK <file>     指定 Lock 固件文件"
    echo "  -T33  <file>     指定 T33  固件文件"
    echo "  -v    <version>  指定版本号 (默认: 101)"
    echo "  -o    <file>     指定输出文件 (默认: S2FMAX_OTA_V<version>_<timestamp>.bin)"
    echo "  -h                显示此帮助信息"
    echo ""
    echo "  version 直接取文件名（去掉 .bin 后缀）"
    echo "  示例: myfw_v1.2.3.bin → version = \"myfw_v1.2.3\""
    exit 0
}

# -------------------- 参数解析 --------------------
if [ $# -eq 0 ]; then
    usage
fi

while [ $# -gt 0 ]; do
    case "$1" in
        -AIC)
            [ $# -ge 2 ] || { echo "错误: -AIC 需要参数" >&2; exit 1; }
            AIC_FILE="$2"; shift 2 ;;
        -LOCK)
            [ $# -ge 2 ] || { echo "错误: -LOCK 需要参数" >&2; exit 1; }
            LOCK_FILE="$2"; shift 2 ;;
        -T33)
            [ $# -ge 2 ] || { echo "错误: -T33 需要参数" >&2; exit 1; }
            T33_FILE="$2"; shift 2 ;;
        -v)
            [ $# -ge 2 ] || { echo "错误: -v 需要参数" >&2; exit 1; }
            VERSION="$2"; shift 2 ;;
        -o)
            [ $# -ge 2 ] || { echo "错误: -o 需要参数" >&2; exit 1; }
            OUTPUT="$2"; shift 2 ;;
        -h|--help) usage ;;
        *) echo "错误: 未知选项 '$1'" >&2; usage ;;
    esac
done

# -------------------- 至少指定一个文件 --------------------
if [ -z "$AIC_FILE" ] && [ -z "$LOCK_FILE" ] && [ -z "$T33_FILE" ]; then
    echo "错误: 至少需要指定一个文件 (-AIC / -LOCK / -T33)" >&2
    usage
fi

# -------------------- 默认输出文件名 --------------------
if [ -z "$OUTPUT" ]; then
    OUTPUT="S2FMAX_OTA_V${VERSION}_$(date +%y%m%d%H%M).bin"
fi

# -------------------- 常量 --------------------
HEADER_PER_DEV=72            # 4(file_size) + 4(offset) + 32(version) + 32(md5)
TOTAL_HEADER_SIZE=$((3 * HEADER_PER_DEV))   # 216

# -------------------- 辅助函数 --------------------

# 将 uint32 以大端序写入 stdout（4 字节二进制）
write_be32() {
    local val=$1
    printf "\\x$(printf '%02x' $(( (val >> 24) & 0xFF )))"
    printf "\\x$(printf '%02x' $(( (val >> 16) & 0xFF )))"
    printf "\\x$(printf '%02x' $(( (val >>  8) & 0xFF )))"
    printf "\\x$(printf '%02x' $((  val        & 0xFF )))"
}

# 从文件名解析 version（取 .bin 之前的文件名）
# 例: myfw_v1.2.3.bin → "myfw_v1.2.3"
#     无 .bin 后缀 → ""
extract_version() {
    local file=$1
    local basename
    basename=$(basename "$file")

    # 直接取 .bin 之前的文件名作为 version
    echo "${basename%.bin}"
}

# 写入 32 字节 version 字段（右补 \0，最多 31 有效字符）
write_version32() {
    local ver=$1
    local len=${#ver}
    local i

    if [ "$len" -gt 31 ]; then
        len=31   # 最多 31 有效字符 + 1 字节 \0
    fi

    # 写入有效字符
    for (( i=0; i<len; i++ )); do
        printf "\\x$(printf '%02x' "'${ver:$i:1}")"
    done

    # 写入 \0
    printf "\\x00"

    # 剩余补 \0
    local pad=$(( 31 - len ))
    for (( i=0; i<pad; i++ )); do
        printf "\\x00"
    done
}

# 获取文件大小，文件不存在返回 0
get_file_size() {
    local file=$1
    if [ -n "$file" ] && [ -f "$file" ]; then
        stat -c%s "$file" 2>/dev/null || stat -f%z "$file" 2>/dev/null
    else
        echo 0
    fi
}

# 判断文件是否有效
file_valid() {
    local file=$1
    [ -n "$file" ] && [ -f "$file" ]
}

# 写入单个 OTAFileHeader (72 字节)
write_one_header() {
    local file=$1
    local offset=$2

    local fsize
    fsize=$(get_file_size "$file")

    # 解析 version
    local ver
    ver=$(extract_version "$file")

    write_be32 "$fsize"           #  4 字节: file_size (大端)
    write_be32 "$offset"          #  4 字节: 文件偏移  (大端)
    write_version32 "$ver"        # 32 字节: version
    if file_valid "$file"; then
        md5sum "$file" | cut -d' ' -f1 | tr -d '\n'   # 32 字节: MD5
    else
        printf '%032d' 0
    fi
}

# -------------------- 计算偏移 --------------------
AIC_SIZE=$(get_file_size "$AIC_FILE")
LOCK_SIZE=$(get_file_size "$LOCK_FILE")
T33_SIZE=$(get_file_size "$T33_FILE")

AIC_OFFSET=$TOTAL_HEADER_SIZE
LOCK_OFFSET=$((TOTAL_HEADER_SIZE + AIC_SIZE))
T33_OFFSET=$((TOTAL_HEADER_SIZE + AIC_SIZE + LOCK_SIZE))

# -------------------- 打印信息 --------------------
echo "=== OTA 打包工具 ==="
echo ""

echo "输入文件:"
for pair in "AIC :$AIC_FILE" "Lock:$LOCK_FILE" "T33 :$T33_FILE"; do
    label="${pair%%:*}"
    f="${pair#*:}"
    if file_valid "$f"; then
        ver=$(extract_version "$f")
        echo "  -${label}  $f  (version: ${ver:-无})"
    else
        echo "  -${label}  (未指定)"
    fi
done
echo "  输出:  $OUTPUT"
echo ""

printf "  %-6s %-10s %-10s %-32s %s\n" "名称" "大小(B)" "偏移" "version" "状态"
for tuple in \
    "AIC :$AIC_SIZE:$AIC_OFFSET:$AIC_FILE" \
    "Lock:$LOCK_SIZE:$LOCK_OFFSET:$LOCK_FILE" \
    "T33 :$T33_SIZE:$T33_OFFSET:$T33_FILE"; do
    label="${tuple%%:*}"
    rest="${tuple#*:}"
    sz="${rest%%:*}"
    rest="${rest#*:}"
    off="${rest%%:*}"
    f="${rest##*:}"
    ver=$(file_valid "$f" && extract_version "$f" || echo "-")
    if file_valid "$f"; then
        printf "  %-6s %-10s %-10s %-32s ok\n" "$label" "$sz" "$off" "${ver:--}"
    else
        printf "  %-6s %-10s %-10s %-32s (缺失)\n" "$label" "0" "$off" "-"
    fi
done

echo ""
echo "MD5:"
for pair in "AIC :$AIC_FILE" "Lock:$LOCK_FILE" "T33 :$T33_FILE"; do
    label="${pair%%:*}"
    f="${pair#*:}"
    if file_valid "$f"; then
        printf "  %-6s %s\n" "$label" "$(md5sum "$f" | cut -d' ' -f1)"
    else
        printf "  %-6s (缺失)\n" "$label"
    fi
done
echo ""

# -------------------- 打包 --------------------
{
    write_one_header "$AIC_FILE"  "$AIC_OFFSET"
    write_one_header "$LOCK_FILE" "$LOCK_OFFSET"
    write_one_header "$T33_FILE"  "$T33_OFFSET"

    file_valid "$AIC_FILE"  && cat "$AIC_FILE"
    file_valid "$LOCK_FILE" && cat "$LOCK_FILE"
    file_valid "$T33_FILE"  && cat "$T33_FILE"
} > "$OUTPUT"

TOTAL_SIZE=$(stat -c%s "$OUTPUT" 2>/dev/null || stat -f%z "$OUTPUT" 2>/dev/null)
echo "完成! 输出: $OUTPUT ($TOTAL_SIZE 字节)"