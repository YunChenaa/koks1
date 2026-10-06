#include "LCDVedioPlayer.h"
#include "LCD.h"
#include "sys_logger.h"
#include <cstdint>
#include <functional>
#include <vector>


const char* vedioPath[11] = { "",
                             "/media/standby.264",
                             "/media/doorbell_press.264",
                             "/media/festival_peach.264",
                             "/media/door_open_success.264",
                             "/media/failed.264",
                             "/media/sunny_cloud.264",
                             "/media/rain_snow.264" ,
                             "/media/festival_peach_append.264",   
                             "/media/restore_factory.264",   //index 23
                             "/media/warning.264",  //index 24       
                            };




static std::vector<std::u32string> lyricList = {
    U"女神节快乐",  //index 8
    U"新春福满门",  //index 9
    U"端午安康",    //index 10
    U"童真伴成长",  //index 11
    U"父亲节快乐",  //index 12
    U"感恩节快乐",  //index 13
    U"喜迎国庆",    //index 14
    U"情人节快乐",  //index 15
    U"圣诞喜乐",    //index 16
    U"新年皆胜意",  //index 17
    U"喜乐闹元宵", //index 18
    U"中秋福满多", //index 19
    U"520",       //index 20
    U"心动皆为你",  //index 20
    U"劳动节快乐", //index 21
    U"母亲节快乐", //index 22
};

int LCDVedioPlayer::init()
{

    this->playThread = std::thread(&LCDVedioPlayer::playLoop, this);
    this->decodeThread = std::thread(&LCDVedioPlayer::decodeLoop, this);
    return 0;
}




void LCDVedioPlayer::play(int index)
{
    LOG_INFO("LCDVedioPlayer play index: %d\n", index);

    if(index < 0 || index >= 25)
    {
        LOG_ERROR("Invalid index: %d\n", index);
        return;
    }

    if(this->NowDisplayIndex == index)
    {
        LOG_INFO("Index %d is already playing\n", index);
        return;
    }

    this->NowDisplayIndex = index;
    this->IsIndexChanged = true;
    // 通知解码线程 index 已改变
    this->decodeCv.notify_one();

}

void LCDVedioPlayer::playLoop()
{
    std::vector<uint8_t> nv12Data;

    LOG_INFO("LCDVedioPlayer thread playLoop start\n");

       static bool firstPlay = false;
    
    //显示线程
    while(true)
    {
        //从帧缓冲区读取一帧图像数据
        auto startTime = std::chrono::high_resolution_clock::now();
        if(this->frameBuffer.read(nv12Data))
        {
            LCD::GetInstance()->DisplayNV12(nv12Data.data(), LCD_WIDTH, LCD_HEIGHT);

            if (!firstPlay) {
                firstPlay = true;
                LCD::GetInstance()->SetBacklightBrightness(80); 
            }

            //将图像数据显示到 LCD 上
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            //控制帧率
            if(duration < 66) // 15fps
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(66 - duration));  
            }
        }
    }
}

void LCDVedioPlayer::decodeLoop()
{
    LOG_INFO("insmod LCD video player kernel modules...\n");
    auto start = std::chrono::high_resolution_clock::now();
    system("insmod /lib/fb_gf.ko");
    auto start2 = std::chrono::high_resolution_clock::now();
    system("insmod /lib/lcd_gf.ko");
    auto end = std::chrono::high_resolution_clock::now();

    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(start2 - start).count();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start2).count();
    LOG_INFO("fb_gf.ko insmod time: %lld ms\n", duration1);
    LOG_INFO("lcd_gf.ko insmod time: %lld ms\n", duration2);

    LCD::GetInstance()->init(FBDEV_PATH);

    //解码线程
    std::unique_lock<std::mutex> lock(this->decodeMutex);

    std::vector<uint8_t> nv12Data;


    LOG_INFO("LCDVedioPlayer thread decodeLoop start\n");
    
    this->decodeCv.wait(lock, [this]{ return IsIndexChanged || NowDisplayIndex; });


    nv12Data.resize(LCD_WIDTH * LCD_HEIGHT * 3 / 2); // NV12 格式的帧数据大小

    int times = 0;
    bool FestivalPeachPlayed = false; // 标记是否已经播放过 festival_peach_append.264
    bool StartTextDisplay = false; // 标记是否开始显示文本

    while (true) {
        // 等待 index 改变
        if (IsIndexChanged) {
            IsIndexChanged = false;
            FestivalPeachPlayed = false;
            StartTextDisplay = false;

            if (NowDisplayIndex == 0) {
                this->frameBuffer.clear();
                // todo 不变 ？ 还是显示黑屏;
                this->decodeCv.wait(lock, [this] { return IsIndexChanged; });
                continue;
            } else if (NowDisplayIndex < 8) {
                this->decoder.inputData(vedioPath[NowDisplayIndex]);
            } else if (NowDisplayIndex >= 23 && NowDisplayIndex <= 24) {
                this->decoder.inputData(vedioPath[NowDisplayIndex-14]);
            } 
            
            else  {
                   if(FestivalPeachPlayed == false)
                {
                    this->decoder.inputData(vedioPath[3]);
                }
            }
            

        } else if (NowDisplayIndex != 0) {

            // 解码一帧图像数据

            auto size = this->decoder.getNV12Frame(nv12Data, LCD_WIDTH, LCD_HEIGHT);

            if(size > 0) // 成功解码出一帧图像数据
            {


            }else if(size == 0 && NowDisplayIndex >= 8 && NowDisplayIndex <= 22) // 解码完成
            {
                if(FestivalPeachPlayed == false)
                {
                    times = 0;
                    LOG_DEBUG("Video %d playback completed, now play the appended video\n", NowDisplayIndex);
                    this->decoder.inputData(vedioPath[8]);
                    StartTextDisplay = true;
                    FestivalPeachPlayed = true;

                }else if(FestivalPeachPlayed == true)
                {
                    IsIndexChanged = true;  //循环播放，等待 index 改变
                    StartTextDisplay = false;
                    FestivalPeachPlayed = false;
                    LOG_DEBUG("Appended video playback completed, replay video\n");
                    continue;
                }

            }else if(size == -1) // 解码错误
            {
                LOG_ERROR("Error decoding video frame\n");
            }

          
            if(StartTextDisplay)
            {
                if(NowDisplayIndex < 20)
                {
                    TextDrawOptions textOpt = Basetrack.ApplyToTextOptions(times, Baseopt);
                    std::u32string text = lyricList[NowDisplayIndex - 8];
                    DrawTextOnNV12(nv12Data.data(), LCD_WIDTH, LCD_HEIGHT, LCD_WIDTH, LCD_WIDTH, text, font, textOpt);
                }else if(NowDisplayIndex == 20)
                {
                    TextDrawOptions text520Opt = B520track.ApplyToTextOptions(times, B520opt);
                    TextDrawOptions text520nextOpt = B520nexttrack.ApplyToTextOptions(times, B520nextopt);
                    std::u32string text520 = lyricList[NowDisplayIndex - 8];
                    std::u32string text520next = lyricList[NowDisplayIndex - 7];
                    DrawTextOnNV12(nv12Data.data(), LCD_WIDTH, LCD_HEIGHT, LCD_WIDTH, LCD_WIDTH, text520, font, text520Opt);
                    DrawTextOnNV12(nv12Data.data(), LCD_WIDTH, LCD_HEIGHT, LCD_WIDTH, LCD_WIDTH, text520next, font, text520nextOpt);
                }else
                {
                    TextDrawOptions textOpt = Basetrack.ApplyToTextOptions(times, Baseopt);
                    std::u32string text = lyricList[NowDisplayIndex - 7];
                    DrawTextOnNV12(nv12Data.data(), LCD_WIDTH, LCD_HEIGHT, LCD_WIDTH, LCD_WIDTH, text, font, textOpt);
                }
               
                times += 66;
            }
            // 将解码后的图像数据写入帧缓冲区
            this->frameBuffer.write(nv12Data);
          
        }
    }
    // LOG_INFO("LCDVedioPlayer decodeLoop wait for index change\n");
}
