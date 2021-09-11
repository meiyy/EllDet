#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#include <fstream>

class CurveSimplifier
{
public:
	std::vector<cv::Point> SimplifyRDP(const std::vector<cv::Point>& edgeList,std::vector<int> &pos);
	std::vector<cv::Point> SimplifyCV(const std::vector<cv::Point>& edgeList);
};

