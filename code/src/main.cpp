#include "EllipseDetector.h"
#include "util.h"
#include "EdgeDetector.h"
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;
using namespace cv;


int main()
{
	auto start = clock();
	EllipseDetector ellipse_Detector;
	std::vector<Ellipse> ellipses;
	cv::Mat img = cv::imread(R"(F:\projects\code_online\a.jpg)");
	ellipses = ellipse_Detector.DetectImage(img);
	cv::Mat3b img0 = ellipse_Detector.image();
	draw_ellipses_all(ellipses, img0);
	printf("Time: %.2f ms\n", (clock() - start) * 1000.0 / CLOCKS_PER_SEC);
	cv::imshow("Result", img0);
	cv::waitKey();
	return 0;
}