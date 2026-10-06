#pragma onnce
#ifdef HAVE_FDK_AAC
int aac_encoder_init();
void aac_encoder_deinit();

int aac_encode(char* pcmData, int size, unsigned char* outbuf, int outbufSize, int* outLen);
#endif