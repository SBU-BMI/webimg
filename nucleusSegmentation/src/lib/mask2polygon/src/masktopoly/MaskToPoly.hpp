#ifndef MASK_TO_POLY_H
#define MASK_TO_POLY_H

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <string.h>
#include <unistd.h>
#include <iomanip>
#include <cstdlib> 

#include <opencv2/opencv.hpp>
//#include <opencv2/highgui.hpp>
//#include <opencv/cv.hpp>

using namespace std;
using namespace cv;

#define AREA_THRESHOLD 4
class MaskToPoly {
	private:
		Mat inputImg;
		vector<vector<Point> > contours;
    		vector<Vec4i> hierarchy;
		vector<vector<Point> > contours_poly;

	public:
		int readMask(char *inpFile);
		int extractPolygons();
		int editBoundaries();
		unsigned int getPolygonCount();
		vector< vector<Point> > getPolygons();
		int writePolygons(char *outFile, unsigned int shiftX, unsigned int shiftY); 
		MaskToPoly() { };
		~MaskToPoly() { };
};

#endif 
