/*
 *  Third party audio device
 */
#include <iostream>
#include "third_party_device.h"
#include "sleep.h"
#include "thread_wrapper.h"

#include "cmcc_thread_pool.h"
#include "cmcc_voip_fun.h"
#include "st_koks_mediamanager.h"

using namespace st::sense_koks;

namespace cmcc_webrtc {

// #define _DEBUG_

Rectifier::Rectifier()
    : _beginningTimePointMs(KBeginningTimePointMsNone), _calledCnt(0) {}

// 获取当次应取样次数(考虑此前时间误差导致的取样偏少情况)
int64_t Rectifier::RestCntForNow(uint64_t currentTime) {

    // 第一次取样，设置统计时间起点为一个周期之前时刻
    if (_beginningTimePointMs == KBeginningTimePointMsNone) {
        _beginningTimePointMs = currentTime - KIntervalMs * 1;
        
        return 1;
    }

    // 期望累积取样次数：累积时间/取样间隔
    // 当次应取样次数 = 期望累积取样次数 - 已取样次数
    int64_t restCnt =
        (currentTime - _beginningTimePointMs) / KIntervalMs - _calledCnt;

    // 重置累计统计值和统计时间起点（以当前时刻为新基点），下次计算应取样次数时生效
    // 该重置是为了避免系统时钟误差累积；若系统时钟误差很小，可调大参数
    // KMaxAccumulatedTimeMs
    if (currentTime - _beginningTimePointMs > KMaxAccumulatedTimeMs) {
        _beginningTimePointMs = currentTime;
        _calledCnt = 0;
    }

    return restCnt;
}

// 递增已取样次数
void Rectifier::AddToStatistics() { _calledCnt++; }

// 清除统计信息
void Rectifier::ClearStatistics() {
    _beginningTimePointMs = KBeginningTimePointMsNone;
    _calledCnt = 0;
}


const int kRecordingFixedSampleRate = 8000;
const int kRecordingNumChannels = 1;
const int kPlayoutFixedSampleRate = 8000;
const int kPlayoutNumChannels = 1;
const int kPlayoutBufferSize = kPlayoutFixedSampleRate / 100
                         * kPlayoutNumChannels * 2;
const int kRecordingBufferSize = kRecordingFixedSampleRate/ 100 
                           * kRecordingNumChannels * 2;

ThirdpartyAudioDevice::ThirdpartyAudioDevice(const int32_t id,
                                 const char* inputFilename,
                                 const char* outputFile):
    _ptrAudioBuffer(NULL),
    _recordingBuffer(NULL),
    _playoutBuffer(NULL),
    _recordingFramesLeft(0),
    _playoutFramesLeft(0),
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _recordingBufferSizeIn10MS(0),
    _recordingFramesIn10MS(0),
    _playoutFramesIn10MS(0),
    _ptrThreadRec(NULL),
    _ptrThreadPlay(NULL),
    _recThreadID(0),
    _playThreadID(0),
    _playing(false),
    _recording(false),
    _lastCallPlayoutMillis(0),
    _lastCallRecordMillis(0),
    _outputFile(*FileWrapper::Create()),
    _inputFile(*FileWrapper::Create()),
    _outputFilename(outputFile),
    _inputFilename(inputFilename),
    _clock(Clock::GetRealTimeClock()) {
    printf("ThirdpartyAudioDevice(构造函数)\n");
}

ThirdpartyAudioDevice::~ThirdpartyAudioDevice() {
  _outputFile.Flush();
  _outputFile.CloseFile();
  delete &_outputFile;
  _inputFile.Flush();
  _inputFile.CloseFile();
  delete &_inputFile;
  delete &_critSect;
  printf("~ThirdpartyAudioDevice(析构函数)\n");
}

int32_t ThirdpartyAudioDevice::ActiveAudioLayer(
    AudioDeviceModule::AudioLayer& audioLayer) const {
      audioLayer = AudioDeviceModule::kLinuxThirdpartyAudio;
  return 0;
}

int32_t ThirdpartyAudioDevice::Init() {

    printf("ThirdpartyAudioDevice::Init()\n");
    //Init Thirdparty device

  return 0; 
  }

int32_t ThirdpartyAudioDevice::Terminate() {
    printf("ThirdpartyAudioDevice::Terminate()\n");

    CriticalSectionScoped lock(&_critSect);

    // RECORDING
    if (_ptrThreadRec)
    {
        ThreadWrapper* tmpThread = _ptrThreadRec;
        _ptrThreadRec = NULL;
        _critSect.Leave();

        tmpThread->SetNotAlive();

        if (tmpThread->Stop())
        {
            delete tmpThread;
        }
        else
        {
            printf("ThirdpartyAudioDevice::Terminate()  failed to close down the rec audio thread\n");
        }

        _critSect.Enter();
    }

    // PLAYOUT
    if (_ptrThreadPlay)
    {
        ThreadWrapper* tmpThread = _ptrThreadPlay;
        _ptrThreadPlay = NULL;
        _critSect.Leave();

        tmpThread->SetNotAlive();

        if (tmpThread->Stop())
        {
            delete tmpThread;
        }
        else
        {
            printf("ThirdpartyAudioDevice::Terminate()  failed to close down the play audio thread");
        }

        _critSect.Enter();
    }


   return 0; 
   }

bool ThirdpartyAudioDevice::Initialized() const 
{ 
  return true;
}

int16_t ThirdpartyAudioDevice::PlayoutDevices() {
  return 1;
}

int16_t ThirdpartyAudioDevice::RecordingDevices() {
  return 1;
}

int32_t ThirdpartyAudioDevice::PlayoutDeviceName(uint16_t index,
                                            char name[kAdmMaxDeviceNameSize],
                                            char guid[kAdmMaxGuidSize]) {
  const char* kName = "thirdparty_device";
  const char* kGuid = "thirdparty_device_unique_id";
  if (index < 1) {
    memset(name, 0, kAdmMaxDeviceNameSize);
    memset(guid, 0, kAdmMaxGuidSize);
    memcpy(name, kName, strlen(kName));
    memcpy(guid, kGuid, strlen(guid));
    return 0;
  }
  return -1;
}

int32_t ThirdpartyAudioDevice::RecordingDeviceName(uint16_t index,
                                              char name[kAdmMaxDeviceNameSize],
                                              char guid[kAdmMaxGuidSize]) {
  const char* kName = "thirdparty_device";
  const char* kGuid = "thirdparty_device_unique_id";
  if (index < 1) {
    memset(name, 0, kAdmMaxDeviceNameSize);
    memset(guid, 0, kAdmMaxGuidSize);
    memcpy(name, kName, strlen(kName));
    memcpy(guid, kGuid, strlen(guid));
    return 0;
  }
  return -1;
}

int32_t ThirdpartyAudioDevice::SetPlayoutDevice(uint16_t index) {
  if (index == 0) {
    _playout_index = index;
    return 0;
  }
  return -1;
}

int32_t ThirdpartyAudioDevice::SetPlayoutDevice(
    AudioDeviceModule::WindowsDeviceType device) {
  return -1;
}

int32_t ThirdpartyAudioDevice::SetRecordingDevice(uint16_t index) {
  if (index == 0) {
    _record_index = index;
    return _record_index;
  }
  return -1;
}

int32_t ThirdpartyAudioDevice::SetRecordingDevice(
    AudioDeviceModule::WindowsDeviceType device) {
  return -1;
}

int32_t ThirdpartyAudioDevice::PlayoutIsAvailable(bool& available) {
  if (_playout_index == 0) {
    available = true;
    return _playout_index;
  }
  available = false;
  return -1;
}

int32_t ThirdpartyAudioDevice::InitPlayout() {
  printf("ThirdpartyAudioDevice::InitPlayout()\n");
  if (_playing)
  {
      return -1;
  }

  // Initialize the speaker (devices might have been added or removed)
  if (InitSpeaker() == -1)
  {
      printf("ThirdpartyAudioDevice  InitSpeaker() failed\n");
  }
  if (_ptrAudioBuffer)
  {
      // Update webrtc audio buffer with the selected parameters
      _ptrAudioBuffer->SetPlayoutSampleRate(kPlayoutFixedSampleRate);
      _ptrAudioBuffer->SetPlayoutChannels(kPlayoutNumChannels);
  }
  return 0;
}

bool ThirdpartyAudioDevice::PlayoutIsInitialized() const {
  return true;
}

int32_t ThirdpartyAudioDevice::RecordingIsAvailable(bool& available) {
  if (_record_index == 0) {
    available = true;
    return _record_index;
  }
  available = false;
  return -1;
}

int32_t ThirdpartyAudioDevice::InitRecording() {
  CriticalSectionScoped lock(&_critSect);

  if (_recording) {
    return -1;
  }

  _recordingFramesIn10MS = kRecordingFixedSampleRate/100;

  if (_ptrAudioBuffer) {
    _ptrAudioBuffer->SetRecordingSampleRate(kRecordingFixedSampleRate);
    _ptrAudioBuffer->SetRecordingChannels(kRecordingNumChannels);
  }
  return 0;
}

bool ThirdpartyAudioDevice::RecordingIsInitialized() const {
  return true;
}

int32_t ThirdpartyAudioDevice::StartPlayout() {
  printf("ThirdpartyAudioDevice::StartPlayout()\n");
  if (_playing)
  {
      return 0;
  }

  _playing = true;
  _playoutFramesLeft = 0;
  _playoutFramesIn10MS = kPlayoutFixedSampleRate / 100;

  _playingRectifier.ClearStatistics(); // 播放累积统计重置

  if (!_playoutBuffer)
      _playoutBuffer = new int8_t[2 *
                                  kPlayoutNumChannels *
                                  kPlayoutFixedSampleRate/100];
  if (!_playoutBuffer)
  {
    _playing = false;
    return -1;
  }

  // PLAYOUT
  const char* threadName = "AudioPlayThread";
  printf("ThirdpartyAudioDevice CreateThread(PlayThreadFunc)\n!");
  _ptrThreadPlay =  ThreadWrapper::CreateThread(PlayThreadFunc,
                                                this,
                                                kRealtimePriority,
                                                threadName);
  if (_ptrThreadPlay == NULL)
  {
      printf("ThirdpartyAudioDevice::StartPlayout() CreateThread(PlayThreadFunc is NULL)\n");
      _playing = false;
      delete [] _playoutBuffer;
      _playoutBuffer = NULL;
      return -1;
  }

#if 0
  if (_outputFile.OpenFile(_outputFilename.c_str(),
                           false, false, false) == -1) {
    printf("ThirdpartyAudioDevice Failed to open playout file %s!", _outputFilename.c_str());
    _playing = false;
    delete [] _playoutBuffer;
    _playoutBuffer = NULL;
    return -1;
  }
#endif

  unsigned int threadID(0);
  if (!_ptrThreadPlay->Start(threadID))
  {
      _playing = false;
      delete _ptrThreadPlay;
      _ptrThreadPlay = NULL;
      delete [] _playoutBuffer;
      _playoutBuffer = NULL;
      return -1;
  }
  _playThreadID = threadID;

  return 0;
}

int32_t ThirdpartyAudioDevice::StopPlayout() {
  printf("ThirdpartyAudioDevice::StopPlayout()\n");
    
  {
      CriticalSectionScoped lock(&_critSect);
      _playing = false;
  }

  // stop playout thread first
  if (_ptrThreadPlay && !_ptrThreadPlay->Stop())
  {
      return -1;
  }
  else {
      delete _ptrThreadPlay;
      _ptrThreadPlay = NULL;
  }

  CriticalSectionScoped lock(&_critSect);

  _playoutFramesLeft = 0;
  delete [] _playoutBuffer;
  _playoutBuffer = NULL;
#if 0
  _outputFile.Flush();
  _outputFile.CloseFile();
#endif
   return 0;
}

bool ThirdpartyAudioDevice::Playing() const {
  const_cast<ThirdpartyAudioDevice *>(this)->StartPlayout();
  return true;
}

int32_t ThirdpartyAudioDevice::StartRecording() {
  _recording = true;
  printf("ThirdpartyAudioDevice::StartRecording()\n");

  _recordingRectifier.ClearStatistics();

  // Make sure we only create the buffer once.
  _recordingBufferSizeIn10MS = _recordingFramesIn10MS *
                               kRecordingNumChannels *
                               2;
  if (!_recordingBuffer) {
      _recordingBuffer = new int8_t[_recordingBufferSizeIn10MS];
  }

#if 0
  if (_inputFile.OpenFile(_inputFilename.c_str(), true,
                              true, false) == -1) {
    printf("ThirdpartyAudioDevice Failed to open audio input file %s!\n",
           _inputFilename.c_str());
    _recording = false;
    delete[] _recordingBuffer;
    _recordingBuffer = NULL;
    return -1;
  }
#endif

  const char* threadName = "AudioCaptureThread";
  printf("ThirdpartyAudioDevice::StartRecording() CreateThread(RecThreadFunc)\n");
  _ptrThreadRec = ThreadWrapper::CreateThread(RecThreadFunc,
                                              this,
                                              kRealtimePriority,
                                              threadName);
  if (_ptrThreadRec == NULL)
  {
      printf("ThirdpartyAudioDevice::StartRecording() CreateThread(RecThreadFunc is NULL)\n");
      _recording = false;
      delete [] _recordingBuffer;
      _recordingBuffer = NULL;
      return -1;
  }

  unsigned int threadID(0);
  if (!_ptrThreadRec->Start(threadID))
  {
      _recording = false;
      delete _ptrThreadRec;
      _ptrThreadRec = NULL;
      delete [] _recordingBuffer;
      _recordingBuffer = NULL;
      return -1;
  }
  _recThreadID = threadID;

  return 0;
}


int32_t ThirdpartyAudioDevice::StopRecording() {
  {
    CriticalSectionScoped lock(&_critSect);
    _recording = false;
  }

  if (_ptrThreadRec && !_ptrThreadRec->Stop())
  {
      return -1;
  }
  else {
      delete _ptrThreadRec;
      _ptrThreadRec = NULL;
  }

  CriticalSectionScoped lock(&_critSect);
  _recordingFramesLeft = 0;
  if (_recordingBuffer)
  {
      delete [] _recordingBuffer;
      _recordingBuffer = NULL;
  }
  return 0;
}

bool ThirdpartyAudioDevice::Recording() const {
  return _recording;
}

int32_t ThirdpartyAudioDevice::SetAGC(bool enable) { return -1; }

bool ThirdpartyAudioDevice::AGC() const { return false; }

int32_t ThirdpartyAudioDevice::SetWaveOutVolume(uint16_t volumeLeft,
                                           uint16_t volumeRight) {
  return -1;
}

int32_t ThirdpartyAudioDevice::WaveOutVolume(uint16_t& volumeLeft,
                                        uint16_t& volumeRight) const {
  return -1;
}

int32_t ThirdpartyAudioDevice::InitSpeaker() 
{ 
  int ret = -2;
  printf("ThirdpartyAudioDevice::InitSpeaker()\n");
  CriticalSectionScoped lock(&_critSect);

  if (_playing)
  {
    printf("ThirdpartyAudioDevice::InitSpeaker _playing is true, return -1\n");
    return -1;
  }

  //ret =_mixerManager.OpenSpeaker(devName);
  ret = 1;

  return ret;
}

bool ThirdpartyAudioDevice::SpeakerIsInitialized() const 
{ 
  printf("ThirdpartyAudioDevice::SpeakerIsInitialized()\n");
  return true; 
}

int32_t ThirdpartyAudioDevice::InitMicrophone() 
{ 
  printf("ThirdpartyAudioDevice::InitMicrophone()\n");
  return 0; 
}

bool ThirdpartyAudioDevice::MicrophoneIsInitialized() const { return true; }

int32_t ThirdpartyAudioDevice::SpeakerVolumeIsAvailable(bool& available) {
  return -1;
}

int32_t ThirdpartyAudioDevice::SetSpeakerVolume(uint32_t volume) { return -1; }

int32_t ThirdpartyAudioDevice::SpeakerVolume(uint32_t& volume) const { return -1; }

int32_t ThirdpartyAudioDevice::MaxSpeakerVolume(uint32_t& maxVolume) const {
  return -1;
}

int32_t ThirdpartyAudioDevice::MinSpeakerVolume(uint32_t& minVolume) const {
  return -1;
}

int32_t ThirdpartyAudioDevice::SpeakerVolumeStepSize(uint16_t& stepSize) const {
  return -1;
}

int32_t ThirdpartyAudioDevice::MicrophoneVolumeIsAvailable(bool& available) {
  return -1;
}

int32_t ThirdpartyAudioDevice::SetMicrophoneVolume(uint32_t volume) { return -1; }

int32_t ThirdpartyAudioDevice::MicrophoneVolume(uint32_t& volume) const {
  return -1;
}

int32_t ThirdpartyAudioDevice::MaxMicrophoneVolume(uint32_t& maxVolume) const {
  return -1;
}

int32_t ThirdpartyAudioDevice::MinMicrophoneVolume(uint32_t& minVolume) const {
  return -1;
}

int32_t ThirdpartyAudioDevice::MicrophoneVolumeStepSize(uint16_t& stepSize) const {
  return -1;
}

int32_t ThirdpartyAudioDevice::SpeakerMuteIsAvailable(bool& available) { return -1; }

int32_t ThirdpartyAudioDevice::SetSpeakerMute(bool enable) { return -1; }

int32_t ThirdpartyAudioDevice::SpeakerMute(bool& enabled) const { return -1; }

int32_t ThirdpartyAudioDevice::MicrophoneMuteIsAvailable(bool& available) {
  return -1;
}

int32_t ThirdpartyAudioDevice::SetMicrophoneMute(bool enable) { return -1; }

int32_t ThirdpartyAudioDevice::MicrophoneMute(bool& enabled) const { return -1; }

int32_t ThirdpartyAudioDevice::MicrophoneBoostIsAvailable(bool& available) {
  return -1;
}

int32_t ThirdpartyAudioDevice::SetMicrophoneBoost(bool enable) { return -1; }

int32_t ThirdpartyAudioDevice::MicrophoneBoost(bool& enabled) const { return -1; }

int32_t ThirdpartyAudioDevice::StereoPlayoutIsAvailable(bool& available) {

    printf("ThirdpartyAudioDevice::StereoPlayoutIsAvailable()\n");
    CriticalSectionScoped lock(&_critSect);

    // Save rec states and the number of rec channels
    bool playing = _playing;

    available = false;


    if (InitPlayout() == 0)
    {
        available = true;
    }

    // Stop/uninitialize recording
    StopPlayout();

    if (playing)
    {
        printf("ThirdpartyAudioDevice::StereoPlayoutIsAvailable(), playing is true, StartPlayout()\n");
        StartPlayout();
    }

    return 0;
}
int32_t ThirdpartyAudioDevice::SetStereoPlayout(bool enable) {
  return 0;
}

int32_t ThirdpartyAudioDevice::StereoPlayout(bool& enabled) const {
  enabled = true;
  return 0;
}

int32_t ThirdpartyAudioDevice::StereoRecordingIsAvailable(bool& available) {
  available = true;
  return 0;
}

int32_t ThirdpartyAudioDevice::SetStereoRecording(bool enable) {
  return 0;
}

int32_t ThirdpartyAudioDevice::StereoRecording(bool& enabled) const {
  enabled = true;
  return 0;
}

int32_t ThirdpartyAudioDevice::SetPlayoutBuffer(
  const AudioDeviceModule::BufferType type,
  uint16_t sizeMS) {
  _playBufType = type;

  return 0;
}

int32_t ThirdpartyAudioDevice::PlayoutBuffer(AudioDeviceModule::BufferType& type,
                                        uint16_t& sizeMS) const {
  type = _playBufType;
  return 0;
}

int32_t ThirdpartyAudioDevice::PlayoutDelay(uint16_t& delayMS) const {
  return 0;
}

int32_t ThirdpartyAudioDevice::RecordingDelay(uint16_t& delayMS) const { return -1; }

int32_t ThirdpartyAudioDevice::CPULoad(uint16_t& load) const { return -1; }

bool ThirdpartyAudioDevice::PlayoutWarning() const { return false; }

bool ThirdpartyAudioDevice::PlayoutError() const { return false; }

bool ThirdpartyAudioDevice::RecordingWarning() const { return false; }

bool ThirdpartyAudioDevice::RecordingError() const { return false; }

void ThirdpartyAudioDevice::ClearPlayoutWarning() {}

void ThirdpartyAudioDevice::ClearPlayoutError() {}

void ThirdpartyAudioDevice::ClearRecordingWarning() {}

void ThirdpartyAudioDevice::ClearRecordingError() {}

void ThirdpartyAudioDevice::AttachAudioBuffer(AudioDeviceBuffer* audioBuffer) {
  CriticalSectionScoped lock(&_critSect);

  _ptrAudioBuffer = audioBuffer;

  // Inform the AudioBuffer about default settings for this implementation.
  // Set all values to zero here since the actual settings will be done by
  // InitPlayout and InitRecording later.
  _ptrAudioBuffer->SetRecordingSampleRate(0);
  _ptrAudioBuffer->SetPlayoutSampleRate(0);
  _ptrAudioBuffer->SetRecordingChannels(0);
  _ptrAudioBuffer->SetPlayoutChannels(0);
}

bool ThirdpartyAudioDevice::PlayThreadFunc(void* pThis)
{
    return (static_cast<ThirdpartyAudioDevice*>(pThis)->PlayThreadProcess());
}

bool ThirdpartyAudioDevice::RecThreadFunc(void* pThis)
{
    return (static_cast<ThirdpartyAudioDevice*>(pThis)->RecThreadProcess());
}

bool ThirdpartyAudioDevice::PlayThreadProcess() {
    if (!_playing) {
        // printf("ThirdpartyAudioDevice _playing if false\n");
        return false;
    }

    // const int KGroupNums = 2;
#ifdef  _DEBUG_
    static int cnt = 0;
#endif

    uint64_t currentTime = _clock->CurrentNtpInMilliseconds();

    int32_t toBeCalledCnt = _playingRectifier.RestCntForNow(currentTime);

#ifdef  _DEBUG_
    cnt++;
    //if (cnt % 500 == 1) 
    {
        printf("play:  toBeCalledCnt:%d, currentTime=%llu, interval=%llu\n", toBeCalledCnt, currentTime,
               currentTime - _lastCallPlayoutMillis);
    }
    if (toBeCalledCnt > 1 && cnt % 20 == 1) {
        printf("play: toBeCalledCnt=%d\n", toBeCalledCnt);
    }
    if (toBeCalledCnt >= 4) {
      printf("play: toBeCalledCnt=%d is to large, meaning the sleep delay is too large!!!\n", toBeCalledCnt);
      printf("play: currentTime=%llu, interval=%llu\n", currentTime,
               currentTime - _lastCallPlayoutMillis);
    }
#endif

    if (toBeCalledCnt < 0) {
      SleepMs(1); //休眠1ms，避免线程空转
        
      return true;
    }

    int ret;
    auto audio_output = MediaManager::GetInstance().GetDeviceAudioOutput(DEV_AUDIO_OUTPUT_0);
    for (int i = 0; i < toBeCalledCnt; i++) {
        _ptrAudioBuffer->RequestPlayoutData(_playoutFramesIn10MS);
        _playoutFramesLeft = _ptrAudioBuffer->GetPlayoutData(_playoutBuffer);
        assert(_playoutFramesLeft == _playoutFramesIn10MS);
#if 1
        IMPAudioFrame frm;
        frm.virAddr = (uint32_t*)_playoutBuffer;
        frm.len = 2 *kPlayoutNumChannels *kPlayoutFixedSampleRate/100;
        ret = audio_output->SendFrame(frm);
        if (ret != 0) {
            LOG_ERROR("SendFrame failed with %#x\n", ret);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
#else
        //OvipManager::getInstance().voip_set_voice_data((const char*)_playoutBuffer, BLOCK_SIZE);

        if (_outputFile.Open()) {
            _outputFile.Write(_playoutBuffer, kPlayoutBufferSize);
            _outputFile.Flush();
        }
#endif

        _playoutFramesLeft = 0;

        _playingRectifier.AddToStatistics();
    }

    _lastCallPlayoutMillis = currentTime;

    uint64_t beforeSleepTs = _clock->CurrentNtpInMilliseconds();
    int      deltaTime = beforeSleepTs - currentTime;
    int      offsetTimeMs = 0;

    if (10 - deltaTime - offsetTimeMs > 0) {
        SleepMs(10 - deltaTime - offsetTimeMs);

#ifdef  _DEBUG_
        //if (cnt % 500 == 1) 
        {
            uint64_t afterSleepTs = _clock->CurrentNtpInMilliseconds();
            printf(
                "play: deltaTime:%d, beforeSleepTs:%llu, afterSleepTs=%llu, reSleepTime=%llu, exSleepTime=%llu, "
                "offsetTimeMs=%d\n",
                deltaTime, beforeSleepTs, afterSleepTs, afterSleepTs - beforeSleepTs,
                10 - deltaTime, offsetTimeMs);
        }
#endif
    }else{
#ifdef  _DEBUG_
      printf("play: not need to sleep, deltaTime:%d beforeSleepTs:%llu\n",deltaTime,beforeSleepTs);
#endif
    }

    return true;
}

bool ThirdpartyAudioDevice::RecThreadProcess() {
    if (!_recording)
        return false;

    // const int KGroupNums = 2;
#ifdef  _DEBUG_
    static int cnt = 0;
#endif

    uint64_t currentTime = _clock->CurrentNtpInMilliseconds();

    int64_t toBeCalledCnt = _recordingRectifier.RestCntForNow(currentTime);

#ifdef  _DEBUG_
    cnt++;
    if (cnt % 500 == 1) {
        printf("record: currentTime=%llu, interval=%llu\n", currentTime,
               currentTime - _lastCallRecordMillis);
    }
    if (toBeCalledCnt > 1 && cnt % 20 == 1) {
        printf("record: toBeCalledCnt=%d\n", toBeCalledCnt);
    }
    if (toBeCalledCnt >= 4) {
         printf("record: toBeCalledCnt=%d is to large, meaning the sleep delay is too large!!!\n", toBeCalledCnt);
    }
#endif
    
    if (toBeCalledCnt < 0) {
      SleepMs(1); //休眠1ms，避免线程空转

      return true;
    }
#if 1
    ST_ERR ret = ST_ERRNO_SUCCESS;
    auto& media_manager = MediaManager::GetInstance();
    auto dev_audio_in = media_manager.GetDeviceAudioInput(DEV_AUDIO_INPUT_0);
#endif

    //LOG_ERROR("start toBeCalledCnt=%d\n", toBeCalledCnt);
    for (int i = 0; i < toBeCalledCnt; i++) {
      //LOG_ERROR("start toBeCalledCnt\n");
#if 0
        if (_inputFile.Open()) {
            if (_inputFile.Read(_recordingBuffer, kRecordingBufferSize) > 0) {
                _ptrAudioBuffer->SetRecordedBuffer(_recordingBuffer,
                                                   _recordingFramesIn10MS);
            } else {
                _inputFile.Rewind();
            }

            _ptrAudioBuffer->DeliverRecordedData();

            _recordingRectifier.AddToStatistics();
        }
#else
        // if (dev_audio_in->GetAeC() == false) {

        //     dev_audio_in->AudioSetAec(true);
        // }

        // IMPAudioFrame frame;
        // ret = dev_audio_in->GetFrame(frame);
        // if (ret != ST_ERRNO_SUCCESS) {
        //     LOG_ERROR("GetFrame failed\n");
        //     break;
        // }

        if (!audio_queue.empty()) {
            auto audio_frame = audio_queue.pop();
            IMPAudioFrame frame;
            frame.virAddr = (uint32_t*)audio_frame.data(); 
            frame.len = audio_frame.size();
            
            _ptrAudioBuffer->SetRecordedBuffer((int8_t* )frame.virAddr, _recordingFramesIn10MS);
            _ptrAudioBuffer->DeliverRecordedData();
            _recordingRectifier.AddToStatistics();
        }

        //LOG_ERROR("start get frame: %d\n", frame.len);
        //uint32_t dataLen = frame.len/2;


        // ret = dev_audio_in->ReleaseFrame(frame);
        // if (ret != ST_ERRNO_SUCCESS) {
        //     LOG_ERROR("ReleaseFrame failed\n");
        //     break;
        // }
#endif
    }
    _lastCallRecordMillis = currentTime;

    uint64_t beforeSleepTs = _clock->CurrentNtpInMilliseconds();
    int      deltaTime = beforeSleepTs - currentTime;
    int      offsetTimeMs = 0;

    if (10 - deltaTime - offsetTimeMs > 0) {
        SleepMs(10 - deltaTime- offsetTimeMs);
#ifdef  _DEBUG_
        if (cnt % 500 == 1) {
            uint64_t afterSleepTs = _clock->CurrentNtpInMilliseconds();
            printf("record: afterSleepTs=%llu, reSleepTime=%llu, "
                   "exSleepTime=%llu, "
                   "offsetTimeMs=%d\n",
                   afterSleepTs, afterSleepTs - beforeSleepTs, 10 - deltaTime,
                   offsetTimeMs);
        }
#endif
    }else{
#ifdef  _DEBUG_
      printf("record: not need to sleep, deltaTime:%d, beforeSleepTs:%llu\n", deltaTime,beforeSleepTs);
#endif
    }

    return true;
}


int ThirdpartyAudioDevice::ProvideRecordedAudioData(const char* recordedData, unsigned int nSamples){
  int ret = -1;
  _critSect.Enter();
  _ptrAudioBuffer->SetRecordedBuffer(recordedData,
                                             _recordingFramesIn10MS);
  ret = _ptrAudioBuffer->DeliverRecordedData();
  
  _critSect.Leave();

  return ret;
}


int ThirdpartyAudioDevice::RequestPlayoutAudioData(char* recordedData){
  int ret = -1;
  _critSect.Enter();
  _ptrAudioBuffer->RequestPlayoutData(_playoutFramesIn10MS);
  _playoutFramesLeft = _ptrAudioBuffer->GetPlayoutData(recordedData);
  assert(_playoutFramesLeft == _playoutFramesIn10MS);
  _playoutFramesLeft = 0;
  _critSect.Leave();
  return ret;
}

}  // namespace cmcc_webrtc
