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
#include "VideoEyeDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CVideoEyeApp

BEGIN_MESSAGE_MAP(CVideoEyeApp, CWinApp)
ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CVideoEyeApp 构造

CVideoEyeApp::CVideoEyeApp()
{
m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

// TODO: 在此处添加构造代码，
// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CVideoEyeApp 对象

CVideoEyeApp theApp;


// CVideoEyeApp 初始化

BOOL CVideoEyeApp::InitInstance()
{
// 如果一个在 Windows XP 上的应用程序清单指定了
// 使用 ComCtl32.dll 版本 6 或更高版本以启用可视化样式，
//则需要 InitCommonControlsEx()，否则无法创建窗口
INITCOMMONCONTROLSEX InitCtrls;
InitCtrls.dwSize = sizeof(InitCtrls);
// 将此设置为包含应用程序所需的
// 所有公共控件类
InitCtrls.dwICC = ICC_WIN95_CLASSES;
InitCommonControlsEx(&InitCtrls);

CWinApp::InitInstance();


AfxEnableControlContainer();

// 创建 shell 管理器，以便拖放操作
// 以及 shell 树形视图控件和 shell 列表视图控件
CShellManager *pShellManager = new CShellManager;

// 标准初始化
// 如果未使用这些功能，建议删除
// 以减小最终可执行文件的大小
// 需要重新设置初始化
// 设置用于存储设置的注册表项
// TODO: 应适当修改该字符串
// 改为公司或组织名
SetRegistryKey(_T("VideoEye Application"));

//设置为英文界面

//SetThreadUILanguage(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));
LoadLaguage();

CVideoEyeDlg dlg;
m_pMainWnd = &dlg;
INT_PTR nResponse = dlg.DoModal();
if (nResponse == IDOK)
{
// TODO: 在此放置处理何时
// 对话框被确认关闭的代码
}
else if (nResponse == IDCANCEL)
{
// TODO: 在此放置处理何时
// 对话框被取消关闭的代码
}

// 删除之前创建的 shell 管理器
if (pShellManager != NULL)
{
delete pShellManager;
}

// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
//  而不是启动应用程序的消息泵
return FALSE;
}

void CVideoEyeApp::LoadLaguage()
{
//获取文件路径
char conf_path[300]={0};
//获取exe的路径
GetModuleFileNameA(NULL,(LPSTR)conf_path,300);//
//获取exe目录路径
strrchr( conf_path, '\\')[0]= '\0';//
strcat(conf_path,"\\configure.ini");
//存储配置的值
char conf_val[300]={0};

if((_access(conf_path, 0 )) == -1 ){  
//配置文件不存在，直接返回
}else{
GetPrivateProfileStringA("Settings","language",NULL,conf_val,300,conf_path);
if(strcmp(conf_val,"Chinese")==0){
SetThreadUILanguage(MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), SORT_DEFAULT));
}else if(strcmp(conf_val,"English")==0){
SetThreadUILanguage(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));
}

}
return;
}