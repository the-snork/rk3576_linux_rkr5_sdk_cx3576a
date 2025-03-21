/*
 * Copyright 2023 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <unistd.h>
#include <malloc.h>

#include "rtsp_demo.h"
#include "sample_comm.h"
#include "font_factory.h"

#if defined(RK3576)
#define MAX_CAMERA_NUM 4
#elif defined(RK3588)
#define MAX_CAMERA_NUM 6
#else
#define MAX_CAMERA_NUM 2
#endif
// main channel(full size), sub channel(2M), third channel(D1), jpeg channel
#define VENC_CHN_MAX (MAX_CAMERA_NUM * 4)
#define CAMERA_GROUP_ID 0

#define VENC_MAIN_CHN 0
#define VENC_SUB_CHN 1
#define VENC_THIRD_CHN 2
#define VENC_JPEG_CHN 3
#define VENC_CHN_NUM_PER_CAM 4

#define TIME_OSD_CHN (MAX_CAMERA_NUM * 2)

#define FONT_LIBRARY_PATH "/usr/share/simsun_cn.ttc"
#define WEB_VIEW_RECT_W 704
#define WEB_VIEW_RECT_H 480
#define MAX_WCH_BYTE 128

#define RED_COLOR 0x0000FF
#define BLUE_COLOR 0xFF0000

#define HDMI_WIDTH 1920
#define HDMI_HEIGHT 1080
#define MIPI_WIDTH 1080
#define MIPI_HEIGHT 1920

#define TRACE_BEGIN() RK_LOGI("Enter\n")
#define TRACE_END() RK_LOGI("Exit\n")

#define UPALIGNTO(value, align) ((value + align - 1) & (~(align - 1)))
#define UPALIGNTO2(value) UPALIGNTO(value, 2)
#define UPALIGNTO4(value) UPALIGNTO(value, 4)
#define UPALIGNTO16(value) UPALIGNTO(value, 16)
#define DOWNALIGNTO16(value) (UPALIGNTO(value, 16) - 16)
#define MULTI_UPALIGNTO16(grad, value) UPALIGNTO16((int)(grad * value))

typedef struct _rkCmdArgs {
	RK_U32 u32MainWidth[MAX_CAMERA_NUM];
	RK_U32 u32MainHeight[MAX_CAMERA_NUM];
	RK_U32 u32SubWidth[MAX_CAMERA_NUM];
	RK_U32 u32SubHeight[MAX_CAMERA_NUM];
	RK_U32 u32ThirdWidth[MAX_CAMERA_NUM];
	RK_U32 u32ThirdHeight[MAX_CAMERA_NUM];
	RK_U32 u32CameraNum;
	RK_U32 u32CifBuffCnt;
	RK_U32 u32ViBuffCnt;
	RK_U32 u32VpssBuffCnt;
	RK_U32 u32Gop;
	RK_CHAR *pIqFileDir;
	RK_CHAR *pIvaModelPath;
	RK_CHAR *pInPathBmp;
	CODEC_TYPE_E enCodecType;
	VENC_RC_MODE_E enRcMode;
	RK_S32 s32LoopCnt;
	RK_S32 s32BitRate;
	RK_U32 u32Fps;
	RK_U32 u32VoDevId;
	rk_aiq_working_mode_t eHdrMode;
	RK_BOOL bEnableVo;
	RK_BOOL bEnableOsd;
	RK_BOOL bEnableCompress;
	RK_BOOL bEnableIva;
	RK_BOOL bEnableSubVenc;
	RK_BOOL bEnableThirdVenc;
	RK_BOOL bEnableJpeg;
	RK_BOOL bEnableGroup;
	RK_BOOL bEnableHwVpss;
	RK_BOOL bEnableAiisp;
	RK_BOOL bEnableNeedDumpYuv;
	RK_BOOL bEnableMemMonitor;
} RkCmdArgs;

typedef struct _rkThreadStatus {
	RK_BOOL bIfMainThreadQuit;
	RK_BOOL bIfVencThreadQuit[VENC_CHN_MAX];
	pthread_t s32TimeOsdThreadId;
	pthread_t s32VpssSendFrmThreadId;
	pthread_t s32VpssGet4MFrmThreadIds[MAX_CAMERA_NUM];
	pthread_t s32VpssGet2MFrmThreadIds[MAX_CAMERA_NUM];
	pthread_t s32VpssGetD1FrmThreadId;
	pthread_t s32MemMonitorThreadId;
} ThreadStatus;

typedef struct _rkMpiCtx {
	SAMPLE_VI_CTX_S vi[MAX_CAMERA_NUM];
	SAMPLE_VPSS_CTX_S hw_vpss[MAX_CAMERA_NUM];
	SAMPLE_VPSS_CTX_S sw_vpss[MAX_CAMERA_NUM];
	SAMPLE_VENC_CTX_S main_venc[MAX_CAMERA_NUM];
	SAMPLE_VENC_CTX_S sub_venc[MAX_CAMERA_NUM];
	SAMPLE_VENC_CTX_S third_venc[MAX_CAMERA_NUM];
	SAMPLE_VENC_CTX_S jpeg_venc[MAX_CAMERA_NUM];
	SAMPLE_IVA_CTX_S iva[3]; // 4M, 2M, D1
} SAMPLE_MPI_CTX_S;

typedef struct _rkIvaFrameInfo {
	VIDEO_FRAME_INFO_S stFrame;
	RK_S32 s32GrpId;
	RK_S32 s32ChnId;
} IvaFrameInfo;

static RkCmdArgs 			*g_cmd_args = RK_NULL;
static SAMPLE_MPI_CTX_S 	*g_mpi_ctx = RK_NULL;
static ThreadStatus 		*g_thread_status = RK_NULL;
static RK_S32 				g_exit_result = RK_FAILURE;

static pthread_mutex_t g_rtsp_mutex = {0};
static RK_BOOL g_rtsp_ifenbale = RK_FALSE;
rtsp_demo_handle g_rtsplive = RK_NULL;
static rtsp_session_handle g_rtsp_session[VENC_CHN_MAX] = {RK_NULL};

static void program_handle_error(const char *func, RK_U32 line) {
	RK_LOGE("func: <%s> line: <%d> error exit!", func, line);
	g_exit_result = RK_FAILURE;
	g_thread_status->bIfMainThreadQuit = RK_TRUE;
}

static void program_normal_exit(const char *func, RK_U32 line) {
	RK_LOGE("func: <%s> line: <%d> normal exit!", func, line);
	g_exit_result = RK_SUCCESS;
	g_thread_status->bIfMainThreadQuit = RK_TRUE;
}

static RK_CHAR optstr[] = "?::a:b:w:h:o:l:m:e:r:f:t:i:I:p:v:c:d:s:g:";
static const struct option long_options[] = {
    {"aiq", optional_argument, RK_NULL, 'a'},
    {"width", required_argument, RK_NULL, 'w'},
    {"height", required_argument, RK_NULL, 'h'},
    {"output_path", required_argument, RK_NULL, 'o'},
    {"loop_count", required_argument, RK_NULL, 'l'},
    {"encode", required_argument, RK_NULL, 'e'},
    {"fps", required_argument, RK_NULL, 'f'},
    {"bitrate", required_argument, RK_NULL, 'b'},
    {"gop", required_argument, RK_NULL, 'g'},
    {"vi_buff_cnt", required_argument, RK_NULL, 'v'},
    {"vpss_buff_cnt", required_argument, RK_NULL, 'v' + 's' + 'b'},
    {"cif_buff_cnt", required_argument, RK_NULL, 'c' + 'f' + 'b'},
    {"vo_dev_id", required_argument, RK_NULL, 'v' + 'o' + 'i' + 'd'},
    {"enable_vo", required_argument, RK_NULL, 'e' + 'v'},
    {"enable_osd", required_argument, RK_NULL, 'e' + 'o' + 's'},
    {"enable_iva", required_argument, RK_NULL, 'e' + 'i' + 'v'},
    {"enable_sub_chn", required_argument, RK_NULL, 'e' + 's' + 'c'},
    {"enable_third_chn", required_argument, RK_NULL, 'e' + 't' + 'c'},
    {"enable_jpeg", required_argument, RK_NULL, 'e' + 'j'},
    {"enable_group", required_argument, RK_NULL, 'e' + 'g'},
    {"enable_hw_vpss", required_argument, RK_NULL, 'e' + 'h' + 'v'},
    {"enable_compress", required_argument, RK_NULL, 'e' + 'c' + 'p'},
    {"enable_aiisp", required_argument, RK_NULL, 'e' + 'a' + 'p'},
    {"enable_dump_yuv", required_argument, RK_NULL, 'e' + 'd' + 'p' + 'y' + 'u'},
    {"enable_mem_monitor", required_argument, RK_NULL, 'e' + 'm' + 'o' + 'n'},
    {"camera_num", required_argument, RK_NULL, 'c' + 'n'},
    {"help", optional_argument, RK_NULL, '?'},
    {RK_NULL, 0, RK_NULL, 0},
};

/******************************************************************************
 * function : show usage
 ******************************************************************************/
static void print_usage(const RK_CHAR *name) {
	printf("usage example:\n");
	printf("\t%s -w 2560 -h 1440 -a /etc/iqfiles/ -l -1 \n", name);
	printf(
	    "\t-a | --aiq : enable aiq with dirpath provided, eg:-a /etc/iqfiles/, \n"
	    "\t		set dirpath empty to using path by default, without this option aiq \n"
	    "\t		should run in other application\n");
	printf("\t-w | --width : mainStream width, Default: 2560\n");
	printf("\t-h | --height : mainStream height, Default: 1440\n");
	printf("\t-b | --bitrate : mainStream bitrate, Default: 4096\n");
	printf("\t-g | --gop : mainStream gop, Default: 75\n");
	printf("\t-l | --loop_count : when encoder output frameCounts equal to <loop_count>, "
	       "process will exit. Default: -1\n");
	printf(
	    "\t-e | --encode : set encode type, Value: h264cbr, h264vbr, h264avbr, h265cbr, "
	    "h265vbr, h265avbr, default: h264cbr \n");
	printf("\t--cif_buff_cnt : cif buffer num, Default: 4\n");
	printf("\t--vi_buff_cnt : vi buffer num, Default: 4\n");
	printf("\t--vpss_buff_cnt : vpss buffer num, Default: 1\n");
	printf("\t--vo_dev_id : 0 HDMI, 1 MIPI, Default: 0\n");
	printf("\t--enable_vo : enable video output, Default: 1\n");
	printf("\t--enable_osd : enable all osd, Default: 1\n");
	printf("\t--enable_iva : enable iva, Default: 0\n");
	printf("\t--enable_sub_chn : enable sub venc channel(1920 * 1080), Default: 1\n");
	printf("\t--enable_third_chn : enable third venc channel(764 * 504), Default: 1\n");
	printf("\t--enable_jpeg : enable jpeg channel, Default: 1\n");
	printf("\t--enable_group : enable isp group mode, Default: 1\n");
	printf("\t--enable_hw_vpss : enable hardware vpss, Default: 1\n");
	printf("\t--enable_compress : enable FBC compress, Default: 0\n");
	printf("\t--enable_aiisp : enable AIISP, Default: 0\n");
	printf("\t--enable_dump_yuv : enable dump yuv frame to /tmp path, Default: 0\n");
	printf("\t--enable_mem_monitor : enable monitor memory stat and dump memory "
		"info to /tmp/mem_monitor.txt, Default: 0\n");
	printf("\t--camera_num : total camera numbers, Default: 4\n");
}

/******************************************************************************
 * function    : parse_cmd_args()
 * Description : Parse command line arguments.
 ******************************************************************************/
static RK_S32 parse_cmd_args(int argc, char **argv, RkCmdArgs *pArgs) {
	pArgs->u32CameraNum = MAX_CAMERA_NUM;
	pArgs->u32CifBuffCnt = 4;
	pArgs->u32ViBuffCnt = 4;
	pArgs->u32VpssBuffCnt = 1;
	pArgs->u32Gop = 60;
	pArgs->pIqFileDir = "/etc/iqfiles/";
	pArgs->pIvaModelPath = "/usr/lib/";
	pArgs->enCodecType = RK_CODEC_TYPE_H265;
	pArgs->enRcMode = VENC_RC_MODE_H265CBR;
	pArgs->s32LoopCnt = -1;
	pArgs->s32BitRate = 4 * 1024;
	pArgs->u32Fps = 25;
	pArgs->u32VoDevId = 0;
	pArgs->eHdrMode = RK_AIQ_WORKING_MODE_NORMAL;
	pArgs->bEnableVo = RK_TRUE;
	pArgs->bEnableOsd = RK_TRUE;
	pArgs->bEnableIva = RK_FALSE;
	pArgs->bEnableSubVenc = RK_TRUE;
	pArgs->bEnableThirdVenc = RK_TRUE;
	pArgs->bEnableJpeg = RK_TRUE;
	pArgs->bEnableGroup = RK_FALSE;
	pArgs->bEnableHwVpss = RK_TRUE;
	pArgs->bEnableCompress = RK_FALSE;
	pArgs->bEnableAiisp = RK_FALSE;
	pArgs->bEnableNeedDumpYuv = RK_FALSE;
	pArgs->bEnableMemMonitor = RK_FALSE;
	for (int i = 0; i != MAX_CAMERA_NUM; ++i) {
		pArgs->u32MainWidth[i] = 2560;
		pArgs->u32MainHeight[i] = 1440;
		pArgs->u32SubWidth[i] = 1920;
		pArgs->u32SubHeight[i] = 1080;
		pArgs->u32ThirdWidth[i] = 704;
		pArgs->u32ThirdHeight[i] = 576;
	}

	RK_S32 c = 0;
	while ((c = getopt_long(argc, argv, optstr, long_options, RK_NULL)) != -1) {
		const char *tmp_optarg = optarg;
		switch (c) {
		case 'a':
			if (!optarg && RK_NULL != argv[optind] && '-' != argv[optind][0]) {
				tmp_optarg = argv[optind++];
			}
			if (tmp_optarg) {
				pArgs->pIqFileDir = (char *)tmp_optarg;
			} else {
				pArgs->pIqFileDir = RK_NULL;
			}
			break;
		case 'w':
			for (int i = 0; i != MAX_CAMERA_NUM; ++i)
				pArgs->u32MainWidth[i] = atoi(optarg);
			break;
		case 'h':
			for (int i = 0; i != MAX_CAMERA_NUM; ++i)
				pArgs->u32MainHeight[i] = atoi(optarg);
			break;
		case 'b':
			pArgs->s32BitRate = atoi(optarg);
			break;
		case 'e':
			if (!strcmp(optarg, "h264cbr")) {
				pArgs->enCodecType = RK_CODEC_TYPE_H264;
				pArgs->enRcMode = VENC_RC_MODE_H264CBR;
			} else if (!strcmp(optarg, "h264vbr")) {
				pArgs->enCodecType = RK_CODEC_TYPE_H264;
				pArgs->enRcMode = VENC_RC_MODE_H264VBR;
			} else if (!strcmp(optarg, "h264avbr")) {
				pArgs->enCodecType = RK_CODEC_TYPE_H264;
				pArgs->enRcMode = VENC_RC_MODE_H264AVBR;
			} else if (!strcmp(optarg, "h265cbr")) {
				pArgs->enCodecType = RK_CODEC_TYPE_H265;
				pArgs->enRcMode = VENC_RC_MODE_H265CBR;
			} else if (!strcmp(optarg, "h265vbr")) {
				pArgs->enCodecType = RK_CODEC_TYPE_H265;
				pArgs->enRcMode = VENC_RC_MODE_H265VBR;
			} else if (!strcmp(optarg, "h265avbr")) {
				pArgs->enCodecType = RK_CODEC_TYPE_H265;
				pArgs->enRcMode = VENC_RC_MODE_H265AVBR;
			} else {
				RK_LOGE("Invalid encoder type!");
				return RK_FAILURE;
			}
			break;
		case 'l':
			pArgs->s32LoopCnt = atoi(optarg);
			break;
		case 'f':
			pArgs->u32Fps = atoi(optarg);
			break;
		case 'v':
			pArgs->u32ViBuffCnt = atoi(optarg);
			break;
		case 'v' + 's' + 'b':
			pArgs->u32VpssBuffCnt = atoi(optarg);
			break;
		case 'c' + 'f' + 'b':
			pArgs->u32CifBuffCnt = atoi(optarg);
			break;
		case 'g':
			pArgs->u32Gop = atoi(optarg);
			break;
		case 'v' + 'o' + 'i' + 'd':
			pArgs->u32VoDevId = atoi(optarg);;
			break;
		case 'c' + 'n':
			pArgs->u32CameraNum = (atoi(optarg) <= MAX_CAMERA_NUM) ? atoi(optarg) : MAX_CAMERA_NUM;
			break;
		case 'e' + 'v':
			pArgs->bEnableVo = (atoi(optarg) == 0) ? RK_FALSE : RK_TRUE;
			break;
		case 'e' + 'o' + 's':
			pArgs->bEnableOsd = (atoi(optarg) == 0) ? RK_FALSE : RK_TRUE;
			break;
		case 'e' + 'i' + 'v':
			pArgs->bEnableIva = (atoi(optarg) == 0) ? RK_FALSE : RK_TRUE;
			break;
		case 'e' + 's' + 'c':
			pArgs->bEnableSubVenc = (atoi(optarg) == 0) ? RK_FALSE : RK_TRUE;
			break;
		case 'e' + 't' + 'c':
			pArgs->bEnableThirdVenc = (atoi(optarg) == 0) ? RK_FALSE : RK_TRUE;
			break;
		case 'e' + 'j':
			pArgs->bEnableJpeg = (atoi(optarg) == 0) ? RK_FALSE : RK_TRUE;
			break;
		case 'e' + 'g':
			pArgs->bEnableGroup = (atoi(optarg) == 0) ? RK_FALSE : RK_TRUE;
			break;
		case 'e' + 'h' + 'v':
			pArgs->bEnableHwVpss = (atoi(optarg) == 0) ? RK_FALSE : RK_TRUE;
			break;
		case 'e' + 'c' + 'p':
			pArgs->bEnableCompress = (atoi(optarg) == 0) ? RK_FALSE : RK_TRUE;
			break;
		case 'e' + 'a' + 'p':
			pArgs->bEnableAiisp = (atoi(optarg) == 0) ? RK_FALSE : RK_TRUE;
			break;
		case 'e' + 'd' + 'p' + 'y' + 'u':
			pArgs->bEnableNeedDumpYuv = (atoi(optarg) == 0) ? RK_FALSE : RK_TRUE;
			break;
		case 'e' + 'm' + 'o' + 'n':
			pArgs->bEnableMemMonitor = (atoi(optarg) == 0) ? RK_FALSE : RK_TRUE;
			break;
		case '?':
		default:
			print_usage(argv[0]);
			return RK_FAILURE;
		}
	}

	return RK_SUCCESS;
}

static int fill_text(osd_data_s *data) {
	if (data->text.font_path == NULL) {
		RK_LOGE("font_path is NULL\n");
		return -1;
	}
	set_font_color(data->text.font_color);
	draw_argb8888_text(data->buffer, data->width, data->height, data->text.wch);
	return 0;
}

static int generate_date_time(wchar_t *result, const int r_size) {
	char time_str[64];
	time_t curtime;
	curtime = time(0);
	// strftime(time_str, sizeof(time_str), "%Y-%m-%d %A %H:%M:%S", localtime(&curtime));
	strftime(time_str, sizeof(time_str), "%Y-%m-%d_%H:%M:%S", localtime(&curtime));
	return swprintf(result, r_size, L"%s", time_str);
}

#define PRINT_AND_DUMP_TO_FILE(str, ...) 			\
do { 												\
	memset(buf, 0, sizeof(buf)); 					\
	snprintf(buf, sizeof(buf), str, ##__VA_ARGS__); \
	fwrite(buf, sizeof(buf), 1, f); 				\
	fflush(f); 										\
	RK_LOGI("%s", buf);								\
} while(0)

static void *memory_monitor_thread (void *pArgs) {
	RK_U32 mem_total = 0, mem_available = 0, mem_available_pre = 0;
	RK_U32 slab_used = 0, slab_used_pre = 0;
	RK_U32 rss_total = 0, rss_anon = 0, rss_file = 0, rss_shmem = 0, rss_total_pre = 0;
	FILE* f = NULL;
	FILE* proc_status = NULL;
	FILE* mem_info = NULL;
	time_t now_time;
	struct mallinfo2 mi;
	char buf[256];
	char proc_file_name[256] = {'\0'};;

	snprintf(proc_file_name, sizeof(proc_file_name), "/proc/%d/status", getpid());

	f = fopen("/tmp/mem_stat.txt", "wt+");
	if (!f) {
		RK_LOGE("open file error!");
		return NULL;
	}

	PRINT_AND_DUMP_TO_FILE("[memory monitor] ==============================================\n");
	PRINT_AND_DUMP_TO_FILE("[memory monitor] =================== dump start ===============\n");
	while (!g_thread_status->bIfMainThreadQuit) {
		time(&now_time);
		PRINT_AND_DUMP_TO_FILE("[memory monitor] ==============================================\n");
		PRINT_AND_DUMP_TO_FILE("[memory monitor] %s", ctime(&now_time));

		mem_info = fopen("/proc/meminfo", "r");
		if (!mem_info) {
			RK_LOGE("open /proc/meminfo failed, is process died?");
			program_handle_error(__func__, __LINE__);
			return RK_NULL;
		}
		memset(buf, 0, sizeof(buf));
		while (true) {
			memset(buf, 0, sizeof(buf));
			if (!fgets(buf, sizeof(buf), mem_info))
				break;
			if (strstr(buf, "MemTotal:")) {
				sscanf(buf, "MemTotal: %u", &mem_total);
				continue;
			}
			if (strstr(buf, "MemAvailable:")) {
				sscanf(buf, "MemAvailable: %u", &mem_available);
				continue;
			}
			if (strstr(buf, "Slab:")) {
				sscanf(buf, "Slab: %u", &slab_used);
				break;
			}
		}
		fclose(mem_info);

		PRINT_AND_DUMP_TO_FILE("[memory monitor] system total mem: %u kB\n", mem_total);
		PRINT_AND_DUMP_TO_FILE("[memory monitor] system available mem: %u kB\n", mem_available);
		if (mem_available_pre && mem_available < mem_available_pre)
			PRINT_AND_DUMP_TO_FILE("[memory monitor] system mem increase in last 2 min: %u kB\n", mem_available_pre - mem_available);
		PRINT_AND_DUMP_TO_FILE("[memory monitor] kernel used slab: %u kB\n", slab_used);
		if (slab_used_pre && slab_used > slab_used_pre)
			PRINT_AND_DUMP_TO_FILE("[memory monitor] slab increase in last 2 min: %u kB\n", slab_used - slab_used_pre);
		slab_used_pre = slab_used;
		mem_available_pre = mem_available;

		proc_status = fopen(proc_file_name, "r");
		if (!proc_status) {
			RK_LOGE("open %s failed, is process died?", proc_file_name);
			program_handle_error(__func__, __LINE__);
			return RK_NULL;
		}
		memset(buf, 0, sizeof(buf));
		while (true) {
			memset(buf, 0, sizeof(buf));
			if (!fgets(buf, sizeof(buf), proc_status))
				break;
			if (strstr(buf, "VmRSS")) {
				sscanf(buf, "VmRSS: %u", &rss_total);
				continue;
			}
			if (strstr(buf, "RssAnon")) {
				sscanf(buf, "RssAnon: %u", &rss_anon);
				continue;
			}
			if (strstr(buf, "RssFile")) {
				sscanf(buf, "RssFile: %u", &rss_file);
				continue;
			}
			if (strstr(buf, "RssShmem")) {
				sscanf(buf, "RssShmem: %u", &rss_shmem);
				break;
			}
		}
		fclose(proc_status);
		PRINT_AND_DUMP_TO_FILE("[memory monitor] process total rss: %u kB\n", rss_total);
		PRINT_AND_DUMP_TO_FILE("[memory monitor] process aono rss: %u kB\n", rss_anon);
		PRINT_AND_DUMP_TO_FILE("[memory monitor] process file rss: %u kB\n", rss_file);
		PRINT_AND_DUMP_TO_FILE("[memory monitor] process shmem rss: %u kB\n", rss_shmem);
		if (rss_total_pre && rss_total > rss_total_pre)
			PRINT_AND_DUMP_TO_FILE("[memory monitor] process rss increment in last 2 min: %u kB\n", rss_total - rss_total_pre);
		rss_total_pre = rss_total;

		mi = mallinfo2();
		PRINT_AND_DUMP_TO_FILE("[memory monitor] malloc mmap mem: %ld kB\n", mi.hblkhd / 1024);
		PRINT_AND_DUMP_TO_FILE("[memory monitor] malloc heap mem: %ld kB\n", mi.arena / 1024);
		PRINT_AND_DUMP_TO_FILE("[memory monitor] ==============================================\n");
		sleep(120);
	}
	PRINT_AND_DUMP_TO_FILE("[memory monitor] =================== dump stop ================\n");
	PRINT_AND_DUMP_TO_FILE("[memory monitor] ==============================================\n");

	fflush(f);
	fclose(f);
	return NULL;
}

static void *vpss_get_4M_frame(void *pArgs) {
	RK_S32 sensor_idx = (RK_S32)((unsigned long) pArgs);
	RK_S32 s32Ret = RK_FAILURE, s32Fd = 0;
	RK_U32 u32Loopcount = 0;
	long cost_time;
	long delay_time = 66; // milli seconds, 15fps
	FILE* file = NULL;
	void *pdata;
	struct timespec start_time, end_time;
	VIDEO_FRAME_INFO_S frame;
	IvaFrameInfo *ivaFrameInfo = NULL;
	RockIvaImage ivaImage;
	char file_path[128] = {'\0'};
	RK_CHAR name[256] = {0};

	sprintf(name, "vpss_%d_get_4M_frame", sensor_idx);
	prctl(PR_SET_NAME, name);

	TRACE_BEGIN();
	while (!g_thread_status->bIfMainThreadQuit) {
		memset(&frame, 0, sizeof(VIDEO_FRAME_INFO_S));
		clock_gettime(CLOCK_MONOTONIC, &start_time);
		s32Ret = RK_MPI_VPSS_GetChnFrame(sensor_idx, VENC_MAIN_CHN, &frame, 1000);
		if (s32Ret == RK_SUCCESS) {
			RK_LOGD("[grp %d, chn %d] width %u height %u seq %d pts %llu"
				, sensor_idx, VENC_MAIN_CHN
				, frame.stVFrame.u32Width, frame.stVFrame.u32Height
				, frame.stVFrame.u32TimeRef, frame.stVFrame.u64PTS);
			if (g_cmd_args->bEnableNeedDumpYuv) {
				if ((u32Loopcount % 90) == 0) {
					pdata = RK_MPI_MB_Handle2VirAddr(frame.stVFrame.pMbBlk);
					if (pdata) {
						memset(file_path, 0, sizeof(file_path));
						snprintf(file_path, sizeof(file_path), "/tmp/main_%d_%d_frm_%d.nv12"
							, frame.stVFrame.u32Width
							, frame.stVFrame.u32Height
							, u32Loopcount);
						file = fopen(file_path, "w+b");
						if (file) {
							fwrite(pdata, frame.stVFrame.u32Height * frame.stVFrame.u32Width * 3 / 2, 1, file);
							fflush(file);
							fclose(file);
						} else {
							RK_LOGE("open %s error", file_path);
						}
					}
				}
			}
			if (g_cmd_args->bEnableIva) {
				ivaFrameInfo = (IvaFrameInfo *)malloc(sizeof(IvaFrameInfo));
				if (!ivaFrameInfo) {
					RK_LOGE("-----error malloc fail for stVpssFrame");
					program_handle_error(__func__, __LINE__);
					break;
				}
				ivaFrameInfo->s32GrpId = sensor_idx;
				ivaFrameInfo->s32ChnId = VENC_MAIN_CHN;
				memcpy(&ivaFrameInfo->stFrame, &frame, sizeof(VIDEO_FRAME_INFO_S));

				s32Fd = RK_MPI_MB_Handle2Fd(frame.stVFrame.pMbBlk);
				memset(&ivaImage, 0, sizeof(RockIvaImage));
				ivaImage.info.transformMode = g_mpi_ctx->iva[0].eImageTransform;
				ivaImage.info.width = frame.stVFrame.u32Width;
				ivaImage.info.height = frame.stVFrame.u32Height;
				ivaImage.info.format = g_mpi_ctx->iva[0].eImageFormat;
				ivaImage.frameId = u32Loopcount;
				ivaImage.dataAddr = NULL;
				ivaImage.dataPhyAddr = NULL;
				ivaImage.dataFd = s32Fd;
				ivaImage.extData = ivaFrameInfo;
				s32Ret = ROCKIVA_PushFrame(g_mpi_ctx->iva[0].ivahandle, &ivaImage, NULL);
				if (s32Ret != RK_SUCCESS) {
					RK_LOGE("push frame to iva failed %#X", s32Ret);
					RK_MPI_VPSS_ReleaseChnFrame(sensor_idx, VENC_MAIN_CHN, &frame);
					free(ivaFrameInfo);
				}
			} else {
				RK_MPI_VPSS_ReleaseChnFrame(sensor_idx, VENC_MAIN_CHN, &frame);
			}
			++u32Loopcount;
		} else {
			RK_LOGE("RK_MPI_VPSS_GetChnFrame failed %#X", s32Ret);
		}
		clock_gettime(CLOCK_MONOTONIC, &end_time);
		cost_time = (end_time.tv_sec * 1000L + end_time.tv_nsec / 1000000L) -
			(start_time.tv_sec * 1000L + start_time.tv_nsec / 1000000L);
		if (delay_time > cost_time)
			usleep(1000 * (delay_time - cost_time));
	}
	TRACE_END();
	return RK_NULL;
}

static void *vpss_get_2M_frame(void *pArgs) {
	RK_S32 sensor_idx = (RK_S32)((unsigned long) pArgs);
	RK_S32 group_id = 0;
	RK_S32 s32Ret = RK_FAILURE;
	RK_S32 s32Fd = 0;
	RK_U32 u32Loopcount = 0;
	long cost_time;
	long delay_time = 33; // milli seconds, 15fps
	struct timespec start_time, end_time;
	VIDEO_FRAME_INFO_S frame;
	IvaFrameInfo *ivaFrameInfo = NULL;
	RockIvaImage ivaImage;
	RK_CHAR name[256] = {0};

	sprintf(name, "vpss_%d_get_2M_frame", sensor_idx);
	prctl(PR_SET_NAME, name);

	TRACE_BEGIN();
	while (!g_thread_status->bIfMainThreadQuit) {
		memset(&frame, 0, sizeof(VIDEO_FRAME_INFO_S));
		clock_gettime(CLOCK_MONOTONIC, &start_time);
		group_id = sensor_idx + MAX_CAMERA_NUM;
		s32Ret = RK_MPI_VPSS_GetChnFrame(group_id, 1, &frame, 1000);
		if (s32Ret == RK_SUCCESS) {
			RK_LOGD("[grp %d, chn %d] width %u height %u seq %d pts %llu"
				, group_id, 1
				, frame.stVFrame.u32Width, frame.stVFrame.u32Height
				, frame.stVFrame.u32TimeRef, frame.stVFrame.u64PTS);
			if (g_cmd_args->bEnableIva) {
				ivaFrameInfo = (IvaFrameInfo *)malloc(sizeof(IvaFrameInfo));
				if (!ivaFrameInfo) {
					RK_LOGE("-----error malloc fail for stVpssFrame");
					program_handle_error(__func__, __LINE__);
					break;
				}
				ivaFrameInfo->s32GrpId = group_id;
				ivaFrameInfo->s32ChnId = 1;
				memcpy(&ivaFrameInfo->stFrame, &frame, sizeof(VIDEO_FRAME_INFO_S));

				s32Fd = RK_MPI_MB_Handle2Fd(frame.stVFrame.pMbBlk);
				memset(&ivaImage, 0, sizeof(RockIvaImage));
				ivaImage.info.transformMode = g_mpi_ctx->iva[1].eImageTransform;
				ivaImage.info.width = frame.stVFrame.u32Width;
				ivaImage.info.height = frame.stVFrame.u32Height;
				ivaImage.info.format = g_mpi_ctx->iva[1].eImageFormat;
				ivaImage.frameId = u32Loopcount;
				ivaImage.dataAddr = NULL;
				ivaImage.dataPhyAddr = NULL;
				ivaImage.dataFd = s32Fd;
				ivaImage.extData = ivaFrameInfo;
				s32Ret = ROCKIVA_PushFrame(g_mpi_ctx->iva[1].ivahandle, &ivaImage, NULL);
				if (s32Ret != RK_SUCCESS) {
					RK_LOGE("push frame to iva failed %#X", s32Ret);
					RK_MPI_VPSS_ReleaseChnFrame(group_id, 1, &frame);
					free(ivaFrameInfo);
				}
			} else {
				RK_MPI_VPSS_ReleaseChnFrame(group_id, 1, &frame);
			}
			++u32Loopcount;
		} else {
			RK_LOGE("RK_MPI_VPSS_GetChnFrame failed %#X", s32Ret);
		}
		clock_gettime(CLOCK_MONOTONIC, &end_time);
		cost_time = (end_time.tv_sec * 1000L + end_time.tv_nsec / 1000000L) -
			(start_time.tv_sec * 1000L + start_time.tv_nsec / 1000000L);
		if (delay_time > cost_time)
			usleep(1000 * (delay_time - cost_time));
	}
	TRACE_END();
	return RK_NULL;
}

static void *vpss_get_D1_frame(void *pArgs) {
	RK_S32 sensor_idx = 0, group_id = 0;
	RK_S32 s32Ret = RK_FAILURE;
	RK_S32 s32Fd = 0;
	RK_U32 u32Loopcount = 0;
	VIDEO_FRAME_INFO_S frame;
	IvaFrameInfo *ivaFrameInfo = NULL;
	RockIvaImage ivaImage;
	prctl(PR_SET_NAME, "vpss_get_D1_frame");

	TRACE_BEGIN();
	while (!g_thread_status->bIfMainThreadQuit) {
		for (sensor_idx = 0; sensor_idx != g_cmd_args->u32CameraNum; ++sensor_idx) {
			memset(&frame, 0, sizeof(VIDEO_FRAME_INFO_S));
			group_id = sensor_idx + MAX_CAMERA_NUM;
			s32Ret = RK_MPI_VPSS_GetChnFrame(group_id, 2, &frame, 1000);
			if (s32Ret == RK_SUCCESS) {
				RK_LOGD("[grp %d, chn %d] width %u height %u seq %d pts %llu"
					, group_id, 2
					, frame.stVFrame.u32Width, frame.stVFrame.u32Height
					, frame.stVFrame.u32TimeRef, frame.stVFrame.u64PTS);
				if (g_cmd_args->bEnableIva) {
					ivaFrameInfo = (IvaFrameInfo *)malloc(sizeof(IvaFrameInfo));
					if (!ivaFrameInfo) {
						RK_LOGE("-----error malloc fail for stVpssFrame");
						program_handle_error(__func__, __LINE__);
						break;
					}
					ivaFrameInfo->s32GrpId = group_id;
					ivaFrameInfo->s32ChnId = 2;
					memcpy(&ivaFrameInfo->stFrame, &frame, sizeof(VIDEO_FRAME_INFO_S));

					s32Fd = RK_MPI_MB_Handle2Fd(frame.stVFrame.pMbBlk);
					memset(&ivaImage, 0, sizeof(RockIvaImage));
					ivaImage.info.transformMode = g_mpi_ctx->iva[2].eImageTransform;
					ivaImage.info.width = frame.stVFrame.u32Width;
					ivaImage.info.height = frame.stVFrame.u32Height;
					ivaImage.info.format = g_mpi_ctx->iva[2].eImageFormat;
					ivaImage.frameId = u32Loopcount;
					ivaImage.dataAddr = NULL;
					ivaImage.dataPhyAddr = NULL;
					ivaImage.dataFd = s32Fd;
					ivaImage.extData = ivaFrameInfo;
					s32Ret = ROCKIVA_PushFrame(g_mpi_ctx->iva[2].ivahandle, &ivaImage, NULL);
					if (s32Ret != RK_SUCCESS) {
						RK_LOGE("push frame to iva failed %#X", s32Ret);
						RK_MPI_VPSS_ReleaseChnFrame(group_id, 2, &frame);
						free(ivaFrameInfo);
					}
				} else {
					RK_MPI_VPSS_ReleaseChnFrame(group_id, 2, &frame);
				}
			} else {
				RK_LOGE("RK_MPI_VPSS_GetChnFrame failed %#X", s32Ret);
			}
		}
		++u32Loopcount;
		usleep(1000 * 200);
	}
	TRACE_END();
	return RK_NULL;
}

static void *vpss_send_frame_to_jpeg(void *pArgs) {
	RK_S32 sensor_idx = 0;
	SAMPLE_VENC_CTX_S *jpeg_ctx = NULL;
	VIDEO_FRAME_INFO_S frame;
	RK_S32 s32Ret = RK_FAILURE;
	prctl(PR_SET_NAME, "vpss_send_frame_to_jpeg");

	TRACE_BEGIN();
	while (true) {
		for (sensor_idx = 0; sensor_idx != g_cmd_args->u32CameraNum; ++sensor_idx) {
			memset(&frame, 0, sizeof(VIDEO_FRAME_INFO_S));
			jpeg_ctx = (SAMPLE_VENC_CTX_S *) &g_mpi_ctx->jpeg_venc[sensor_idx];
			if (g_thread_status->bIfVencThreadQuit[jpeg_ctx->s32ChnId]) {
				TRACE_END();
				return RK_NULL;
			}
			s32Ret = RK_MPI_VPSS_GetChnFrame(sensor_idx, VENC_JPEG_CHN, &frame, -1);
			if (s32Ret == RK_SUCCESS) {
				s32Ret = RK_MPI_VENC_SendFrame(jpeg_ctx->s32ChnId, &frame, 1000);
				if (s32Ret != RK_SUCCESS)
					RK_LOGE("RK_MPI_VENC_SendFrame failed %#X", s32Ret);
				RK_MPI_VPSS_ReleaseChnFrame(sensor_idx, VENC_JPEG_CHN, &frame);
			} else {
				RK_LOGE("RK_MPI_VPSS_GetChnFrame failed %#X", s32Ret);
			}
		}
		usleep(1000 * 1000);
	}
	TRACE_END();
	return RK_NULL;
}

static void *venc_get_jpeg(void *pArgs) {
	SAMPLE_VENC_CTX_S *ctx = (SAMPLE_VENC_CTX_S *)pArgs;
	RK_S32 s32Ret = RK_FAILURE;
	RK_S32 s32LoopCnt = 0;
	RK_VOID *pData = RK_NULL;
	RK_CHAR name[256] = {0};

	sprintf(name, "venc_%d_get_stream", ctx->s32ChnId);
	prctl(PR_SET_NAME, name);

	TRACE_BEGIN();
	while (!g_thread_status->bIfVencThreadQuit[ctx->s32ChnId]) {
		s32Ret = SAMPLE_COMM_VENC_GetStream(ctx, &pData);
		if (s32Ret == RK_SUCCESS) {
			RK_LOGD("chn:%d, frame %d, len:%u, pts:%llu, seq:%u", ctx->s32ChnId,
					s32LoopCnt, ctx->stFrame.pstPack->u32Len,
					ctx->stFrame.pstPack->u64PTS, ctx->stFrame.u32Seq);
			SAMPLE_COMM_VENC_ReleaseStream(ctx);
			s32LoopCnt++;
		}
	}
	TRACE_END();
	return RK_NULL;
}

static void *venc_get_stream(void *pArgs) {
	SAMPLE_VENC_CTX_S *ctx = (SAMPLE_VENC_CTX_S *)pArgs;
	RK_S32 s32Ret = RK_FAILURE;
	RK_S32 s32LoopCnt = 0;
	RK_VOID *pData = RK_NULL;
	RK_CHAR name[256] = {0};
	sprintf(name, "venc_%d_get_stream", ctx->s32ChnId);
	prctl(PR_SET_NAME, name);

	TRACE_BEGIN();
	while (!g_thread_status->bIfVencThreadQuit[ctx->s32ChnId]) {
		s32Ret = SAMPLE_COMM_VENC_GetStream(ctx, &pData);
		if (s32Ret == RK_SUCCESS) {

			if (ctx->s32loopCount > 0) {
				if (s32LoopCnt >= ctx->s32loopCount) {
					SAMPLE_COMM_VENC_ReleaseStream(ctx);
					program_normal_exit(__func__, __LINE__);
					break;
				}
			}

			if (g_rtsp_ifenbale) {
				pthread_mutex_lock(&g_rtsp_mutex);
				rtsp_tx_video(g_rtsp_session[ctx->s32ChnId], pData,
				              ctx->stFrame.pstPack->u32Len, ctx->stFrame.pstPack->u64PTS);
				rtsp_do_event(g_rtsplive);
				pthread_mutex_unlock(&g_rtsp_mutex);
			}

			RK_LOGV("chn:%d, frame %d, len:%u, pts:%llu, seq:%u", ctx->s32ChnId,
					s32LoopCnt, ctx->stFrame.pstPack->u32Len,
					ctx->stFrame.pstPack->u64PTS, ctx->stFrame.u32Seq);

			SAMPLE_COMM_VENC_ReleaseStream(ctx);
			s32LoopCnt++;
		}
	}

	TRACE_END();
	return RK_NULL;
}

static void *update_time_osd(void *arg) {
	printf("#Start %s thread, arg:%p\n", __func__, arg);
	prctl(PR_SET_NAME, "update_time_osd", 0, 0, 0);
	int last_time_sec, wchar_cnt;
	int ret;
	int video_width = 1920;
	int video_height = 1080;
	int normalized_screen_width = WEB_VIEW_RECT_W;
	int normalized_screen_height = WEB_VIEW_RECT_H;
	double x_rate = (double)video_width / (double)normalized_screen_width;
	double y_rate = (double)video_height / (double)normalized_screen_height;
	osd_data_s osd_data;
	time_t rawtime;
	struct tm *cur_time_info;
	BITMAP_S stBitmap;

	memset(&osd_data, 0, sizeof(osd_data));
	// init
	osd_data.enable = 1;
	osd_data.origin_x = UPALIGNTO16((int)(16 * x_rate));
	osd_data.origin_y = UPALIGNTO16((int)(16 * y_rate));
	osd_data.text.font_size = 32;
	osd_data.text.font_color = 0xfff799;
	osd_data.text.color_inverse = 1;
	osd_data.text.font_path = FONT_LIBRARY_PATH;

	// Initialize font
	ret = create_font(osd_data.text.font_path, osd_data.text.font_size);
	if (ret != 0) {
		RK_LOGE("Failed create font!\n");
		return NULL;
	}

	time(&rawtime);
	cur_time_info = localtime(&rawtime);
	last_time_sec = cur_time_info->tm_sec;
	while (!g_thread_status->bIfMainThreadQuit) {
		// 500 millis
		usleep(500000);
		time(&rawtime);
		cur_time_info = localtime(&rawtime);
		if (cur_time_info->tm_sec == last_time_sec)
			continue;
		else
			last_time_sec = cur_time_info->tm_sec;
		// generate time string.
		wchar_cnt = generate_date_time(osd_data.text.wch, MAX_WCH_BYTE);
		if (wchar_cnt <= 0) {
			RK_LOGE("generate_date_time error\n");
			continue;
		}
		// calculate really buffer size and allocate buffer for time string.
		osd_data.width =
		    UPALIGNTO16(wstr_get_actual_advance_x(osd_data.text.wch));
		osd_data.height = UPALIGNTO16(osd_data.text.font_size);
		osd_data.size = osd_data.width * osd_data.height * 4; // BGRA8888 4byte
		osd_data.buffer = malloc(osd_data.size);
		memset(osd_data.buffer, 0, osd_data.size);
		// draw font in buffer
		fill_text(&osd_data);
		// set bitmap
		stBitmap.enPixelFormat = RK_FMT_ARGB8888;
		stBitmap.u32Width = osd_data.width;
		stBitmap.u32Height = osd_data.height;
		stBitmap.pData = (RK_VOID *)osd_data.buffer;
		ret = RK_MPI_RGN_SetBitMap(TIME_OSD_CHN, &stBitmap);
		if (ret != RK_SUCCESS)
			RK_LOGE("RK_MPI_RGN_SetBitMap failed with %#x\n", ret);
		free(osd_data.buffer);
	}
	destroy_font();
	RK_LOGI("exit\n");

	return NULL;
}

static void iva_result_callback(const RockIvaBaResult *result,
                                const RockIvaExecuteStatus status, void *userData) {

	if (result->objNum == 0)
		return;
	for (int i = 0; i < result->objNum; i++) {
		RK_LOGD("topLeft:[%d,%d], bottomRight:[%d,%d],"
		        "objId is %d, frameId is %d, score is %d, type is %d\n",
		        result->triggerObjects[i].objInfo.rect.topLeft.x,
		        result->triggerObjects[i].objInfo.rect.topLeft.y,
		        result->triggerObjects[i].objInfo.rect.bottomRight.x,
		        result->triggerObjects[i].objInfo.rect.bottomRight.y,
		        result->triggerObjects[i].objInfo.objId,
		        result->triggerObjects[i].objInfo.frameId,
		        result->triggerObjects[i].objInfo.score,
		        result->triggerObjects[i].objInfo.type);
	}
}

static void iva_release_callback(const RockIvaReleaseFrames *releaseFrames,
                                       void *userdata) {
	/* when iva handle out of the video frame，this func will be called*/
	RK_S32 s32Ret = RK_SUCCESS;
	for (RK_S32 i = 0; i < releaseFrames->count; i++) {
		if (!releaseFrames->frames[i].extData) {
			RK_LOGE("---------error release frame is null");
			program_handle_error(__func__, __LINE__);
			continue;
		}
		s32Ret = RK_MPI_VPSS_ReleaseChnFrame(1, 1, releaseFrames->frames[i].extData);
		if (s32Ret != RK_SUCCESS) {
			RK_LOGE("RK_MPI_VI_ReleaseChnFrame failure:%#X", s32Ret);
			program_handle_error(__func__, __LINE__);
		}
		free(releaseFrames->frames[i].extData);
	}
}

static RK_S32 global_param_init(void) {
	TRACE_BEGIN();
	g_thread_status = (ThreadStatus *)malloc(sizeof(ThreadStatus));
	if (!g_thread_status) {
		RK_LOGI("malloc for g_thread_status failure\n");
		goto __global_init_fail;
	}
	memset(g_thread_status, 0, sizeof(ThreadStatus));

	// Allocate global ctx.
	g_mpi_ctx = (SAMPLE_MPI_CTX_S *)(malloc(sizeof(SAMPLE_MPI_CTX_S)));
	if (!g_mpi_ctx) {
		printf("ctx is null, malloc failure\n");
		goto __global_init_fail;
	}
	memset(g_mpi_ctx, 0, sizeof(SAMPLE_MPI_CTX_S));

	g_cmd_args = malloc(sizeof(RkCmdArgs));
	if (!g_cmd_args) {
		printf("g_cmd_args is null, malloc failure\n");
		goto __global_init_fail;
	}
	memset(g_cmd_args, 0, sizeof(RkCmdArgs));

	if (RK_SUCCESS != pthread_mutex_init(&g_rtsp_mutex, RK_NULL)) {
		RK_LOGE("pthread_mutex_init failure");
		goto __global_init_fail;
	}

	TRACE_END();
	return RK_SUCCESS;

__global_init_fail:
	if (g_thread_status) {
		free(g_thread_status);
		g_thread_status = RK_NULL;
	}
	if (g_mpi_ctx) {
		free(g_mpi_ctx);
		g_mpi_ctx = NULL;
	}
	if (g_cmd_args) {
		free(g_cmd_args);
		g_cmd_args = NULL;
	}
	TRACE_END();
	return RK_FAILURE;
}

static RK_S32 global_param_deinit(void) {
	TRACE_BEGIN();

	pthread_mutex_destroy(&g_rtsp_mutex);

	if (g_thread_status) {
		free(g_thread_status);
		g_thread_status = RK_NULL;
	}
	if (g_mpi_ctx) {
		free(g_mpi_ctx);
		g_mpi_ctx = NULL;
	}
	if (g_cmd_args) {
		free(g_cmd_args);
		g_cmd_args = NULL;
	}

	TRACE_END();
	return RK_SUCCESS;
}

static RK_S32 rtsp_init(CODEC_TYPE_E enCodecType) {
	RK_S32 i = 0;
	g_rtsplive = create_rtsp_demo(554);
	RK_CHAR rtspAddr[255] = {0};

	for (i = 0; i < VENC_CHN_MAX; i++) {
		sprintf(rtspAddr, "/live/%d", i);
		g_rtsp_session[i] = rtsp_new_session(g_rtsplive, rtspAddr);
		if (enCodecType == RK_CODEC_TYPE_H264) {
			rtsp_set_video(g_rtsp_session[i], RTSP_CODEC_ID_VIDEO_H264, RK_NULL, 0);
		} else if (enCodecType == RK_CODEC_TYPE_H265) {
			rtsp_set_video(g_rtsp_session[i], RTSP_CODEC_ID_VIDEO_H265, RK_NULL, 0);
		} else {
			RK_LOGE("not support other type\n");
			g_rtsp_ifenbale = RK_FALSE;
			return RK_SUCCESS;
		}
		rtsp_sync_video_ts(g_rtsp_session[i], rtsp_get_reltime(), rtsp_get_ntptime());
		RK_LOGE("rtsp <%s> init success", rtspAddr);
	}
	g_rtsp_ifenbale = RK_TRUE;
	return RK_SUCCESS;
}

static RK_S32 rtsp_deinit(void) {
	if (g_rtsplive)
		rtsp_del_demo(g_rtsplive);
	return RK_SUCCESS;
}

static RK_S32 isp_init(void) {
	int ret = RK_SUCCESS;
	int sensor_idx = 0;
	rk_aiq_camgroup_instance_cfg_t camgroup_cfg;
	TRACE_BEGIN();
	for (sensor_idx = 0; sensor_idx < g_cmd_args->u32CameraNum; ++sensor_idx)
		SAMPLE_COMM_PreInit_devBufCnt(sensor_idx, g_cmd_args->u32CifBuffCnt);
	if (g_cmd_args->bEnableGroup && g_cmd_args->u32CameraNum > 1) {
		memset(&camgroup_cfg, 0, sizeof(camgroup_cfg));
		camgroup_cfg.sns_num = g_cmd_args->u32CameraNum;
		camgroup_cfg.config_file_dir = g_cmd_args->pIqFileDir;
		ret = SAMPLE_COMM_ISP_CamGroup_Init(CAMERA_GROUP_ID, g_cmd_args->eHdrMode,
		                              RK_TRUE, 0, RK_NULL, &camgroup_cfg);
		if (ret != RK_SUCCESS) {
			RK_LOGE("SAMPLE_COMM_ISP_CamGroup_Init failure:%#X", ret);
			return ret;
		}
		ret = SAMPLE_COMM_ISP_CamGroup_SetFrameRate(CAMERA_GROUP_ID, g_cmd_args->u32Fps);
		if (ret != RK_SUCCESS) {
			RK_LOGE("SAMPLE_COMM_ISP_CamGroup_SetFrameRate failure:%#X", ret);
			return ret;
		}
		printf("#ISP cam group %d init success\n", CAMERA_GROUP_ID);
	} else {
		for (sensor_idx = 0; sensor_idx < g_cmd_args->u32CameraNum; ++sensor_idx) {
			ret = SAMPLE_COMM_ISP_Init(sensor_idx, g_cmd_args->eHdrMode,
			                              g_cmd_args->u32CameraNum > 1, g_cmd_args->pIqFileDir);
			if (ret != RK_SUCCESS) {
				printf("#ISP cam %d init failed!\n", sensor_idx);
				return ret;
			}
#if defined(RK3576)
			if (g_cmd_args->bEnableAiisp) {
				ret = SAMPLE_COMM_ISP_EnablsAiisp(sensor_idx);
				if (ret != RK_SUCCESS) {
					printf("#ISP cam %d set fps failed!\n", sensor_idx);
					return ret;
				}
			}
#endif
			ret = SAMPLE_COMM_ISP_SetFrameRate(sensor_idx, g_cmd_args->u32Fps);
			if (ret != RK_SUCCESS) {
				printf("#ISP cam %d set fps failed!\n", sensor_idx);
				return ret;
			}
			printf("#ISP cam %d init success\n", sensor_idx);
			ret = SAMPLE_COMM_ISP_Run(sensor_idx);
			if (ret != RK_SUCCESS) {
				printf("#ISP cam %d run failed!\n", sensor_idx);
				return ret;
			}
		}
	}
	TRACE_END();
	return ret;
}

static RK_S32 isp_deinit(void) {
	int sensor_idx = 0;
	if (g_cmd_args->bEnableGroup && g_cmd_args->u32CameraNum > 1) {
		SAMPLE_COMM_ISP_CamGroup_Stop(CAMERA_GROUP_ID);
		return RK_SUCCESS;
	}
	for (sensor_idx = 0; sensor_idx < g_cmd_args->u32CameraNum; ++sensor_idx) {
		SAMPLE_COMM_ISP_Stop(sensor_idx);
	}
	return RK_SUCCESS;
}

static RK_S32 vi_init(void) {
	int ret = RK_SUCCESS;
	int sensor_idx = 0;
	TRACE_BEGIN();
	for (sensor_idx = 0; sensor_idx < g_cmd_args->u32CameraNum; ++sensor_idx) {
		g_mpi_ctx->vi[sensor_idx].u32Width = g_cmd_args->u32MainWidth[sensor_idx];
		g_mpi_ctx->vi[sensor_idx].u32Height = g_cmd_args->u32MainHeight[sensor_idx];
		g_mpi_ctx->vi[sensor_idx].s32DevId = sensor_idx;
		g_mpi_ctx->vi[sensor_idx].u32PipeId = sensor_idx;
		g_mpi_ctx->vi[sensor_idx].s32ChnId = 0;
		g_mpi_ctx->vi[sensor_idx].stChnAttr.stIspOpt.stMaxSize.u32Width = g_cmd_args->u32MainWidth[sensor_idx];
		g_mpi_ctx->vi[sensor_idx].stChnAttr.stIspOpt.stMaxSize.u32Height = g_cmd_args->u32MainHeight[sensor_idx];
		g_mpi_ctx->vi[sensor_idx].stChnAttr.stIspOpt.u32BufCount = g_cmd_args->u32ViBuffCnt;
		g_mpi_ctx->vi[sensor_idx].stChnAttr.stIspOpt.enMemoryType = VI_V4L2_MEMORY_TYPE_DMABUF;
		g_mpi_ctx->vi[sensor_idx].stChnAttr.enPixelFormat = RK_FMT_YUV420SP;
		g_mpi_ctx->vi[sensor_idx].stChnAttr.enCompressMode = COMPRESS_MODE_NONE;
		g_mpi_ctx->vi[sensor_idx].stChnAttr.stFrameRate.s32SrcFrameRate = -1;
		g_mpi_ctx->vi[sensor_idx].stChnAttr.stFrameRate.s32DstFrameRate = -1;
		ret = SAMPLE_COMM_VI_CreateChn(&(g_mpi_ctx->vi[sensor_idx]));
		if (ret != RK_SUCCESS)
			RK_LOGE("SAMPLE_COMM_VI_CreateChn %d failure:%#X", sensor_idx, ret);
	}
	TRACE_END();
	return ret;
}

static RK_S32 vi_deinit(void) {
	int ret = RK_SUCCESS;
	int sensor_idx = 0;
	TRACE_BEGIN();
	for (sensor_idx = 0; sensor_idx < g_cmd_args->u32CameraNum; ++sensor_idx)
		SAMPLE_COMM_VI_DestroyChn(&(g_mpi_ctx->vi[sensor_idx]));
	TRACE_END();
	return ret;
}

static RK_S32 vo_init(void) {
	int ret = RK_SUCCESS;
	int dev_id = g_cmd_args->u32VoDevId, layer_id = 0;
	int sensor_idx = 0;
	VO_PUB_ATTR_S VoPubAttr;
	VO_VIDEO_LAYER_ATTR_S stLayerAttr;
	VO_CHN_ATTR_S stChnAttr;

	if (!g_cmd_args->bEnableVo)
		return ret;
	memset(&VoPubAttr, 0, sizeof(VO_PUB_ATTR_S));
	memset(&stLayerAttr, 0, sizeof(VO_VIDEO_LAYER_ATTR_S));
	TRACE_BEGIN();
	stLayerAttr.enPixFormat = RK_FMT_RGB888;
	stLayerAttr.enCompressMode
		= g_cmd_args->bEnableCompress ? COMPRESS_AFBC_16x16 : COMPRESS_MODE_NONE;
	stLayerAttr.stDispRect.s32X = 0;
	stLayerAttr.stDispRect.s32Y = 0;
	stLayerAttr.u32DispFrmRt = g_cmd_args->u32Fps;
	// Default, use gpu to resize output.
	if (dev_id == 0) {
		stLayerAttr.stDispRect.u32Width = HDMI_WIDTH;
		stLayerAttr.stDispRect.u32Height = HDMI_HEIGHT;
	} else {
		stLayerAttr.stDispRect.u32Width = MIPI_WIDTH;
		stLayerAttr.stDispRect.u32Height = MIPI_HEIGHT;
	}
	stLayerAttr.stImageSize.u32Width = g_cmd_args->u32ThirdWidth[0] * 2;
	stLayerAttr.stImageSize.u32Height = g_cmd_args->u32ThirdHeight[0] * 2;
	ret = RK_MPI_VO_GetPubAttr(dev_id, &VoPubAttr);
	if (ret != RK_SUCCESS) {
		RK_LOGE("RK_MPI_VO_GetPubAttr failed %#X", ret);
		return ret;
	}
	if (dev_id == 0)
		VoPubAttr.enIntfType = VO_INTF_HDMI;
	else
		VoPubAttr.enIntfType = VO_INTF_MIPI;
	VoPubAttr.enIntfSync = VO_OUTPUT_DEFAULT;
	ret = RK_MPI_VO_SetPubAttr(dev_id, &VoPubAttr);
	if (ret != RK_SUCCESS) {
		RK_LOGE("RK_MPI_VO_SetPubAttr failed %#X", ret);
		return ret;
	}
	ret = RK_MPI_VO_Enable(dev_id);
	if (ret != RK_SUCCESS) {
		RK_LOGE("RK_MPI_VO_Enable failed %#X\n", ret);
		return ret;
	}
	ret = RK_MPI_VO_BindLayer(dev_id, layer_id, VO_LAYER_MODE_GRAPHIC);
	if (ret != RK_SUCCESS) {
		RK_LOGE("RK_MPI_VO_BindLayer failed,s32Ret:%x\n", ret);
		return ret;
	}
	ret = RK_MPI_VO_SetLayerAttr(layer_id, &stLayerAttr);
	if (ret != RK_SUCCESS) {
		RK_LOGE("RK_MPI_VO_SetLayerAttr failed,s32Ret:%x\n", ret);
		return ret;
	}
	RK_MPI_VO_SetLayerPriority(layer_id, 7);
	ret = RK_MPI_VO_EnableLayer(layer_id);
	if (ret != RK_SUCCESS) {
		RK_LOGE("RK_MPI_VO_EnableLayer failed,s32Ret:%x\n", ret);
		return ret;
	}
	for (sensor_idx = 0; sensor_idx < g_cmd_args->u32CameraNum; sensor_idx++) {
		stChnAttr.stRect.u32Width = g_cmd_args->u32ThirdWidth[0];
		stChnAttr.stRect.u32Height = g_cmd_args->u32ThirdHeight[0];
		if (sensor_idx == 0) {
			stChnAttr.stRect.s32X = 0;
			stChnAttr.stRect.s32Y = 0;
		} else if (sensor_idx == 1) {
			stChnAttr.stRect.s32X = stChnAttr.stRect.u32Width;
			stChnAttr.stRect.s32Y = 0;
		} else if (sensor_idx == 2) {
			stChnAttr.stRect.s32X = 0;
			stChnAttr.stRect.s32Y = stChnAttr.stRect.u32Height;
		} else {
			stChnAttr.stRect.s32X = stChnAttr.stRect.u32Width;
			stChnAttr.stRect.s32Y = stChnAttr.stRect.u32Height;
		}
		stChnAttr.u32Priority = sensor_idx;
		stChnAttr.u32FgAlpha = 128;
		stChnAttr.u32BgAlpha = 128;
		if (dev_id == 1)
			stChnAttr.enRotation = ROTATION_90;
		ret = RK_MPI_VO_SetChnAttr(layer_id, sensor_idx, &stChnAttr);
		if (ret != RK_SUCCESS) {
			RK_LOGE("set chn Attr failed,s32Ret:%x\n", ret);
			return RK_FAILURE;
		}
		ret = RK_MPI_VO_EnableChn(layer_id, sensor_idx);
		if (ret != RK_SUCCESS) {
			RK_LOGE("enable chn failed,s32Ret:%x\n", ret);
			return RK_FAILURE;
		}
	}

	TRACE_END();
	return ret;
}

static RK_S32 vo_deinit(void) {
	int ret = RK_SUCCESS;
	int sensor_idx = 0;
	int dev_id = g_cmd_args->u32VoDevId, layer_id = 0;
	if (!g_cmd_args->bEnableVo)
		return ret;
	TRACE_BEGIN();
	for (sensor_idx = 0; sensor_idx < g_cmd_args->u32CameraNum; sensor_idx++)
		RK_MPI_VO_DisableChn(layer_id, sensor_idx);
	RK_MPI_VO_DisableLayer(layer_id);
	RK_MPI_VO_Disable(dev_id);
	RK_MPI_VO_UnBindLayer(layer_id, dev_id);
	RK_MPI_VO_CloseFd();
	TRACE_END();
	return ret;
}

static RK_S32 vpss_init(void) {
	int ret = RK_SUCCESS;
	int sensor_idx = 0;
	TRACE_BEGIN();
	for (sensor_idx = 0; sensor_idx < g_cmd_args->u32CameraNum; ++sensor_idx) {
		// Create hardware vpss node for all sensor.
		g_mpi_ctx->hw_vpss[sensor_idx].s32GrpId = sensor_idx;
		g_mpi_ctx->hw_vpss[sensor_idx].s32ChnId = 0;
		// Use hardware vpss here.
		g_mpi_ctx->hw_vpss[sensor_idx].enVProcDevType
			= g_cmd_args->bEnableHwVpss ? VIDEO_PROC_DEV_VPSS : VIDEO_PROC_DEV_GPU;
		g_mpi_ctx->hw_vpss[sensor_idx].stGrpVpssAttr.enPixelFormat = RK_FMT_YUV420SP;
		g_mpi_ctx->hw_vpss[sensor_idx].stGrpVpssAttr.enCompressMode
			= g_cmd_args->bEnableCompress ? COMPRESS_AFBC_16x16 : COMPRESS_MODE_NONE;
		g_mpi_ctx->hw_vpss[sensor_idx].s32ChnRotation[0] = ROTATION_0;

		// SET hw_vpss channel 0
		// For video encoder and AI analyzer.
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[0].enChnMode = VPSS_CHN_MODE_AUTO;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[0].enCompressMode
			= g_cmd_args->bEnableCompress ? COMPRESS_AFBC_16x16 : COMPRESS_MODE_NONE;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[0].enDynamicRange = DYNAMIC_RANGE_SDR8;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[0].enPixelFormat = RK_FMT_YUV420SP;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[0].stFrameRate.s32SrcFrameRate = -1;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[0].stFrameRate.s32DstFrameRate = -1;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[0].u32Width = g_cmd_args->u32MainWidth[sensor_idx];
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[0].u32Height = g_cmd_args->u32MainHeight[sensor_idx];
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[0].u32FrameBufCnt = g_cmd_args->u32VpssBuffCnt;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[0].u32Depth = 1;

		// SET hw_vpss channel 1
		// For 1080p yuv output to next vpss group.
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[1].enChnMode = VPSS_CHN_MODE_AUTO;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[1].enCompressMode
			= g_cmd_args->bEnableCompress ? COMPRESS_AFBC_16x16 : COMPRESS_MODE_NONE;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[1].enDynamicRange = DYNAMIC_RANGE_SDR8;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[1].enPixelFormat = RK_FMT_YUV420SP;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[1].stFrameRate.s32SrcFrameRate = -1;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[1].stFrameRate.s32DstFrameRate = -1;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[1].u32Width = g_cmd_args->u32SubWidth[sensor_idx];
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[1].u32Height = g_cmd_args->u32SubHeight[sensor_idx];
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[1].u32FrameBufCnt = g_cmd_args->u32VpssBuffCnt;

		// SET hw_vpss channel 2
		// For D1 video encoder.
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[2].enChnMode = VPSS_CHN_MODE_AUTO;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[2].enCompressMode
			= g_cmd_args->bEnableCompress ? COMPRESS_AFBC_16x16 : COMPRESS_MODE_NONE;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[2].enDynamicRange = DYNAMIC_RANGE_SDR8;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[2].enPixelFormat = RK_FMT_YUV420SP;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[2].stFrameRate.s32SrcFrameRate = -1;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[2].stFrameRate.s32DstFrameRate = -1;
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[2].u32Width = g_cmd_args->u32ThirdWidth[sensor_idx];
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[2].u32Height = g_cmd_args->u32ThirdHeight[sensor_idx];
		g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[2].u32FrameBufCnt = g_cmd_args->u32VpssBuffCnt;

		// SET hw_vpss channel 3
		// For jpeg encoder.
		if (g_cmd_args->bEnableJpeg) {
			g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[3].enChnMode = VPSS_CHN_MODE_AUTO;
			g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[3].enCompressMode
				= g_cmd_args->bEnableCompress ? COMPRESS_AFBC_16x16 : COMPRESS_MODE_NONE;
			g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[3].enDynamicRange = DYNAMIC_RANGE_SDR8;
			g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[3].enPixelFormat = RK_FMT_YUV420SP;
			g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[3].stFrameRate.s32SrcFrameRate = -1;
			g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[3].stFrameRate.s32DstFrameRate = -1;
			g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[3].u32Width = g_cmd_args->u32MainWidth[sensor_idx];
			g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[3].u32Height = g_cmd_args->u32MainHeight[sensor_idx];
			g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[3].u32FrameBufCnt = 2;
			g_mpi_ctx->hw_vpss[sensor_idx].stVpssChnAttr[3].u32Depth = 1;
		}

		ret = SAMPLE_COMM_VPSS_CreateChn(&(g_mpi_ctx->hw_vpss[sensor_idx]));
		if (ret != RK_SUCCESS)
			RK_LOGE("create hardware vpss group %d failed %#X\n", sensor_idx, ret);

		// Create software vpss node for all sensor.
		g_mpi_ctx->sw_vpss[sensor_idx].s32GrpId = sensor_idx + MAX_CAMERA_NUM;
		g_mpi_ctx->sw_vpss[sensor_idx].s32ChnId = 0;
		g_mpi_ctx->sw_vpss[sensor_idx].enVProcDevType = VIDEO_PROC_DEV_RGA;
		g_mpi_ctx->sw_vpss[sensor_idx].stGrpVpssAttr.enPixelFormat = RK_FMT_YUV420SP;
		g_mpi_ctx->sw_vpss[sensor_idx].stGrpVpssAttr.enCompressMode
			= g_cmd_args->bEnableCompress ? COMPRESS_AFBC_16x16 : COMPRESS_MODE_NONE;
		g_mpi_ctx->sw_vpss[sensor_idx].s32ChnRotation[0] = ROTATION_0;

		// SET sw_vpss channel 0
		// For 1080P video encoder.
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[0].enChnMode = VPSS_CHN_MODE_PASSTHROUGH;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[0].enCompressMode
			= g_cmd_args->bEnableCompress ? COMPRESS_AFBC_16x16 : COMPRESS_MODE_NONE;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[0].enDynamicRange = DYNAMIC_RANGE_SDR8;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[0].enPixelFormat = RK_FMT_YUV420SP;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[0].stFrameRate.s32SrcFrameRate = -1;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[0].stFrameRate.s32DstFrameRate = -1;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[0].u32Width = g_cmd_args->u32SubWidth[sensor_idx];
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[0].u32Height = g_cmd_args->u32SubHeight[sensor_idx];
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[0].u32Depth = 0;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[0].u32FrameBufCnt = g_cmd_args->u32VpssBuffCnt;

		// SET sw_vpss channel 1
		// For AI analyzer.
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[1].enChnMode = VPSS_CHN_MODE_AUTO;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[1].enCompressMode
			= g_cmd_args->bEnableCompress ? COMPRESS_AFBC_16x16 : COMPRESS_MODE_NONE;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[1].enDynamicRange = DYNAMIC_RANGE_SDR8;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[1].enPixelFormat = RK_FMT_YUV420SP;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[1].stFrameRate.s32SrcFrameRate = -1;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[1].stFrameRate.s32DstFrameRate = -1;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[1].u32Width = g_cmd_args->u32SubWidth[sensor_idx];
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[1].u32Height = g_cmd_args->u32SubHeight[sensor_idx];
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[1].u32Depth = 1;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[1].u32FrameBufCnt = g_cmd_args->u32VpssBuffCnt;

		// SET sw_vpss channel 2
		// For VO and AI analyzer.
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[2].enChnMode = VPSS_CHN_MODE_AUTO;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[2].enCompressMode
			= g_cmd_args->bEnableCompress ? COMPRESS_AFBC_16x16 : COMPRESS_MODE_NONE;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[2].enDynamicRange = DYNAMIC_RANGE_SDR8;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[2].enPixelFormat = RK_FMT_YUV420SP;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[2].stFrameRate.s32SrcFrameRate = -1;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[2].stFrameRate.s32DstFrameRate = -1;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[2].u32Width = g_cmd_args->u32ThirdWidth[sensor_idx];
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[2].u32Height = g_cmd_args->u32ThirdHeight[sensor_idx];
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[2].u32Depth = 1;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[2].u32FrameBufCnt = g_cmd_args->u32VpssBuffCnt;

		// SET sw_vpss channel 3
		// For AI analyzer.
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[3].enChnMode = VPSS_CHN_MODE_AUTO;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[3].enCompressMode
			= g_cmd_args->bEnableCompress ? COMPRESS_AFBC_16x16 : COMPRESS_MODE_NONE;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[3].enDynamicRange = DYNAMIC_RANGE_SDR8;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[3].enPixelFormat = RK_FMT_YUV420SP;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[3].stFrameRate.s32SrcFrameRate = -1;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[3].stFrameRate.s32DstFrameRate = -1;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[3].u32Width = g_cmd_args->u32ThirdWidth[sensor_idx];
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[3].u32Height = g_cmd_args->u32ThirdHeight[sensor_idx];
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[3].u32Depth = 0;
		g_mpi_ctx->sw_vpss[sensor_idx].stVpssChnAttr[3].u32FrameBufCnt = g_cmd_args->u32VpssBuffCnt;

		ret = SAMPLE_COMM_VPSS_CreateChn(&(g_mpi_ctx->sw_vpss[sensor_idx]));
		if (ret != RK_SUCCESS)
			RK_LOGE("create software vpss group %d failed %#X\n", sensor_idx, ret);
	}
	TRACE_END();
	return ret;
}

static RK_S32 vpss_deinit(void) {
	int ret = RK_SUCCESS;
	int sensor_idx = 0;
	TRACE_BEGIN();
	for (sensor_idx = 0; sensor_idx < g_cmd_args->u32CameraNum; ++sensor_idx) {
		SAMPLE_COMM_VPSS_DestroyChn(&(g_mpi_ctx->hw_vpss[sensor_idx]));
		SAMPLE_COMM_VPSS_DestroyChn(&(g_mpi_ctx->sw_vpss[sensor_idx]));
	}
	TRACE_END();
	return ret;
}

static RK_S32 venc_init(void) {
	int ret = RK_SUCCESS;
	int sensor_idx = 0;
	TRACE_BEGIN();
	for (sensor_idx = 0; sensor_idx < g_cmd_args->u32CameraNum; ++sensor_idx) {
		// Init main venc, for main video stream.
		g_mpi_ctx->main_venc[sensor_idx].s32ChnId = (sensor_idx * VENC_CHN_NUM_PER_CAM) + VENC_MAIN_CHN;
		g_mpi_ctx->main_venc[sensor_idx].u32Width = g_cmd_args->u32MainWidth[sensor_idx];
		g_mpi_ctx->main_venc[sensor_idx].u32Height = g_cmd_args->u32MainHeight[sensor_idx];
		g_mpi_ctx->main_venc[sensor_idx].u32Fps = g_cmd_args->u32Fps;
		g_mpi_ctx->main_venc[sensor_idx].u32Gop = g_cmd_args->u32Gop;
		g_mpi_ctx->main_venc[sensor_idx].u32BitRate = g_cmd_args->s32BitRate;
		g_mpi_ctx->main_venc[sensor_idx].enCodecType = g_cmd_args->enCodecType;
		g_mpi_ctx->main_venc[sensor_idx].enRcMode = g_cmd_args->enRcMode;
		g_mpi_ctx->main_venc[sensor_idx].getStreamCbFunc = venc_get_stream;
		g_mpi_ctx->main_venc[sensor_idx].s32loopCount = g_cmd_args->s32LoopCnt;
		g_mpi_ctx->main_venc[sensor_idx].dstFilePath = NULL;
		g_mpi_ctx->main_venc[sensor_idx].u32BuffSize = g_cmd_args->u32MainWidth[sensor_idx]
			* g_cmd_args->u32MainHeight[sensor_idx] * 3 / 2;
		// H264  66：Baseline  77：Main Profile 100：High Profile
		// H265  0：Main Profile  1：Main 10 Profile
		// MJPEG 0：Baseline
		g_mpi_ctx->main_venc[sensor_idx].stChnAttr.stGopAttr.enGopMode =
		    VENC_GOPMODE_NORMALP; // VENC_GOPMODE_SMARTP
		if (RK_CODEC_TYPE_H264 != g_cmd_args->enCodecType) {
			g_mpi_ctx->main_venc[sensor_idx].stChnAttr.stVencAttr.u32Profile = 0;
		} else {
			g_mpi_ctx->main_venc[sensor_idx].stChnAttr.stVencAttr.u32Profile = 100;
		}
		ret = SAMPLE_COMM_VENC_CreateChn(&g_mpi_ctx->main_venc[sensor_idx]);
		if (ret != RK_SUCCESS)
			RK_LOGE("SAMPLE_COMM_VENC_CreateChn venc chn %d failed %#X\n"
				, g_mpi_ctx->main_venc[sensor_idx].s32ChnId, ret);

		// Init sub venc, for sub video stream.
		g_mpi_ctx->sub_venc[sensor_idx].s32ChnId = (sensor_idx * VENC_CHN_NUM_PER_CAM) + VENC_SUB_CHN;
		g_mpi_ctx->sub_venc[sensor_idx].u32Width = g_cmd_args->u32SubWidth[sensor_idx];
		g_mpi_ctx->sub_venc[sensor_idx].u32Height = g_cmd_args->u32SubHeight[sensor_idx];
		g_mpi_ctx->sub_venc[sensor_idx].u32Fps = g_cmd_args->u32Fps;
		g_mpi_ctx->sub_venc[sensor_idx].u32Gop = g_cmd_args->u32Gop;
		g_mpi_ctx->sub_venc[sensor_idx].u32BitRate = 2048;
		g_mpi_ctx->sub_venc[sensor_idx].enCodecType = g_cmd_args->enCodecType;
		g_mpi_ctx->sub_venc[sensor_idx].enRcMode = g_cmd_args->enRcMode;
		g_mpi_ctx->sub_venc[sensor_idx].getStreamCbFunc = venc_get_stream;
		g_mpi_ctx->sub_venc[sensor_idx].s32loopCount = g_cmd_args->s32LoopCnt;
		g_mpi_ctx->sub_venc[sensor_idx].dstFilePath = NULL;
		g_mpi_ctx->sub_venc[sensor_idx].u32BuffSize = g_cmd_args->u32SubWidth[sensor_idx]
			* g_cmd_args->u32SubHeight[sensor_idx] * 3 / 2;
		g_mpi_ctx->sub_venc[sensor_idx].stChnAttr.stGopAttr.enGopMode =
		    VENC_GOPMODE_NORMALP; // VENC_GOPMODE_SMARTP
		if (RK_CODEC_TYPE_H264 != g_cmd_args->enCodecType) {
			g_mpi_ctx->sub_venc[sensor_idx].stChnAttr.stVencAttr.u32Profile = 0;
		} else {
			g_mpi_ctx->sub_venc[sensor_idx].stChnAttr.stVencAttr.u32Profile = 100;
		}
		if (g_cmd_args->bEnableSubVenc) {
			ret = SAMPLE_COMM_VENC_CreateChn(&g_mpi_ctx->sub_venc[sensor_idx]);
			if (ret != RK_SUCCESS)
				RK_LOGE("SAMPLE_COMM_VENC_CreateChn venc chn %d failed %#X\n"
					, g_mpi_ctx->sub_venc[sensor_idx].s32ChnId, ret);
		}

		// Init D1 venc, for D1 video stream.
		g_mpi_ctx->third_venc[sensor_idx].s32ChnId = (sensor_idx * VENC_CHN_NUM_PER_CAM) + VENC_THIRD_CHN;
		g_mpi_ctx->third_venc[sensor_idx].u32Width = g_cmd_args->u32ThirdWidth[sensor_idx];
		g_mpi_ctx->third_venc[sensor_idx].u32Height = g_cmd_args->u32ThirdHeight[sensor_idx];
		g_mpi_ctx->third_venc[sensor_idx].u32Fps = g_cmd_args->u32Fps;
		g_mpi_ctx->third_venc[sensor_idx].u32Gop = g_cmd_args->u32Gop;
		g_mpi_ctx->third_venc[sensor_idx].u32BitRate = 1024;
		g_mpi_ctx->third_venc[sensor_idx].enCodecType = g_cmd_args->enCodecType;
		g_mpi_ctx->third_venc[sensor_idx].enRcMode = g_cmd_args->enRcMode;
		g_mpi_ctx->third_venc[sensor_idx].getStreamCbFunc = venc_get_stream;
		g_mpi_ctx->third_venc[sensor_idx].s32loopCount = g_cmd_args->s32LoopCnt;
		g_mpi_ctx->third_venc[sensor_idx].dstFilePath = NULL;
		g_mpi_ctx->third_venc[sensor_idx].u32BuffSize = g_cmd_args->u32ThirdWidth[sensor_idx]
			* g_cmd_args->u32ThirdHeight[sensor_idx] * 3 / 2;
		// g_mpi_ctx->venc[sensor_idx].enable_buf_share = RK_TRUE;
		// H264  66：Baseline  77：Main Profile 100：High Profile
		// H265  0：Main Profile  1：Main 10 Profile
		// MJPEG 0：Baseline
		g_mpi_ctx->third_venc[sensor_idx].stChnAttr.stGopAttr.enGopMode =
		    VENC_GOPMODE_NORMALP; // VENC_GOPMODE_SMARTP
		if (RK_CODEC_TYPE_H264 != g_cmd_args->enCodecType) {
			g_mpi_ctx->third_venc[sensor_idx].stChnAttr.stVencAttr.u32Profile = 0;
		} else {
			g_mpi_ctx->third_venc[sensor_idx].stChnAttr.stVencAttr.u32Profile = 100;
		}
		if (g_cmd_args->bEnableThirdVenc) {
			ret = SAMPLE_COMM_VENC_CreateChn(&g_mpi_ctx->third_venc[sensor_idx]);
			if (ret != RK_SUCCESS)
				RK_LOGE("SAMPLE_COMM_VENC_CreateChn venc chn %d failed %#X\n"
					, g_mpi_ctx->third_venc[sensor_idx].s32ChnId, ret);
		}

		// Init jpeg venc.
		g_mpi_ctx->jpeg_venc[sensor_idx].s32ChnId = (sensor_idx * VENC_CHN_NUM_PER_CAM) + VENC_JPEG_CHN;
		g_mpi_ctx->jpeg_venc[sensor_idx].u32Width = g_cmd_args->u32MainWidth[sensor_idx];
		g_mpi_ctx->jpeg_venc[sensor_idx].u32Height = g_cmd_args->u32MainHeight[sensor_idx];
		g_mpi_ctx->jpeg_venc[sensor_idx].u32Fps = 1;
		g_mpi_ctx->jpeg_venc[sensor_idx].u32Gop = 50;
		g_mpi_ctx->jpeg_venc[sensor_idx].u32Qfactor = 50;
		g_mpi_ctx->jpeg_venc[sensor_idx].u32BitRate = g_cmd_args->s32BitRate;
		g_mpi_ctx->jpeg_venc[sensor_idx].enCodecType = RK_CODEC_TYPE_JPEG;
		g_mpi_ctx->jpeg_venc[sensor_idx].enRcMode = VENC_RC_MODE_MJPEGCBR;
		g_mpi_ctx->jpeg_venc[sensor_idx].getStreamCbFunc = venc_get_jpeg;
		g_mpi_ctx->jpeg_venc[sensor_idx].s32loopCount = g_cmd_args->s32LoopCnt;
		g_mpi_ctx->jpeg_venc[sensor_idx].dstFilePath = NULL;
		g_mpi_ctx->jpeg_venc[sensor_idx].u32BuffSize = g_cmd_args->u32MainWidth[sensor_idx]
			* g_cmd_args->u32MainHeight[sensor_idx] / 2;
		g_mpi_ctx->jpeg_venc[sensor_idx].bComboIfEnable = RK_TRUE;
		g_mpi_ctx->jpeg_venc[sensor_idx].u32ComboChnId = g_mpi_ctx->jpeg_venc[sensor_idx].s32ChnId;
		// g_mpi_ctx->venc[sensor_idx].enable_buf_share = RK_TRUE;
		// H264  66：Baseline  77：Main Profile 100：High Profile
		// H265  0：Main Profile  1：Main 10 Profile
		// MJPEG 0：Baseline
		g_mpi_ctx->jpeg_venc[sensor_idx].stChnAttr.stGopAttr.enGopMode =
		    VENC_GOPMODE_INIT; // VENC_GOPMODE_SMARTP
		g_mpi_ctx->jpeg_venc[sensor_idx].stChnAttr.stVencAttr.u32Profile = 0;
		if (g_cmd_args->bEnableJpeg) {
			ret = SAMPLE_COMM_VENC_CreateChn(&g_mpi_ctx->jpeg_venc[sensor_idx]);
			if (ret != RK_SUCCESS)
				RK_LOGE("SAMPLE_COMM_VENC_CreateChn venc chn %d failed %#X\n"
					, g_mpi_ctx->jpeg_venc[sensor_idx].s32ChnId, ret);
		}
	}
	TRACE_END();
	return ret;
}

static RK_S32 venc_deinit(void) {
	int ret = RK_SUCCESS;
	int sensor_idx = 0;
	TRACE_BEGIN();
	for (int i = 0; i != VENC_CHN_MAX; ++i)
		g_thread_status->bIfVencThreadQuit[i] = RK_TRUE;
	if (g_cmd_args->bEnableJpeg)
		pthread_join(g_thread_status->s32VpssSendFrmThreadId, NULL);
	for (sensor_idx = 0; sensor_idx < g_cmd_args->u32CameraNum; ++sensor_idx) {
		SAMPLE_COMM_VENC_DestroyChn(&g_mpi_ctx->main_venc[sensor_idx]);
		if (g_cmd_args->bEnableSubVenc)
			SAMPLE_COMM_VENC_DestroyChn(&g_mpi_ctx->sub_venc[sensor_idx]);
		if (g_cmd_args->bEnableThirdVenc)
			SAMPLE_COMM_VENC_DestroyChn(&g_mpi_ctx->third_venc[sensor_idx]);
		if (g_cmd_args->bEnableJpeg)
			SAMPLE_COMM_VENC_DestroyChn(&g_mpi_ctx->jpeg_venc[sensor_idx]);
	}
	TRACE_END();
	return ret;
}

static RK_S32 time_osd_init() {
	int ret = RK_SUCCESS;
	RGN_CHN_ATTR_S stRgnChnAttr;
	RGN_ATTR_S stRgnAttr;
	MPP_CHN_S stMppChn;
	TRACE_BEGIN();
	stRgnAttr.enType = OVERLAY_EX_RGN;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = RK_FMT_ARGB8888;
	stRgnAttr.unAttr.stOverlay.stSize.u32Width = 512;
	stRgnAttr.unAttr.stOverlay.stSize.u32Height = 32;
	stRgnAttr.unAttr.stOverlay.u32CanvasNum = 4;
	stRgnAttr.unAttr.stOverlay.enVProcDev = VIDEO_PROC_DEV_RGA;
	ret = RK_MPI_RGN_Create(TIME_OSD_CHN, &stRgnAttr);
	if (ret != RK_SUCCESS) {
		RK_LOGE("RK_MPI_RGN_Create failed %#x", ret);
		return ret;
	}
	// display overlay regions to venc groups
	stMppChn.enModId = RK_ID_VENC;
	stMppChn.s32DevId = 0;
	stMppChn.s32ChnId = 0;
	memset(&stRgnChnAttr, 0, sizeof(stRgnChnAttr));
	stRgnChnAttr.bShow = RK_TRUE;
	stRgnChnAttr.enType = OVERLAY_EX_RGN;
	stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = 0;
	stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = 0;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 128;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32Layer = 1;
	for (int sensor_idx = 0; sensor_idx != g_cmd_args->u32CameraNum; ++sensor_idx) {
		stMppChn.s32DevId = 0;
		stMppChn.s32ChnId = sensor_idx * VENC_CHN_NUM_PER_CAM + VENC_MAIN_CHN;
		ret = RK_MPI_RGN_AttachToChn(TIME_OSD_CHN, &stMppChn, &stRgnChnAttr);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_AttachToChn (%d) to vpss(%d,%d) failed with %#x\n"
				, TIME_OSD_CHN, stMppChn.s32DevId, stMppChn.s32ChnId, ret);
			return ret;
		}
		if (g_cmd_args->bEnableSubVenc) {
			stMppChn.s32DevId = 0;
			stMppChn.s32ChnId = sensor_idx * VENC_CHN_NUM_PER_CAM + VENC_SUB_CHN;
			ret = RK_MPI_RGN_AttachToChn(TIME_OSD_CHN, &stMppChn, &stRgnChnAttr);
			if (RK_SUCCESS != ret) {
				RK_LOGE("RK_MPI_RGN_AttachToChn (%d) to vpss(%d,%d) failed with %#x\n"
					, TIME_OSD_CHN, stMppChn.s32DevId, stMppChn.s32ChnId, ret);
				return ret;
			}
		}
		if (g_cmd_args->bEnableThirdVenc) {
			stMppChn.s32DevId = 0;
			stMppChn.s32ChnId = sensor_idx * VENC_CHN_NUM_PER_CAM + VENC_THIRD_CHN;
			ret = RK_MPI_RGN_AttachToChn(TIME_OSD_CHN, &stMppChn, &stRgnChnAttr);
			if (RK_SUCCESS != ret) {
				RK_LOGE("RK_MPI_RGN_AttachToChn (%d) to vpss(%d,%d) failed with %#x\n"
					, TIME_OSD_CHN, stMppChn.s32DevId, stMppChn.s32ChnId, ret);
				return ret;
			}
		}
		if (g_cmd_args->bEnableJpeg) {
			stMppChn.s32DevId = 0;
			stMppChn.s32ChnId = sensor_idx * VENC_CHN_NUM_PER_CAM + VENC_JPEG_CHN;
			ret = RK_MPI_RGN_AttachToChn(TIME_OSD_CHN, &stMppChn, &stRgnChnAttr);
			if (RK_SUCCESS != ret) {
				RK_LOGE("RK_MPI_RGN_AttachToChn (%d) to vpss(%d,%d) failed with %#x\n"
					, TIME_OSD_CHN, stMppChn.s32DevId, stMppChn.s32ChnId, ret);
				return ret;
			}
		}
	}
	TRACE_END();
	return ret;
}

static RK_S32 time_osd_deinit() {
	int ret = RK_SUCCESS;
	MPP_CHN_S stMppChn;
	TRACE_BEGIN();
	// display overlay regions to venc groups
	stMppChn.enModId = RK_ID_VENC;
	stMppChn.s32DevId = 0;
	for (int sensor_idx = 0; sensor_idx != g_cmd_args->u32CameraNum; ++sensor_idx) {
		stMppChn.s32DevId = 0;
		stMppChn.s32ChnId = sensor_idx * VENC_CHN_NUM_PER_CAM + VENC_MAIN_CHN;
		RK_MPI_RGN_DetachFromChn(TIME_OSD_CHN, &stMppChn);
		stMppChn.s32DevId = 0;
		stMppChn.s32ChnId = sensor_idx * VENC_CHN_NUM_PER_CAM + VENC_SUB_CHN;
		if (g_cmd_args->bEnableSubVenc)
			RK_MPI_RGN_DetachFromChn(TIME_OSD_CHN, &stMppChn);
		stMppChn.s32DevId = 0;
		stMppChn.s32ChnId = sensor_idx * VENC_CHN_NUM_PER_CAM + VENC_THIRD_CHN;
		if (g_cmd_args->bEnableThirdVenc)
			RK_MPI_RGN_DetachFromChn(TIME_OSD_CHN, &stMppChn);
		stMppChn.s32DevId = 0;
		stMppChn.s32ChnId = sensor_idx * VENC_CHN_NUM_PER_CAM + VENC_JPEG_CHN;
		if (g_cmd_args->bEnableJpeg)
			RK_MPI_RGN_DetachFromChn(TIME_OSD_CHN, &stMppChn);
	}
	RK_MPI_RGN_Destroy(TIME_OSD_CHN);
	TRACE_END();
	return ret;
}

__attribute__((unused))
static RK_S32 cover_osd_init() {
	int ret = RK_SUCCESS;
	int cover_chn;
	RGN_ATTR_S stCoverAttr;
	MPP_CHN_S stCoverChn;
	RGN_CHN_ATTR_S stCoverChnAttr;

	TRACE_BEGIN();
	memset(&stCoverAttr, 0, sizeof(stCoverAttr));
	memset(&stCoverChnAttr, 0, sizeof(stCoverChnAttr));
	// create cover regions
	stCoverAttr.enType = COVER_RGN;

	// display cover regions to venc groups
	stCoverChn.enModId = RK_ID_VPSS;
	stCoverChn.s32DevId = 0;
	memset(&stCoverChnAttr, 0, sizeof(stCoverChnAttr));
	stCoverChnAttr.bShow = RK_TRUE;
	stCoverChnAttr.enType = COVER_RGN;
	stCoverChnAttr.unChnAttr.stCoverChn.u32Color = 0xffffffff;
	for (int sensor_idx = 0; sensor_idx != g_cmd_args->u32CameraNum; ++sensor_idx) {
		cover_chn = sensor_idx;
		ret = RK_MPI_RGN_Create(cover_chn, &stCoverAttr);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_Create (%d) failed with %#x\n", cover_chn, ret);
			RK_MPI_RGN_Destroy(cover_chn);
			return RK_FAILURE;
		}
		RK_LOGI("The handle: %d, create success\n", cover_chn);
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32X = g_cmd_args->u32MainWidth[0] - 256;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32Y = g_cmd_args->u32MainHeight[0] - 256;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Width = 256;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Height = 256;
		stCoverChnAttr.unChnAttr.stCoverChn.u32Layer = 1;
		stCoverChn.s32DevId = sensor_idx;
		stCoverChn.s32ChnId = 0;
		ret = RK_MPI_RGN_AttachToChn(cover_chn, &stCoverChn, &stCoverChnAttr);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_AttachToChn (%d) to vpss(%d,%d) failed with %#x\n"
				, cover_chn, stCoverChn.s32DevId, stCoverChn.s32ChnId, ret);
			return ret;
		}

		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32X = g_cmd_args->u32SubWidth[0] - 256;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32Y = g_cmd_args->u32SubHeight[0] - 256;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Width = 256;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Height = 256;
		stCoverChnAttr.unChnAttr.stCoverChn.u32Layer = 2;
		stCoverChn.s32DevId = sensor_idx;
		stCoverChn.s32ChnId = 1;
		ret = RK_MPI_RGN_AttachToChn(cover_chn, &stCoverChn, &stCoverChnAttr);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_AttachToChn (%d) to vpss(%d,%d) failed with %#x\n"
				, cover_chn, stCoverChn.s32DevId, stCoverChn.s32ChnId, ret);
			return ret;
		}

		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32X = g_cmd_args->u32ThirdWidth[0] - 128;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32Y = g_cmd_args->u32ThirdHeight[0] - 128;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Width = 128;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Height = 128;
		stCoverChnAttr.unChnAttr.stCoverChn.u32Layer = 3;
		stCoverChn.s32DevId = sensor_idx;
		stCoverChn.s32ChnId = 2;
		ret = RK_MPI_RGN_AttachToChn(cover_chn, &stCoverChn, &stCoverChnAttr);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_AttachToChn (%d) to vpss(%d,%d) failed with %#x\n"
				, cover_chn, stCoverChn.s32DevId, stCoverChn.s32ChnId, ret);
			return ret;
		}
	}
	TRACE_END();
	return ret;
}

__attribute__((unused))
static RK_S32 cover_osd_deinit() {
	int ret = RK_SUCCESS;
	int cover_chn;
	MPP_CHN_S stCoverChn;

	TRACE_BEGIN();
	// display cover regions to venc groups
	stCoverChn.enModId = RK_ID_VPSS;
	stCoverChn.s32DevId = 0;
	for (int sensor_idx = 0; sensor_idx != g_cmd_args->u32CameraNum; ++sensor_idx) {
		cover_chn = sensor_idx;
		stCoverChn.s32DevId = sensor_idx;
		stCoverChn.s32ChnId = 0;
		ret = RK_MPI_RGN_DetachFromChn(cover_chn, &stCoverChn);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_DetachFromChn (%d) to vpss(%d,%d) failed with %#x\n"
				, cover_chn, stCoverChn.s32DevId, stCoverChn.s32ChnId, ret);
		}
		stCoverChn.s32DevId = sensor_idx;
		stCoverChn.s32ChnId = 1;
		ret = RK_MPI_RGN_DetachFromChn(cover_chn, &stCoverChn);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_DetachFromChn (%d) to vpss(%d,%d) failed with %#x\n"
				, cover_chn, stCoverChn.s32DevId, stCoverChn.s32ChnId, ret);
		}
		stCoverChn.s32DevId = sensor_idx;
		stCoverChn.s32ChnId = 2;
		ret = RK_MPI_RGN_DetachFromChn(cover_chn, &stCoverChn);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_DetachFromChn (%d) to vpss(%d,%d) failed with %#x\n"
				, cover_chn, stCoverChn.s32DevId, stCoverChn.s32ChnId, ret);
		}
		RK_MPI_RGN_Destroy(cover_chn);
	}
	TRACE_END();
	return ret;
}

__attribute__((unused))
static RK_S32 mosaic_osd_init() {
	int ret = 0;
	int mosaic_chn = 0;
	RGN_ATTR_S stMosaicAttr;
	MPP_CHN_S stMosaicChn;
	RGN_CHN_ATTR_S stMosaicChnAttr;

	TRACE_BEGIN();
	memset(&stMosaicAttr, 0, sizeof(stMosaicAttr));
	memset(&stMosaicChnAttr, 0, sizeof(stMosaicChnAttr));
	// create mosaic regions
	stMosaicAttr.enType = MOSAIC_RGN;

	// display mosaic regions to venc groups
	stMosaicChn.enModId = RK_ID_VPSS;
	stMosaicChn.s32DevId = 0;
	memset(&stMosaicChnAttr, 0, sizeof(stMosaicChnAttr));
	stMosaicChnAttr.bShow = RK_TRUE;
	stMosaicChnAttr.enType = MOSAIC_RGN;
	stMosaicChnAttr.unChnAttr.stMosaicChn.enMosaicType = AREA_RECT;
	stMosaicChnAttr.unChnAttr.stMosaicChn.enBlkSize = MOSAIC_BLK_SIZE_64;
	for (int sensor_idx = 0; sensor_idx != g_cmd_args->u32CameraNum; ++sensor_idx) {
		mosaic_chn = sensor_idx + MAX_CAMERA_NUM;
		ret = RK_MPI_RGN_Create(mosaic_chn, &stMosaicAttr);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_Create (%d) failed with %#x\n", mosaic_chn, ret);
			RK_MPI_RGN_Destroy(mosaic_chn);
			return RK_FAILURE;
		}
		RK_LOGI("The handle: %d, create success\n", mosaic_chn);
		stMosaicChnAttr.unChnAttr.stMosaicChn.stRect.s32X = g_cmd_args->u32MainWidth[0] / 2 - 128;
		stMosaicChnAttr.unChnAttr.stMosaicChn.stRect.s32Y = g_cmd_args->u32MainHeight[0] / 2 - 128;
		stMosaicChnAttr.unChnAttr.stMosaicChn.stRect.u32Width = 256;
		stMosaicChnAttr.unChnAttr.stMosaicChn.stRect.u32Height = 256;
		stMosaicChnAttr.unChnAttr.stMosaicChn.u32Layer = 4;
		stMosaicChn.s32DevId = sensor_idx;
		stMosaicChn.s32ChnId = 0;
		ret = RK_MPI_RGN_AttachToChn(mosaic_chn, &stMosaicChn, &stMosaicChnAttr);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_AttachToChn (%d) to vpss(%d,%d) failed with %#x\n"
				, mosaic_chn, stMosaicChn.s32DevId, stMosaicChn.s32ChnId, ret);
			return ret;
		}
		stMosaicChnAttr.unChnAttr.stMosaicChn.stRect.s32X = g_cmd_args->u32SubWidth[0] / 2 - 128;
		stMosaicChnAttr.unChnAttr.stMosaicChn.stRect.s32Y = g_cmd_args->u32SubHeight[0] / 2 - 128;
		stMosaicChnAttr.unChnAttr.stMosaicChn.stRect.u32Width = 256;
		stMosaicChnAttr.unChnAttr.stMosaicChn.stRect.u32Height = 256;
		stMosaicChnAttr.unChnAttr.stMosaicChn.u32Layer = 5;
		stMosaicChn.s32DevId = sensor_idx;
		stMosaicChn.s32ChnId = 1;
		ret = RK_MPI_RGN_AttachToChn(mosaic_chn, &stMosaicChn, &stMosaicChnAttr);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_AttachToChn (%d) to vpss(%d,%d) failed with %#x\n"
				, mosaic_chn, stMosaicChn.s32DevId, stMosaicChn.s32ChnId, ret);
			return ret;
		}
		stMosaicChnAttr.unChnAttr.stMosaicChn.stRect.s32X = g_cmd_args->u32ThirdWidth[0] / 2 - 64;
		stMosaicChnAttr.unChnAttr.stMosaicChn.stRect.s32Y = g_cmd_args->u32ThirdHeight[0] / 2 - 64;
		stMosaicChnAttr.unChnAttr.stMosaicChn.stRect.u32Width = 128;
		stMosaicChnAttr.unChnAttr.stMosaicChn.stRect.u32Height = 128;
		stMosaicChnAttr.unChnAttr.stMosaicChn.u32Layer = 6;
		stMosaicChn.s32DevId = sensor_idx;
		stMosaicChn.s32ChnId = 2;
		ret = RK_MPI_RGN_AttachToChn(mosaic_chn, &stMosaicChn, &stMosaicChnAttr);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_AttachToChn (%d) to vpss(%d,%d) failed with %#x\n"
				, mosaic_chn, stMosaicChn.s32DevId, stMosaicChn.s32ChnId, ret);
			return ret;
		}
	}
	return ret;
	TRACE_END();
}

__attribute__((unused))
static RK_S32 mosaic_osd_deinit() {
	int ret = 0;
	int mosaic_chn;
	MPP_CHN_S stMosaicChn;

	TRACE_BEGIN();

	// display mosaic regions to venc groups
	stMosaicChn.enModId = RK_ID_VPSS;
	stMosaicChn.s32DevId = 0;
	for (int sensor_idx = 0; sensor_idx != g_cmd_args->u32CameraNum; ++sensor_idx) {
		mosaic_chn = sensor_idx + MAX_CAMERA_NUM;
		stMosaicChn.s32DevId = sensor_idx;
		stMosaicChn.s32ChnId = 0;
		ret = RK_MPI_RGN_DetachFromChn(mosaic_chn, &stMosaicChn);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_DetachFromChn (%d) to vpss(%d,%d) failed with %#x\n"
				, mosaic_chn, stMosaicChn.s32DevId, stMosaicChn.s32ChnId, ret);
		}
		stMosaicChn.s32DevId = sensor_idx;
		stMosaicChn.s32ChnId = 1;
		ret = RK_MPI_RGN_DetachFromChn(mosaic_chn, &stMosaicChn);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_DetachFromChn (%d) to vpss(%d,%d) failed with %#x\n"
				, mosaic_chn, stMosaicChn.s32DevId, stMosaicChn.s32ChnId, ret);
		}
		stMosaicChn.s32DevId = sensor_idx;
		stMosaicChn.s32ChnId = 2;
		ret = RK_MPI_RGN_DetachFromChn(mosaic_chn, &stMosaicChn);
		if (RK_SUCCESS != ret) {
			RK_LOGE("RK_MPI_RGN_DetachFromChn (%d) to vpss(%d,%d) failed with %#x\n"
				, mosaic_chn, stMosaicChn.s32DevId, stMosaicChn.s32ChnId, ret);
		}
		RK_MPI_RGN_Destroy(mosaic_chn);
	}
	TRACE_END();
	return ret;
}

static RK_S32 osd_init() {
	RK_S32 ret = RK_SUCCESS;
	TRACE_BEGIN();
	if (g_cmd_args->bEnableOsd) {
		ret |= time_osd_init();
// INFO: RK3576 hardware vpss support mosaic and cover.
#if defined(RK3576)
		ret |= cover_osd_init();
		ret |= mosaic_osd_init();
#endif
	}
	TRACE_END();
	return ret;
}

static RK_S32 osd_deinit() {
	RK_S32 ret = RK_SUCCESS;
	TRACE_BEGIN();
	if (g_cmd_args->bEnableOsd) {
#if defined(RK3576)
		ret |= mosaic_osd_deinit();
		ret |= cover_osd_deinit();
#endif
		ret |= time_osd_deinit();
	}
	TRACE_END();
	return ret;
}

static RK_S32 iva_init() {
	RK_S32 ret = RK_SUCCESS;

	if (!g_cmd_args->bEnableIva)
		return RK_SUCCESS;
	TRACE_BEGIN();
	g_mpi_ctx->iva[0].pModelDataPath = g_cmd_args->pIvaModelPath;
	g_mpi_ctx->iva[0].u32ImageWidth = g_cmd_args->u32MainWidth[0];
	g_mpi_ctx->iva[0].u32ImageHeight = g_cmd_args->u32MainHeight[0];
	g_mpi_ctx->iva[0].u32DetectStartX = 0;
	g_mpi_ctx->iva[0].u32DetectStartY = 0;
	g_mpi_ctx->iva[0].u32DetectWidth = g_cmd_args->u32MainWidth[0];
	g_mpi_ctx->iva[0].u32DetectHight = g_cmd_args->u32MainHeight[0];
	g_mpi_ctx->iva[0].eImageTransform = ROCKIVA_IMAGE_TRANSFORM_NONE;
	g_mpi_ctx->iva[0].eImageFormat = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12;
	g_mpi_ctx->iva[0].eModeType = ROCKIVA_DET_MODEL_PFP;
	g_mpi_ctx->iva[0].resultCallback = iva_result_callback;
	g_mpi_ctx->iva[0].releaseCallback = iva_release_callback;
	ret = SAMPLE_COMM_IVA_Create(&g_mpi_ctx->iva[0]);
	if (ret != RK_SUCCESS) {
		RK_LOGE("SAMPLE_COMM_IVA_Create 0 failure:%#X", ret);
		return ret;
	}

	g_mpi_ctx->iva[1].pModelDataPath = g_cmd_args->pIvaModelPath;
	g_mpi_ctx->iva[1].u32ImageWidth = g_cmd_args->u32SubWidth[0];
	g_mpi_ctx->iva[1].u32ImageHeight = g_cmd_args->u32SubHeight[0];
	g_mpi_ctx->iva[1].u32DetectStartX = 0;
	g_mpi_ctx->iva[1].u32DetectStartY = 0;
	g_mpi_ctx->iva[1].u32DetectWidth = g_cmd_args->u32SubWidth[0];
	g_mpi_ctx->iva[1].u32DetectHight = g_cmd_args->u32SubHeight[0];
	g_mpi_ctx->iva[1].eImageTransform = ROCKIVA_IMAGE_TRANSFORM_NONE;
	g_mpi_ctx->iva[1].eImageFormat = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12;
	g_mpi_ctx->iva[1].eModeType = ROCKIVA_DET_MODEL_PFP;
	g_mpi_ctx->iva[1].resultCallback = iva_result_callback;
	g_mpi_ctx->iva[1].releaseCallback = iva_release_callback;
	ret = SAMPLE_COMM_IVA_Create(&g_mpi_ctx->iva[1]);
	if (ret != RK_SUCCESS) {
		RK_LOGE("SAMPLE_COMM_IVA_Create 1 failure:%#X", ret);
		return ret;
	}

	g_mpi_ctx->iva[2].pModelDataPath = g_cmd_args->pIvaModelPath;
	g_mpi_ctx->iva[2].u32ImageWidth = g_cmd_args->u32ThirdWidth[0];
	g_mpi_ctx->iva[2].u32ImageHeight = g_cmd_args->u32ThirdHeight[0];
	g_mpi_ctx->iva[2].u32DetectStartX = 0;
	g_mpi_ctx->iva[2].u32DetectStartY = 0;
	g_mpi_ctx->iva[2].u32DetectWidth = g_cmd_args->u32ThirdWidth[0];
	g_mpi_ctx->iva[2].u32DetectHight = g_cmd_args->u32ThirdHeight[0];
	g_mpi_ctx->iva[2].eImageTransform = ROCKIVA_IMAGE_TRANSFORM_NONE;
	g_mpi_ctx->iva[2].eImageFormat = ROCKIVA_IMAGE_FORMAT_YUV420SP_NV12;
	g_mpi_ctx->iva[2].eModeType = ROCKIVA_DET_MODEL_PFP;
	g_mpi_ctx->iva[2].resultCallback = iva_result_callback;
	g_mpi_ctx->iva[2].releaseCallback = iva_release_callback;
	ret = SAMPLE_COMM_IVA_Create(&g_mpi_ctx->iva[2]);
	if (ret != RK_SUCCESS) {
		RK_LOGE("SAMPLE_COMM_IVA_Create 2 failure:%#X", ret);
		return ret;
	}
	TRACE_END();

	return ret;
}

static RK_S32 iva_deinit() {
	if (!g_cmd_args->bEnableIva)
		return RK_SUCCESS;
	TRACE_BEGIN();
	SAMPLE_COMM_IVA_Destroy(&g_mpi_ctx->iva[0]);
	SAMPLE_COMM_IVA_Destroy(&g_mpi_ctx->iva[1]);
	SAMPLE_COMM_IVA_Destroy(&g_mpi_ctx->iva[2]);
	TRACE_END();
	return RK_SUCCESS;
}

static RK_S32 bind_pipe(void) {
	int ret = RK_SUCCESS;
	int sensor_idx = 0;
	MPP_CHN_S src_chn, dst_chn;
	TRACE_BEGIN();
	for (sensor_idx = 0; sensor_idx < g_cmd_args->u32CameraNum; ++sensor_idx) {
		src_chn.enModId = RK_ID_VI;
		src_chn.s32DevId = g_mpi_ctx->vi[sensor_idx].s32DevId;
		src_chn.s32ChnId = g_mpi_ctx->vi[sensor_idx].s32ChnId;
		dst_chn.enModId = RK_ID_VPSS;
		dst_chn.s32DevId = g_mpi_ctx->hw_vpss[sensor_idx].s32GrpId;
		dst_chn.s32ChnId = 0;
		ret = SAMPLE_COMM_Bind(&src_chn, &dst_chn);
		if (ret != RK_SUCCESS) {
			RK_LOGE("bind vi %d to hw vpss %d failed %#X", sensor_idx, sensor_idx, ret);
			break;
		}

		src_chn.enModId = RK_ID_VPSS;
		src_chn.s32DevId = g_mpi_ctx->hw_vpss[sensor_idx].s32GrpId;
		src_chn.s32ChnId = 0;
		dst_chn.enModId = RK_ID_VENC;
		dst_chn.s32DevId = 0;
		dst_chn.s32ChnId = g_mpi_ctx->main_venc[sensor_idx].s32ChnId;
		ret = SAMPLE_COMM_Bind(&src_chn, &dst_chn);
		if (ret != RK_SUCCESS) {
			RK_LOGE("bind hw vpss %d to venc %d failed %#X", sensor_idx, g_mpi_ctx->main_venc[sensor_idx].s32ChnId, ret);
			break;
		}

		src_chn.enModId = RK_ID_VPSS;
		src_chn.s32DevId = g_mpi_ctx->hw_vpss[sensor_idx].s32GrpId;
		src_chn.s32ChnId = 1;
		dst_chn.enModId = RK_ID_VPSS;
		dst_chn.s32DevId = g_mpi_ctx->sw_vpss[sensor_idx].s32GrpId;
		dst_chn.s32ChnId = 0;
		ret = SAMPLE_COMM_Bind(&src_chn, &dst_chn);
		if (ret != RK_SUCCESS) {
			RK_LOGE("bind hw vpss %d to sw vpss %d failed %#X", sensor_idx, sensor_idx, ret);
			break;
		}

		if (g_cmd_args->bEnableThirdVenc) {
			src_chn.enModId = RK_ID_VPSS;
			src_chn.s32DevId = g_mpi_ctx->hw_vpss[sensor_idx].s32GrpId;
			src_chn.s32ChnId = 2;
			dst_chn.enModId = RK_ID_VENC;
			dst_chn.s32DevId = 0;
			dst_chn.s32ChnId = g_mpi_ctx->third_venc[sensor_idx].s32ChnId;
			ret = SAMPLE_COMM_Bind(&src_chn, &dst_chn);
			if (ret != RK_SUCCESS) {
				RK_LOGE("bind hw vpss %d to venc %d failed %#X", sensor_idx, g_mpi_ctx->third_venc[sensor_idx].s32ChnId, ret);
				break;
			}
		}

		if (g_cmd_args->bEnableSubVenc) {
			src_chn.enModId = RK_ID_VPSS;
			src_chn.s32DevId = g_mpi_ctx->sw_vpss[sensor_idx].s32GrpId;
			src_chn.s32ChnId = 0;
			dst_chn.enModId = RK_ID_VENC;
			dst_chn.s32DevId = 0;
			dst_chn.s32ChnId = g_mpi_ctx->sub_venc[sensor_idx].s32ChnId;
			ret = SAMPLE_COMM_Bind(&src_chn, &dst_chn);
			if (ret != RK_SUCCESS) {
				RK_LOGE("bind sw vpss %d to venc %d failed %#X", sensor_idx, g_mpi_ctx->sub_venc[sensor_idx].s32ChnId, ret);
				break;
			}
		}

		if (g_cmd_args->bEnableVo) {
			src_chn.enModId = RK_ID_VPSS;
			src_chn.s32DevId = g_mpi_ctx->sw_vpss[sensor_idx].s32GrpId;
			src_chn.s32ChnId = 2;
			dst_chn.enModId = RK_ID_VO;
			dst_chn.s32DevId = 0;
			dst_chn.s32ChnId = sensor_idx;
			ret = SAMPLE_COMM_Bind(&src_chn, &dst_chn);
			if (ret != RK_SUCCESS) {
				RK_LOGE("bind sw vpss %d to vo %d failed %#X", sensor_idx, g_mpi_ctx->sub_venc[sensor_idx].s32ChnId, ret);
				break;
			}
		}
	}
	TRACE_END();
	return ret;
}

static RK_S32 unbind_pipe(void) {
	int ret = RK_SUCCESS;
	int sensor_idx = 0;
	MPP_CHN_S src_chn, dst_chn;
	TRACE_BEGIN();
	for (sensor_idx = 0; sensor_idx < g_cmd_args->u32CameraNum; ++sensor_idx) {
		src_chn.enModId = RK_ID_VI;
		src_chn.s32DevId = g_mpi_ctx->vi[sensor_idx].s32DevId;
		src_chn.s32ChnId = g_mpi_ctx->vi[sensor_idx].s32ChnId;
		dst_chn.enModId = RK_ID_VPSS;
		dst_chn.s32DevId = g_mpi_ctx->hw_vpss[sensor_idx].s32GrpId;
		dst_chn.s32ChnId = 0;
		SAMPLE_COMM_UnBind(&src_chn, &dst_chn);

		src_chn.enModId = RK_ID_VPSS;
		src_chn.s32DevId = g_mpi_ctx->hw_vpss[sensor_idx].s32GrpId;
		src_chn.s32ChnId = 0;
		dst_chn.enModId = RK_ID_VENC;
		dst_chn.s32DevId = 0;
		dst_chn.s32ChnId = g_mpi_ctx->main_venc[sensor_idx].s32ChnId;
		SAMPLE_COMM_UnBind(&src_chn, &dst_chn);

		src_chn.enModId = RK_ID_VPSS;
		src_chn.s32DevId = g_mpi_ctx->hw_vpss[sensor_idx].s32GrpId;
		src_chn.s32ChnId = 1;
		dst_chn.enModId = RK_ID_VPSS;
		dst_chn.s32DevId = g_mpi_ctx->sw_vpss[sensor_idx].s32GrpId;
		dst_chn.s32ChnId = 0;
		SAMPLE_COMM_UnBind(&src_chn, &dst_chn);

		if (g_cmd_args->bEnableThirdVenc) {
			src_chn.enModId = RK_ID_VPSS;
			src_chn.s32DevId = g_mpi_ctx->hw_vpss[sensor_idx].s32GrpId;
			src_chn.s32ChnId = 2;
			dst_chn.enModId = RK_ID_VENC;
			dst_chn.s32DevId = 0;
			dst_chn.s32ChnId = g_mpi_ctx->third_venc[sensor_idx].s32ChnId;
			SAMPLE_COMM_UnBind(&src_chn, &dst_chn);
		}

		if (g_cmd_args->bEnableSubVenc) {
			src_chn.enModId = RK_ID_VPSS;
			src_chn.s32DevId = g_mpi_ctx->sw_vpss[sensor_idx].s32GrpId;
			src_chn.s32ChnId = 0;
			dst_chn.enModId = RK_ID_VENC;
			dst_chn.s32DevId = 0;
			dst_chn.s32ChnId = g_mpi_ctx->sub_venc[sensor_idx].s32ChnId;
			SAMPLE_COMM_UnBind(&src_chn, &dst_chn);
		}

		if (g_cmd_args->bEnableVo) {
			src_chn.enModId = RK_ID_VPSS;
			src_chn.s32DevId = g_mpi_ctx->sw_vpss[sensor_idx].s32GrpId;
			src_chn.s32ChnId = 2;
			dst_chn.enModId = RK_ID_VO;
			dst_chn.s32DevId = 0;
			dst_chn.s32ChnId = sensor_idx;
			SAMPLE_COMM_UnBind(&src_chn, &dst_chn);
		}
	}
	TRACE_END();
	return ret;
}

static RK_S32 start_pipe(void) {
	RK_S32 ret = RK_SUCCESS;
	TRACE_BEGIN();
	for (int sensor_idx = 0; sensor_idx != g_cmd_args->u32CameraNum; ++sensor_idx) {
		pthread_create(&g_thread_status->s32VpssGet4MFrmThreadIds[sensor_idx], NULL
			, vpss_get_4M_frame, (void *)((unsigned long) sensor_idx));
		pthread_create(&g_thread_status->s32VpssGet2MFrmThreadIds[sensor_idx], NULL
			, vpss_get_2M_frame, (void *)((unsigned long) sensor_idx));
	}
	pthread_create(&g_thread_status->s32VpssGetD1FrmThreadId, NULL, vpss_get_D1_frame, NULL);
	if (g_cmd_args->bEnableJpeg)
		pthread_create(&g_thread_status->s32VpssSendFrmThreadId, NULL, vpss_send_frame_to_jpeg, NULL);
	if (g_cmd_args->bEnableOsd)
		pthread_create(&g_thread_status->s32TimeOsdThreadId, NULL, update_time_osd, NULL);
	TRACE_END();
	return ret;
}

static RK_S32 stop_pipe(void) {
	RK_S32 ret = RK_SUCCESS, i = 0;
	TRACE_BEGIN();
	for (i = 0; i != VENC_CHN_MAX; ++i)
		g_thread_status->bIfVencThreadQuit[i] = RK_TRUE;
	if (g_cmd_args->bEnableJpeg)
		pthread_join(g_thread_status->s32VpssSendFrmThreadId, NULL);
	if (g_cmd_args->bEnableOsd)
		pthread_join(g_thread_status->s32TimeOsdThreadId, NULL);
	pthread_join(g_thread_status->s32VpssGetD1FrmThreadId, NULL);
	for (i = 0; i != g_cmd_args->u32CameraNum; ++i) {
		pthread_join(g_mpi_ctx->main_venc[i].getStreamThread, NULL);
		if (g_cmd_args->bEnableSubVenc)
			pthread_join(g_mpi_ctx->sub_venc[i].getStreamThread, NULL);
		if (g_cmd_args->bEnableThirdVenc)
			pthread_join(g_mpi_ctx->third_venc[i].getStreamThread, NULL);
		if (g_cmd_args->bEnableJpeg)
			pthread_join(g_mpi_ctx->jpeg_venc[i].getStreamThread, NULL);
		pthread_join(g_thread_status->s32VpssGet4MFrmThreadIds[i], NULL);
		pthread_join(g_thread_status->s32VpssGet2MFrmThreadIds[i], NULL);
	}
	TRACE_END();
	return ret;

}

void start_mem_monitor() {
	if (g_cmd_args->bEnableMemMonitor)
		pthread_create(&g_thread_status->s32MemMonitorThreadId, NULL, memory_monitor_thread, NULL);
}

void stop_mem_monitor() {
	if (g_cmd_args->bEnableMemMonitor)
		pthread_join(g_thread_status->s32MemMonitorThreadId, NULL);
}

int main(int argc, char *argv[]) {
	int ret = RK_SUCCESS;

	RK_MPI_SYS_Init();
	ret = global_param_init();
	if (ret != RK_SUCCESS) {
		RK_LOGE("global_param_init failed! exit program!");
		goto __global_param_init_failed;
	}
	if (parse_cmd_args(argc, argv, g_cmd_args) != RK_SUCCESS) {
		global_param_deinit();
		return RK_FAILURE;
	}
	start_mem_monitor();
	ret = isp_init();
	if (ret != RK_SUCCESS) {
		RK_LOGE("isp_init failed! exit program!");
		goto __isp_init_failed;
	}
	ret = vi_init();
	if (ret != RK_SUCCESS) {
		RK_LOGE("vi_init failed! exit program!");
		goto __vi_init_failed;
	}
	ret = vo_init();
	if (ret != RK_SUCCESS) {
		RK_LOGE("vo_init failed! exit program!");
		goto __vo_init_failed;
	}
	ret = vpss_init();
	if (ret != RK_SUCCESS) {
		RK_LOGE("vpss_init failed! exit program!");
		goto __vpss_init_failed;
	}
	ret = venc_init();
	if (ret != RK_SUCCESS) {
		RK_LOGE("venc_init failed! exit program!");
		goto __venc_init_failed;
	}
	ret = osd_init();
	if (ret != RK_SUCCESS) {
		RK_LOGE("time_osd_init failed! exit program!");
		goto __osd_init_failed;
	}
	ret = rtsp_init(g_cmd_args->enCodecType);
	if (ret != RK_SUCCESS) {
		RK_LOGE("rtsp_init failed! exit program!");
		goto __rtsp_init_failed;
	}
	ret = iva_init();
	if (ret != RK_SUCCESS) {
		RK_LOGE("iva_init failed! exit program!");
		goto __iva_init_failed;
	}
	ret = start_pipe();
	if (ret != RK_SUCCESS) {
		RK_LOGE("bind_pipe failed! exit program!");
		goto __start_pipe_failed;
	}
	ret = bind_pipe();
	if (ret != RK_SUCCESS) {
		RK_LOGE("bind_pipe failed! exit program!");
		goto __bind_pipe_failed;
	}

	RK_LOGE("-------------------- program loop enter ------------------");
	while (!g_thread_status->bIfMainThreadQuit)
		usleep(1000);
	RK_LOGE("-------------------- program loop exit -------------------");

	unbind_pipe();
__bind_pipe_failed:
	stop_pipe();
__start_pipe_failed:
	iva_deinit();
__iva_init_failed:
	rtsp_deinit();
__rtsp_init_failed:
	osd_deinit();
__osd_init_failed:
	venc_deinit();
__venc_init_failed:
	vpss_deinit();
__vpss_init_failed:
	vo_deinit();
__vo_init_failed:
	vi_deinit();
__vi_init_failed:
	isp_deinit();
__isp_init_failed:
	stop_mem_monitor();
	global_param_deinit();
__global_param_init_failed:
	RK_MPI_SYS_Exit();
	return g_exit_result;
}
