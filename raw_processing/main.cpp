#include <iostream>
#include "raw_processing.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/utility.hpp>
#include "opencv2/photo.hpp"

using namespace std;

#define RAW_WIDTH_DEFAULT 5312
#define RAW_HEIGHT_DEFAULT 3984
#define RAW_BITDEPTHS_DEFAULT 12
#define RAW_FILE_NAME "./res/raw1.buffer"
//#define RAW_FILE_NAME "./res/RawCapture0_20160526_163445.raw"
int main(int argc, char**argv) {
    int height = RAW_HEIGHT_DEFAULT;
    int width = RAW_WIDTH_DEFAULT;
    const char *file = RAW_FILE_NAME;
    int bitdepths = RAW_BITDEPTHS_DEFAULT;
    //int pattern = bayer_buffer::BAYER_RGrGbB;
    int pattern = bayer_buffer::BAYER_GrRBGb;
    //int pattern = bayer_buffer::BAYER_BGbGrR;
    //int pattern = bayer_buffer::BAYER_GbBRGr;
    raw_processing & raw = raw_processing::getInstance();
    raw.selectRawFile(file,
            height,
            width,
            bitdepths);
    const Mat & bayerMat = raw.getBayerMat();
    cout <<"finish the raw parser" << endl;
    Mat bayer8(height, width, CV_8UC1);
    for (int row = 0; row < bayerMat.rows; ++row) {
        for (int col = 0; col < bayerMat.cols; ++col) {
            bayer8.at<uchar>(row, col) = (uchar)(bayerMat.at<ushort>(row, col)>>4  & 0xFF);
        }
    }

    Mat bayer16(height, width, CV_16UC1);
    for (int row = 0; row < bayerMat.rows; ++row) {
        for (int col = 0; col < bayerMat.cols; ++col) {
            bayer16.at<ushort>(row, col) = (ushort)(bayerMat.at<ushort>(row, col) << 6);
        }
    }
#if 1
    FILE *fp8 = fopen("res/bayer8.raw", "wb");
    fwrite(bayer8.data, 1, height * width * sizeof(uchar), fp8);
    fclose(fp8);

    FILE *fp16 = fopen("res/bayer16.raw", "wb");
    fwrite(bayer16.data, 1, height * width * sizeof(ushort), fp16);
    fclose(fp16);
#endif
    //raw.applyWBGains(bayer8, 1900 , 1024, 1700, pattern);
    raw.deNoise(bayer8, noise_reduction::AVERAGE_DENOISE, pattern);
#if 1
    Mat imgGray(height, width, CV_8UC1);
    switch (pattern) {
        case bayer_buffer::BAYER_RGrGbB:
            cvtColor(bayer8, imgGray, COLOR_BayerRG2GRAY);
            break;
        case  bayer_buffer::BAYER_GrRBGb:
            cvtColor(bayer8, imgGray, COLOR_BayerGR2GRAY);
            break;
        case bayer_buffer::BAYER_BGbGrR:
            cvtColor(bayer8, imgGray, COLOR_BayerBG2GRAY);
            break;
        case  bayer_buffer::BAYER_GbBRGr:
            cvtColor(bayer8, imgGray, COLOR_BayerGB2GRAY);
            break;
        default:
            break;
    }
    imwrite("res/gray.jpg", imgGray);
    resize(imgGray, imgGray, Size(800, 600));
    imshow("imgGray", imgGray);
    waitKey();
    Mat imgColor(height, width, CV_8UC3);
    switch (pattern) {
        case bayer_buffer::BAYER_RGrGbB:
            cvtColor(bayer8, imgColor, COLOR_BayerRG2BGR);
            break;
        case  bayer_buffer::BAYER_GrRBGb:
            cvtColor(bayer8, imgColor, COLOR_BayerGB2BGR);
            break;
        case bayer_buffer::BAYER_BGbGrR:
            cvtColor(bayer8, imgColor, COLOR_BayerBG2BGR);
            break;
        case  bayer_buffer::BAYER_GbBRGr:
            cvtColor(bayer8, imgColor, COLOR_BayerGR2BGR);
            break;
        default:
            break;
    }
    raw.applyCcm(imgColor);
    raw.applyGamma(imgColor);
    imwrite("res/imgColor.jpg", imgColor);
    resize(imgColor, imgColor, Size(800, 600));
    imshow("imgColor", imgColor);
    waitKey();
#endif


#if 1
    Mat rgbMat(height, width, CV_8UC3, Scalar(0, 0, 0));
    raw.demosaicing(bayer8, &rgbMat, pattern);
    //GaussianBlur(rgbMat, rgbMat, Size(7,7), 1.5, 1.5);
    //medianBlur(rgbMat, rgbMat,  3);
    raw.applyCcm(rgbMat);
    raw.applyGamma(rgbMat);
    imwrite("res/demosaicing.jpg", rgbMat);
    resize(rgbMat, rgbMat, Size(800, 600));
    imshow("demosaicing", rgbMat);
    waitKey();
#endif

#if 0
    // down scale the 12bit raw to 8bit raw.
    Mat rgbShow(600, 800, CV_8UC3);

    MatIterator_<uchar> it_start1 = rgbShow.begin<uchar>();
    MatIterator_<uchar> it_end1 = rgbShow.end<uchar>();

    MatIterator_<ushort> it_start2 = rgbMat.begin<ushort>();
    MatIterator_<ushort> it_end2 = rgbMat.end<ushort>();

    while (it_start1 != it_end1) {
        for (int i = 0; i < 3; i++) {
            it_start1[i] = (it_start2[i] > (1 << bitdepths)) ? 128 : (it_start2[i] >> (bitdepths - 8));
        }
        ++it_start1;
    }
    imshow("demosaicing", rgbShow);
    waitKey();
#endif
    return 0;
}
