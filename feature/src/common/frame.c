/**
 *
 *  Copyright 2016-2019 Netflix, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 *
 */

#include <stdio.h>
#include <string.h>
#include "file_io.h"
#include "frame.h"
#include "cpu.h"

#define read_image_b       read_image_b2s
#define read_image_w       read_image_w2s

int read_frame(float *ref_data, float *dis_data, float *temp_data, int stride_byte, void *s)
{
    struct data *user_data = (struct data *)s;
    char *fmt = user_data->format;
    int w = user_data->width;
    int h = user_data->height;
    int ret;

    // read ref y
    if (!strcmp(fmt, "yuv420p") || !strcmp(fmt, "yuv422p") || !strcmp(fmt, "yuv444p"))
    {
        ret = read_image_b(user_data->ref_rfile, ref_data, 0, w, h, stride_byte);
    }
    else if (!strcmp(fmt, "yuv420p10le") || !strcmp(fmt, "yuv422p10le") || !strcmp(fmt, "yuv444p10le"))
    {
        ret = read_image_w(user_data->ref_rfile, ref_data, 0, w, h, stride_byte);
    }
    else
    {
        fprintf(stderr, "unknown format %s.\n", fmt);
        return 1;
    }
    if (ret)
    {
        if (feof(user_data->ref_rfile))
        {
            ret = 2; // OK if end of file
        }
        return ret;
    }

    // read dis y
    if (!strcmp(fmt, "yuv420p") || !strcmp(fmt, "yuv422p") || !strcmp(fmt, "yuv444p"))
    {
        ret = read_image_b(user_data->dis_rfile, dis_data, 0, w, h, stride_byte);
    }
    else if (!strcmp(fmt, "yuv420p10le") || !strcmp(fmt, "yuv422p10le") || !strcmp(fmt, "yuv444p10le"))
    {
        ret = read_image_w(user_data->dis_rfile, dis_data, 0, w, h, stride_byte);
    }
    else
    {
        fprintf(stderr, "unknown format %s.\n", fmt);
        return 1;
    }
    if (ret)
    {
        if (feof(user_data->dis_rfile))
        {
            ret = 2; // OK if end of file
        }
        return ret;
    }

    // ref skip u and v
    if (!strcmp(fmt, "yuv420p") || !strcmp(fmt, "yuv422p") || !strcmp(fmt, "yuv444p"))
    {
        if (fread(temp_data, 1, user_data->offset, user_data->ref_rfile) != (size_t)user_data->offset)
        {
            fprintf(stderr, "ref fread u and v failed.\n");
            goto fail_or_end;
        }
    }
    else if (!strcmp(fmt, "yuv420p10le") || !strcmp(fmt, "yuv422p10le") || !strcmp(fmt, "yuv444p10le"))
    {
        if (fread(temp_data, 2, user_data->offset, user_data->ref_rfile) != (size_t)user_data->offset)
        {
            fprintf(stderr, "ref fread u and v failed.\n");
            goto fail_or_end;
        }
    }
    else
    {
        fprintf(stderr, "unknown format %s.\n", fmt);
        goto fail_or_end;
    }

    // dis skip u and v
    if (!strcmp(fmt, "yuv420p") || !strcmp(fmt, "yuv422p") || !strcmp(fmt, "yuv444p"))
    {
        if (fread(temp_data, 1, user_data->offset, user_data->dis_rfile) != (size_t)user_data->offset)
        {
            fprintf(stderr, "dis fread u and v failed.\n");
            goto fail_or_end;
        }
    }
    else if (!strcmp(fmt, "yuv420p10le") || !strcmp(fmt, "yuv422p10le") || !strcmp(fmt, "yuv444p10le"))
    {
        if (fread(temp_data, 2, user_data->offset, user_data->dis_rfile) != (size_t)user_data->offset)
        {
            fprintf(stderr, "dis fread u and v failed.\n");
            goto fail_or_end;
        }
    }
    else
    {
        fprintf(stderr, "unknown format %s.\n", fmt);
        goto fail_or_end;
    }


fail_or_end:
    return ret;
}

int read_noref_frame(float *dis_data, float *temp_data, int stride_byte, void *s)
{
    struct noref_data *user_data = (struct noref_data *)s;
    char *fmt = user_data->format;
    int w = user_data->width;
    int h = user_data->height;
    int ret;

    // read dis y
    if (!strcmp(fmt, "yuv420p") || !strcmp(fmt, "yuv422p") || !strcmp(fmt, "yuv444p"))
    {
        ret = read_image_b(user_data->dis_rfile, dis_data, 0, w, h, stride_byte);
    }
    else if (!strcmp(fmt, "yuv420p10le") || !strcmp(fmt, "yuv422p10le") || !strcmp(fmt, "yuv444p10le"))
    {
        ret = read_image_w(user_data->dis_rfile, dis_data, 0, w, h, stride_byte);
    }
    else
    {
        fprintf(stderr, "unknown format %s.\n", fmt);
        return 1;
    }
    if (ret)
    {
        if (feof(user_data->dis_rfile))
        {
            ret = 2; // OK if end of file
        }
        return ret;
    }

    // dis skip u and v
    if (!strcmp(fmt, "yuv420p") || !strcmp(fmt, "yuv422p") || !strcmp(fmt, "yuv444p"))
    {
        if (fread(temp_data, 1, user_data->offset, user_data->dis_rfile) != (size_t)user_data->offset)
        {
            fprintf(stderr, "dis fread u and v failed.\n");
            goto fail_or_end;
        }
    }
    else if (!strcmp(fmt, "yuv420p10le") || !strcmp(fmt, "yuv422p10le") || !strcmp(fmt, "yuv444p10le"))
    {
        if (fread(temp_data, 2, user_data->offset, user_data->dis_rfile) != (size_t)user_data->offset)
        {
            fprintf(stderr, "dis fread u and v failed.\n");
            goto fail_or_end;
        }
    }
    else
    {
        fprintf(stderr, "unknown format %s.\n", fmt);
        goto fail_or_end;
    }


fail_or_end:
    return ret;
}

int get_frame_offset(const char *fmt, int w, int h, size_t *offset)
{
    if (!strcmp(fmt, "yuv420p") || !strcmp(fmt, "yuv420p10le"))
    {
        if ((w * h) % 2 != 0)
        {
            fprintf(stderr, "(width * height) %% 2 != 0, width = %d, height = %d.\n", w, h);
            return 1;
        }
        *offset = w * h / 2;
    }
    else if (!strcmp(fmt, "yuv422p") || !strcmp(fmt, "yuv422p10le"))
    {
        *offset = w * h;
    }
    else if (!strcmp(fmt, "yuv444p") || !strcmp(fmt, "yuv444p10le"))
    {
        *offset = w * h * 2;
    }
    else
    {
        fprintf(stderr, "unknown format %s.\n", fmt);
        return 1;
    }
    return 0;
}

int fmt_multiplier(
    const char * fmt,
    int        * num,
    int        * den)
{
    // read ref y
    if (!strcmp(fmt, "yuv420p"))
    {
        *num = 3;
        *den = 2;
    }
    else if (!strcmp(fmt, "yuv422p"))
    {
        *num = 2;
        *den = 1;
    }
    else if (!strcmp(fmt, "yuv444p"))
    {
        *num = 3;
        *den = 1;
    }
    else if (!strcmp(fmt, "yuv420p10le"))
    {
        *num = 3 * 2;
        *den = 2;
    }
    else if (!strcmp(fmt, "yuv422p10le"))
    {
        *num = 2 * 2;
        *den = 1;
    }
    else if (!strcmp(fmt, "yuv444p10le"))
    {
        *num = 3 * 2;
        *den = 1;
    }
    else
    {
        fprintf(stderr, "unknown format %s.\n", fmt);
        return 1;
    }
    return 0;
}

int convert_frame(
    t_convert_image   p_func,
    unsigned char   * in_frame,
    void            * mem_data,
    int               stride_byte)
{
    struct vmaf_frame_data
        * vmem = mem_data;
    float
        * converted_data = vmem->frame;
    char
        * fmt = * vmem->format;
    int
        w = vmem->w,
        h = vmem->h;
    int ret;

    // read ref y
    ret = (*p_func)(in_frame, converted_data, 0, w, h, stride_byte);
    // in_frame skip u and v
fail_or_end:
    return ret;
}

int vmem_alloc(
    struct vmaf_frame_data * fmem,
    struct data            * in,
    size_t                   data_sz)
{
    if (!data_sz)
        return 1;

    fmem->h = in->height;
    fmem->w = in->width;
    fmem->format = &in->format;
    if (fmem->h == 0 || fmem->w == 0)
        return 1;
    fmem->frame = (float*)aligned_malloc(data_sz, MAX_ALIGN);
    if (fmem->frame == NULL)
        return 1;
    memset(fmem->frame, 0, data_sz);
    return 0;
}

void vmem_free(
    struct vmaf_frame_data *fmem)
{
    if(fmem->frame)
        aligned_free(fmem->frame);
}

int init_vmaf_mem(
    void            * m,
    void            * s,
    void            * d,
    int               cpuVal,
    const char      * fmt)
{
    struct data 
        *in = d;
    struct vmaf_frame_mem
        *vmem = m;
    struct vmaf_frame_score
        *scores = s;
    t_convert_image
        ** convert_image = &vmem->p_conversion_func;

    if (!strcmp(fmt, "yuv420p") || !strcmp(fmt, "yuv422p") || !strcmp(fmt, "yuv444p"))
        CPU_DISP_INIT_SSE4_C(cpuVal, b2s, *convert_image);
    else if (!strcmp(fmt, "yuv420p10le") || !strcmp(fmt, "yuv422p10le") || !strcmp(fmt, "yuv444p10le"))
        CPU_DISP_INIT_SSE4_C(cpuVal, w2s, *convert_image);
    else
    {
        fprintf(stderr, "unknown format %s.\n", fmt);
        return 1;
    }

    scores->score = 0.0;
    scores->score_num = 0.0;
    scores->score_den = 0.0;
    scores->score_psnr = 0.0;
    memset(scores->mscores, 0, sizeof(scores->mscores));

    if (in->width <= 0 || in->height <= 0 || in->width > ALIGN_FLOOR(INT_MAX) / sizeof(float))
    {
        return 1;
    }

    int
        stride = ALIGN_CEIL(in->width * sizeof(float));
#if defined(_WIN64) || defined(_WIN32)
    if ((size_t)in->height > SIZE_MAX / stride)
#else
    if ((size_t)in->height > ULLONG_MAX / stride)
#endif
    {
        return 1;
    }

    size_t
        data_sz = (size_t)stride * in->height;
    if (data_sz == 0)
        return 1;

    for (int i = 0; i < VMEMBUFSIZE; i++)
    {
        if (vmem_alloc(&vmem->ref, in, data_sz))
            return 1;
        if (vmem_alloc(&vmem->dis, in, data_sz))
            return 1;
    }
    vmem->ref.frame_index = -1;
    vmem->dis.frame_index = -1;
    return 0;
}

void free_vmaf_mem(
    void *m)
{
    struct vmaf_frame_mem
        *vmem = m;
    vmem_free(&vmem->ref);
    vmem_free(&vmem->dis);
}
