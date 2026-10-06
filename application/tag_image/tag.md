# 编译 OTA

## 编译步骤

- 请将对应分支的 Inlock 编译后的分区镜像放在 application/tag_image 目录，如 Inlock/out/camera/tag.bin 和 Inlock/out/camera/model.bin
- 如果更新 rootfs，需要重新命名镜像为 rootfs.bin
- 如果更新单个模型，需要把 Inlock/out/camera/model_header.bin 和更新的模型也放在 application/tag_image 目录
- 执行 `./build.sh -p <project_name> -u 1`

生成的 OTA 包在 install/ota 目录下。
