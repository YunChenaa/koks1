/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2024-12-11 17:19:22
 * @LastEditors: 洪照
 * @LastEditTime: 2025-07-29 14:31:58
 * @FilePath: /sense-koks/application/entry/main.cpp
 */

#include <chrono>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define FRAME_PYADDR 0x0312a000

enum {
    PRO_ZRT_IOCTL_DMA_GET_FRAME = 1,
    PRO_ZRT_IOCTL_FREE_DMA_FRAME,
    PRO_ZRT_IOCTL_FRAME_DONE,
    PRO_ZRT_IOCTL_INVALUE,
};

#define MEM_DEV "/dev/mem"
#define SNAP_DEV "/proc/jz/zeratul/dma_frame"
#define ZRT_IOCTL_TYPE 'Z'
#define ZRT_IOCTL_GET_DMA_FRAME _IOWR(ZRT_IOCTL_TYPE, PRO_ZRT_IOCTL_DMA_GET_FRAME, struct fast_frame_info)
#define ZRT_IOCTL_FREE_DMA_FRAME _IOWR(ZRT_IOCTL_TYPE, PRO_ZRT_IOCTL_FREE_DMA_FRAME, struct fast_frame_info)
#define ZRT_IOCTL_FRAME_DONE    _IOWR(ZRT_IOCTL_TYPE, PRO_ZRT_IOCTL_FRAME_DONE, struct fast_frame_ev_info)

struct fast_frame_ev_info {
    int isReady;
    int AeIntegrationTime0;
    int AeIntegrationTime1;
    int AeAGain;
    int AeIspDGain;
};

int GetFastFrame(int16_t width, int16_t height, struct fast_frame_ev_info *info)
{
    int fd = 0;
    int mem_fd = 0;
    void *vaddr = NULL, *firstImagePtr = NULL, *secondImagePtr = NULL;
    int ret = 0;
    const int PAGE_SIZE = sysconf(_SC_PAGESIZE);
    unsigned int image_size = width * height * 3 / 2;

    off_t offset = FRAME_PYADDR & ~(PAGE_SIZE - 1);
    size_t extra = FRAME_PYADDR - offset;
    size_t alloc_size = image_size * 4 + extra; //4张图
    printf("page size = %d, offset = %p, extra = %d, alloc_size = %d, image size = %d\n", PAGE_SIZE, offset, extra, alloc_size, image_size);

    do {
        fd = open(SNAP_DEV, O_RDWR);
        if (fd == -1) {
            printf("Failed to open device\n");
            break;
        }

        while(info->isReady != 0x3) {
            ret = ioctl(fd, ZRT_IOCTL_FRAME_DONE, info);
            if (ret == -1) {
                printf("ioctl error: %s\n", strerror(errno));
                break;
            }
            printf("isDone:%d\n", info->isReady);
            usleep(15 * 1000);
        }

        printf("AeIntegrationTime:%d, AeAGain:%d, AeIspDGain:%d\n", info->AeIntegrationTime, info->AeAGain, info->AeIspDGain);
        mem_fd = open(MEM_DEV, O_RDONLY);
        if (mem_fd <= 0) {
            printf("open %s failed\n", MEM_DEV);
            ret = -1;
            break;
        }

        vaddr = mmap(NULL, alloc_size, PROT_READ, MAP_SHARED, mem_fd, offset);
        if (vaddr == MAP_FAILED) {
            printf("mmap failed, error: %s\n", strerror(errno));
            ret = -1;
            break;
        }

        firstImagePtr = (unsigned char*)vaddr + extra;
        printf("firstImagePtr = %p\n", firstImagePtr);
        const char *file_name[] = {
            "/tmp/ir_origin.raw",
            "/tmp/ir_rotated.raw",
            "/tmp/rgb_origin.raw",
            "/tmp/rgb_rotated.raw"
        };

        for(int i = 0; i < 4; i++) {
            int fd = open(file_name[i], O_RDWR | O_CREAT | O_TRUNC, 0644);
            if(fd > 0) {
                void *imagePtr =(unsigned char*)vaddr + extra + i * image_size;
                write(fd, imagePtr, image_size);
                close(fd);
            }
        }
    } while (0);

    if (vaddr) {
        munmap(vaddr, alloc_size);
    }

    if (mem_fd > 0) {
        close(mem_fd);
    }

    if (fd > 0) {
        close(fd);
    }

    return ret;
}

int main(void)
{
    fast_frame_ev_info evInfo = {0};
    GetFastFrame(800, 608, &evInfo);

    return 0;
}