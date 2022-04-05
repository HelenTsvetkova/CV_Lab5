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
    DETECT_MARKER,
    CALIBRATE_CAMERA
};

int main(int argc, char* argv[])
{

    // lab part!!!!!1
    int labCurrentPart = CALIBRATE_CAMERA;

    // params
    int markersX = 5;
    int markersY = 5;
    int markerLength = 50;
    int markerSeparation = 10;
    int margins = markerSeparation;
    int dictionaryName = cv::aruco::DICT_6X6_250;
    cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(dictionaryName);

    cv::Mat cameraMatrix, distCoeffs;

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

    case CALIBRATE_CAMERA:
    {
        // мировые координаты точек маркеров на доске
        std::vector<cv::Point3f> objPointsBuf (markersX * markersY * 4);
        int markerNum = markersX * markersY * 4;
        for(int j = 0; j < markersY; j++) {
            for(int i = 0; i < markersX; i++) {
                markerNum -= 4;
                objPointsBuf[markerNum] = cv::Point3f(margins*(i+1) + markerLength*i, margins*(j+1) + markerLength*j, 0);
                objPointsBuf[markerNum + 1] = cv::Point3f(margins*(i+1) + markerLength*(i+1), margins*(j+1) + markerLength*j, 0);
                objPointsBuf[markerNum + 2] = cv::Point3f(margins*(i+1) + markerLength*(i+1), margins*(j+1) + markerLength*(j+1), 0);
                objPointsBuf[markerNum + 3] = cv::Point3f(margins*(i+1) + markerLength*i, margins*(j+1) + markerLength*(j+1), 0);
            }
        }

        // массивы для калибровки
        std::vector<std::vector<cv::Point3f>> calibObjPoints;
        std::vector<std::vector<cv::Point2f>> calibImgPoints;
        cv::Size calibSize;

        // открываем видео
        cv::VideoCapture markersVideo("../images/markersVideo.mp4");
        if(!markersVideo.isOpened()){
            std::cout << "Error opening video markersVideo.mp4." << endl;
            return 0;
        }
        cv::Mat markersImage;
        markersVideo >> markersImage;
        calibSize = markersImage.size();


        while(1) {

            markersVideo >> markersImage;
            if (markersImage.empty()) {
                break;
            }

            // детектируем маркеры
            std::vector<int> markerIds;
            std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
            cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
            cv::aruco::detectMarkers(markersImage, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);

            // отрисовка детектированных маркеров
            cv::Mat showImage;
            showImage = markersImage.clone();
            if(markerIds.size() > 0) {
                cv::aruco::drawDetectedMarkers(showImage, markerCorners, markerIds);
            }

            // если детектированы все маркеры - сохраняем результат для дальнейшей калибровки
            if(markerIds.size() == markersX * markersY) {
                std::vector<cv::Point2f> imgPointsBuf;
                for(size_t i = 0; i < markerCorners.size(); i++) {
                    imgPointsBuf.push_back(markerCorners[i][0]);
                    imgPointsBuf.push_back(markerCorners[i][1]);
                    imgPointsBuf.push_back(markerCorners[i][2]);
                    imgPointsBuf.push_back(markerCorners[i][3]);
                }
                calibImgPoints.push_back(imgPointsBuf);
                calibObjPoints.push_back(objPointsBuf);
            }

            cv::imshow( "Markers", showImage );
            char c = (char)cv::waitKey(25);
            if(c == 27) {
                break;
            }
        }

        std::vector<cv::Mat> rvecs, tvecs;
        cv::calibrateCamera(calibObjPoints, calibImgPoints, calibSize, cameraMatrix, distCoeffs, rvecs, tvecs);

        // проверка калибровки камеры
        std::cout << "imageSize : " << calibSize << std::endl;
        std::cout << "cameraMatrix : \n" << cameraMatrix << std::endl;
        std::cout << "distCoeffs : \n" << distCoeffs << std::endl;

        markersVideo.release();
        cv::destroyAllWindows();
        /****************************************************************************************/
//        break;
        labCurrentPart = DETECT_MARKER;
        /****************************************************************************************/
    }

    case DETECT_MARKER:
    {

        // входное изображение
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

        // детектируем маркеры
        std::vector<int> markerIds;
        std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;
        cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
        cv::aruco::detectMarkers(inputImage, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);

        // проверка детектированных маркеров
        cv::Mat outputImage = inputImage.clone();

        if(markerIds.size() > 0) {
            cv::aruco::drawDetectedMarkers(outputImage, markerCorners, markerIds);
        }
        cv::imshow(" markers ", outputImage);
        cv::waitKey();

        // находим ориентацию маркеров
        std::vector<cv::Mat> rvecs, tvecs;
        cv::aruco::estimatePoseSingleMarkers(markerCorners, markerLength, cameraMatrix, distCoeffs, rvecs, tvecs);

        // проверка ориентации маркеров
        for (int i = 0; i < rvecs.size(); ++i) {
            auto rvec = rvecs[i];
            auto tvec = tvecs[i];
            cv::drawFrameAxes(outputImage, cameraMatrix, distCoeffs, rvec, tvec, 0.1);
        }

        cv::imshow(" markers ", outputImage);
        cv::waitKey();

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
