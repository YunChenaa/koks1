#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

#include "sys_logger.h"
#include "fdk-aac/aacenc_lib.h"

HANDLE_AACENCODER aac_handle = NULL;
const UINT channels = 1;
const UINT sample_rate = 8000;
const UINT bitrate = 64000;
const UINT bitwidth = 16;
const UINT afterburner = 1; // Enable afterburner
static UINT pcm_data_offset = 0;
static char pcmDataBuffer[bitwidth / 8 * channels * 1024];

int aac_encoder_init()
{
    AACENC_InfoStruct info;
    memset(&info, 0, sizeof(info));
    if (aacEncOpen(&aac_handle, 0, channels) != AACENC_OK)
    {
        LOG_ERROR("aacEncOpen failed");
        return -1;
    }

    if (aacEncoder_SetParam(aac_handle, AACENC_AOT, AOT_AAC_LC) != AACENC_OK)
    {
        LOG_ERROR("aacEncoder_SetParam AACENC_AOT failed");
        return -1;
    }

    if (aacEncoder_SetParam(aac_handle, AACENC_SAMPLERATE, sample_rate) != AACENC_OK)
    {
        LOG_ERROR("aacEncoder_SetParam AACENC_SAMPLERATE failed");
        return -1;
    }

    if (aacEncoder_SetParam(aac_handle, AACENC_CHANNELMODE, MODE_1) != AACENC_OK)
    {
        LOG_ERROR("aacEncoder_SetParam AACENC_CHANNELMODE failed");
        return -1;
    }

    if (aacEncoder_SetParam(aac_handle, AACENC_CHANNELORDER, 1) != AACENC_OK)
    {
        LOG_ERROR("aacEncoder_SetParam AACENC_CHANNELORDER failed");
        return -1;
    }

    if (aacEncoder_SetParam(aac_handle, AACENC_BITRATE, bitrate) != AACENC_OK)
    {
        LOG_ERROR("aacEncoder_SetParam AACENC_BITRATE failed");
        return -1;
    }

    if (aacEncoder_SetParam(aac_handle, AACENC_TRANSMUX, TT_MP4_ADTS) != AACENC_OK)
    {
        LOG_ERROR("aacEncoder_SetParam AACENC_TRANSMUX failed");
        return -1;
    }

    if (aacEncoder_SetParam(aac_handle, AACENC_AFTERBURNER, 1) != AACENC_OK)
    {
        LOG_ERROR("aacEncoder_SetParam AACENC_AFTERBURNER failed");
        return -1;
    }

    if (aacEncEncode(aac_handle, NULL, NULL, NULL, NULL) != AACENC_OK)
    {
        LOG_ERROR("aacEncEncode failed");
        return -1;
    }

    if (aacEncInfo(aac_handle, &info) != AACENC_OK)
    {
        LOG_ERROR("aacEncInfo failed");
        return -1;
    }
    pcm_data_offset = 0;
    memset(pcmDataBuffer, 0, sizeof(pcmDataBuffer));

    return 0;
}

static int cnt = 0;

static int aac_encode_internal(char *inbuf, int size, unsigned char *outbuf, int outbufSize, int *outLen)
{
    AACENC_BufDesc in_buf = {0}, out_buf = {0};
    AACENC_InArgs in_args = {0};
    AACENC_OutArgs out_args = {0};
    int in_identifier = IN_AUDIO_DATA;
    int in_size, in_elem_size;
    int out_identifier = OUT_BITSTREAM_DATA;
    int out_size, out_elem_size;
    void *in_ptr, *out_ptr;

    in_ptr = inbuf;
    in_size = size;
    in_elem_size = 2;

    in_args.numInSamples = size <= 0 ? -1 : size / 2;
    in_buf.numBufs = 1;
    in_buf.bufs = &in_ptr;
    in_buf.bufferIdentifiers = &in_identifier;
    in_buf.bufSizes = &in_size;
    in_buf.bufElSizes = &in_elem_size;

    *outLen = 0;
    out_ptr = outbuf;
    out_size = outbufSize;
    out_elem_size = 1;

    out_buf.numBufs = 1;
    out_buf.bufs = &out_ptr;
    out_buf.bufferIdentifiers = &out_identifier;
    out_buf.bufSizes = &out_size;
    out_buf.bufElSizes = &out_elem_size;

    AACENC_ERROR err = aacEncEncode(aac_handle, &in_buf, &out_buf, &in_args, &out_args);
    if (err != AACENC_OK)
    {
        LOG_ERROR("aacEncEncode failed, error code: %d", err);
        return -1;
    }

    if(out_args.numOutBytes > 0)
    {
        *outLen = out_args.numOutBytes;
    }
    else
    {
        *outLen = 0;
    }
    return 0;
}

int aac_encode(char* pcmData, int size, unsigned char* outbuf, int outbufSize, int* outLen)
{
    int copy_size = size;
    char* pcmDataPtr = pcmDataBuffer;
    if (pcm_data_offset + copy_size < sizeof(pcmDataBuffer))
    {
        memcpy(pcmDataPtr + pcm_data_offset, pcmData, copy_size);
        pcm_data_offset += copy_size;
        return 0;
    }
    else if(pcm_data_offset + copy_size == sizeof(pcmDataBuffer))
    {
        memcpy(pcmDataPtr + pcm_data_offset, pcmData, copy_size);
        pcm_data_offset = 0;
        return aac_encode_internal(pcmDataBuffer, sizeof(pcmDataBuffer), outbuf, outbufSize, outLen);
    }
    else
    {
        int remaining_size = sizeof(pcmDataBuffer) - pcm_data_offset;
        memcpy(pcmDataPtr + pcm_data_offset, pcmData, remaining_size);
        int ret = aac_encode_internal(pcmDataBuffer, sizeof(pcmDataBuffer), outbuf, outbufSize, outLen);
        memcpy(pcmDataPtr, pcmData + remaining_size, size - remaining_size);
        pcm_data_offset = size - remaining_size;
        return ret;
    }
}

void aac_encoder_deinit()
{
    if (aac_handle)
    {
        aacEncClose(&aac_handle);
        aac_handle = nullptr;
    }
}