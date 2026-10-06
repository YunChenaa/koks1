#include <cerrno>
#include <csetjmp>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

#include "jpeglib.h"
#include "jerror.h"
#include "koks_project_def.h"
#include "st_koks_uvc.h"

typedef struct {
    struct jpeg_error_mgr pub;
    jmp_buf jump;
} jpeg_error_mgr_t;

typedef struct {
    struct jpeg_destination_mgr pub;
    uint8_t *buf;
    size_t cap;
    size_t size;
} jpeg_dest_mgr_t;

static uint8_t *raw_scratch;
static size_t raw_scratch_size;

static void jpeg_error_exit(j_common_ptr cinfo)
{
    jpeg_error_mgr_t *err = (jpeg_error_mgr_t *)cinfo->err;
    longjmp(err->jump, 1);
}

static void jpeg_dest_init(j_compress_ptr cinfo)
{
    jpeg_dest_mgr_t *dest = (jpeg_dest_mgr_t *)cinfo->dest;

    dest->pub.next_output_byte = dest->buf;
    dest->pub.free_in_buffer = dest->cap;
    dest->size = 0;
}

static boolean jpeg_dest_empty(j_compress_ptr cinfo)
{
    cinfo->err->msg_code = JERR_BUFFER_SIZE;
    (*cinfo->err->error_exit)((j_common_ptr)cinfo);
    return FALSE;
}

static void jpeg_dest_term(j_compress_ptr cinfo)
{
    jpeg_dest_mgr_t *dest = (jpeg_dest_mgr_t *)cinfo->dest;
    dest->size = dest->cap - dest->pub.free_in_buffer;
}

static int prepare_raw_scratch(int width)
{
    int y_width = (width + 15) & ~15;
    int uv_width = y_width / 2;
    size_t need = ((size_t)y_width + (size_t)uv_width * 2U) * DCTSIZE;

    if (raw_scratch_size >= need) {
        return 0;
    }

    free(raw_scratch);
    raw_scratch = (uint8_t *)malloc(need);
    if (raw_scratch == NULL) {
        raw_scratch_size = 0;
        return -ENOMEM;
    }

    raw_scratch_size = need;
    return 0;
}

static void copy_y_row(const uint8_t *src, uint8_t *dst,
                       int width, int padded_width)
{
    memcpy(dst, src, width);
    if (padded_width > width) {
        memset(dst + width, src[width - 1], (size_t)(padded_width - width));
    }
}

static void copy_uv_row(const uint8_t *src, uint8_t *u_dst,
                        uint8_t *v_dst, int width, int padded_width)
{
    int x;
    int uv_width = width / 2;
    int padded_uv_width = padded_width / 2;
    uint8_t last_u = src[width - 2];
    uint8_t last_v = src[width - 1];

    for (x = 0; x < uv_width; x++) {
        u_dst[x] = src[x * 2];
        v_dst[x] = src[x * 2 + 1];
    }
    for (; x < padded_uv_width; x++) {
        u_dst[x] = last_u;
        v_dst[x] = last_v;
    }
}

int nv12_to_jpeg422(const uint8_t *nv12, uint8_t *jpeg_buf,
					int width, int height, size_t jpeg_cap,
					size_t *jpeg_size)
{
    struct jpeg_compress_struct cinfo;
    jpeg_error_mgr_t jerr;
    jpeg_dest_mgr_t dest;
    JSAMPROW y_rows[DCTSIZE];
    JSAMPROW u_rows[DCTSIZE];
    JSAMPROW v_rows[DCTSIZE];
    JSAMPARRAY planes[3];
    const uint8_t *y_src;
    const uint8_t *uv_src;
    uint8_t *y_scratch;
    uint8_t *u_scratch;
    uint8_t *v_scratch;
    int y_width;
    int uv_width;
    int row;

    if (nv12 == NULL || jpeg_buf == NULL || jpeg_size == NULL ||
        width <= 0 || height <= 0 || (width & 1) != 0 || (height & 1) != 0) {
        return -EINVAL;
    }
    if (prepare_raw_scratch(width) < 0) {
        return -ENOMEM;
    }

    y_width = (width + 15) & ~15;
    uv_width = y_width / 2;
    y_src = nv12;
    uv_src = nv12 + (size_t)width * (size_t)height;
    y_scratch = raw_scratch;
    u_scratch = y_scratch + (size_t)y_width * DCTSIZE;
    v_scratch = u_scratch + (size_t)uv_width * DCTSIZE;

    memset(&cinfo, 0, sizeof(cinfo));
    memset(&jerr, 0, sizeof(jerr));
    memset(&dest, 0, sizeof(dest));

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpeg_error_exit;
    if (setjmp(jerr.jump)) {
        jpeg_destroy_compress(&cinfo);
        return -EIO;
    }

    jpeg_create_compress(&cinfo);

    dest.buf = jpeg_buf;
    dest.cap = jpeg_cap;
    dest.pub.init_destination = jpeg_dest_init;
    dest.pub.empty_output_buffer = jpeg_dest_empty;
    dest.pub.term_destination = jpeg_dest_term;
    cinfo.dest = &dest.pub;

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;
    jpeg_set_defaults(&cinfo);
    jpeg_set_colorspace(&cinfo, JCS_YCbCr);
    jpeg_set_quality(&cinfo, 85, TRUE);
    cinfo.raw_data_in = TRUE;
    cinfo.dct_method = JDCT_IFAST;
    cinfo.comp_info[0].h_samp_factor = 2;
    cinfo.comp_info[0].v_samp_factor = 1;
    cinfo.comp_info[1].h_samp_factor = 1;
    cinfo.comp_info[1].v_samp_factor = 1;
    cinfo.comp_info[2].h_samp_factor = 1;
    cinfo.comp_info[2].v_samp_factor = 1;

    jpeg_start_compress(&cinfo, TRUE);

    planes[0] = y_rows;
    planes[1] = u_rows;
    planes[2] = v_rows;

    for (row = 0; row < height; row += DCTSIZE) {
        int j;

        for (j = 0; j < DCTSIZE; j++) {
            int src_y = row + j;
            const uint8_t *y_row;
            const uint8_t *uv_row;
            uint8_t *y_out = y_scratch + (size_t)j * (size_t)y_width;
            uint8_t *u_out = u_scratch + (size_t)j * (size_t)uv_width;
            uint8_t *v_out = v_scratch + (size_t)j * (size_t)uv_width;

            if (src_y >= height) {
                src_y = height - 1;
            }
            y_row = y_src + (size_t)src_y * (size_t)width;
            uv_row = uv_src + (size_t)(src_y / 2) * (size_t)width;

            copy_y_row(y_row, y_out, width, y_width);
            copy_uv_row(uv_row, u_out, v_out, width, y_width);

#ifdef KOKS_FEATURE_UVC_JOINT_LINE
			int line_x = 0;
			const int line_width = 4;

			auto& uvc = MediaUvc::GetInstance();
			if(uvc.NeedDrawMjpegLineV(line_x))
			{
				if (line_x >= 0 && line_x < width) {
					int start_x = line_x - line_width / 2;
					int end_x = line_x + line_width;

					if (end_x > width) end_x = width;
					if(start_x < width)
					{
						for (int w = start_x; w < end_x; w++) {
							y_out[w] = 0;
						}

						int u_start = start_x / 2;
						int u_end = (end_x - 1) / 2;
						for (int idx = u_start; idx <= u_end; idx++) {
							u_out[idx] = 128;
							v_out[idx] = 128;
						}
					}
				}
			}
#endif
            y_rows[j] = y_out;
            u_rows[j] = u_out;
            v_rows[j] = v_out;
        }

        if (jpeg_write_raw_data(&cinfo, planes, DCTSIZE) != DCTSIZE) {
            cinfo.err->msg_code = JERR_BUFFER_SIZE;
            (*cinfo.err->error_exit)((j_common_ptr)&cinfo);
        }
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    *jpeg_size = dest.size;
    return 0;
}


#ifdef KOKS_FEATURE_UVC_ENCODE_YUV420P
int nv12_to_jpeg420(const uint8_t *nv12, uint8_t *jpeg_buf,
                    int width, int height, size_t jpeg_cap,
                    size_t *jpeg_size)
{
    struct jpeg_compress_struct cinfo;
    jpeg_error_mgr_t jerr;
    jpeg_dest_mgr_t dest;

    const int v_samp = 2;                     // Y 垂直采样因子
    const int mcu_rows = v_samp * DCTSIZE;    // = 2*DCTSIZE

    // 所有分量均分配 mcu_rows 个指针
    JSAMPROW y_rows[mcu_rows];
    JSAMPROW u_rows[mcu_rows];
    JSAMPROW v_rows[mcu_rows];
    JSAMPARRAY planes[3];

    const uint8_t *y_src, *uv_src;
    uint8_t *y_scratch, *u_scratch, *v_scratch;
    int y_width, uv_width;
    int row;

    // ---- 参数校验 ----
    if (!nv12 || !jpeg_buf || !jpeg_size ||
        width <= 0 || height <= 0 || (width & 1) || (height & 1))
        return -EINVAL;

    y_width = (width + 15) & ~15;
    uv_width = y_width / 2;

    // ---- 分配临时缓冲区（按 mcu_rows 行分配） ----
    size_t y_need = (size_t)y_width * mcu_rows;
    size_t uv_need = (size_t)uv_width * mcu_rows;   // U/V 也分配 mcu_rows 行
    size_t total_need = y_need + uv_need * 2;

    uint8_t *scratch = (uint8_t*)malloc(total_need);
    if (!scratch)
        return -ENOMEM;

    y_scratch = scratch;
    u_scratch = y_scratch + y_need;
    v_scratch = u_scratch + uv_need;

    // ---- 初始化 JPEG ----
    memset(&cinfo, 0, sizeof(cinfo));
    memset(&jerr, 0, sizeof(jerr));
    memset(&dest, 0, sizeof(dest));

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = jpeg_error_exit;
    if (setjmp(jerr.jump)) {
        jpeg_destroy_compress(&cinfo);
        free(scratch);
        return -EIO;
    }

    jpeg_create_compress(&cinfo);

    dest.buf = jpeg_buf;
    dest.cap = jpeg_cap;
    dest.pub.init_destination = jpeg_dest_init;
    dest.pub.empty_output_buffer = jpeg_dest_empty;
    dest.pub.term_destination = jpeg_dest_term;
    cinfo.dest = &dest.pub;

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;
    jpeg_set_defaults(&cinfo);
    jpeg_set_colorspace(&cinfo, JCS_YCbCr);
    jpeg_set_quality(&cinfo, 85, TRUE);
    cinfo.raw_data_in = TRUE;
    cinfo.dct_method = JDCT_IFAST;

    // ---- 4:2:0 采样因子 ----
    cinfo.comp_info[0].h_samp_factor = 2;
    cinfo.comp_info[0].v_samp_factor = 2;   // Y: 2x2
    cinfo.comp_info[1].h_samp_factor = 1;
    cinfo.comp_info[1].v_samp_factor = 1;   // Cb: 1x1
    cinfo.comp_info[2].h_samp_factor = 1;
    cinfo.comp_info[2].v_samp_factor = 1;   // Cr: 1x1

    jpeg_start_compress(&cinfo, TRUE);

    planes[0] = y_rows;
    planes[1] = u_rows;
    planes[2] = v_rows;

    y_src = nv12;
    uv_src = nv12 + (size_t)width * height;

    for (row = 0; row < height; row += mcu_rows) {
        // 1) 填充 Y 行（mcu_rows 行）
        for (int i = 0; i < mcu_rows; i++) {
            int src_y = row + i;
            if (src_y >= height)
                src_y = height - 1;
            const uint8_t *y_row = y_src + (size_t)src_y * width;
            uint8_t *y_out = y_scratch + (size_t)i * y_width;
            copy_y_row(y_row, y_out, width, y_width);
            y_rows[i] = y_out;
        }

        // 2) 填充 U/V 行（mcu_rows 行，其中前 DCTSIZE 行从数据拷贝，后 DCTSIZE 行复制最后一行）
        for (int i = 0; i < mcu_rows; i++) {
            int src_y;
            if (i < DCTSIZE) {
                src_y = row + i * 2;          // 取每两行的第一行
            } else {
                // 重复最后一行数据（索引 DCTSIZE-1）
                src_y = row + (DCTSIZE - 1) * 2;
            }
            if (src_y >= height)
                src_y = height - 1;

            const uint8_t *uv_row = uv_src + (size_t)(src_y / 2) * width;
            uint8_t *u_out = u_scratch + (size_t)i * uv_width;
            uint8_t *v_out = v_scratch + (size_t)i * uv_width;
            copy_uv_row(uv_row, u_out, v_out, width, y_width);
            u_rows[i] = u_out;
            v_rows[i] = v_out;
        }

        // ---- 画线（如果启用） ----
#ifdef KOKS_FEATURE_UVC_JOINT_LINE
        int line_x = 0;
        const int line_width = 4;
        auto& uvc = MediaUvc::GetInstance();
        if (uvc.NeedDrawMjpegLineV(line_x) && line_x >= 0 && line_x < width) {
            int start_x = line_x - line_width / 2;
            int end_x = line_x + line_width;
            if (start_x < 0) start_x = 0;
            if (end_x > width) end_x = width;

            // 修改所有 Y 行
            for (int i = 0; i < mcu_rows; i++) {
                uint8_t *y_out = y_rows[i];
                for (int w = start_x; w < end_x; w++)
                    y_out[w] = 0;
            }

            // 修改所有 U/V 行
            int u_start = start_x / 2;
            int u_end = (end_x - 1) / 2;
            for (int i = 0; i < mcu_rows; i++) {
                uint8_t *u_out = u_rows[i];
                uint8_t *v_out = v_rows[i];
                for (int idx = u_start; idx <= u_end; idx++) {
                    u_out[idx] = 128;
                    v_out[idx] = 128;
                }
            }
        }
#endif

        // ---- 写入 JPEG ----
        if (jpeg_write_raw_data(&cinfo, planes, mcu_rows) != mcu_rows) {
            cinfo.err->msg_code = JERR_BUFFER_SIZE;
            (*cinfo.err->error_exit)((j_common_ptr)&cinfo);
        }
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    *jpeg_size = dest.size;
    free(scratch);
    return 0;
}
#endif