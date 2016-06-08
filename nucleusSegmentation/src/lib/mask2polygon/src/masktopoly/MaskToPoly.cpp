#include "MaskToPoly.hpp"

using namespace std;
using namespace cv;

#ifdef BOUNDARY_EDIT  
#include <sstream>
void processBoundaryFixing(stringstream &instream, stringstream &outstream);
void processBoundaryFixing2Contours(stringstream &instream, stringstream &outstream, 
					vector<vector<Point> > *contours);

string TAB = "\t";
string SEMI_COLON = ";";
string SPACE = " ";
string COMMA = ",";
#endif

int MaskToPoly::readMask(char *inpFile) 
{
	inputImg = imread(inpFile, CV_8UC1);
	if (inputImg.data==NULL) 
		return 1;
	else
		return 0;
}

int MaskToPoly::extractPolygons()
{
	std::cout << "Zeros 1" << endl;
	Mat temp = Mat::zeros(inputImg.size() + Size(2,2), inputImg.type());
	std::cout << "Zeros 2" << endl;
	copyMakeBorder(inputImg, temp, 1, 1, 1, 1, BORDER_CONSTANT, 0);
	std::cout << "Zeros 3" << endl;

	findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, Point(0,0));
	std::cout << "Zeros 4" << endl;

	if (contours.size() > 0) {
		contours_poly.resize(contours.size());
		for (int i = 0; i < contours.size(); i++) 
		   	approxPolyDP( Mat(contours[i]), contours_poly[i], 2, true );
	}
	std::cout << "Zeros 5" << endl;
	return 0;
}

int MaskToPoly::editBoundaries()
{
#ifdef BOUNDARY_EDIT  
	stringstream tmpInStream;
	stringstream tmpOutStream;
	stringstream finalOutStream;

	if (contours.size() > 0) {
		for (int i = 0; i < contours.size(); i++) {
			tmpInStream << i << ": ";
			for (unsigned int ptc = 0; ptc < contours[i].size(); ++ptc) {
				tmpInStream << SPACE << contours[i][ptc].x << COMMA <<  contours[i][ptc].y;
			}
			tmpInStream << endl;
		}
		tmpInStream.clear();
		processBoundaryFixing(tmpInStream, tmpOutStream);
		tmpOutStream.clear();
		contours_poly.clear();
		processBoundaryFixing2Contours(tmpOutStream, finalOutStream, &contours_poly);
	}
#endif
	return 0;
}

unsigned int MaskToPoly::getPolygonCount()
{
	return contours_poly.size();
}

vector< vector<Point> > MaskToPoly::getPolygons()
{
	return contours_poly;
}

int MaskToPoly::writePolygons(char *outFile, unsigned int shiftX, unsigned int shiftY)
{
	ofstream fout;

	if (contours_poly.size()<=0) {
		cerr << "Number of polygons is zero. ";
		cerr << "No output file will be written." << endl;
		return 1;
	}

	fout.open(outFile);

   	fout << "PolygonNo\tX\tY\tArea\tBoundaries" << endl;	
	for (int idx = 0; idx < contours_poly.size(); idx++) {
		if (contours_poly[idx].size()>2) {

			// Find average point of polygon 
			unsigned int mid_x = 0;
			unsigned int mid_y = 0;
			for (unsigned int ptc = 0; ptc < contours_poly[idx].size(); ++ptc) {
				mid_x += contours_poly[idx][ptc].x;
				mid_y += contours_poly[idx][ptc].y;
			} 
			mid_x = (mid_x/contours_poly[idx].size()) + shiftX;
			mid_y = (mid_y/contours_poly[idx].size()) + shiftY;

			// Find area of polygon
			float poly_area = (float)contourArea(contours_poly[idx]);
			if (poly_area>AREA_THRESHOLD) { // if area is greater than threshold, write polygon out
				fout << idx << "\t" << mid_x << "\t" << mid_y << "\t" << poly_area << "\t";
				for (unsigned int ptc = 0; ptc < contours_poly[idx].size(); ++ptc) {
					fout << (contours_poly[idx][ptc].x+shiftX) << ",";
					fout << (contours_poly[idx][ptc].y+shiftY) << ";";
				}
				fout << endl;
			}
		}
	}	
	fout.close();

	return 0;
}

