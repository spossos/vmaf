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
#include <stdlib.h>
#include <assert.h>
#include <immintrin.h>
#include "cpu.h"

/**
 * Note: stride is in terms of bytes
 */
float apply_frame_differencing(const float *current_frame, const float *previous_frame, float *frame_difference, int width, int height, int stride)
{
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            frame_difference[i * stride + j] = current_frame[i * stride + j] - previous_frame[i * stride + j];
        }
    }
}

/**
 * Note: stride is in terms of bytes
 */
int read_image(FILE *rfile, void *buf, int width, int height, int stride, int elem_size)
{
	char *byte_ptr = buf;
	int i;
	int ret = 1;

	if (width <= 0 || height <= 0 || elem_size <= 0)
	{
		goto fail_or_end;
	}

	for (i = 0; i < height; ++i)
	{
		if (fread(byte_ptr, elem_size, width, rfile) != (size_t)width)
		{
			goto fail_or_end;
		}

		byte_ptr += stride;
	}

	ret = 0;

fail_or_end:
	return ret;
}

/**
 * Note: stride is in terms of bytes
 */
int write_image(FILE *wfile, const void *buf, int width, int height, int stride, int elem_size)
{
	const char *byte_ptr = buf;
	int i;
	int ret = 1;

	if (width <= 0 || height <= 0 || elem_size <= 0)
	{
		goto fail_or_end;
	}

	for (i = 0; i < height; ++i)
	{
		if (fwrite(byte_ptr, elem_size, width, wfile) != (size_t)width)
		{
			goto fail_or_end;
		}

		byte_ptr += stride;
	}

	ret = 0;

fail_or_end:
	return ret;
}

/**
 * Note: stride is in terms of bytes
 */
int read_image_b2s(FILE * rfile, float *buf, float off, int width, int height, int stride)
{
	char *byte_ptr = (char *)buf;
	unsigned char *tmp_buf = 0;
	int i, j;
	int ret = 1;

	if (width <= 0 || height <= 0)
	{
		goto fail_or_end;
	}

	if (!(tmp_buf = malloc(width)))
	{
		goto fail_or_end;
	}

	for (i = 0; i < height; ++i)
	{
		float *row_ptr = (float *)byte_ptr;

		if (fread(tmp_buf, 1, width, rfile) != (size_t)width)
		{
			goto fail_or_end;
		}

		for (j = 0; j < width; ++j)
		{
			row_ptr[j] = tmp_buf[j] + off;
		}

		byte_ptr += stride;
	}

	ret = 0;

fail_or_end:
	free(tmp_buf);
	return ret;
}

/**
 * Note: stride is in terms of bytes; image is 10-bit little-endian
 */
int read_image_w2s(FILE * rfile, float *buf, float off, int width, int height, int stride)
{
	// make sure unsigned short is 2 bytes
	assert(sizeof(unsigned short) == 2);

	char *byte_ptr = (char *)buf;
	unsigned short *tmp_buf = 0;
	int i, j;
	int ret = 1;

	if (width <= 0 || height <= 0)
	{
		goto fail_or_end;
	}

	if (!(tmp_buf = malloc(width * 2))) // '*2' to accommodate words
	{
		goto fail_or_end;
	}

	for (i = 0; i < height; ++i)
	{
		float *row_ptr = (float *)byte_ptr;

		if (fread(tmp_buf, 2, width, rfile) != (size_t)width) // '2' for word
		{
			goto fail_or_end;
		}

		for (j = 0; j < width; ++j)
		{
			row_ptr[j] = tmp_buf[j] / 4.0 + off; // '/4' to convert from 10 to 8-bit
		}

		byte_ptr += stride;
	}

	ret = 0;

fail_or_end:
	free(tmp_buf);
	return ret;
}

/**
 * Note: stride is in terms of bytes
 */
int offset_image_s(float *buf, float off, int width, int height, int stride)
{
	char *byte_ptr = (char *)buf;
	int ret = 1;
	int i, j;

	for (i = 0; i < height; ++i)
	{
		float *row_ptr = (float *)byte_ptr;

		for (j = 0; j < width; ++j)
		{
			row_ptr[j] += off;
		}

		byte_ptr += stride;
	}

	ret = 0;

	return ret;
}
/**
* Note: stride is in terms of bytes
*/
int convert_image_b2s_C(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride)
{
    char *byte_ptr = (char *)buf;
    int i, j;
    int ret = 1;

    if (width <= 0 || height <= 0)
    {
        goto fail_or_end;
    }

    for (i = 0; i < height; ++i)
    {
        float *row_ptr = (float *)byte_ptr;

        for (j = 0; j < width; ++j)
        {
            row_ptr[j] = frame[j] + off;
        }

        byte_ptr += stride;
    }

    ret = 0;

fail_or_end:

    return ret;
}

int convert_image_b2s_SSE4(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride)
{
    char
        *byte_ptr = (char *)buf;
    int
        i;
    __m128i
        *intPx;
    __m128
        *fltPx;
    int
        ret = 1;

    if (width <= 0 || height <= 0)
    {
        goto fail_or_end;
    }

    for (i = 0; i < width * height - 3; i += 4)
    {
        fltPx = (__m128*) &buf[i];
        intPx = (__m128i*) &frame[i];
        *fltPx = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(*intPx));
    }
    for (; i < width * height; i++)
        buf[i] = (float)frame[i];

    ret = 0;

fail_or_end:

    return ret;
}

int convert_image_b2s_AVX(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride)
{
    char
        *byte_ptr = (char *)buf;
    int
        i,
        j;
    __m256i
        pxl32;
    __m256
        *fltPx;
    __m128i
        *_intPx;
    __m128
        *_fltPx;
    int
        ret = 1;

    if (width <= 0 || height <= 0)
    {
        goto fail_or_end;
    }



    for (i = 0; i < width * height - 7; i += 8)
    {
        fltPx  = (__m256*) &buf[i];
        pxl32  = _mm256_cvtepu8_epi32(_mm_loadl_epi64((__m128i *) frame[i]));
        *fltPx = _mm256_cvtepi32_ps(pxl32);
    }
    for (; i < width * height - 3; i += 4)//remaining pixels
    {
        _fltPx = (__m128*) &buf[i];
        _intPx = (__m128i*) &frame[i];
        *_fltPx = _mm_cvtepi32_ps(_mm_cvtepu8_epi32(*_intPx));
    }
    for (; i < width * height; i++)//remaining pixels
        buf[i] = (float)frame[i];

    ret = 0;

fail_or_end:

    return ret;
}

int convert_image_w2s_C(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride)
{
    // make sure unsigned short is 2 bytes
    assert(sizeof(unsigned short) == 2);

    char *byte_ptr = (char *)buf;
    unsigned short *tmp_frame = (unsigned short*)frame;
    int i, j;
    int ret = 1;

    if (width <= 0 || height <= 0)
    {
        goto fail_or_end;
    }

    for (i = 0; i < height; ++i)
    {
        float *row_ptr = (float *)byte_ptr;

        for (j = 0; j < width; ++j)
        {
            row_ptr[j] = tmp_frame[j] + off;
        }

        byte_ptr += stride;
    }

    ret = 0;

fail_or_end:
    return ret;
}

int convert_image_w2s_SSE4(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride)
{
    // make sure unsigned short is 2 bytes
    assert(sizeof(unsigned short) == 2);

    int
        i,
        ret = 1;

    __m128i
        pxl32,
        *p_intPx = (__m128*) buf;
    __m128
        *p_fltPx = (__m128i*) frame;

    if (width <= 0 || height <= 0)
    {
        goto fail_or_end;
    }

    for (i = 0; i < width * height - 1; i += 2)
    {
        *p_fltPx = _mm_cvtepi32_ps(_mm_cvtepu16_epi32(*p_intPx));
        p_fltPx += 2;
        p_intPx += 2;
    }
    for (; i < width * height; i++) //Remaining pixels
        buf[i] = (float)frame[i];

    ret = 0;

fail_or_end:
    return ret;
}

int convert_image_w2s_AVX2(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride)
{
    // make sure unsigned short is 2 bytes
    assert(sizeof(unsigned short) == 2);
    int
        i,
        j,
        ret = 1;
    __m128i
        *p_intPx;
    __m128
        *p_fltSPx;
    __m256i
        pxl32;
    __m256
        *p_fltPx = (__m256*) buf;

    if (width <= 0 || height <= 0)
    {
        goto fail_or_end;
    }

    for (i = 0; i < width * height - 3; i += 4)
    {
        pxl32  = _mm256_cvtepu16_epi32(_mm_loadl_epi64((__m128i *) frame[i]));
        *p_fltPx = _mm256_cvtepi32_ps(pxl32);
        p_fltPx += 4;
    }
    for (; i < width * height - 1; i += 2)//Remaining pixels
    {
        p_fltSPx  = (__m128 *) &buf[i];
        p_intPx   = (__m128i *) &frame[i];
        *p_fltSPx = _mm_cvtepi32_ps(_mm_cvtepu16_epi32(*p_intPx));
    }
    for (; i < width * height; i++)       //Remaining pixels
        buf[i] = (float)frame[i];

    ret = 0;

fail_or_end:
    return ret;
}

int convert_image_b2s(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride)
{
    char *byte_ptr = (char *)buf;
    int i, j;
    int ret = 1;

    if (width <= 0 || height <= 0)
    {
        goto fail_or_end;
    }

    for (i = 0; i < height; ++i)
    {
        float *row_ptr = (float *)byte_ptr;

        for (j = 0; j < width; ++j)
        {
            row_ptr[j] = frame[j] + off;
        }
        byte_ptr += stride;
    }

    ret = 0;

fail_or_end:

    return ret;
}




int convert_image_w2s(
    unsigned char * frame,
    float         * buf,
    float           off,
    int             width,
    int             height,
    int             stride)
{
    // make sure unsigned short is 2 bytes
    assert(sizeof(unsigned short) == 2);

    char *byte_ptr = (char *)buf;
    unsigned short *tmp_frame = (unsigned short*)frame;
    int i, j;
    int ret = 1;

    if (width <= 0 || height <= 0)
    {
        goto fail_or_end;
    }

    for (i = 0; i < height; ++i)
    {
        float *row_ptr = (float *)byte_ptr;

        for (j = 0; j < width; ++j)
        {
            row_ptr[j] = tmp_frame[j] + off;
        }

        byte_ptr += stride;
    }

    ret = 0;

fail_or_end:
    return ret;
}