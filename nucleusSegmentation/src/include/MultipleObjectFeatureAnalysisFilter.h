#ifndef MultipleObjectFeatureAnalysisFilter_h_
#define MultipleObjectFeatureAnalysisFilter_h_

#include <vector>
#include <sstream>

// itk
#include "itkImage.h"
#include "itkRGBPixel.h"
#include "itkTypedefs.h"


namespace ImagenomicAnalytics
{
  class MultipleObjectFeatureAnalysisFilter
  {
  public:
    typedef MultipleObjectFeatureAnalysisFilter Self;

    ////////////////////////////////////////////////////////////////////////////////
    /// ctor
    MultipleObjectFeatureAnalysisFilter();
    MultipleObjectFeatureAnalysisFilter(std::ostream& outputStream);
    ~MultipleObjectFeatureAnalysisFilter() {}
    /// ctor, end
    ////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////
    /// public fn

    ///--------------------------------------------------------------------------------
    /// Input: Note that we Assume the spacing of both m_RGBImage and
    /// m_binaryObjectMask are correctly set to mpp
    void setInputRGBImage(itkRGBImageType::Pointer inputRGBImage) {m_RGBImage = inputRGBImage;}
    void setObjectBinaryMask(itkBinaryMaskImageType::Pointer binaryObjectMask) {m_binaryObjectMask = binaryObjectMask;}
    void setTopLeft(int64_t x, int64_t y) {m_TopLeftX = x; m_TopLeftY = y;}

    std::vector< std::vector<FeatureValueType> > getFeatures();

    void update();
    ///================================================================================



    ///--------------------------------------------------------------------------------
    /// Output
    void outputFeaturesToFile(std::string outputFeaturesFileName);
    void outputFeaturesToConsole();
    ///================================================================================

    /// public fn, end
    ////////////////////////////////////////////////////////////////////////////////


  private:
    std::ostream& m_outputStream;

    ////////////////////////////////////////////////////////////////////////////////
    /// private data

    ///--------------------------------------------------------------------------------
    /// Input data
    itkRGBImageType::Pointer m_RGBImage;
    itkBinaryMaskImageType::Pointer m_binaryObjectMask;
    int64_t m_TopLeftX; // the x of the top left cornor of this tile in the WSI, in index (pixel) space
    int64_t m_TopLeftY;
    ///================================================================================


    ///--------------------------------------------------------------------------------
    /// Computed data
    itkLabelImageType::Pointer m_objectLabelImage;
    std::vector<std::string> m_featureNames;
    std::vector< std::vector<FeatureValueType> > m_featuresOfAllObjects;

    int64_t m_totalNumberOfConnectedComponents;
    ///================================================================================

    bool m_allDone;
    /// private data, end
    ////////////////////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////////////////////
    /// private fn
    void _init();
    void _computeFeaturesForAllObjects();
    /// private fn
    ////////////////////////////////////////////////////////////////////////////////
  };

}// namespace gth818n


#endif
