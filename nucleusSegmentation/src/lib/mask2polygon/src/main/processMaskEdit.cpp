#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <string.h>
#include <unistd.h>
#include <iomanip>
#include <cstdlib> 

#include "BoundaryFix.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv/cv.hpp>

using namespace std;
using namespace cv;

string TAB = "\t";
string SEMI_COLON = ";";
string SPACE = " ";
string COMMA = ",";

void processBoundaryObject(stringstream &instream);

/* The first parameter is the filename,
 * the second parameter is optional, if there is a 2nd argument,
 *     the program will perform the boundary fixing/correction.
 */
int main (int argc, char **argv){

  if (argc < 2)
    {
      std::cerr<<"Argument: inputImageName"<<std::endl;
      exit(-1);
    }

	string line;
	stringstream tmpInStream;
	stringstream tmpOutStream;
	stringstream finalOutStream;

	char *buffer;
	Mat inputImg; /* InputImg */

	/* Read the input mask */
	inputImg = imread(argv[1], CV_8UC1);
	if (inputImg.data > 0) {
		Mat temp = Mat::zeros(inputImg.size() + Size(2,2), inputImg.type());
		copyMakeBorder(inputImg, temp, 1, 1, 1, 1, BORDER_CONSTANT, 0);

		std::vector<std::vector<Point> > contours;
		std::vector<Vec4i> hierarchy;

		/* Find the contour / boundary of nuclei */
		findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE);
		
		int counter = 1;
		if (contours.size() > 0) {
			for (int idx = 0; idx >= 0; idx = hierarchy[idx][0]) {
				tmpInStream << idx << ": ";
				for (unsigned int ptc = 0; ptc < contours[idx].size(); ++ptc) {
					tmpInStream << SPACE << contours[idx][ptc].x << COMMA <<  contours[idx][ptc].y;
				}
				tmpInStream << endl;
			}
			++counter;
		}
		/* Clear the stream (reset eof and etc.) */
		tmpInStream.clear();
		
		processBoundaryFixing(tmpInStream, tmpOutStream);
		tmpOutStream.clear();
		/* Output the result from boundary fix 2 to std output */
		processBoundaryFixing2(tmpOutStream, finalOutStream);
		//finalOutStream.clear();
		/* Handle the boundary objects and output them to std output  */
		processBoundaryObject(finalOutStream);

	}
	return 0;
}

void processBoundaryObject(stringstream &instream) {
	string line;
	int count = 1;
	while (instream.good()){
		getline(instream, line, '\n');
		if (!line.empty()) {
			cout << line << endl;
		}
	}
}
