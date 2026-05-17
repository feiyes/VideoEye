/* 
 *
 * 
 * VideoEye
 *
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 */

#include "stdafx.h"

#include "ve_play.h"
const char program_name[] = "VE_Player";
const int program_birth_year = 2013;


#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define MIN_FRAMES 5

/* SDL audio buffer size, in samples. Should be small to have precise
A/V sync as SDL does not have hardware buffer fullness info. */
#define SDL_AUDIO_BUFFER_SIZE 1024

/* no AV sync correction is done if below the AV sync threshold */
#define AV_SYNC_THRESHOLD 0.01
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)


static int sws_flags = SWS_BICUBIC;

//获取网络文件协议时使用，来自ffmpeg源码
typedef struct URLContext {
	const AVClass *av_class; ///< information for av_log(). Set by url_open().
	struct URLProtocol *prot;
	int flags;
	int is_streamed;  /**< true if streamed (no seek possible), default = false */
	int max_packet_size;  /**< if non zero, the stream is packetized with this max packet size */
	void *priv_data;
	char *filename; /**< specified URL */
	int is_connected;
	AVIOInterruptCB interrupt_callback;
} URLContext;
typedef struct URLProtocol {
	const char *name;
	int (*url_open)(URLContext *h, const char *url, int flags);
	int (*url_read)(URLContext *h, unsigned char *buf, int size);
	int (*url_write)(URLContext *h, const unsigned char *buf, int size);
	int64_t (*url_seek)(URLContext *h, int64_t pos, int whence);
	int (*url_close)(URLContext *h);
	struct URLProtocol *next;
	int (*url_read_pause)(URLContext *h, int pause);
	int64_t (*url_read_seek)(URLContext *h, int stream_index,
		int64_t timestamp, int flags);
	int (*url_get_file_handle)(URLContext *h);
	int priv_data_size;
	const AVClass *priv_data_class;
	int flags;
	int (*url_check)(URLContext *h, int mask);
} URLProtocol;
//---------------------


typedef struct PacketQueue {
	AVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	int abort_request;
	SDL_mutex *mutex;
	SDL_Condition *cond;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE 4
#define SUBPICTURE_QUEUE_SIZE 4


long  my_rint(double x)
{
	if(x >= 0.)
		return (long)(x + 0.5);
	else
		return (long)(x - 0.5);
}

typedef struct VideoPicture {
	double pts;                                  ///< presentation time stamp for this picture
	int64_t pos;                                 ///< byte position in file
	int skip;
	SDL_Texture *bmp;
	int width, height; /* source height & width */
	AVRational sample_aspect_ratio;
	int allocated;
	int reallocate;

#if CONFIG_AVFILTER
	AVFilterBufferRef *picref;
#endif
} VideoPicture;

typedef struct SubPicture {
	double pts; /* presentation time stamp for this picture */
	AVSubtitle sub;
} SubPicture;

typedef struct AudioParams {
	int freq;
	int channels;
	int64_t channel_layout;
	enum AVSampleFormat fmt;
} AudioParams;

enum {
	AV_SYNC_AUDIO_MASTER, /* default choice */
	AV_SYNC_VIDEO_MASTER,
	AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};
//视频显示模式
enum V_Show_Mode {
	SHOW_MODE_YUV = 0, SHOW_MODE_Y, SHOW_MODE_U, SHOW_MODE_V
};

typedef struct VideoState {
	SDL_Thread *read_tid;
	SDL_Thread *video_tid;
	SDL_Thread *refresh_tid;
	const AVInputFormat *iformat;
	int no_background;
	int abort_request;
	int force_refresh;
	int paused;
	int last_paused;
	int que_attachments_req;
	int seek_req;
	int seek_flags;
	int64_t seek_pos;
	int64_t seek_rel;
	int read_pause_return;
	AVFormatContext *ic;

	int audio_stream;
	AVCodecContext *video_codec_ctx;
	AVCodecContext *audio_codec_ctx;

	int av_sync_type;
	double external_clock; /* external clock base */
	int64_t external_clock_time;

	double audio_clock;
	double audio_diff_cum; /* used for AV difference average computation */
	double audio_diff_avg_coef;
	double audio_diff_threshold;
	int audio_diff_avg_count;
	AVStream *audio_st;
	PacketQueue audioq;
	int audio_hw_buf_size;
	uint8_t audio_buf2[192000 * 4]; // Approximate max audio frame size
	uint8_t silence_buf[SDL_AUDIO_BUFFER_SIZE];
	uint8_t *audio_buf;
	uint8_t *audio_buf1;
	unsigned int audio_buf_size; /* in bytes */
	unsigned int audio_buf_index; /* in bytes */
	int audio_write_buf_size;
	AVPacket audio_pkt_temp;
	AVPacket audio_pkt;
	struct AudioParams audio_src;
	struct AudioParams audio_tgt;
	struct SwrContext *swr_ctx;
	SDL_AudioDeviceID audio_dev;
	SDL_AudioStream *audio_stream_obj;
	double audio_current_pts;
	double audio_current_pts_drift;
	int frame_drops_early;
	int frame_drops_late;
	AVFrame *frame;

	enum ShowMode {
		SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
	} show_mode;
	int16_t sample_array[SAMPLE_ARRAY_SIZE];
	int sample_array_index;
	int last_i_start;
	RDFTContext *rdft;
	int rdft_bits;
	FFTSample *rdft_data;
	int xpos;

	SDL_Thread *subtitle_tid;
	int subtitle_stream;
	int subtitle_stream_changed;
	AVStream *subtitle_st;
	PacketQueue subtitleq;
	SubPicture subpq[SUBPICTURE_QUEUE_SIZE];
	int subpq_size, subpq_rindex, subpq_windex;
	SDL_mutex *subpq_mutex;
	SDL_Condition *subpq_cond;

	double frame_timer;
	double frame_last_pts;
	double frame_last_duration;
	double frame_last_dropped_pts;
	double frame_last_returned_time;
	double frame_last_filter_delay;
	int64_t frame_last_dropped_pos;
	double video_clock;                          ///< pts of last decoded frame / predicted pts of next decoded frame
	int video_stream;
	AVStream *video_st;
	PacketQueue videoq;
	double video_current_pts;                    ///< current displayed pts (different from video_clock if frame fifos are used)
	double video_current_pts_drift;              ///< video_current_pts - time (av_gettime) at which we updated video_current_pts - used to have running video pts
	int64_t video_current_pos;                   ///< current displayed file pos
	VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
	int pictq_size, pictq_rindex, pictq_windex;
	SDL_mutex *pictq_mutex;
	SDL_Condition *pictq_cond;
#if !CONFIG_AVFILTER
	struct SwsContext *img_convert_ctx;
#endif

	char filename[1024];
	int width, height, xleft, ytop;
	int step;

#if CONFIG_AVFILTER
	AVFilterContext *in_video_filter;           ///< the first filter in the video chain
	AVFilterContext *out_video_filter;          ///< the last filter in the video chain
	int use_dr1;
	FrameBuffer *buffer_pool;
#endif

	int refresh;
	int last_video_stream, last_audio_stream, last_subtitle_stream;

	SDL_Condition *continue_read_thread;
	//视频显示模式------------
	enum V_Show_Mode v_show_mode;
} VideoState;

VideoState *g_is = NULL;

enum ShowMode {
	SHOW_MODE_NONE = -1, SHOW_MODE_VIDEO = 0, SHOW_MODE_WAVES, SHOW_MODE_RDFT, SHOW_MODE_NB
} ;
//指向MFC对话框的指针
CVideoEyeDlg * dlg;

/* options specified by the user */
static const AVInputFormat *file_iformat;
static char *input_filename;

static const char *window_title;
static int fs_screen_width;
static int fs_screen_height;
static int screen_width  = 0;
static int screen_height = 0;
static int audio_disable ;
static int video_disable ;
static int wanted_stream[AVMEDIA_TYPE_NB] = {-1,-1,0,-1,0};
//   [AVMEDIA_TYPE_AUDIO]    = -1,
//   [AVMEDIA_TYPE_VIDEO]    = -1,
//   [AVMEDIA_TYPE_SUBTITLE] = -1,};
static int seek_by_bytes = -1;
static int display_disable;
static int show_status = 1;
static int av_sync_type = AV_SYNC_AUDIO_MASTER;
static int64_t start_time = AV_NOPTS_VALUE;
static int64_t duration = AV_NOPTS_VALUE;
static int workaround_bugs = 1;
static int fast = 0;
static int genpts = 0;
static int lowres = 0;
static int idct = FF_IDCT_AUTO;
static enum AVDiscard skip_frame       = AVDISCARD_DEFAULT;
static enum AVDiscard skip_idct        = AVDISCARD_DEFAULT;
static enum AVDiscard skip_loop_filter = AVDISCARD_DEFAULT;
static int error_concealment = 3;
static int decoder_reorder_pts = -1;
static int autoexit;
static int exit_on_keydown;
static int exit_on_mousedown;
static int loop = 1;
static int framedrop = -1;
static int infinite_buffer = -1;
static enum ShowMode show_mode = SHOW_MODE_NONE;
static const char *audio_codec_name;
static const char *subtitle_codec_name;
static const char *video_codec_name;
static int rdftspeed = 20;
#if CONFIG_AVFILTER
static char *vfilters = NULL;
#endif


/* current context */
static int is_full_screen;
static int64_t audio_callback_time;

static AVPacket flush_pkt;
//支持输出FLV和MP4等，以及h.264原始码流--------------
//需要在输出文件前写入SPS,PPS------------
int dataoutput_h264_ft;

#define FF_ALLOC_EVENT   (SDL_USEREVENT)
#define FF_REFRESH_EVENT (SDL_USEREVENT + 1)
#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)
//---------------------------

//自定义一个事件用于进度条更新
#define VE_SEEK_BAR_EVENT    (SDL_USEREVENT + 4)
int seek_bar_pos;


//是否拉伸-------------------------
#define VE_STRETCH_EVENT (SDL_USEREVENT + 5)
int is_stretch=1;
//--------------------------------
//专门使用的标志，在程序要退出时设置为1
static int exit_remark=0;

//---------------------------------
static SDL_Window *window;
static SDL_Renderer *renderer;

//视频帧索引
//视频帧计数
int vframe_index=0;
//音频帧计数
int aframe_index=0;
//Packet计数
int packet_index=0;
int ve_reset_index(){
	vframe_index=0;
	aframe_index=0;
	packet_index=0;
	return 0;
}
//更新MFC界面信息成功返回0失败返回-1
//全局的，只调用一次
int ve_param_global(VideoState *is){
	//初始化
	CString input_protocol,input_format,wxh,decoder_name,
		decoder_type,bitrate,extention,pix_fmt,framerate,timelong,decoder_name_au,sample_rate_au,channels_au;
	float framerate_temp,timelong_temp,bitrate_temp;
	//注意：从int不能直接转换为LPCTSTR
	//CString可以直接赋值给LPCTSTR
	AVFormatContext *pFormatCtx = is->ic;
	int video_stream=is->video_stream;
	int audio_stream=is->audio_stream;
	AVCodecContext *pCodecCtx = NULL;
	AVCodecContext *pCodecCtx_au = NULL;
	
	if (video_stream >= 0 && video_stream < (int)pFormatCtx->nb_streams) {
		pCodecCtx = avcodec_alloc_context3(NULL);
		avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[video_stream]->codecpar);
	}
	
	AVCodecParameters *audio_par = NULL;
	if (audio_stream >= 0 && audio_stream < (int)pFormatCtx->nb_streams) {
		pCodecCtx_au = avcodec_alloc_context3(NULL);
		audio_par = pFormatCtx->streams[audio_stream]->codecpar;
		avcodec_parameters_to_context(pCodecCtx_au, audio_par);
	}

#ifdef _UNICODE
	USES_CONVERSION;
#endif

	if(pFormatCtx->pb!=NULL){
		URLContext *uc=(URLContext *)pFormatCtx->pb->opaque;
		if(strcmp(is->filename,"read_memory")==0){
			dlg->m_input_protocol.SetWindowText(_T("Memory"));
		}else{
			URLProtocol *up=(URLProtocol *)uc->prot;
			//输入文件的协议----------
#ifdef _UNICODE
			input_protocol.Format(_T("%s"),A2W(up->name));
#else
			input_protocol.Format(_T("%s"),up->name);
#endif
			dlg->m_input_protocol.SetWindowText(input_protocol);
		}

	}
	//视频信息是否为视频的时候显示？
	if(pCodecCtx){
		wxh.Format(_T("%d x %d"),pCodecCtx->width,pCodecCtx->height);
		dlg->m_wxh.SetWindowText(wxh);

#ifdef _UNICODE
		decoder_name.Format(_T("%s"),A2W(avcodec_get_name(pCodecCtx->codec_id)));
#else
		decoder_name.Format(_T("%s"),avcodec_get_name(pCodecCtx->codec_id));
#endif

		dlg->m_decoder_name.SetWindowText(decoder_name);
		//帧率
		framerate_temp=static_cast<float>(pFormatCtx->streams[video_stream]->r_frame_rate.num)/pFormatCtx->streams[video_stream]->r_frame_rate.den;
		framerate.Format(_T("%5.2ffps"),framerate_temp);
		dlg->m_framerate.SetWindowText(framerate);

		switch(pCodecCtx->pix_fmt){
		case 0:
			pix_fmt.Format(_T("YUV420P"));break;
		case 1:
			pix_fmt.Format(_T("YUYV422"));break;
		case 2:
			pix_fmt.Format(_T("RGB24"));break;
		case 3:
			pix_fmt.Format(_T("BGR24"));break;
		case 12:
			pix_fmt.Format(_T("PIX_FMT_YUVJ420P"));break;	
		default:
			pix_fmt.Format(_T("UNKNOWN"));
		}
		dlg->m_pix_fmt.SetWindowText(pix_fmt);
	}
	//音频信息更新，当有音频流时更新
	if(pCodecCtx_au && audio_par){
#ifdef _UNICODE
		decoder_name_au.Format(_T("%s"),A2W(avcodec_get_name(pCodecCtx_au->codec_id)));
#else
		decoder_name_au.Format(_T("%s"),avcodec_get_name(pCodecCtx_au->codec_id));
#endif
		dlg->m_decoder_name_au.SetWindowText(decoder_name_au);
		sample_rate_au.Format(_T("%d"),pCodecCtx_au->sample_rate);
		dlg->m_sample_rate_au.SetWindowText(sample_rate_au);
		channels_au.Format(_T("%d"), audio_par->ch_layout.nb_channels);
		dlg->m_channels_au.SetWindowText(channels_au);
	}
	//显示比特率k为单位
	bitrate_temp=((float)(pFormatCtx->bit_rate))/1000;
	bitrate.Format(_T("%5.2fkbps"),bitrate_temp);
	dlg->m_bitrate.SetWindowText(bitrate);
	//duration以微秒为单位
	timelong_temp=static_cast<float>(pFormatCtx->duration)/1000000;
	//转换为hh:mm:ss格式
	int tns, thh, tmm, tss;
	tns  = static_cast<int>(pFormatCtx->duration)/1000000;
	thh  = tns / 3600;
	tmm  = (tns % 3600) / 60;
	tss  = (tns % 60);
	timelong.Format(_T("%02d:%02d:%02d"),thh,tmm,tss);
	dlg->m_timelong.SetWindowText(timelong);
	dlg->m_duration.SetWindowText(timelong);
	//输入文件的封装格式------
#ifdef _UNICODE
	input_format.Format(_T("%s"),A2W(pFormatCtx->iformat->long_name));
#else
	input_format.Format(_T("%s"),pFormatCtx->iformat->long_name);
#endif
	
	dlg->m_input_format.SetWindowText(input_format);
	//------------------------


	//bitrate.Format("%d",pCodecCtx->bit_rate);
	//dlg->m_bitrate.SetWindowText(bitrate);

#ifdef _UNICODE
	extention.Format(_T("%s"),A2W(pFormatCtx->iformat->extensions));
#else
	extention.Format(_T("%s"),pFormatCtx->iformat->extensions);
#endif

	dlg->m_extention.SetWindowText(extention);
	//MetaData------------------------------------------------------------
	//��AVDictionary���?
	//需要用到AVDictionaryEntry结构
	//CString author,copyright,description;
	CString meta=NULL,key,value;
	AVDictionaryEntry *m = NULL;
	//一个个的查找
	/*	m=av_dict_get(pFormatCtx->metadata,"author",m,0);
	author.Format("���ߣ�%s",m->value);
	m=av_dict_get(pFormatCtx->metadata,"copyright",m,0);
	copyright.Format("��Ȩ��%s",m->value);
	m=av_dict_get(pFormatCtx->metadata,"description",m,0);
	description.Format("描述：%s",m->value);
	*/
	//使用循环获取
	//(需要获取所有数据，键名，前一个键，循环时使用，忽略后缀)
	while(m=av_dict_get(pFormatCtx->metadata,"",m,AV_DICT_IGNORE_SUFFIX)){
#ifdef _UNICODE
		key.Format(_T("%s"),A2W(m->key));
		value.Format(_T("%s"),A2W(m->value));
		meta.AppendFormat(_T("%s\t:%s\r\n"),(LPCTSTR)key,(LPCTSTR)value);
#else
		key.Format(_T("%s"),m->key);
		value.Format(_T("%s"),m->value);
		meta.AppendFormat(_T("%s\t:%s\r\n"),(LPCTSTR)key,(LPCTSTR)value);
#endif
	}

	//EditControl中显示\n的地方，需要使用\r\n
	//如果要换\r\n行，需要将CEdit控件属性设置为，
	//Auto HScroll 设置为 False
	//MultiLine  设置为 True

	//dlg->m_metadata.SetWindowText(author+"\r\n"+copyright+"\r\n"+description);
	dlg->m_metadata.SetWindowText(meta);
	//--------------------------------------------------------------------
	return 0;
}
//每帧调用一次
//做一些全局更新

//更新画面
int ve_param_packet(VideoState *is,AVPacket *packet){
	//--------------------------------------------------------------------
	AVFormatContext *pFormatCtx = is->ic;
	int video_stream=is->video_stream;
	int audio_stream=is->audio_stream;
	AVCodecContext *pCodecCtx = get_stream_codec_context(pFormatCtx, video_stream);
	//帧数太多，处理一部分之后，让用户选择--------------------------
	if(packet_index>=MAX_PACKET_NUM){
		dlg->SystemClear();
	}

	//计数-----------------------------------------------
	packet_index++;
	return 0;
}


//视频帧参数提取
int ve_param_vframe(VideoState *is,AVFrame *pFrame,AVPacket *packet){
	//--------------------------------------------------------------------
	CString key_frame,pict_type,reference,f_index,pts,codednum;
	AVFormatContext *pFormatCtx = is->ic;
	int video_stream=is->video_stream;
	AVCodecContext *pCodecCtx = get_stream_codec_context(pFormatCtx, video_stream);
	//帧数太多，处理一部分之后，让用户选择--------------------------

	if(vframe_index>=MAX_FRAME_NUM){
		dlg->SystemClear();
	}
	//
	//------------------------------
	f_index.Format(_T("%d"),vframe_index);
	//获取当前记录条数
	int nIndex=dlg->videodecode->m_decodeframe_v.GetItemCount();
	//创建数据结构
	LV_ITEM lvitem;
	lvitem.mask=LVIF_TEXT;
	lvitem.iItem=nIndex;
	lvitem.iSubItem=0;
	//注意vframe_index不能直接赋值给
	//要使用f_index执行Format!后再赋值
	lvitem.pszText=f_index.GetBuffer();
	//------------------------


	if(pFrame->pict_type == AV_PICTURE_TYPE_I){
		key_frame.Format(_T("Yes"));
	} else {
		key_frame.Format(_T("No"));
	}

	switch(pFrame->pict_type){
	case 0:
		pict_type.Format(_T("Unknown"));break;
	case 1:
		pict_type.Format(_T("I"));break;
	case 2:
		pict_type.Format(_T("P"));break;
	case 3:
		pict_type.Format(_T("B"));break;
	case 4:
		pict_type.Format(_T("S"));break;
	case 5:
		pict_type.Format(_T("SI"));break;
	case 6:
		pict_type.Format(_T("SP"));break;
	case 7:
		pict_type.Format(_T("BI"));break;
	default:
		pict_type.Format(_T("Unknown"));
	}

	reference.Format(_T("%d"), 0); // reference deprecated in newer FFmpeg
	pts.Format(_T("%lld"), pFrame->pts);
	codednum.Format(_T("%d"), 0); // coded_picture_number deprecated in newer FFmpeg

	//添加到末尾?-----------------------
	dlg->videodecode->m_decodeframe_v.InsertItem(&lvitem);
	dlg->videodecode->m_decodeframe_v.SetItemText(nIndex,1,pict_type);
	dlg->videodecode->m_decodeframe_v.SetItemText(nIndex,2,key_frame);
	dlg->videodecode->m_decodeframe_v.SetItemText(nIndex,3,codednum);
	dlg->videodecode->m_decodeframe_v.SetItemText(nIndex,4,pts);
	dlg->videodecode->m_decodeframe_v.SendMessage(WM_VSCROLL, SB_BOTTOM, NULL);
	//准备工作
	//视频帧相关准备-----------------
	// Note: Many AVFrame members are deprecated in newer FFmpeg versions
	dlg->dfanalysis->qscale_table = NULL; // AVFrame_qscale_table deprecated
	dlg->dfanalysis->qscale_type = 0; // AVFrame_qscale_type deprecated
	dlg->dfanalysis->mb_stride = pCodecCtx->width/16+1;
	dlg->dfanalysis->mb_sum = ((pCodecCtx->height+15)>>4)*(pCodecCtx->width/16+1);
	dlg->dfanalysis->mb_type = NULL; // AVFrame_mb_type deprecated
	dlg->dfanalysis->motion_val0 = NULL; // AVFrame_motion_val deprecated
	dlg->dfanalysis->motion_val1 = NULL; // AVFrame_motion_val deprecated
	dlg->dfanalysis->motion_subsample_log2 = 0; // AVFrame_motion_subsample_log2 deprecated
	dlg->dfanalysis->mb_width =(pCodecCtx->width+15)>>4;
	
	dlg->dfanalysis->codec_id=pCodecCtx->codec_id;
	dlg->dfanalysis->ref_index0 = NULL; // AVFrame_ref_index deprecated
	dlg->dfanalysis->ref_index1 = NULL; // AVFrame_ref_index deprecated
	dlg->dfanalysis->frame_index = vframe_index;
	dlg->dfanalysis->pict_type = pFrame->pict_type;

	dlg->dfanalysis->pts = pFrame->pts;


	dlg->dfanalysis->width = pFrame->width;
	dlg->dfanalysis->height = pFrame->height;
	//数据
	dlg->dfanalysis->data[0]=pFrame->data[0];
	dlg->dfanalysis->data[1]=pFrame->data[1];
	dlg->dfanalysis->data[2]=pFrame->data[2];
	dlg->dfanalysis->linesize[0]=pFrame->linesize[0];
	dlg->dfanalysis->linesize[1]=pFrame->linesize[1];
	dlg->dfanalysis->linesize[2]=pFrame->linesize[2];

	dlg->dfanalysis->pix_fmt=pCodecCtx->pix_fmt;

	//判断视频帧图像是否有效
	// AVFrame_owner deprecated in newer FFmpeg
	dlg->dfanalysis->refs = 0;
	//获取时间，以微秒为单位
	dlg->dfanalysis->ptime =((float)pFrame->pts)*((float)pFormatCtx->streams[video_stream]->time_base.num)/((float)pFormatCtx->streams[video_stream]->time_base.den);
	//视频帧自动分析----------------------
	//是否自动
	if(dlg->dfanalysis->m_dfanalysisauto.GetCheck()==1){
		//获取帧数间隔
		int interframe=dlg->dfanalysis->m_dfanalysisautointerframenum;
		if(vframe_index%interframe==0){
			dlg->dfanalysis->DrawPic();
		}
	}

	vframe_index++;
	return 0;
}

//音频帧参数提取
int ve_param_aframe(VideoState *is,AVFrame *pFrame,AVPacket *packet){
	//--------------------------------------------------------------------
	AVFormatContext *pFormatCtx = is->ic;
	int audio_stream=is->audio_stream;
	AVCodecContext *pCodecCtx = get_stream_codec_context(pFormatCtx, audio_stream);
	//帧数太多，处理一部分之后，让用户选择--------------------------

	if(aframe_index>=MAX_FRAME_NUM){
		dlg->SystemClear();
	}
	//------------------------------
	CString f_index,packet_size,dts,pts;
	//---------------
	f_index.Format(_T("%d"),aframe_index);
	//获取当前记录条数
	int nIndex=dlg->audiodecode->m_decodeframe_a.GetItemCount();
	//创建数据结构
	LV_ITEM lvitem;
	lvitem.mask=LVIF_TEXT;
	lvitem.iItem=nIndex;
	lvitem.iSubItem=0;
	//注意frame_index不能直接赋值给
	//要使用f_index执行Format后再赋值
	lvitem.pszText=f_index.GetBuffer();
	//------------------------
	packet_size.Format(_T("%d"),packet->size);
	pts.Format(_T("%lld"),static_cast<int64_t>(packet->pts));
	dts.Format(_T("%lld"),static_cast<int64_t>(packet->dts));
	//---------------
	dlg->audiodecode->m_decodeframe_a.InsertItem(&lvitem);
	dlg->audiodecode->m_decodeframe_a.SetItemText(nIndex,1,packet_size);
	dlg->audiodecode->m_decodeframe_a.SetItemText(nIndex,2,pts);
	dlg->audiodecode->m_decodeframe_a.SetItemText(nIndex,3,dts);
	dlg->audiodecode->m_decodeframe_a.SendMessage(WM_VSCROLL, SB_BOTTOM, NULL);

	aframe_index++;
	return 0;
}


//--------------------------------------------------
static int packet_queue_put(PacketQueue *q, AVPacket *pkt);
//放入队列Packet(队列)?
static int packet_queue_put_private(PacketQueue *q, AVPacket *pkt)
{
	AVPacketList *pkt1;

	if (q->abort_request)
		return -1;
	//PacketList和Packet是一个链表结构?
	//每个PacketList只有一个Packet
	pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	if (!q->last_pkt)
		//第一个
		q->first_pkt = pkt1;
	else
		//添加到末尾?
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size + sizeof(*pkt1);
	/* XXX: should duplicate packet data in DV case */
	SDL_SignalCondition(q->cond);
	return 0;
}
//������������Packet
static int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{
	int ret;

	/* duplicate the packet */
	if (pkt != &flush_pkt) {
		AVPacket *new_pkt = av_packet_alloc();
		if (!new_pkt || av_packet_ref(new_pkt, pkt) < 0) {
			av_packet_free(&new_pkt);
			return -1;
		}
		pkt = new_pkt;
	}

	SDL_LockMutex(q->mutex);
	//放入队列Packet(队列)?
	ret = packet_queue_put_private(q, pkt);
	SDL_UnlockMutex(q->mutex);

	if (pkt != &flush_pkt && ret < 0)
		av_packet_unref(pkt);

	return ret;
}

/* packet queue handling */
static void packet_queue_init(PacketQueue *q)
{
	memset(q, 0, sizeof(PacketQueue));
	q->mutex = SDL_CreateMutex();
	q->cond = SDL_CreateCondition();
	q->abort_request = 1;
}

static void packet_queue_flush(PacketQueue *q)
{
	AVPacketList *pkt, *pkt1;

	SDL_LockMutex(q->mutex);
	for (pkt = q->first_pkt; pkt != NULL; pkt = pkt1) {
		pkt1 = pkt->next;
		av_packet_unref(&pkt->pkt);
		av_freep(&pkt);
	}
	q->last_pkt = NULL;
	q->first_pkt = NULL;
	q->nb_packets = 0;
	q->size = 0;
	SDL_UnlockMutex(q->mutex);
}

static void packet_queue_destroy(PacketQueue *q)
{
	packet_queue_flush(q);
	SDL_DestroyMutex(q->mutex);
	SDL_DestroyCondition(q->cond);
}

static void packet_queue_abort(PacketQueue *q)
{
	SDL_LockMutex(q->mutex);

	q->abort_request = 1;

	SDL_SignalCondition(q->cond);

	SDL_UnlockMutex(q->mutex);
}

static void packet_queue_start(PacketQueue *q)
{
	SDL_LockMutex(q->mutex);
	q->abort_request = 0;
	packet_queue_put_private(q, &flush_pkt);
	SDL_UnlockMutex(q->mutex);
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
	AVPacketList *pkt1;
	int ret;

	SDL_LockMutex(q->mutex);

	for (;;) {
		if (q->abort_request) {
			ret = -1;
			break;
		}

		pkt1 = q->first_pkt;
		if (pkt1) {
			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			q->size -= pkt1->pkt.size + sizeof(*pkt1);
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = 1;
			break;
		} else if (!block) {
			ret = 0;
			break;
		} else {
			SDL_WaitCondition(q->cond, q->mutex);
		}
	}
	SDL_UnlockMutex(q->mutex);
	return ret;
}

static inline void fill_rectangle(SDL_Renderer *renderer,
	int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b)
{
	SDL_FRect rect;
	rect.x = (float)x;
	rect.y = (float)y;
	rect.w = (float)w;
	rect.h = (float)h;
	SDL_SetRenderDrawColor(renderer, r, g, b, 255);
	SDL_RenderFillRect(renderer, &rect);
}

#define ALPHA_BLEND(a, oldp, newp, s)\
	((((oldp << s) * (255 - (a))) + (newp * (a))) / (255 << s))

#define RGBA_IN(r, g, b, a, s)\
{\
	unsigned int v = ((const uint32_t *)(s))[0];\
	a = (v >> 24) & 0xff;\
	r = (v >> 16) & 0xff;\
	g = (v >> 8) & 0xff;\
	b = v & 0xff;\
}

#define YUVA_IN(y, u, v, a, s, pal)\
{\
	unsigned int val = ((const uint32_t *)(pal))[*(const uint8_t*)(s)];\
	a = (val >> 24) & 0xff;\
	y = (val >> 16) & 0xff;\
	u = (val >> 8) & 0xff;\
	v = val & 0xff;\
}

#define YUVA_OUT(d, y, u, v, a)\
{\
	((uint32_t *)(d))[0] = (a << 24) | (y << 16) | (u << 8) | v;\
}


#define BPP 1

static void blend_subrect(AVPicture *dst, const AVSubtitleRect *rect, int imgw, int imgh)
{
	int wrap, wrap3, width2, skip2;
	int y, u, v, a, u1, v1, a1, w, h;
	uint8_t *lum, *cb, *cr;
	const uint8_t *p;
	const uint32_t *pal;
	int dstx, dsty, dstw, dsth;

	dstw = av_clip(rect->w, 0, imgw);
	dsth = av_clip(rect->h, 0, imgh);
	dstx = av_clip(rect->x, 0, imgw - dstw);
	dsty = av_clip(rect->y, 0, imgh - dsth);
	lum = dst->data[0] + dsty * dst->linesize[0];
	cb  = dst->data[1] + (dsty >> 1) * dst->linesize[1];
	cr  = dst->data[2] + (dsty >> 1) * dst->linesize[2];

	width2 = ((dstw + 1) >> 1) + (dstx & ~dstw & 1);
	skip2 = dstx >> 1;
	wrap = dst->linesize[0];
	wrap3 = rect->linesize[0];
	p = rect->data[0];
	pal = (const uint32_t *)rect->data[1];  /* Now in YCrCb! */

	if (dsty & 1) {
		lum += dstx;
		cb += skip2;
		cr += skip2;

		if (dstx & 1) {
			YUVA_IN(y, u, v, a, p, pal);
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
			cr[0] = ALPHA_BLEND(a >> 2, cr[0], v, 0);
			cb++;
			cr++;
			lum++;
			p += BPP;
		}
		for (w = dstw - (dstx & 1); w >= 2; w -= 2) {
			YUVA_IN(y, u, v, a, p, pal);
			u1 = u;
			v1 = v;
			a1 = a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

			YUVA_IN(y, u, v, a, p + BPP, pal);
			u1 += u;
			v1 += v;
			a1 += a;
			lum[1] = ALPHA_BLEND(a, lum[1], y, 0);
			cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 1);
			cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 1);
			cb++;
			cr++;
			p += 2 * BPP;
			lum += 2;
		}
		if (w) {
			YUVA_IN(y, u, v, a, p, pal);
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
			cr[0] = ALPHA_BLEND(a >> 2, cr[0], v, 0);
			p++;
			lum++;
		}
		p += wrap3 - dstw * BPP;
		lum += wrap - dstw - dstx;
		cb += dst->linesize[1] - width2 - skip2;
		cr += dst->linesize[2] - width2 - skip2;
	}
	for (h = dsth - (dsty & 1); h >= 2; h -= 2) {
		lum += dstx;
		cb += skip2;
		cr += skip2;

		if (dstx & 1) {
			YUVA_IN(y, u, v, a, p, pal);
			u1 = u;
			v1 = v;
			a1 = a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			p += wrap3;
			lum += wrap;
			YUVA_IN(y, u, v, a, p, pal);
			u1 += u;
			v1 += v;
			a1 += a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 1);
			cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 1);
			cb++;
			cr++;
			p += -wrap3 + BPP;
			lum += -wrap + 1;
		}
		for (w = dstw - (dstx & 1); w >= 2; w -= 2) {
			YUVA_IN(y, u, v, a, p, pal);
			u1 = u;
			v1 = v;
			a1 = a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

			YUVA_IN(y, u, v, a, p + BPP, pal);
			u1 += u;
			v1 += v;
			a1 += a;
			lum[1] = ALPHA_BLEND(a, lum[1], y, 0);
			p += wrap3;
			lum += wrap;

			YUVA_IN(y, u, v, a, p, pal);
			u1 += u;
			v1 += v;
			a1 += a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

			YUVA_IN(y, u, v, a, p + BPP, pal);
			u1 += u;
			v1 += v;
			a1 += a;
			lum[1] = ALPHA_BLEND(a, lum[1], y, 0);

			cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 2);
			cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 2);

			cb++;
			cr++;
			p += -wrap3 + 2 * BPP;
			lum += -wrap + 2;
		}
		if (w) {
			YUVA_IN(y, u, v, a, p, pal);
			u1 = u;
			v1 = v;
			a1 = a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			p += wrap3;
			lum += wrap;
			YUVA_IN(y, u, v, a, p, pal);
			u1 += u;
			v1 += v;
			a1 += a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u1, 1);
			cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v1, 1);
			cb++;
			cr++;
			p += -wrap3 + BPP;
			lum += -wrap + 1;
		}
		p += wrap3 + (wrap3 - dstw * BPP);
		lum += wrap + (wrap - dstw - dstx);
		cb += dst->linesize[1] - width2 - skip2;
		cr += dst->linesize[2] - width2 - skip2;
	}
	/* handle odd height */
	if (h) {
		lum += dstx;
		cb += skip2;
		cr += skip2;

		if (dstx & 1) {
			YUVA_IN(y, u, v, a, p, pal);
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
			cr[0] = ALPHA_BLEND(a >> 2, cr[0], v, 0);
			cb++;
			cr++;
			lum++;
			p += BPP;
		}
		for (w = dstw - (dstx & 1); w >= 2; w -= 2) {
			YUVA_IN(y, u, v, a, p, pal);
			u1 = u;
			v1 = v;
			a1 = a;
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);

			YUVA_IN(y, u, v, a, p + BPP, pal);
			u1 += u;
			v1 += v;
			a1 += a;
			lum[1] = ALPHA_BLEND(a, lum[1], y, 0);
			cb[0] = ALPHA_BLEND(a1 >> 2, cb[0], u, 1);
			cr[0] = ALPHA_BLEND(a1 >> 2, cr[0], v, 1);
			cb++;
			cr++;
			p += 2 * BPP;
			lum += 2;
		}
		if (w) {
			YUVA_IN(y, u, v, a, p, pal);
			lum[0] = ALPHA_BLEND(a, lum[0], y, 0);
			cb[0] = ALPHA_BLEND(a >> 2, cb[0], u, 0);
			cr[0] = ALPHA_BLEND(a >> 2, cr[0], v, 0);
		}
	}
}

static void free_subpicture(SubPicture *sp)
{
	avsubtitle_free(&sp->sub);
}
//������ʾ���ڵ�λ�ã����̶ֹ��Ŀ��߱ȣ�
static void calculate_display_rect(SDL_Rect *rect, int scr_xleft, int scr_ytop, int scr_width, int scr_height, VideoPicture *vp)
{
	float aspect_ratio;
	int width, height, x, y;

	if (vp->sample_aspect_ratio.num == 0)
		aspect_ratio = 0;
	else
		aspect_ratio = static_cast<float>(av_q2d(vp->sample_aspect_ratio));

	if (aspect_ratio <= 0.0)
		aspect_ratio = 1.0;
	aspect_ratio *= (float)vp->width / (float)vp->height;

	/* XXX: we suppose the screen has a 1.0 pixel ratio */
	height = scr_height;
	width = ((int)my_rint(height * aspect_ratio)) & ~1;
	if (width > scr_width) {
		width = scr_width;
		height = ((int)my_rint(width / aspect_ratio)) & ~1;
	}
	x = (scr_width - width) / 2;
	y = (scr_height - height) / 2;
	rect->x = scr_xleft + x;
	rect->y = scr_ytop  + y;
	rect->w = FFMAX(width,  1);
	rect->h = FFMAX(height, 1);
}

//������ʾ���ڵ�λ�ã����������Ļ��?
static void calculate_display_rect_f(SDL_Rect *rect, int scr_xleft, int scr_ytop, int scr_width, int scr_height, VideoPicture *vp)
{
	rect->x = scr_xleft;
	rect->y = scr_ytop;
	rect->w =scr_width;
	rect->h = scr_height;
}


static void video_image_display(VideoState *is)
{
	VideoPicture *vp;
	SDL_FRect rect;
	char debugBuf[512];

	vp = &is->pictq[is->pictq_rindex];
	sprintf_s(debugBuf, sizeof(debugBuf), "video_image_display: vp=%p, vp->bmp=%p, pictq_rindex=%d\n", vp, vp->bmp, is->pictq_rindex);
	OutputDebugStringA(debugBuf);
	
	if (vp->bmp) {
		sprintf_s(debugBuf, sizeof(debugBuf), "video_image_display: Rendering texture, renderer=%p\n", renderer);
		OutputDebugStringA(debugBuf);
		
		//������ʾ����λ�ã����ַ�����
		if(is_stretch==0){
			SDL_Rect r;
			calculate_display_rect(&r, is->xleft, is->ytop, is->width, is->height, vp);
			rect.x = (float)r.x;
			rect.y = (float)r.y;
			rect.w = (float)r.w;
			rect.h = (float)r.h;
		}else if(is_stretch==1){
			SDL_Rect r;
			calculate_display_rect_f(&r, is->xleft, is->ytop, is->width, is->height, vp);
			rect.x = (float)r.x;
			rect.y = (float)r.y;
			rect.w = (float)r.w;
			rect.h = (float)r.h;
		}
		
		sprintf_s(debugBuf, sizeof(debugBuf), "video_image_display: Calling SDL_RenderTexture, rect=(%f,%f,%f,%f)\n", rect.x, rect.y, rect.w, rect.h);
		OutputDebugStringA(debugBuf);
		
		int ret1 = SDL_RenderTexture(renderer, vp->bmp, NULL, &rect);
		sprintf_s(debugBuf, sizeof(debugBuf), "video_image_display: SDL_RenderTexture returned %d\n", ret1);
		OutputDebugStringA(debugBuf);
		
		sprintf_s(debugBuf, sizeof(debugBuf), "video_image_display: Calling SDL_RenderPresent\n");
		OutputDebugStringA(debugBuf);
		
		SDL_RenderPresent(renderer);
		sprintf_s(debugBuf, sizeof(debugBuf), "video_image_display: SDL_RenderPresent returned\n");
		OutputDebugStringA(debugBuf);
	} else {
		sprintf_s(debugBuf, sizeof(debugBuf), "video_image_display: vp->bmp is NULL!\n");
		OutputDebugStringA(debugBuf);
	}
}

static inline int compute_mod(int a, int b)
{
	return a < 0 ? a%b + b : a%b;
}

static void video_audio_display(VideoState *s)
{
	int i, i_start, x, y1, y, ys, delay, n, nb_display_channels;
	int ch, channels, h, h2, fgcolor;
	int64_t time_diff;
	int rdft_bits, nb_freq;

	for (rdft_bits = 1; (1 << rdft_bits) < 2 * s->height; rdft_bits++)
		;
	nb_freq = 1 << (rdft_bits - 1);

	/* compute display index : center on currently output samples */
	channels = s->audio_tgt.channels;
	nb_display_channels = channels;
	if (!s->paused) {
		int data_used= s->show_mode == SHOW_MODE_WAVES ? s->width : (2*nb_freq);
		n = 2 * channels;
		delay = s->audio_write_buf_size;
		delay /= n;

		/* to be more precise, we take into account the time spent since
		the last buffer computation */
		if (audio_callback_time) {
			time_diff = av_gettime() - audio_callback_time;
			delay -= static_cast<int>((time_diff * s->audio_tgt.freq) / 1000000);
		}

		delay += 2 * data_used;
		if (delay < data_used)
			delay = data_used;

		i_start= x = compute_mod(s->sample_array_index - delay * channels, SAMPLE_ARRAY_SIZE);
		if (s->show_mode == SHOW_MODE_WAVES) {
			h = INT_MIN;
			for (i = 0; i < 1000; i += channels) {
				int idx = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
				int a = s->sample_array[idx];
				int b = s->sample_array[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
				int c = s->sample_array[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
				int d = s->sample_array[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
				int score = a - d;
				if (h < score && (b ^ c) < 0) {
					h = score;
					i_start = idx;
				}
			}
		}

		s->last_i_start = i_start;
	} else {
		i_start = s->last_i_start;
	}

	if (s->show_mode == SHOW_MODE_WAVES) {
		fill_rectangle(renderer,
			s->xleft, s->ytop, s->width, s->height,
			0x00, 0x00, 0x00);

		fgcolor = 0xffffff;

		/* total height for one channel */
		h = s->height / nb_display_channels;
		/* graph height / 2 */
		h2 = (h * 9) / 20;
		for (ch = 0; ch < nb_display_channels; ch++) {
			i = i_start + ch;
			y1 = s->ytop + ch * h + (h / 2); /* position of center line */
			for (x = 0; x < s->width; x++) {
				y = (s->sample_array[i] * h2) >> 15;
				if (y < 0) {
					y = -y;
					ys = y1 - y;
				} else {
					ys = y1;
				}
				fill_rectangle(renderer,
					s->xleft + x, ys, 1, y,
					0xff, 0xff, 0xff);
				i += channels;
				if (i >= SAMPLE_ARRAY_SIZE)
					i -= SAMPLE_ARRAY_SIZE;
			}
		}

		for (ch = 1; ch < nb_display_channels; ch++) {
			y = s->ytop + ch * h;
			fill_rectangle(renderer,
				s->xleft, y, s->width, 1,
				0x00, 0x00, 0xff);
		}
		SDL_RenderPresent(renderer);
	} else {
		nb_display_channels= FFMIN(nb_display_channels, 2);
		if (nb_display_channels < 1) {
			SDL_RenderPresent(renderer);
			return;
		}
		if (rdft_bits != s->rdft_bits) {
			av_rdft_end(s->rdft);
			av_free(s->rdft_data);
			s->rdft = av_rdft_init(rdft_bits, DFT_R2C);
			s->rdft_bits = rdft_bits;
			s->rdft_data = (FFTSample *)av_malloc(4 * nb_freq * sizeof(*s->rdft_data));
		}
		{
			FFTSample *data[2];
			for (ch = 0; ch < nb_display_channels; ch++) {
				data[ch] = s->rdft_data + 2 * nb_freq * ch;
				i = i_start + ch;
				for (x = 0; x < 2 * nb_freq; x++) {
					double w = (x-nb_freq) * (1.0 / nb_freq);
					data[ch][x] = static_cast<FFTSample>(s->sample_array[i] * (1.0 - w * w));
					i += channels;
					if (i >= SAMPLE_ARRAY_SIZE)
						i -= SAMPLE_ARRAY_SIZE;
				}
				av_rdft_calc(s->rdft, data[ch]);
			}
			// least efficient way to do this, we should of course directly access it but its more than fast enough
			for (y = 0; y < s->height; y++) {
				double w = 1 / sqrt((float)nb_freq);
				int a = static_cast<int>(sqrt(w * sqrt(data[0][2 * y + 0] * data[0][2 * y + 0] + data[0][2 * y + 1] * data[0][2 * y + 1])));
				int b = (nb_display_channels == 2 ) ? static_cast<int>(sqrt(w * sqrt(data[1][2 * y + 0] * data[1][2 * y + 0]
				+ data[1][2 * y + 1] * data[1][2 * y + 1]))) : a;
				a = FFMIN(a, 255);
				b = FFMIN(b, 255);
				fill_rectangle(renderer,
					s->xpos, s->height-y, 1, 1,
					(Uint8)a, (Uint8)b, (Uint8)((a + b) / 2));
			}
		}
		SDL_RenderPresent(renderer);
		if (!s->paused)
			s->xpos++;
		if (s->xpos >= s->width)
			s->xpos= s->xleft;
	}
}

static void stream_close(VideoState *is)
{
	VideoPicture *vp;
	int i;
	/* XXX: use a special url_shutdown call to abort parse cleanly */
	is->abort_request = 1;
	SDL_WaitThread(is->read_tid, NULL);
	SDL_WaitThread(is->refresh_tid, NULL);
	packet_queue_destroy(&is->videoq);
	packet_queue_destroy(&is->audioq);
	packet_queue_destroy(&is->subtitleq);

	/* free all pictures */
	for (i = 0; i < VIDEO_PICTURE_QUEUE_SIZE; i++) {
		vp = &is->pictq[i];
#if CONFIG_AVFILTER
		avfilter_unref_bufferp(&vp->picref);
#endif
		if (vp->bmp) {
			SDL_DestroyTexture(vp->bmp);
			vp->bmp = NULL;
		}
	}
	SDL_DestroyMutex(is->pictq_mutex);
	SDL_DestroyCondition(is->pictq_cond);
	SDL_DestroyMutex(is->subpq_mutex);
	SDL_DestroyCondition(is->subpq_cond);
	SDL_DestroyCondition(is->continue_read_thread);
#if !CONFIG_AVFILTER
	if (is->img_convert_ctx)
		sws_freeContext(is->img_convert_ctx);
#endif
	av_free(is);
}

//�˳�
static void do_exit(VideoState *is)
{
	exit_remark=1;
	if (is) {
		//ֱ��abort_request = 1��sleep�ǲ������εķ���
		//���������ֱ���˳�?
		//���������������?
		//is->abort_request = 1;
		//Sleep(2000);
		stream_close(is);
	}
	av_lockmgr_register(NULL);
	uninit_opts();
#if CONFIG_AVFILTER
	avfilter_uninit();
	av_freep(&vfilters);
#endif
	avformat_network_deinit();
	if (show_status)
		printf("\n");
	SDL_Quit();
	//FIX:������沥�ŵĴ��ڴ�С��̳�ǰһ����Ƶ���ڵĴ�С������
	//�ͷ�һЩȫ�ֱ���
	//SDL_FreeSurface(screen);
	screen_width  = 0;
	screen_height = 0;

	av_log(NULL, AV_LOG_QUIET, "%s", "");

	//����ֱ��ʹ��exit(0)����������������˳�?
	//exit(0);
	//��Ϊһ����ǣ���event_loop���жϲ��Զ��˳�
	//exit_remark=1;
}


void ve_quit()
{
	//����һЩ����
	dataoutput_h264_ft=1;
	//do_exit(g_is);
	if (g_is)
	{
		SDL_Event event;
		event.type = FF_QUIT_EVENT;
		event.user.data1 = g_is;
		{
			//����ֱ�Ӱ�is->abort_request����Ϊ1��������event_loop()ѭ����ֱ������
			//�Ӷ��޷�������������
			//g_is->abort_request = 1;
			SDL_PushEvent(&event);
		}
	}
	else
	{
		do_exit(NULL);
	}

}

static void sigterm_handler(int sig)
{
	exit(123);
}
//SDL��ʼ������
static int video_open(VideoState *is, int force_set_video_mode)
{
	char debugBuf[512];
	sprintf_s(debugBuf, "video_open called, is=%p, force_set_video_mode=%d\n", is, force_set_video_mode);
	OutputDebugStringA(debugBuf);
	int w,h;
	VideoPicture *vp = &is->pictq[is->pictq_rindex];
	SDL_Rect rect;

	if (is_full_screen && fs_screen_width) {
		w = fs_screen_width;
		h = fs_screen_height;
	} else if (!is_full_screen && screen_width) {
		w = screen_width;
		h = screen_height;
	} else if (vp->width) {
		calculate_display_rect(&rect, 0, 0, INT_MAX, vp->height, vp);
		w = rect.w;
		h = rect.h;
	} else {
		w = 640;
		h = 480;
	}
	if (window && is->width == w && is->height == h && !force_set_video_mode)
		return 0;
	if (window) {
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		window = NULL;
		renderer = NULL;
	}
	
	sprintf_s(debugBuf, "SDL_CreateWindow parameters: title='Video Window', w=%d, h=%d, flags=%u\n", w, h, (unsigned int)(SDL_WINDOW_RESIZABLE | (is_full_screen ? SDL_WINDOW_FULLSCREEN : 0)));
	OutputDebugStringA(debugBuf);
	window = SDL_CreateWindow("Video Window", w, h, SDL_WINDOW_RESIZABLE | (is_full_screen ? SDL_WINDOW_FULLSCREEN : 0));
	sprintf_s(debugBuf, "SDL_CreateWindow returned: %p\n", window);
	OutputDebugStringA(debugBuf);
	if (!window) {
		sprintf_s(debugBuf, "SDL_CreateWindow failed: %s\n", SDL_GetError());
		OutputDebugStringA(debugBuf);
		AfxMessageBox(_T("SDL: could not create window - exiting\n"));
		do_exit(is);
	}
	
	sprintf_s(debugBuf, "Calling SDL_ShowWindow...\n");
	OutputDebugStringA(debugBuf);
	SDL_ShowWindow(window);
	sprintf_s(debugBuf, "SDL_ShowWindow returned successfully\n");
	OutputDebugStringA(debugBuf);
	
	sprintf_s(debugBuf, "Calling SDL_CreateRenderer...\n");
	OutputDebugStringA(debugBuf);
	renderer = SDL_CreateRenderer(window, NULL);
	sprintf_s(debugBuf, "SDL_CreateRenderer returned: %p\n", renderer);
	OutputDebugStringA(debugBuf);
	if (!renderer) {
		sprintf_s(debugBuf, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
		OutputDebugStringA(debugBuf);
		AfxMessageBox(_T("SDL: could not create renderer - exiting\n"));
		do_exit(is);
	}

	window_title="Video Window";
	if (!window_title)
		window_title = input_filename;
	SDL_SetWindowTitle(window, window_title);

	is->width  = w;
	is->height = h;

	return 0;
}

/* display the current picture, if any */
static void video_display(VideoState *is)
{
	if (!window)
		video_open(is, 0);
	if (is->audio_st && is->show_mode != SHOW_MODE_VIDEO)
		video_audio_display(is);
	else if (is->video_st)
		video_image_display(is);
}

static int refresh_thread(void *opaque)
{
	VideoState *is= (VideoState *)opaque;
	char debugBuf[512];
	sprintf_s(debugBuf, sizeof(debugBuf), "refresh_thread started, is=%p, abort_request=%d\n", is, is->abort_request);
	OutputDebugStringA(debugBuf);
	
	while (!is->abort_request) {
		SDL_Event event;
		event.type = FF_REFRESH_EVENT;
		event.user.data1 = opaque;
		// 简化条件，确保视频帧能持续刷新
		if (!is->paused || is->force_refresh) {
			is->refresh = 1;
			int ret = SDL_PushEvent(&event);
			sprintf_s(debugBuf, sizeof(debugBuf), "refresh_thread: SDL_PushEvent returned %d\n", ret);
			OutputDebugStringA(debugBuf);
		}
		//FIXME ideally we should wait the correct time but SDLs event passing is so slow it would be silly
		av_usleep(is->audio_st && is->show_mode != SHOW_MODE_VIDEO ? rdftspeed*1000 : 5000);
	}
	sprintf_s(debugBuf, sizeof(debugBuf), "refresh_thread exiting\n");
	OutputDebugStringA(debugBuf);
	return 0;
}

/* get the current audio clock value */
static double get_audio_clock(VideoState *is)
{
	if (is->paused) {
		return is->audio_current_pts;
	} else {
		return is->audio_current_pts_drift + av_gettime() / 1000000.0;
	}
}

/* get the current video clock value */
static double get_video_clock(VideoState *is)
{
	if (is->paused) {
		return is->video_current_pts;
	} else {
		return is->video_current_pts_drift + av_gettime() / 1000000.0;
	}
}

/* get the current external clock value */
static double get_external_clock(VideoState *is)
{
	int64_t ti;
	ti = av_gettime();
	return is->external_clock + ((ti - is->external_clock_time) * 1e-6);
}

/* get the current master clock value */
static double get_master_clock(VideoState *is)
{
	double val;

	if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
		if (is->video_st)
			val = get_video_clock(is);
		else
			val = get_audio_clock(is);
	} else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
		if (is->audio_st)
			val = get_audio_clock(is);
		else
			val = get_video_clock(is);
	} else {
		val = get_external_clock(is);
	}
	return val;
}

/* seek in the stream */
static void stream_seek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes)
{
	if (!is->seek_req) {
		is->seek_pos = pos;
		is->seek_rel = rel;
		is->seek_flags &= ~AVSEEK_FLAG_BYTE;
		if (seek_by_bytes)
			is->seek_flags |= AVSEEK_FLAG_BYTE;
		is->seek_req = 1;
	}
}

/* pause or resume the video */
static void stream_toggle_pause(VideoState *is)
{
	if (is->paused) {
		is->frame_timer += av_gettime() / 1000000.0 + is->video_current_pts_drift - is->video_current_pts;
		if (is->read_pause_return != AVERROR(ENOSYS)) {
			is->video_current_pts = is->video_current_pts_drift + av_gettime() / 1000000.0;
		}
		is->video_current_pts_drift = is->video_current_pts - av_gettime() / 1000000.0;
	}
	is->paused = (is->paused == 0);
}

static double compute_target_delay(double delay, VideoState *is)
{
	double sync_threshold, diff;

	/* update delay to follow master synchronisation source */
	if (((is->av_sync_type == AV_SYNC_AUDIO_MASTER && is->audio_st) ||
		is->av_sync_type == AV_SYNC_EXTERNAL_CLOCK)) {
			/* if video is slave, we try to correct big delays by
			duplicating or deleting a frame */
			diff = get_video_clock(is) - get_master_clock(is);

			/* skip or repeat frame. We take into account the
			delay to compute the threshold. I still don't know
			if it is the best guess */
			sync_threshold = FFMAX(AV_SYNC_THRESHOLD, delay);
			if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
				if (diff <= -sync_threshold)
					delay = 0;
				else if (diff >= sync_threshold)
					delay = 2 * delay;
			}
	}

	//av_dlog(NULL, "video: delay=%0.3f A-V=%f\n",       delay, -diff);

	return delay;
}

static void pictq_next_picture(VideoState *is) {
	/* update queue size and signal for next picture */
	if (++is->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE)
		is->pictq_rindex = 0;

	SDL_LockMutex(is->pictq_mutex);
	is->pictq_size--;
	SDL_SignalCondition(is->pictq_cond);
	SDL_UnlockMutex(is->pictq_mutex);
}

static void pictq_prev_picture(VideoState *is) {
	VideoPicture *prevvp;
	/* update queue size and signal for the previous picture */
	prevvp = &is->pictq[(is->pictq_rindex + VIDEO_PICTURE_QUEUE_SIZE - 1) % VIDEO_PICTURE_QUEUE_SIZE];
	if (prevvp->allocated && !prevvp->skip) {
		SDL_LockMutex(is->pictq_mutex);
		if (is->pictq_size < VIDEO_PICTURE_QUEUE_SIZE - 1) {
			if (--is->pictq_rindex == -1)
				is->pictq_rindex = VIDEO_PICTURE_QUEUE_SIZE - 1;
			is->pictq_size++;
		}
		SDL_SignalCondition(is->pictq_cond);
		SDL_UnlockMutex(is->pictq_mutex);
	}
}

static void update_video_pts(VideoState *is, double pts, int64_t pos) {
	double time = av_gettime() / 1000000.0;
	/* update current video pts */
	is->video_current_pts = pts;
	is->video_current_pts_drift = is->video_current_pts - time;
	is->video_current_pos = pos;
	is->frame_last_pts = pts;
}

/* called to display each frame */
//ÿ40ms����һ�Σ���ʾͼƬ��
static void video_refresh(void *opaque)
{
	VideoState *is = (VideoState *)opaque;
	VideoPicture *vp;
	double time;

	SubPicture *sp, *sp2;

	char debugBuf[512];
	sprintf_s(debugBuf, sizeof(debugBuf), "video_refresh called, is=%p, video_st=%p, pictq_size=%d\n", is, is->video_st, is->pictq_size);
	OutputDebugStringA(debugBuf);

	if (is->video_st) {
		if (is->force_refresh)
			pictq_prev_picture(is);
retry:
		if (is->pictq_size == 0) {
			SDL_LockMutex(is->pictq_mutex);
			if (is->frame_last_dropped_pts != AV_NOPTS_VALUE && is->frame_last_dropped_pts > is->frame_last_pts) {
				update_video_pts(is, is->frame_last_dropped_pts, is->frame_last_dropped_pos);
				is->frame_last_dropped_pts = AV_NOPTS_VALUE;
			}
			SDL_UnlockMutex(is->pictq_mutex);
			// nothing to do, no picture to display in the que
		} else {
			double last_duration, duration, delay;
			/* dequeue the picture */
			vp = &is->pictq[is->pictq_rindex];

			if (vp->skip) {
				pictq_next_picture(is);
				goto retry;
			}

			if (is->paused)
				goto display;

			/* compute nominal last_duration */
			last_duration = vp->pts - is->frame_last_pts;
			if (last_duration > 0 && last_duration < 10.0) {
				/* if duration of the last frame was sane, update last_duration in video state */
				is->frame_last_duration = last_duration;
			}
			delay = compute_target_delay(is->frame_last_duration, is);

			time= av_gettime()/1000000.0;
			if (time < is->frame_timer + delay)
				return;

			if (delay > 0)
				is->frame_timer += delay * FFMAX(1, floor((time-is->frame_timer) / delay));

			SDL_LockMutex(is->pictq_mutex);
			update_video_pts(is, vp->pts, vp->pos);
			SDL_UnlockMutex(is->pictq_mutex);

			if (is->pictq_size > 1) {
				VideoPicture *nextvp = &is->pictq[(is->pictq_rindex + 1) % VIDEO_PICTURE_QUEUE_SIZE];
				duration = nextvp->pts - vp->pts;
				if((framedrop>0 || (framedrop && is->audio_st)) && time > is->frame_timer + duration){
					is->frame_drops_late++;
					pictq_next_picture(is);
					goto retry;
				}
			}

			if (is->subtitle_st) {
				if (is->subtitle_stream_changed) {
					SDL_LockMutex(is->subpq_mutex);

					while (is->subpq_size) {
						free_subpicture(&is->subpq[is->subpq_rindex]);

						/* update queue size and signal for next picture */
						if (++is->subpq_rindex == SUBPICTURE_QUEUE_SIZE)
							is->subpq_rindex = 0;

						is->subpq_size--;
					}
					is->subtitle_stream_changed = 0;

					SDL_SignalCondition(is->subpq_cond);
					SDL_UnlockMutex(is->subpq_mutex);
				} else {
					if (is->subpq_size > 0) {
						sp = &is->subpq[is->subpq_rindex];

						if (is->subpq_size > 1)
							sp2 = &is->subpq[(is->subpq_rindex + 1) % SUBPICTURE_QUEUE_SIZE];
						else
							sp2 = NULL;

						if ((is->video_current_pts > (sp->pts + ((float) sp->sub.end_display_time / 1000)))
							|| (sp2 && is->video_current_pts > (sp2->pts + ((float) sp2->sub.start_display_time / 1000))))
						{
							free_subpicture(sp);

							/* update queue size and signal for next picture */
							if (++is->subpq_rindex == SUBPICTURE_QUEUE_SIZE)
								is->subpq_rindex = 0;

							SDL_LockMutex(is->subpq_mutex);
							is->subpq_size--;
							SDL_SignalCondition(is->subpq_cond);
							SDL_UnlockMutex(is->subpq_mutex);
						}
					}
				}
			}

display:
			/* display picture */
			if (!display_disable)
				video_display(is);

			pictq_next_picture(is);
		}
	} else if (is->audio_st) {
		/* draw the next audio frame */

		/* if only audio stream, then display the audio bars (better
		than nothing, just to test the implementation */

		/* display picture */
		if (!display_disable)
			video_display(is);
	}
	is->force_refresh = 0;
	if (show_status) {
		static int64_t last_time;
		int64_t cur_time;
		int aqsize, vqsize, sqsize;
		double av_diff;

		cur_time = av_gettime();
		if (!last_time || (cur_time - last_time) >= 30000) {
			aqsize = 0;
			vqsize = 0;
			sqsize = 0;
			if (is->audio_st)
				aqsize = is->audioq.size;
			if (is->video_st)
				vqsize = is->videoq.size;
			if (is->subtitle_st)
				sqsize = is->subtitleq.size;
			av_diff = 0;
			if (is->audio_st && is->video_st)
				av_diff = get_audio_clock(is) - get_video_clock(is);
			printf("%7.2f A-V:%7.3f fd=%4d aq=%5dKB vq=%5dKB sq=%5dB f=%I64d/%I64d   \r",
				get_master_clock(is),
				av_diff,
				is->frame_drops_early + is->frame_drops_late,
				aqsize / 1024,
				vqsize / 1024,
				sqsize,
				(int64_t)0,
				(int64_t)0);
			//���ʱ����Ϣ��ʱ����?----------------
			int pos=static_cast<int>(1000*get_master_clock(is)/(is->ic->duration/1000000.0));
			dlg->m_playprogress.SetPos(pos);
			//��ǰʱ��---
			CString currentclockstr;
			int tns, thh, tmm, tss;
			tns  = static_cast<int>(get_master_clock(is));
			thh  = tns / 3600;
			tmm  = (tns % 3600) / 60;
			tss  = (tns % 60);
			currentclockstr.Format(_T("%02d:%02d:%02d"),thh,tmm,tss);
			dlg->m_currentclock.SetWindowText(currentclockstr);
			//���д�С
			CString vqsizestr,aqsizestr,avdiffstr;
			vqsizestr.Format(_T("%dKB"),vqsize/1024);
			aqsizestr.Format(_T("%dKB"),aqsize/1024);
			avdiffstr.Format(_T("%f"),av_diff);
			dlg->m_vqsize.SetWindowText(vqsizestr);
			dlg->m_aqsize.SetWindowText(aqsizestr);
			dlg->m_avdiff.SetWindowText(avdiffstr);
			//-----------------------------
			fflush(stdout);
			last_time = cur_time;
		}
	}
}

/* allocate a picture (needs to do that in main thread to avoid
potential locking problems */
static void alloc_picture(VideoState *is)
{
	VideoPicture *vp;
	char debugBuf[512];

	vp = &is->pictq[is->pictq_windex];

	if (vp->bmp)
		SDL_DestroyTexture(vp->bmp);

#if CONFIG_AVFILTER
	avfilter_unref_bufferp(&vp->picref);
#endif

	video_open(is, 0);

	sprintf_s(debugBuf, sizeof(debugBuf), "alloc_picture: Creating texture, vp->width=%d, vp->height=%d\n", vp->width, vp->height);
	OutputDebugStringA(debugBuf);
	
	vp->bmp = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, vp->width, vp->height);
	
	if (!vp->bmp) {
		sprintf_s(debugBuf, sizeof(debugBuf), "alloc_picture: SDL_CreateTexture failed! Error: %s\n", SDL_GetError());
		OutputDebugStringA(debugBuf);
		fprintf(stderr, "Error: the video system does not support an image\n"
			"size of %dx%d pixels. Try using -lowres or -vf \"scale=w:h\"\n"
			"to reduce the image size.\n", vp->width, vp->height );
		do_exit(is);
	} else {
		sprintf_s(debugBuf, sizeof(debugBuf), "alloc_picture: SDL_CreateTexture succeeded, vp->bmp=%p\n", vp->bmp);
		OutputDebugStringA(debugBuf);
	}

	SDL_LockMutex(is->pictq_mutex);
	vp->allocated = 1;
	SDL_SignalCondition(is->pictq_cond);
	SDL_UnlockMutex(is->pictq_mutex);
}
//����ɹ���������ʾ��Ҳ�Ƿŵ���һ�������У�?
static int queue_picture(VideoState *is, AVFrame *src_frame, double pts1, int64_t pos)
{
	VideoPicture *vp;
	double frame_delay, pts = pts1;

	/* compute the exact PTS for the picture if it is omitted in the stream
	* pts1 is the dts of the pkt / pts of the frame */
	if (pts != 0) {
		/* update video clock with pts, if present */
		is->video_clock = pts;
	} else {
		pts = is->video_clock;
	}
	/* update video clock for next frame */
	AVCodecContext *codec_ctx = avcodec_alloc_context3(NULL);
	avcodec_parameters_to_context(codec_ctx, is->video_st->codecpar);
	frame_delay = av_q2d(codec_ctx->time_base);
	avcodec_free_context(&codec_ctx);
	/* for MPEG2, the frame can be repeated, so we update the
	clock accordingly */
	frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
	is->video_clock += frame_delay;

#if defined(DEBUG_SYNC) && 0
	printf("frame_type=%c clock=%0.3f pts=%0.3f\n",
		av_get_picture_type_char(src_frame->pict_type), pts, pts1);
#endif

	/* wait until we have space to put a new picture */
	SDL_LockMutex(is->pictq_mutex);

	/* keep the last already displayed picture in the queue */
	while (is->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE - 2 &&
		!is->videoq.abort_request) {
			SDL_WaitCondition(is->pictq_cond, is->pictq_mutex);
	}
	SDL_UnlockMutex(is->pictq_mutex);

	if (is->videoq.abort_request)
		return -1;

	vp = &is->pictq[is->pictq_windex];

#if CONFIG_AVFILTER
	vp->sample_aspect_ratio = ((AVFilterBufferRef *)src_frame->opaque)->video->sample_aspect_ratio;
#else
	vp->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, src_frame);
#endif

	/* alloc or resize hardware picture buffer */
	if (!vp->bmp || vp->reallocate || !vp->allocated ||
		vp->width  != src_frame->width ||
		vp->height != src_frame->height) {
			SDL_Event event;

			vp->allocated  = 0;
			vp->reallocate = 0;
			vp->width = src_frame->width;
			vp->height = src_frame->height;

			/* the allocation must be done in the main thread to avoid
			locking problems. */
			event.type = FF_ALLOC_EVENT;
			event.user.data1 = is;
			SDL_PushEvent(&event);

			/* wait until the picture is allocated */
			SDL_LockMutex(is->pictq_mutex);
			while (!vp->allocated && !is->videoq.abort_request) {
				SDL_WaitCondition(is->pictq_cond, is->pictq_mutex);
			}
			/* if the queue is aborted, we have to pop the pending ALLOC event or wait for the allocation to complete */
			if (is->videoq.abort_request && SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST) != 1) {
				while (!vp->allocated) {
					SDL_WaitCondition(is->pictq_cond, is->pictq_mutex);
				}
			}
			SDL_UnlockMutex(is->pictq_mutex);

			if (is->videoq.abort_request)
				return -1;
	}

	/* if the frame is not skipped, then display it */
	if (vp->bmp) {
		AVFrame *pict = av_frame_alloc();
		uint8_t *pict_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, vp->width, vp->height, 1));
		av_image_fill_arrays(pict->data, pict->linesize, pict_buffer, AV_PIX_FMT_YUV420P, vp->width, vp->height, 1);

#if CONFIG_AVFILTER
		avfilter_unref_bufferp(&vp->picref);
		vp->picref = src_frame->opaque;
#endif

		//���YUV����-----------------
		int y_size=vp->width*vp->height;
		int u_size=y_size/4;
		int v_size=u_size;

#if CONFIG_AVFILTER
		// FIXME use direct rendering
		av_picture_copy((AVPicture *)pict, (AVPicture *)src_frame,
			src_frame->format, vp->width, vp->height);
#else
		//sws_flags = av_get_int(sws_opts, "sws_flags", NULL);
		is->img_convert_ctx = sws_getCachedContext(is->img_convert_ctx,
			vp->width, vp->height, (AVPixelFormat)(src_frame->format), vp->width, vp->height,
			AV_PIX_FMT_YUV420P, sws_flags, NULL, NULL, NULL);
		if (is->img_convert_ctx == NULL) {
			fprintf(stderr, "Cannot initialize the conversion context\n");
			exit(1);
		}
		sws_scale(is->img_convert_ctx, src_frame->data, src_frame->linesize,
			0, vp->height, pict->data, pict->linesize);
#endif

		//����������?----------------
		if(dlg->dataoutput->m_outcheckryuv.GetCheck()==1){
			char *dir=(char *)malloc(MAX_URL_LENGTH);
			GetWindowTextA(dlg->dataoutput->m_outdirryuv.m_hWnd,dir,MAX_URL_LENGTH);

			FILE *fp=fopen(dir,"ab");
			fwrite(pict->data[0],y_size,1,fp);
			fwrite(pict->data[1],u_size,1,fp);
			fwrite(pict->data[2],v_size,1,fp);
			fclose(fp);
			free(dir);
		}else if(dlg->dataoutput->m_outcheckry.GetCheck()==1){
			char *dir=(char *)malloc(MAX_URL_LENGTH);
			GetWindowTextA(dlg->dataoutput->m_outdirry.m_hWnd,dir,MAX_URL_LENGTH);

			FILE *fp=fopen(dir,"ab");
			fwrite(pict->data[0],y_size,1,fp);
			fclose(fp);
			free(dir);
		}else if(dlg->dataoutput->m_outcheckru.GetCheck()==1){
			char *dir=(char *)malloc(MAX_URL_LENGTH);
			if (dir && dlg->dataoutput->m_outdirru.m_hWnd) {
				GetWindowTextA(dlg->dataoutput->m_outdirru.m_hWnd,dir,MAX_URL_LENGTH);

				FILE *fp=fopen(dir,"ab");
				if (fp) {
					fwrite(pict->data[1],u_size,1,fp);
					fclose(fp);
				}
			}
			free(dir);
		}else if(dlg->dataoutput->m_outcheckrv.GetCheck()==1){
			char *dir=(char *)malloc(MAX_URL_LENGTH);
			if (dir && dlg->dataoutput->m_outdirrv.m_hWnd) {
				GetWindowTextA(dlg->dataoutput->m_outdirrv.m_hWnd,dir,MAX_URL_LENGTH);

				FILE *fp=fopen(dir,"ab");
				if (fp) {
					fwrite(pict->data[2],v_size,1,fp);
					fclose(fp);
				}
			}
			free(dir);
		}

		//�����opencv------------------------------------
		dlg->rawanalysis->y_data=(unsigned char *)pict->data[0];
		dlg->rawanalysis->u_data=(unsigned char *)pict->data[1];
		dlg->rawanalysis->v_data=(unsigned char *)pict->data[2];
		dlg->rawanalysis->y_width=vp->width;
		dlg->rawanalysis->y_height=vp->height;
		dlg->rawanalysis->frame_index=vframe_index;

		if(dlg->rawanalysis->m_rawanalysisauto.GetCheck()==1){
			int interframenum=dlg->rawanalysis->m_rawanalysisautointerframenum;
			if(vframe_index%interframenum==0){
				dlg->rawanalysis->PostMessage(WM_COMMAND, MAKEWPARAM(IDC_RAWANALYSIS_OPEN, BN_CLICKED), NULL);
			}
		}

		/* update the texture content */
		SDL_UpdateYUVTexture(vp->bmp, NULL, pict->data[0], pict->linesize[0],
			pict->data[1], pict->linesize[1], pict->data[2], pict->linesize[2]);

		av_freep(&pict_buffer);
		av_frame_free(&pict);

		vp->pts = pts;
		vp->pos = pos;
		vp->skip = 0;

		/* now we can update the picture count */
		if (++is->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE)
			is->pictq_windex = 0;
		SDL_LockMutex(is->pictq_mutex);
		is->pictq_size++;
		SDL_UnlockMutex(is->pictq_mutex);
	}
	return 0;
}
//����һ֡��Ƶ
static int get_video_frame(VideoState *is, AVFrame *frame, int64_t *pts, AVPacket *pkt)
{
	int got_picture = 0, i;
	//��Packet�����л�ȡPacket
	if (packet_queue_get(&is->videoq, pkt, 1) < 0)
		return -1;

	if (pkt->data == flush_pkt.data) {
		SDL_LockMutex(is->pictq_mutex);
		// Make sure there are no long delay timers (ideally we should just flush the que but thats harder)
		for (i = 0; i < VIDEO_PICTURE_QUEUE_SIZE; i++) {
			is->pictq[i].skip = 1;
		}
		while (is->pictq_size && !is->videoq.abort_request) {
			SDL_WaitCondition(is->pictq_cond, is->pictq_mutex);
		}
		is->video_current_pos = -1;
		is->frame_last_pts = AV_NOPTS_VALUE;
		is->frame_last_duration = 0;
		is->frame_timer = (double)av_gettime() / 1000000.0;
		is->frame_last_dropped_pts = AV_NOPTS_VALUE;
		SDL_UnlockMutex(is->pictq_mutex);

		return 0;
	}
	// 使用已经打开的解码器上下�?
	if (avcodec_send_packet(is->video_codec_ctx, pkt) >= 0) {
		if (avcodec_receive_frame(is->video_codec_ctx, frame) >= 0) {
			got_picture = 1;
		}
	}

	if (got_picture) {
		//ע�⣺�˴�����MFC������
		ve_param_vframe(is,frame,pkt);
		//--------------------------
		int ret = 1;

		if (decoder_reorder_pts == -1) {
			*pts = frame->best_effort_timestamp;
		} else if (decoder_reorder_pts) {
			*pts = frame->pts;
		} else {
			*pts = frame->pkt_dts;
		}

		if (*pts == AV_NOPTS_VALUE) {
			*pts = 0;
		}

		if (((is->av_sync_type == AV_SYNC_AUDIO_MASTER && is->audio_st) || is->av_sync_type == AV_SYNC_EXTERNAL_CLOCK) &&
			(framedrop>0 || (framedrop && is->audio_st))) {
				SDL_LockMutex(is->pictq_mutex);
				if (is->frame_last_pts != AV_NOPTS_VALUE && *pts) {
					double clockdiff = get_video_clock(is) - get_master_clock(is);
					double dpts = av_q2d(is->video_st->time_base) * *pts;
					double ptsdiff = dpts - is->frame_last_pts;
					if (fabs(clockdiff) < AV_NOSYNC_THRESHOLD &&
						ptsdiff > 0 && ptsdiff < AV_NOSYNC_THRESHOLD &&
						clockdiff + ptsdiff - is->frame_last_filter_delay < 0) {
							is->frame_last_dropped_pos = pkt->pos;
							is->frame_last_dropped_pts = dpts;
							is->frame_drops_early++;
							ret = 0;
					}
				}
				SDL_UnlockMutex(is->pictq_mutex);
		}

		return ret;
	}
	return 0;
}

#if CONFIG_AVFILTER
static int configure_filtergraph(AVFilterGraph *graph, const char *filtergraph,
	AVFilterContext *source_ctx, AVFilterContext *sink_ctx)
{
	int ret;
	AVFilterInOut *outputs = NULL, *inputs = NULL;

	if (filtergraph) {
		outputs = avfilter_inout_alloc();
		inputs  = avfilter_inout_alloc();
		if (!outputs || !inputs) {
			ret = AVERROR(ENOMEM);
			goto fail;
		}

		outputs->name       = av_strdup("in");
		outputs->filter_ctx = source_ctx;
		outputs->pad_idx    = 0;
		outputs->next       = NULL;

		inputs->name        = av_strdup("out");
		inputs->filter_ctx  = sink_ctx;
		inputs->pad_idx     = 0;
		inputs->next        = NULL;

		if ((ret = avfilter_graph_parse(graph, filtergraph, &inputs, &outputs, NULL)) < 0)
			goto fail;
	} else {
		if ((ret = avfilter_link(source_ctx, 0, sink_ctx, 0)) < 0)
			goto fail;
	}

	return avfilter_graph_config(graph, NULL);
fail:
	avfilter_inout_free(&outputs);
	avfilter_inout_free(&inputs);
	return ret;
}

static int configure_video_filters(AVFilterGraph *graph, VideoState *is, const char *vfilters)
{
	static const enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
	char sws_flags_str[128];
	char buffersrc_args[256];
	int ret;
	AVBufferSinkParams *buffersink_params = av_buffersink_params_alloc();
	AVFilterContext *filt_src = NULL, *filt_out = NULL, *filt_format, *filt_crop;
	AVCodecContext *codec = avcodec_alloc_context3(NULL);
	avcodec_parameters_to_context(codec, is->video_st->codecpar);

	snprintf(sws_flags_str, sizeof(sws_flags_str), "flags=%d", sws_flags);
	graph->scale_sws_opts = av_strdup(sws_flags_str);

	snprintf(buffersrc_args, sizeof(buffersrc_args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		codec->width, codec->height, codec->pix_fmt,
		is->video_st->time_base.num, is->video_st->time_base.den,
		codec->sample_aspect_ratio.num, codec->sample_aspect_ratio.den);

	if ((ret = avfilter_graph_create_filter(&filt_src,
		avfilter_get_by_name("buffer"),
		"ffplay_buffer", buffersrc_args, NULL,
		graph)) < 0)
		return ret;

	buffersink_params->pixel_fmts = pix_fmts;
	ret = avfilter_graph_create_filter(&filt_out,
		avfilter_get_by_name("ffbuffersink"),
		"ffplay_buffersink", NULL, buffersink_params, graph);
	av_freep(&buffersink_params);
	if (ret < 0)
		return ret;

	/* SDL YUV code is not handling odd width/height for some driver
	* combinations, therefore we crop the picture to an even width/height. */
	if ((ret = avfilter_graph_create_filter(&filt_crop,
		avfilter_get_by_name("crop"),
		"ffplay_crop", "floor(in_w/2)*2:floor(in_h/2)*2", NULL, graph)) < 0)
		return ret;
	if ((ret = avfilter_graph_create_filter(&filt_format,
		avfilter_get_by_name("format"),
		"format", "yuv420p", NULL, graph)) < 0)
		return ret;
	if ((ret = avfilter_link(filt_crop, 0, filt_format, 0)) < 0)
		return ret;
	if ((ret = avfilter_link(filt_format, 0, filt_out, 0)) < 0)
		return ret;

	if ((ret = configure_filtergraph(graph, vfilters, filt_src, filt_crop)) < 0)
		return ret;

	is->in_video_filter  = filt_src;
	is->out_video_filter = filt_out;

	return ret;
}

#endif  /* CONFIG_AVFILTER */
//������Ƶ
static int video_thread(void *arg)
{
	AVPacket pkt = { 0 };
	VideoState *is = (VideoState *)arg;
	AVFrame *frame = av_frame_alloc();
	int64_t pts_int = AV_NOPTS_VALUE, pos = -1;
	double pts;
	int ret;

#if CONFIG_AVFILTER
	AVCodecContext *codec = avcodec_alloc_context3(NULL);
	avcodec_parameters_to_context(codec, is->video_st->codecpar);
	AVFilterGraph *graph = avfilter_graph_alloc();
	AVFilterContext *filt_out = NULL, *filt_in = NULL;
	int last_w = 0;
	int last_h = 0;
	enum AVPixelFormat last_format = -2;

	AVCodec *decoder = avcodec_find_decoder(codec->codec_id);
	if (decoder && (decoder->capabilities & AV_CODEC_CAP_DR1)) {
		is->use_dr1 = 1;
		codec->get_buffer     = codec_get_buffer;
		codec->release_buffer = codec_release_buffer;
		codec->opaque         = &is->buffer_pool;
	}
#endif

	for (;;) {
#if CONFIG_AVFILTER
		AVFilterBufferRef *picref;
		AVRational tb;
		AVCodecParameters *video_codecpar = is->video_st->codecpar;
#endif
		while (is->paused && !is->videoq.abort_request)
			SDL_Delay(10);

		av_frame_unref(frame);
		av_packet_unref(&pkt);
		//����һ֡��Ƶ
		ret = get_video_frame(is, frame, &pts_int, &pkt);
		if (ret < 0)
			goto the_end;

		if (!ret)
			continue;

#if CONFIG_AVFILTER
		if (   last_w != video_codecpar->width
			|| last_h != video_codecpar->height
			|| last_format != codec->pix_fmt) {
				av_log(NULL, AV_LOG_INFO, "Frame changed from size:%dx%d to size:%dx%d\n",
					last_w, last_h, video_codecpar->width, video_codecpar->height);
				avfilter_graph_free(&graph);
				graph = avfilter_graph_alloc();
				if ((ret = configure_video_filters(graph, is, vfilters)) < 0) {
					SDL_Event event;
					event.type = FF_QUIT_EVENT;
					event.user.data1 = is;
					SDL_PushEvent(&event);
					av_packet_unref(&pkt);
					goto the_end;
				}
				filt_in  = is->in_video_filter;
				filt_out = is->out_video_filter;
				last_w = video_codecpar->width;
				last_h = video_codecpar->height;
				last_format = codec->pix_fmt;
		}

		frame->pts = pts_int;
		frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);
		if (is->use_dr1 && frame->opaque) {
			FrameBuffer      *buf = frame->opaque;
			AVFilterBufferRef *fb = avfilter_get_video_buffer_ref_from_arrays(
				frame->data, frame->linesize,
				AV_PERM_READ | AV_PERM_PRESERVE,
				frame->width, frame->height,
				frame->format);

			avfilter_copy_frame_props(fb, frame);
			fb->buf->priv           = buf;
			fb->buf->free           = filter_release_buffer;

			buf->refcount++;
			av_buffersrc_add_ref(filt_in, fb, AV_BUFFERSRC_FLAG_NO_COPY);

		} else
			av_buffersrc_write_frame(filt_in, frame);

		av_packet_unref(&pkt);

		while (ret >= 0) {
			is->frame_last_returned_time = av_gettime() / 1000000.0;

			ret = av_buffersink_get_buffer_ref(filt_out, &picref, 0);
			if (ret < 0) {
				ret = 0;
				break;
			}

			is->frame_last_filter_delay = av_gettime() / 1000000.0 - is->frame_last_returned_time;
			if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
				is->frame_last_filter_delay = 0;

			avfilter_copy_buf_props(frame, picref);

			pts_int = picref->pts;
			tb      = filt_out->inputs[0]->time_base;
			pos     = picref->pos;
			frame->opaque = picref;

			if (av_cmp_q(tb, is->video_st->time_base)) {
				av_unused int64_t pts1 = pts_int;
				pts_int = av_rescale_q(pts_int, tb, is->video_st->time_base);
				av_dlog(NULL, "video_thread(): "
					"tb:%d/%d pts:%"PRId64" -> tb:%d/%d pts:%"PRId64"\n",
					tb.num, tb.den, pts1,
					is->video_st->time_base.num, is->video_st->time_base.den, pts_int);
			}
			pts = pts_int * av_q2d(is->video_st->time_base);
			ret = queue_picture(is, frame, pts, pos);
		}
#else
		pts = pts_int * av_q2d(is->video_st->time_base);
		//����ɹ���������ʾ��Ҳ�Ƿŵ���һ�������У�?
		ret = queue_picture(is, frame, pts, pkt.pos);
#endif

		if (ret < 0)
			goto the_end;

		if (is->step)
			stream_toggle_pause(is);
	}
the_end:
{
}
#if CONFIG_AVFILTER
	avfilter_graph_free(&graph);
#endif
	av_packet_unref(&pkt);
	av_frame_free(&frame);
	return 0;
}

static int subtitle_thread(void *arg)
{
	VideoState *is = (VideoState *)arg;
	SubPicture *sp;
	AVPacket pkt1, *pkt = &pkt1;
	int got_subtitle = 0;
	double pts;
	size_t i, j;
	int r, g, b, y, u, v, a;

	for (;;) {
		while (is->paused && !is->subtitleq.abort_request) {
			SDL_Delay(10);
		}
		if (packet_queue_get(&is->subtitleq, pkt, 1) < 0)
			break;

		if (pkt->data == flush_pkt.data) {
			continue;
		}
		SDL_LockMutex(is->subpq_mutex);
		while (is->subpq_size >= SUBPICTURE_QUEUE_SIZE &&
			!is->subtitleq.abort_request) {
				SDL_WaitCondition(is->subpq_cond, is->subpq_mutex);
		}
		SDL_UnlockMutex(is->subpq_mutex);

		if (is->subtitleq.abort_request)
			return 0;

		sp = &is->subpq[is->subpq_windex];

		/* NOTE: ipts is the PTS of the _first_ picture beginning in
		this packet, if any */
		pts = 0;
		if (pkt->pts != AV_NOPTS_VALUE)
			pts = av_q2d(is->subtitle_st->time_base) * pkt->pts;

{
		AVCodecContext *subtitle_codec_ctx = avcodec_alloc_context3(NULL);
		if (subtitle_codec_ctx) {
			avcodec_parameters_to_context(subtitle_codec_ctx, is->subtitle_st->codecpar);
			avcodec_decode_subtitle2(subtitle_codec_ctx, &sp->sub,
				&got_subtitle, pkt);
			avcodec_free_context(&subtitle_codec_ctx);
		}
}
		if (got_subtitle && sp->sub.format == 0) {
			if (sp->sub.pts != AV_NOPTS_VALUE)
				pts = sp->sub.pts / (double)AV_TIME_BASE;
			sp->pts = pts;

			for (i = 0; i < sp->sub.num_rects; i++)
			{
				for (j = 0; j < sp->sub.rects[i]->nb_colors; j++)
				{
					RGBA_IN(r, g, b, a, (uint32_t*)sp->sub.rects[i]->data[1] + j);
					y = ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16;
					u = ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128;
					v = ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128;
					YUVA_OUT((uint32_t*)sp->sub.rects[i]->data[1] + j, y, u, v, a);
				}
			}

			/* now we can update the picture count */
			if (++is->subpq_windex == SUBPICTURE_QUEUE_SIZE)
				is->subpq_windex = 0;
			SDL_LockMutex(is->subpq_mutex);
			is->subpq_size++;
			SDL_UnlockMutex(is->subpq_mutex);
		}
		av_packet_unref(pkt);
	}
	return 0;
}
//������Ƶ��ʱ��SDL����ʾ
/* copy samples for viewing in editor window */
static void update_sample_display(VideoState *is, short *samples, int samples_size)
{
	int size, len;

	size = samples_size / sizeof(short);
	while (size > 0) {
		len = SAMPLE_ARRAY_SIZE - is->sample_array_index;
		if (len > size)
			len = size;
		memcpy(is->sample_array + is->sample_array_index, samples, len * sizeof(short));
		samples += len;
		is->sample_array_index += len;
		if (is->sample_array_index >= SAMPLE_ARRAY_SIZE)
			is->sample_array_index = 0;
		size -= len;
	}
}

/* return the wanted number of samples to get better sync if sync_type is video
* or external master clock */
static int synchronize_audio(VideoState *is, int nb_samples)
{
	int wanted_nb_samples = nb_samples;

	/* if not master, then we try to remove or add samples to correct the clock */
	if (((is->av_sync_type == AV_SYNC_VIDEO_MASTER && is->video_st) ||
		is->av_sync_type == AV_SYNC_EXTERNAL_CLOCK)) {
			double diff, avg_diff;
			int min_nb_samples, max_nb_samples;

			diff = get_audio_clock(is) - get_master_clock(is);

			if (diff < AV_NOSYNC_THRESHOLD) {
				is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
				if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
					/* not enough measures to have a correct estimate */
					is->audio_diff_avg_count++;
				} else {
					/* estimate the A-V difference */
					avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

					if (fabs(avg_diff) >= is->audio_diff_threshold) {
						wanted_nb_samples = nb_samples + (int)(diff * is->audio_src.freq);
						min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
						max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
						wanted_nb_samples = FFMIN(FFMAX(wanted_nb_samples, min_nb_samples), max_nb_samples);
					}
					// av_dlog deprecated, using av_log instead
					av_log(NULL, AV_LOG_DEBUG, "diff=%f adiff=%f sample_diff=%d apts=%0.3f vpts=%0.3f %f\n",
						diff, avg_diff, wanted_nb_samples - nb_samples,
						is->audio_clock, is->video_clock, is->audio_diff_threshold);
				}
			} else {
				/* too big difference : may be initial PTS errors, so
				reset A-V filter */
				is->audio_diff_avg_count = 0;
				is->audio_diff_cum       = 0;
			}
	}

	return wanted_nb_samples;
}

/* decode one audio frame and returns its uncompressed size */
static int audio_decode_frame(VideoState *is, double *pts_ptr)
{
	AVPacket *pkt_temp = &is->audio_pkt_temp;
	AVPacket *pkt = &is->audio_pkt;
	AVCodecContext *dec = avcodec_alloc_context3(NULL);
	avcodec_parameters_to_context(dec, is->audio_st->codecpar);
	int len1, len2, data_size, resampled_data_size;
	int64_t dec_channel_layout;
	int got_frame;
	double pts;
	int new_packet = 0;
	int flush_complete = 0;
	int wanted_nb_samples;

	for (;;) {
		/* NOTE: the audio packet can contain several frames */
		while (pkt_temp->size > 0 || (!pkt_temp->data && new_packet)) {
			if (!is->frame) {
				if (!(is->frame = av_frame_alloc()))
					return AVERROR(ENOMEM);
			} else
				av_frame_unref(is->frame);

			if (is->paused)
				return -1;

			if (flush_complete)
				break;
			new_packet = 0;
			got_frame = 0;
			len1 = avcodec_receive_frame(dec, is->frame);
			if (len1 == AVERROR(EAGAIN)) {
				len1 = avcodec_send_packet(dec, pkt_temp);
				if (len1 < 0 && len1 != AVERROR(EAGAIN)) {
					pkt_temp->size = 0;
					break;
				}
				len1 = avcodec_receive_frame(dec, is->frame);
			}
			if (len1 < 0) {
				pkt_temp->size = 0;
				break;
			}
			got_frame = 1;
			//�����Ƶ����?------------------
			if(dlg->dataoutput->m_outcheckrpcm.GetCheck()==1){
				char *dir=(char *)malloc(MAX_URL_LENGTH);
				GetWindowTextA(dlg->dataoutput->m_outdirrpcm.m_hWnd,dir,MAX_URL_LENGTH);

				FILE *fp=fopen(dir,"ab");
				fwrite(is->frame->data[0],is->frame->linesize[0],1,fp);
				fclose(fp);
				free(dir);
			}
			//ע�⣺�˴�����MFC����---
			ve_param_aframe(is,is->frame,pkt_temp);
			//----------
			//���ȶ��������ᴥ���ϵ㣬��ʱ����
			//ve_score_aframe(is,is->frame,pkt_temp);

			//------------------------
			pkt_temp->data += len1;
			pkt_temp->size -= len1;

			if (!got_frame) {
				/* stop sending empty packets if the decoder is finished */
				if (!pkt_temp->data) {
					const AVCodec *audio_decoder = avcodec_find_decoder(dec->codec_id);
					if (audio_decoder && (audio_decoder->capabilities & AV_CODEC_CAP_DELAY))
						flush_complete = 1;
				}
				continue;
			}
			data_size = av_samples_get_buffer_size(NULL, dec->sample_rate,
				is->frame->nb_samples,
				dec->sample_fmt, 1);

			dec_channel_layout = is->audio_st->codecpar->ch_layout.u.mask;
			wanted_nb_samples = synchronize_audio(is, is->frame->nb_samples);

			if (dec->sample_fmt    != is->audio_src.fmt            ||
				dec_channel_layout != is->audio_src.channel_layout ||
				dec->sample_rate   != is->audio_src.freq           ||
				(wanted_nb_samples != is->frame->nb_samples && !is->swr_ctx)) {
					swr_free(&is->swr_ctx);
					is->swr_ctx = swr_alloc();
					if (is->swr_ctx) {
						av_opt_set_int(is->swr_ctx, "in_channel_layout", dec_channel_layout, 0);
						av_opt_set_int(is->swr_ctx, "out_channel_layout", is->audio_tgt.channel_layout, 0);
						av_opt_set_int(is->swr_ctx, "in_sample_rate", dec->sample_rate, 0);
						av_opt_set_int(is->swr_ctx, "out_sample_rate", is->audio_tgt.freq, 0);
						av_opt_set_sample_fmt(is->swr_ctx, "in_sample_fmt", dec->sample_fmt, 0);
						av_opt_set_sample_fmt(is->swr_ctx, "out_sample_fmt", is->audio_tgt.fmt, 0);
					}
					if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
						fprintf(stderr, "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
							dec->sample_rate,   av_get_sample_fmt_name(dec->sample_fmt),   is->audio_st->codecpar->ch_layout.nb_channels,
							is->audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
						break;
					}
					is->audio_src.channel_layout = dec_channel_layout;
					is->audio_src.channels = is->audio_st->codecpar->ch_layout.nb_channels;
					is->audio_src.freq = dec->sample_rate;
					is->audio_src.fmt = dec->sample_fmt;
			}

			if (is->swr_ctx) {
				const uint8_t **in = (const uint8_t **)is->frame->extended_data;
				uint8_t *out[] = {is->audio_buf2};
				int out_count = sizeof(is->audio_buf2) / is->audio_tgt.channels / av_get_bytes_per_sample(is->audio_tgt.fmt);
				if (wanted_nb_samples != is->frame->nb_samples) {
					if (swr_set_compensation(is->swr_ctx, (wanted_nb_samples - is->frame->nb_samples) * is->audio_tgt.freq / dec->sample_rate,
						wanted_nb_samples * is->audio_tgt.freq / dec->sample_rate) < 0) {
							fprintf(stderr, "swr_set_compensation() failed\n");
							break;
					}
				}
				len2 = swr_convert(is->swr_ctx, out, out_count, in, is->frame->nb_samples);
				if (len2 < 0) {
					fprintf(stderr, "swr_convert() failed\n");
					break;
				}
				if (len2 == out_count) {
					fprintf(stderr, "warning: audio buffer is probably too small\n");
					swr_init(is->swr_ctx);
				}
				is->audio_buf = is->audio_buf2;
				resampled_data_size = len2 * is->audio_tgt.channels * av_get_bytes_per_sample(is->audio_tgt.fmt);
			} else {
				is->audio_buf = is->frame->data[0];
				resampled_data_size = data_size;
			}

			/* if no pts, then compute it */
			pts = is->audio_clock;
			*pts_ptr = pts;
			is->audio_clock += (double)data_size /
				(is->audio_st->codecpar->ch_layout.nb_channels * dec->sample_rate * av_get_bytes_per_sample(dec->sample_fmt));
#ifdef DEBUG
			{
				static double last_clock;
				printf("audio: delay=%0.3f clock=%0.3f pts=%0.3f\n",
					is->audio_clock - last_clock,
					is->audio_clock, pts);
				last_clock = is->audio_clock;
			}
#endif
			return resampled_data_size;
		}

		/* free the current packet */
		if (pkt->data)
			av_packet_unref(pkt);
		memset(pkt_temp, 0, sizeof(*pkt_temp));

		if (is->paused || is->audioq.abort_request) {
			return -1;
		}

		if (is->audioq.nb_packets == 0)
			SDL_SignalCondition(is->continue_read_thread);

		/* read next packet */
		if ((new_packet = packet_queue_get(&is->audioq, pkt, 1)) < 0)
			return -1;

		if (pkt->data == flush_pkt.data) {
			flush_complete = 0;
		}

		*pkt_temp = *pkt;

		/* if update the audio clock with the pts */
		if (pkt->pts != AV_NOPTS_VALUE) {
			is->audio_clock = av_q2d(is->audio_st->time_base)*pkt->pts;
		}
	}
}

/* prepare a new audio buffer */
static void sdl_audio_callback(void *opaque, Uint8 *stream, int len)
{
	if (!opaque || !stream) {
		return;
	}
	
	VideoState *is = (VideoState *)opaque;
	
	// 检�?audio_tgt 是否已初始化
	if (is->audio_tgt.channels <= 0 || is->audio_tgt.freq <= 0) {
		memset(stream, 0, len);
		return;
	}
	
	int audio_size, len1;
	int bytes_per_sec;
	int frame_size = av_samples_get_buffer_size(NULL, is->audio_tgt.channels, 1, is->audio_tgt.fmt, 1);
	double pts;

	audio_callback_time = av_gettime();

	while (len > 0) {
		if (is->audio_buf_index >= is->audio_buf_size) {
			audio_size = audio_decode_frame(is, &pts);
			if (audio_size < 0) {
				/* if error, just output silence */
				is->audio_buf      = is->silence_buf;
				is->audio_buf_size = sizeof(is->silence_buf) / frame_size * frame_size;
			} else {
				if (is->show_mode != SHOW_MODE_VIDEO)
					update_sample_display(is, (int16_t *)is->audio_buf, audio_size);
				is->audio_buf_size = audio_size;
			}
			is->audio_buf_index = 0;
		}
		len1 = is->audio_buf_size - is->audio_buf_index;
		if (len1 > len)
			len1 = len;
		memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
		len -= len1;
		stream += len1;
		is->audio_buf_index += len1;
	}
	bytes_per_sec = is->audio_tgt.freq * is->audio_tgt.channels * av_get_bytes_per_sample(is->audio_tgt.fmt);
	is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
	/* Let's assume the audio driver that is used by SDL has two periods. */
	is->audio_current_pts = is->audio_clock - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / bytes_per_sec;
	is->audio_current_pts_drift = is->audio_current_pts - audio_callback_time / 1000000.0;
}

static void sdl_audio_stream_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
	if (!userdata) {
		return;
	}
	
	VideoState *is = (VideoState *)userdata;
	
	// 检�?audio_tgt 是否已初始化
	if (is->audio_tgt.channels <= 0 || is->audio_tgt.freq <= 0) {
		return;
	}
	
	Uint8 *buffer = NULL;
	int len = total_amount * sizeof(int16_t) * is->audio_tgt.channels;
	
	buffer = (Uint8 *)av_malloc(len);
	if (!buffer) {
		return;
	}
	
	sdl_audio_callback(userdata, buffer, len);
	SDL_PutAudioStreamData(stream, buffer, len);
	av_free(buffer);
}

static int audio_open(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, struct AudioParams *audio_hw_params)
{
	SDL_AudioSpec wanted_spec;
	memset(&wanted_spec, 0, sizeof(wanted_spec));
	const char *env;
	const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
	VideoState *is = (VideoState *)opaque;

	env = SDL_getenv("SDL_AUDIO_CHANNELS");
	if (env) {
		wanted_nb_channels = atoi(env);
	}
	
	wanted_spec.channels = wanted_nb_channels;
	wanted_spec.freq = wanted_sample_rate;
	if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
		fprintf(stderr, "Invalid sample rate or channel count!\n");
		return -1;
	}
	wanted_spec.format = AUDIO_S16SYS;
	
	is->audio_dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &wanted_spec);
	fprintf(stderr, "SDL_OpenAudioDevice returned: %u, wanted_spec: channels=%d, freq=%d\n", 
			(unsigned int)is->audio_dev, wanted_spec.channels, wanted_spec.freq);
	if (!is->audio_dev) {
		fprintf(stderr, "SDL_OpenAudioDevice (%d channels): %s\n", wanted_spec.channels, SDL_GetError());
		wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
		if (!wanted_spec.channels) {
			fprintf(stderr, "No more channel combinations to try, audio open failed\n");
			return -1;
		}
		is->audio_dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &wanted_spec);
		fprintf(stderr, "SDL_OpenAudioDevice (retry) returned: %u\n", (unsigned int)is->audio_dev);
		if (!is->audio_dev) {
			fprintf(stderr, "SDL_OpenAudioDevice (%d channels): %s\n", wanted_spec.channels, SDL_GetError());
			return -1;
		}
	}
	
	// SDL 3.x中，SDL_CreateAudioStream需要两个参数：源和目标音频规格
	// 这里使用相同的规格表示不需要格式转�?
	is->audio_stream_obj = SDL_CreateAudioStream(&wanted_spec, &wanted_spec);
	fprintf(stderr, "SDL_CreateAudioStream returned: %p\n", is->audio_stream_obj);
	if (!is->audio_stream_obj) {
		fprintf(stderr, "SDL_CreateAudioStream: %s\n", SDL_GetError());
		SDL_CloseAudioDevice(is->audio_dev);
		is->audio_dev = 0;
		return -1;
	}
	
	audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
	audio_hw_params->freq = wanted_spec.freq;
	audio_hw_params->channel_layout = wanted_channel_layout;
	audio_hw_params->channels = wanted_spec.channels;

	// 在绑定音频流之前先初始化audio_tgt，防止回调函数中访问未初始化的�?
	is->audio_tgt = *audio_hw_params;
	fprintf(stderr, "audio_tgt initialized: channels=%d, freq=%d, fmt=%d\n", 
			is->audio_tgt.channels, is->audio_tgt.freq, is->audio_tgt.fmt);
	
	SDL_SetAudioStreamGetCallback(is->audio_stream_obj, sdl_audio_stream_callback, opaque);
	fprintf(stderr, "SDL_SetAudioStreamGetCallback called\n");
	
	bool bind_result = SDL_BindAudioStream(is->audio_dev, is->audio_stream_obj);
	fprintf(stderr, "SDL_BindAudioStream returned: %d\n", bind_result);
	
	return SDL_AUDIO_BUFFER_SIZE * wanted_spec.channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
}

/* open a given stream. Return 0 if OK */
//第一个Stream����Ƶ����Ƶ
static int stream_component_open(VideoState *is, int stream_index)
{
	AVFormatContext *ic = is->ic;
	AVCodecContext *avctx;
	const AVCodec *codec;
	AVDictionary *opts;
	AVDictionaryEntry *t = NULL;

	if (stream_index < 0 || stream_index >= (int)ic->nb_streams)
		return -1;
	AVStream *st = ic->streams[stream_index];
	avctx = avcodec_alloc_context3(NULL);
	if (!avctx)
		return -1;
	avcodec_parameters_to_context(avctx, st->codecpar);
	// Ϊ��Ƶ��Ѱ�ҽ�����
	//ע�⣺�˴�����ָ��������
	codec = avcodec_find_decoder(avctx->codec_id);
	opts = filter_codec_opts(codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);

	switch(avctx->codec_type){
	case AVMEDIA_TYPE_AUDIO   : is->last_audio_stream    = stream_index; if(audio_codec_name   ) codec= avcodec_find_decoder_by_name(   audio_codec_name); break;
	case AVMEDIA_TYPE_SUBTITLE: is->last_subtitle_stream = stream_index; if(subtitle_codec_name) codec= avcodec_find_decoder_by_name(subtitle_codec_name); break;
	case AVMEDIA_TYPE_VIDEO   : is->last_video_stream    = stream_index; if(video_codec_name   ) codec= avcodec_find_decoder_by_name(   video_codec_name); break;
	}
	if (!codec)
		return -1;

	avctx->workaround_bugs   = workaround_bugs;
	avctx->lowres            = lowres;
	if(avctx->lowres > codec->max_lowres){
		av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
			codec->max_lowres);
		avctx->lowres= codec->max_lowres;
	}
	avctx->idct_algo         = idct;
	avctx->skip_frame        = skip_frame;
	avctx->skip_idct         = skip_idct;
	avctx->skip_loop_filter  = skip_loop_filter;
	avctx->error_concealment = error_concealment;

	if(avctx->lowres) avctx->flags |= AV_CODEC_FLAG_EMU_EDGE;
	if (fast)   avctx->flags2 |= AV_CODEC_FLAG2_FAST;
	if(codec->capabilities & AV_CODEC_CAP_DR1)
		avctx->flags |= AV_CODEC_FLAG_EMU_EDGE;
	//MetaData?
	if (!av_dict_get(opts, "threads", NULL, 0))
		av_dict_set(&opts, "threads", "auto", 0);
	// �򿪽�����������֮�佨����ϵ
	if (!codec ||
		avcodec_open2(avctx, codec, &opts) < 0)
		return -1;
	if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
		av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
		return AVERROR_OPTION_NOT_FOUND;
	}

	/* prepare audio output */
	//׼��SDL��Ƶ���?
	if (avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
		int audio_hw_buf_size = audio_open(is, st->codecpar->ch_layout.u.mask, st->codecpar->ch_layout.nb_channels, avctx->sample_rate, &is->audio_src);
		if (audio_hw_buf_size < 0)
			return -1;
		is->audio_hw_buf_size = audio_hw_buf_size;
		is->audio_tgt = is->audio_src;
	}

	ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
	switch (avctx->codec_type) {
		//����is�ṹ��
	case AVMEDIA_TYPE_AUDIO:
		is->audio_stream = stream_index;
		is->audio_st = ic->streams[stream_index];
		is->audio_buf_size  = 0;
		is->audio_buf_index = 0;

		/* init averaging filter */
		is->audio_diff_avg_coef  = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
		is->audio_diff_avg_count = 0;
		/* since we do not have a precise anough audio fifo fullness,
		we correct audio sync only if larger than this threshold */
		is->audio_diff_threshold = 2.0 * is->audio_hw_buf_size / av_samples_get_buffer_size(NULL, is->audio_tgt.channels, is->audio_tgt.freq, is->audio_tgt.fmt, 1);

		memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));
		memset(&is->audio_pkt_temp, 0, sizeof(is->audio_pkt_temp));
		//��ʼ��Packet����
		packet_queue_start(&is->audioq);
		//����
		if (is->audio_stream_obj) {
			SDL_ResumeAudioStreamDevice(is->audio_stream_obj);
		}
		break;
	case AVMEDIA_TYPE_VIDEO:
		is->video_stream = stream_index;
		is->video_st = ic->streams[stream_index];
		is->video_codec_ctx = avctx;

		packet_queue_start(&is->videoq);
		//��Ƶ�߳�
		is->video_tid = SDL_CreateThread(video_thread, "video_thread", is);
		break;
	case AVMEDIA_TYPE_SUBTITLE:
		is->subtitle_stream = stream_index;
		is->subtitle_st = ic->streams[stream_index];
		packet_queue_start(&is->subtitleq);

		is->subtitle_tid = SDL_CreateThread(subtitle_thread, "subtitle_thread", is);
		break;
	default:
		break;
	}
	return 0;
}

static void stream_component_close(VideoState *is, int stream_index)
{
	AVFormatContext *ic = is->ic;
	AVCodecContext *avctx;

	if (stream_index < 0 || stream_index >= (int)ic->nb_streams)
		return;
	AVStream *st = ic->streams[stream_index];
	avctx = avcodec_alloc_context3(NULL);
	if (!avctx)
		return;
	avcodec_parameters_to_context(avctx, st->codecpar);

	switch (avctx->codec_type) {
	case AVMEDIA_TYPE_AUDIO:
		packet_queue_abort(&is->audioq);

		if (is->audio_stream_obj) {
			SDL_DestroyAudioStream(is->audio_stream_obj);
			is->audio_stream_obj = NULL;
		}
		if (is->audio_dev) {
			SDL_CloseAudioDevice(is->audio_dev);
			is->audio_dev = 0;
		}

		packet_queue_flush(&is->audioq);
		av_packet_unref(&is->audio_pkt);
		swr_free(&is->swr_ctx);
		av_freep(&is->audio_buf1);
		is->audio_buf = NULL;
		avcodec_free_frame(&is->frame);

		if (is->rdft) {
			av_rdft_end(is->rdft);
			av_freep(&is->rdft_data);
			is->rdft = NULL;
			is->rdft_bits = 0;
		}
		break;
	case AVMEDIA_TYPE_VIDEO:
		packet_queue_abort(&is->videoq);

		/* note: we also signal this mutex to make sure we deblock the
		video thread in all cases */
		SDL_LockMutex(is->pictq_mutex);
		SDL_SignalCondition(is->pictq_cond);
		SDL_UnlockMutex(is->pictq_mutex);

		SDL_WaitThread(is->video_tid, NULL);

		packet_queue_flush(&is->videoq);
		break;
	case AVMEDIA_TYPE_SUBTITLE:
		packet_queue_abort(&is->subtitleq);

		/* note: we also signal this mutex to make sure we deblock the
		video thread in all cases */
		SDL_LockMutex(is->subpq_mutex);
		is->subtitle_stream_changed = 1;

		SDL_SignalCondition(is->subpq_cond);
		SDL_UnlockMutex(is->subpq_mutex);

		SDL_WaitThread(is->subtitle_tid, NULL);

		packet_queue_flush(&is->subtitleq);
		break;
	default:
		break;
	}

	ic->streams[stream_index]->discard = AVDISCARD_ALL;
	switch (avctx->codec_type) {
	case AVMEDIA_TYPE_AUDIO:
		is->audio_st = NULL;
		is->audio_stream = -1;
		break;
	case AVMEDIA_TYPE_VIDEO:
		is->video_st = NULL;
		is->video_stream = -1;
		break;
	case AVMEDIA_TYPE_SUBTITLE:
		is->subtitle_st = NULL;
		is->subtitle_stream = -1;
		break;
	default:
		break;
	}
	avcodec_free_context(&avctx);
#if CONFIG_AVFILTER
	free_buffer_pool(&is->buffer_pool);
#endif
}

static int decode_interrupt_cb(void *ctx)
{
	VideoState *is = (VideoState *)ctx;
	return is->abort_request;
}

static int is_realtime(AVFormatContext *s)
{
	if(   !strcmp(s->iformat->name, "rtp")
		|| !strcmp(s->iformat->name, "rtsp")
		|| !strcmp(s->iformat->name, "sdp")
		)
		return 1;

	if(s->pb && (   av_strstart(s->url, "rtp:", NULL)
		|| av_strstart(s->url, "udp:", NULL)
		)
		)
		return 1;
	return 0;
}

/* this thread gets the stream from the disk or the network */
//�����̣߳��������ƵPacket���������?
static int read_thread(void *arg)
{
	VideoState *is = (VideoState *)arg;
	AVFormatContext *ic = NULL;
	int err, ret;
	size_t i;
	int st_index[AVMEDIA_TYPE_NB];
	AVPacket pkt1, *pkt = &pkt1;
	int eof = 0;
	int pkt_in_play_range = 0;
	AVDictionaryEntry *t;
	AVDictionary **opts;
	int orig_nb_streams;
	SDL_mutex *wait_mutex = SDL_CreateMutex();

	memset(st_index, -1, sizeof(st_index));
	is->last_video_stream = is->video_stream = -1;
	is->last_audio_stream = is->audio_stream = -1;
	is->last_subtitle_stream = is->subtitle_stream = -1;

	ic = avformat_alloc_context();
	ic->interrupt_callback.callback = decode_interrupt_cb;
	ic->interrupt_callback.opaque = is;
	// ����Ƶ�ļ���û�д򿪱������������ʼ��AVFormatContext
	// �°����Ѿ��������?
	//if(av_open_input_file(&pFormatCtx, filename, NULL, 0, NULL)!=0)
	//{
	//	printf("Couldn't open file.\n");
	//	return -1;
	//}
	//��Ϊavformat_open_input()
	//--------------------------
	//if(avformat_open_input(&pFormatCtx,filename,NULL,NULL)!=0){
	//char rtspurl[100]="rtsp://169.254.197.35:8554/sh1943.mpg";
	//Ϊ��ʹFFMPEG����ֱ�Ӵ��ڴ��ж�ȡ����
	//ץ����ʱ��ֱ�Ӵ��ڴ��ж�ȡ����

	char debugBuf[512];
	sprintf_s(debugBuf, "read_thread: opening stream: %s\n", is->filename ? is->filename : "NULL");
	OutputDebugStringA(debugBuf);
	
	err = avformat_open_input(&ic, is->filename, is->iformat, &format_opts);
	
	sprintf_s(debugBuf, "read_thread: avformat_open_input returned: %d\n", err);
	OutputDebugStringA(debugBuf);

	//if(avformat_open_input(&pFormatCtx,"sprink_12M.ts",NULL,NULL)!=0){
	//AfxMessageBox()��ȫ�ֵĺ�������ס��VC����AFX���صĶ���ȫ�ֺ�����
	//��MessageBox()��CWnd�ĳ�Ա������Ҳ����˵���Ķ����Ե���AfxMessageBox������
	//����MessageBox()ֻ���ڻ�����CWnd��������ֱ�ӵ��á� 
	//printf("������?%s\n",ic->iformat->name);
	//printf("IO������?%d\n",ic->pb.buffer_size);
	if (err < 0) {
		sprintf_s(debugBuf, "read_thread: avformat_open_input failed, calling print_error\n");
		OutputDebugStringA(debugBuf);
		print_error(is->filename, err);
		ret = -1;
		goto fail;
	}
	if ((t = av_dict_get(format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
		av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
		ret = AVERROR_OPTION_NOT_FOUND;
		goto fail;
	}
	is->ic = ic;

	if (genpts)
		ic->flags |= AVFMT_FLAG_GENPTS;

	opts = setup_find_stream_info_opts(ic, codec_opts);
	orig_nb_streams = ic->nb_streams;
	sprintf_s(debugBuf, "read_thread: calling avformat_find_stream_info, nb_streams=%d\n", ic->nb_streams);
	OutputDebugStringA(debugBuf);
	
	// ��ȡ����Ϣ������AVFormatContext��
	//This is useful for file formats with no headers such as MPEG. 
	err = avformat_find_stream_info(ic, opts);
	
	sprintf_s(debugBuf, "read_thread: avformat_find_stream_info returned: %d\n", err);
	OutputDebugStringA(debugBuf);
	
	if (err < 0) {
		sprintf_s(debugBuf, "read_thread: avformat_find_stream_info failed: %s\n", is->filename);
		OutputDebugStringA(debugBuf);
		fprintf(stderr, "%s: could not find codec parameters\n", is->filename);
		ret = -1;
		goto fail;
	}
	
	sprintf_s(debugBuf, "read_thread: found %d streams\n", ic->nb_streams);
	OutputDebugStringA(debugBuf);
	for (i = 0; i < orig_nb_streams; i++)
		av_dict_free(&opts[i]);
	av_freep(&opts);

	if (ic->pb)
		ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use url_feof() to test for the end

	if (seek_by_bytes < 0)
		seek_by_bytes = !!(ic->iformat->flags & AVFMT_TS_DISCONT);

	/* if seeking requested, we execute it */
	if (start_time != AV_NOPTS_VALUE) {
		int64_t timestamp;

		timestamp = start_time;
		/* add the stream start time */
		if (ic->start_time != AV_NOPTS_VALUE)
			timestamp += ic->start_time;
		ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
		if (ret < 0) {
			fprintf(stderr, "%s: could not seek to position %0.3f\n",
				is->filename, (double)timestamp / AV_TIME_BASE);
		}
	}
	// ��ȡ�ڸ���Ƶ������Ƶ������
	for (i = 0; i < ic->nb_streams; i++)
		ic->streams[i]->discard = AVDISCARD_ALL;
	if (!video_disable)
		st_index[AVMEDIA_TYPE_VIDEO] =
		av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO,
		wanted_stream[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
	//int video_index = st_index[AVMEDIA_TYPE_VIDEO];
	//printf("��Ƶ���������ƣ�%s\n",ic->streams[video_index]->codec->codec_name);
	//printf("��Ƶ����%d\n",ic->streams[video_index]->codec->width);
	//printf("��Ƶ�ߣ�%d\n",ic->streams[video_index]->codec->height);
	//zhenlv=(ic->streams[video_index]->codec->time_base.den)/(ic->streams[video_index]->codec->time_base.num);
	//printf("��Ƶ֡�ʣ�%f\n",zhenlv);
	if (!audio_disable)
		st_index[AVMEDIA_TYPE_AUDIO] =
		av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
		wanted_stream[AVMEDIA_TYPE_AUDIO],
		st_index[AVMEDIA_TYPE_VIDEO],
		NULL, 0);
	//int audio_index = st_index[AVMEDIA_TYPE_AUDIO];
	//printf("��Ƶ���������ƣ�%s\n",ic->streams[audio_index]->codec->codec_name);
	//printf("�����ʣ�%d\n",ic->streams[audio_index]->codec->sample_rate);
	//printf("֡��С��%d\n",ic->streams[audio_index]->codec->frame_size);
	//printf("��������%d\n",ic->streams[audio_index]->codec->channels);
	if (!video_disable)
		st_index[AVMEDIA_TYPE_SUBTITLE] =
		av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE,
		wanted_stream[AVMEDIA_TYPE_SUBTITLE],
		(st_index[AVMEDIA_TYPE_AUDIO] >= 0 ?
		st_index[AVMEDIA_TYPE_AUDIO] :
	st_index[AVMEDIA_TYPE_VIDEO]),
		NULL, 0);
	if (show_status) {
		av_dump_format(ic, 0, is->filename, 0);
	}

	is->show_mode = (VideoState::ShowMode)show_mode;

	/* open the streams */
	//��Stream����Ƶ����Ƶ
	if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
		//��
		stream_component_open(is, st_index[AVMEDIA_TYPE_AUDIO]);
	}

	ret = -1;
	if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
		sprintf_s(debugBuf, "read_thread: Opening video stream at index %d\n", st_index[AVMEDIA_TYPE_VIDEO]);
		OutputDebugStringA(debugBuf);
		ret = stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO]);
		sprintf_s(debugBuf, "read_thread: stream_component_open returned %d, video_stream=%d\n", ret, is->video_stream);
		OutputDebugStringA(debugBuf);
	} else {
		sprintf_s(debugBuf, "read_thread: No video stream found\n");
		OutputDebugStringA(debugBuf);
	}
	
	sprintf_s(debugBuf, "read_thread: Creating refresh thread...\n");
	OutputDebugStringA(debugBuf);
	is->refresh_tid = SDL_CreateThread(refresh_thread, "refresh_thread", is);
	if (!is->refresh_tid) {
		sprintf_s(debugBuf, "read_thread: Failed to create refresh thread!\n");
		OutputDebugStringA(debugBuf);
	} else {
		sprintf_s(debugBuf, "read_thread: Refresh thread created successfully\n");
		OutputDebugStringA(debugBuf);
	}
	
	sprintf_s(debugBuf, "read_thread: Video stream: %d, Audio stream: %d\n", is->video_stream, is->audio_stream);
	OutputDebugStringA(debugBuf);
	sprintf_s(debugBuf, "read_thread: Show mode: %d\n", is->show_mode);
	OutputDebugStringA(debugBuf);

	if (is->show_mode == SHOW_MODE_NONE)
		is->show_mode = ret >= 0 ? (VideoState::ShowMode)SHOW_MODE_VIDEO : (VideoState::ShowMode)SHOW_MODE_RDFT;

	if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) {
		stream_component_open(is, st_index[AVMEDIA_TYPE_SUBTITLE]);
	}

	if (is->video_stream < 0 && is->audio_stream < 0) {
		AfxMessageBox(_T("could not open codecs"));
		ret = -1;
		goto fail;
	}

	if (infinite_buffer < 0 && is_realtime(ic))
		infinite_buffer = 1;
	//��ȡһЩϵͳ��Ϣ

	//ע�⣺�ڴ˴�����MFC����
	ve_param_global(is);
	
	//--------------------------------
	//��Ҫд��ѭ�������̫�˷�ϵͳ��Դ
	for (;;) {
		if (is->abort_request)
			break;
		if (is->paused != is->last_paused) {
			is->last_paused = is->paused;
			if (is->paused)
				is->read_pause_return = av_read_pause(ic);
			else
				av_read_play(ic);
		}
#if CONFIG_RTSP_DEMUXER || CONFIG_MMSH_PROTOCOL
		if (is->paused &&
			(!strcmp(ic->iformat->name, "rtsp") ||
			(ic->pb && !strncmp(input_filename, "mmsh:", 5)))) {
				/* wait 10 ms to avoid trying to get another packet */
				/* XXX: horrible */
				SDL_Delay(10);
				continue;
		}
#endif
		//���������Ž����Ժ�
		if (is->seek_req) {
			int64_t seek_target = is->seek_pos;
			int64_t seek_min    = is->seek_rel > 0 ? seek_target - is->seek_rel + 2: INT64_MIN;
			int64_t seek_max    = is->seek_rel < 0 ? seek_target - is->seek_rel - 2: INT64_MAX;
			// FIXME the +-2 is due to rounding being not done in the correct direction in generation
			//      of the seek_pos/seek_rel variables

			ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max, is->seek_flags);
			if (ret < 0) {
				fprintf(stderr, "%s: error while seeking\n", is->ic->url);
			} else {
				if (is->audio_stream >= 0) {
					//��յ�ǰ��PAcket����
					packet_queue_flush(&is->audioq);
					packet_queue_put(&is->audioq, &flush_pkt);
				}
				if (is->subtitle_stream >= 0) {
					packet_queue_flush(&is->subtitleq);
					packet_queue_put(&is->subtitleq, &flush_pkt);
				}
				if (is->video_stream >= 0) {
					packet_queue_flush(&is->videoq);
					packet_queue_put(&is->videoq, &flush_pkt);
				}
			}
			is->seek_req = 0;
			eof = 0;
		}
		if (is->que_attachments_req) {
			avformat_queue_attached_pictures(ic);
			is->que_attachments_req = 0;
		}

		/* if the queue are full, no need to read more */
		//���Packet�������ˣ���ȴ�?
		if (infinite_buffer<1 &&
			(is->audioq.size + is->videoq.size + is->subtitleq.size > MAX_QUEUE_SIZE
			|| (   (is->audioq   .nb_packets > MIN_FRAMES || is->audio_stream < 0 || is->audioq.abort_request)
			&& (is->videoq   .nb_packets > MIN_FRAMES || is->video_stream < 0 || is->videoq.abort_request)
			&& (is->subtitleq.nb_packets > MIN_FRAMES || is->subtitle_stream < 0 || is->subtitleq.abort_request)))) {
				/* wait 10 ms */
				SDL_LockMutex(wait_mutex);
				SDL_WaitConditionTimeout(is->continue_read_thread, wait_mutex, 10);
				SDL_UnlockMutex(wait_mutex);
				continue;
		}
		if (eof) {
			if (is->video_stream >= 0) {
				av_packet_unref(pkt);
				pkt->data = NULL;
				pkt->size = 0;
				pkt->stream_index = is->video_stream;
				packet_queue_put(&is->videoq, pkt);
			}
			if (is->audio_stream >= 0 &&
				avcodec_find_decoder(is->audio_st->codecpar->codec_id)->capabilities & AV_CODEC_CAP_DELAY) {
					av_packet_unref(pkt);
					pkt->data = NULL;
					pkt->size = 0;
					pkt->stream_index = is->audio_stream;
					packet_queue_put(&is->audioq, pkt);
			}
			SDL_Delay(10);
			if (is->audioq.size + is->videoq.size + is->subtitleq.size == 0) {
				if (loop != 1 && (!loop || --loop)) {
					stream_seek(is, start_time != AV_NOPTS_VALUE ? start_time : 0, 0, 0);
				} else if (autoexit) {
					ret = AVERROR_EOF;
					dlg->OnClickedStop();
					goto fail;
				}
			}
			eof=0;
			continue;
		}

		//��ȡһ��Packet
		ret = av_read_frame(ic, pkt);
		//printf("Packet dts��%d\n",pkt->dts);
		//printf("Packet pts��%d\n",pkt->pts);
		//printf("Packet Size��%d\n",pkt->size);
		//printf("Packet dts��%d\n",pkt->dts);
		//�˴�����ͼ������

		ve_param_packet(is,pkt);
		//--------------------
		if (ret < 0) {
			if (ret == AVERROR_EOF || (ic->pb && ic->pb->eof_reached))
				eof = 1;
			if (ic->pb && ic->pb->error)
				break;
			SDL_LockMutex(wait_mutex);
			SDL_WaitConditionTimeout(is->continue_read_thread, wait_mutex, 10);
			SDL_UnlockMutex(wait_mutex);
			continue;
		}
		/* check if packet is in play range specified by user, then queue, otherwise discard */
		pkt_in_play_range = duration == AV_NOPTS_VALUE ||
			(pkt->pts - ic->streams[pkt->stream_index]->start_time) *
			av_q2d(ic->streams[pkt->stream_index]->time_base) -
			(double)(start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000
			<= ((double)duration / 1000000);
		if (pkt->stream_index == is->audio_stream && pkt_in_play_range) {
			packet_queue_put(&is->audioq, pkt);
			//printf("��ƵPacket��������Ƶ����\n");
			//�����Ƶѹ������?----------------
			if(dlg->dataoutput->m_outcheckma.GetCheck()==1){
				char *dir=(char *)malloc(MAX_URL_LENGTH);
				//dlg->dataoutput->m_outdirma.GetWindowText(dir,MAX_URL_LENGTH);
				GetWindowTextA(dlg->dataoutput->m_outdirma.m_hWnd,dir,MAX_URL_LENGTH);
				FILE *fp=fopen(dir,"ab");
				fwrite(pkt->data,pkt->size,1,fp);
				fclose(fp);
				free(dir);
			}
			if(dlg->dataoutput->m_outcheckmaaac.GetCheck()==1){
				//AAC�ļ��Ǵ����ݣ������Ҫ���ŵĻ�����Ҫ����?�ֽ�ADTS�ļ�ͷ
				/*char adts_header[7]={0};
				int frame_length =is->ic->streams[is->audio_stream]->codec->frame_size;
				int channels=is->ic->streams[is->audio_stream]->codec->channels;*/
				char *dir=(char *)malloc(MAX_URL_LENGTH);
				//dlg->dataoutput->m_outdirmaaac.GetWindowTextA(dir,MAX_URL_LENGTH);
				GetWindowTextA(dlg->dataoutput->m_outdirmaaac.m_hWnd,dir,MAX_URL_LENGTH);
				FILE *fp=fopen(dir,"ab");
				//ÿһ֡ǰ�����ADTS�ļ�ͷ
				//fwrite(is->ic->streams[is->audio_stream]->codec->extradata,is->ic->streams[is->audio_stream]->codec->extradata_size,1,fp);
				fwrite(pkt->data,pkt->size,1,fp);
				fclose(fp);
				free(dir);

			}
			//---------------------------------
		} else if (pkt->stream_index == is->video_stream && pkt_in_play_range) {
			packet_queue_put(&is->videoq, pkt);
			//printf("��ƵPacket��������Ƶ����\n");
			//�����Ƶѹ������?----------------
			if(dlg->dataoutput->m_outcheckmv.GetCheck()==1){
				char *dir=(char *)malloc(MAX_URL_LENGTH);
				//dlg->dataoutput->m_outdirmv.GetWindowTextA(dir,MAX_URL_LENGTH);
				GetWindowTextA(dlg->dataoutput->m_outdirmv.m_hWnd,dir,MAX_URL_LENGTH);
				FILE *fp=fopen(dir,"ab");
				fwrite(pkt->data,pkt->size,1,fp);
				fclose(fp);
				free(dir);
			}
			//�����������MP4,FLV����H.264��Ƶѹ������------------------
			if(dlg->dataoutput->m_outcheckmvh264.GetCheck()==1){
				char *dir=(char *)malloc(MAX_URL_LENGTH);
				GetWindowTextA(dlg->dataoutput->m_outdirmvh264.m_hWnd,dir,MAX_URL_LENGTH);
				FILE *fp=fopen(dir,"ab");
				if(dataoutput_h264_ft==1){
					fwrite(is->ic->streams[is->video_stream]->codecpar->extradata, is->ic->streams[is->video_stream]->codecpar->extradata_size, 1, fp);
					dataoutput_h264_ft=0;
				}
				//packet�е�������ʼ��û�зָ���(0x00000001), Ҳ����0x65��0x67��0x68��0x41���ֽڣ����Կ��Կ϶��ⲻ�Ǳ�׼��nalu��
				//��ʵ��ǰ4����0x000032ce��ʾ����nalu�ĳ��ȣ��ӵ�5���ֽڿ�ʼ����nalu�����ݡ�����ֱ�ӽ�ǰ4���ֽ��滻Ϊ0x00000001���ɵõ���׼��nalu���ݡ�
				char nal_start[]={0,0,0,1};
				fwrite(nal_start,4,1,fp);
				fwrite(pkt->data+4,pkt->size-4,1,fp);
				fclose(fp);
				free(dir);
			}
			//---------------------------------
		} else if (pkt->stream_index == is->subtitle_stream && pkt_in_play_range) {
			packet_queue_put(&is->subtitleq, pkt);
			//�����Ļѹ������?----------------
			if(dlg->dataoutput->m_outcheckmo.GetCheck()==1){
				char *dir=(char *)malloc(MAX_URL_LENGTH);
				//dlg->dataoutput->m_outdirmo.GetWindowTextA(dir,MAX_URL_LENGTH);
				GetWindowTextA(dlg->dataoutput->m_outdirmo.m_hWnd,dir,MAX_URL_LENGTH);
				FILE *fp=fopen(dir,"ab");
				fwrite(pkt->data,pkt->size,1,fp);
				fclose(fp);
				free(dir);
			}
			//---------------------------------
		} else {
			av_packet_unref(pkt);
		}
	}
	/* wait until the end */
	while (!is->abort_request) {
		SDL_Delay(100);
	}

	ret = 0;
fail:
	/* close each stream */
	if (is->audio_stream >= 0)
		stream_component_close(is, is->audio_stream);
	if (is->video_stream >= 0)
		stream_component_close(is, is->video_stream);
	if (is->subtitle_stream >= 0)
		stream_component_close(is, is->subtitle_stream);
	if (is->ic) {
		avformat_close_input(&is->ic);
	}

	if (ret != 0) {
		SDL_Event event;

		event.type = FF_QUIT_EVENT;
		event.user.data1 = is;
		SDL_PushEvent(&event);
	}
	SDL_DestroyMutex(wait_mutex);
	return 0;
}
//���ø���SDL�źţ���ʼ�����߳�
static VideoState *stream_open(const char *filename, const AVInputFormat *iformat)
{
	VideoState *is;

	is = (VideoState *)av_mallocz(sizeof(VideoState));
	if (!is)
		return NULL;
	av_strlcpy(is->filename, filename, sizeof(is->filename));
	is->iformat = iformat;
	is->ytop    = 0;
	is->xleft   = 0;

	/* start video display */
	//��ʼ�����ֱ���
	is->pictq_mutex = SDL_CreateMutex();
	is->pictq_cond  = SDL_CreateCondition();

	is->subpq_mutex = SDL_CreateMutex();
	is->subpq_cond  = SDL_CreateCondition();
	//��ʼ��Packet����
	packet_queue_init(&is->videoq);
	packet_queue_init(&is->audioq);
	packet_queue_init(&is->subtitleq);

	is->continue_read_thread = SDL_CreateCondition();

	is->av_sync_type = av_sync_type;
	//�����߳�
	is->read_tid     = SDL_CreateThread(read_thread, "read_thread", is);
	if (!is->read_tid) {
		av_free(is);
		return NULL;
	}
	return is;
}
//���¼����������Ǵ���event_loop()�еĸ��ֲ�����
static void stream_cycle_channel(VideoState *is, int codec_type)
{
	AVFormatContext *ic = is->ic;
	int start_index, stream_index;
	int old_index;
	AVStream *st;

	if (codec_type == AVMEDIA_TYPE_VIDEO) {
		start_index = is->last_video_stream;
		old_index = is->video_stream;
	} else if (codec_type == AVMEDIA_TYPE_AUDIO) {
		start_index = is->last_audio_stream;
		old_index = is->audio_stream;
	} else {
		start_index = is->last_subtitle_stream;
		old_index = is->subtitle_stream;
	}
	stream_index = start_index;
	for (;;) {
		if (++stream_index >= (int)is->ic->nb_streams)
		{
			if (codec_type == AVMEDIA_TYPE_SUBTITLE)
			{
				stream_index = -1;
				is->last_subtitle_stream = -1;
				goto the_end;
			}
			if (start_index == -1)
				return;
			stream_index = 0;
		}
		if (stream_index == start_index)
			return;
		st = ic->streams[stream_index];
		if (st->codecpar->codec_type == codec_type) {
			/* check that parameters are OK */
			switch (codec_type) {
			case AVMEDIA_TYPE_AUDIO:
				if (st->codecpar->sample_rate != 0 &&
					st->codecpar->ch_layout.nb_channels != 0)
					goto the_end;
				break;
			case AVMEDIA_TYPE_VIDEO:
			case AVMEDIA_TYPE_SUBTITLE:
				goto the_end;
			default:
				break;
			}
		}
	}
the_end:
	stream_component_close(is, old_index);
	stream_component_open(is, stream_index);
	if (codec_type == AVMEDIA_TYPE_VIDEO)
		is->que_attachments_req = 1;
}


static void toggle_full_screen(VideoState *is)
{
#if defined(__APPLE__) && SDL_VERSION_ATLEAST(1, 2, 14)
	/* OS X needs to reallocate the SDL overlays */
	int i;
	for (i = 0; i < VIDEO_PICTURE_QUEUE_SIZE; i++)
		is->pictq[i].reallocate = 1;
#endif
	is_full_screen = (is_full_screen == 0);
	video_open(is, 1);
}

void ve_play_fullcreen(){
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.key = SDLK_f;
	SDL_PushEvent(&event);
}

static void toggle_pause(VideoState *is)
{
	stream_toggle_pause(is);
	is->step = 0;
}
//��̬����ֻ�������������ļ����пɼ������ܱ������ļ������ã�Ҳ����˵�þ�̬����ֻ
//�����䶨���?cpp��.c�е��ã�������.cpp��.c�ļ��ĺ������ǲ��ܱ����õġ�
//��Ҫ��MFC�е��ã��ͱ�����һ����ͨ�����������з�װ
//�����Ƕԡ���ͣ���ķ�װ
void ve_play_pause(){
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.key = SDLK_p;
	SDL_PushEvent(&event);
}

static void step_to_next_frame(VideoState *is)
{
	/* if the stream is paused unpause it, then step */
	if (is->paused)
		stream_toggle_pause(is);
	is->step = 1;
}

void ve_seek_step(){
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.key = SDLK_s;
	SDL_PushEvent(&event);
}

void ve_aspectratio(int num,int den){
	int w=g_is->width;
	int h=g_is->height;
	int w_re=h*num/den;
	SDL_SetWindowSize(window, w_re, h);
}

void ve_size(int percentage){
	AVCodecParameters *par = g_is->ic->streams[g_is->video_stream]->codecpar;
	int w = par->width;
	int h = par->height;
	SDL_SetWindowSize(window, w * percentage / 100, h * percentage / 100);
}

void ve_audio_display(int mode){

	SDL_Event event;
	event.type = SDL_KEYDOWN;
	switch(mode){
	case 0:event.key.key = SDLK_w;break;
	case 1:event.key.key = SDLK_e;break;
	case 2:event.key.key = SDLK_r;break;
	}
	SDL_PushEvent(&event);
}

void ve_seek(int time){
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	switch (time){
	case -10 :event.key.key = SDLK_LEFT;break;
	case 10 :event.key.key = SDLK_RIGHT;break;
	case -60 :event.key.key = SDLK_DOWN;break;
	case 60 :event.key.key = SDLK_UP;break;
	case -600 :event.key.key = SDLK_PAGEDOWN;break;
	case 600 :event.key.key = SDLK_PAGEUP;break;
	default :event.key.key = SDLK_RIGHT;break;
	}
	SDL_PushEvent(&event);
}

void ve_seek_bar(int pos){
	SDL_Event event;
	event.type = VE_SEEK_BAR_EVENT;
	seek_bar_pos=pos;
	SDL_PushEvent(&event);
}

void ve_stretch(int stretch){
	SDL_Event event;
	event.type = VE_STRETCH_EVENT;
	SDL_PushEvent(&event);
	is_stretch=stretch;
}


static void toggle_audio_display(VideoState *is,int mode)
{
	switch(mode){
	case SHOW_MODE_VIDEO:is->show_mode=(VideoState::ShowMode)SHOW_MODE_VIDEO;break;
	case SHOW_MODE_WAVES:is->show_mode=(VideoState::ShowMode)SHOW_MODE_WAVES;break;
	case SHOW_MODE_RDFT:is->show_mode=(VideoState::ShowMode)SHOW_MODE_RDFT;break;
	default:is->show_mode=(VideoState::ShowMode)SHOW_MODE_VIDEO;break;
	}
	fill_rectangle(renderer,
		is->xleft, is->ytop, is->width, is->height,
		0x00, 0x00, 0x00);
	SDL_RenderPresent(renderer);
}

/* handle an event sent by the GUI */
//������������������,���������¼�
static void event_loop(VideoState *cur_stream)
{
	SDL_Event event;
	double incr, frac;
	int64_t pos;
	char debugBuf[512];

	sprintf_s(debugBuf, sizeof(debugBuf), "event_loop started, cur_stream=%p\n", cur_stream);
	OutputDebugStringA(debugBuf);
	
	for (;;) {
		sprintf_s(debugBuf, sizeof(debugBuf), "event_loop iteration: exit_remark=%d, abort_request=%d\n", exit_remark, cur_stream->abort_request);
		OutputDebugStringA(debugBuf);

		double x;
		//�ж��˳�-------
		if(exit_remark==1) {
			sprintf_s(debugBuf, sizeof(debugBuf), "event_loop exiting due to exit_remark\n");
			OutputDebugStringA(debugBuf);
			break;
		}
		//---------------
		if (cur_stream->abort_request) {
			sprintf_s(debugBuf, sizeof(debugBuf), "event_loop exiting due to abort_request\n");
			OutputDebugStringA(debugBuf);
			break;
		}

		sprintf_s(debugBuf, sizeof(debugBuf), "Waiting for event...\n");
		OutputDebugStringA(debugBuf);
		
		// 使用 SDL_PollEvent 轮询事件，避免阻�?
		int event_handled = 0;
		while (SDL_PollEvent(&event)) {
			event_handled = 1;
			sprintf_s(debugBuf, sizeof(debugBuf), "Got event: type=%d\n", event.type);
			OutputDebugStringA(debugBuf);
			switch (event.type) {
		case SDL_KEYDOWN:
			if (exit_on_keydown) 
			{
				do_exit(cur_stream);
				break;
			}
			switch (event.key.key) {
			case SDLK_ESCAPE:
			case SDLK_q:
				dlg->PostMessage(WM_COMMAND, MAKEWPARAM(IDC_STOP, BN_CLICKED), NULL);
				do_exit(cur_stream);
				break;
			case SDLK_f:
				toggle_full_screen(cur_stream);
				cur_stream->force_refresh = 1;
				break;
			case SDLK_p:
			case SDLK_SPACE:
				toggle_pause(cur_stream);
				break;
			case SDLK_s: // S: Step to next frame
				step_to_next_frame(cur_stream);
				break;
			case SDLK_a:
				stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
				break;
			case SDLK_v:
				stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
				break;
			case SDLK_t:
				stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
				break;
				//�޸���һ�£�������ʾģʽ�ֳ���������
			case SDLK_w:
				toggle_audio_display(cur_stream,SHOW_MODE_VIDEO);
				cur_stream->force_refresh = 1;
				break;
			case SDLK_e:
				toggle_audio_display(cur_stream,SHOW_MODE_WAVES);
				cur_stream->force_refresh = 1;
				break;
			case SDLK_r:
				toggle_audio_display(cur_stream,SHOW_MODE_RDFT);
				cur_stream->force_refresh = 1;
				break;
			case SDLK_y:
				cur_stream->v_show_mode=SHOW_MODE_Y;
				break;
			case SDLK_PAGEUP:
				incr = 600.0;
				goto do_seek;
			case SDLK_PAGEDOWN:
				incr = -600.0;
				goto do_seek;
				//�����?
			case SDLK_LEFT:
				incr = -10.0;
				goto do_seek;
			case SDLK_RIGHT:
				incr = 10.0;
				goto do_seek;
			case SDLK_UP:
				incr = 60.0;
				goto do_seek;
			case SDLK_DOWN:
				incr = -60.0;
do_seek:
				if (seek_by_bytes) {
					if (cur_stream->video_stream >= 0 && cur_stream->video_current_pos >= 0) {
						pos = cur_stream->video_current_pos;
					} else if (cur_stream->audio_stream >= 0 && cur_stream->audio_pkt.pos >= 0) {
						pos = cur_stream->audio_pkt.pos;
					} else
						pos = avio_tell(cur_stream->ic->pb);
					if (cur_stream->ic->bit_rate)
						incr *= cur_stream->ic->bit_rate / 8.0;
					else
						incr *= 180000.0;
					pos += static_cast<int64_t>(incr);
					stream_seek(cur_stream, pos, static_cast<int64_t>(incr), 1);
				} else {
					pos = static_cast<int64_t>(get_master_clock(cur_stream) * AV_TIME_BASE);
					pos += static_cast<int64_t>(incr * AV_TIME_BASE);
					stream_seek(cur_stream, pos, static_cast<int64_t>(incr * AV_TIME_BASE), 0);
				}
				break;
			default:
				break;
			}
			break;
			//��굥��?
		case SDL_MOUSEBUTTONDOWN:
			if (exit_on_mousedown) {
				do_exit(cur_stream);
				break;
			}
		case SDL_MOUSEMOTION:
			if (event.type == SDL_MOUSEBUTTONDOWN) {
				x = event.button.x;
			} else {
				if (!(event.motion.state & SDL_BUTTON_LMASK))
					break;
				x = event.motion.x;
			}
			if (seek_by_bytes || cur_stream->ic->duration <= 0) {
				uint64_t size =  avio_size(cur_stream->ic->pb);
				stream_seek(cur_stream, static_cast<int64_t>(size*x/cur_stream->width), 0, 1);
			} else {
				int64_t ts;
				int ns, hh, mm, ss;
				int tns, thh, tmm, tss;
				tns  = static_cast<int>(cur_stream->ic->duration / 1000000LL);
				thh  = tns / 3600;
				tmm  = (tns % 3600) / 60;
				tss  = (tns % 60);
				frac = x / cur_stream->width;
				ns   = static_cast<int>(frac * tns);
				hh   = ns / 3600;
				mm   = (ns % 3600) / 60;
				ss   = (ns % 60);
				fprintf(stderr, "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac*100,
					hh, mm, ss, thh, tmm, tss);
				ts = static_cast<int64_t>(frac * cur_stream->ic->duration);
				if (cur_stream->ic->start_time != AV_NOPTS_VALUE)
					ts += cur_stream->ic->start_time;
				stream_seek(cur_stream, ts, 0, 0);
			}
			break;
		case SDL_EVENT_WINDOW_RESIZED:{
			SDL_SetWindowSize(window, event.window.data1, event.window.data2);
			screen_width  = cur_stream->width  = event.window.data1;
			screen_height = cur_stream->height = event.window.data2;
			//Refresh----------
			fill_rectangle(renderer, cur_stream->xleft, cur_stream->ytop, cur_stream->width, cur_stream->height, 0x00, 0x00, 0x00);
			SDL_RenderPresent(renderer);
			//-----------------
			cur_stream->force_refresh = 1;
			break;
			}
		case SDL_QUIT:
		case FF_QUIT_EVENT:
			//�൱�ڵ�����ֹͣ��
			dlg->PostMessage(WM_COMMAND, MAKEWPARAM(IDC_STOP, BN_CLICKED), NULL);
			do_exit(cur_stream);
			return; // 立即退出事件循环，避免使用已释放的指针
		case FF_ALLOC_EVENT:
			alloc_picture((VideoState *)(event.user.data1));
			break;
		case FF_REFRESH_EVENT:
			video_refresh(event.user.data1);
			cur_stream->refresh = 0;
			break;
		case VE_SEEK_BAR_EVENT:{
			if (seek_by_bytes || cur_stream->ic->duration <= 0) {
				uint64_t size =  avio_size(cur_stream->ic->pb);
				stream_seek(cur_stream, static_cast<int64_t>(size*seek_bar_pos/1000), 0, 1);
			} else {
				int64_t ts;
				frac=(double)seek_bar_pos/1000;
				ts = static_cast<int64_t>(frac * cur_stream->ic->duration);
				if (cur_stream->ic->start_time != AV_NOPTS_VALUE)
					ts += cur_stream->ic->start_time;
				stream_seek(cur_stream, ts, 0, 0);
			}

			break;
							   }
		case VE_STRETCH_EVENT:{
			//ˢ��--------------------
			fill_rectangle(renderer, cur_stream->xleft, cur_stream->ytop, cur_stream->width, cur_stream->height, 0x00, 0x00, 0x00);
			SDL_RenderPresent(renderer);
			//--
			break;
								}
			default:
				break;
			}
		}
		
		// 如果没有处理事件，短暂休眠以降低CPU占用
		if (!event_handled) {
			SDL_Delay(10);
		}
	}
}

static int opt_frame_size(void *optctx, const char *opt, const char *arg)
{
	av_log(NULL, AV_LOG_WARNING, "Option -s is deprecated, use -video_size.\n");
	return opt_default(NULL, "video_size", arg);
}

static int opt_width(void *optctx, const char *opt, const char *arg)
{
	screen_width = static_cast<int>(parse_number_or_die(opt, arg, OPT_INT64, 1, INT_MAX));
	return 0;
}

static int opt_height(void *optctx, const char *opt, const char *arg)
{
	screen_height = static_cast<int>(parse_number_or_die(opt, arg, OPT_INT64, 1, INT_MAX));
	return 0;
}

static int opt_format(void *optctx, const char *opt, const char *arg)
{
	file_iformat = av_find_input_format(arg);
	if (!file_iformat) {
		fprintf(stderr, "Unknown input format: %s\n", arg);
		return AVERROR(EINVAL);
	}
	return 0;
}

static int opt_frame_pix_fmt(void *optctx, const char *opt, const char *arg)
{
	av_log(NULL, AV_LOG_WARNING, "Option -pix_fmt is deprecated, use -pixel_format.\n");
	return opt_default(NULL, "pixel_format", arg);
}

static int opt_sync(void *optctx, const char *opt, const char *arg)
{
	if (!strcmp(arg, "audio"))
		av_sync_type = AV_SYNC_AUDIO_MASTER;
	else if (!strcmp(arg, "video"))
		av_sync_type = AV_SYNC_VIDEO_MASTER;
	else if (!strcmp(arg, "ext"))
		av_sync_type = AV_SYNC_EXTERNAL_CLOCK;
	else {
		fprintf(stderr, "Unknown value for %s: %s\n", opt, arg);
		exit(1);
	}
	return 0;
}

static int opt_seek(void *optctx, const char *opt, const char *arg)
{
	start_time = parse_time_or_die(opt, arg, 1);
	return 0;
}

static int opt_duration(void *optctx, const char *opt, const char *arg)
{
	duration = parse_time_or_die(opt, arg, 1);
	return 0;
}

static int opt_show_mode(void *optctx, const char *opt, const char *arg)
{
	show_mode = !strcmp(arg, "video") ? SHOW_MODE_VIDEO :
		!strcmp(arg, "waves") ? SHOW_MODE_WAVES :
		!strcmp(arg, "rdft" ) ? SHOW_MODE_RDFT  :
		(ShowMode)(int)parse_number_or_die(opt, arg, OPT_INT, 0, SHOW_MODE_NB-1);
return 0;
}

static void opt_input_file(void *optctx, const char *filename)
{
	//if (input_filename) {
	//    fprintf(stderr, "Argument '%s' provided as input filename, but '%s' was already specified.\n",
	//            filename, input_filename);
	//    exit(1);
	//}
	//if (!strcmp(filename, "-"))
	//    filename = "pipe:";
	//input_filename = filename;
}

static int opt_codec(void *o, const char *opt, const char *arg)
{
	switch(opt[strlen(opt)-1]){
	case 'a' :    audio_codec_name = arg; break;
	case 's' : subtitle_codec_name = arg; break;
	case 'v' :    video_codec_name = arg; break;
	}
	return 0;
}

static int dummy;

static const OptionDef options[] = {
#include "cmdutils_common_opts.h"
	{ "x", HAS_ARG, { (void*) opt_width }, "force displayed width", "width" },
	{ "y", HAS_ARG, { (void*) opt_height }, "force displayed height", "height" },
	{ "s", HAS_ARG | OPT_VIDEO, { (void*) opt_frame_size }, "set frame size (WxH or abbreviation)", "size" },
	{ "fs", OPT_BOOL, { &is_full_screen }, "force full screen" },
	{ "an", OPT_BOOL, { &audio_disable }, "disable audio" },
	{ "vn", OPT_BOOL, { &video_disable }, "disable video" },
	{ "ast", OPT_INT | HAS_ARG | OPT_EXPERT, { &wanted_stream[AVMEDIA_TYPE_AUDIO] }, "select desired audio stream", "stream_number" },
	{ "vst", OPT_INT | HAS_ARG | OPT_EXPERT, { &wanted_stream[AVMEDIA_TYPE_VIDEO] }, "select desired video stream", "stream_number" },
	{ "sst", OPT_INT | HAS_ARG | OPT_EXPERT, { &wanted_stream[AVMEDIA_TYPE_SUBTITLE] }, "select desired subtitle stream", "stream_number" },
	{ "ss", HAS_ARG, { (void*) opt_seek }, "seek to a given position in seconds", "pos" },
	{ "t", HAS_ARG, { (void*) opt_duration }, "play  \"duration\" seconds of audio/video", "duration" },
	{ "bytes", OPT_INT | HAS_ARG, { &seek_by_bytes }, "seek by bytes 0=off 1=on -1=auto", "val" },
	{ "nodisp", OPT_BOOL, { &display_disable }, "disable graphical display" },
	{ "f", HAS_ARG, { (void*) opt_format }, "force format", "fmt" },
	{ "pix_fmt", HAS_ARG | OPT_EXPERT | OPT_VIDEO, { (void*) opt_frame_pix_fmt }, "set pixel format", "format" },
	{ "stats", OPT_BOOL | OPT_EXPERT, { &show_status }, "show status", "" },
	{ "bug", OPT_INT | HAS_ARG | OPT_EXPERT, { &workaround_bugs }, "workaround bugs", "" },
	{ "fast", OPT_BOOL | OPT_EXPERT, { &fast }, "non spec compliant optimizations", "" },
	{ "genpts", OPT_BOOL | OPT_EXPERT, { &genpts }, "generate pts", "" },
	{ "drp", OPT_INT | HAS_ARG | OPT_EXPERT, { &decoder_reorder_pts }, "let decoder reorder pts 0=off 1=on -1=auto", ""},
	{ "lowres", OPT_INT | HAS_ARG | OPT_EXPERT, { &lowres }, "", "" },
	{ "skiploop", OPT_INT | HAS_ARG | OPT_EXPERT, { &skip_loop_filter }, "", "" },
	{ "skipframe", OPT_INT | HAS_ARG | OPT_EXPERT, { &skip_frame }, "", "" },
	{ "skipidct", OPT_INT | HAS_ARG | OPT_EXPERT, { &skip_idct }, "", "" },
	{ "idct", OPT_INT | HAS_ARG | OPT_EXPERT, { &idct }, "set idct algo",  "algo" },
	{ "ec", OPT_INT | HAS_ARG | OPT_EXPERT, { &error_concealment }, "set error concealment options",  "bit_mask" },
	{ "sync", HAS_ARG | OPT_EXPERT, { (void*) opt_sync }, "set audio-video sync. type (type=audio/video/ext)", "type" },
	{ "autoexit", OPT_BOOL | OPT_EXPERT, { &autoexit }, "exit at the end", "" },
	{ "exitonkeydown", OPT_BOOL | OPT_EXPERT, { &exit_on_keydown }, "exit on key down", "" },
	{ "exitonmousedown", OPT_BOOL | OPT_EXPERT, { &exit_on_mousedown }, "exit on mouse down", "" },
	{ "loop", OPT_INT | HAS_ARG | OPT_EXPERT, { &loop }, "set number of times the playback shall be looped", "loop count" },
	{ "framedrop", OPT_BOOL | OPT_EXPERT, { &framedrop }, "drop frames when cpu is too slow", "" },
	{ "infbuf", OPT_BOOL | OPT_EXPERT, { &infinite_buffer }, "don't limit the input buffer size (useful with realtime streams)", "" },
	{ "window_title", OPT_STRING | HAS_ARG, { &window_title }, "set window title", "window title" },
#if CONFIG_AVFILTER
	{ "vf", OPT_STRING | HAS_ARG, { &vfilters }, "video filters", "filter list" },
#endif
	{ "rdftspeed", OPT_INT | HAS_ARG| OPT_AUDIO | OPT_EXPERT, { &rdftspeed }, "rdft speed", "msecs" },
	{ "showmode", HAS_ARG, { (void*) opt_show_mode}, "select show mode (0 = video, 1 = waves, 2 = RDFT)", "mode" },
	{ "default", HAS_ARG | OPT_AUDIO | OPT_VIDEO | OPT_EXPERT, { (void*)opt_default }, "generic catch all option", "" },
	{ "i", OPT_BOOL, { &dummy}, "read specified file", "input_file"},
	{ "codec", HAS_ARG, { (void*) opt_codec}, "force decoder", "decoder" },
	{ NULL, },
};

static void show_usage(void)
{
	av_log(NULL, AV_LOG_INFO, "Simple media player\n");
	av_log(NULL, AV_LOG_INFO, "usage: %s [options] input_file\n", program_name);
	av_log(NULL, AV_LOG_INFO, "\n");
}

void show_help_default(const char *opt, const char *arg)
{
	av_log_set_callback(log_callback_help);
	show_usage();
	show_help_options(options, "Main options:", 0, OPT_EXPERT, 0);
	show_help_options(options, "Advanced options:", OPT_EXPERT, 0, 0);
	printf("\n");
	show_help_children(avcodec_get_class(), AV_OPT_FLAG_DECODING_PARAM);
	show_help_children(avformat_get_class(), AV_OPT_FLAG_DECODING_PARAM);
#if !CONFIG_AVFILTER
	show_help_children(sws_get_class(), AV_OPT_FLAG_ENCODING_PARAM);
#else
	show_help_children(avfilter_get_class(), AV_OPT_FLAG_FILTERING_PARAM);
#endif
	printf("\nWhile playing:\n"
		"q, ESC              quit\n"
		"f                   toggle full screen\n"
		"p, SPC              pause\n"
		"a                   cycle audio channel\n"
		"v                   cycle video channel\n"
		"t                   cycle subtitle channel\n"
		"w                   show audio waves\n"
		"s                   activate frame-step mode\n"
		"left/right          seek backward/forward 10 seconds\n"
		"down/up             seek backward/forward 1 minute\n"
		"page down/page up   seek backward/forward 10 minutes\n"
		"mouse click         seek to percentage in file corresponding to fraction of width\n"
		);
}

static int lockmgr(void **mtx, int op)
{
	return 0;
}

/* Called from the main */

#define __MINGW32__
int ve_play(LPVOID lpParam)
{
	dlg=(CVideoEyeDlg *)lpParam;
	//�ú�������/��ָֹ���Ĵ��ڻ�ؼ��������ͼ��̵�����?
	//�����뱻��ֹʱ�����ڲ���Ӧ���Ͱ��������룬��������ʱ�����ڽ������е����롣
	//�˳���������
	exit_remark=0;
	int flags;
	//���Ľṹ��
	VideoState *is;

	char dummy_videodriver[] = "SDL_VIDEODRIVER=dummy";


	//   av_log_set_flags(AV_LOG_SKIP_REPEATED);
	//    parse_loglevel(argc, argv, options);

	// Codecs and formats are auto-registered in FFmpeg 4.0+
#if CONFIG_AVDEVICE
	avdevice_register_all();
#endif
#if CONFIG_AVFILTER
	avfilter_register_all();
#endif
	//���û�б���������?
	//Warning:Using network protocols without global network initialization.
	//�����������ݵ�ʱ��ֱ��ʹ��rtsp://169.254.197.35:8554/sh1943.mpg��ʽ�Ĳ�������
	//����ʹ�� rtmp://localhost/vod/sample.flv������
	//��Ϊ�˰汾ffmpeg������librtmp
	//���ļ�������
	avformat_network_init();

	//it_opts();

	signal(SIGINT , sigterm_handler); /* Interrupt (ANSI).    */
	signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */
	//input_filename = "rtmp://live.hkstv.hk.lxdns.com/live/hks live=1";
	//�����ļ�·��
	input_filename = (char *)malloc(MAX_URL_LENGTH);
	if (input_filename && dlg->url[0] != '\0') {
		strncpy(input_filename, dlg->url, MAX_URL_LENGTH - 1);
		input_filename[MAX_URL_LENGTH - 1] = '\0';
	}

	//show_banner(argc, argv, options);
	//��ȡ���ŵ���ѡ��
	int opt_argc;
	char **opt_argv;
	opt_argc=dlg->optplayer->generate_opt_argc();
	opt_argv=dlg->optplayer->generate_opt_argv();

	parse_options(NULL, opt_argc, opt_argv, options, opt_input_file);
	
	//dlg->optplayer->free_opt_argv(opt_argv,opt_argc);
	/*
	if (!input_filename) 
	{
	show_usage();
	fprintf(stderr, "An input file must be specified\n");
	fprintf(stderr, "Use -h to get full help or, even better, run 'man %s'\n", program_name);
	exit(1);
	}
	*/
	if (display_disable) 
	{
		video_disable = 1;
	}
	//����Ϊ�Զ��˳�-------------------
	//����ͼƬ��ʱ�򣬻��Զ��˳��������Ȳ�������~
	//autoexit=1;
	//---------------------------------
	// SDL 3.x需要显式初始化事件子系�?
	flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
	// if (audio_disable)
	//	flags &= ~SDL_INIT_AUDIO;


	//------SDL------------------------
	//��ʼ��
	char debugBuf[512];
	sprintf_s(debugBuf, "Initializing SDL with flags: %d\n", flags);
	OutputDebugStringA(debugBuf);
	if (!SDL_Init(flags)) {
		sprintf_s(debugBuf, "SDL_Init failed: %s\n", SDL_GetError());
		OutputDebugStringA(debugBuf);
		AfxMessageBox(_T("SDL_Init failed!"));
		CString errorMsg;
		errorMsg.Format(_T("Could not initialize SDL: %s"), SDL_GetError());
		AfxMessageBox(errorMsg);
		exit(1);
	}
	fprintf(stderr, "SDL initialized successfully\n");

	if (!display_disable) {
		SDL_DisplayID display_id = SDL_GetPrimaryDisplay();
		if (display_id) {
			const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display_id);
			if (mode) {
				fs_screen_width = mode->w;
				fs_screen_height = mode->h;
			}
		}
	}

	if (av_lockmgr_register(lockmgr)) {
		AfxMessageBox(_T("Could not initialize lock manager!"));
		do_exit(NULL);
	}

	av_packet_unref(&flush_pkt);
	flush_pkt.data = (uint8_t *)(intptr_t)"FLUSH";
	//����������
	fprintf(stderr, "Opening stream: %s\n", input_filename ? input_filename : "NULL");
	is = stream_open(input_filename, file_iformat);
	fprintf(stderr, "stream_open returned: %p\n", is);
	if (!is) {
		AfxMessageBox(_T("Failed to initialize VideoState!"));
		do_exit(NULL);
	}

	g_is = is;
	//��ʼ��һЩ����-------------
	autoexit=1;
	dataoutput_h264_ft=1;
	//const char *info=(char *)malloc(500);
	//info=avformat_configuration();

	sprintf_s(debugBuf, "ve_play: Before entering event loop, is=%p, abort_request=%d, exit_remark=%d\n", is, is->abort_request, exit_remark);
	OutputDebugStringA(debugBuf);
	
	//--------------------
	sprintf_s(debugBuf, "ve_play: Entering event loop...\n");
	OutputDebugStringA(debugBuf);
	event_loop(is);
	fprintf(stderr, "Exited event loop\n");

	/* never returns */

	return 0;
}
