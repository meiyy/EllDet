#pragma once
#include <opencv2/opencv.hpp>
struct Ellipse
{
	cv::Point2d center;
	double a;
	double b;
	double theta;
	double score;
	bool operator<(const Ellipse& other) const
	{
		if (score == other.score)
		{
			float lhs_e = b / a;
			float rhs_e = other.b / other.a;
			if (lhs_e == rhs_e)
			{
				return false;
			}
			return lhs_e > rhs_e;
		}
		return score > other.score;
	};
	std::vector<int> ids;
};

