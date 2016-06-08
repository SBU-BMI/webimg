#include <cstdio>
#include <iostream>
#include <string>
#include <omp.h>

#include <fstream>
#include <sstream>
#include <iostream>

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>       /* time */

#ifdef ADD_UUID
#include <uuid/uuid.h>
#endif

//itk
#include "itkImage.h"
#include "itkRGBPixel.h"
#include "itkOpenCVImageBridge.h"

// openslide
#include "openslide.h"

// normalization
#include "Normalization.h"

// openCV
#include <opencv2/opencv.hpp>

#include "itkTypedefs.h"
#include "BinaryMaskAnalysisFilter.h"
#include "SFLSLocalChanVeseSegmentor2D.h"
#include "utilityIO.h"
#include "utilityTileAnalysis.h"
#include "utilityScalarImage.h"
#include "utilityWholeSlideProcessing.h"
#include "MultipleObjectFeatureAnalysisFilter.h"

#include "InputParameters.h"

const int _numOfFeatures = 25;
const std::string _featureNames[] = {
	"BoundingBoxTopLeftX",
        "BoundingBoxTopLeftY",
        "BoundingBoxBottomRightX",
        "BoundingBoxBottomRightY",
        "SizeInPixels",
        "PhysicalSize",
        "NumberOfPixelsOnBorder",
        "FeretDiameter",
        "PrincipalMoments0",
        "PrincipalMoments1",
        "Elongation",
        "Perimeter",
        "Roundness",
        "EquivalentSphericalRadius",
        "EquivalentSphericalPerimeter",
        "EquivalentEllipsoidDiameter0",
        "EquivalentEllipsoidDiameter1",
        "Flatness",
	"meanR",
    	"meanG",
    	"meanB",
    	"stdR",
    	"stdG",
    	"stdB",
	"Polygon"
};

typedef struct _PatchInfo {
	std::string inpFile;
	std::string outFolder;
	std::string outPrefix;
	int64_t     topLeftX;
	int64_t     topLeftY;
	int64_t     sizeX;
	int64_t     sizeY;
} PatchInfo;

typedef struct _PatchList {
	std::size_t patchCount;
	std::vector<PatchInfo> patches;
} PatchList;

typedef struct _AnalysisParameters {
	int         inpType; 
	std::string inpFile;

	float       otsuRatio;
	double      curvatureWeight;
	float       sizeLowerThld;
	float       sizeUpperThld;
	float       msKernel;
  	int64_t     levelsetNumberOfIteration;

	int64_t 	tileMinX, tileMinY;
	int64_t		tileWidth, tileHeight; 
	int64_t     patchMinX,patchMinY;	
	int64_t     patchWidth, patchHeight;

	int         outputLevel;
	std::string outFilePrefix;

	std::string analysisId;
	std::string analysisDesc;
	std::string subjectId;
	std::string caseId;

	float       mpp;
	int64_t		imgWidth, imgHeight;
} AnalysisParameters;	

int captureAnalysisParameters(AnalysisParameters *analysisParams, InputParameters *inpParams) 
{
	analysisParams->inpType = inpParams->inpType;
	analysisParams->inpFile = inpParams->inpFile;

	analysisParams->otsuRatio = inpParams->otsuRatio;
	analysisParams->curvatureWeight = inpParams->curvatureWeight;
	analysisParams->sizeLowerThld = inpParams->sizeLowerThld;
	analysisParams->sizeUpperThld = inpParams->sizeUpperThld;
	analysisParams->msKernel = inpParams->msKernel;
  	analysisParams->levelsetNumberOfIteration = inpParams->levelsetNumberOfIteration;

	analysisParams->tileMinX   = inpParams->topLeftX;
	analysisParams->tileMinY   = inpParams->topLeftY;
	analysisParams->tileWidth  = inpParams->sizeX;
	analysisParams->tileHeight = inpParams->sizeY;

	analysisParams->patchWidth  = inpParams->tileSizeX;	
	analysisParams->patchHeight = inpParams->tileSizeY;	

	analysisParams->outputLevel  = inpParams->outputLevel;

	analysisParams->analysisId   = inpParams->analysisId;
	analysisParams->analysisDesc = inpParams->analysisDesc;
	analysisParams->subjectId    = inpParams->subjectId;
	analysisParams->caseId       = inpParams->caseId;

	return 0;
}

int writeAnalysisParametersJSON(std::string outFilePrefix, AnalysisParameters *analysisParams)  
{
	std::ostringstream oss;
	oss << outFilePrefix << "-algmeta.json";
	std::ofstream outputMetadataFile(oss.str().c_str());

	std::string inpTypeStr;
	switch (analysisParams->inpType) {
		case WSI:
			inpTypeStr = "wsi";
			break;
		case TILES:
			inpTypeStr = "tiles";
			break;
		case ONETILE:
			inpTypeStr = "onetile";
			break;
		case IMG:
			inpTypeStr = "img";
			break;
		default:
			inpTypeStr = "undefined";
			break;
	}
	
	std::string outputLevelStr;
	switch (analysisParams->outputLevel) {
		case MASK_ONLY: 
			outputLevelStr = "mask";
			break;
		case MASK_IMG: 
			outputLevelStr = "mask:img";
			break;
		case MASK_IMG_OVERLAY: 
			outputLevelStr = "mask:img:overlay";
			break;
		default:
			outputLevelStr = "undefined";
			break;
	} 


	outputMetadataFile	<< "{ "
				<< "\"input_type\" : \"" << inpTypeStr << "\", " 
				<< "\"otsu_ratio\" : " << analysisParams->otsuRatio << ", "
				<< "\"curvature_weight\" : " << analysisParams->curvatureWeight << ", "
				<< "\"min_size\" : " << analysisParams->sizeLowerThld << ", "
				<< "\"max_size\" : " << analysisParams->sizeUpperThld << ", "
				<< "\"ms_kernel\" : " << analysisParams->msKernel << ", "
				<< "\"levelset_num_iters\" : " << analysisParams->levelsetNumberOfIteration << ", "
				<< "\"mpp\" : " << analysisParams->mpp << ", "
				<< "\"image_width\" : " << analysisParams->imgWidth << ", "
				<< "\"image_height\" : " << analysisParams->imgHeight << ", "
				<< "\"tile_minx\" : " << analysisParams->tileMinX << ", "
				<< "\"tile_miny\" : " << analysisParams->tileMinY << ", "
				<< "\"tile_width\" : " << analysisParams->tileWidth << ", "
				<< "\"tile_height\" : " << analysisParams->tileHeight << ", "
				<< "\"patch_minx\" : " << analysisParams->patchMinX << ", "
				<< "\"patch_miny\" : " << analysisParams->patchMinY << ", "
				<< "\"patch_width\" : " << analysisParams->patchWidth << ", "
				<< "\"patch_height\" : " << analysisParams->patchHeight << ", "
				<< "\"output_level\" : \"" << outputLevelStr << "\", "
				<< "\"out_file_prefix\" : \"" << analysisParams->outFilePrefix << "\", "
				<< "\"subject_id\" : \"" << analysisParams->subjectId << "\", "
				<< "\"case_id\" : \"" << analysisParams->caseId << "\", "
				<< "\"analysis_id\" : \"" << analysisParams->analysisId << "\", "
				<< "\"analysis_desc\" : \"" << analysisParams->analysisDesc << "\""
				<< " }" << std::endl;
	outputMetadataFile.close();

	return 0;
}

int writeAnalysisParametersCSV(std::string outFilePrefix, AnalysisParameters *analysisParams)  
{
	std::ostringstream oss;
	oss << outFilePrefix << "-algmeta.csv";
	std::ofstream outputMetadataFile(oss.str().c_str());
	outputMetadataFile	<< "input_type," 
				<< "otsu_ratio,"
				<< "curvature_weight,"
				<< "min_size,"
				<< "max_size,"
				<< "ms_kernel,"
				<< "levelset_num_iters,"
				<< "mpp,"
				<< "image_width,"
				<< "image_height,"
				<< "tile_minx,"
				<< "tile_miny,"
				<< "tile_width,"
				<< "tile_height,"
				<< "patch_minx,"
				<< "patch_miny,"
				<< "patch_width,"
				<< "patch_height,"
				<< "output_level,"
				<< "out_file_prefix,"
				<< "subject_id,"
				<< "case_id,"
				<< "analysis_id,"
				<< "analysis_desc" 
				<< std::endl;
	std::string inpTypeStr;
	switch (analysisParams->inpType) {
		case WSI:
			inpTypeStr = "wsi";
			break;
		case TILES:
			inpTypeStr = "tiles";
			break;
		case ONETILE:
			inpTypeStr = "onetile";
			break;
		case IMG:
			inpTypeStr = "img";
			break;
		default:
			inpTypeStr = "undefined";
			break;
	}
	
	std::string outputLevelStr;
	switch (analysisParams->outputLevel) {
		case MASK_ONLY: 
			outputLevelStr = "mask";
			break;
		case MASK_IMG: 
			outputLevelStr = "mask:img";
			break;
		case MASK_IMG_OVERLAY: 
			outputLevelStr = "mask:img:overlay";
			break;
		default:
			outputLevelStr = "undefined";
			break;
	} 
	outputMetadataFile	<< inpTypeStr << "," 
				<< analysisParams->otsuRatio << ","
				<< analysisParams->curvatureWeight << ","
				<< analysisParams->sizeLowerThld << ","
				<< analysisParams->sizeUpperThld << ","
				<< analysisParams->msKernel << ","
				<< analysisParams->levelsetNumberOfIteration << ","
				<< analysisParams->mpp << ","
				<< analysisParams->imgWidth << ","
				<< analysisParams->imgHeight << ","
				<< analysisParams->tileMinX << ","
				<< analysisParams->tileMinY << ","
				<< analysisParams->tileWidth << ","
				<< analysisParams->tileHeight << ","
				<< analysisParams->patchMinX << ","
				<< analysisParams->patchMinY << ","
				<< analysisParams->patchWidth << ","
				<< analysisParams->patchHeight << ","
				<< outputLevelStr << ","
				<< analysisParams->outFilePrefix << ","
				<< analysisParams->subjectId << ","
				<< analysisParams->caseId << ","
				<< analysisParams->analysisId << ","
				<< analysisParams->analysisDesc 
				<< std::endl;
	outputMetadataFile.close();

	return 0;
}

#define SKIP_BBOX 4 // do not output the bounding box info -- it is computed while loading to the database
int writeFeatureCSV(std::string outFilePrefix, std::vector< std::vector<FeatureValueType> > &features)
{
	std::ostringstream oss;
	oss << outFilePrefix << "-features.csv";
	std::ofstream outputFeatureFile(oss.str().c_str());
	int i;
	for (i=SKIP_BBOX;i<_numOfFeatures-1;i++) 
		outputFeatureFile<<_featureNames[i]<<",";
	outputFeatureFile<<_featureNames[i]<<std::endl;

	std::size_t iObject, iFeature;
	for (iObject = 0; iObject < features.size(); ++iObject) {
		for (iFeature = SKIP_BBOX; iFeature <_numOfFeatures-1; ++iFeature) 
			outputFeatureFile<<features[iObject][iFeature]<<",";
		outputFeatureFile<<"[";
		for (;iFeature<features[iObject].size()-1; ++iFeature) {
			outputFeatureFile<<features[iObject][iFeature]<<":";
		}
		outputFeatureFile<<features[iObject][iFeature]<<"]"<<std::endl<<std::flush;
	}
	outputFeatureFile.close();
}

#ifdef ADD_UUID
inline std::string generateUUIDString()
{
	uuid_t outId;
	char uuidStr[64];
	uuid_generate(outId);
	uuid_unparse(outId,uuidStr);
	return uuidStr;
}
#endif

inline void initRandom()
{
	srand(time(NULL));
}

inline std::string getRandomIDString()
{
	std::stringstream ss;
	ss << rand();
	return ss.str();
}

int segmentWSI(InputParameters *inpParams)
{
	openslide_t *osr = openslide_open(inpParams->inpFile.c_str());
	if (osr==NULL) return 1;

	inpParams->mpp = ImagenomicAnalytics::WholeSlideProcessing::extractMPP<char>(osr);

	int32_t levelOfLargestSize = 0; // 0-th level is the largest
	int64_t largestW;
	int64_t largestH;
	{
		int64_t w[1];
		int64_t h[1];

		openslide_get_level_dimensions(osr, levelOfLargestSize, w, h);

		largestW = w[0];
		largestH = h[0];
	}

	std::vector<int64_t> tileTopleftX;
	std::vector<int64_t> tileTopleftY;
	std::vector<int64_t> tileSizeX;
	std::vector<int64_t> tileSizeY;

	if (inpParams->tileSizeX<0) {
		inpParams->tileSizeX = DEFAULT_WSI_TILE;
		inpParams->tileSizeY = DEFAULT_WSI_TILE;
	}

	ImagenomicAnalytics::WholeSlideProcessing::generateTileRegions<char>(largestW, largestH, inpParams->tileSizeX, 
													tileTopleftX, tileTopleftY, tileSizeX, tileSizeY);
	std::size_t numberOfTiles = tileSizeY.size();

#pragma omp parallel for
	for (std::size_t iTile = 0; iTile < numberOfTiles; ++iTile)
	{
		int64_t topLeftX = tileTopleftX[iTile];
		int64_t topLeftY = tileTopleftY[iTile];
		int64_t sizeX    = tileSizeX[iTile];
		int64_t sizeY    = tileSizeY[iTile];

		AnalysisParameters analysisParams;
		captureAnalysisParameters(&analysisParams,inpParams);
		std::stringstream outFilePrefix;
		std::stringstream outPathPrefix;
		outFilePrefix   << inpParams->subjectId 
						<< "." << inpParams->caseId 
						<< "." << getRandomIDString()
						<< "_mpp_"  << inpParams->mpp
						<< "_x" << topLeftX
						<< "_y" << topLeftY;  
		outPathPrefix	<< inpParams->outFolder 
						<< "/" 
						<< outFilePrefix.str();

		analysisParams.outFilePrefix = outFilePrefix.str();
		analysisParams.mpp       = inpParams->mpp;
		analysisParams.imgWidth  = largestW;
		analysisParams.imgHeight = largestH;
		analysisParams.tileMinX    = topLeftX;
		analysisParams.tileMinY    = topLeftY;
		analysisParams.tileWidth   = sizeX;
		analysisParams.tileHeight  = sizeY;
		analysisParams.patchMinX   = topLeftX;
		analysisParams.patchMinY   = topLeftY;
		analysisParams.patchWidth  = sizeX;
		analysisParams.patchHeight = sizeY;

		cv::Mat thisTile;
#pragma omp critical
		{
			thisTile = ImagenomicAnalytics::WholeSlideProcessing::extractTileFromWSI<char>(osr, levelOfLargestSize, topLeftX, topLeftY, sizeX, sizeY);
		}
		itkRGBImageType::Pointer thisTileItk =  itk::OpenCVImageBridge::CVMatToITKImage< itkRGBImageType >( thisTile );

		itkUShortImageType::Pointer outputLabelImageUShort = itkUShortImageType::New();
		itkUCharImageType::Pointer nucleusBinaryMask = ImagenomicAnalytics::TileAnalysis::processTile<char>(thisTile, outputLabelImageUShort, 
				inpParams->otsuRatio, inpParams->curvatureWeight, inpParams->sizeLowerThld, 
				inpParams->sizeUpperThld, inpParams->mpp, inpParams->msKernel, inpParams->levelsetNumberOfIteration);

#pragma omp critical
		{
			if (inpParams->outputLevel>=MASK_IMG) {
				std::ostringstream oss;
				oss << outPathPrefix.str() << "-tile.jpg"; // Output tile
				ImagenomicAnalytics::IO::writeImage<itkRGBImageType>(thisTileItk, oss.str().c_str(), 0);
			}
			if (inpParams->outputLevel>=MASK_ONLY) {
				std::ostringstream oss;
				oss << outPathPrefix.str() << "-seg.png"; // Mask tile
				ImagenomicAnalytics::IO::writeImage<itkUCharImageType>(nucleusBinaryMask, oss.str().c_str(), 0);
			}
			if (inpParams->outputLevel>=MASK_IMG_OVERLAY) {
				std::ostringstream oss;
				oss << outPathPrefix.str() << "-overlay.jpg"; 
				itkRGBImageType::Pointer overlay = ImagenomicAnalytics::ScalarImage::generateSegmentationOverlay<char>(thisTileItk, nucleusBinaryMask);
				ImagenomicAnalytics::IO::writeImage<itkRGBImageType>(overlay, oss.str().c_str(), 0);
			}
		}

		// Compute features
  		ImagenomicAnalytics::MultipleObjectFeatureAnalysisFilter featureAnalyzer;
  		featureAnalyzer.setInputRGBImage(thisTileItk);
  		featureAnalyzer.setObjectBinaryMask(nucleusBinaryMask);
		featureAnalyzer.setTopLeft(topLeftX,topLeftY);
  		featureAnalyzer.update();

  		std::vector< std::vector<FeatureValueType> > features = featureAnalyzer.getFeatures();

#pragma omp critical
		{
			writeFeatureCSV(outPathPrefix.str(), features);
			writeAnalysisParametersJSON(outPathPrefix.str(),&analysisParams);
			writeAnalysisParametersCSV(outPathPrefix.str(),&analysisParams);
		}
	}

#pragma omp barrier
	openslide_close(osr);

	return 0;
}

int segmentImg(InputParameters *inpParams)
{
  std::cout<<"------------in segmentImg ------------------\n"<<std::flush;

	const int ImageDimension = 2;

	AnalysisParameters analysisParams;
	captureAnalysisParameters(&analysisParams,inpParams);

    std::cout<<"------------in segmentImg 11111111111 ------------------\n"<<std::flush;

	cv::Mat thisTile = imread(inpParams->inpFile.c_str());
	itkRGBImageType::Pointer thisTileItk =  itk::OpenCVImageBridge::CVMatToITKImage< itkRGBImageType >( thisTile );
	analysisParams.imgWidth  = (int64_t)thisTile.cols;
	analysisParams.imgHeight = (int64_t)thisTile.rows;
	analysisParams.mpp       = inpParams->mpp;
	analysisParams.tileMinX    = 0;
	analysisParams.tileMinY    = 0;
	analysisParams.tileWidth   = analysisParams.imgWidth;
	analysisParams.tileHeight  = analysisParams.imgHeight;
	analysisParams.patchMinX   = 0;
	analysisParams.patchMinY   = 0;
	analysisParams.patchWidth  = analysisParams.imgHeight;
	analysisParams.patchHeight = analysisParams.imgHeight;

    std::cout<<"here 2\n"<<std::flush;

	std::stringstream outFilePrefix;
	std::stringstream outPathPrefix;
	outFilePrefix   << inpParams->subjectId 
					<< "." << inpParams->caseId 
					<< "." << getRandomIDString()
					<< "_mpp_"  << inpParams->mpp
					<< "_x" << 0
					<< "_y" << 0;  
	outPathPrefix	<< inpParams->outFolder 
					<< "/" 
					<< outFilePrefix.str();

	analysisParams.outFilePrefix = outFilePrefix.str();

    std::cout<<"here 3\n"<<std::flush;

	itkUShortImageType::Pointer outputLabelImage  = itkUShortImageType::New();
	itkUCharImageType::Pointer  nucleusBinaryMask = ImagenomicAnalytics::TileAnalysis::processTile<char>(thisTile, outputLabelImage, 
			inpParams->otsuRatio, inpParams->curvatureWeight, inpParams->sizeLowerThld, 
			inpParams->sizeUpperThld, inpParams->mpp, inpParams->msKernel,
			inpParams->levelsetNumberOfIteration);

	if (inpParams->outputLevel>=MASK_ONLY) {
		std::ostringstream oss;
		oss << outPathPrefix.str() << "-seg.png";
		ImagenomicAnalytics::IO::writeImage<itkUCharImageType>(nucleusBinaryMask, oss.str().c_str(), 0);
	}
	if (inpParams->outputLevel>=MASK_IMG) {
		std::ostringstream oss;
		oss << outPathPrefix.str() << "-tile.jpg"; 
		ImagenomicAnalytics::IO::writeImage<itkRGBImageType>(thisTileItk, oss.str().c_str(), 0);
	}
	if (inpParams->outputLevel>=MASK_IMG_OVERLAY) {
		std::ostringstream oss;
		oss << outPathPrefix.str() << "-overlay.jpg"; 
		itkRGBImageType::Pointer overlay = ImagenomicAnalytics::ScalarImage::generateSegmentationOverlay<char>(thisTileItk, nucleusBinaryMask);
		ImagenomicAnalytics::IO::writeImage<itkRGBImageType>(overlay, oss.str().c_str(), 0);
	}

	// Compute features
	ImagenomicAnalytics::MultipleObjectFeatureAnalysisFilter featureAnalyzer;
	featureAnalyzer.setInputRGBImage(thisTileItk);
	featureAnalyzer.setObjectBinaryMask(nucleusBinaryMask);
	featureAnalyzer.setTopLeft(0,0);
	featureAnalyzer.update();

	std::vector< std::vector<FeatureValueType> > features = featureAnalyzer.getFeatures();

	writeFeatureCSV(outPathPrefix.str(), features);
	writeAnalysisParametersJSON(outPathPrefix.str(),&analysisParams);
	writeAnalysisParametersCSV(outPathPrefix.str(),&analysisParams);

	return 0;
}

void resetPatchList(PatchList& patchList)
{
	patchList.patches.clear(); 
	patchList.patchCount = 0;
}

size_t generatePatchList(InputParameters *inpParams, PatchList& patchList) 
{
	int64_t tileSizeX = inpParams->tileSizeX;
	int64_t tileSizeY = inpParams->tileSizeY;
	int64_t startX    = inpParams->topLeftX;
	int64_t startY    = inpParams->topLeftY;
	int64_t endX      = startX + inpParams->sizeX;
	int64_t endY	  = startY + inpParams->sizeY;

	if (inpParams->tileSizeX<0 || inpParams->tileSizeY<0) {
		tileSizeX = inpParams->sizeX+1;
		tileSizeY = inpParams->sizeY+1;
	}

	resetPatchList(patchList);

	int64_t sizeExtX, sizeExtY;
	std::string outPrefix;
	for (int64_t i=startX;i<endX;i+=tileSizeX) {
		if ((i+tileSizeX)>endX) 
			sizeExtX = (endX-i);
		else
			sizeExtX = tileSizeX;
		for (int64_t j=startY;j<endY;j+=tileSizeY) {
			if ((j+tileSizeY)>endY) 
				sizeExtY = endY-j;
			else
				sizeExtY = tileSizeY;

			PatchInfo patchInfo;
			patchInfo.topLeftX = i;
			patchInfo.topLeftY = j;
			patchInfo.sizeX = sizeExtX;
			patchInfo.sizeY = sizeExtY;
			patchInfo.outPrefix = inpParams->subjectId + "." 
						+ inpParams->caseId + "." 
						+ getRandomIDString();  
			patchInfo.outFolder = inpParams->outFolder;
			patchInfo.inpFile   = inpParams->inpFile;
			patchList.patches.push_back(patchInfo);
		}
	}
	patchList.patchCount = patchList.patches.size();

	return (std::size_t) patchList.patchCount; 
}

std::size_t readPatchList(InputParameters *inpParams, std::vector<PatchList>& patchListArray)
{
	std::ifstream infile(inpParams->inpFile.c_str());

	std::string line;
	PatchList patchList; 
	InputParameters tmpParams;
	int lineNum = 1;
	while (std::getline(infile,line)) {
		std::istringstream ss(line);
		std::string token;

		// Input image file
		if (!(std::getline(ss, tmpParams.inpFile, ','))) {
			std::cerr << "Error reading the tile list file: Missing input file column at line: " 
					<< lineNum << std::endl;
			return 0;
		}

		// subjectId
		if (!(std::getline(ss, tmpParams.subjectId, ','))) {
			std::cerr << "Error readling the tile list file: Missing subjectId column at line: " 
					<< lineNum << std::endl;
			return 0;
		}

		// caseId
		if (!(std::getline(ss, tmpParams.caseId, ','))) {
			std::cerr << "Error readling the tile list file: Missing caseId column at line: " 
					<< lineNum << std::endl;
			return 0;
		}

		// Output folder	
		if (!(std::getline(ss, tmpParams.outFolder, ','))) {
			std::cerr << "Error readling the tile list file: Missing output folder column at line: " 
					<< lineNum << std::endl;
			return 0;
		}

		// top left X,Y
		if (!(std::getline(ss, token, ','))) {
			std::cerr << "Error readling the tile list file: Missing top left X column at line: " 
					<< lineNum << std::endl;
			return 0;
		}		
		tmpParams.topLeftX = atoi(token.c_str());
		if (!(std::getline(ss, token, ','))) {
			std::cerr << "Error readling the tile list file: Missing top left Y column at line: " 
					<< lineNum << std::endl;
			return 0;
		}
		tmpParams.topLeftY = atoi(token.c_str());

		// width and height
		if (!(std::getline(ss, token, ','))) {
			std::cerr << "Error readling the tile list file: Missing width (sizeX) column at line: " 
					<< lineNum << std::endl;
			return 0;
		}
		tmpParams.sizeX = atoi(token.c_str());
		if (!(std::getline(ss, token, ','))) {
			std::cerr << "Error readling the tile list file: Missing height (sizeY) column at line: " 
					<< lineNum << std::endl;
			return 0;
		}
		tmpParams.sizeY = atoi(token.c_str());

		// tiling size 
		if (!(std::getline(ss, token, ','))) {
        	tmpParams.tileSizeX = DEFAULT_SMALL_TILE;
		} else {
        	tmpParams.tileSizeX = atoi(token.c_str());
		}
        if (!(std::getline(ss, token, ','))) {
        	tmpParams.tileSizeY = DEFAULT_SMALL_TILE;
		} else {
        	tmpParams.tileSizeY = atoi(token.c_str());
		}

		generatePatchList(&tmpParams,patchList);
		patchListArray.push_back(patchList);
	}

	return (std::size_t) patchListArray.size();
}

int compressTiles(InputParameters *inpParams)
{
	if (inpParams->isZipped==0) return 1;
	std::string cmd = "zip -ujr " + inpParams->zipFile + " " + inpParams->outFolder + "/" + "* -x \\*.svs"; 
	system(cmd.c_str());
	return 0;
}

int segmentTiles(InputParameters *inpParams, PatchList *patchList)
{
  std::cout<<"------------I'm in 3333333 ------------------\n"<<std::flush;

#pragma omp parallel for
	for (std::size_t iPatch=0;iPatch<patchList->patchCount;iPatch++) {
		PatchInfo patchInfo   = patchList->patches[iPatch];
		std::string fileName  = patchInfo.inpFile; 
		std::string outPrefix = patchInfo.outPrefix; 
		std::string outFolder = patchInfo.outFolder;
		int64_t topLeftX      = patchInfo.topLeftX;  
		int64_t topLeftY      = patchInfo.topLeftY; 
		int64_t sizeX         = patchInfo.sizeX;  
		int64_t sizeY         = patchInfo.sizeY; 


  std::cout<<"------------I'm in 4444444444 ------------------\n"<<std::flush;

		AnalysisParameters analysisParams;
		captureAnalysisParameters(&analysisParams,inpParams);
		analysisParams.tileMinX    = inpParams->topLeftX;
		analysisParams.tileMinY    = inpParams->topLeftY;
		analysisParams.tileWidth   = inpParams->sizeX;
		analysisParams.tileHeight  = inpParams->sizeY;
		analysisParams.patchMinX   = topLeftX;
		analysisParams.patchMinY   = topLeftY;
		analysisParams.patchWidth  = sizeX;
		analysisParams.patchHeight = sizeY; 

  std::cout<<"------------I'm in 555555555555 ------------------\n"<<std::flush;

  std::cout<<fileName<<std::endl<<std::flush;

#pragma omp critical
		{
			std::cout 	<< "INPUT READING: " << fileName << " " << outPrefix << " "
						<< topLeftX << " " << topLeftY <<  " "
						<< sizeX << " " << sizeY << std::endl;
		}

		cv::Mat thisTile;
		int32_t levelOfLargestSize = 0; // 0-th level is the largest
		float   mpp;
		char    noErrors = 1;
		std::stringstream outFilePrefix;
		std::stringstream outPathPrefix;
#pragma omp critical
		{
			int64_t w[1],h[1];
			try {
				openslide_t *osr = openslide_open(fileName.c_str());
				mpp = ImagenomicAnalytics::WholeSlideProcessing::extractMPP<char>(osr);
				openslide_get_level_dimensions(osr, levelOfLargestSize, w, h);
				if ((topLeftX+sizeX)>w[0] || (topLeftY+sizeY)>h[0]) {
					openslide_close(osr);
					throw 1;
				}
				thisTile = ImagenomicAnalytics::WholeSlideProcessing::extractTileFromWSI<char>(osr, levelOfLargestSize, 
							topLeftX, topLeftY, sizeX, sizeY);

				analysisParams.imgWidth  = w[0]; 
				analysisParams.imgHeight = h[0]; 
				analysisParams.mpp       = mpp;

				outFilePrefix   << outPrefix  
								<< "_mpp_" << mpp
								<< "_x" << topLeftX
								<< "_y" << topLeftY;  
				outPathPrefix	<< outFolder 
								<< "/" 
								<< outFilePrefix.str();
				analysisParams.outFilePrefix = outFilePrefix.str();
				openslide_close(osr);
			} catch (...) {
				std::cerr 	<< "ERROR: Requested tile ("  
							<< topLeftX <<"," << topLeftY << "," << topLeftX+sizeX << "," << topLeftY+sizeY
							<< ") is out of bounds (" 
							<< w[0] << "," << h[0] 
							<< ") in image: " << fileName << std::endl;
				noErrors = 0; 
			}
		}
		if (noErrors) {
			itkRGBImageType::Pointer thisTileItk = itk::OpenCVImageBridge::CVMatToITKImage< itkRGBImageType >( thisTile );
			analysisParams.mpp = mpp;

			itkUShortImageType::Pointer outputLabelImageUShort = itkUShortImageType::New();
			itkUCharImageType::Pointer nucleusBinaryMask = ImagenomicAnalytics::TileAnalysis::processTile<char>(thisTile, outputLabelImageUShort, 
					inpParams->otsuRatio, inpParams->curvatureWeight, inpParams->sizeLowerThld, 
					inpParams->sizeUpperThld, mpp, inpParams->msKernel, inpParams->levelsetNumberOfIteration);

#pragma omp critical
			{
				if (inpParams->outputLevel>=MASK_IMG) {
					std::ostringstream oss;
					oss << outPathPrefix.str() << "-tile.jpg"; 
					ImagenomicAnalytics::IO::writeImage<itkRGBImageType>(thisTileItk, oss.str().c_str(), 0);
				}
				if (inpParams->outputLevel>=MASK_ONLY) {
					std::ostringstream oss;
					oss << outPathPrefix.str() << "-seg.png"; // Mask tile 
					ImagenomicAnalytics::IO::writeImage<itkUCharImageType>(nucleusBinaryMask, oss.str().c_str(), 0);
				}
				if (inpParams->outputLevel>=MASK_IMG_OVERLAY) {
					std::ostringstream oss;
					oss << outPathPrefix.str() << "-overlay.jpg"; 
					itkRGBImageType::Pointer overlay = ImagenomicAnalytics::ScalarImage::generateSegmentationOverlay<char>(thisTileItk, nucleusBinaryMask);
					ImagenomicAnalytics::IO::writeImage<itkRGBImageType>(overlay, oss.str().c_str(), 0);
				}
			}

			// Compute features
			ImagenomicAnalytics::MultipleObjectFeatureAnalysisFilter featureAnalyzer;
			featureAnalyzer.setInputRGBImage(thisTileItk);
			featureAnalyzer.setObjectBinaryMask(nucleusBinaryMask);
			featureAnalyzer.setTopLeft(topLeftX,topLeftY);
			featureAnalyzer.update();
			std::vector< std::vector<FeatureValueType> > features = featureAnalyzer.getFeatures();

#pragma omp critical
			{
				writeFeatureCSV(outPathPrefix.str(),features);
				writeAnalysisParametersJSON(outPathPrefix.str(),&analysisParams);
				writeAnalysisParametersCSV(outPathPrefix.str(),&analysisParams);
			}
		}
	}

#pragma omp barrier
	return 0;
}

#if 0
int segmentSingleTile(InputParameters *inpParams)
{
	std::string fileName  = inpParams->inpFile; 
	std::string outPrefix = inpParams->outPrefix;
	int64_t topLeftX = inpParams->topLeftX;  
	int64_t topLeftY = inpParams->topLeftY; 
	int64_t sizeX = inpParams->sizeX; 
	int64_t sizeY = inpParams->sizeY;

	AnalysisParameters analysisParams;
	captureAnalysisParameters(&analysisParams,inpParams);

	openslide_t *osr = openslide_open(fileName.c_str());
	if (osr==NULL) return 1;

	analysisParams.mpp = ImagenomicAnalytics::WholeSlideProcessing::extractMPP<char>(osr);

	int32_t levelOfLargestSize = 0; // 0-th level is the largest
	int64_t w[1],h[1];
	openslide_get_level_dimensions(osr, levelOfLargestSize, w, h);
	if ((topLeftX+sizeX)>w[0] || (topLeftY+sizeY)>h[0]) {
		std::cerr 	<< "ERROR: Requested tile ("  
					<< topLeftX <<"," << topLeftY << "," << topLeftX+sizeX << "," << topLeftY+sizeY
					<< ") is out of bounds (" 
					<< w[0] << "," << h[0] 
					<< ") in image: " << fileName << std::endl;
		return 1;
	}

	cv::Mat thisTile;
	thisTile = ImagenomicAnalytics::WholeSlideProcessing::extractTileFromWSI<char>(osr, levelOfLargestSize, 
					topLeftX, topLeftY, sizeX, sizeY);
	openslide_close(osr);

	itkRGBImageType::Pointer thisTileItk =  itk::OpenCVImageBridge::CVMatToITKImage< itkRGBImageType >( thisTile );

	itkUShortImageType::Pointer outputLabelImageUShort = itkUShortImageType::New();
	itkUCharImageType::Pointer nucleusBinaryMask = ImagenomicAnalytics::TileAnalysis::processTile<char>(thisTile, outputLabelImageUShort, 
			inpParams->otsuRatio, inpParams->curvatureWeight, inpParams->sizeLowerThld, 
			inpParams->sizeUpperThld, inpParams->mpp, inpParams->msKernel, inpParams->levelsetNumberOfIteration);

	if (inpParams->outputLevel>=MASK_IMG) {
		std::ostringstream oss;
		oss << inpParams->outPrefix << "_mpp_" << inpParams->mpp << "_x" << topLeftX << "_y" << topLeftY << "-tile.jpg"; 
		ImagenomicAnalytics::IO::writeImage<itkRGBImageType>(thisTileItk, oss.str().c_str(), 0);
	}
	if (inpParams->outputLevel>=MASK_ONLY) {
		std::ostringstream oss;
		oss << inpParams->outPrefix << "_mpp_" << inpParams->mpp << "_x" << topLeftX << "_y" << topLeftY << "-seg.png"; // Mask tile 
		ImagenomicAnalytics::IO::writeImage<itkUCharImageType>(nucleusBinaryMask, oss.str().c_str(), 0);
	}
	if (inpParams->outputLevel>=MASK_IMG_OVERLAY) {
		std::ostringstream oss;
		oss << inpParams->outPrefix << "_mpp_" << inpParams->mpp << "_x" << topLeftX << "_y" << topLeftY << "-overlay.jpg"; 
		itkRGBImageType::Pointer overlay = ImagenomicAnalytics::ScalarImage::generateSegmentationOverlay<char>(thisTileItk, nucleusBinaryMask);
		ImagenomicAnalytics::IO::writeImage<itkRGBImageType>(overlay, oss.str().c_str(), 0);
	}

	// Compute features
	ImagenomicAnalytics::MultipleObjectFeatureAnalysisFilter featureAnalyzer;
	featureAnalyzer.setInputRGBImage(thisTileItk);
	featureAnalyzer.setObjectBinaryMask(nucleusBinaryMask);
	featureAnalyzer.setTopLeft(topLeftX,topLeftY);
	featureAnalyzer.update();
	std::vector< std::vector<FeatureValueType> > features = featureAnalyzer.getFeatures();

	writeFeatureCSV(inpParams->outPrefix, inpParams->mpp, topLeftX, topLeftY, features);
	writeAnalysisParametersJSON(&analysisParams);

	return 0;
}
#endif

int main(int argc, char **argv)
{
    std::cout<<"------------I'm in ------------------\n"<<std::flush;

	const int ImageDimension = 2;
	InputParameters inpParams;

	if (parseInputParameters(argc,argv,&inpParams)!=0) {
		printParseError(argv);
		return 1;
	}
	printInputParameters(&inpParams);

    std::cout<<"------------I'm in 111111 ------------------\n"<<std::flush;

	initRandom();

    std::cout<<"------------I'm in 2222222 ------------------\n"<<std::flush;
    std::cout<<inpParams.inpType<<std::endl<<std::flush;
    std::cout<<WSI<<std::endl<<std::flush;
    std::cout<<IMG<<std::endl<<std::flush;
    std::cout<<TILES<<std::endl<<std::flush;

	if (inpParams.inpType==WSI) {
		segmentWSI(&inpParams);
	} else if (inpParams.inpType==IMG) {
		segmentImg(&inpParams);
	} else if (inpParams.inpType==TILES) {
		std::vector<PatchList> patchListArray;
		std::size_t patchArrayCount = readPatchList(&inpParams,patchListArray); 
		if (patchArrayCount<=0) return 1;
		for (int i=0;i<patchArrayCount;i++) 
			segmentTiles(&inpParams,&patchListArray[i]);
	} else if (inpParams.inpType==ONETILE) {
		PatchList patchList;
		std::size_t patchCount = generatePatchList(&inpParams,patchList);
		if (patchCount<=0) return 1;
		segmentTiles(&inpParams,&patchList);
		compressTiles(&inpParams);
	} else {
		std::cerr << "Unknown input type." << std::endl;
		printParseError(argv);
		return 1;
	}

	return 0;
}
