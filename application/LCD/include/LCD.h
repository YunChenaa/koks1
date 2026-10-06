#pragma once
#include "sys_logger.h"
#include <cstdint>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

class LCD {
public:
  static LCD *GetInstance() {
    static LCD instance;
    return &instance;
  }

  int32_t init(const char *lcd_dev);
  int32_t DisplayNV12(void *data, int screenwidth, int screenheight);
  int32_t DisplayNV12toRGB565(void *data, int screenwidth, int screenheight);

  int32_t DisplayColor(uint8_t r, uint8_t g, uint8_t b, int screenwidth, int screenheight);
  int32_t DisplayColorRGB565(uint8_t r, uint8_t g, uint8_t b, int screenwidth, int screenheight);


  // brightness: 0-255
  int32_t SetBacklightBrightness(uint8_t brightness)
  {
      LOG_INFO("Set backlight brightness to %d\n", brightness);
      char cmd[128];
      snprintf(cmd, sizeof(cmd), "echo %d > /sys/class/truly_bl/bl_ctrl/brightness", brightness);
      system(cmd);
      return 0;
  }



  int32_t SetBacklightBrightnessPWM(uint8_t brightness)
  {
      LOG_INFO("Set backlight brightness to %d\n", brightness);
      int ret = 0;
      static bool pwm_initialized = false;
      if (!pwm_initialized) {
          ret = system("echo 3 > /sys/class/pwm/pwmchip0/export");
          if (ret != 0) {
              LOG_ERROR("Failed to export PWM channel: %d\n", ret);
              return ret;
          }
          system("echo 20000 > /sys/class/pwm/pwmchip0/pwm3/period");
          if (ret != 0) {
              LOG_ERROR("Failed to set PWM period: %d\n", ret);
              return ret;
          }

          pwm_initialized = true;
      }
      ret = system("echo 0 > /sys/class/pwm/pwmchip0/pwm3/enable"); // Disable PWM before changing duty cycle
      if (ret != 0) {
          LOG_ERROR("Failed to disable PWM: %d\n", ret);
          return ret;
      }

      int duty_cycle = (brightness * 20000) / 255;
      char cmd[128];
      snprintf(cmd, sizeof(cmd), "echo %d > /sys/class/pwm/pwmchip0/pwm3/duty_cycle", duty_cycle);

      ret = system(cmd);
      if (ret != 0) {
          LOG_ERROR("Failed to set PWM duty cycle: %d\n", ret);
          return ret;
      }
      ret = system("echo 1 > /sys/class/pwm/pwmchip0/pwm3/enable");
      if (ret != 0) {
          LOG_ERROR("Failed to enable PWM: %d\n", ret);
          return ret;
      }
      return 0;
  }

  int32_t draw_nv12picture(int startx,int starty , void *Picdata , int Picwidth, int Picheight , int screenwidth, int screenheight);

  int32_t draw_nv12picturefile(int startx, int starty ,  char * file_path  , int Picwidth, int Picheight , int screenwidth, int screenheight); 

private:
  LCD() = default;
  ~LCD()
  {
    if (fb_dev.buf_addr != nullptr) {
        munmap(fb_dev.buf_addr, fb_dev.fb_size);
        fb_dev.buf_addr = nullptr;
     }
     if (fb_dev.fd > 0) {
        close(fb_dev.fd);
        fb_dev.fd = -1;
     }
  }

  struct jzfb_dev {
    unsigned int data_buf[3][1];
    unsigned int num_buf;
    void *buf_addr;

    int width;
    int height;

    unsigned int vid_size;
    unsigned int fb_size;
    int bpp;
    int format;

    struct fb_fix_screeninfo fix_info;
    struct fb_var_screeninfo var_info;

    int fd;
  };

  jzfb_dev fb_dev;

  int32_t fram_num = 0;

  int32_t jzfb_pan_display();
};