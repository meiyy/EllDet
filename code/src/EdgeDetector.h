#pragma once
#include <vector>
#include <opencv2/opencv.hpp>

using cv::Mat1b;
typedef std::vector<cv::Point> VP;
typedef std::vector<std::vector<cv::Point>> VVP;

class EdgeDetector
{
public:
	static cv::Mat2f Canny(Mat1b image, VVP& edge_vector, Mat1b &edge_map, int min_length);
private:
	static void MarkEdgeCanny(const Mat1b &image, Mat1b &edge, cv::Mat2f& direction);
	static void ComputeThreshold(const Mat1b &dx, const Mat1b &dy, int &threshold_low,int &threshold_high);
	
};
