#include <stdio.h>
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/utility.hpp>
#include "common.h"

using namespace cv;
using namespace std;

int main( int argc, char ** argv) {
    const char * imagename = argc > 1 ? argv[1] : LENA_JPG;

    Ptr<IplImage> iplimg(cvLoadImage(imagename));
    if (!iplimg) {
        cout << "can't load image" << imagename << endl;
        return -1;
    }

    Mat img = cv::cvarrToMat(iplimg);
    if (img.empty()) return -1;

    Mat img_yuv;
    cvtColor(img, img_yuv, COLOR_BGR2YCrCb);
    vector<Mat> planes;
    split(img_yuv, planes);


    for (int y = 0; y < img_yuv.rows; ++y) {
        for (int x = 0; x < img_yuv.cols; ++x) {
#if 0 //negtive
            planes[0].at<uchar>(y, x) = 255 - planes[0].at<uchar>(y, x);  // negtive
            planes[1].at<uchar>(y, x) = 255 - planes[1].at<uchar>(y, x);
            planes[2].at<uchar>(y, x) = 255 - planes[2].at<uchar>(y, x);
#else //garyscale
            planes[1].at<uchar>(y, x) = 128;
            planes[2].at<uchar>(y, x) = 128;
#endif
        }
    }

    merge(planes, img_yuv);
    cvtColor(img_yuv, img, COLOR_YCrCb2BGR);

    namedWindow("image with gray", WINDOW_AUTOSIZE);

#if 0
    cvShotImage("image with gray", iplimg);
#else
    imshow("image with gray", img);
#endif
    waitKey();
    return 0;
}
