#include "util.h"
#include <filesystem>
#include <string>
#include <unordered_set>
#include <fstream>
using namespace std;
using namespace std::filesystem;
using namespace cv;

void drawVP(Mat& img,const VP& seg, int thickness, bool draw_point, bool random_color)
{
	Scalar color(0, 255, 0), point_color(0, 0, 255);
	if (random_color)
		color = Scalar(44 + rand() % 211, 44 + rand() % 211, 44 + rand() % 211);
	for (int i = 1; i < seg.size(); i++)
	{
		line(img, seg[i - 1], seg[i], color, thickness,LINE_AA);
	}
	if (draw_point)
	{
		for (int i = 0; i < seg.size(); i++)
		{
			circle(img, seg[i], thickness, point_color, -1, LINE_AA);
		}
	}
}

void drawVVP(Mat& img, const VVP& segs, int thickness, bool draw_point, bool random_color)
{
	for (auto seg : segs)
	{
		drawVP(img, seg, thickness, draw_point, random_color);
	}
}

void draw_ellipses(std::vector<Ellipse> ellipses, Mat& img)
{
	for (auto& ell : ellipses)
	{
		auto tmp = img.clone();
		ellipse(tmp, ell.center, Size(cvRound(ell.a), cvRound(ell.b)), ell.theta, 0.0, 360.0,
			Scalar(0, 255, 0), 2, LINE_AA);
		imshow("tmp", tmp);
		waitKey();
	}
}

void draw_ellipses_all(std::vector<Ellipse> ellipses, Mat& tmp)
{
	for (auto& ell : ellipses)
	{
		ellipse(tmp, ell.center, Size(cvRound(ell.a), cvRound(ell.b)), ell.theta, 0.0, 360.0,
			Scalar(0, 255, 0), 2, LINE_AA);
	}
}

double deg_between_vec(const Point& a, const Point& b)
{
	assert(norm(a) > 0 && norm(b) > 0);
	double res = a.cross(b) / norm(a) / norm(b);
	if (res < -1)res = -1;
	if (res > 1)res = 1;
	return asin(res) / CV_PI * 180;
}
