#!/bin/bash
rm -f *.264
# 遍历当前文件夹下所有..mp4文件
for mp4_file in *.mp4; do
    # 跳过非文件（如空值）
    [ -f "$mp4_file" ] || continue
    # 获取文件名（不含后缀）
    filename="${mp4_file%.mp4}"
    # 执行FFmpeg转换命令
     # 压缩+解码优化：fast预设（解码快）+ CRF30轻度压缩
     # 解码优化：关键帧间隔缩短为1秒（15帧），减少回溯 ; 仅保留1个参考帧（默认3-4个）
     # 解码优化：基础编码档次，兼容所有解码器且解码快
      #编码264：输出为Annex B格式，适合流式传输和解码器兼容
    ffmpeg -i "$mp4_file" -c:v libx264 -pix_fmt yuv420p -vf "transpose=2,scale=240:284:force_original_aspect_ratio=decrease,pad=240:284:(ow-iw)/2:(oh-ih)/2" -crf 28 -r 15 -preset placebo -refs 1 -bf 0 -g 15 -keyint_min 15 -profile:v main -level 3.0 -bsf:v h264_mp4toannexb "${filename}.264" > /dev/null 2>&1
    
    echo "已转换：$mp4_file -> ${filename}.264"
done

echo "所有mp4文件转换完成！"

find . -maxdepth 1 -type f \( -name '*.264'  \) -printf '%s\n' | awk '{s+=$1} END{printf "总大小: %.2f MiB (%d bytes)\n", s/1024/1024, s}'

