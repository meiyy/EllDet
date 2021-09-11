#include "CurveSimplifier.h"
#include <algorithm>
using std::vector;
using cv::Point;

struct maxLineDev
{
	float max_dev, index, D_temp, del_tol_max;
};

double function_digital_case_with_max_error(double ss)
{
	double max_del;
	if(ss<10)
		max_del=1.767349*pow(ss,-1.102022);
	else
		max_del=1.436644*pow(ss,-1.004084);
	return max_del;
}

maxLineDev maxlinedev_opt(vector<Point>::const_iterator begin,vector<Point>::const_iterator end)
{
	vector<double> dev;
	double rowSum = 0.0;
	const Point &front=*begin,&back=*end;
	const double temp1 = static_cast<double>(front.y - back.y) / (front.y * back.x - back.y * front.x);
	const double temp2 = static_cast<double>(front.x - back.x) / (front.x * back.y - back.x * front.y);
	if (isnan(temp1) && isnan(temp2))
	{
		for (auto k = begin; k <= end; k++)
		{
			dev.push_back(norm(*k-front));
		}
	}
	else if (isinf(temp1) && isinf(temp2))
	{
		double c = (double)front.x / front.y;
		double d = sqrt(c * c + 1);
		for (auto k = begin; k <= end; k++)
		{
			rowSum = (k->x - c * k->y) / d;
			dev.push_back(abs(rowSum));
		}
	}
	else
	{
		double deno = sqrt(temp1*temp1+temp2*temp2);
		for (auto k = begin; k <= end; k++)
		{
			double d = k->x * temp1 + k->y * temp2 - 1;
			rowSum = d / deno;
			dev.push_back(abs(rowSum));
		}
	}
	// find the maximum element and the corresponding index of dev
	auto maxPos = max_element(dev.begin(), dev.end());

	vector<double> devTemp;
	for (auto k = begin; k <= end; k++)
	{
		rowSum = norm(*k-front);
		devTemp.push_back(rowSum);
	}
	auto maxPosTemp = max_element(devTemp.begin(), devTemp.end());
	double S_max = *maxPosTemp;
	double del_phi_max = function_digital_case_with_max_error(S_max);
	double del_tol_max = tan(del_phi_max) * S_max;
	
	maxLineDev lineResult;
	lineResult.del_tol_max = del_tol_max;
	lineResult.D_temp = S_max;
	lineResult.index = maxPos-dev.begin();
	lineResult.max_dev = *maxPos;
	return lineResult;
}

std::vector<cv::Point> CurveSimplifier::SimplifyRDP(const std::vector<cv::Point>& edge, std::vector<int>& pos)
{
	vector<cv::Point> opt;
	if (edge.size()<3)
	{
		for (int i = 0; i < edge.size(); i++)
		{
			pos.push_back(i);
			opt.push_back(edge[i]);
		}
		return opt;
	}
	pos.push_back(0);
	opt.push_back(edge[0]);
	int first = 0;
	int last = edge.size()-1;
	while (first < last)
	{
		maxLineDev Result = maxlinedev_opt(edge.begin()+first, edge.begin()+last);
		while (Result.max_dev > Result.del_tol_max)
		{
			last = Result.index + first;
			if (first == last)
				break;
			Result = maxlinedev_opt(edge.begin()+first, edge.begin()+last);
		}
		if (last == first)
		{
			last= edge.size() - 1;
		}
		pos.push_back(last);
		opt.push_back(edge[last]);
		first = last; // reset first and last for next iteration
		last = edge.size()-1;
	}
	return opt;
}
