/* 
 *
 * 
 * VideoEye
 *
 * Author: Lei Xiaohua
 * leixiaohua1020@126.com
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 */


// stdafx.h : Standard system include file.
// Used frequently but rarely changed.
// Include for all project files.

#pragma once

// Preprocessor definitions to avoid conflicts
#define _CRT_SECURE_NO_WARNINGS
#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS
// Enable SDL2 compatibility mode for SDL3
#define SDL_ENABLE_OLD_NAMES

// Define INTMAX_MAX before any system headers to avoid conflicts with MSVC's <ratio>
#ifndef INTMAX_MAX
#define INTMAX_MAX 9223372036854775807LL
#endif

// Include standard headers
#include <cstdint>

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 锟斤拷 Windows 头锟斤拷锟脚筹拷锟斤拷锟斤拷使锟矫碉拷锟斤拷锟斤拷
#endif


#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 锟斤拷锟届函锟斤拷锟斤拷锟斤拷锟斤拷式锟斤拷

// 锟截憋拷 MFC 锟斤拷某些锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟缴凤拷锟侥猴拷锟皆的撅拷锟斤拷锟斤拷息锟斤拷锟斤拷锟斤拷
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC 锟斤拷锟斤拷锟斤拷锟斤拷捅锟阶硷拷锟斤拷
#include <afxext.h>         // MFC 锟斤拷展
#include <afxdisp.h>        // MFC 锟皆讹拷锟斤拷锟斤拷

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC 锟斤拷 Internet Explorer 4 锟斤拷锟斤拷锟截硷拷锟斤拷支锟斤拷
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC 锟斤拷 Windows 锟斤拷锟斤拷锟截硷拷锟斤拷支锟斤拷
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // 锟斤拷锟斤拷锟斤拷锟酵控硷拷锟斤拷锟斤拷 MFC 支锟斤拷


#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

// Standard C++ headers - included before FFmpeg to avoid type conflicts
#include <vector>
#include <string>
#include <memory>
#include <iterator>
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <locale>
#include <type_traits>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cinttypes>
#include <chrono>

// Windows headers
#include <direct.h>
#include <io.h>

//FFmpeg - wrapped in extern "C" to avoid name mangling
extern "C"
{
#include "libavutil/avstring.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"

// Define RDFT types and functions for compatibility with newer FFmpeg
#ifndef RDFTContext
typedef void RDFTContext;
#endif
#ifndef FFTSample
typedef float FFTSample;
#endif
#ifndef av_rdft_init
#define av_rdft_init(bits, dir) NULL
#endif
#ifndef av_rdft_end
#define av_rdft_end(ctx)
#endif
#ifndef av_rdft_calc
#define av_rdft_calc(ctx, data)
#endif
#ifndef DFT_R2C
#define DFT_R2C 0
#endif

// Define QSCALE types for compatibility with newer FFmpeg
#ifndef FF_QSCALE_TYPE_MPEG1
#define FF_QSCALE_TYPE_MPEG1 0
#endif
#ifndef FF_QSCALE_TYPE_MPEG2
#define FF_QSCALE_TYPE_MPEG2 1
#endif
#ifndef FF_QSCALE_TYPE_H264
#define FF_QSCALE_TYPE_H264 2
#endif
#ifndef FF_QSCALE_TYPE_VP56
#define FF_QSCALE_TYPE_VP56 3
#endif

// Define MB_TYPE constants for compatibility with newer FFmpeg
#ifndef MB_TYPE_INTRA4x4
#define MB_TYPE_INTRA4x4 (1 << 0)
#endif
#ifndef MB_TYPE_INTRA16x16
#define MB_TYPE_INTRA16x16 (1 << 1)
#endif
#ifndef MB_TYPE_INTRA_PCM
#define MB_TYPE_INTRA_PCM (1 << 2)
#endif
#ifndef MB_TYPE_16x16
#define MB_TYPE_16x16 (1 << 3)
#endif
#ifndef MB_TYPE_16x8
#define MB_TYPE_16x8 (1 << 4)
#endif
#ifndef MB_TYPE_8x16
#define MB_TYPE_8x16 (1 << 5)
#endif
#ifndef MB_TYPE_8x8
#define MB_TYPE_8x8 (1 << 6)
#endif
#ifndef MB_TYPE_SKIP
#define MB_TYPE_SKIP (1 << 7)
#endif
#ifndef MB_TYPE_L0
#define MB_TYPE_L0 (1 << 8)
#endif
#ifndef MB_TYPE_L1
#define MB_TYPE_L1 (1 << 9)
#endif

// Define PIX_FMT_BGR24 for compatibility
#ifndef PIX_FMT_BGR24
#define PIX_FMT_BGR24 AV_PIX_FMT_BGR24
#endif

// Define av_register_all for compatibility (deprecated in newer FFmpeg)
#ifndef av_register_all
#define av_register_all()
#endif

// Define av_lockmgr_register for compatibility (deprecated in newer FFmpeg)
#ifndef av_lockmgr_register
#define av_lockmgr_register(cb) (0)
#endif

// Define avpicture_get_size for compatibility
#ifndef avpicture_get_size
#define avpicture_get_size(pix_fmt, width, height) av_image_get_buffer_size((pix_fmt), (width), (height), 1)
#endif

// Define avpicture_fill for compatibility
#ifndef avpicture_fill
#define avpicture_fill(picture, ptr, pix_fmt, width, height) av_image_fill_arrays((picture)->data, (picture)->linesize, (ptr), (pix_fmt), (width), (height), 1)
#endif

// Define AVPicture as alias for AVFrame
typedef AVFrame AVPicture;

// Compatibility for avcodec_decode_video2 (deprecated in newer FFmpeg)
#ifndef avcodec_decode_video2
static inline int avcodec_decode_video2(AVCodecContext* ctx, AVFrame* frame, int* got_picture, AVPacket* pkt) {
    int ret = avcodec_send_packet(ctx, pkt);
    if (ret >= 0) {
        ret = avcodec_receive_frame(ctx, frame);
    }
    *got_picture = (ret >= 0) ? 1 : 0;
    return (ret >= 0) ? 0 : ((ret == AVERROR(EAGAIN)) ? ret : 0);
}
#endif

// Compatibility for avcodec_alloc_frame
#ifndef avcodec_alloc_frame
#define avcodec_alloc_frame() av_frame_alloc()
#endif

// Compatibility for avcodec_free_frame
#ifndef avcodec_free_frame
#define avcodec_free_frame(frame) av_frame_free(frame)
#endif

// Compatibility for av_free_packet
#ifndef av_free_packet
#define av_free_packet(pkt) av_packet_unref(pkt)
#endif

// Compatibility for CODEC_CAP_* constants
#ifndef CODEC_CAP_DR1
#define CODEC_CAP_DR1 AV_CODEC_CAP_DR1
#endif
#ifndef CODEC_CAP_DELAY
#define CODEC_CAP_DELAY AV_CODEC_CAP_DELAY
#endif
#ifndef CODEC_CAP_FRAME_THREADS
#define CODEC_CAP_FRAME_THREADS AV_CODEC_CAP_FRAME_THREADS
#endif
#ifndef CODEC_CAP_SLICE_THREADS
#define CODEC_CAP_SLICE_THREADS AV_CODEC_CAP_SLICE_THREADS
#endif
#ifndef CODEC_CAP_EXPERIMENTAL
#define CODEC_CAP_EXPERIMENTAL AV_CODEC_CAP_EXPERIMENTAL
#endif
#ifndef CODEC_CAP_DRAW_HORIZ_BAND
#define CODEC_CAP_DRAW_HORIZ_BAND AV_CODEC_CAP_DRAW_HORIZ_BAND
#endif

// Compatibility for AV_CODEC_FLAG_* constants
#ifndef AV_CODEC_FLAG_EMU_EDGE
#define AV_CODEC_FLAG_EMU_EDGE (1 << 2)
#endif

// Compatibility for PIX_FMT_NB
#ifndef PIX_FMT_NB
#define PIX_FMT_NB AV_PIX_FMT_NB
#endif

// Compatibility for CPU flags functions
#ifndef av_get_cpu_flags
#define av_get_cpu_flags() 0
#endif
#ifndef av_parse_cpu_caps
#define av_parse_cpu_caps(flags, arg) 0
#endif
#ifndef av_force_cpu_flags
#define av_force_cpu_flags(flags)
#endif

// Compatibility for format iteration functions
#ifndef av_oformat_next
#define av_oformat_next(fmt) NULL
#endif
#ifndef av_iformat_next
#define av_iformat_next(fmt) NULL
#endif

// Compatibility for codec iteration functions
#ifndef av_codec_next
#define av_codec_next(codec) NULL
#endif
#ifndef av_codec_iterate
#define av_codec_iterate(opaque) NULL
#endif

// Compatibility for channel layout string
#ifndef av_get_channel_layout_string
#define av_get_channel_layout_string(buf, buf_size, nb_channels, channel_layout)
#endif

// Compatibility for bitstream filters
#ifndef AVBitStreamFilter
typedef struct AVBitStreamFilter AVBitStreamFilter;
#endif
#ifndef av_bitstream_filter_next
#define av_bitstream_filter_next(bsf) NULL
#endif
#ifndef av_bitstream_filter_iterate
#define av_bitstream_filter_iterate(opaque) NULL
#endif

// Compatibility for AVInputFormat members (deprecated in newer FFmpeg)
#ifndef AVInputFormat_priv_data_size
#define AVInputFormat_priv_data_size(fmt) 0
#endif
#ifndef AVInputFormat_next
#define AVInputFormat_next(fmt) NULL
#endif

// Compatibility for AVCodec members (deprecated in newer FFmpeg)
#ifndef AVCodec_priv_data_size
#define AVCodec_priv_data_size(codec) 0
#endif
#ifndef AVCodec_next
#define AVCodec_next(codec) NULL
#endif
}

// Compatibility function to get AVCodecContext from AVStream
static inline AVCodecContext* get_stream_codec_context(AVFormatContext *fmt_ctx, int stream_index) {
    if (!fmt_ctx || stream_index < 0 || stream_index >= (int)fmt_ctx->nb_streams) {
        return NULL;
    }
    AVStream *st = fmt_ctx->streams[stream_index];
    AVCodecContext *ctx = avcodec_alloc_context3(NULL);
    if (ctx) {
        avcodec_parameters_to_context(ctx, st->codecpar);
    }
    return ctx;
}


//INT64 definitions - protected to avoid redefinition
#ifndef INT64_MIN
#define INT64_MIN       (-9223372036854775807i64 - 1)
#endif
#ifndef INT64_MAX
#define INT64_MAX       9223372036854775807i64
#endif

//Maximum frame number
#define MAX_FRAME_NUM 10000
//Maximum packet number - one reference frame
#define MAX_PACKET_NUM 10000
//Maximum I/O status check information
#define MAX_IOCHECK_NUM 10000
//URL length
#define MAX_URL_LENGTH 500


// OpenCV headers
#include "opencv2/opencv.hpp"

// Project headers
#include "ve_play.h"

// TinyXML headers
#include "tinyxml/tinyxml.h"
#include "tinyxml/tinystr.h"
