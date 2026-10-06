#include "LCD.h"
#include "sys_logger.h"
#include <cstdint>
#include <cstdio>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <vector>
#include "koks_project_def.h"

int32_t LCD::init(const char *lcd_dev) {

  int i = 0, j = 0;
  int ret = 0;

  fb_dev.fd = open(lcd_dev, O_RDWR);
  if (fb_dev.fd <= 2) {
    LOG_ERROR("fb0 open error\n");
    return fb_dev.fd;
  }

  /* 获取 framebuffer var_info */
  /* get framebuffer's var_info */
  if ((ret = ioctl(fb_dev.fd, FBIOGET_VSCREENINFO, &fb_dev.var_info)) < 0) {
    LOG_ERROR("FBIOGET_VSCREENINFO failed\n");
    goto err_getinfo;
  }

  /* 获取 framebuffer's fix_info */
  /* get framebuffer's fix_info */
  if ((ret = ioctl(fb_dev.fd, FBIOGET_FSCREENINFO, &fb_dev.fix_info)) < 0) {
    LOG_ERROR("FBIOGET_FSCREENINFO failed\n");
    goto err_getinfo;
  }

  fb_dev.var_info.width = fb_dev.var_info.xres;
  fb_dev.var_info.height = fb_dev.var_info.yres;
  fb_dev.bpp = fb_dev.var_info.bits_per_pixel >> 3;

  fb_dev.width = fb_dev.var_info.xres;
  fb_dev.height = fb_dev.var_info.yres;
  /* format rgb888 use 4 word ; format nv12/nv21 user 2 word */
  fb_dev.fb_size = fb_dev.var_info.xres * fb_dev.var_info.yres * fb_dev.bpp;
  fb_dev.num_buf = fb_dev.var_info.yres_virtual / fb_dev.var_info.yres;
  fb_dev.vid_size = fb_dev.fb_size * fb_dev.num_buf;

  fb_dev.buf_addr = mmap(0, fb_dev.vid_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                         fb_dev.fd, 0);
  if (fb_dev.buf_addr == 0) {
    LOG_ERROR("Map failed");
    ret = -1;
    goto err_getinfo;
  }

  for (i = 0; i < fb_dev.num_buf; i++) {
    for (j = 0; j < 1; j++) {
      fb_dev.data_buf[i][j] =
          (unsigned int)(fb_dev.buf_addr) + j * fb_dev.fb_size +i * fb_dev.fb_size;
    }
  }
  LOG_INFO("xres = %d, yres = %d line_length = %d fbsize = %d, num_buf = %d, "
            "vidSize = %d\n",
            fb_dev.var_info.xres, fb_dev.var_info.yres,
            fb_dev.fix_info.line_length, fb_dev.fb_size, fb_dev.num_buf,
            fb_dev.vid_size);
  return ret;

err_getinfo:
  close(fb_dev.fd);
  return ret;
}

int32_t LCD::jzfb_pan_display() {
  int ret = 0;
  fb_dev.var_info.yoffset = fb_dev.height * fram_num;

  fb_dev.var_info.activate = FB_ACTIVATE_NOW;
  ret = ioctl(fb_dev.fd, FBIOPAN_DISPLAY, &fb_dev.var_info);

#ifdef KOKS_PROJECT_M53D
  fram_num++;
  if (fram_num >= fb_dev.num_buf)
    fram_num = 0;
#endif


  if (ret < 0) {
    LOG_ERROR("pan display error!");
    return ret;
  }
  return 0;
}

int32_t LCD::DisplayNV12(void *data, int screenwidth, int screenheight) {

  memcpy((void *)fb_dev.data_buf[fram_num][0], (void *)(data),
         screenwidth * screenheight * 3 / 2);

  if (jzfb_pan_display()) {
    LOG_ERROR("pan display error!");
    return -1;
  }

  return 0;
}

int32_t LCD::DisplayNV12toRGB565(void *data, int screenwidth, int screenheight)
{
    uint16_t *RGB565dist = (uint16_t *)fb_dev.data_buf[fram_num][0];
    uint8_t *NV12src = (uint8_t *)data;

    uint8_t *Yplane  = NV12src;                    // Y 平面
    uint8_t *UVplane = NV12src + screenwidth * screenheight;   // UV 交错平面

    for (int y = 0; y < screenheight; y++) {
        for (int x = 0; x < screenwidth; x++) {
            // 从 NV12 提取 Y、U、V
            int Y_val = Yplane[y * screenwidth + x];
            int U_val = UVplane[(y / 2) * screenwidth + (x & ~1)];      // 偶数 x 取 U
            int V_val = UVplane[(y / 2) * screenwidth + (x & ~1) + 1];  // 奇数 x 取 V

            // YUV → RGB (ITU-R BT.601, 定点运算避免浮点)
            int C = Y_val - 16;
            int D = U_val - 128;
            int E = V_val - 128;

            int R = (298 * C + 409 * E + 128) >> 8;
            int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
            int B = (298 * C + 516 * D + 128) >> 8;

            // 钳位到 [0, 255]
            if (R < 0) R = 0; if (R > 255) R = 255;
            if (G < 0) G = 0; if (G > 255) G = 255;
            if (B < 0) B = 0; if (B > 255) B = 255;

            // 打包为 RGB565
            uint16_t rgb565 = (uint16_t)(((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3));
            RGB565dist[y * screenwidth + x] = rgb565;
        }
    }

    if (jzfb_pan_display()) {
        LOG_ERROR("pan display error!");
        return -1;
    }

    return 0;
}

int32_t LCD::DisplayColor(uint8_t r, uint8_t g, uint8_t b, int screenwidth, int screenheight)
{   
    //rgb to nv12
    uint8_t y = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
    uint8_t u = (uint8_t)(-0.169 * r - 0.331 * g + 0.5 * b + 128);
    uint8_t v = (uint8_t)(0.5 * r - 0.419 * g - 0.081 * b + 128);

    uint8_t * dist = (uint8_t *)fb_dev.data_buf[fram_num][0];

    LOG_INFO("fram_num %d DisplayColor: r=%d, g=%d, b=%d, y=%d, u=%d, v=%d\n", fram_num, r, g, b, y, u, v);

    memset(dist, y, screenwidth * screenheight);
    for (int i = 0; i < screenwidth * screenheight / 4; i++) {
        dist[screenwidth * screenheight + i * 2] = u;
        dist[screenwidth * screenheight + i * 2 + 1] =v;
    }

    if (jzfb_pan_display()) {
        LOG_ERROR("pan display error!");
        return -1;
    }
    return 0;
}




int32_t LCD::DisplayColorRGB565(uint8_t r, uint8_t g, uint8_t b, int screenwidth, int screenheight)
{   

    uint16_t * dist = (uint16_t *)fb_dev.data_buf[fram_num][0];
    uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    for (int i = 0; i < screenwidth * screenheight; i++) {
        dist[i] = rgb565;
    }
    LOG_INFO("fram_num %d DisplayColor: r=%d, g=%d, b=%d\n", fram_num, r, g, b);

    if (jzfb_pan_display()) {
        LOG_ERROR("pan display error!");
        return -1;
    }
    return 0;
}

int32_t LCD::draw_nv12picture(int startx, int starty, void *Picdata,
                               int Picwidth, int Picheight,
                               int screenwidth, int screenheight)
{
    uint8_t *src = (uint8_t *)Picdata;
    uint8_t *dst = (uint8_t *)fb_dev.data_buf[fram_num][0];

    // ========== 1. 拷贝 Y 平面 ==========
    // 源 Y 起始地址: src + 0
    // 目标 Y 起始地址: dst + 0
    for (int y = 0; y < Picheight; y++) {
        for (int x = 0; x < Picwidth; x++) {
            int screen_x = startx + x;
            int screen_y = starty + y;

            if (screen_x >= 0 && screen_x < screenwidth &&
                screen_y >= 0 && screen_y < screenheight) {
                // Y 平面：1 字节/像素
                dst[screen_y * screenwidth + screen_x] =
                    src[y * Picwidth + x];
            }
        }
    }

    // ========== 2. 拷贝 UV 平面 ==========
    // NV12 UV 是 2x2 子采样的交错平面
    // 每个 2x2 宏块对应一对 UV（U 在偏移0，V 在偏移1）
    int src_uv_offset  = Picwidth * Picheight;       // 源 UV 起始偏移
    int dst_uv_offset  = screenwidth * screenheight;  // 目标 UV 起始偏移

    for (int y = 0; y < Picheight; y += 2) {
        for (int x = 0; x < Picwidth; x += 2) {
            int screen_x = startx + x;
            int screen_y = starty + y;

            if (screen_x >= 0 && screen_x < screenwidth &&
                screen_y >= 0 && screen_y < screenheight) {
                // UV 在交错平面中的位置
                int src_uv_idx = src_uv_offset + (y / 2) * Picwidth + x;
                int dst_uv_idx = dst_uv_offset +
                                 (screen_y / 2) * screenwidth + screen_x;

                dst[dst_uv_idx]     = src[src_uv_idx];      // U
                dst[dst_uv_idx + 1] = src[src_uv_idx + 1];  // V
            }
        }
    }

    if (jzfb_pan_display()) {
        LOG_ERROR("pan display error!");
        return -1;
    }
    return 0;
}



int32_t LCD::draw_nv12picturefile(int startx, int starty ,  char * file_path  , int Picwidth, int Picheight , int screenwidth, int screenheight){
    FILE *fp = fopen(file_path, "rb");
    if (!fp) {
        LOG_ERROR("Failed to open file: %s\n", file_path);
        return -1;
    }

    // 计算 NV12 数据大小
    int nv12_size = Picwidth * Picheight * 3 / 2;
    std::vector<uint8_t> buffer(nv12_size);

    // 读取 NV12 数据
    size_t read_size = fread(buffer.data(), 1, nv12_size, fp);
    fclose(fp);

    if (read_size != nv12_size) {
        LOG_ERROR("Failed to read NV12 data from file: %s\n", file_path);
        return -1;
    }

    // 调用 draw_nv12picture 绘制图像  
    return draw_nv12picture(startx, starty, buffer.data(), Picwidth, Picheight, screenwidth, screenheight);
}