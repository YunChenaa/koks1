#pragma once

#include "LCDframebuf.h"
#include "h264decoder.h"
#include "sys_logger.h"
#include <condition_variable>
#include <iostream>
#include <stdint.h>
#include <sys/stat.h>
#include <thread>
#include "st_koks_db.h"
#include "cn_font_48_2bpp.h"
#include "draw_text_nv12.h"
#include "text_motion_track.h"



#define LCD_WIDTH 240
#define LCD_HEIGHT 284

#define FBDEV_PATH "/dev/fb0"

#define FRAME_BUFFER_COUNT 3


class LCDVedioPlayer
{
public:
    static LCDVedioPlayer * getInstance()
    {
        static LCDVedioPlayer instance;
        return &instance;
    }

  

    //播放视频
    void play(int index);

    void playdefault(){
        #ifdef KOKS_FEATURE_LCD_EMOTION
        
        st::sense_koks::LcdEmotionStrategy EmotionStrategy;
        uint8_t default_emotion_index = 0;
        st::sense_koks::koks_db::GetInstance().readData(&EmotionStrategy, &default_emotion_index);
        LCDVedioPlayer::getInstance()->play(default_emotion_index);
        #endif // DEBUG
  
    }

    //停止播放
    void stop(){
         this->play(0);
    }


private:

    LCDVedioPlayer():
        frameBuffer( LCD_WIDTH, LCD_HEIGHT,FRAME_BUFFER_COUNT )
    {
        font = MakeFontView(cn_font_48_2bpp_glyphs, cn_font_48_2bpp_glyph_count, cn_font_48_2bpp_font_width, cn_font_48_2bpp_font_height, cn_font_48_2bpp_font_bpp);

        Baseopt.x = LCD_WIDTH / 2;                                  // 基准点 x（画面中点）
        Baseopt.y = LCD_HEIGHT / 2;                                 // 基准点 y（画面中点）
        Baseopt.offsetX = 0;                                    // 基准点偏移 x
        Baseopt.offsetY = 0;                                    // 基准点偏移 y
        Baseopt.anchor = TextDrawOptions::Anchor::MiddleCenter; // 以文字框中点对齐到基准点
        Baseopt.reverse = true;                                // 倒序绘制
        Baseopt.scale = 1.0f;                                   // 动画模块会覆盖缩放
        Baseopt.letterSpacing = -6;                             // 字间距
        Baseopt.color = { 255, 255, 255 };                      // RGB 颜色
        Baseopt.layout = TextDrawOptions::Layout::Vertical;     // 垂直布局


        B520opt = Baseopt;
        B520opt.x -= 24; // 520动画的基准点向右偏移48像素

        B520nextopt = Baseopt;
        B520nextopt.x += 24; // 520动画的基准点向右偏移48像素

        Basetrack.AddKeyframe(TextMotionKeyframe(0, LCD_WIDTH / 2, LCD_HEIGHT / 2, 0.2f, 55.0f, 55.0f, 55.0f, MotionEase::Linear));
        Basetrack.AddKeyframe(TextMotionKeyframe(440, LCD_WIDTH / 2, LCD_HEIGHT / 2, 1.17f, 255.0f, 255.0f, 255.0f, MotionEase::Linear));
        Basetrack.AddKeyframe(TextMotionKeyframe(960, LCD_WIDTH / 2 - 20, LCD_HEIGHT / 2, 1.17f, 255.0f, 255.0f, 255.0f, MotionEase::Linear));
        Basetrack.AddKeyframe(TextMotionKeyframe(2040, LCD_WIDTH / 2 + 20, LCD_HEIGHT / 2, 1.17f, 255.0f, 255.0f, 255.0f, MotionEase::Linear));
        Basetrack.AddKeyframe(TextMotionKeyframe(2560, LCD_WIDTH / 2, LCD_HEIGHT / 2, 1.17f, 255.0f, 255.0f, 255.0f, MotionEase::Linear));

        B520track.AddKeyframe(TextMotionKeyframe(0, LCD_WIDTH / 2 - 24, LCD_HEIGHT / 2, 0.2f, 55.0f, 55.0f, 55.0f, MotionEase::Linear));
        B520track.AddKeyframe(TextMotionKeyframe(440, LCD_WIDTH / 2 - 24, LCD_HEIGHT / 2, 1.17f, 255.0f, 255.0f, 255.0f, MotionEase::Linear));
        B520track.AddKeyframe(TextMotionKeyframe(960, LCD_WIDTH / 2 - 20 - 24, LCD_HEIGHT / 2, 1.17f, 255.0f, 255.0f, 255.0f, MotionEase::Linear));
        B520track.AddKeyframe(TextMotionKeyframe(2040, LCD_WIDTH / 2 + 20 - 24, LCD_HEIGHT / 2, 1.17f, 255.0f, 255.0f, 255.0f, MotionEase::Linear));
        B520track.AddKeyframe(TextMotionKeyframe(2560, LCD_WIDTH / 2 - 24, LCD_HEIGHT / 2, 1.17f, 255.0f, 255.0f, 255.0f, MotionEase::Linear));

        B520nexttrack.AddKeyframe(TextMotionKeyframe(0, LCD_WIDTH / 2 + 24, LCD_HEIGHT / 2, 0.2f, 55.0f, 55.0f, 55.0f, MotionEase::Linear));
        B520nexttrack.AddKeyframe(TextMotionKeyframe(440, LCD_WIDTH / 2 + 24, LCD_HEIGHT / 2, 1.17f, 255.0f, 255.0f, 255.0f, MotionEase::Linear));
        B520nexttrack.AddKeyframe(TextMotionKeyframe(960, LCD_WIDTH / 2 - 20 + 24, LCD_HEIGHT / 2, 1.17f, 255.0f, 255.0f, 255.0f, MotionEase::Linear));
        B520nexttrack.AddKeyframe(TextMotionKeyframe(2040, LCD_WIDTH / 2 + 20 + 24, LCD_HEIGHT / 2, 1.17f, 255.0f, 255.0f, 255.0f, MotionEase::Linear));
        B520nexttrack.AddKeyframe(TextMotionKeyframe(2560, LCD_WIDTH / 2 + 24, LCD_HEIGHT / 2, 1.17f, 255.0f, 255.0f, 255.0f, MotionEase::Linear));

        this->init();
    }

    FontView font;
    TextDrawOptions Baseopt;
    TextDrawOptions B520opt;
    TextDrawOptions B520nextopt;

    TextMotionTrack Basetrack; //除了520 else
    TextMotionTrack B520track; //除了520 else
    TextMotionTrack B520nexttrack; //除了520 else

    
      //初始化
    int init();
    
    h264decoder decoder;

    std::thread playThread;
    std::thread decodeThread;
    LCDFrameBuffer frameBuffer;

    bool isPlaying = true;
    int PerDisplayIndex = 0;
    int NowDisplayIndex = 0;
    bool IsIndexChanged = false;

    void playLoop();
    void decodeLoop();

    std::condition_variable decodeCv;
    std::mutex decodeMutex;
};