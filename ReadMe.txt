VideoEye

雷霄骅 Lei Xiaohua
leixiaohua1020@126.com
feiiw2014@163.com
中国传媒大学/数字电视技术
Communication University of China / Digital TV Technology
http://blog.csdn.net/leixiaohua1020

VideoEye是一款开源的视频码流分析软件。它可以播放并分析视频比特流。支持多种视频输入方式：HTTP、RTMP、RTSP以及文件等。该软件可以实时分析码流并以图形化的方式显示结果。目前该软件仍在开发阶段。

VideoEye is an open-source stream analysis software. It can play and make analysis of video bit stream. It supports multiple kind of sources, include: HTTP, RTMP, RTSP and files, etc. The software can make real time stream analysis and show the result graphically. Currently the software is under development yet.

===============================================================================
编译环境 (Build Environment)
===============================================================================
- 开发工具: Microsoft Visual Studio 2022 (v145)
- 目标平台: Win32 / x64
- MFC: 静态链接 (Static)
- 字符集: Unicode (Release) / MultiByte (Debug)

===============================================================================
依赖外部库 (External Dependencies)
===============================================================================
- FFmpeg (libavcodec, libavformat, libavutil, libavdevice, libavfilter, swresample, swscale)
- OpenCV 4.12.0 (opencv_world4120d.lib)
- SDL 3 (SDL3.lib)
- TinyXML (tinyxml.lib)
- MediaInfo (MediaInfoDLL)
- Windows SDK Libraries (Winmm.lib, Ws2_32.lib)
