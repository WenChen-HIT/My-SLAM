/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/


#include<iostream>
#include<algorithm>
#include<fstream>
#include<chrono>
#include<iomanip>

#include<opencv2/core/core.hpp>

#include"System.h"

//darknet
#include"Thirdparty/darknet/src/yolo.h"
#include <string>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <fstream>
#include <vector>

#include "Converter.h"

using namespace std;
using namespace cv;

void LoadImages_robotcar(const string &strSequence, vector<string> &vstrImageFilenames,
                vector<double> &vTimestamps);
void GetGTFile(vector<double> &vTimestamps);

int main(int argc, char **argv)
{
    if(argc != 5)
    {
        cerr << endl << "Usage: ./mono_robotcar path_to_vocabulary path_to_settings path_to_yolo_settings path_to_sequence" << endl;
        return 1;
    }
    // Retrieve paths to images, load Image file path and time of every frame (second)
    vector<string> vstrImageFilenames;
    vector<double> vTimestamps;
    LoadImages_robotcar(string(argv[4]), vstrImageFilenames, vTimestamps);
//    GetGTFile(vTimestamps);
    int nImages = vstrImageFilenames.size();

    // Create SLAM system. It initializes all system threads and gets ready to process frames.

    ORB_SLAM2::System SLAM(argv[1],argv[2],argv[3],ORB_SLAM2::System::MONOCULAR,true);


    // Vector for tracking time statistics
    vector<float> vTimesTrack;
    vTimesTrack.resize(nImages);
    cout << endl << "-------" << endl;
    cout << "Start processing sequence ..." << endl;
    cout << "Images in the sequence: " << nImages << endl << endl;

    ofstream timeRecordfile("./timeRec.txt");
    if(!timeRecordfile.is_open()){
        cout << "ERROR: cannot record time for every frame" << endl;
        exit(1);
    }

    // Main loop
    cv::Mat im;
    for(int ni=0; ni<nImages; ni++)
    {
        // Read image from file
        im = cv::imread(vstrImageFilenames[ni],CV_LOAD_IMAGE_UNCHANGED);
        double tframe = vTimestamps[ni];
        if(im.empty())
        {
            cerr << endl << "Failed to load image at: " << vstrImageFilenames[ni] << endl;
            return 1;
        }

#ifdef COMPILEDWITHC11
        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
#else
        std::chrono::monotonic_clock::time_point t1 = std::chrono::monotonic_clock::now();
#endif

        // Pass the image to the SLAM system
        SLAM.TrackMonocular(im,tframe);

#ifdef COMPILEDWITHC11
        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
#else
        std::chrono::monotonic_clock::time_point t2 = std::chrono::monotonic_clock::now();
#endif

        double ttrack= std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1).count();

        std::cout << "Tracking this frame costs: " << ttrack << "seconds" << endl << endl;
        vTimesTrack[ni]=ttrack;

        timeRecordfile << ttrack << endl;
        // Wait to load the next frame
        double T=0;
//        if(ni<nImages-1)
//            T = vTimestamps[ni+1]-tframe;
//        else if(ni>0)
//            T = tframe-vTimestamps[ni-1];

        if(ttrack<T)
            usleep((T-ttrack)*1e6);
    }

    timeRecordfile.close();

    // Stop all threads
    SLAM.Shutdown();

    //saving map
    const string path = SLAM.GetMapPath();
    if(path.compare("Localization") != 0){
        std::cout << "Saving Map" << std::endl;
        SLAM.SaveMap(path);
        std::cout << "Map saving done!"<< std::endl;
    }
    // Tracking time statistics
    sort(vTimesTrack.begin(),vTimesTrack.end());
    float totaltime = 0;
    for(int ni=0; ni<nImages; ni++)
    {
        totaltime+=vTimesTrack[ni];
    }
    cout << "-------" << endl << endl;
    cout << "median tracking time: " << vTimesTrack[nImages/2] << endl;
    cout << "mean tracking time: " << totaltime/nImages << endl;

    // Save camera trajectory
    SLAM.SaveTrajectoryKITTI("YOLOTrajectory05Mono.txt");
    return 0;
}

void LoadImages_robotcar(const string &strPathToSequence, vector<string> &vstrImageFilenames, vector<double> &vTimestamps)
{
    ifstream fTimes;
    string strPathTimeFile = strPathToSequence + "time.txt";
    fTimes.open(strPathTimeFile.c_str());
    if(!fTimes.is_open()){
        cerr << "Cannot open " << strPathTimeFile << endl;
        exit(0);
    }
    while(!fTimes.eof())
    {
        string s;
        getline(fTimes,s);
        if(!s.empty())
        {
            stringstream ss;
            ss << s;
            double t;
            ss >> t;
            vTimestamps.push_back(t);
        }
    }

    string strPrefixLeft = strPathToSequence + "imagePath.txt";
    ifstream pathFile(strPrefixLeft.c_str());
    if(!pathFile.is_open()){
        cerr << "Cannot open " << strPrefixLeft << endl;
        exit(0);
    }

    const int nTimes = vTimestamps.size();
    vstrImageFilenames.resize(nTimes);

    for(int i=0; i<nTimes; i++)
    {
        string s;
        getline(pathFile, s);
        if(!s.empty()){
            vstrImageFilenames[i] = s;
        }
    }
    fTimes.close();
    pathFile.close();
}

void GetGTFile(vector<double> &vTimestamps)
{
    ifstream fGTIn;
    string strSequence = "/home/wchen/evaluate_ate_scale-master/data_odometry_poses/dataset/poses/";
    string strPathGTFile =  strSequence + "05.txt";
     string strPathGTOFile = strSequence + "05_.txt";

    ofstream f;
    f.open(strPathGTOFile.c_str());
    f << fixed;

    fGTIn.open(strPathGTFile.c_str());

    if(!fGTIn.is_open()){
//        cerr << "Cannot open " << strPathTimeFile << endl;
        exit(0);
    }

    int i = 0;
    while(!fGTIn.eof())
    {
        string s;
        getline(fGTIn,s);
        cv::Mat R = cv::Mat::ones(3, 3, CV_32F);
        cv::Mat t = cv::Mat::ones(1, 3, CV_32F);
        if(!s.empty())
        {
            stringstream ss;
            ss << s;
            ss >> R.at<float>(0,0) >> R.at<float>(0,1) >> R.at<float>(0,2) >> t.at<float>(0)
                                   >> R.at<float>(1,0) >> R.at<float>(1,1) >> R.at<float>(1,2) >> t.at<float>(1)
                                   >> R.at<float>(2,0) >> R.at<float>(2,1) >> R.at<float>(2,2) >> t.at<float>(2);

            std::cout << R << t << std::endl;

            vector<float> q = ORB_SLAM2::Converter::toQuaternion(R);

            f << setprecision(6) << vTimestamps.at(i) << setprecision(7) << " " << t.at<float>(0) << " " << t.at<float>(1) << " " << t.at<float>(2)
              << " " << q[0] << " " << q[1] << " " << q[2] << " " << q[3] << endl;

        }
        i++;
    }
}
