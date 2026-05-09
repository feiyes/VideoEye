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



#pragma once



#include "stdafx.h"

#include "resource.h"



//-----------

#define MR(Y,U,V) (Y + (1.403)*(V-128))  

#define MG(Y,U,V) (Y - (0.344) * (U-128) - (0.714) * (V-128) )   

#define MB(Y,U,V) (Y + ((1.773) * (U-128))) 



class CVideoEyeDlg;



// Color component constants

#define SHOW_R_COMP 0

#define SHOW_G_COMP 1

#define SHOW_B_COMP 2

#define SHOW_Y_COMP 3

#define SHOW_U_COMP 4

#define SHOW_V_COMP 5



class Rawanalysis : public CDialogEx

{

	DECLARE_DYNAMIC(Rawanalysis)



public:

	Rawanalysis(CWnd* pParent = NULL);   // standard constructor

	virtual ~Rawanalysis();



// Dialog Data

	enum { IDD = IDD_RAWANALYSIS };



protected:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support



	DECLARE_MESSAGE_MAP()

public:

	afx_msg void OnBnClickedRawanalysisOpen();

	afx_msg void OnBnClickedCancel();

	virtual BOOL OnInitDialog();

	afx_msg void OnBnClickedRawanalysisAuto();

	afx_msg void OnKillfocusRawanalysisAutoInterframenum();

	afx_msg void OnClickedRawanalysisOutpicfolder();



	int m_rawanalysismethod;

	int m_rawanalysisautointerframenum;

	int m_rawanalysiscolorhbin;

	int m_rawanalysiscolorsbin;

	int m_rawanalysiscontourthres;

	int m_rawanalysiscannythres1;

	int m_rawanalysiscannythres2;

	CString m_rawanalysisfacexmlurl;



	CButton m_rawanalysisauto;

	CButton m_rawanalysisoutpicfolder;

	CEdit m_rawanalysisoutpicfolderurl;



	CVideoEyeDlg *maindlg;



	int y_width;

	int y_height;

	unsigned char *y_data;

	unsigned char *u_data;

	unsigned char *v_data;



	// Analysis methods

	int YUVtoIpl1();

	int Color_Histogram();

	int Canny();

	int Contour();

	int DFT();

	int face_detect();

	int show_color_component(int component);

	void cvShiftDFT(cv::Mat& src_arr, cv::Mat& dst_arr);

	void detect_and_draw(cv::Mat& img);

	int YUVtoIpl2();

	void YUV420_C_RGB(char* pY, char* pU, char* pV, unsigned char* pRGB, int height, int width);

	void SystemClear();



	// Additional member variables

	cv::Mat result_image;

	cv::Mat yuvimage;

	cv::CascadeClassifier cascade;

	CCriticalSection critical_section;

	int frame_index;

};

