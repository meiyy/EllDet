#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#include "Ellipse.h"
#include <unordered_map>
#include "EdgeDetector.h"


class Digraph
{
	class edge {
	public:
		int u, v, nxt;
	};
public:
	std::vector<int> he;
	std::vector<edge> es;
	void init(int n)
	{
		he.assign(n, -1);
		es.clear();
	}
	void adde(int u, int v)
	{
		es.push_back(edge{ u,v,he[u] });
		he[u] = es.size() - 1;
	}
	bool exist(int u, int v)
	{
		for (int i = he[u]; i != -1; i = es[i].nxt)
		{
			if (es[i].v == v)
				return true;
		}
		return false;
	}
};

struct DetectionInfo
{
	DetectionInfo() {}
	DetectionInfo(const cv::Mat3b &img) :original_img(img.clone()),num_arcs(0) {}
	cv::Mat3b original_img;
	cv::Mat1b gray_blured_img;
	cv::Mat1b edge_map;
	cv::Mat2f direction_mat;
	VVP curves;
	VVP arcs;
	std::vector<double> angles;
	int num_arcs;
	std::vector<Ellipse> ellipses;
	Digraph dg;
};

class EllipseDetector
{
public:
	EllipseDetector(
		int min_length = 29,
		double distance_to_ellipse_contour = 1.94,
		double max_angle = 9.95,
		double min_score = 0.71,
		double pair_length_ratio = 9.24,
		double pair_dis_ratio = 19.25,
		double aspect_ratio = 26.54,
		double split_angle = 60,
		double ellipse_cluster_distance = 10.62
	) :
		min_length_(min_length),
		min_score_(min_score),
		distance_to_ellipse_contour_(distance_to_ellipse_contour),
		max_angle_(max_angle),
		pair_length_ratio(pair_length_ratio),
		pair_dis_ratio(pair_dis_ratio),
		aspect_ratio_(aspect_ratio),
		split_angle_(split_angle),
		ellipse_cluster_distance_(ellipse_cluster_distance)
	{

	}

	std::vector<Ellipse> DetectImage(const cv::Mat3b &image);
	std::vector<Ellipse> operator()(const cv::Mat3b& image);

	cv::Mat image()const{return info_.original_img.clone();}
	bool DfsEnumerateArcs(int start,int u,std::vector<int> &vis,std::vector<int> &nos);
	void SplitCurvesToArcs();
	double IsGoodArc(const std::vector<cv::Point>& arc);
	bool CanFormArcPair(int e1,int e2);
	void EnumerateArcs();
	Ellipse FitEllipses(const std::vector<int>& arc_ids,const VVP& arcs, double cover = 0.4);
	double ValidateEllipse(const Ellipse &ell, const VVP& arcs,double cover);
	void ClusterEllipses();
	void BuildDigraph();
	void MakeArcsCounterClockwise();
	void Preprocess();
	void DetectEdge();
	bool CheckConvex(const cv::Point& s1, const cv::Point& m1, const cv::Point& n1, const cv::Point& e1,
		const cv::Point& s2, const cv::Point& m2, const cv::Point& n2, const cv::Point& e2);
	double EllipseDistance(const Ellipse& a, const Ellipse& b);
	void LocalGroup(VVP& as, cv::Size sz);

	int min_length_;
	double min_score_;
	double distance_to_ellipse_contour_;
	double max_angle_;
	double pair_length_ratio;
	double pair_dis_ratio;
	double split_angle_;
	double aspect_ratio_;
	double ellipse_cluster_distance_;

	DetectionInfo info_;
};

