/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2025-01-04 23:59:13
 * @LastEditors: 洪照
 * @LastEditTime: 2025-05-23 17:43:17
 * @FilePath: /sense-koks/application/media_core/include/audio_customer_encoder.h
 */
#pragma once

#include "imp_audio.h"
#ifdef AUDIO_CODEC
namespace st
{
namespace sense_koks
{
/* My G711 Encoder */
#define SIGN_BIT (0x80)  /* Sign bit for a A-law byte. */
#define QUANT_MASK (0xf) /* Quantization field mask. */
#define NSEGS (8)        /* Number of A-law segments. */
#define SEG_SHIFT (4)    /* Left shift for segment number. */
#define SEG_MASK (0x70)  /* Segment field mask. */
#define BIAS (0x84)      /* Bias for linear code. */
class AudioUtils
{
    public:
    static int search(int val, short* table, int size);

    static int alaw2linear(unsigned char a_val);

    static int ulaw2linear(unsigned char u_val);

    static unsigned char linear2alaw(int pcm_val);

    static unsigned char linear2ulaw(int pcm_val);

    static int g711a_decode(short amp[], const unsigned char g711a_data[], int g711a_bytes);

    static int g711u_decode(short amp[], const unsigned char g711u_data[], int g711u_bytes);

    static int g711a_encode(unsigned char g711_data[], const short amp[], int len);

    static int g711u_encode(unsigned char g711_data[], const short amp[], int len);

    static int Audio_PCM2G711a(char* InAudioData, char* OutAudioData, int DataLen);

    static int Audio_PCM2G711u(char* InAudioData, char* OutAudioData, int DataLen);

    static int Audio_G711a2PCM(char* InAudioData, char* OutAudioData, int DataLen);

    static int Audio_G711u2PCM(char* InAudioData, char* OutAudioData, int DataLen);

    static int G711A_Encode_Frm(void* encoder, IMPAudioFrame* data, unsigned char* outbuf, int* outLen);

    static int G711U_Encode_Frm(void* encoder, IMPAudioFrame* data, unsigned char* outbuf, int* outLen);

    static int G711A_Decode_Frm(void* decoder, unsigned char* inbuf, int inLen, unsigned short* outbuf, int* outLen, int* chns);

    static int G711U_Decode_Frm(void* decoder, unsigned char* inbuf, int inLen, unsigned short* outbuf, int* outLen, int* chns);
};
} // namespace sense_koks

} // namespace st

#endif