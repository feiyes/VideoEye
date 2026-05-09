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
#include "VideoEye.h"
#include "Dfanalysispic.h"
#include "afxdialogex.h"


// Dfanalysispic 对话框

IMPLEMENT_DYNAMIC(Dfanalysispic, CDialogEx)

Dfanalysispic::Dfanalysispic(CWnd* pParent /*=NULL*/)
	: CDialogEx(Dfanalysispic::IDD, pParent)
{

}

Dfanalysispic::~Dfanalysispic()
{
}

void Dfanalysispic::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(Dfanalysispic, CDialogEx)

	ON_WM_PAINT()
	ON_WM_MOVE()
END_MESSAGE_MAP()


// Dfanalysispic ��Ϣ��������
BOOL Dfanalysispic::OnInitDialog(){
	CDialogEx::OnInitDialog();
	return TRUE;
}






void Dfanalysispic::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: �ڴ˴�������Ϣ�����������?
	// ��Ϊ��ͼ��Ϣ���� CDialogEx::OnPaint();

	//ÿ���ػ��ʱ�򣬴��ڴ��м���?
	if(dfanalysisdlg->dffinish==TRUE){  
		CRect rect;    
		GetClientRect(&rect);    
		CDC   dcMem;    
		dcMem.CreateCompatibleDC(&dc);    
		CBitmap   bmpBackground;
		bmpBackground.Attach(dfanalysisdlg->hpic);  //�Ի���ı���ͼ�? 

		BITMAP   bitmap;    
		bmpBackground.GetBitmap(&bitmap);    
		CBitmap   *pbmpOld=dcMem.SelectObject(&bmpBackground);    
		dc.StretchBlt(0,0,rect.Width(),rect.Height(),&dcMem,0,0,    
			bitmap.bmWidth,bitmap.bmHeight,SRCCOPY); 
		//һ��ҪDetach()��������군��?
		bmpBackground.Detach();
	}
	
	//if(dfanalysisdlg->dffinish==TRUE)
	//dfanalysisdlg->SaveBmp(dfanalysisdlg->hpic, "test.bmp");
}


void Dfanalysispic::OnMove(int x, int y)
{
	CDialogEx::OnMove(x, y);
}

