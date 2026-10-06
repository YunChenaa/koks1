/*
 * @Copyright: (c) 2014-2024 SenseTime :  All rights reserved.
 * @Description: file content
 * @Author: 洪照
 * @Date: 2025-01-04 23:58:38
 * @LastEditors: 洪照
 * @LastEditTime: 2025-05-23 17:43:50
 * @FilePath: /sense-koks/application/media_core/src/audio_customer_encoder.cpp
 */
#include "audio_customer_encoder.h"
#include "sys_logger.h"
#ifdef AUDIO_CODEC
static short seg_end[8] = { 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF };

namespace st
{
namespace sense_koks
{

int AudioUtils::search(int val, short* table, int size)
{
    int i;

    for (i = 0; i < size; i++) {
        if (val <= *table++)
            return (i);
    }
    return (size);
}

int AudioUtils::alaw2linear(unsigned char a_val)
{
    int t;
    int seg;

    a_val ^= 0x55;

    t = (a_val & QUANT_MASK) << 4;
    seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
    switch (seg) {
    case 0:
        t += 8;
        break;
    case 1:
        t += 0x108;
        break;
    default:
        t += 0x108;
        t <<= seg - 1;
    }
    return ((a_val & SIGN_BIT) ? t : -t);
}

int AudioUtils::ulaw2linear(unsigned char u_val)
{
    int t;

    u_val = ~u_val;

    t = ((u_val & QUANT_MASK) << 3) + BIAS;
    t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

    return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

unsigned char AudioUtils::linear2alaw(int pcm_val)
{
    int mask;
    int seg;
    unsigned char aval;

    if (pcm_val >= 0) {
        mask = 0xD5; /* sign (7th) bit = 1 */
    } else {
        mask = 0x55; /* sign bit = 0 */
        pcm_val = -pcm_val - 8;
    }

    seg = search(pcm_val, seg_end, 8);
    if (seg >= 8) /* out of range, return maximum value. */
        return (0x7F ^ mask);
    else {
        aval = seg << SEG_SHIFT;
        if (seg < 2)
            aval |= (pcm_val >> 4) & QUANT_MASK;
        else
            aval |= (pcm_val >> (seg + 3)) & QUANT_MASK;
        return (aval ^ mask);
    }
}

unsigned char AudioUtils::linear2ulaw(int pcm_val)
{
    int mask;
    int seg;
    unsigned char uval;

    if (pcm_val < 0) {
        pcm_val = BIAS - pcm_val;
        mask = 0x7F;
    } else {
        pcm_val += BIAS;
        mask = 0xFF;
    }

    seg = search(pcm_val, seg_end, 8);
    if (seg >= 8) /* out of range, return maximum value. */
        return (0x7F ^ mask);
    else {
        uval = (seg << 4) | ((pcm_val >> (seg + 3)) & 0xF);
        return (uval ^ mask);
    }
}

int AudioUtils::g711a_decode(short amp[], const unsigned char g711a_data[], int g711a_bytes)
{
    int i;
    int samples;
    unsigned char code;
    int sl;

    for (samples = i = 0;;) {
        if (i >= g711a_bytes)
            break;
        code = g711a_data[i++];

        sl = alaw2linear(code);

        amp[samples++] = (short)sl;
    }
    return samples * 2;
}

int AudioUtils::g711u_decode(short amp[], const unsigned char g711u_data[], int g711u_bytes)
{
    int i;
    int samples;
    unsigned char code;
    int sl;

    for (samples = i = 0;;) {
        if (i >= g711u_bytes)
            break;
        code = g711u_data[i++];

        sl = ulaw2linear(code);

        amp[samples++] = (short)sl;
    }
    return samples * 2;
}

int AudioUtils::g711a_encode(unsigned char g711_data[], const short amp[], int len)
{
    int i;

    for (i = 0; i < len; i++) {
        g711_data[i] = linear2alaw(amp[i]);
    }

    return len;
}

int AudioUtils::g711u_encode(unsigned char g711_data[], const short amp[], int len)
{
    int i;

    for (i = 0; i < len; i++) {
        g711_data[i] = linear2ulaw(amp[i]);
    }

    return len;
}

int AudioUtils::Audio_PCM2G711a(char* InAudioData, char* OutAudioData, int DataLen)
{
    int Retaen = 0;

    if ((NULL == InAudioData) && (NULL == OutAudioData) && (0 == DataLen)) {
        LOG_DEBUG("Error, empty data or transmit failed, exit !\n");
        return -1;
    }

    Retaen = g711a_encode((unsigned char*)OutAudioData, (short*)InAudioData, DataLen / 2);

    return Retaen;
}

int AudioUtils::Audio_PCM2G711u(char* InAudioData, char* OutAudioData, int DataLen)
{
    int Retuen = 0;

    if ((NULL == InAudioData) && (NULL == OutAudioData) && (0 == DataLen)) {
        LOG_DEBUG("Error, empty data or transmit failed, exit !\n");
        return -1;
    }

    Retuen = g711u_encode((unsigned char*)OutAudioData, (short*)InAudioData, DataLen / 2);

    return Retuen;
}

int AudioUtils::Audio_G711a2PCM(char* InAudioData, char* OutAudioData, int DataLen)
{
    int Retade = 0;

    if ((NULL == InAudioData) && (NULL == OutAudioData) && (0 == DataLen)) {
        LOG_DEBUG("Error, empty data or transmit failed, exit !\n");
        return -1;
    }

    Retade = g711a_decode((short*)OutAudioData, (unsigned char*)InAudioData, DataLen);

    return Retade;
}

int AudioUtils::Audio_G711u2PCM(char* InAudioData, char* OutAudioData, int DataLen)
{
    int Retude = 0;

    if ((NULL == InAudioData) && (NULL == OutAudioData) && (0 == DataLen)) {
        LOG_DEBUG("Error, empty data or transmit failed, exit !\n");
        return -1;
    }

    Retude = g711u_decode((short*)OutAudioData, (unsigned char*)InAudioData, DataLen);

    return Retude;
}

int AudioUtils::G711A_Encode_Frm(void* encoder, IMPAudioFrame* data, unsigned char* outbuf, int* outLen)
{
    LOG_DEBUG("G711A_Encode_Frm\n");
    *outLen = Audio_PCM2G711a((char*)data->virAddr, (char*)outbuf, data->len);
    return 0;
}

int AudioUtils::G711U_Encode_Frm(void* encoder, IMPAudioFrame* data, unsigned char* outbuf, int* outLen)
{
    *outLen = Audio_PCM2G711u((char*)data->virAddr, (char*)outbuf, data->len);
    return 0;
}

int AudioUtils::G711A_Decode_Frm(void* decoder, unsigned char* inbuf, int inLen, unsigned short* outbuf, int* outLen, int* chns)
{
    LOG_DEBUG("G711A_Decode_Frm\n");
    *outLen = Audio_G711a2PCM((char*)inbuf, (char*)outbuf, inLen);
    return 0;
}

int AudioUtils::G711U_Decode_Frm(void* decoder, unsigned char* inbuf, int inLen, unsigned short* outbuf, int* outLen, int* chns)
{
    *outLen = Audio_G711u2PCM((char*)inbuf, (char*)outbuf, inLen);
    return 0;
}

} // namespace sense_koks

} // namespace st
#endif