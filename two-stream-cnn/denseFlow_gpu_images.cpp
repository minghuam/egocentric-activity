#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/gpu/gpu.hpp"

#include <iostream>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <string>
#include <cstring>

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>

using namespace cv;
using namespace cv::gpu;

static void convertFlowToImage(const Mat &flow_x, const Mat &flow_y, Mat &img_x, Mat &img_y,
       double lowerBound, double higherBound) {
	#define CAST(v, L, H) ((v) > (H) ? 255 : (v) < (L) ? 0 : cvRound(255*((v) - (L))/((H)-(L))))
	for (int i = 0; i < flow_x.rows; ++i) {
		for (int j = 0; j < flow_y.cols; ++j) {
			float x = flow_x.at<float>(i,j);
			float y = flow_y.at<float>(i,j);
			img_x.at<uchar>(i,j) = CAST(x, lowerBound, higherBound);
			img_y.at<uchar>(i,j) = CAST(y, lowerBound, higherBound);
		}
	}
	#undef CAST
}

static void drawOptFlowMap(const Mat& flow, Mat& cflowmap, int step,double, const Scalar& color){
    for(int y = 0; y < cflowmap.rows; y += step)
        for(int x = 0; x < cflowmap.cols; x += step)
        {
            const Point2f& fxy = flow.at<Point2f>(y, x);
            line(cflowmap, Point(x,y), Point(cvRound(x+fxy.x), cvRound(y+fxy.y)),
                 color);
            circle(cflowmap, Point(x,y), 2, color, -1);
        }
}

int main(int argc, char** argv){
	// IO operation
	const char* keys =
		{
			"{ f  | vidFile      | ex2.avi | filename of video }"
			"{ x  | xFlowFile    | flow_x | filename of flow x component }"
			"{ y  | yFlowFile    | flow_y | filename of flow x component }"
			"{ i  | imgFile      | flow_i | filename of flow image}"
			"{ b  | bound | 15 | specify the maximum of optical flow}"
			"{ t  | type | 0 | specify the optical flow algorithm }"
			"{ d  | device_id    | 0  | set gpu id}"
			"{ s  | step  | 1 | specify the step for frame sampling}"
		};

	CommandLineParser cmd(argc, argv, keys);
	string vidFile = cmd.get<string>("vidFile");
	string xFlowFile = cmd.get<string>("xFlowFile");
	string yFlowFile = cmd.get<string>("yFlowFile");
	string imgFile = cmd.get<string>("imgFile");
	int bound = cmd.get<int>("bound");
    int type  = cmd.get<int>("type");
    int device_id = cmd.get<int>("device_id");
    int step = cmd.get<int>("step");

	/*VideoCapture capture(vidFile);
	if(!capture.isOpened()) {
		printf("Could not initialize capturing..\n");
		return -1;
	}*/

	DIR  *dpimg;
	struct dirent  *dirpimg;
	dpimg = opendir(vidFile.c_str());

	int frame_num = 0;
	Mat image, prev_image, prev_grey, grey, frame, flow_x, flow_y;
	GpuMat frame_0, frame_1, flow_u, flow_v;

	setDevice(device_id);
	FarnebackOpticalFlow alg_farn;
	OpticalFlowDual_TVL1_GPU alg_tvl1;
	BroxOpticalFlow alg_brox(0.197f, 50.0f, 0.8f, 10, 77, 10);

	string old_imgname = "";
	string imgfile1, imgfile2;
	Mat frame0;  //= imread(argv[1], IMREAD_GRAYSCALE);
    Mat frame1;  //= imread(argv[2], IMREAD_GRAYSCALE);
	while ((dirpimg = readdir( dpimg ))) {
		string imgname = dirpimg->d_name;
		if(strcmp(imgname.c_str(), ".") ==0  || strcmp(imgname.c_str(), "..") ==0)
            continue;
        int len = imgname.size();
        if(!(imgname[len - 3] == 'j' && imgname[len - 2] == 'p' && imgname[len - 1] == 'g' ))
        {
            continue;
        }
        if (old_imgname.size() == 0)
        {
            old_imgname = imgname;
            imgfile1 = vidFile + "/" + old_imgname;
            frame0 = imread(imgfile1.c_str(), IMREAD_GRAYSCALE);
            continue;
        }
        imgfile1 = vidFile + "/" + old_imgname;
        imgfile2 = vidFile + "/" + imgname;
        frame1 = imread(imgfile2.c_str(), IMREAD_GRAYSCALE);

		frame_0.upload(frame0);
		frame_1.upload(frame1);


        // GPU optical flow
		switch(type){
		case 0:
			alg_farn(frame_0,frame_1,flow_u,flow_v);
			break;
		case 1:
			alg_tvl1(frame_0,frame_1,flow_u,flow_v);
			break;
		case 2:
			GpuMat d_frame0f, d_frame1f;
	        frame_0.convertTo(d_frame0f, CV_32F, 1.0 / 255.0);
	        frame_1.convertTo(d_frame1f, CV_32F, 1.0 / 255.0);
			alg_brox(d_frame0f, d_frame1f, flow_u,flow_v);
			break;
		}

		flow_u.download(flow_x);
		flow_v.download(flow_y);

		// Output optical flow
		Mat imgX(flow_x.size(),CV_8UC1);
		Mat imgY(flow_y.size(),CV_8UC1);
		convertFlowToImage(flow_x, flow_y, imgX, imgY, -bound, bound);
		char tmp[20];
		sprintf(tmp,"%04d",int(frame_num));
		string stmp = tmp;

		Mat imgX_, imgY_, image_;
		imgX_ = imgX;
		imgY_ = imgY;
		// resize(imgX,imgX_,cv::Size(340,256));
		// resize(imgY,imgY_,cv::Size(340,256));
		// resize(image,image_,cv::Size(340,256));

		string tx_save = stmp + "_x.jpg";
		string ty_save = stmp + "_y.jpg";

		imwrite(xFlowFile + tx_save, imgX_);
		imwrite(xFlowFile + ty_save, imgY_);
		// imwrite(imgFile + tmp, image_);

		frame_num = frame_num + 1;

        frame0 = frame1;
        old_imgname = imgname;
	}
	return 0;
}
