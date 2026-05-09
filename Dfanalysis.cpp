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
#include "Dfanalysis.h"
#include "afxdialogex.h"


// Dfanalysis 对话框

IMPLEMENT_DYNAMIC(Dfanalysis, CDialogEx)

	Dfanalysis::Dfanalysis(CWnd* pParent /*=NULL*/)
	: CDialogEx(Dfanalysis::IDD, pParent)
{

	m_dfanalysisnum16x16 = 0;
	m_dfanalysisnum16x8 = 0;
	m_dfanalysisnum8x16 = 0;
	m_dfanalysisnum8x8 = 0;
	m_dfanalysisnumavgq = 0;
	m_dfanalysisnumintra16x16 = 0;
	m_dfanalysisnumintra4x4 = 0;
	m_dfanalysisnumskip = 0;
	m_dfanalysisnummaxq = 0;
	m_dfanalysisnumminq = 0;
	m_dfanalysisnuml0 = 0;
	m_dfanalysisnuml1 = 0;
	m_dfanalysisautointerframenum = 0;
}

Dfanalysis::~Dfanalysis()
{
}

void Dfanalysis::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DFANALYSIS_METHOD, m_dfanalysismethod);
	DDX_Control(pDX, IDC_DFANALYSIS_MB_HEIGHT, m_dfanalysismbheight);
	DDX_Control(pDX, IDC_DFANALYSIS_MB_SUM, m_dfanalysismbsum);
	DDX_Control(pDX, IDC_DFANALYSIS_MB_WIDTH, m_dfanalysismbwidth);
	DDX_Control(pDX, IDC_DFANALYSIS_MV_SUBSAMPLE, m_dfanalysismvsubsample);
	DDX_Control(pDX, IDC_DFANALYSIS_OPTION, m_dfanalysisoption);
	DDX_Text(pDX, IDC_DFANALYSIS_NUM_16X16, m_dfanalysisnum16x16);
	DDX_Text(pDX, IDC_DFANALYSIS_NUM_16X8, m_dfanalysisnum16x8);
	DDX_Text(pDX, IDC_DFANALYSIS_NUM_8X16, m_dfanalysisnum8x16);
	DDX_Text(pDX, IDC_DFANALYSIS_NUM_8X8, m_dfanalysisnum8x8);
	DDX_Text(pDX, IDC_DFANALYSIS_NUM_AVGQ, m_dfanalysisnumavgq);
	DDX_Text(pDX, IDC_DFANALYSIS_NUM_INTRA16X16, m_dfanalysisnumintra16x16);
	DDX_Text(pDX, IDC_DFANALYSIS_NUM_INTRA4X4, m_dfanalysisnumintra4x4);
	DDX_Text(pDX, IDC_DFANALYSIS_NUM_SKIP, m_dfanalysisnumskip);
	DDX_Text(pDX, IDC_DFANALYSIS_NUM_MAXQ, m_dfanalysisnummaxq);
	DDX_Text(pDX, IDC_DFANALYSIS_NUM_MINQ, m_dfanalysisnumminq);
	DDX_Text(pDX, IDC_DFANALYSIS_NUM_L0, m_dfanalysisnuml0);
	DDX_Text(pDX, IDC_DFANALYSIS_NUM_L1, m_dfanalysisnuml1);
	//  DDX_Control(pDX, IDC_DFANALYSIS_MB_PACKETSIZE, m_dfanalysismbpacketsize);
	DDX_Control(pDX, IDC_DFANALYSIS_MB_PICTTYPE, m_dfanalysismbpictype);
	DDX_Control(pDX, IDC_DFANALYSIS_MB_PTIME, m_dfanalysismbptime);
	DDX_Control(pDX, IDC_DFANALYSIS_MB_PTS, m_dfanalysismbpts);
	DDX_Control(pDX, IDC_DFANALYSIS_MB_REF, m_dfanalysismbref);
	DDX_Control(pDX, IDC_DFANALYSIS_MB_FRAMEINDEX, m_dfanalysismbframeindex);
	DDX_Control(pDX, IDC_DFANALYSIS_MB_QTYPE, m_dfanalysismbqtype);
	DDX_Text(pDX, IDC_DFANALYSIS_AUTO_INTERFRAMENUM, m_dfanalysisautointerframenum);
	DDX_Control(pDX, IDC_DFANALYSIS_AUTO, m_dfanalysisauto);
	DDX_Control(pDX, IDC_DFANALYSIS_OUTPICFOLDER, m_dfanalysisoutpicfolder);
	DDX_Control(pDX, IDC_DFANALYSIS_OUTPICFOLDER_URL, m_dfanalysisoutpicfolderurl);
	DDX_Control(pDX, IDC_DFANALYSIS_OUTDATAFOLDER, m_dfanalysisoutdatafolder);
	DDX_Control(pDX, IDC_DFANALYSIS_OUTDATAFOLDER_URL, m_dfanalysisoutdatafolderurl);
}


BEGIN_MESSAGE_MAP(Dfanalysis, CDialogEx)
	ON_BN_CLICKED(IDC_DFANALYSIS_OPEN, &Dfanalysis::OnBnClickedDfanalysisOpen)
	ON_BN_CLICKED(IDC_DFANALYSIS_AUTO, &Dfanalysis::OnBnClickedDfanalysisAuto)
	ON_EN_KILLFOCUS(IDC_DFANALYSIS_AUTO_INTERFRAMENUM, &Dfanalysis::OnKillfocusDfanalysisAutoInterframenum)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDCANCEL, &Dfanalysis::OnBnClickedCancel)
//	ON_WM_CLOSE()
ON_WM_DESTROY()
ON_BN_CLICKED(IDC_DFANALYSIS_OUTPICFOLDER, &Dfanalysis::OnClickedDfanalysisOutpicfolder)
ON_BN_CLICKED(IDC_DFANALYSIS_OUTDATAFOLDER, &Dfanalysis::OnClickedDfanalysisOutdatafolder)
END_MESSAGE_MAP()


// Dfanalysis 消息处理程序
BOOL Dfanalysis::OnInitDialog(){
	CDialogEx::OnInitDialog();
	//默认设置自动分析帧数
	m_dfanalysisautointerframenum=20;
	UpdateData(FALSE);
	GetDlgItem(IDC_DFANALYSIS_AUTO_INTERFRAMENUM)->EnableWindow(FALSE);
	m_dfanalysisauto.SetCheck(0);
	//-----------------
	//加载StringTable
	CString resloader;
	resloader.LoadString(IDS_DFANALYSIS_PARAMETER_QP);
	m_dfanalysismethod.InsertString(0,resloader);
	resloader.LoadString(IDS_DFANALYSIS_PARAMETER_MBTYPE);
	m_dfanalysismethod.InsertString(1,resloader);
	resloader.LoadString(IDS_DFANALYSIS_PARAMETER_MV0);
	m_dfanalysismethod.InsertString(2,resloader);
	resloader.LoadString(IDS_DFANALYSIS_PARAMETER_MV1);
	m_dfanalysismethod.InsertString(3,resloader);
	resloader.LoadString(IDS_DFANALYSIS_PARAMETER_REF0);
	m_dfanalysismethod.InsertString(4,resloader);
	resloader.LoadString(IDS_DFANALYSIS_PARAMETER_REF1);
	m_dfanalysismethod.InsertString(5,resloader);
	m_dfanalysismethod.SetCurSel(0);
	//-----------------
	m_dfanalysisoption.EnableDescriptionArea();
	m_dfanalysisoption.SetVSDotNetLook();
	m_dfanalysisoption.MarkModifiedProperties();
	m_dfanalysisoption.EnableHeaderCtrl(FALSE);
	//设置第一列的宽度-----------------------
	HDITEM item; 
	item.cxy=120; 
	item.mask=HDI_WIDTH; 
	//m_dfanalysisoption.GetHeaderCtrl().SetItem(0, new HDITEM(item)); 
	m_dfanalysisoption.GetHeaderCtrl().SetItem(0, &item); 
	resloader.LoadString(IDS_DFANALYSIS_OPT_GLOBAL);
	prop_global=new CMFCPropertyGridProperty(resloader);
	resloader.LoadString(IDS_DFANALYSIS_OPT_Q);
	prop_q=new CMFCPropertyGridProperty(resloader);
	resloader.LoadString(IDS_DFANALYSIS_OPT_MBTYPE);
	prop_mb_type=new CMFCPropertyGridProperty(resloader);
	resloader.LoadString(IDS_DFANALYSIS_OPT_MV0);
	prop_mv0=new CMFCPropertyGridProperty(resloader);
	resloader.LoadString(IDS_DFANALYSIS_OPT_MV1);
	prop_mv1=new CMFCPropertyGridProperty(resloader);
	resloader.LoadString(IDS_DFANALYSIS_OPT_GLOBAL_MBBORDER);
	prop_global_showmbedge=new CMFCPropertyGridProperty(resloader,(_variant_t) true, _T(""));
	resloader.LoadString(IDS_DFANALYSIS_OPT_GLOBAL_MBBORDER_COLOR);
	prop_global_showmbedge_color=new CMFCPropertyGridColorProperty(resloader,RGB(0, 0, 0), NULL,_T(""));
	//字体
	//LOGFONT lf;
	//CFont* font = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	//font->GetLogFont(&lf);
	//resloader.LoadString(IDS_DFANALYSIS_OPT_GLOBAL_FONT);
	//prop_global_font=new CMFCPropertyGridFontProperty(resloader, lf, CF_EFFECTS | CF_SCREENFONTS, "绘图时的字体");

	resloader.LoadString(IDS_DFANALYSIS_OPT_GLOBAL_WINSIZE);
	prop_global_winsize=new CMFCPropertyGridProperty(resloader,(_variant_t) 120, _T(""));
	prop_global_winsize->EnableSpinControl(TRUE,80,200);

	resloader.LoadString(IDS_DFANALYSIS_OPT_Q_SHOWVAL);
	prop_q_shownum=new CMFCPropertyGridProperty(resloader,(_variant_t) true, _T(""));
	resloader.LoadString(IDS_DFANALYSIS_OPT_Q_SHOWCOLOR);
	prop_q_showcolor=new CMFCPropertyGridProperty(resloader, (_variant_t) false,  _T(""));
	resloader.LoadString(IDS_DFANALYSIS_OPT_MBTYPE_SHOWSUBMB);
	prop_mb_type_showsubmb=new CMFCPropertyGridProperty(resloader,(_variant_t) true, _T(""));
	resloader.LoadString(IDS_DFANALYSIS_OPT_MBTYPE_SHOWCOLOR);
	prop_mb_type_showcolor=new CMFCPropertyGridProperty(resloader,(_variant_t) false, _T(""));
	resloader.LoadString(IDS_DFANALYSIS_OPT_MBTYPE_SHOWSKIP);
	prop_mb_type_showskip=new CMFCPropertyGridProperty(resloader,(_variant_t) true, _T(""));
	resloader.LoadString(IDS_DFANALYSIS_OPT_MBTYPE_SHOWLIST);
	prop_mb_type_showlist=new CMFCPropertyGridProperty(resloader,(_variant_t) false, _T(""));
	resloader.LoadString(IDS_DFANALYSIS_OPT_MV0_COLOR);
	prop_mv0_colortype=new CMFCPropertyGridColorProperty(resloader,RGB(255, 0, 0), NULL, _T(""));
	prop_mv0_colortype->EnableOtherButton(_T("Other..."));
	prop_mv0_colortype->EnableAutomaticButton(_T("Default"), ::GetSysColor(COLOR_3DFACE));
	resloader.LoadString(IDS_DFANALYSIS_OPT_MV0_STYLE);
	prop_mv0_linetype=new CMFCPropertyGridProperty(resloader,(_variant_t) "line", _T(""));
	resloader.LoadString(IDS_DFANALYSIS_OPT_MV1_COLOR);
	prop_mv1_colortype=new CMFCPropertyGridColorProperty(resloader,RGB(0, 0, 255), NULL, _T(""));
	prop_mv1_colortype->EnableOtherButton(_T("Other..."));
	prop_mv1_colortype->EnableAutomaticButton(_T("Default"), ::GetSysColor(COLOR_3DFACE));
	resloader.LoadString(IDS_DFANALYSIS_OPT_MV1_STYLE);
	prop_mv1_linetype=new CMFCPropertyGridProperty(resloader,(_variant_t) "line", _T(""));
	prop_global->AddSubItem(prop_global_showmbedge);
	prop_global->AddSubItem(prop_global_showmbedge_color);
	//prop_global->AddSubItem(prop_global_font);
	prop_global->AddSubItem(prop_global_winsize);
	prop_q->AddSubItem(prop_q_shownum);
	prop_q->AddSubItem(prop_q_showcolor);
	prop_mb_type->AddSubItem(prop_mb_type_showsubmb);
	prop_mb_type->AddSubItem(prop_mb_type_showcolor);
	prop_mb_type->AddSubItem(prop_mb_type_showskip);
	prop_mb_type->AddSubItem(prop_mb_type_showlist);
	prop_mv0->AddSubItem(prop_mv0_colortype);
	prop_mv0->AddSubItem(prop_mv0_linetype);
	prop_mv0->AddOption(_T("line"));
	prop_mv0->AddOption(_T("arrow"));
	prop_mv0->AllowEdit(FALSE);
	prop_mv1->AddSubItem(prop_mv1_colortype);
	prop_mv1->AddSubItem(prop_mv1_linetype);
	prop_mv1->AddOption(_T("line"));
	prop_mv1->AddOption(_T("arrow"));
	prop_mv1->AllowEdit(FALSE);
	m_dfanalysisoption.AddProperty(prop_global);
	m_dfanalysisoption.AddProperty(prop_q);
	m_dfanalysisoption.AddProperty(prop_mb_type);
	m_dfanalysisoption.AddProperty(prop_mv0);
	m_dfanalysisoption.AddProperty(prop_mv1);
	//------------------
	m_dfanalysisoutpicfolderurl.EnableFolderBrowseButton();
	m_dfanalysisoutdatafolderurl.EnableFolderBrowseButton();
	
	TCHAR realpathpic[MAX_URL_LENGTH]={0};   	
	TCHAR realpathdata[MAX_URL_LENGTH]={0};   	
	

	//获取文件目录
	GetCurrentDirectory(MAX_URL_LENGTH,realpathpic);
	GetCurrentDirectory(MAX_URL_LENGTH,realpathdata);

	CString realpathpic1(realpathpic);
	CString realpathdata1(realpathdata);

	realpathpic1.Append(_T("\\dfanalysispic"));
	realpathdata1.Append(_T("\\dfanalysisdata"));


	m_dfanalysisoutpicfolderurl.SetWindowText(realpathpic1);
	m_dfanalysisoutpicfolderurl.EnableWindow(FALSE);

	m_dfanalysisoutdatafolderurl.SetWindowText(realpathdata1);
	m_dfanalysisoutdatafolderurl.EnableWindow(FALSE);

	//------------------
	picdlg=new Dfanalysispic;
	picdlg->Create(IDD_DFANALYSIS_PIC,NULL);
	picdlg->dfanalysisdlg=this;
	//------------------
	dffinish=FALSE;
	//------------------
	return TRUE;
}

void Dfanalysis::GetOption(){
	prop_global_showmbedge_val=prop_global_showmbedge->GetValue().boolVal;
	prop_q_shownum_val=prop_q_shownum->GetValue().boolVal;
	prop_q_showcolor_val=prop_q_showcolor->GetValue().boolVal;
	prop_mb_type_showsubmb_val=prop_mb_type_showsubmb->GetValue().boolVal;
	prop_mb_type_showcolor_val=prop_mb_type_showcolor->GetValue().boolVal;
	prop_mb_type_showskip_val=prop_mb_type_showskip->GetValue().boolVal;
	prop_mb_type_showlist_val=prop_mb_type_showlist->GetValue().boolVal;
	prop_mv0_colortype_val=prop_mv0_colortype->GetColor();
	prop_mv1_colortype_val=prop_mv1_colortype->GetColor();

	prop_global_showmbedge_color_val=prop_global_showmbedge_color->GetColor();
	prop_global_winsize_val=static_cast<float>(prop_global_winsize->GetValue().intVal)/100.0f;
	/*
	LOGFONT lf;
	lf=*prop_global_font->GetLogFont();
	if(prop_global_font_val!=NULL){
		prop_global_font_val=new CFont;
	}
	prop_global_font_val->CreateFontIndirect(&lf);
	*/
	//字符串比较复杂，可以稍后实现
	//prop_mv0_linetype;
	//prop_mv1_linetype;
}

void Dfanalysis::OnBnClickedDfanalysisOpen(){
	try{
		DrawPic();
	}catch(...){
		CString resloader;
		resloader.LoadString(IDS_MSGBOX_ERROR);
		AfxMessageBox(resloader);
		return;
	}
}

void Dfanalysis::DrawPic(){
	dffinish=FALSE;
	//强制刷新窗口
	//Invalidate();
	//如果没有播放，则不分�?
	if(maindlg->is_playing==0){
		CString resloader;
		resloader.LoadString(IDS_MSGBOX_NOPLAYING);
		AfxMessageBox(resloader);
		return ;
	}
	//显示图片---
	picdlg->ShowWindow(TRUE);
		//选项----
	GetOption();
	//设置信息------------------
	CString temp;
	temp.Format(_T("%d"),mb_width);
	m_dfanalysismbwidth.SetWindowText(temp);
	temp.Format(_T("%d"),mb_sum/mb_width);
	m_dfanalysismbheight.SetWindowText(temp);
	temp.Format(_T("%d"),mb_sum);
	m_dfanalysismbsum.SetWindowText(temp);
	//每个像素的运动向�?
	int mv_sample_log2_temp= 4 - motion_subsample_log2;
	temp.Format(_T("%d"),1<<(mv_sample_log2_temp*2));
	m_dfanalysismvsubsample.SetWindowText(temp);
	//当前帧的相关信息---------------------
	temp.Format(_T("%d"),frame_index);
	m_dfanalysismbframeindex.SetWindowText(temp);
	temp.Format(_T("%ld"),pts);
	m_dfanalysismbpts.SetWindowText(temp);
	temp.Format(_T("%.3fs"),ptime);
	m_dfanalysismbptime.SetWindowText(temp);
	switch(pict_type){
	case AV_PICTURE_TYPE_I:
		temp.Format(_T("I"));
		m_dfanalysismbpictype.SetWindowText(temp);
		break;
	case AV_PICTURE_TYPE_B:
		temp.Format(_T("B"));
		m_dfanalysismbpictype.SetWindowText(temp);
		break;
	case AV_PICTURE_TYPE_P:
		temp.Format(_T("P"));
		m_dfanalysismbpictype.SetWindowText(temp);
		break;
	default:
		temp.Format(_T("Other"));
		m_dfanalysismbpictype.SetWindowText(temp);
		break;
	}
	temp.Format(_T("%d"),refs);
	m_dfanalysismbref.SetWindowText(temp);
	switch(qscale_type){
	case FF_QSCALE_TYPE_MPEG1:
		temp.Format(_T("MPEG1"));break;
	case FF_QSCALE_TYPE_MPEG2:
		temp.Format(_T("MPEG2"));break;
	case FF_QSCALE_TYPE_H264:
		temp.Format(_T("H.264"));break;
	case FF_QSCALE_TYPE_VP56:
		temp.Format(_T("VP5,VP6"));break;
	default:
		temp.Format(_T("Unknown"));break;
	}
	m_dfanalysismbqtype.SetWindowText(temp);
	
	CRect rect;
	CWnd *dcd=NULL;
	HWND phWnd=NULL;
	//dcd = picdlg->GetDlgItem(IDD_DFANALYSIS_PIC); // 获取控件句柄
	//这里没有获取到，所以直接强制转�?
	dcd=(CWnd *)picdlg;

	CDC *pDC = dcd->GetDC();  // 获取控件设备
	dcd->Invalidate();
	//马上重绘cdc的OnPaint()
	dcd->UpdateWindow();
	//设置窗口大小，保持宽高比
	picdlg->SetWindowPos(NULL,0,0,(int)(width*prop_global_winsize_val),(int)(height*prop_global_winsize_val),SWP_NOMOVE);
	dcd->GetClientRect(&rect);
	//绘制边框
	DrawFrame();

	pDC->SetWindowExt(rect.Width(), rect.Height());     // 设置窗口范围
	//确保绘图区域在窗口内
	CRgn rgn;
	rgn.CreateRectRgn(rect.left,rect.top,rect.right,rect.bottom);
	pDC->SelectClipRgn(&rgn,RGN_AND);
	//ͳ��һ��--------------------------
	int total_intra4x4=0;
	int total_intra16x16=0;
	int total_16x16=0;
	int total_16x8=0;
	int total_8x16=0;
	int total_8x8=0;
	int total_skip=0;
	int total_l0=0;
	int total_l1=0;
	int avgq=0;
	int totalq=0;
	int maxq=0;
	int minq=100;
	//数据统计----------------------------------
	CString datastat;

	//开始画�?
	//step_w,step_h为目标宽�?
	float step_h=(float)rect.Height()/(float)(mb_sum/mb_stride); 
	//模拟，第一个值为0，所以不�?
	float step_w=(float)rect.Width()/(float)(mb_stride-1); 
	//x,y为目标坐�?
	int i, j;
	float x, y;
	//绘图时用到的变量
	int qval,qvalt,mbtype,refval,refvalt;
	//存储运动向量的x,y�?
	short mv[2];
	CString qval_s,refval_s;
	//刷新-------------------
	pBrush=new CBrush();
	//循环绘制
	for(j=0;j<(mb_sum/mb_stride);j++){
		//for(i=0;i<mb_stride;i++){
		//�?开始，第一个为0，所以不�?
		for(i=0;i<mb_width;i++){
			//创建一个Rect
			CRect rect(static_cast<int>(i*step_w),static_cast<int>(j*step_h),static_cast<int>((i+1)*step_w),static_cast<int>((j+1)*step_h));
			//宏块序号
			int num=j*mb_stride+i;
			//运动向量模式
			//q�?
			switch(m_dfanalysismethod.GetCurSel()){
			case DRAW_Q:{
				qval=qscale_table[num];
				qval_s.Format(_T("%d"),qval);
				//添加到末尾?
				if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
					datastat.AppendFormat(_T("%d,"),qval);
				
				//首先计算一下
				//qp取值范围0-51，颜色取值范围0-255
				if(prop_q_showcolor_val!=FALSE){
					qvalt=qval*255/51;
					//�ᵼ���ڴ�й¶
					//pDC->FillRect(rect,new CBrush(RGB(qvalt,qvalt,qvalt)));
					//�ȴ���һ����Ȼ��ÿ��Createһ�����ʣ���Ҫ�ǵ�DeleteObject
					pBrush->CreateSolidBrush(RGB(qvalt,qvalt,qvalt));
					pDC->FillRect(rect,pBrush);
					pBrush->DeleteObject();
				}
				//����ַ�?
				if(prop_q_shownum_val!=FALSE){
					//����ɫ͸��
					//if(prop_global_font_val!=NULL){
					//	pDC->SelectObject(prop_global_font_val);
					//}
					pDC->SetBkMode(TRANSPARENT);
					pDC->TextOut(rect.left+2,rect.top+2,qval_s);
				}
				//��ָ���ľ����ڻ����ַ�,(û������ʲô����)
				//pDC->DrawText(qval_s,2,rect,DT_CENTER);
				//ͳ����-------
				if(maxq<qval){
					maxq=qval;
				}
				//��Ե������һ��q=0�ĺ��?
				if(minq>qval&&qval!=0){
					minq=qval;
				}
				totalq=totalq+qval;
				//-------
				break;
						}
			case DRAW_MB_TYPE:{
				//��ֹͼƬ����Ƶ��û�д�����Ϣ
				if(mb_type==NULL){
					return;
				}
				CPen pen;
				pen.CreatePen(PS_SOLID,0,prop_global_showmbedge_color_val);
				pDC->SelectObject(pen);

				mbtype=mb_type[num];
				if(mbtype&MB_TYPE_INTRA4x4){
					//添加到末尾?
					if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
						datastat.AppendFormat(_T("intra4x4,"));
					

					//4x4���?
					if(prop_mb_type_showcolor_val!=FALSE){
						pBrush->CreateSolidBrush(RGB(255,0,0));
						pDC->FillRect(rect,pBrush);
						pBrush->DeleteObject();
					}
					if(prop_mb_type_showsubmb_val!=FALSE){
						//��3�����ߺ�����
						
						int m;
						//����
						for(m=1;m<4;m++){
							pDC->MoveTo(rect.left+m*rect.Width()/4,rect.top);
							pDC->LineTo(rect.left+m*rect.Width()/4,rect.bottom);
						}
						//����
						for(m=1;m<4;m++){
							pDC->MoveTo(rect.left,rect.top+m*rect.Height()/4);
							pDC->LineTo(rect.right,rect.top+m*rect.Height()/4);
						}
					}
					//-----
					total_intra4x4++;
					//-----
				}
				if(mbtype&MB_TYPE_INTRA16x16){

					//添加到末尾?
					if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
						datastat.AppendFormat(_T("intra16x16,"));
					


					if(prop_mb_type_showcolor_val!=FALSE){
						pBrush->CreateSolidBrush(RGB(128,0,0));
						pDC->FillRect(rect,pBrush);
						pBrush->DeleteObject();
					}
					//-----
					total_intra16x16++;
					//-----
				}
				if(mbtype&MB_TYPE_INTRA_PCM){
					if(prop_mb_type_showcolor_val!=FALSE){
						pBrush->CreateSolidBrush(RGB(166,0,0));
						pDC->FillRect(rect,pBrush);
						pBrush->DeleteObject();
					}
				}
				if(mbtype&MB_TYPE_16x16){

					//添加到末尾?
					if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
						datastat.AppendFormat(_T("16x16,"));
					

					if(prop_mb_type_showcolor_val!=FALSE){
						pBrush->CreateSolidBrush(RGB(255,99,0));
						pDC->FillRect(rect,pBrush);//�ػ�
						pBrush->DeleteObject();
					}
					//-----
					total_16x16++;
					//-----
				}
				if(mbtype&MB_TYPE_16x8){

					//添加到末尾?
					if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
						datastat.AppendFormat(_T("16x8,"));
					

					if(prop_mb_type_showcolor_val!=FALSE){
						pBrush->CreateSolidBrush(RGB(0,99,0));
						pDC->FillRect(rect,pBrush);
						pBrush->DeleteObject();
					}
					if(prop_mb_type_showsubmb_val!=FALSE){
						//��1������
						int m;
						//����
						for(m=1;m<2;m++){
							pDC->MoveTo(rect.left,rect.top+m*rect.Height()/2);
							pDC->LineTo(rect.right,rect.top+m*rect.Height()/2);
						}
					}
					//-----
					total_16x8++;
					//-----
				}
				if(mbtype&MB_TYPE_8x16){

					//添加到末尾?
					if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
						datastat.AppendFormat(_T("8x16,"));
					


					if(prop_mb_type_showcolor_val!=FALSE){
						pBrush->CreateSolidBrush(RGB(255,99,255));
						pDC->FillRect(rect,pBrush);
						pBrush->DeleteObject();
					}
					if(prop_mb_type_showsubmb_val!=FALSE){
						//��1������
						int m;
						//����
						for(m=1;m<2;m++){
							pDC->MoveTo(rect.left+m*rect.Width()/2,rect.top);
							pDC->LineTo(rect.left+m*rect.Width()/2,rect.bottom);
						}
					}
					//-----
					total_8x16++;
					//-----
				}
				if(mbtype&MB_TYPE_8x8){

					//添加到末尾?
					if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
						datastat.AppendFormat(_T("8x8,"));
					

					if(prop_mb_type_showcolor_val!=FALSE){
						pBrush->CreateSolidBrush(RGB(0,99,255));
						pDC->FillRect(rect,pBrush);
						pBrush->DeleteObject();
					}
					if(prop_mb_type_showsubmb_val!=FALSE){
						//��1�����ߺ�����
						int m;
						//����
						for(m=1;m<2;m++){
							pDC->MoveTo(rect.left+m*rect.Width()/2,rect.top);
							pDC->LineTo(rect.left+m*rect.Width()/2,rect.bottom);
						}
						//����
						for(m=1;m<2;m++){
							pDC->MoveTo(rect.left,rect.top+m*rect.Height()/2);
							pDC->LineTo(rect.right,rect.top+m*rect.Height()/2);
						}
					}
					//-----
					total_8x8++;
					//-----
				}

				if(mbtype&MB_TYPE_SKIP){
					//�����������?
					if(prop_mb_type_showskip_val!=FALSE){
						//����ɫ͸��
						pDC->SetBkMode(TRANSPARENT);
						//����ַ�?
						pDC->TextOut(rect.left+2,rect.top+2,_T("s"));
					}
					//-----
					total_skip++;
					//-----
				}
				//�Ƿ�ʹ��list0����list1����Ԥ��
				//��б��
				if(mbtype&MB_TYPE_L0&&(prop_mb_type_showlist_val!=FALSE)){
					pDC->SetBkMode(TRANSPARENT);
					pDC->MoveTo(rect.left,rect.top);
					pDC->LineTo(rect.right,rect.bottom);
					total_l0++;
				}
				if(mbtype&MB_TYPE_L1&&(prop_mb_type_showlist_val!=FALSE)){
					pDC->SetBkMode(TRANSPARENT);
					pDC->MoveTo(rect.left,rect.bottom);
					pDC->LineTo(rect.right,rect.top);
					total_l1++;
				}

				/*
				//ûŪ��~
				
				
				//�����������?
				if(mbtype&MB_TYPE_INTERLACED){
				pDC->FillRect(rect,new CBrush(RGB(0,255,0)));}
				if(mbtype&MB_TYPE_DIRECT2){pDC->FillRect(rect,new CBrush(RGB(0,200,0)));
				}
				if(mbtype&MB_TYPE_ACPRED){
				pDC->FillRect(rect,new CBrush(RGB(0,166,0)));}
				if(mbtype&MB_TYPE_GMC){pDC->FillRect(rect,new CBrush(RGB(0,133,0)));
				}
				if(mbtype&MB_TYPE_QUANT){
				pDC->FillRect(rect,new CBrush(RGB(0,0,66)));
				}
				if(mbtype&MB_TYPE_CBP){
				pDC->FillRect(rect,new CBrush(RGB(0,0,33)));
				}
				*/

				break;
							  }
			case DRAW_MV0:{
				//��ֹͼƬ����Ƶ��MPEG2��û�д�����Ϣ
				if(motion_val0==NULL){
					CString resloader;
					resloader.LoadString(IDS_MSGBOX_NODATA);
					AfxMessageBox(resloader);
					return;
				}
				//ע��:FFMPEG���˶�ʸ���ǵ�����ŵģ����鲢û��ֱ�ӵĹ��?
				//�����ڴ˴���������Ҫͨ����鵥Ԫ�����?
				//����8x8���ֵ��˶�ʸ������Ĺ�ϵ��?
				//-------------------------
				//|			 |			  |
				//|mv[x]	 |mv[x+1]	  |
				//-------------------------
				//|			 |			  |
				//|mv[x+line]|mv[x+line+1]|
				//-------------------------
				//���Ҫ�Ѳ�������? =
				//�˶������Ӽ�ͷ-------------
				//ARROWSTRUCT a;
				//a.bFill=FALSE;
				//a.nWidth=5;
				//a.fTheta=60;
				//-------------
				int mv_sample_log2= 4 - motion_subsample_log2;
				//һ��MV�ĸ���
				//FIX:Ŀǰ����H.264��mv_stride��MPEG4��RMVB��һ��
				//ֻ��������
				int mv_stride=0;
				if(codec_id==AV_CODEC_ID_H264){
					mv_stride=(mb_width << mv_sample_log2);
				}else{
					mv_stride=(mb_width << mv_sample_log2)+1;
				}
				//һ�ź�������MV����
				int mv_mb_stride= mv_stride<<mv_sample_log2;
				//һ�����һ�л�һ��MV����
				int mv_sample=1<<mv_sample_log2;
				//��ǰ����һ��MV����ֵ
				int mv_num=j*mv_mb_stride+i*mv_sample;
				//���˶�ʸ��
				CPen pen;
				//������ɫ
				//pen.CreatePen(PS_SOLID,0,RGB(255,0,0));//��Ϊ��ɫ
				pen.CreatePen(PS_SOLID,0,prop_mv0_colortype_val);
				pDC->SelectObject(pen);
				int m,n;
				//�������[���߽�]
				if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
					datastat.AppendFormat(_T("[,"));
				
				//���к��л���m�����У�n������
				for(m=0;m<mv_sample;m++){
					for(n=0;n<mv_sample;n++){
						mv[0]=motion_val0[mv_num+n+m*mv_stride][0];
						mv[1]=motion_val0[mv_num+n+m*mv_stride][1];
						pDC->MoveTo(rect.left+n*rect.Width()/mv_sample,rect.top+m*rect.Height()/mv_sample);
						pDC->LineTo(rect.left+n*rect.Width()/mv_sample+mv[0],rect.top+m*rect.Height()/mv_sample+mv[1]);
						//���������Ǽ�ͷ������ֱ��
						//ArrowTo1(pDC->m_hDC,rect.left+n*rect.Width()/mv_sample+mv[0],rect.top+m*rect.Height()/mv_sample+mv[1],&a);
						//添加到末尾?
						if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
							datastat.AppendFormat(_T("(%d-%d),"),mv[0],mv[1]);
						
					}
				}
				//�������[���߽�]
				if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
					datastat.AppendFormat(_T("],"));
				
				break;
						  }
			case DRAW_MV1:{
				//��ֹͼƬ����Ƶ��MPEG2��û�д�����Ϣ
				if(motion_val1==NULL){
					CString resloader;
					resloader.LoadString(IDS_MSGBOX_NODATA);
					AfxMessageBox(resloader);
					return;
				}
				//���Ҫ�Ѳ�������? =
				int mv_sample_log2= 4 - motion_subsample_log2;
				//һ��MV�ĸ���
				int mv_stride= (mb_width << mv_sample_log2);
				//һ�ź�������MV����
				int mv_mb_stride= mv_stride<<mv_sample_log2;
				//һ�����һ�л�һ��MV����
				int mv_sample=1<<mv_sample_log2;
				//��ǰ����һ��MV����ֵ
				int mv_num=j*mv_mb_stride+i*mv_sample;
				//���˶�ʸ��
				CPen pen;
				//pen.CreatePen(PS_SOLID,0,RGB(0,0,255));//��Ϊ��ɫ
				pen.CreatePen(PS_SOLID,0,prop_mv1_colortype_val);
				pDC->SelectObject(pen);
				int m,n;

				//�������[���߽�]
				if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
					datastat.AppendFormat(_T("[,"));
				

				//���к��л���m�����У�n������
				for(m=0;m<mv_sample;m++){
					for(n=0;n<mv_sample;n++){
						mv[0]=motion_val1[mv_num+n+m*mv_stride][0];
						mv[1]=motion_val1[mv_num+n+m*mv_stride][1];
						pDC->MoveTo(rect.left+n*rect.Width()/mv_sample,rect.top+m*rect.Height()/mv_sample);
						pDC->LineTo(rect.left+n*rect.Width()/mv_sample+mv[0],rect.top+m*rect.Height()/mv_sample+mv[1]);

						//添加到末尾?
						if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
							datastat.AppendFormat(_T("(%d-%d),"),mv[0],mv[1]);
						
					}
				}

				//�������[���߽�]
				if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
					datastat.AppendFormat(_T("],"));
				

				break;
						  }
			case DRAW_REFINDEX0:{
				//��ֹͼƬ����Ƶ��MPEG2��û�д�����Ϣ
				if(ref_index0==NULL){
					CString resloader;
					resloader.LoadString(IDS_MSGBOX_NODATA);
					AfxMessageBox(resloader);
					return;
				}
				//ref_index����Դ���жϣ�ÿ�������?��
				//�˴�ֻ��ȡ��һ��
				//�����ȡ�ĸ��Ļ�����Ļ����ʾ���µ�? =
				refval=ref_index0[num*4];
				refval_s.Format(_T("%d"),refval);

				//添加到末尾?
				if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
					datastat.AppendFormat(_T("%d,"),refval);

				//�ȱ�����һ��
				//����ο�֡���ֵ��16֡
				//���죬ƽ��
				//refvalt=refval*255/16;
				//��ȡ�ο�֡�����ֵrefs
				refvalt=refval*128/refs;
				refvalt=refvalt+128;
				pBrush->CreateSolidBrush(RGB(refvalt,refvalt,refvalt));
				pDC->FillRect(rect,pBrush);
				pBrush->DeleteObject();
				//��ʾ
				pDC->SetBkMode(TRANSPARENT);
				pDC->TextOut(rect.left+2,rect.top+2,refval_s);
				break;
						  }
			case DRAW_REFINDEX1:{
				//��ֹͼƬ����Ƶ��MPEG2��û�д�����Ϣ
				if(ref_index1==NULL){
					CString resloader;
					resloader.LoadString(IDS_MSGBOX_NODATA);
					AfxMessageBox(resloader);
					return;
				}
				//ref_index����Դ���жϣ�ÿ�������?��
				//�˴�ֻ��ȡ��һ��
				//�����ȡ�ĸ��Ļ�����Ļ����ʾ���µ�? =
				refval=ref_index1[num*4];
				refval_s.Format(_T("%d"),refval);

				//添加到末尾?
				if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
					datastat.AppendFormat(_T("%d,"),refval);

				//�ȱ�����һ��
				//����ο�֡���ֵ��16֡
				//���죬ƽ��
				//refvalt=refval*255/16;
				//��ȡ�ο�֡�����ֵrefs
				refvalt=refval*128/refs;
				refvalt=refvalt+128;
				pBrush->CreateSolidBrush(RGB(refvalt,refvalt,refvalt));
				pDC->FillRect(rect,pBrush);
				pBrush->DeleteObject();
				//��ʾ
				pDC->SetBkMode(TRANSPARENT);
				pDC->TextOut(rect.left+2,rect.top+2,refval_s);
				break;
								}
			}
		}
		//���������ͳ����һ�к��
		if(m_dfanalysisoutdatafolder.GetCheck()==TRUE)
			datastat.AppendFormat(_T("\n"));
		
	}
	//���ͳ����?---------------
	if(m_dfanalysismethod.GetCurSel()==DRAW_MB_TYPE){
		m_dfanalysisnumintra4x4=total_intra4x4;
		m_dfanalysisnumintra16x16=total_intra16x16;
		m_dfanalysisnum16x16=total_16x16;
		m_dfanalysisnum16x8=total_16x8;
		m_dfanalysisnum8x16=total_8x16;
		m_dfanalysisnum8x8=total_8x8;
		m_dfanalysisnumskip=total_skip;
		m_dfanalysisnuml0=total_l0;
		m_dfanalysisnuml1=total_l1;
		//�����߳��е��ã���֧��UpdateData
		if(m_dfanalysisauto.GetCheck()!=1)
			UpdateData(FALSE);
	}
	//---------------------------
	if(m_dfanalysismethod.GetCurSel()==DRAW_Q){
		m_dfanalysisnumavgq=totalq/((mb_sum/mb_stride)*mb_width);
		m_dfanalysisnummaxq=maxq;
		m_dfanalysisnumminq=minq;
		//�����߳��е��ã���֧��UpdateData
		if(m_dfanalysisauto.GetCheck()!=1)
		UpdateData(FALSE);
	}
	//----------------
	//�Ѻ�黭�������?
	if(prop_global_showmbedge_val!=FALSE){
		CPen pen_default;
		//pen_default.CreatePen(PS_SOLID,0,RGB(0,0,0));//��Ϊ��ɫ
		pen_default.CreatePen(PS_SOLID,0,prop_global_showmbedge_color_val);
		pDC->SelectObject(pen_default);
		//������
		x=0;
		y=0;
		for(i=0;i<mb_stride;i++){
			x=x+step_w;
			pDC->MoveTo(static_cast<int>(x), 0);
			pDC->LineTo(static_cast<int>(x),rect.Height());
		}
		//������
		x=0;
		y=0;
		for(j=0;j<(mb_sum/mb_stride);j++){
			y=y+step_h;
			pDC->MoveTo(0, static_cast<int>(y));
			pDC->LineTo(rect.Width(),static_cast<int>(y));
		}
	}
	delete pBrush;
	//-----------

	//------------------
	ReleaseDC(pDC); 
	//------
	dffinish=TRUE;
	//�����Ҫ�����BMP���Ա���һ
	hpic=CopyDCToBitmap(picdlg->GetDC()->GetSafeHdc(),rect);
	//����ͼ��
	if(m_dfanalysisoutpicfolder.GetCheck()==TRUE){
		CString folder_url,pic_name;
		m_dfanalysisoutpicfolderurl.GetWindowText(folder_url);
		//����ļ���·���Ƿ����
		CreateDirectory(folder_url,NULL);

		switch(m_dfanalysismethod.GetCurSel()){
		case DRAW_Q:pic_name.Format(_T("qp_%d.bmp"),frame_index);break;
		case DRAW_MB_TYPE:pic_name.Format(_T("mbtype_%d.bmp"),frame_index);break;
		case DRAW_MV0:pic_name.Format(_T("mv0_%d.bmp"),frame_index);break;
		case DRAW_MV1:pic_name.Format(_T("mv1_%d.bmp"),frame_index);break;
		case DRAW_REFINDEX0:pic_name.Format(_T("refindex_%d.bmp"),frame_index);break;
		case DRAW_REFINDEX1:pic_name.Format(_T("refindex_%d.bmp"),frame_index);break;
		}

		folder_url.AppendFormat(_T("\\%s"),pic_name);
		SaveBmp(hpic, folder_url);
	}

	//��������
	if(m_dfanalysisoutdatafolder.GetCheck()==TRUE){
		CString folder_url,data_name;
		m_dfanalysisoutdatafolderurl.GetWindowText(folder_url);
		//����ļ���·���Ƿ����
		CreateDirectory(folder_url,NULL);

		switch(m_dfanalysismethod.GetCurSel()){
		case DRAW_Q:data_name.Format(_T("qp_%d.csv"),frame_index);break;
		case DRAW_MB_TYPE:data_name.Format(_T("mbtype_%d.csv"),frame_index);break;
		case DRAW_MV0:data_name.Format(_T("mv0_%d.csv"),frame_index);break;
		case DRAW_MV1:data_name.Format(_T("mv1_%d.csv"),frame_index);break;
		case DRAW_REFINDEX0:data_name.Format(_T("refindex_%d.csv"),frame_index);break;
		case DRAW_REFINDEX1:data_name.Format(_T("refindex_%d.csv"),frame_index);break;
		}

		folder_url.AppendFormat(_T("\\%s"),data_name);
		CFile mFile(folder_url,CFile::modeReadWrite|CFile::modeCreate);
		mFile.Write(datastat,datastat.GetLength()*sizeof(TCHAR));
		mFile.Close();
	}
}



void Dfanalysis::ArrowTo1(HDC hDC, int x, int y, ARROWSTRUCT *pA) {

	POINT ptTo = {x, y};

	ArrowTo(hDC, &ptTo, pA);
}

void Dfanalysis::ArrowTo(HDC hDC, const POINT *lpTo, ARROWSTRUCT *pA) {

	POINT pFrom;
	POINT pBase;
	POINT aptPoly[3];
	float vecLine[2];
	float vecLeft[2];
	float fLength;
	float th;
	float ta;

	// get from point
	MoveToEx(hDC, 0, 0, &pFrom);

	// set to point
	aptPoly[0].x = lpTo->x;
	aptPoly[0].y = lpTo->y;

	// build the line vector
	vecLine[0] = (float) aptPoly[0].x - pFrom.x;
	vecLine[1] = (float) aptPoly[0].y - pFrom.y;

	// build the arrow base vector - normal to the line
	vecLeft[0] = -vecLine[1];
	vecLeft[1] = vecLine[0];

	// setup length parameters
	fLength = (float) sqrt(vecLine[0] * vecLine[0] + vecLine[1] * vecLine[1]);
	th = pA->nWidth / (2.0f * fLength);
	ta = pA->nWidth / (2.0f * (tanf(pA->fTheta) / 2.0f) * fLength);

	// find the base of the arrow
	pBase.x = (int) (aptPoly[0].x + -ta * vecLine[0]);
	pBase.y = (int) (aptPoly[0].y + -ta * vecLine[1]);

	// build the points on the sides of the arrow
	aptPoly[1].x = (int) (pBase.x + th * vecLeft[0]);
	aptPoly[1].y = (int) (pBase.y + th * vecLeft[1]);
	aptPoly[2].x = (int) (pBase.x + -th * vecLeft[0]);
	aptPoly[2].y = (int) (pBase.y + -th * vecLeft[1]);

	MoveToEx(hDC, pFrom.x, pFrom.y, NULL);

	// draw we're fillin'...
	if(pA->bFill) {
		LineTo(hDC, aptPoly[0].x, aptPoly[0].y);
		Polygon(hDC, aptPoly, 3);
	}

	// ... or even jes chillin'...
	else {
		LineTo(hDC, pBase.x, pBase.y);
		LineTo(hDC, aptPoly[1].x, aptPoly[1].y);
		LineTo(hDC, aptPoly[0].x, aptPoly[0].y);
		LineTo(hDC, aptPoly[2].x, aptPoly[2].y);
		LineTo(hDC, pBase.x, pBase.y);
		MoveToEx(hDC, aptPoly[0].x, aptPoly[0].y, NULL);
	}
}


void Dfanalysis::OnBnClickedDfanalysisAuto()
{
	if(m_dfanalysisauto.GetCheck()==1){
		CString resloader;
		GetDlgItem(IDC_DFANALYSIS_AUTO_INTERFRAMENUM)->EnableWindow(TRUE);
		GetDlgItem(IDC_DFANALYSIS_AUTO_OK)->EnableWindow(TRUE);
	}else{
		GetDlgItem(IDC_DFANALYSIS_AUTO_INTERFRAMENUM)->EnableWindow(FALSE);
		GetDlgItem(IDC_DFANALYSIS_AUTO_OK)->EnableWindow(FALSE);
	}
}


void Dfanalysis::OnKillfocusDfanalysisAutoInterframenum()
{
	UpdateData(TRUE);
}


void Dfanalysis::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	//��ͼ�������ӵ�OnInitDialog������Ч
	//��Ӧ�����ӵ�OnPaint������
	DrawSample();
}




void Dfanalysis::DrawSample(){
	//-----------------
	CDC *pDC=NULL;
	CRect rect;
	pDC=GetDlgItem(IDC_DFANALYSIS_SAMPLE_INTRA4X4)->GetDC();
	//4x4���?
	GetDlgItem(IDC_DFANALYSIS_SAMPLE_INTRA4X4)->GetClientRect(&rect);
	pBrush=new CBrush();
	pBrush->CreateSolidBrush(RGB(255,0,0));
	pDC->FillRect(rect,pBrush);
	pBrush->DeleteObject();
	//��3�����ߺ�����
	int m;
	//����
	for(m=1;m<4;m++){
		pDC->MoveTo(rect.left+m*rect.Width()/4,rect.top);
		pDC->LineTo(rect.left+m*rect.Width()/4,rect.bottom);
	}
	//����
	for(m=1;m<4;m++){
		pDC->MoveTo(rect.left,rect.top+m*rect.Height()/4);
		pDC->LineTo(rect.right,rect.top+m*rect.Height()/4);
	}
	pDC=GetDlgItem(IDC_DFANALYSIS_SAMPLE_INTRA16X16)->GetDC();
	GetDlgItem(IDC_DFANALYSIS_SAMPLE_INTRA16X16)->GetClientRect(&rect);
	pBrush->CreateSolidBrush(RGB(128,0,0));
	pDC->FillRect(rect,pBrush);
	pBrush->DeleteObject();
	pDC=GetDlgItem(IDC_DFANALYSIS_SAMPLE_16X16)->GetDC();
	GetDlgItem(IDC_DFANALYSIS_SAMPLE_16X16)->GetClientRect(&rect);
	pBrush->CreateSolidBrush(RGB(255,99,0));
	pDC->FillRect(rect,pBrush);//�ػ�
	pBrush->DeleteObject();
	pDC=GetDlgItem(IDC_DFANALYSIS_SAMPLE_16X8)->GetDC();
	GetDlgItem(IDC_DFANALYSIS_SAMPLE_16X8)->GetClientRect(&rect);
	pBrush->CreateSolidBrush(RGB(0,99,0));
	pDC->FillRect(rect,pBrush);
	pBrush->DeleteObject();

	//��1������
	//����
	for(m=1;m<2;m++){
		pDC->MoveTo(rect.left,rect.top+m*rect.Height()/2);
		pDC->LineTo(rect.right,rect.top+m*rect.Height()/2);
	}
	pDC=GetDlgItem(IDC_DFANALYSIS_SAMPLE_8X16)->GetDC();
	GetDlgItem(IDC_DFANALYSIS_SAMPLE_8X16)->GetClientRect(&rect);
	pBrush->CreateSolidBrush(RGB(255,99,255));
	pDC->FillRect(rect,pBrush);
	pBrush->DeleteObject();

	//��1������
	//����
	for(m=1;m<2;m++){
		pDC->MoveTo(rect.left+m*rect.Width()/2,rect.top);
		pDC->LineTo(rect.left+m*rect.Width()/2,rect.bottom);
	}
	pDC=GetDlgItem(IDC_DFANALYSIS_SAMPLE_8X8)->GetDC();
	GetDlgItem(IDC_DFANALYSIS_SAMPLE_8X8)->GetClientRect(&rect);
	pBrush->CreateSolidBrush(RGB(0,99,255));
	pDC->FillRect(rect,pBrush);
	pBrush->DeleteObject();

	//��1�����ߺ�����
	//����
	for(m=1;m<2;m++){
		pDC->MoveTo(rect.left+m*rect.Width()/2,rect.top);
		pDC->LineTo(rect.left+m*rect.Width()/2,rect.bottom);
	}
	//����
	for(m=1;m<2;m++){
		pDC->MoveTo(rect.left,rect.top+m*rect.Height()/2);
		pDC->LineTo(rect.right,rect.top+m*rect.Height()/2);
	}
	pDC=GetDlgItem(IDC_DFANALYSIS_SAMPLE_SKIP)->GetDC();
	GetDlgItem(IDC_DFANALYSIS_SAMPLE_SKIP)->GetClientRect(&rect);
	//����ɫ͸��
	pDC->SetBkMode(TRANSPARENT);
	//����ַ�?
	pDC->TextOut(rect.left+2,rect.top+2,_T("s"));
	//����-------
	pDC=GetDlgItem(IDC_DFANALYSIS_SAMPLE_L0)->GetDC();
	GetDlgItem(IDC_DFANALYSIS_SAMPLE_L0)->GetClientRect(&rect);
	//б��
	pDC->MoveTo(rect.left,rect.top);
	pDC->LineTo(rect.right,rect.bottom);
	pDC=GetDlgItem(IDC_DFANALYSIS_SAMPLE_L1)->GetDC();
	GetDlgItem(IDC_DFANALYSIS_SAMPLE_L1)->GetClientRect(&rect);
	//б��
	pDC->MoveTo(rect.left,rect.bottom);
	pDC->LineTo(rect.right,rect.top);
	//��佥�����?---------
	//CDC��HDC����
	//CDC��MFC��DC��һ���� 
	//HDC��DC�ľ��?API�е�һ������ָ�����������?
	//CDC���豸���·���,������һ����ĳ�Ա����?m_nHdc;��HDC���͵ľ��? 
	pDC=GetDlgItem(IDC_DFANALYSIS_SAMPLE_Q)->GetDC();
	GetDlgItem(IDC_DFANALYSIS_SAMPLE_Q)->GetClientRect(&rect);
	HDC hDC = pDC->m_hDC;
	//��ɫ�ṹ----
	//�����ǰ�ɫ����ɫ
	TRIVERTEX rcVertex[2];
	rcVertex[0].x=rect.left;
	rcVertex[0].y=rect.top;
	rcVertex[0].Red=255<<8; // color values from 0x0000 to 0xff00 !!!!
	rcVertex[0].Green=255<<8;
	rcVertex[0].Blue=255<<8;
	rcVertex[0].Alpha=0x0000;
	rcVertex[1].x=rect.right; 
	rcVertex[1].y=rect.bottom;
	rcVertex[1].Red=0;
	rcVertex[1].Green=0;
	rcVertex[1].Blue=0;
	rcVertex[1].Alpha=0;
	//��ɫ�ṹTRIVERTEX��ͼ�񷽿�Ķ�Ӧ���?-------
	GRADIENT_RECT rect1;
	rect1.UpperLeft=0;
	rect1.LowerRight=1;

	//������ĺ���?
	GradientFill(hDC,rcVertex,2,&rect1,1,GRADIENT_FILL_RECT_V);
	//----
	delete(pBrush);
	ReleaseDC(pDC); 
}

void Dfanalysis::DrawFrame(){
	//�Ƚ�����������ת����RGB��ʽ�ģ��Է�����ʾ
	AVFrame	*pFrameYUV=NULL;
	pFrameYUV=av_frame_alloc();
	uint8_t *out_buffer=NULL;
	struct SwsContext *img_convert_ctx=NULL;
	out_buffer=new uint8_t[av_image_get_buffer_size(PIX_FMT_BGR24, width, height, 1)];
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, PIX_FMT_BGR24, width, height, 1);
	img_convert_ctx = sws_getContext(width, height, pix_fmt, width, height, PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL); 
	sws_scale(img_convert_ctx, (const uint8_t* const*)data, linesize, 0, height, pFrameYUV->data, pFrameYUV->linesize);
	sws_freeContext(img_convert_ctx);


	//----------------------------------
	CRect rect;
	CWnd *dcd=NULL;
	HWND phWnd=NULL;
	//dcd = picdlg->GetDlgItem(IDD_DFANALYSIS_PIC); // ��ȡ�ؼ����?
	//�Ρ�����û�뵽��Ȼ����ǿ������ת��
	dcd=(CWnd *)picdlg;

	CDC *pDC = dcd->GetDC();  // ��ȡ�ؼ��豸
	dcd->GetClientRect(&rect);
	HDC m_hdc=pDC->GetSafeHdc();

	pDC->SetWindowExt(rect.Width(), rect.Height());     // ���ô��ڷ�Χ
	//BMP�ļ�ͷ
	BITMAPINFO *m_bmphdr=NULL;
	DWORD dwBmpHdr = sizeof(BITMAPINFO);
	m_bmphdr = new BITMAPINFO[dwBmpHdr];
	m_bmphdr->bmiHeader.biBitCount = 24;
	m_bmphdr->bmiHeader.biClrImportant = 0;
	m_bmphdr->bmiHeader.biSize = dwBmpHdr;
	m_bmphdr->bmiHeader.biSizeImage = 0;
	m_bmphdr->bmiHeader.biWidth = width;
	//ע��BMP��y�����Ƿ��Ŵ洢�ģ�һ�α�������һ����ֵ������ʹͼ��������ʾ����
	m_bmphdr->bmiHeader.biHeight = -height;
	m_bmphdr->bmiHeader.biXPelsPerMeter = 0;
	m_bmphdr->bmiHeader.biYPelsPerMeter = 0;
	m_bmphdr->bmiHeader.biClrUsed = 0;
	m_bmphdr->bmiHeader.biPlanes = 1;
	m_bmphdr->bmiHeader.biCompression = BI_RGB;
	//��RGB���ݻ��ڿؼ���
	//ͼ�������Ƿ��ģ�Why��
	int nResult = ::StretchDIBits(m_hdc,
		0,0,
		rect.Width(),//rc.right - rc.left,
		rect.Height(),//rc.top,
		0, 0,
		width, height,
		pFrameYUV->data[0],
		m_bmphdr,
		DIB_RGB_COLORS,
		SRCCOPY);

	free(m_bmphdr);
	free(out_buffer);
	av_free(pFrameYUV);
}

void Dfanalysis::OnBnClickedCancel()
{
	// TODO: �ڴ����ӿؼ�֪ͨ�����������?
	m_dfanalysisauto.SetCheck(0);
	ShowWindow(FALSE);
}

HBITMAP Dfanalysis::CopyDCToBitmap(HDC hScrDC, LPRECT lpRect)
{
	if(hScrDC==NULL || lpRect==NULL || IsRectEmpty(lpRect)){
		AfxMessageBox(_T("Parameter Error"));
		return NULL;
	}

	HDC hMemDC;      
	// ��Ļ���ڴ��豸������
	HBITMAP    hBitmap,hOldBitmap;   
	// λͼ���?
	int  nX, nY, nX2, nY2;      
	// ѡ����������
	int  nWidth, nHeight;      
	// λͼ���Ⱥ͸߶�
	// ȷ��ѡ������Ϊ�վ���
	if (IsRectEmpty(lpRect))
		return NULL;
	// ���ѡ����������?
	nX = lpRect->left;
	nY = lpRect->top;
	nX2 = lpRect->right;
	nY2 = lpRect->bottom;

	nWidth = nX2 - nX;
	nHeight = nY2 - nY;
	//Ϊָ���豸�������������ݵ��ڴ��豸������
	hMemDC = CreateCompatibleDC(hScrDC);
	// ����һ����ָ���豸���������ݵ�λͼ
	hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);
	// ����λͼѡ���ڴ��豸��������
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
	// ����Ļ�豸�������������ڴ��豸��������
	StretchBlt(hMemDC,0,0,nWidth,nHeight,hScrDC,nX,nY,nWidth,nHeight,SRCCOPY);
	//BitBlt(hMemDC, 0, 0, nWidth, nHeight,hScrDC, nX, nY, SRCCOPY);
	//�õ���Ļλͼ�ľ��?

	hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap);
	//���?

	DeleteDC(hMemDC);
	DeleteObject(hOldBitmap);
	// ����λͼ���?
	return hBitmap;
}

BOOL Dfanalysis::SaveBmp(HBITMAP hBitmap, CString FileName) 
{ 
	HDC hDC; 
	//��ǰ�ֱ�����ÿ������ռ�ֽ��� 
	int iBits; 
	//λͼ��ÿ������ռ�ֽ��� 
	WORD wBitCount; 
	//�����ɫ���С�� λͼ�������ֽڴ�С ��λͼ�ļ���С �� д���ļ��ֽ��� 
	DWORD dwPaletteSize=0, dwBmBitsSize=0, dwDIBSize=0, dwWritten=0; 
	//λͼ���Խṹ 
	BITMAP Bitmap; 
	//λͼ�ļ�ͷ�ṹ 
	BITMAPFILEHEADER bmfHdr; 
	//λͼ��Ϣͷ�ṹ 
	BITMAPINFOHEADER bi; 
	//ָ��λͼ��Ϣͷ�ṹ 
	LPBITMAPINFOHEADER lpbi; 
	//�����ļ��������ڴ�������ɫ���� 
	HANDLE fh, hDib, hPal,hOldPal=NULL; 

	//����λͼ�ļ�ÿ��������ռ�ֽ��� 
	hDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL); 
	iBits = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES); 
	DeleteDC(hDC); 
	if (iBits <= 1) wBitCount = 1; 
	else if (iBits <= 4) wBitCount = 4; 
	else if (iBits <= 8) wBitCount = 8; 
	else wBitCount = 24; 

	GetObject( hBitmap, sizeof( Bitmap ), ( LPSTR )&Bitmap ); 
	bi.biSize = sizeof( BITMAPINFOHEADER ); 
	bi.biWidth = Bitmap.bmWidth; 
	bi.biHeight = Bitmap.bmHeight; 
	bi.biPlanes = 1; 
	bi.biBitCount = wBitCount; 
	bi.biCompression = BI_RGB; 
	bi.biSizeImage = 0; 
	bi.biXPelsPerMeter = 0; 
	bi.biYPelsPerMeter = 0; 
	bi.biClrImportant = 0; 
	bi.biClrUsed = 0; 

	dwBmBitsSize = ((Bitmap.bmWidth * wBitCount + 31) / 32) * 4 * Bitmap.bmHeight; 

	//Ϊλͼ���ݷ����ڴ� 
	hDib = GlobalAlloc(GHND,dwBmBitsSize + dwPaletteSize + sizeof(BITMAPINFOHEADER)); 
	lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib); 
	*lpbi = bi; 

	// ������ɫ�� 
	hPal = GetStockObject(DEFAULT_PALETTE); 
	if (hPal) 
	{ 
		hDC = ::GetDC(NULL); 
		//hDC = m_pDc->GetSafeHdc(); 
		hOldPal = ::SelectPalette(hDC, (HPALETTE)hPal, FALSE); 
		RealizePalette(hDC); 
	} 
	// ��ȡ�õ�ɫ�����µ�����ֵ 
	GetDIBits(hDC, hBitmap, 0, (UINT) Bitmap.bmHeight, (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) 
		+dwPaletteSize, (BITMAPINFO *)lpbi, DIB_RGB_COLORS); 

	//�ָ���ɫ�� 
	if (hOldPal) 
	{ 
		::SelectPalette(hDC, (HPALETTE)hOldPal, TRUE); 
		RealizePalette(hDC); 
		::ReleaseDC(NULL, hDC); 
	} 


	//����λͼ�ļ� 
	fh = CreateFile(FileName, GENERIC_WRITE,0, NULL, CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL); 

	if (fh == INVALID_HANDLE_VALUE) return FALSE; 

	// ����λͼ�ļ�ͷ 
	bmfHdr.bfType = 0x4D42; // "BM" 
	dwDIBSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwPaletteSize + dwBmBitsSize; 
	bmfHdr.bfSize = dwDIBSize; 
	bmfHdr.bfReserved1 = 0; 
	bmfHdr.bfReserved2 = 0; 
	bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER) + dwPaletteSize;
	// д��λͼ�ļ�ͷ 
	WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL); 
	// д��λͼ�ļ��������� 
	WriteFile(fh, (LPSTR)lpbi, dwDIBSize, &dwWritten, NULL); 
	//���?
	GlobalUnlock(hDib); 
	GlobalFree(hDib); 
	CloseHandle(fh); 

	return TRUE; 
} 



void Dfanalysis::OnDestroy()
{
	delete picdlg;
	CDialogEx::OnDestroy();
}


void Dfanalysis::OnClickedDfanalysisOutpicfolder()
{
	if(m_dfanalysisoutpicfolder.GetCheck()==FALSE){
		m_dfanalysisoutpicfolderurl.EnableWindow(FALSE);
	}else{
		m_dfanalysisoutpicfolderurl.EnableWindow(TRUE);
	}
}


void Dfanalysis::OnClickedDfanalysisOutdatafolder()
{
	if(m_dfanalysisoutdatafolder.GetCheck()==FALSE){
		m_dfanalysisoutdatafolderurl.EnableWindow(FALSE);
	}else{
		m_dfanalysisoutdatafolderurl.EnableWindow(TRUE);
	}
}
