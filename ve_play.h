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


#include "VideoEye.h"
#include "VideoEyeDlg.h"
#include "afxdialogex.h"

//--------------
#include "config.h"
#include <inttypes.h>
#include <cmath>
#include <limits.h>
#include <signal.h>
#include <assert.h>

// FFmpeg headers - wrapped in extern "C" to avoid name mangling
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

#if CONFIG_AVFILTER
# include "libavfilter/avcodec.h"
# include "libavfilter/avfilter.h"
# include "libavfilter/avfiltergraph.h"
# include "libavfilter/buffersink.h"
# include "libavfilter/buffersrc.h"
#endif
}

#include "SDL3/SDL.h"
#include "SDL3/SDL_thread.h"

#include "cmdutils.h"

// Function declarations
void ve_play_pause();
void ve_seek_step();
void ve_play_fullcreen();
void ve_seek(int time);
void ve_aspectratio(int num, int den);
void ve_size(int percentage);
void ve_audio_display(int mode);

//通过SDL的Push Event实现退出功能，不直接调用退出函数
//发送退出消息，在EventLoop中执行
void ve_quit();

//主要的播放函数
int ve_play(LPVOID lpParam);

//--------
int ve_reset_index();

//拖动进度条
void ve_seek_bar(int pos);

void ve_stretch(int stretch);
