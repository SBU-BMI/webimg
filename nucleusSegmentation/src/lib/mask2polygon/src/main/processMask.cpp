#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <string.h>
#include <unistd.h>
#include <iomanip>
#include <cstdlib> 

#include "MaskToPoly.hpp"

using namespace std;
using namespace cv;

/* The first parameter is the filename,
 * the second parameter is optional, if there is a 2nd argument,
 *     the program will perform the boundary fixing/correction.
 */
int main (int argc, char **argv){

  	if (argc < 5) {
      		std::cerr<<"Argument: inputMask shift_x shift_y outFile" << endl;
		std::cerr<<"Number: " << argc << endl;
      		exit(-1);
	}
	
	char *inpfile = argv[1];
	char *outfile = argv[4];
	int shift_x = atoi(argv[2]);
	int shift_y = atoi(argv[3]);

	MaskToPoly maskToPoly;

	std::cout << "Reading file..." << endl;
	maskToPoly.readMask(inpfile);

	std::cout << "Extracting polygons..." << endl;
	maskToPoly.extractPolygons();
	std::cout << "Editing boundaries..." << endl;
	maskToPoly.editBoundaries();
	std::cout << "Writing file..." << endl;
	maskToPoly.writePolygons(outfile,shift_x,shift_y);
	std::cout << "Finished..." << endl;

	return 0;
}
