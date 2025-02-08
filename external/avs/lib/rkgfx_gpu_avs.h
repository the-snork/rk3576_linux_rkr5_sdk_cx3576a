/****************************************************************************
*
*    Copyright (c) 2020 - 2021 by Rockchip Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Rockchip Corporation. This is proprietary information owned by
*    Rockchip Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Rockchip Corporation.
*
*****************************************************************************/

#ifndef RKGFX_GPU_AVS_H
#define RKGFX_GPU_AVS_H

#include <vector>

typedef unsigned int uint32_t;

typedef enum rkPSR_FORMAT_E {
    PSR_TYPE_GRAY     = 0,
    PSR_TYPE_RGB      = 1,
    PSR_TYPE_YUV420SP = 2,         // 仅支持NV12格式，其他为预留，未支持
} PSR_FORMAT_E;

typedef enum rkPSR_FUSE_TYPE_E {
    FUSE_ALPHA      = 0,              // alpha融合,当频段数为0时为alpha融合，未支持
    FUSE_MULTI_BAND = 1,              // 多频段融合
} PSR_FUSE_TYPE_E;

typedef struct rkPSR_BLOCK_GAINS_PARAMS { //亮度矫正使用，预留，未支持
    int input_image_width;
    int input_image_height;
    int input_image_stride;
    int input_image_channel;
    int block_image_width;
    int block_image_height;
    int gain_step_x;
    int gain_step_y;
} PSR_BLOCK_GAINS_PARAMS;

typedef struct rkPSR_OVERLAP_BLOCK {
    int overlap_num;                 // 重叠区域的个数
    std::vector<int> band_num;       // 图像进行多频段融合时分成的频段数
    std::vector<int> dst_startx;     // 通过表校正后输出图的起始位置
    std::vector<int> dst_width;      // 通过表校正后输出图的宽度
    std::vector<int> dst_stride;     // 通过表校正后输出图的stride
    std::vector<int> mesh_width;     // mesh表的宽度
    std::vector<int> mesh_height;    // mesh表的高度
    std::vector<int> alpha_width;    // alpha表的宽度(第0层)
    std::vector<int> alpha_height;   // alpha表的高度(第0层)
} PSR_OVERLAP_BLOCK;

typedef struct rkPSR_NONOVERLAP_BLOCK {
    int nonoverlap_num;              // 非重叠区域的个数
    std::vector<int> dst_startx;     // 通过表校正后输出图的起始位置
    std::vector<int> dst_width;      // 通过表校正后输出图的宽度
    std::vector<int> mesh_width;     // mesh表的宽度
    std::vector<int> mesh_height;    // mesh表的高度
} PSR_NONOVERLAP_BLOCK;

typedef struct PSR_INITPARAMS_S {
    int camera_num;                                     //相机的个数
    int image_stride;                                   //图像的stride
    unsigned int output_image_stride;                   //输出图像的stride
    PSR_FORMAT_E image_formate;                         //图像的格式
    int src_width;                                      //输入图的宽度
    int src_height;                                     //输入图的高度
    int bld_width;                                      //融合后图的宽度
    int bld_height;                                     //融合后图的高度
    int step_x;                                         //生成mesh表时宽度方向的步长
    int step_y;                                         //生成mesh表时高度方向的步长
    std::vector<PSR_NONOVERLAP_BLOCK> nonoverlap_block; //非重叠区域信息
    std::vector<PSR_OVERLAP_BLOCK> overlap_block;       //重叠区域信息
    std::vector<int> image_block_num;                   //每帧图有几个mesh表，即该帧图需要分成几张图进行计算

    //亮度校正所需的配置参数
    uint32_t use_gain;                                  //是否使用gain变换进行亮度校正
    int gain_step_x;                                    //分块亮度校正时x方向的步长
    int gain_step_y;                                    //分块亮度校正时y方向的步长

    uint32_t useSync;
} PSR_INITPARAMS_T;

typedef struct RKGFX_AVS_CONTEXT_S {
    uint32_t  apiVersion;
    uint32_t  numView;
    char     *meshFilePath;
    uint32_t  inputBufferWidth;
    uint32_t  inputBufferHeight;
    uint32_t  inputBufferFormat;
    uint32_t  inputIsAFBC;
    uint32_t  outputBufferWidth;
    uint32_t  outputBufferHeight;
    uint32_t  outputBufferFormat;
    uint32_t  outputIsAFBC;
    uint32_t  useFd;
    int      *inputFdArray;
    uint32_t  useSync;
    uint32_t  useMeshBufAddress;
    void *AVS_initParams;
    void **meshxBufAddressArray;
    void **meshyBufAddressArray;
    void **meshAlphaBufAddressArray;
} RKGFX_AVS_CONTEXT_T;

typedef enum {
    RKGFX_AVS_SUCCESS,
    RKGFX_AVS_INVALID_PARAM,
    RKGFX_AVS_FAILED,
} RKGFX_AVS_STATUS;

#ifdef __cplusplus
extern "C"
{
#endif

RKGFX_AVS_STATUS RKGFX_AVS_init(void* context);
RKGFX_AVS_STATUS RKGFX_AVS_AllProcess(void* context);
RKGFX_AVS_STATUS RKGFX_AVS_resize (void* context);
RKGFX_AVS_STATUS RKGFX_AVS_deinit(void* context);
RKGFX_AVS_STATUS RKGFX_AVS_wait_sync(void* gles_proceess_sync);
void* RKGFX_AVS_create_sync(void);

#ifdef __cplusplus
}
#endif
#endif  // RKGFX_GPU_AVS_H





