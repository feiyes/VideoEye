/* 
 * FFplay for MFC
 *
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 *
 * http://blog.csdn.net/leixiaohua1020
 * 
 * 本程序移植ffmpeg项目中的ffplay，将ffplay.c移植到VC环境下。
 * 使用MFC构建一个简单的图形界面。
 * This software transplant ffplay to Microsoft VC++ environment. 
 * And use MFC to build a simple Graphical User Interface. 
 */

#include "stdafx.h"
#include "Sysinfo.h"
#include "afxdialogex.h"


// Sysinfosubac 对话框

IMPLEMENT_DYNAMIC(Sysinfosubac, CDialogEx)

	Sysinfosubac::Sysinfosubac(CWnd* pParent /*=NULL*/)
	: CDialogEx(Sysinfosubac::IDD, pParent)
{

}

Sysinfosubac::~Sysinfosubac()
{
}

void Sysinfosubac::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SYSINFO_AC, m_sysinfoac);
}


BEGIN_MESSAGE_MAP(Sysinfosubac, CDialogEx)
END_MESSAGE_MAP()



// Sysinfosubif 对话框

IMPLEMENT_DYNAMIC(Sysinfosubif, CDialogEx)

	Sysinfosubif::Sysinfosubif(CWnd* pParent /*=NULL*/)
	: CDialogEx(Sysinfosubif::IDD, pParent)
{

}

Sysinfosubif::~Sysinfosubif()
{
}

void Sysinfosubif::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SYSINFO_IF, m_sysinfoif);
}


BEGIN_MESSAGE_MAP(Sysinfosubif, CDialogEx)
END_MESSAGE_MAP()


// Sysinfosubif 消息处理函数

// Sysinfosuboc 对话框

IMPLEMENT_DYNAMIC(Sysinfosuboc, CDialogEx)

	Sysinfosuboc::Sysinfosuboc(CWnd* pParent /*=NULL*/)
	: CDialogEx(Sysinfosuboc::IDD, pParent)
{

}

Sysinfosuboc::~Sysinfosuboc()
{
}

void Sysinfosuboc::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SYSINFO_OC, m_sysinfooc);
}


BEGIN_MESSAGE_MAP(Sysinfosuboc, CDialogEx)
END_MESSAGE_MAP()


// Sysinfosuboc 消息处理函数

// Sysinfosubup 对话框

IMPLEMENT_DYNAMIC(Sysinfosubup, CDialogEx)

	Sysinfosubup::Sysinfosubup(CWnd* pParent /*=NULL*/)
	: CDialogEx(Sysinfosubup::IDD, pParent)
{

}

Sysinfosubup::~Sysinfosubup()
{
}

void Sysinfosubup::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SYSINFO_UP, m_sysinfoup);
}


BEGIN_MESSAGE_MAP(Sysinfosubup, CDialogEx)
END_MESSAGE_MAP()


// Sysinfosubup 消息处理函数


// Sysinfosubvc 对话框

IMPLEMENT_DYNAMIC(Sysinfosubvc, CDialogEx)

	Sysinfosubvc::Sysinfosubvc(CWnd* pParent /*=NULL*/)
	: CDialogEx(Sysinfosubvc::IDD, pParent)
{

}

Sysinfosubvc::~Sysinfosubvc()
{
}

void Sysinfosubvc::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SYSINFO_VC, m_sysinfovc);
}


BEGIN_MESSAGE_MAP(Sysinfosubvc, CDialogEx)
END_MESSAGE_MAP()


// Sysinfosubvc 消息处理函数



// Sysinfo 对话框

IMPLEMENT_DYNAMIC(Sysinfo, CDialogEx)

Sysinfo::Sysinfo(CWnd* pParent /*=NULL*/)
	: CDialogEx(Sysinfo::IDD, pParent), m_CurSelTab(0)
{
	for (int i = 0; i < 5; i++) {
		pDialog[i] = NULL;
	}
	memset(&si, 0, sizeof(SystemInfo));
}

Sysinfo::~Sysinfo()
{
}

void Sysinfo::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SYSINFO_TAB, m_sysinfotab);
}


BEGIN_MESSAGE_MAP(Sysinfo, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_SYSINFO_TAB, &Sysinfo::OnSelchangeSysinfoTab)
END_MESSAGE_MAP()


// Sysinfo 消息处理函数
BOOL Sysinfo::OnInitDialog(){
	CDialogEx::OnInitDialog();

	//系统支持信息
	CString resloader;
	(void)resloader.LoadString(IDS_SYSINFO);
	SetWindowText(resloader);
	
	sysinfosubup.Create(IDD_SYSINFOSUB_UP,&m_sysinfotab);
	sysinfosubif.Create(IDD_SYSINFOSUB_IF,&m_sysinfotab);
	sysinfosubvc.Create(IDD_SYSINFOSUB_VC,&m_sysinfotab);
	sysinfosubac.Create(IDD_SYSINFOSUB_AC,&m_sysinfotab);
	sysinfosuboc.Create(IDD_SYSINFOSUB_OC,&m_sysinfotab);

	pDialog[0]=&sysinfosubup;
	pDialog[1]=&sysinfosubif;
	pDialog[2]=&sysinfosubvc;
	pDialog[3]=&sysinfosubac;
	pDialog[4]=&sysinfosuboc;

	(void)resloader.LoadString(IDS_SYSINFO_UP);
	m_sysinfotab.InsertItem(0,resloader);
	(void)resloader.LoadString(IDS_SYSINFO_IF);
	m_sysinfotab.InsertItem(1,resloader);
	(void)resloader.LoadString(IDS_SYSINFO_VC);
	m_sysinfotab.InsertItem(2,resloader);
	(void)resloader.LoadString(IDS_SYSINFO_AC);
	m_sysinfotab.InsertItem(3,resloader);
	(void)resloader.LoadString(IDS_SYSINFO_OC);
	m_sysinfotab.InsertItem(4,resloader);

	//设置列表控件样式：全行选中、网格线、表头拖拽、单击激活
	DWORD dwExStyle=LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_HEADERDRAGDROP|LVS_EX_ONECLICKACTIVATE;
	//设置列表：单选、报表模式、始终显示选中状态
	//输入格式
	sysinfosubif.m_sysinfoif.ModifyStyle(0,LVS_SINGLESEL|LVS_REPORT|LVS_SHOWSELALWAYS);
	sysinfosubif.m_sysinfoif.SetExtendedStyle(dwExStyle);

	(void)resloader.LoadString(IDS_SYSINFO_COMMON_NUM);
	sysinfosubif.m_sysinfoif.InsertColumn(0,resloader,LVCFMT_CENTER,40,0);
	(void)resloader.LoadString(IDS_SYSINFO_COMMON_NAME);
	sysinfosubif.m_sysinfoif.InsertColumn(1,resloader,LVCFMT_CENTER,60,0);
	(void)resloader.LoadString(IDS_SYSINFO_COMMON_FULLNAME);
	sysinfosubif.m_sysinfoif.InsertColumn(2,resloader,LVCFMT_CENTER,200,0);
	(void)resloader.LoadString(IDS_SYSINFO_IF_EXT);
	sysinfosubif.m_sysinfoif.InsertColumn(3,resloader,LVCFMT_CENTER,60,0);
	(void)resloader.LoadString(IDS_SYSINFO_COMMON_PRIVSIZE);
	sysinfosubif.m_sysinfoif.InsertColumn(4,resloader,LVCFMT_CENTER,90,0);
	//-------------------------------------
	sysinfosubac.m_sysinfoac.ModifyStyle(0,LVS_SINGLESEL|LVS_REPORT|LVS_SHOWSELALWAYS);
	sysinfosubac.m_sysinfoac.SetExtendedStyle(dwExStyle);

	(void)resloader.LoadString(IDS_SYSINFO_COMMON_NUM);
	sysinfosubac.m_sysinfoac.InsertColumn(0,resloader,LVCFMT_CENTER,40,0);
	(void)resloader.LoadString(IDS_SYSINFO_COMMON_NAME);
	sysinfosubac.m_sysinfoac.InsertColumn(1,resloader,LVCFMT_CENTER,60,0);
	(void)resloader.LoadString(IDS_SYSINFO_COMMON_FULLNAME);
	sysinfosubac.m_sysinfoac.InsertColumn(2,resloader,LVCFMT_CENTER,200,0);
	(void)resloader.LoadString(IDS_SYSINFO_AC_SAMPLERATE);
	sysinfosubac.m_sysinfoac.InsertColumn(3,resloader,LVCFMT_CENTER,80,0);
	(void)resloader.LoadString(IDS_SYSINFO_AC_SAMPLEFMT);
	sysinfosubac.m_sysinfoac.InsertColumn(4,resloader,LVCFMT_CENTER,80,0);
	(void)resloader.LoadString(IDS_SYSINFO_COMMON_PRIVSIZE);
	sysinfosubac.m_sysinfoac.InsertColumn(5,resloader,LVCFMT_CENTER,90,0);
	//--------------------------------------
	sysinfosubvc.m_sysinfovc.ModifyStyle(0,LVS_SINGLESEL|LVS_REPORT|LVS_SHOWSELALWAYS);
	sysinfosubvc.m_sysinfovc.SetExtendedStyle(dwExStyle);

	(void)resloader.LoadString(IDS_SYSINFO_COMMON_NUM);
	sysinfosubvc.m_sysinfovc.InsertColumn(0,resloader,LVCFMT_CENTER,40,0);
	(void)resloader.LoadString(IDS_SYSINFO_COMMON_NAME);
	sysinfosubvc.m_sysinfovc.InsertColumn(1,resloader,LVCFMT_CENTER,60,0);
	(void)resloader.LoadString(IDS_SYSINFO_COMMON_FULLNAME);
	sysinfosubvc.m_sysinfovc.InsertColumn(2,resloader,LVCFMT_CENTER,200,0);
	(void)resloader.LoadString(IDS_SYSINFO_VC_FRAMERATE);
	sysinfosubvc.m_sysinfovc.InsertColumn(3,resloader,LVCFMT_CENTER,80,0);
	(void)resloader.LoadString(IDS_SYSINFO_VC_PIXFMT);
	sysinfosubvc.m_sysinfovc.InsertColumn(4,resloader,LVCFMT_CENTER,80,0);
	(void)resloader.LoadString(IDS_SYSINFO_COMMON_PRIVSIZE);
	sysinfosubvc.m_sysinfovc.InsertColumn(5,resloader,LVCFMT_CENTER,90,0);
	//--------------------------------------
	sysinfosubup.m_sysinfoup.ModifyStyle(0,LVS_SINGLESEL|LVS_REPORT|LVS_SHOWSELALWAYS);
	sysinfosubup.m_sysinfoup.SetExtendedStyle(dwExStyle);

	(void)resloader.LoadString(IDS_SYSINFO_COMMON_NUM);
	sysinfosubup.m_sysinfoup.InsertColumn(0,resloader,LVCFMT_CENTER,40,0);
	(void)resloader.LoadString(IDS_SYSINFO_COMMON_NAME);
	sysinfosubup.m_sysinfoup.InsertColumn(1,resloader,LVCFMT_CENTER,200,0);
	//--------------------------------------
	sysinfosuboc.m_sysinfooc.ModifyStyle(0,LVS_SINGLESEL|LVS_REPORT|LVS_SHOWSELALWAYS);
	sysinfosuboc.m_sysinfooc.SetExtendedStyle(dwExStyle);

	(void)resloader.LoadString(IDS_SYSINFO_COMMON_NUM);
	sysinfosuboc.m_sysinfooc.InsertColumn(0,resloader,LVCFMT_CENTER,40,0);
	(void)resloader.LoadString(IDS_SYSINFO_COMMON_NAME);
	sysinfosuboc.m_sysinfooc.InsertColumn(1,resloader,LVCFMT_CENTER,60,0);
	(void)resloader.LoadString(IDS_SYSINFO_COMMON_FULLNAME);
	sysinfosuboc.m_sysinfooc.InsertColumn(2,resloader,LVCFMT_CENTER,200,0);
	(void)resloader.LoadString(IDS_SYSINFO_COMMON_PRIVSIZE);
	sysinfosuboc.m_sysinfooc.InsertColumn(3,resloader,LVCFMT_CENTER,90,0);
	//--------------------------------------
	//�趨��Tab����ʾ�ķ�Χ
	CRect rc;
	m_sysinfotab.GetClientRect(rc);
	rc.top += 20;
	rc.bottom -= 0;
	rc.left += 0;
	rc.right -= 0;

	sysinfosubup.MoveWindow(&rc);
	sysinfosubif.MoveWindow(&rc);
	sysinfosubvc.MoveWindow(&rc);
	sysinfosubac.MoveWindow(&rc);
	sysinfosuboc.MoveWindow(&rc);

	pDialog[0]->ShowWindow(TRUE);
	//-------------
	m_CurSelTab=0;

	GetSysinfo();

	return TRUE;
}

void Sysinfo::GetSysinfo(){


	//ȡ��ϵͳ֧�ֵĸ�ʽ��Ϣ��Э�飬��װ��ʽ����������
	// 注意：av_register_all() 在新版FFmpeg中已被弃�?
	// av_register_all();
	
	// 使用新版FFmpeg API枚举协议
	void *opaque = NULL;
	int up_index = 0;
	const char *protocol_name = NULL;
	
	while ((protocol_name = avio_enum_protocols(&opaque, 0)) != NULL) {
		CString f_index, name, isread, iswrite, priv_data_size;
		int nIndex = 0;

#ifdef _UNICODE
		name.Format(_T("%S"), protocol_name);
#else
		name.Format(_T("%s"), protocol_name);
#endif
		f_index.Format(_T("%d"), up_index);
		// 获取当前行数
		nIndex = sysinfosubup.m_sysinfoup.GetItemCount();
		// 小结构数�?
		LV_ITEM lvitem;
		lvitem.mask = LVIF_TEXT;
		lvitem.iItem = nIndex;
		lvitem.iSubItem = 0;
		lvitem.pszText = f_index.GetBuffer();
		sysinfosubup.m_sysinfoup.InsertItem(&lvitem);
		sysinfosubup.m_sysinfoup.SetItemText(nIndex, 1, name);
		up_index++;
	}




	si.first_if= av_iformat_next(NULL);
	si.first_c=av_codec_next(NULL);

	AVInputFormat *if_temp=si.first_if;
	AVCodec *c_temp=si.first_c;
	//InputFormat
	int if_index=0;
	while(if_temp!=NULL){
		CString f_index,name,long_name,extensions,priv_data_size;
		int nIndex=0;
#ifdef _UNICODE
		name.Format(_T("%S"),if_temp->name);
		long_name.Format(_T("%S"),if_temp->long_name);
		extensions.Format(_T("%S"),if_temp->extensions);
#else
		name.Format(_T("%s"),if_temp->name);
		long_name.Format(_T("%s"),if_temp->long_name);
		extensions.Format(_T("%s"),if_temp->extensions);
#endif
		priv_data_size.Format(_T("%d"), AVInputFormat_priv_data_size(if_temp));
		f_index.Format(_T("%d"),if_index);
		//��ȡ��ǰ��¼����
		nIndex=sysinfosubif.m_sysinfoif.GetItemCount();
		//���С����ݽṹ
		LV_ITEM lvitem;
		lvitem.mask=LVIF_TEXT;
		lvitem.iItem=nIndex;
		lvitem.iSubItem=0;
		//ע��vframe_index������ֱ�Ӹ�ֵ��
		//���ʹ��f_indexִ��Format!�ٸ�ֵ��
		lvitem.pszText=f_index.GetBuffer();
		//------------------------
		sysinfosubif.m_sysinfoif.InsertItem(&lvitem);
		sysinfosubif.m_sysinfoif.SetItemText(nIndex,1,name);
		sysinfosubif.m_sysinfoif.SetItemText(nIndex,2,long_name);
		sysinfosubif.m_sysinfoif.SetItemText(nIndex,3,extensions);
		sysinfosubif.m_sysinfoif.SetItemText(nIndex,4,priv_data_size);
		if_temp = (AVInputFormat *)AVInputFormat_next(if_temp);
		if_index++;
	}
	//Codec
	int c_index=0;
	while(c_temp!=NULL){
		CString f_index,name,long_name,priv_data_size,capabilities,
			supported_framerates,pix_fmts,supported_samplerates,sample_fmts,channel_layouts;
		int nIndex=0;

#ifdef _UNICODE
		name.Format(_T("%S"),c_temp->name);
		long_name.Format(_T("%S"),c_temp->long_name);
#else
		name.Format(_T("%s"),c_temp->name);
		long_name.Format(_T("%s"),c_temp->long_name);
#endif
		priv_data_size.Format(_T("%d"), AVCodec_priv_data_size(c_temp));
		f_index.Format(_T("%d"),c_index);
		//���С����ݽṹ
		LV_ITEM lvitem;
		lvitem.mask=LVIF_TEXT;
		lvitem.iSubItem=0;

		switch(c_temp->type){
		case AVMEDIA_TYPE_VIDEO:
		{
			// 使用旧版FFmpeg API（新版API在当前版本中不存在）
			#pragma warning(push)
			#pragma warning(disable: 4996)
			const AVRational *fps_list = c_temp->supported_framerates;
			const enum AVPixelFormat *pf_list = c_temp->pix_fmts;
			#pragma warning(pop)

			if(fps_list == NULL || fps_list->num == 0){
				supported_framerates.Format(_T("Any"));
			}else{
				float sf_cal = (float)fps_list->num / (float)fps_list->den;
				supported_framerates.Format(_T("%f"), sf_cal);
			}

			if(pf_list == NULL){
				pix_fmts.Format(_T("Unknown"));
			}else{
				const enum AVPixelFormat *pf_temp = pf_list;
				while(*pf_temp != -1){
					pix_fmts.AppendFormat(_T("%d;"), *pf_temp);
					pf_temp++;
				}
			}

			// 获取当前行数
			nIndex=sysinfosubvc.m_sysinfovc.GetItemCount();
			lvitem.iItem=nIndex;

			lvitem.pszText=f_index.GetBuffer();
			//------------------------
			sysinfosubvc.m_sysinfovc.InsertItem(&lvitem);
			sysinfosubvc.m_sysinfovc.SetItemText(nIndex,1,name);
			sysinfosubvc.m_sysinfovc.SetItemText(nIndex,2,long_name);
			sysinfosubvc.m_sysinfovc.SetItemText(nIndex,3,supported_framerates);
			sysinfosubvc.m_sysinfovc.SetItemText(nIndex,4,pix_fmts);
			sysinfosubvc.m_sysinfovc.SetItemText(nIndex,5,priv_data_size);

			break;
		}
		case AVMEDIA_TYPE_AUDIO:
		{
			// 使用旧版FFmpeg API（新版API在当前版本中不存在）
			#pragma warning(push)
			#pragma warning(disable: 4996)
			const int *sr_list = c_temp->supported_samplerates;
			const enum AVSampleFormat *sf_list = c_temp->sample_fmts;
			#pragma warning(pop)

			if(sr_list == NULL){
				supported_samplerates.Format(_T("Unknown"));
			}else{
				const int *sr_temp = sr_list;
				while(*sr_temp != 0){
					supported_samplerates.AppendFormat(_T("%d;"), *sr_temp);
					sr_temp++;
				}
			}

			if(sf_list == NULL){
				sample_fmts.Format(_T("Any"));
			}else{
				const enum AVSampleFormat *sf_temp = sf_list;
				while(*sf_temp != -1){
					sample_fmts.AppendFormat(_T("%d;"), *sf_temp);
					sf_temp++;
				}
			}

			//��ȡ��ǰ��¼����
			nIndex=sysinfosubac.m_sysinfoac.GetItemCount();
			lvitem.iItem=nIndex;

			lvitem.pszText=f_index.GetBuffer();
			//------------------------
			sysinfosubac.m_sysinfoac.InsertItem(&lvitem);
			sysinfosubac.m_sysinfoac.SetItemText(nIndex,1,name);
			sysinfosubac.m_sysinfoac.SetItemText(nIndex,2,long_name);
			sysinfosubac.m_sysinfoac.SetItemText(nIndex,3,supported_samplerates);
			sysinfosubac.m_sysinfoac.SetItemText(nIndex,4,sample_fmts);
			sysinfosubac.m_sysinfoac.SetItemText(nIndex,5,priv_data_size);
			break;
		}
		default:
			//��ȡ��ǰ��¼����
			nIndex=sysinfosuboc.m_sysinfooc.GetItemCount();
			lvitem.iItem=nIndex;

			lvitem.pszText=f_index.GetBuffer();
			//------------------------
			sysinfosuboc.m_sysinfooc.InsertItem(&lvitem);
			sysinfosuboc.m_sysinfooc.SetItemText(nIndex,1,name);
			sysinfosuboc.m_sysinfooc.SetItemText(nIndex,2,long_name);
			sysinfosuboc.m_sysinfooc.SetItemText(nIndex,3,priv_data_size);
			break;
		}
		c_temp = (AVCodec *)AVCodec_next(c_temp);
		c_index++;
	}

	



}

void Sysinfo::OnSelchangeSysinfoTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	//�ѵ�ǰ��ҳ����������
	pDialog[m_CurSelTab]->ShowWindow(SW_HIDE);
	//�õ��µ�ҳ������
	m_CurSelTab = m_sysinfotab.GetCurSel();
	//���µ�ҳ����ʾ����
	pDialog[m_CurSelTab]->ShowWindow(SW_SHOW);
	*pResult = 0;
}
