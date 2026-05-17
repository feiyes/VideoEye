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
#include "Rawanalysis.h"
#include "afxdialogex.h"


// Dfanalysispic 对话框

IMPLEMENT_DYNAMIC(Rawanalysis, CDialogEx)

Rawanalysis::Rawanalysis(CWnd* pParent /*=NULL*/)
	: CDialogEx(Rawanalysis::IDD, pParent)
{

	m_rawanalysismethod = 0;
	m_rawanalysisautointerframenum = 0;
	m_rawanalysiscolorhbin = 0;
	m_rawanalysiscolorsbin = 0;
	m_rawanalysiscontourthres = 0;
	m_rawanalysiscannythres1 = 0;
	m_rawanalysiscannythres2 = 0;
	m_rawanalysisfacexmlurl = _T("");
}

Rawanalysis::~Rawanalysis()
{
}

void Rawanalysis::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RAWANALYSIS_METHOD, m_rawanalysismethod);
	DDX_Text(pDX, IDC_RAWANALYSIS_AUTO_INTERFRAMENUM, m_rawanalysisautointerframenum);
	DDX_Control(pDX, IDC_RAWANALYSIS_AUTO, m_rawanalysisauto);
	DDX_Control(pDX, IDC_RAWANALYSIS_OUTPICFOLDER, m_rawanalysisoutpicfolder);
	DDX_Control(pDX, IDC_RAWANALYSIS_OUTPICFOLDER_URL, m_rawanalysisoutpicfolderurl);
	DDX_Text(pDX, IDC_RAWANALYSIS_COLOR_HBIN, m_rawanalysiscolorhbin);
	DDX_Text(pDX, IDC_RAWANALYSIS_COLOR_SBIN, m_rawanalysiscolorsbin);
	DDX_Text(pDX, IDC_RAWANALYSIS_CONTOUR_THRES, m_rawanalysiscontourthres);
	DDX_Text(pDX, IDC_RAWANALYSIS_CANNY_THRES1, m_rawanalysiscannythres1);
	DDX_Text(pDX, IDC_RAWANALYSIS_CANNY_THRES2, m_rawanalysiscannythres2);
	DDX_Text(pDX, IDC_RAWANALYSIS_FACE_XMLURL, m_rawanalysisfacexmlurl);
}


BEGIN_MESSAGE_MAP(Rawanalysis, CDialogEx)
	ON_BN_CLICKED(IDC_RAWANALYSIS_OPEN, &Rawanalysis::OnBnClickedRawanalysisOpen)
	ON_BN_CLICKED(IDCANCEL, &Rawanalysis::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_RAWANALYSIS_AUTO, &Rawanalysis::OnBnClickedRawanalysisAuto)
	ON_EN_KILLFOCUS(IDC_RAWANALYSIS_AUTO_INTERFRAMENUM, &Rawanalysis::OnKillfocusRawanalysisAutoInterframenum)
	ON_BN_CLICKED(IDC_RAWANALYSIS_OUTPICFOLDER, &Rawanalysis::OnClickedRawanalysisOutpicfolder)
END_MESSAGE_MAP()


// Dfanalysispic ��Ϣ��������
BOOL Rawanalysis::OnInitDialog(){
	CDialogEx::OnInitDialog();
	m_rawanalysismethod=0;
	m_rawanalysisautointerframenum=25;
	y_width=0;
	y_height=0;
	UpdateData(FALSE);
	GetDlgItem(IDC_RAWANALYSIS_AUTO_INTERFRAMENUM)->EnableWindow(FALSE);
	GetDlgItem(IDC_RAWANALYSIS_AUTO_OK)->EnableWindow(FALSE);
	y_data=NULL;
	u_data=NULL;
	v_data=NULL;
	//
	// m_rawanalysisoutpicfolderurl.EnableFolderBrowseButton(); // Not available for CEdit
	
	TCHAR realpath[MAX_URL_LENGTH]={0};
	//�����ļ�·��
	GetCurrentDirectory(MAX_URL_LENGTH,realpath);
	CString realpath1(realpath);
	realpath1.Append(_T("\\rawanalysispic"));

	m_rawanalysisoutpicfolderurl.SetWindowText(realpath1);


	m_rawanalysisoutpicfolderurl.EnableWindow(FALSE);
	//----------------------
	m_rawanalysiscolorhbin=16;
	m_rawanalysiscolorsbin=8;
	m_rawanalysiscontourthres=60;
	m_rawanalysiscannythres1=50;
	m_rawanalysiscannythres2=150;
	m_rawanalysisfacexmlurl.Format(_T("haarcascade_frontalface_alt2.xml"));
	UpdateData(FALSE);
	//----------------------
	return TRUE;
}

void Rawanalysis::OnBnClickedRawanalysisOpen()
{
	//����ڴ���yuv�����Ƿ����?
	//��������ʣ�ͨ��catch����������
	

	try{
		YUVtoIpl1();
	}catch(...){
		CString resloader;
		resloader.LoadString(IDS_MSGBOX_NODATA);
		AfxMessageBox(resloader);
		return;
	}
	//����ļ���·���Ƿ����
	if(m_rawanalysisoutpicfolder.GetCheck()==TRUE){
		CString folder_url;
		m_rawanalysisoutpicfolderurl.GetWindowText(folder_url);
		CreateDirectory(folder_url,NULL);
	}

	//-------

	UpdateData(TRUE);


	switch(m_rawanalysismethod){
	case 0:
		Color_Histogram();
		break;
	case 1:
		Canny();
		break;
	case 2:
		Contour();
		break;
	case 3:
		DFT();
		break;
	case 4:
		face_detect();
		break;
	case 5:
		show_color_component(SHOW_R_COMP);
		break;
	case 6:
		show_color_component(SHOW_G_COMP);
		break;
	case 7:
		show_color_component(SHOW_B_COMP);
		break;
	case 8:
		show_color_component(SHOW_Y_COMP);
		break;
	case 9:
		show_color_component(SHOW_U_COMP);
		break;
	case 10:
		show_color_component(SHOW_V_COMP);
		break;
	default:
		Color_Histogram();
		break;
	}
}


// Rearrange the quadrants of Fourier image so that the origin is at
// the image center
// src & dst arrays of equal size & type
void Rawanalysis::cvShiftDFT(cv::Mat& src_arr, cv::Mat& dst_arr)
{
	cv::Mat tmp;
	cv::Mat q1, q2, q3, q4;
	cv::Mat d1, d2, d3, d4;

	cv::Size size = src_arr.size();
	cv::Size dst_size = dst_arr.size();
	int cx, cy;

	if(dst_size.width != size.width || 
		dst_size.height != size.height){
			throw std::runtime_error("Source and Destination arrays must have equal sizes");
	}

	if(src_arr.data != dst_arr.data){
		tmp.create(size.height/2, size.width/2, src_arr.type());
	}

	cx = size.width/2;
	cy = size.height/2; // image center

	q1 = src_arr(cv::Rect(0, 0, cx, cy));
	q2 = src_arr(cv::Rect(cx, 0, cx, cy));
	q3 = src_arr(cv::Rect(cx, cy, cx, cy));
	q4 = src_arr(cv::Rect(0, cy, cx, cy));
	d1 = dst_arr(cv::Rect(0, 0, cx, cy));
	d2 = dst_arr(cv::Rect(cx, 0, cx, cy));
	d3 = dst_arr(cv::Rect(cx, cy, cx, cy));
	d4 = dst_arr(cv::Rect(0, cy, cx, cy));

	if(src_arr.data != dst_arr.data){
		if( q1.type() != d1.type() ){
			throw std::runtime_error("Source and Destination arrays must have the same format");
		}
		q3.copyTo(d1);
		q4.copyTo(d2);
		q1.copyTo(d3);
		q2.copyTo(d4);
	}
	else{
		q3.copyTo(tmp);
		q1.copyTo(q3);
		tmp.copyTo(q1);
		q4.copyTo(tmp);
		q2.copyTo(q4);
		tmp.copyTo(q2);
	}
}

int Rawanalysis::DFT()
{

	CString folder_url,folder_url1,folder_url2,pic_name,pic_name1,pic_name2;
	m_rawanalysisoutpicfolderurl.GetWindowText(folder_url);
	m_rawanalysisoutpicfolderurl.GetWindowText(folder_url1);
	m_rawanalysisoutpicfolderurl.GetWindowText(folder_url2);

	cv::Mat im;
	cv::Mat realInput;
	cv::Mat imaginaryInput;
	cv::Mat complexInput;
	int dft_M, dft_N;
	cv::Mat dft_A;
	cv::Mat image_Re;
	cv::Mat image_Im;
	double m, M;
	//-------------
	cv::cvtColor(result_image, im, cv::COLOR_BGR2GRAY);
	//-------------
	if( im.empty() )
		return -1;

	realInput.create(im.size(), CV_64FC1);
	imaginaryInput.create(im.size(), CV_64FC1);
	complexInput.create(im.size(), CV_64FC2);

	im.convertTo(realInput, CV_64FC1);
	imaginaryInput.setTo(cv::Scalar::all(0));
	std::vector<cv::Mat> planes;
	planes.push_back(realInput);
	planes.push_back(imaginaryInput);
	cv::merge(planes, complexInput);

	dft_M = cv::getOptimalDFTSize( im.rows - 1 );
	dft_N = cv::getOptimalDFTSize( im.cols - 1 );

	dft_A.create( dft_M, dft_N, CV_64FC2 );
	image_Re.create( dft_M, dft_N, CV_64FC1 );
	image_Im.create( dft_M, dft_N, CV_64FC1 );

	// copy A to dft_A and pad dft_A with zeros
	cv::Mat tmp = dft_A(cv::Rect(0, 0, im.cols, im.rows));
	complexInput.copyTo(tmp);
	if( dft_A.cols > im.cols )
	{
		cv::Mat tmp2 = dft_A(cv::Rect(im.cols, 0, dft_A.cols - im.cols, im.rows));
		tmp2.setTo(cv::Scalar::all(0));
	}

	// no need to pad bottom part of dft_A with zeros because of
	// use nonzero_rows parameter in cvDFT() call below

	cv::dft( dft_A, dft_A, 0, complexInput.rows );

	cv::namedWindow("win", 0);
	cv::namedWindow("magnitude", 0);
	cv::namedWindow("im", 0);
	cv::imshow("win", im);

	// Split Fourier in real and imaginary parts
	cv::split( dft_A, planes );
	image_Re = planes[0];
	image_Im = planes[1];

	// Compute the magnitude of the spectrum Mag = sqrt(Re^2 + Im^2)
	cv::pow( image_Re, 2.0, image_Re );
	cv::pow( image_Im, 2.0, image_Im );
	cv::add( image_Re, image_Im, image_Re );
	cv::pow( image_Re, 0.5, image_Re );

	// Compute log(1 + Mag)
	cv::add( image_Re, cv::Scalar::all(1.0), image_Re ); // 1 + Mag
	cv::log( image_Re, image_Re ); // log(1 + Mag)


	// Rearrange the quadrants of Fourier image so that the origin is at
	// the image center
	cvShiftDFT( image_Re, image_Re );

	cv::minMaxLoc(image_Re, &m, &M, NULL, NULL);
	image_Re.convertTo(image_Re, CV_64FC1, 1.0/(M-m), -m/(M-m));
	cv::imshow("magnitude", image_Re);
	cv::imshow("im", image_Im);

	if(m_rawanalysisoutpicfolder.GetCheck()==TRUE){
		pic_name.Format(_T("pic_%d.jpg"),frame_index);
		pic_name1.Format(_T("pic_%d_dft_magnitude.jpg"),frame_index);
		pic_name2.Format(_T("pic_%d_dft_im.jpg"),frame_index);
		folder_url.AppendFormat(_T("\\%s"),pic_name);
		folder_url1.AppendFormat(_T("\\%s"),pic_name1);
		folder_url2.AppendFormat(_T("\\%s"),pic_name2);
#ifdef _UNICODE
		USES_CONVERSION;
		cv::imwrite(std::string(W2A(folder_url)), im);
		cv::imwrite(std::string(W2A(folder_url1)), image_Re);
		cv::imwrite(std::string(W2A(folder_url2)), image_Im);
#else
		cv::imwrite(std::string(folder_url.GetString()), im);
		cv::imwrite(std::string(folder_url1.GetString()), image_Re);
		cv::imwrite(std::string(folder_url2.GetString()), image_Im);
#endif
	}


	cv::waitKey(-1);
	return 0;
}

int Rawanalysis::Color_Histogram()
{
	CString folder_url,folder_url1,pic_name,pic_name1;
	m_rawanalysisoutpicfolderurl.GetWindowText(folder_url);
	m_rawanalysisoutpicfolderurl.GetWindowText(folder_url1);

	cv::Mat src = result_image.clone();

	cv::Mat hsv;
	std::vector<cv::Mat> hsv_planes;
	hsv_planes.resize(3);

	/** H ��������Ϊ16���ȼ���S��������Ϊ8���ȼ� */
	int h_bins = m_rawanalysiscolorhbin, s_bins = m_rawanalysiscolorsbin;
	int hist_size[] = {h_bins, s_bins};

	/** H �����ı仯��Χ */
	float h_ranges[] = { 0, 180 }; 

	/** S �����ı仯��Χ*/
	float s_ranges[] = { 0, 255 };
	const float* ranges[] = { h_ranges, s_ranges };

	/** ����ͼ��ת����HSV��ɫ�ռ� */
	cv::cvtColor( src, hsv, cv::COLOR_BGR2HSV );
	cv::split( hsv, hsv_planes );

	std::vector<cv::Mat> planes;
	planes.push_back(hsv_planes[0]);
	planes.push_back(hsv_planes[1]);

	cv::Mat hist;
	int channels[] = {0, 0};
	cv::calcHist(&planes[0], 2, channels, cv::noArray(), hist, 2, hist_size, ranges);

	/** ��ȡֱ��ͼͳ�Ƶ����ֵ�����ڶ�̬��ʾֱ���?*/
	double max_value;
	cv::minMaxLoc(hist, NULL, &max_value, NULL, NULL);


	/** ����ֱ��ͼ��ʾͼ�� */
	int height = 240;
	int width = (h_bins*s_bins*6);
	cv::Mat hist_img = cv::Mat::zeros(height, width, CV_8UC3);

	/** ��������HSV��RGB��ɫת������ʱ��λͼ�� */
	cv::Mat hsv_color(1, 1, CV_8UC3);
	cv::Mat rgb_color(1, 1, CV_8UC3);
	int bin_w = width / (h_bins * s_bins);
	for(int h = 0; h < h_bins; h++)
	{
		for(int s = 0; s < s_bins; s++)
		{
			int i = h*s_bins + s;
			/** ���ֱ��ͼ�е�ͳ�ƴ�����������ʾ��ͼ���еĸ߶�?*/
			float bin_val = hist.at<float>(h, s);
			int intensity = cvRound(bin_val*height/max_value);

			/** ��õ�ǰֱ��ͼ��������ɫ��ת����RGB���ڻ��� */
			hsv_color.at<cv::Vec3b>(0, 0) = cv::Vec3b(static_cast<uchar>(h*180.f / h_bins), static_cast<uchar>(s*255.f/s_bins), 255);
			cv::cvtColor(hsv_color, rgb_color, cv::COLOR_HSV2BGR);
			cv::Vec3b color = rgb_color.at<cv::Vec3b>(0, 0);

			cv::rectangle( hist_img, cv::Point(i*bin_w, height),
				cv::Point((i+1)*bin_w, height - intensity),
				cv::Scalar(color[0], color[1], color[2]), -1, 8, 0 );
		}
	}

	cv::namedWindow( "Source", 1 );
	cv::imshow( "Source", src );

	cv::namedWindow( "H-S Histogram", 1 );
	cv::imshow( "H-S Histogram", hist_img );

	if(m_rawanalysisoutpicfolder.GetCheck()==TRUE){
		pic_name.Format(_T("pic_%d.jpg"),frame_index);
		pic_name1.Format(_T("pic_%d_histogram.jpg"),frame_index);
		folder_url.AppendFormat(_T("\\%s"),pic_name);
		folder_url1.AppendFormat(_T("\\%s"),pic_name1);
#ifdef _UNICODE
		USES_CONVERSION;
		cv::imwrite(std::string(W2A(folder_url)), src);
		cv::imwrite(std::string(W2A(folder_url1)), hist_img);
#else
		cv::imwrite(std::string(folder_url.GetString()), src);
		cv::imwrite(std::string(folder_url1.GetString()), hist_img);
#endif
	}


	cv::waitKey(0);
	return 0;
}

int Rawanalysis::Canny(){

	CString folder_url,folder_url1,pic_name,pic_name1;
	m_rawanalysisoutpicfolderurl.GetWindowText(folder_url);
	m_rawanalysisoutpicfolderurl.GetWindowText(folder_url1);

	//����ͼ��ǿ��ת��ΪGray
	cv::Mat pImg;
	cv::cvtColor(result_image, pImg, cv::COLOR_BGR2GRAY);
	
	//Ϊcanny��Եͼ������ռ�?
	cv::Mat pCannyImg;
	//canny��Ե���?
	cv::Canny(pImg, pCannyImg, m_rawanalysiscannythres1, m_rawanalysiscannythres2, 3);

	//��������
	cv::namedWindow("src", 1);
	cv::namedWindow("canny", 1);


	//��ʾͼ��
	cv::imshow( "src", pImg );
	cv::imshow( "canny", pCannyImg );

	if(m_rawanalysisoutpicfolder.GetCheck()==TRUE){
		pic_name.Format(_T("pic_%d.jpg"),frame_index);
		pic_name1.Format(_T("pic_%d_canny.jpg"),frame_index);
		folder_url.AppendFormat(_T("\\%s"),pic_name);
		folder_url1.AppendFormat(_T("\\%s"),pic_name1);
#ifdef _UNICODE
		USES_CONVERSION;
		cv::imwrite(std::string(W2A(folder_url)), pImg);
		cv::imwrite(std::string(W2A(folder_url1)), pCannyImg);
#else
		cv::imwrite(std::string(folder_url.GetString()), pImg);
		cv::imwrite(std::string(folder_url1.GetString()), pCannyImg);
#endif
	}
	
	cv::waitKey(0); //�ȴ�����

	//���ٴ���
	cv::destroyWindow( "src" );
	cv::destroyWindow( "canny" );

	return 0;
}


int Rawanalysis::face_detect()
{
	CString folder_url, pic_name;
	m_rawanalysisoutpicfolderurl.GetWindowText(folder_url);
#ifdef _UNICODE
	USES_CONVERSION;
	cascade.load(std::string(W2A(m_rawanalysisfacexmlurl)));
#else
	cascade.load(std::string(m_rawanalysisfacexmlurl.GetString()));
#endif
    

    if( cascade.empty() )
    {
        AfxMessageBox(_T("Error: Can't load classifier cascade"));
        return -1;
    }
 
    cv::namedWindow( "result", 1 );
	//-------------
	UpdateData(TRUE);

	cv::Mat image = result_image.clone();
	
	//-------------
    

    if( !image.empty() )
    {
        detect_and_draw( image );

		if(m_rawanalysisoutpicfolder.GetCheck()==TRUE){
			pic_name.Format(_T("pic_%d_facedetect.jpg"),frame_index);
			folder_url.AppendFormat(_T("\\%s"),pic_name);
#ifdef _UNICODE
			USES_CONVERSION;
			cv::imwrite(std::string(W2A(folder_url)), image);
#else
			cv::imwrite(std::string(folder_url.GetString()), image);
#endif
		}

        cv::waitKey(0);
    }
 
    cv::destroyWindow("result");
 
    return 0;
}
 
void Rawanalysis::detect_and_draw(cv::Mat& img )
{
    static cv::Scalar colors[] = 
    {
        cv::Scalar(0, 0, 255),
        cv::Scalar(0, 128, 255),
        cv::Scalar(0, 255, 255),
        cv::Scalar(0, 255, 0),
        cv::Scalar(255, 128, 0),
        cv::Scalar(255, 255, 0),
        cv::Scalar(255, 0, 0),
        cv::Scalar(255, 0, 255)
    };
 
    double scale = 1.3;
    cv::Mat gray;
    cv::Mat small_img;
    std::vector<cv::Rect> faces;
    int i;
 
    cv::cvtColor( img, gray, cv::COLOR_BGR2GRAY );
    cv::resize( gray, small_img, cv::Size( cvRound(img.cols/scale), cvRound(img.rows/scale) ), 0, 0, cv::INTER_LINEAR );
    cv::equalizeHist( small_img, small_img );
 
    if( !cascade.empty() )
    {
        double t = (double)cv::getTickCount();
        cascade.detectMultiScale( small_img, faces, 1.1, 2, 0/*CV_HAAR_DO_CANNY_PRUNING*/, cv::Size(30, 30) );
        t = (double)cv::getTickCount() - t;
        printf( "detection time = %gms\n", t/((double)cv::getTickFrequency()*1000.) );
        for( i = 0; i < faces.size(); i++ )
        {
            cv::Point center;
            int radius;
            center.x = cvRound((faces[i].x + faces[i].width*0.5)*scale);
            center.y = cvRound((faces[i].y + faces[i].height*0.5)*scale);
            radius = cvRound((faces[i].width + faces[i].height)*0.25*scale);
            cv::circle( img, center, radius, colors[i%8], 3, 8, 0 );
        }
    }
 
    cv::imshow( "result", img );
}

int Rawanalysis::Contour()
{
	CString folder_url,folder_url1,pic_name,pic_name1;
	m_rawanalysisoutpicfolderurl.GetWindowText(folder_url);
	m_rawanalysisoutpicfolderurl.GetWindowText(folder_url1);

	cv::Mat pImg;
	cv::Mat pContourImg;

	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	int mode = cv::RETR_CCOMP;

	cv::namedWindow("src", 1);
	cv::namedWindow("contour", 1);

	cv::cvtColor(result_image, pImg, cv::COLOR_BGR2GRAY);

	if(!pImg.empty())
	{
		cv::imshow("src", pImg);

		pContourImg.create(pImg.size(), CV_8UC3);
		cv::cvtColor(pImg, pContourImg, cv::COLOR_GRAY2BGR);

		cv::threshold(pImg, pImg, m_rawanalysiscontourthres, 255.0, cv::THRESH_BINARY);
		cv::findContours(pImg, contours, hierarchy, mode, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

		cv::drawContours(pContourImg, contours, -1, cv::Scalar(0, 0, 255), 2, 8, hierarchy, 2, cv::Point(0, 0));
		cv::imshow("contour", pContourImg);

		if(m_rawanalysisoutpicfolder.GetCheck()==TRUE){
			pic_name.Format(_T("pic_%d.jpg"),frame_index);
			pic_name1.Format(_T("pic_%d_contour.jpg"),frame_index);
			folder_url.AppendFormat(_T("\\%s"),pic_name);
			folder_url1.AppendFormat(_T("\\%s"),pic_name1);
#ifdef _UNICODE
			USES_CONVERSION;
			cv::imwrite(std::string(W2A(folder_url)), pImg);
			cv::imwrite(std::string(W2A(folder_url1)), pContourImg);
#else
			cv::imwrite(std::string(folder_url.GetString()), pImg);
			cv::imwrite(std::string(folder_url1.GetString()), pContourImg);
#endif
		}

		cv::waitKey(0);
	}
	else
	{
		cv::destroyWindow("src");
		cv::destroyWindow("contour");
		return -1;
	}

	cv::destroyWindow("src");
	cv::destroyWindow("contour");

	return 0;
}


//�򵥵ķ���
int Rawanalysis::YUVtoIpl1(){
	if(y_width == 0 || y_height == 0){
		throw 123;
	}

	cv::Mat yimg(y_height, y_width, CV_8UC1, y_data);
	cv::Mat uimg(y_height/2, y_width/2, CV_8UC1, u_data);
	cv::Mat vimg(y_height/2, y_width/2, CV_8UC1, v_data);

	cv::Mat uuimg(y_height, y_width, CV_8UC1);
	cv::Mat vvimg(y_height, y_width, CV_8UC1);

	critical_section.Lock();
	cv::resize(uimg, uuimg, cv::Size(y_width, y_height), 0, 0, cv::INTER_LINEAR);
	cv::resize(vimg, vvimg, cv::Size(y_width, y_height), 0, 0, cv::INTER_LINEAR);

	std::vector<cv::Mat> yuv_channels;
	yuv_channels.push_back(yimg);
	yuv_channels.push_back(uuimg);
	yuv_channels.push_back(vvimg);
	cv::merge(yuv_channels, yuvimage);

	cv::cvtColor(yuvimage, result_image, cv::COLOR_YCrCb2RGB);
	critical_section.Unlock();

	return 0;
}
//�Ƚϸ��ӵķ���
int Rawanalysis::YUVtoIpl2(){
	unsigned char* pRGB = NULL;

	pRGB = (unsigned char*)malloc(y_height*y_width*sizeof(unsigned char)*3);

	YUV420_C_RGB((char*)y_data, (char*)u_data, (char*)v_data, pRGB, y_height, y_width);

	result_image = cv::Mat(y_height, y_width, CV_8UC3, pRGB, y_width * 3);
	return 0;
}

void Rawanalysis::YUV420_C_RGB( char* pY,char* pU,char* pV, unsigned char* pRGB, int height, int width){

    unsigned char* pBGR = NULL;
    unsigned char R = 0;
    unsigned char G = 0;
    unsigned char B = 0;
    char Y = 0;
    char U = 0;
    char V = 0;
    double tmp = 0;
    for ( int i = 0; i < height; ++i )
    {
        for ( int j = 0; j < width; ++j )
        {
            pBGR = pRGB+ i*width*3+j*3;

            Y = *(pY+i*width+j);
            U = *pU;
            V = *pV;

            //B
            tmp = MB(Y, U, V);
            //B = (tmp > 255) ? 255 : (char)tmp;
            //B = (B<0) ? 0 : B;
			B = (unsigned char)tmp;
            //G
            tmp = MG(Y, U, V);
            //G = (tmp > 255) ? 255 : (char)tmp;
           // G = (G<0) ? 0 : G;
			G = (unsigned char)tmp;
            //R
            tmp = MR(Y, U, V);
            //R = (tmp > 255) ? 255 : (char)tmp;
            //R = (R<0) ? 0 : R;
			R = (unsigned char)tmp;


            *pBGR     = R;            
            *(pBGR+1) = G;        
            *(pBGR+2) = B;
        

            if ( i%2 == 0 && j%2 == 0)
            {
                *pU++;
				//*pV++;
            }
            else
            {
                if ( j%2 == 0 )
                {
                    *pV++ ;
                }
			}
        }
    
    }
}

void Rawanalysis::SystemClear(){
	m_rawanalysiscolorhbin=16;
	m_rawanalysiscolorsbin=8;
	m_rawanalysiscontourthres=60;
	m_rawanalysiscannythres1=50;
	m_rawanalysiscannythres2=150;
	m_rawanalysisfacexmlurl.Format(_T("haarcascade_frontalface_alt2.xml"));
	UpdateData(FALSE);
}

void Rawanalysis::OnBnClickedCancel()
{
	// TODO: �ڴ����ӿؼ�֪ͨ�����������?
	//�ر�
	SystemClear();
	//�ر����д���
	cv::destroyAllWindows();
	m_rawanalysisauto.SetCheck(0);
	//CDialogEx::OnCancel();
	ShowWindow(FALSE);
}

int Rawanalysis::show_color_component(int nameid){
	std::vector<cv::Mat> bgr_planes(3);
	std::vector<cv::Mat> yuv_planes(3);

	cv::split(result_image, bgr_planes);
	cv::split(yuvimage, yuv_planes);

	cv::Mat b_comp = bgr_planes[0];
	cv::Mat g_comp = bgr_planes[1];
	cv::Mat r_comp = bgr_planes[2];
	cv::Mat y_comp = yuv_planes[0];
	cv::Mat u_comp = yuv_planes[1];
	cv::Mat v_comp = yuv_planes[2];

	cv::Mat img_r = cv::Mat::zeros(result_image.size(), CV_8UC3);
	cv::Mat img_g = cv::Mat::zeros(result_image.size(), CV_8UC3);
	cv::Mat img_b = cv::Mat::zeros(result_image.size(), CV_8UC3);

	std::vector<cv::Mat> channels_r = {cv::Mat(), cv::Mat(), r_comp};
	std::vector<cv::Mat> channels_g = {cv::Mat(), g_comp, cv::Mat()};
	std::vector<cv::Mat> channels_b = {b_comp, cv::Mat(), cv::Mat()};

	cv::merge(channels_b, img_b);
	cv::merge(channels_g, img_g);
	cv::merge(channels_r, img_r);

	CString folder_url, pic_name;
	m_rawanalysisoutpicfolderurl.GetWindowText(folder_url);

	switch(nameid){
	case SHOW_R_COMP:{
		cv::namedWindow("Image", 1);
		cv::imshow("Image", img_r);
		if(m_rawanalysisoutpicfolder.GetCheck()==TRUE){
			pic_name.Format(_T("pic_%d_r.jpg"), frame_index);
			folder_url.AppendFormat(_T("\\%s"), pic_name);
#ifdef _UNICODE
			USES_CONVERSION;
			cv::imwrite(std::string(W2A(folder_url)), img_r);
#else
			cv::imwrite(std::string(folder_url.GetString()), img_r);
#endif
		}
		break;
					 }
	case SHOW_G_COMP:{
		cv::namedWindow("Image", 1);
		cv::imshow("Image", img_g);
		if(m_rawanalysisoutpicfolder.GetCheck()==TRUE){
			pic_name.Format(_T("pic_%d_g.jpg"), frame_index);
			folder_url.AppendFormat(_T("\\%s"), pic_name);
#ifdef _UNICODE
			USES_CONVERSION;
			cv::imwrite(std::string(W2A(folder_url)), img_g);
#else
			cv::imwrite(std::string(folder_url.GetString()), img_g);
#endif
		}
		break;
					 }
	case SHOW_B_COMP:{
		cv::namedWindow("Image", 1);
		cv::imshow("Image", img_b);
		if(m_rawanalysisoutpicfolder.GetCheck()==TRUE){
			pic_name.Format(_T("pic_%d_b.jpg"), frame_index);
			folder_url.AppendFormat(_T("\\%s"), pic_name);
#ifdef _UNICODE
			USES_CONVERSION;
			cv::imwrite(std::string(W2A(folder_url)), img_b);
#else
			cv::imwrite(std::string(folder_url.GetString()), img_b);
#endif
		}
		break;
					 }
	case SHOW_Y_COMP:{
		cv::namedWindow("Image", 1);
		cv::imshow("Image", y_comp);
		if(m_rawanalysisoutpicfolder.GetCheck()==TRUE){
			pic_name.Format(_T("pic_%d_y.jpg"), frame_index);
			folder_url.AppendFormat(_T("\\%s"), pic_name);
#ifdef _UNICODE
			USES_CONVERSION;
			cv::imwrite(std::string(W2A(folder_url)), y_comp);
#else
			cv::imwrite(std::string(folder_url.GetString()), y_comp);
#endif
		}
		break;
					 }
	case SHOW_U_COMP:{
		cv::namedWindow("Image", 1);
		cv::imshow("Image", u_comp);
		if(m_rawanalysisoutpicfolder.GetCheck()==TRUE){
			pic_name.Format(_T("pic_%d_u.jpg"), frame_index);
			folder_url.AppendFormat(_T("\\%s"), pic_name);
#ifdef _UNICODE
			USES_CONVERSION;
			cv::imwrite(std::string(W2A(folder_url)), u_comp);
#else
			cv::imwrite(std::string(folder_url.GetString()), u_comp);
#endif
		}
		break;
					 }
	case SHOW_V_COMP:{
		cv::namedWindow("Image", 1);
		cv::imshow("Image", v_comp);
		if(m_rawanalysisoutpicfolder.GetCheck()==TRUE){
			pic_name.Format(_T("pic_%d_v.jpg"), frame_index);
			folder_url.AppendFormat(_T("\\%s"), pic_name);
#ifdef _UNICODE
			USES_CONVERSION;
			cv::imwrite(std::string(W2A(folder_url)), v_comp);
#else
			cv::imwrite(std::string(folder_url.GetString()), v_comp);
#endif
		}
		break;
					 }
	}

	return 0;
}

void Rawanalysis::OnBnClickedRawanalysisAuto()
{
	if(m_rawanalysisauto.GetCheck()==1){
		GetDlgItem(IDC_RAWANALYSIS_AUTO_INTERFRAMENUM)->EnableWindow(TRUE);
		GetDlgItem(IDC_RAWANALYSIS_AUTO_OK)->EnableWindow(TRUE);
	}else{
		GetDlgItem(IDC_RAWANALYSIS_AUTO_INTERFRAMENUM)->EnableWindow(FALSE);
		GetDlgItem(IDC_RAWANALYSIS_AUTO_OK)->EnableWindow(FALSE);
	}
}

void Rawanalysis::OnKillfocusRawanalysisAutoInterframenum()
{
	UpdateData(TRUE);
}


void Rawanalysis::OnClickedRawanalysisOutpicfolder()
{
	if(m_rawanalysisoutpicfolder.GetCheck()==FALSE){
		m_rawanalysisoutpicfolderurl.EnableWindow(FALSE);
	}else{
		m_rawanalysisoutpicfolderurl.EnableWindow(TRUE);
	}
}
