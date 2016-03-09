#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include <iostream>
#include <stdio.h>
#include <iomanip>

using namespace std;
using namespace cv;

int gamma_index[] = {
    0, 64, 128, 256, 384, 512, 640, 768 ,
    896, 1024, 1280, 1536, 1792, 2048, 2304, 2560 ,
    2816, 3072, 3584, 4096, 4608, 5120, 6144, 7168 ,
    8192, 9216, 10240, 11264, 12288, 13312, 14336, 16384 ,
};

// normal gamma
int gamma1_table[] = {
    0, 1024, 1448, 2048, 2508, 2896, 3238, 3547,
    3831, 4096, 4579, 5016, 5418, 5792, 6144, 6476,
    6792, 7094, 7662, 8192, 8688, 9158, 10033, 10836,
    11585, 12288, 12952, 13584, 14188, 14768, 15325, 16384,
};

// gamma indoor
int gamma2_table[] = {
    0, 399, 799, 1198, 1598, 1910, 2194, 2456,
    2703, 2936, 3372, 3776, 4154, 4513, 4855, 5183,
    5498, 5803, 6385, 6936, 7462, 7965, 8919, 9813,
    10660, 11468, 12242, 12987, 13707, 14404, 15082, 16384,
};

// gamma outdoor
int gamma3_table[] = {
    0, 769, 1539, 2308, 3078, 3499, 3864, 4191 ,
    4489, 4764, 5262, 5708, 6113, 6488, 6837, 7166 ,
    7477, 7772, 8325, 8835, 9311, 9758, 10584, 11336, 
    12031, 12679, 13289, 13865, 14413, 14936, 15437, 16384 ,
};

int gamma4_table[] = {
    0, 0, 0, 0, 0, 0, 841, 3552,
    4575, 5608, 6672, 7753, 8856, 11104, 13404, 15730,
    16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384,
    16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384
};

int gamma_w = 16384;
int gamma_h = 16384;
void dot(vector<Mat> gamma_planes, int x, int y, int detla, int b, int g, int r) {
    int x1 = x - detla;
    int x2 = x + detla;
    int y1 = y - detla;
    int y2 = y + detla;
    if (x1 < 0) x1 = 0;
    if (x2 > gamma_w -1) x2 = gamma_w -1;
    if (y1 < 0) y1 = 0;
    if (y2 > gamma_h -1) y2 = gamma_h -1;

    for (int x3 = x1; x3 < x2; x3++) {
        for (int y3 = y1; y3 < y2; y3++) {
            gamma_planes[0].at<uchar>(y3, x3) = b;
            gamma_planes[1].at<uchar>(y3, x3) = g;
            gamma_planes[2].at<uchar>(y3, x3) = r;
        }
    }
}

void Gamma(double R, int w, int h, vector<Mat> &gamma_planes) {
    double C = h / pow(h, R);
    cout << "willis" << C << endl;
    for (int x = 0; x < w; x++) {
        int y = C * pow(x, R);
        dot(gamma_planes, x, y, 20, 0, 0, 0);
    }
};
int main(int arg, char **argv) {
    Mat gamma(gamma_h, gamma_w, CV_8UC3, Scalar(255, 255,255));
    vector<Mat> gamma_planes;
    split(gamma, gamma_planes);

    for (int i = 0; i < 31; i++) {
        int x1 = gamma_index[i];
        int x2 = gamma_index[i+1];
        int y1 = gamma1_table[i];
        int y2 = gamma1_table[i+1];

        double a = (y2 -y1) / (double)(x2 - x1);
        double b =  y1 - a * x1;
        cout << a << "," << b << endl;

        for (int x = x1; x < x2; x++) {
            int y = a * x + b;
            dot(gamma_planes, x, y, 10, 255, 0, 0);
        }
    }

    for (int i = 0; i < 31; i++) {
        int x1 = gamma_index[i];
        int x2 = gamma_index[i+1];
        int y1 = gamma2_table[i];
        int y2 = gamma2_table[i+1];

        double a = (y2 -y1) / (double)(x2 - x1);
        double b =  y1 - a * x1;
        cout << a << "," << b << endl;

        for (int x = x1; x < x2; x++) {
            int y = a * x + b;
            dot(gamma_planes, x, y, 10, 0, 255, 0);
        }
    }

    for (int i = 0; i < 31; i++) {
        int x1 = gamma_index[i];
        int x2 = gamma_index[i+1];
        int y1 = gamma3_table[i];
        int y2 = gamma3_table[i+1];

        double a = (y2 -y1) / (double)(x2 - x1);
        double b =  y1 - a * x1;
        cout << a << "," << b << endl;

        for (int x = x1; x < x2; x++) {
            int y = a * x + b;
            dot(gamma_planes, x, y, 10, 0, 0, 255);
        }
    }

    for (int i = 0; i < 31; i++) {
        int x1 = gamma_index[i];
        int x2 = gamma_index[i+1];
        int y1 = gamma4_table[i];
        int y2 = gamma4_table[i+1];

        double a = (y2 -y1) / (double)(x2 - x1);
        double b =  y1 - a * x1;
        cout << a << "," << b << endl;

        for (int x = x1; x < x2; x++) {
            int y = a * x + b;
            dot(gamma_planes, x, y, 10, 128, 128, 128);
        }
    }
    cout << "goto here" << endl;

    merge(gamma_planes, gamma);
    flip(gamma, gamma, 0);

    Mat resize_gamma;
    resize(gamma, resize_gamma, Size(640, 640));
    //imshow("gamma", gamma);
    //waitKey();
    imwrite("willis.jpg", resize_gamma);
    return 0;
}

