#include "EllipseDetector.h"
#include "EdgeDetector.h"
#include "CurveSimplifier.h"
#include "util.h"
#include <unordered_map>
#include <numeric>
using namespace std;
using namespace cv;

bool EllipseDetector::CheckConvex(const Point& s1, const Point& m1, const Point& n1, const Point& e1,
	const Point& s2, const Point& m2, const Point& n2, const Point& e2)
{
	if ((e1 - n1).cross(s2 - e1) < 0)
		return false;
	if ((s2 - e1).cross(m2 - s2) < 0)
		return false;
	if ((e2 - n2).cross(s1 - e2) < 0)
		return false;
	if ((s1 - e2).cross(m1 - s1) < 0)
		return false;
	return true;
}

double EllipseDetector::IsGoodArc(const std::vector<Point>& arc)
{
	if (arc.size() < min_length_)
		return -2;
	auto rect = minAreaRect(arc);
	double ratio = rect.size.height / rect.size.width;
	if (ratio > aspect_ratio_ || ratio < 1.0 / aspect_ratio_)
		return -2;
	auto ell = FitEllipses({ 0 }, { arc },0);
	if (ell.score < min_score_)
		return -2;
	auto v = Point2d(arc[arc.size() / 2]) - ell.center;
	double angle = atan2(v.y, v.x)+CV_PI;//0~2PI

	return angle;
}

void EllipseDetector::SplitCurvesToArcs()
{
	const VVP& curves = info_.curves;
	VVP& arcs = info_.arcs;
	auto& ags = info_.angles;

	CurveSimplifier curve_simplifier;
	for (auto& i : curves)
	{
		vector<int> pos;
		auto L = curve_simplifier.SimplifyRDP(i, pos);

		if (L.size() < 3)
		{
			continue;
		}
		std::vector<double> angle;
		double a_max = 0;
		for (int j = 2; j < L.size(); j++)
		{
			double cur_angle = deg_between_vec(L[j - 1] - L[j - 2], L[j] - L[j - 1]);
			angle.push_back(cur_angle);
			a_max = max(a_max, abs(cur_angle));
		}

		if (a_max < 60)
		{
			auto ell = FitEllipses({ 0 }, { i }, 0.6);
			if (ell.score > min_score_)
			{
				info_.ellipses.push_back(ell);

				continue;
			}
		}

		int start = 0, start_k = 0;

		std::vector<cv::Point> sub_arc;
		for (int j = 0; j < angle.size();)
		{
			if (abs(angle[j]) > split_angle_)
			{
				j++;
				int k = pos[j];
				sub_arc = std::vector<cv::Point>(i.begin() + start_k, i.begin() + k + 1);

				start = j;
				start_k = k;
				double ra = IsGoodArc(sub_arc);
				if (ra > -1)
				{
					arcs.push_back(sub_arc);
					ags.push_back(ra);
				}
			}
			else if (angle[j] * angle[start] < 0)
			{
				// need to decide segment L[j~j+1] belong to the former or latter part
				if (abs(angle[j - 1]) < abs(angle[j]))
				{
					j++;
				}
				int k = pos[j];
				sub_arc = std::vector<cv::Point>(i.begin() + start_k, i.begin() + k + 1);

				start = j;
				start_k = k;
				double ra = IsGoodArc(sub_arc);
				if (ra > -1)
				{
					arcs.push_back(sub_arc);
					ags.push_back(ra);
				}
			}
			else
				j++;

		}
		if (i.size() - start_k > min_length_)
		{
			sub_arc = std::vector<cv::Point>(i.begin() + start_k, i.end());

			double ra = IsGoodArc(sub_arc);
			if (ra > -1)
			{
				arcs.push_back(sub_arc);
				ags.push_back(ra);
			}
		}
	}

	info_.num_arcs = info_.arcs.size();
}

double EllipseDetector::EllipseDistance(const Ellipse& a, const Ellipse& b)
{
	double t = min(abs(a.a - a.b) / (a.a + a.b), abs(b.a - b.b) / (b.a + b.b));
	t = t * t * t;
	return sqrt(
		(a.center.x - b.center.x) * (a.center.x - b.center.x) +
		(a.center.y - b.center.y) * (a.center.y - b.center.y) +
		(a.a - b.a) * (a.a - b.a) +
		(a.b - b.b) * (a.b - b.b) +
		(a.theta - b.theta) * (a.theta - b.theta) * t
	);
}

bool EllipseDetector::CanFormArcPair(int ne1,int ne2)
{
	const VP& e1 = info_.arcs[ne1], & e2 = info_.arcs[ne2];
	// short arc should not be with a long arc
	double ratio=(double)e1.size()/e2.size();
	if (ratio > pair_length_ratio || ratio < 1.0 / pair_length_ratio)
		return false;
	
	// e1 and e2 is too far compared to there length
	if(norm(e1[e1.size()/2]-e2[e2.size()/2])/min(e1.size(),e2.size())>pair_dis_ratio)
		return false;

	//e1 e2 should form convex
	int n = 7;
	if(!CheckConvex(e1[(e1.size()-1) *1/ n], e1[(e1.size() - 1) * 2 / n], e1[(e1.size() - 1) * (n-2) / n], e1[(e1.size() - 1) * (n-1) / n],
		e2[(e2.size() - 1) * (1) / n], e2[(e2.size() - 1) * 2 / n], e2[(e2.size() - 1) * (n - 2) / n],e2[(e2.size() - 1) * (n-1) / n]))
		return false;


	auto ell = FitEllipses({ 0 }, { e1,e2 }, 0);
	if (ell.score < min_score_)
		return false;

	return true;
}

void EllipseDetector::EnumerateArcs()
{
	auto& dg = info_.dg;
	vector<int> vis(dg.he.size(), 0);
	for (int i = 0; i < dg.he.size(); i++)
	{
		if (info_.arcs[i].size() > 3u * min_length_)
		{
			auto ell = FitEllipses({ i }, info_.arcs,0.58);
			if (ell.score > 0.8)
			{
				info_.ellipses.push_back(ell);
				vis[i] = true;
			}
		}
	}
	for (int i = 0; i < dg.he.size(); i++)
	{
		auto vis_t = vis;
		if (!vis_t[i])
		{
			vis_t[i] = 1;
			vector<int> nos;
			nos.reserve(10);
			if (DfsEnumerateArcs(i, i, vis_t, nos))
			{
				for (auto j : nos)
				{
					vis[j] = true;
				}
			}
		}
	}

}


bool EllipseDetector::DfsEnumerateArcs(int start, int u, std::vector<int>& vis, std::vector<int>& nos)
{
	bool f = 1;
	if (f)
	{
		nos.push_back(u);
		if (nos.size() == 4)
		{
			nos.pop_back();
			return false;
		}
		auto& dg = info_.dg;
		if (nos.size() == 2)
		{
			auto ell = FitEllipses(nos, info_.arcs);
			if (ell.score > min_score_)
			{
				info_.ellipses.push_back(ell);
				return true;
			}
		}
		for (int i = dg.he[u]; i != -1; i = dg.es[i].nxt)
		{
			int v = dg.es[i].v;
			if (!vis[v])
			{
				vis[v] = 1;
				if (DfsEnumerateArcs(start, v, vis, nos))
				{
					return true;
				}

				vis[v] = 0;
			}
			else if (v == start)
			{
				auto ell = FitEllipses(nos, info_.arcs);
				if (ell.score > min_score_)
				{
					info_.ellipses.push_back(ell);
					return true;
				}
			}
		}
		nos.pop_back();
		return false;
	}
	else
	{
		nos.push_back(u);
		auto& dg = info_.dg;
		for (int i = dg.he[u]; i != -1; i = dg.es[i].nxt)
		{
			int v = dg.es[i].v;
			if (!vis[v])
			{
				vis[v] = 1;
				if (DfsEnumerateArcs(start, v, vis, nos))
				{
					return true;
				}
			}
			else if (v == start)
			{
				auto ell = FitEllipses(nos, info_.arcs);
				if (ell.score > min_score_)
				{
					info_.ellipses.push_back(ell);
					return true;
				}
			}
		}
		nos.pop_back();
		return false;
	}
}

Ellipse EllipseDetector::FitEllipses(const vector<int> &arc_ids,const VVP& arcs,double cover)
{
	// collect all the points
	int num_pt_all = 0;
	for(const auto &i:arc_ids)
	{
		num_pt_all += arcs[i].size();
	}
	VP pts;
	pts.reserve(num_pt_all);

	for (const auto& i : arc_ids)
	{
		pts.insert(pts.end(), arcs[i].begin(), arcs[i].end());
	}

	// fit and validate the ellipse
	const auto ell_rect = fitEllipse(pts);
	Ellipse ell{ell_rect.center,ell_rect.size.width/2.0,ell_rect.size.height/2.0,ell_rect.angle,1,arc_ids};
	ell.score = ValidateEllipse(ell,arcs, cover);
	return ell;
}

double EllipseDetector::ValidateEllipse(const Ellipse &ell, const VVP& arcs,double cover)
{
	const std::vector<int>& arc_ids = ell.ids;
	if(max(ell.b,ell.a)>max(info_.original_img.cols,info_.original_img.rows))
		return 0;
	if (min(ell.b, ell.a)<5)
		return 0;

	double L = CV_PI * (3 * (ell.a + ell.b) - sqrt((3 * ell.a + ell.b) * (ell.a + 3 * ell.b)));// approx ellipse circumference 

	float _cos = cos(-ell.theta*CV_PI/180);
	float _sin = sin(-ell.theta*CV_PI/180);

	float invA2 = 1.f / (ell.a * ell.a);
	float invB2 = 1.f / (ell.b * ell.b);

	auto count_on_ellipse=[ell,_cos,_sin,invA2,invB2,this](const VP &e)->double
	{
		if(e.size()==0)
			return 1;
		int counter_on_perimeter = 0;
		for (auto i:e)
		{
		auto tp = Point2d(i) - ell.center;
		float rx = (tp.x * _cos - tp.y * _sin);
		float ry = (tp.x * _sin + tp.y * _cos);
		float h = (rx * rx) * invA2 + (ry * ry) * invB2;
		//float d=norm(tp)*(1-1/sqrt(h));
		float d2 = (tp.x * tp.x + tp.y * tp.y) * (h * h * 0.25 - h * 0.5 + 0.25);//approx of above

		auto cdir = info_.direction_mat(i);
		Vec2f rdir(2 * rx * _cos * invA2 + 2 * ry * _sin * invB2, 2 * rx * -_sin * invA2 + 2 * ry * _cos * invB2);
		rdir /= norm(rdir);
		double angle = abs(acos(cdir.dot(rdir))) * 180.0 / CV_PI;
		if (angle > 90)
			angle = 180 - angle;
		if (d2 < distance_to_ellipse_contour_ && angle < max_angle_)
		{
			++counter_on_perimeter;
		}
		}
		return (double)counter_on_perimeter / e.size();
	};

	double average_score = 0;
	double pts_all=0;
	for (auto i : arc_ids)
	{
		double score_i = count_on_ellipse(arcs[i]);
		if(score_i<0.5)
		{
			return false;
		}
		average_score += arcs[i].size() * score_i;
		pts_all += arcs[i].size();
	}
	average_score /= pts_all;
	if (pts_all < L * cover)
		return 0;
	return average_score;
}

void EllipseDetector::ClusterEllipses()
{
	sort(info_.ellipses.begin(), info_.ellipses.end());
	if (info_.ellipses.size() < 2)return;

	vector<Ellipse> clusters;
	clusters.push_back(info_.ellipses[0]);
	set<int> st;

	for (int i = 1; i < info_.ellipses.size(); ++i)
	{
		const Ellipse& e1 = info_.ellipses[i];
		bool found_cluster = false;
		for (auto& e2 : clusters)
		{
			double dis = EllipseDistance(e1, e2);
			if (dis < ellipse_cluster_distance_)
			{
				found_cluster = true;
				break;
			}
		}
		if (!found_cluster)
		{
			clusters.push_back(e1);
		}
	}

	return info_.ellipses.swap(clusters);
};


void EllipseDetector::MakeArcsCounterClockwise()
{
	for (auto &i : info_.arcs)
	{
		auto v1 = i[i.size()/2] - i.front();
		auto v2 = i.back() - i[i.size() / 2];
		if (v1.cross(v2) < 0)
		{
			i = VP(i.rbegin(), i.rend());
		}
	}
	std::vector<std::pair<VP, double>> st;
	st.reserve(info_.arcs.size());
	for (int i = 0; i < info_.arcs.size(); i++)
	{
		st.push_back(std::make_pair(info_.arcs[i], info_.angles[i]));
	}

	std::sort(st.begin(), st.end(), [](const std::pair<VP, double> &a,const std::pair<VP, double> &b) {
		return a.second < b.second;
		});

	for (int i = 0; i < info_.arcs.size(); i++)
	{
		info_.arcs[i] = st[i].first;
		info_.angles[i] = st[i].second;
	}
}

void EllipseDetector::LocalGroup(VVP& as,cv::Size sz)
{
	vector<int> prev(as.size(), -1);
	vector<int> next(as.size(), -1);
	for (int i = 0; i < as.size(); i++)
	{
		for (int j = i+1; j < as.size(); j++)
		{
			if (as[i].back() == as[j].front() && next[i]!=-1 && prev[j]!=-1)
			{
				next[i] = j;
				prev[j] = i;
			}
			else if (as[i].front() == as[j].back() && next[j] != -1 && prev[i] != -1)
			{
				next[j] = i;
				prev[i] = j;
			}
		}
	}
	vector<int> vis(as.size(), 0);
}

void EllipseDetector::BuildDigraph()
{
	info_.dg.init(info_.num_arcs);
	for (int i = 0; i < info_.num_arcs; i++)
	{
		for (int j = i + 1; j < info_.num_arcs; j++)
		{
			if (CanFormArcPair(i, j))
			{

				double diff = info_.angles[i] - info_.angles[j];

				//auto tmp3 = info_.original_img.clone();
				//drawVVP(tmp3, { info_.arcs[i],info_.arcs[j] }, 2, 0, 1);
				//imshow("pair", tmp3);
				//waitKey();

				if (diff < 0)
					diff += CV_PI*2;
				if (diff < CV_PI)
				{
					info_.dg.adde(j, i);
				}
				else
				{
					info_.dg.adde(i, j);
				}
			}
		}
	}
}

void EllipseDetector::Preprocess()
{
	Mat1b tmp;
	cvtColor(info_.original_img, tmp, COLOR_BGR2GRAY);
	bilateralFilter(tmp, info_.gray_blured_img, 9, 90, 7);
	GaussianBlur(info_.gray_blured_img, info_.gray_blured_img,Size(3,3),1.0);
}

void EllipseDetector::DetectEdge()
{
	EdgeDetector edge_Detector;
	info_.direction_mat = edge_Detector.Canny(info_.gray_blured_img, info_.curves, info_.edge_map, min_length_);
}

std::vector<Ellipse> EllipseDetector::DetectImage(const Mat3b& image)
{
	info_ = DetectionInfo(image);
	Preprocess();
	DetectEdge();

	SplitCurvesToArcs();
	
	MakeArcsCounterClockwise();
	BuildDigraph();
	EnumerateArcs();
	ClusterEllipses();

	return info_.ellipses;
}

std::vector<Ellipse> EllipseDetector::operator()(const Mat3b& image)
{
	return DetectImage(image);
}