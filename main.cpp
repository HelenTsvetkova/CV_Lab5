#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/calib3d.hpp>

#include <opencv2/aruco.hpp>

using std::endl;
using std::cout;
using std::cin;
using std::vector;
using std::string;
using std::stoi;
using std::sort;

enum labPart {
    NONE,
    GENERATE_MARKER,
    DETECT_MARKER
};

int main(int argc, char* argv[])
{

    // lab part!!!!!1
    int labCurrentPart = DETECT_MARKER;

    // params
    int markersX = 5;
    int markersY = 5;
    int markerLength = 50;
    int markerSeparation = 10;
    int margins = markerSeparation;
    int dictionaryName = cv::aruco::DICT_6X6_250;
    cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(dictionaryName);

    switch (labCurrentPart) {
    case GENERATE_MARKER:
    {
        cv::Ptr<cv::aruco::GridBoard> board = cv::aruco::GridBoard::create(
                    markersX, markersY, markerLength, markerSeparation, dictionary);

        // show created board
        cv::Mat markerImage;
        cv::Size imageSize;
        imageSize.width = markersX * (markerLength + markerSeparation) - markerSeparation + 2 * margins;
        imageSize.height = markersY * (markerLength + markerSeparation) - markerSeparation + 2 * margins;
        board->draw(imageSize, markerImage, margins);
        cv::imwrite("../images/markers.png", markerImage);
        break;
    }

    case DETECT_MARKER:
    {
        if(argc == 1) {
            std::cout << "Please enter input photo file path" << std::endl;
            return 0;
        }
        std::string inputImagePath = argv[1];

        cv::Mat inputImage = cv::imread(inputImagePath);
        if(inputImage.empty()) {
            std::cout << "Error with input image" << std::endl;
            return 0;
        }
        std::vector<int> markerIds;
        std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
        cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
        cv::aruco::detectMarkers(inputImage, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);

        // calibrate camera
        std::vector<std::vector<cv::Point3f>> markerObjPoints;
        for(int i = 0; i < markersX; i++) {
            for(int j = 0; j < markersY; i++) {
                std::vector<cv::Point3f> buf;
                buf.push_back(cv::Point3f(margins + (markerLength + markerSeparation)*i, margins + (markerLength + markerSeparation)*j, 0));
                buf.push_back(cv::Point3f(margins + (markerLength + markerSeparation)*i + markerLength, margins + (markerLength + markerSeparation)*j, 0));
                buf.push_back(cv::Point3f(margins + (markerLength + markerSeparation)*i + markerLength, margins + (markerLength + markerSeparation)*j + markerLength, 0));
                buf.push_back(cv::Point3f(margins + (markerLength + markerSeparation)*i, margins + (markerLength + markerSeparation)*j + markerLength, 0));

                markerObjPoints.push_back(buf);
            }
        }

//        cv::Mat cameraMatrix, distCoeffs;
//        std::vector<cv::Mat> rvecs, tvecs;
//        cv::aruco::estimatePoseSingleMarkers(markerCorners, markerLength, cameraMatrix, distCoeffs, rvecs, tvecs);

//        cv::Mat outputImage = inputImage.clone();
//        if(markerIds.size() > 0) {
//            cv::aruco::drawDetectedMarkers(outputImage, markerCorners, markerIds);
//        }

//        for (int i = 0; i < rvecs.size(); ++i) {
//            auto rvec = rvecs[i];
//            auto tvec = tvecs[i];
//            cv::drawFrameAxes(outputImage, cameraMatrix, distCoeffs, rvec, tvec, 0.1);
//        }

//        cv::imshow(" markers ", outputImage);
//        cv::waitKey();

        break;
    }
    case NONE:
    {
        std::cout << "Please define part of work" << std::endl;
        break;
    }
    default:
        break;
    }

	return 0;
}
