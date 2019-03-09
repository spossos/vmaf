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

#ifndef FRAME_H_
#define FRAME_H_

#include <limits.h>
#include "../common/alloc.h"
#include "../common/file_io.h"

#define VMEMBUFSIZE 3

struct data
{
    char* format; /* yuv420p, yuv422p, yuv444p, yuv420p10le, yuv422p10le, yuv444p10le */
    int width;
    int height;
    size_t offset;
    FILE *ref_rfile;
    FILE *dis_rfile;
};

struct noref_data
{
    char* format; /* yuv420p, yuv422p, yuv444p, yuv420p10le, yuv422p10le, yuv444p10le */
    int width;
    int height;
    size_t offset;
    FILE *dis_rfile;
};

struct vmaf_frame_score
{
    double
        score,
        mscores[4 * 2],
        score_num,
        score_den,
        score_psnr,
        score_adm,
        score_ansnr,
        score_vif;
};

struct vmaf_frame_data
{
    float
        *frame;
    int
        frame_index,
        w,//frame width
        h;//frame height
    char
        **format;
};

struct vmaf_frame_mem
{
    struct vmaf_frame_data
        ref;
    struct vmaf_frame_data
        dis;
    t_convert_image
        p_conversion_func;

};

int read_frame(float *ref_data, float *dis_data, float *temp_data, int stride_byte, void *s);

int read_noref_frame(float *dis_data, float *temp_data, int stride_byte, void *s);

int get_frame_offset(const char *fmt, int w, int h, size_t *offset);

int fmt_multiplier(
    const char * fmt,
    int        * num,
    int        * den);

int convert_frame(
    t_convert_image   p_func,
    unsigned char   * in_frame,
    void            * mem_data,
    int               stride_byte);

int vmem_alloc(
    struct vmaf_frame_data * vmem,
    struct data * in,
    size_t data_sz);

void vmem_free(
    struct vmaf_frame_data * vmem);

int init_vmaf_mem(
    void            * m,
    void            * s,
    void            * d,
    int               cpuVal,
    const char      * fmt);

void free_vmaf_mem(
    void *m);

#endif /* FRAME_H_ */
