#pragma onnce

#ifdef KOKS_FEATURE_UVC_ENCODE_YUV420P
int nv12_to_jpeg420(const uint8_t *nv12, uint8_t *jpeg_buf,
                    int width, int height, size_t jpeg_cap,
                    size_t *jpeg_size);
#endif

int nv12_to_jpeg422(const uint8_t *nv12, uint8_t *jpeg_buf,
					int width, int height, size_t jpeg_cap,
					size_t *jpeg_size);