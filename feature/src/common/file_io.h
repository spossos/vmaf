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

#pragma once

#ifndef FILE_IO_H_
#define FILE_IO_H_

/* Whether to use [0,255] or [-128,127] input pixel range. */
//#define OPT_RANGE_PIXEL_OFFSET 0
#define OPT_RANGE_PIXEL_OFFSET (-128)

#define CPU_DISP_INIT_C(func, bdepth)                  (func = (func ## _ ## bdepth ## _C))
#define CPU_DISP_INIT_SSE4(func, bdepth)               (func = (func ## _ ## bdepth ## _SSE4))
#define CPU_DISP_INIT_SSE4_C(cpuCap, bdepth, func)     (cpuCap >= VMAF_CPU_SSE4 ? CPU_DISP_INIT_SSE4(func, bdepth) : CPU_DISP_INIT_C(func, bdepth))

#define CPU_DISP_INIT_AVX(func, bdepth)                (func = (func ## _ ## bdepth ## _AVX))
#define CPU_DISP_INIT_AVX_SSE4_C(cpuCap, bdepth, func) (cpuCap == VMAF_CPU_AVX ? CPU_DISP_INIT_AVX(func, bdepth) : CPU_DISP_INIT_SSE4_C(cpuCap, func))

float apply_frame_differencing(const float *current_frame, const float *previous_frame, float *frame_difference, int width, int height, int stride);
int read_image(FILE *rfile, void *buf, int width, int height, int stride, int elem_size);
int write_image(FILE *wfile, const void *buf, int width, int height, int stride, int elem_size);

int read_image_b2s(FILE *rfile, float *buf, float off, int width, int height, int stride);

int read_image_w2s(FILE *rfile, float *buf, float off, int width, int height, int stride);

int offset_image_s(float *buf, float off, int width, int height, int stride);


typedef int(*t_convert_image)(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride);

int convert_image_b2s_C(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride);
int convert_image_b2s_SSE4(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride);
int convert_image_b2s_AVX(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride);

int convert_image_w2s_C(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride);
int convert_image_w2s_SSE4(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride);
int convert_image_w2s_AVX2(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride);

int convert_image_b2s(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride);

int convert_image_w2s(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride);
#endif /* FILE_IO_H_ */
