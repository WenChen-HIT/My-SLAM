#ifndef OBJECTTRACKER_H
#define OBJECTTRACKER_H

#include <opencv2/videoio.hpp>
#include <opencv2/opencv.hpp>
#include <vector>
#include <iostream>
#include <iomanip>
#include "stats.h" // Stats structure definition
#include "utils.h" // Drawing and printing functions
#include "Thirdparty/darknet/src/object.h"
#include "HungarianAlg.h"
#include "ORBextractor.h"
#include "ORBmatcher.h"

using namespace std;
using namespace cv;

typedef std::vector<DetectedObject> vObjects;

class ObjectTracker
{
public:
    ObjectTracker(const float maxdistTh, const cv::Size frameSize_);
    ~ObjectTracker();

    void grabImgWithObjects(cv::Mat& frame, vObjects& vCurrentObjects);
    double CalcDistJaccard(cv::Rect& current, cv::Rect& former);

    std::vector<int> CalculateAssignment(vObjects& vFormerObjects,
                                         vObjects& vCurrentObjects);

    void CutInBiggestBox(vObjects& vObjects_, cv::Rect& FrameBox);

protected:
    //config parameters
    const float dist_thres;
    const cv::Size frameSize;
    long unsigned int frameId;
    long unsigned int trackId;

    ORB_SLAM2::ORBextractor* detector;
    ORB_SLAM2::ORBmatcher* matcher;

    // Record overall Box information
    std::vector<std::pair<int, vObjects>>* frameObject;
    std::vector<std::pair<int, std::vector<int>>>* frameObjectID;

    // Record last frame's box information
    vObjects mvlastDetecedBox;
    std::vector<int> mvnLastTrackObjectID;

    AssignmentProblemSolver* APS;

    string imgStorePath;

    DetectedObject* initObject;

    cv::VideoWriter* writeFrame;


};

#endif