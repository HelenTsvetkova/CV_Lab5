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

void drawEdges(cv::Mat &outputImage, std::vector<cv::Point2f> lowerEdge, std::vector<cv::Point2f> upperEdge) {
    cv::line(outputImage,lowerEdge[0], upperEdge[0], cv::Scalar(255, 0, 0), 1.5);
    cv::line(outputImage,lowerEdge[1], upperEdge[1], cv::Scalar(255, 0, 0), 1.5);
    cv::line(outputImage,lowerEdge[2], upperEdge[2], cv::Scalar(255, 0, 0), 1.5);
    cv::line(outputImage,lowerEdge[3], upperEdge[3], cv::Scalar(255, 0, 0), 1.5);
}

void drawSquare(cv::Mat &outputImage, std::vector<cv::Point2f> squarePoints) {
    cv::line(outputImage, squarePoints[0], squarePoints[1], cv::Scalar(0,0,255), 2);
    cv::line(outputImage, squarePoints[1], squarePoints[2], cv::Scalar(0,0,255), 2);
    cv::line(outputImage, squarePoints[2], squarePoints[3], cv::Scalar(0,0,255), 2);
    cv::line(outputImage, squarePoints[3], squarePoints[0], cv::Scalar(0,0,255), 2);
}

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
    cv::Ptr<cv::aruco::GridBoard> board = cv::aruco::GridBoard::create(markersX, markersY, markerLength, markerSeparation, dictionary);


    switch (labCurrentPart) {
    case GENERATE_MARKER:
    {
        // show and save created board
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
        // загружаем параметры камеры
        cv::FileStorage fs("../camera.xml", cv::FileStorage::READ);
        cv::Mat cameraMatrix, distCoeffs;
        fs["cameraMatrix"] >> cameraMatrix;
        fs["distCoeffs"] >> distCoeffs;

        int flipImage = 0;

        // открываем видео
        cv::VideoCapture markersVideo;
        if(argc == 1) {
            std::cout << "Enter path to video (or just '0', if you want to use web camera) and necessity to flip (                         '1') or not ('0') input video." << endl;
        } else {
            if(argv[1][0] == '0') {
                markersVideo.open(0);
            } else {
                markersVideo.open(argv[1]);
            }
            flipImage = atoi(argv[2]);
        }

        if(!markersVideo.isOpened()){
            std::cout << "Error opening video." << endl;
            return 0;
        }

        while(1) {

            cv::Mat inputImage;
            markersVideo >> inputImage;
            if (inputImage.empty()) {
                break;
            }

            if(flipImage) {
                cv::Mat buf = inputImage.clone();
                cv::flip(buf, inputImage, 1);
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
            } else {
                cv::imshow("estimated ", outputImage);
                char c = (char)cv::waitKey(10);
                if(c == 27) {
                    break;
                }
                continue;
            }

            // находим ориентацию маркеров
            std::vector<cv::Vec3d> rvecs, tvecs;
            cv::aruco::estimatePoseSingleMarkers(markerCorners, markerLength, cameraMatrix, distCoeffs, rvecs, tvecs);

            // отрисовка ориентации маркеров
            for (int i = 0; i < rvecs.size(); i++) {
                auto rvec = rvecs[i];
                auto tvec = tvecs[i];
                cv::drawFrameAxes(outputImage, cameraMatrix, distCoeffs, rvec, tvec, 15);
            }

            // точки в 3d для отрисовки кубиков
            std::vector<std::vector<cv::Point3f>> cubeObjPoints;
            for(size_t i = 0; i < markerIds.size(); i++) {
                int id = markerIds[i];
                int x = id % markersX;
                int y = markersY - (id / markersY) - 1;
                std::vector<cv::Point3f> buf;
                buf.push_back(cv::Point3f(margins*(x+1) + markerLength*x, margins*(y+1) + markerLength*(y+1), markerLength));
                buf.push_back(cv::Point3f(margins*(x+1) + markerLength*(x+1), margins*(y+1) + markerLength*(y+1), markerLength));
                buf.push_back(cv::Point3f(margins*(x+1) + markerLength*(x+1), margins*(y+1) + markerLength*y, markerLength));
                buf.push_back(cv::Point3f(margins*(x+1) + markerLength*x, margins*(y+1) + markerLength*y, markerLength));
                cubeObjPoints.push_back(buf);
            }

            // рисуем кубики
            cv::Vec3d rvecBoard, tvecBoard;
            cv::aruco::estimatePoseBoard(markerCorners, markerIds, board, cameraMatrix, distCoeffs, rvecBoard, tvecBoard);
            cv::drawFrameAxes(outputImage, cameraMatrix, distCoeffs, rvecBoard, tvecBoard, 50);
            for(size_t i = 0; i < markerIds.size(); i++) {
                std::vector<cv::Point2f> buf;
                cv::projectPoints(cubeObjPoints[i], rvecBoard, tvecBoard, cameraMatrix, distCoeffs, buf);
                drawEdges(outputImage, markerCorners[i], buf);
            }

            for(size_t i = 0; i < markerIds.size(); i++) {
                std::vector<cv::Point2f> buf;
                cv::projectPoints(cubeObjPoints[i], rvecBoard, tvecBoard, cameraMatrix, distCoeffs, buf);
                drawSquare(outputImage, buf);
            }

            cv::resize(outputImage, outputImage, cv::Size(), 2, 2);
            cv::imshow("estimated ", outputImage);
            char c = (char)cv::waitKey(10);
            if(c == 27) {
                break;
            }
        }

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
