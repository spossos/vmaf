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
#include <string.h>
#include <stdbool.h>

#include "common/frame.h"
#include "common/cpu.h"

#define FRAMEVMAF 1

int adm(int(*read_frame)(float *ref_data, float *main_data, float *temp_data, int stride, void *user_data), void *user_data, int w, int h, const char *fmt);
int ansnr(int(*read_frame)(float *ref_data, float *main_data, float *temp_data, int stride, void *user_data), void *user_data, int w, int h, const char *fmt);
int vif(int(*read_frame)(float *ref_data, float *main_data, float *temp_data, int stride, void *user_data), void *user_data, int w, int h, const char *fmt);
int vifdiff(int(*read_frame)(float *ref_data, float *main_data, float *temp_data, int stride, void *user_data), void *user_data, int w, int h, const char *fmt);
int motion(int(*read_noref_frame)(float *main_data, float *temp_data, int stride, void *user_data), void *user_data, int w, int h, const char *fmt);
int all(int(*read_frame)(float *ref_data, float *main_data, float *temp_data, int stride, void *user_data), void *user_data, int w, int h, const char *fmt);
int all_frame(
    float      * ref_frame,
    float      * dis_frame,
    void       * frame_scores,
    int          frm_idx,
    int          w,
    int          h,
    const char * fmt,
    bool         print_all);

enum vmaf_cpu cpu; // global

static void usage(void)
{
#if FRAMEVMAF
    puts("usage: vmaf fmt ref dis w h\n"
        "fmts:\n"
        "\tyuv420p\n"
        "\tyuv422p\n"
        "\tyuv444p\n"
        "\tyuv420p10le\n"
        "\tyuv422p10le\n"
        "\tyuv444p10le"
    );
#else
    puts("usage: vmaf app fmt ref dis w h\n"
        "apps:\n"
        "\tadm\n"
        "\tansnr\n"
        "\tmotion\n"
        "\tvif\n"
        "\tvifdiff\n"
        "\tall\n"
        "fmts:\n"
        "\tyuv420p\n"
        "\tyuv422p\n"
        "\tyuv444p\n"
        "\tyuv420p10le\n"
        "\tyuv422p10le\n"
        "\tyuv444p10le"
    );
#endif
}

int run_vmaf(const char *app, const char *fmt, const char *ref_path, const char *dis_path, int w, int h)
{
    int ret = 0;
    cpu = cpu_autodetect();

    if (!strcmp(app, "motion"))
    {
        struct noref_data *s;
        s = (struct noref_data *)malloc(sizeof(struct noref_data));
        s->format = fmt;
        s->width = w;
        s->height = h;

        ret = get_frame_offset(fmt, w, h, &(s->offset));
        if (ret)
        {
            goto fail_or_end_noref;
        }

        /* NOTE: below is legitimate: noref_data has a field called dis_rfile,
        * but what's needed to be passed to motion is ref_path!
        */
        if (!(s->dis_rfile = fopen(ref_path, "rb")))
        {
            fprintf(stderr, "fopen ref_path %s failed.\n", ref_path);
            ret = 1;
            goto fail_or_end_noref;
        }

        ret = motion(read_noref_frame, s, w, h, fmt);

    fail_or_end_noref:
        if (s->dis_rfile)
        {
            fclose(s->dis_rfile);
        }
        if (s)
        {
            free(s);
        }
        return ret;
    }
    else
    {
        struct data *s;
        s = (struct data *)malloc(sizeof(struct data));
        s->format = fmt;
        s->width = w;
        s->height = h;

        ret = get_frame_offset(fmt, w, h, &(s->offset));
        if (ret)
        {
            goto fail_or_end;
        }

        if (!(s->ref_rfile = fopen(ref_path, "rb")))
        {
            fprintf(stderr, "fopen ref_path %s failed.\n", ref_path);
            ret = 1;
            goto fail_or_end;
        }
        if (!(s->dis_rfile = fopen(dis_path, "rb")))
        {
            fprintf(stderr, "fopen ref_path %s failed.\n", dis_path);
            ret = 1;
            goto fail_or_end;
        }

        if (!strcmp(app, "adm"))
            ret = adm(read_frame, s, w, h, fmt);
        else if (!strcmp(app, "ansnr"))
            ret = ansnr(read_frame, s, w, h, fmt);
        else if (!strcmp(app, "vif"))
            ret = vif(read_frame, s, w, h, fmt);
        else if (!strcmp(app, "all"))
            ret = all(read_frame, s, w, h, fmt);
        else if (!strcmp(app, "vifdiff"))
            ret = vifdiff(read_frame, s, w, h, fmt);
        else
            ret = 2;

    fail_or_end:
        if (s->ref_rfile)
        {
            fclose(s->ref_rfile);
        }
        if (s->dis_rfile)
        {
            fclose(s->dis_rfile);
        }
        if (s)
        {
            free(s);
        }
        return ret;

    }
}

int run_vmaf_frame(unsigned char *ref_frame, unsigned char *dis_frame, void *m, void *frame_scores, int frame_idx)
{
    struct vmaf_frame_score
        * scores = frame_scores;
    struct vmaf_frame_mem
        * vmem = m;
    int
        ret = 0,
        w = vmem->ref.w,
        h = vmem->ref.h,
        stride = ALIGN_CEIL(w * sizeof(float));
    char
        * fmt = *vmem->ref.format;

    if ((size_t)h > SIZE_MAX / stride)
    {
        return 1;
    };

    ret = convert_frame(
        vmem->p_conversion_func,
        ref_frame,
        &vmem->ref,
        stride
    );
    if (ret)
        return ret;

    ret = convert_frame(
        vmem->p_conversion_func,
        dis_frame,
        &vmem->dis,
        stride
    );
    if (ret)
        return ret;

    ret = all_frame(
        vmem->ref.frame,
        vmem->dis.frame,
        (void *)scores,
        frame_idx,
        w,
        h,
        fmt,
        false);

    return ret;
}

int main(int argc, const char **argv)
{
#if FRAMEVMAF
    const char
        * ref_path,
        *dis_path,
        *fmt;
    int
        w,
        h,
        num,
        den,
        data_sz,
        frame_num = 0,
        ret = 0;

    if (argc < 6) {
        usage();
        return 2;
    }

    fmt = argv[1];
    ref_path = argv[2];
    dis_path = argv[3];
    w = atoi(argv[4]);
    h = atoi(argv[5]);

    if (w <= 0 || h <= 0) {
        usage();
        return 2;
    }
    /*Determine CPU caps*/
    cpu = cpu_autodetect();
    /*Frame pointers*/
    unsigned char
        * ref_frame = NULL,
        *dis_frame = NULL;
    /*Frame metric scores collector*/
    struct vmaf_frame_score
        frame_scores;
    /*Memory structure for VMAF operation*/
    struct vmaf_frame_mem
        vmem;
    vmem.ref.frame = NULL;
    vmem.dis.frame = NULL;
    /*User data*/
    struct data
        * d = NULL;
    /*Memory allocation and initialization of User data*/
    d = (struct data *)malloc(sizeof(struct data));
    if (!d)
    {
        ret = 1;
        goto fail_or_end;
    }
    d->format = fmt;
    d->width = w;
    d->height = h;
    d->ref_rfile = NULL;
    d->dis_rfile = NULL;
    /*Opening of yuv video files*/
    if (!(d->ref_rfile = fopen(ref_path, "rb")))
    {
        fprintf(stderr, "fopen ref_path %s failed.\n", ref_path);
        ret = 1;
        goto fail_or_end;
    }
    if (!(d->dis_rfile = fopen(dis_path, "rb")))
    {
        fprintf(stderr, "fopen dis_path %s failed.\n", dis_path);
        ret = 1;
        goto fail_or_end;
    }
    /*Define frame size modifier based on Chroma format*/
    ret = fmt_multiplier(
        fmt,
        &num,
        &den
    );
    if (ret)
        goto fail_or_end;
    /*Frame size calculation based on user inputs and chroma fmt*/
    data_sz = w * h * num / den;
    /*VMAF memory allocation and initialization*/
    ret = init_vmaf_mem(
        &vmem,
        &frame_scores,
        d,
        cpu,
        fmt);
    if (ret)
        goto fail_or_end;
    /*Input frame allocation*/
    ref_frame = (unsigned char *)aligned_malloc(data_sz, MAX_ALIGN);//Reference or original frame to compare against
    dis_frame = (unsigned char *)aligned_malloc(data_sz, MAX_ALIGN);//Distorted or processed frame to compare
    if (ref_frame == NULL || dis_frame == NULL)
    {
        ret = 1;
        goto fail_or_end;
    }
    /*Main loop - Read frames from files - Measure VMAF between the two frames*/
    while ((fread(ref_frame, 1, data_sz, d->ref_rfile) > (data_sz - 1)) &&
        (fread(dis_frame, 1, data_sz, d->dis_rfile) > (data_sz - 1)))
    {
        ret = run_vmaf_frame(
            ref_frame,
            dis_frame,
            &vmem,
            &frame_scores,
            frame_num);
        if (ret)
            goto fail_or_end;
        frame_num++;
    }

fail_or_end:
    /*Free all memory and close of all YUV files*/
    free_vmaf_mem(&vmem);
    if (d != NULL)
    {
        if (d->ref_rfile != NULL)
            fclose(d->ref_rfile);
        if (d->dis_rfile != NULL)
            fclose(d->dis_rfile);
        free(d);
    }
    if (ref_frame != NULL)
        aligned_free(ref_frame);
    if (dis_frame != NULL)
        aligned_free(dis_frame);
    return ret;
#else
    const char *app;
    const char *ref_path;
    const char *dis_path;
    const char *fmt;
    int w;
    int h;

    if (argc < 7) {
        usage();
        return 2;
    }

    app = argv[1];
    fmt = argv[2];
    ref_path = argv[3];
    dis_path = argv[4];
    w = atoi(argv[5]);
    h = atoi(argv[6]);

    if (w <= 0 || h <= 0) {
        usage();
        return 2;
    }

    return run_vmaf(app, fmt, ref_path, dis_path, w, h);
#endif
}