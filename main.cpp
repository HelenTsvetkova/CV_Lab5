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

    case CALIBRATE_CAMERA:
    {

        // данные для калибровки
        cv::Mat cameraMatrix, distCoeffs;
        std::vector<std::vector<cv::Point3f>> calibObjPoints;
        std::vector<std::vector<cv::Point2f>> calibImgPoints;
        cv::Size calibImgSize;
        cv::Size calibBoardSize(9, 6);
        float gridSize = 27;

        std::vector<cv::Point3f> objPointsBuf;
        for(size_t y = 0; y < calibBoardSize.height; y++) {
            for(size_t x = 0; x < calibBoardSize.width; x++) {
                objPointsBuf.push_back(cv::Point3f(x * gridSize, y * gridSize, 0.));
            }
        }

        // открываем видео
        cv::VideoCapture chessboardVideo(0);
        if(!chessboardVideo.isOpened()){
            std::cout << "Error opening video." << endl;
            return 0;
        }
        cv::Mat chessboardImage;
        chessboardVideo >> chessboardImage;
        calibImgSize = chessboardImage.size();


        while(1) {

            chessboardVideo >> chessboardImage;
            if (chessboardImage.empty()) {
                break;
            }

            std::vector<cv::Point2f> cornersBuf;
            bool found = cv::findChessboardCorners(chessboardImage, calibBoardSize, cornersBuf);
            cv::Mat showImage = chessboardImage.clone();
            if(found) {
                cv::drawChessboardCorners(showImage, calibBoardSize, cornersBuf, found);
                calibImgPoints.push_back(cornersBuf);
                calibObjPoints.push_back(objPointsBuf);
            }

            cv::imshow( "Markers", showImage );
            char c = (char)cv::waitKey(200);
            if(c == 27) {
                break;
            }
        }

        std::cout << "Num of photoes : " << calibImgPoints.size() << std::endl;
        std::vector<cv::Mat> rvecs, tvecs;
        cv::calibrateCamera(calibObjPoints, calibImgPoints, calibImgSize, cameraMatrix, distCoeffs, rvecs, tvecs);

        // проверка калибровки камеры
        std::cout << "imageSize : " << calibImgSize << std::endl;
        std::cout << "cameraMatrix : \n" << cameraMatrix << std::endl;
        std::cout << "distCoeffs : \n" << distCoeffs << std::endl;

        cv::FileStorage fs("../camera.xml", cv::FileStorage::WRITE);
        fs << "cameraMatrix" << cameraMatrix;
        fs << "distCoeffs" << distCoeffs;

        chessboardVideo.release();
        cv::destroyAllWindows();
        break;
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
        cv::FileStorage fs("../camera.xml", cv::FileStorage::READ);
        cv::Mat cameraMatrix, distCoeffs;
        fs["cameraMatrix"] >> cameraMatrix;
        fs["distCoeffs"] >> distCoeffs;
        std::vector<cv::Vec3d> rvecs, tvecs;
        cv::aruco::estimatePoseSingleMarkers(markerCorners, markerLength, cameraMatrix, distCoeffs, rvecs, tvecs);

        // проверка ориентации маркеров
        for (int i = 0; i < rvecs.size(); ++i) {
            auto rvec = rvecs[i];
            auto tvec = tvecs[i];
            cv::drawFrameAxes(outputImage, cameraMatrix, distCoeffs, rvec, tvec, 15);

        }

        cv::resize(outputImage, outputImage, cv::Size(), 2, 2);
        cv::imshow("estimated ", outputImage);
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
