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

#pragma once

#include "Dfanalysis.h"
// Dfanalysispic 对话框
class Dfanalysis;

class Dfanalysispic : public CDialogEx
{
	DECLARE_DYNAMIC(Dfanalysispic)

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	Dfanalysispic(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~Dfanalysispic();

	// 对话框数据
	enum { IDD = IDD_DFANALYSIS_PIC };

	virtual BOOL OnInitDialog();

	//父对话框移动时需要刷新，新的绘制函数
	Dfanalysis *dfanalysisdlg;

	afx_msg void OnPaint();
	afx_msg void OnMove(int x, int y);

	
};
