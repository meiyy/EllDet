#pragma once
#include <opencv2/opencv.hpp>
#include <functional>
#include "Ellipse.h"
typedef std::vector<cv::Point> VP;
typedef std::vector<VP> VVP;
void drawVP(cv::Mat& img, const VP& seg, int thickness, bool draw_point, bool random_color);
void drawVVP(cv::Mat& img, const  VVP& segs, int thickness, bool draw_point, bool random_color);
void draw_ellipses(std::vector<Ellipse> ellipses, cv::Mat& img);
void draw_ellipses_all(std::vector<Ellipse> ellipses, cv::Mat& tmp);
double deg_between_vec(const cv::Point& a, const cv::Point& b);