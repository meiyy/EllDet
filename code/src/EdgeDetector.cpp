#include "EdgeDetector.h"
#include "util.h"
using namespace cv;

constexpr static int dx[]{ 1,-1, 0, 0, 1, 1,-1,-1 };
constexpr static int dy[]{ 0, 0, 1,-1, 1,-1, 1,-1 };

void CollectingDfs(VP &seg, Mat1b& vis, Mat1b& corner, int x, int y)
{
	vis(y,x)=0;
	seg.push_back(Point(x,y));
	for(int i=0;i<8;i++)
	{
		int tx=x+dx[i],ty=y+dy[i];
		if(tx>=0 && tx < vis.cols && ty >=0 && ty < vis.rows)
		{
			double angle_change = 0;// acos(abs(grad(ty, tx).dot(grad(y, x)))) * 180.0 / CV_PI;
			if(vis(ty,tx) && angle_change<15)
			{
				if (corner(ty, tx))
				{
					seg.push_back(Point(tx, ty));
					return;
				}
				else
					CollectingDfs(seg,vis,corner,tx,ty);
				return;
			}
		}
	}
}

void Collecting(Mat1b& image,VVP& segments, int min_length)
{
	auto corner = image.clone();

	// for (int y = 0; y < image.rows; y++)
	// {
	// 	for (int x = 0; x < image.cols; x++)
	// 	{
	// 		if (!image(y, x))
	// 		{
	// 			continue;
	// 		}
	// 		int a = 0, b = 0;
	// 		for (int i = 0; i < 4; i++)
	// 		{
	// 			int tx = x + dx[i], ty = y + dy[i];
	// 			if (tx>=0&&ty>=0&&tx< image.cols&&ty< image.rows&&image(ty,tx))
	// 				a++;
	// 		}
	// 		for (int i = 4; i < 8; i++)
	// 		{
	// 			int tx = x + dx[i], ty = y + dy[i];
	// 			if (tx >= 0 && ty >= 0 && tx < image.cols && ty < image.rows && image(ty,tx))
	// 				b++;
	// 		}
	// 		if (a > 2 || b > 2)
	// 			corner(y, x) = 0;
	// 		else
	// 			corner(y, x) = 0;
	// 	}
	// }

	auto vis=image.clone();
	for(int y=0;y < vis.rows;y++)
	{
		for(int x=0;x<vis.cols;x++)
		{
			if(vis(y,x))
			{
				VP seg;
				CollectingDfs(seg,vis,corner,x,y);
				if(seg.size()>min_length)
				{
					if (seg.size() > 0 && corner(seg.front()))
					{
						vis(seg.front()) = 1;
						seg = VP(seg.begin() + 1, seg.end());
					}
					if (seg.size() > 0 && corner(seg.back()))
					{
						vis(seg.back()) = 1;
						seg.pop_back();
					}
					segments.push_back(seg);
				}
				else
				{
					for (auto& p : seg)
					{
						vis(p) = 1;
					}
				}
			}
		}
	}

}

cv::Mat2f EdgeDetector::Canny(Mat1b image, VVP& edge_vector, Mat1b& edge_map, int min_length)
{
	cv::Mat2f direction(image.size(),Vec2f(0,0));
	MarkEdgeCanny(image, edge_map,direction);
	Collecting(edge_map, edge_vector, min_length);
	return direction;
}

void EdgeDetector::MarkEdgeCanny(const Mat1b& image, Mat1b& edge, cv::Mat2f& direction)
{
	Mat1s dx,dy;
	Sobel(image,dx,CV_16S,1,0);
	Sobel(image,dy,CV_16S,0,1);
	int threshold_low,threshold_high;
	ComputeThreshold(dx,dy,threshold_low,threshold_high);

	cv::Canny(dx,dy, edge,threshold_low,threshold_high);
	
	for (int i = 0; i < edge.rows; ++i)
	{
		for (int j = 0; j < edge.cols; ++j)
		{
			if (edge(i,j)!=0 )
			{
				double len=sqrt(dx(i,j)*dx(i,j)+dy(i,j)*dy(i,j));
				if (len!=0)
				{
					direction(i, j)=Point2f(dx(i,j)/len,dy(i,j)/len);
				}
				else
					direction(i, j) = Point2f(0, 0);
				edge(i,j) = 255;
			}
			else
			{
				edge(i, j) = 0;
			}
		}
	}

}

void EdgeDetector::ComputeThreshold(const Mat1b& dx, const Mat1b& dy, int& threshold_low,
	int& threshold_high)
{
	Mat1f magGrad(dx.size());
	double maxGrad(0);
	double val(0);
	for (int i = 0; i < magGrad.rows; i++)
	{
		for (int j = 0; j < magGrad.cols; j++)
		{
			val = float(abs(dx(i, j)) + abs(dy(i, j)));
			magGrad(i, j) = val;
			maxGrad = max(maxGrad, val);
		}
	}
	//set magic numbers
	constexpr int NUM_BINS = 80;
	const double percent_of_pixels_not_edges = 0.93;
	const double threshold_ratio = 0.3;

	//compute histogram
	int bin_size = std::floor(maxGrad / double(NUM_BINS) + 0.5) + 1;
	if (bin_size < 1)
		bin_size = 1;
	int bins[NUM_BINS] = { 0 };
	for (int i = 0; i < magGrad.rows; i++)
	{
		for (int j = 0; j < magGrad.cols; j++)
		{
			bins[int(magGrad(i, j)) / bin_size]++;
		}
	}


	//% Select the thresholds
	double total = 0;
	double target = magGrad.rows * magGrad.cols * percent_of_pixels_not_edges;
	threshold_high = 0;
	while (total < target)
	{
		total += bins[threshold_high];
		threshold_high++;
	}
	threshold_high *= bin_size;

	total = 0;
	target = target * threshold_ratio;
	threshold_low = 0;
	while (total < target)
	{
		total += bins[threshold_low];
		threshold_low++;
	}
	threshold_low *= bin_size;

}

void findContour(const int Wise[8][2], const int antiWise[8][2], Mat1b& edge, int x, int y, int len, VVP& edgeContours)
{
	bool isEnd;
	int find_x, find_y;
	int move_x = x, move_y = y;
	VP oneContour, oneContourOpp;
	oneContour.push_back(Point(x, y));

	while (1)
	{
		isEnd = true;
		for (int i = 0; i < 8; i++)
		{
			find_x = move_x + Wise[i][0];
			find_y = move_y + Wise[i][1];
			if (edge(find_x, find_y))
			{
				edge(find_x, find_y) = 0;
				isEnd = false;
				move_x = find_x;
				move_y = find_y;
				oneContour.push_back(Point(move_x, move_y));
				break;
			}
		}
		if (isEnd)
			break;
	}
	move_x = oneContour[0].x; move_y = oneContour[0].y;
	while (1)
	{
		isEnd = true;
		for (int i = 0; i < 8; i++)
		{
			find_x = move_x + antiWise[i][0];
			find_y = move_y + antiWise[i][1];
			if (edge(find_x, find_y))
			{
				edge(find_x, find_y) = 0;
				isEnd = false;
				move_x = find_x;
				move_y = find_y;
				oneContourOpp.push_back(Point(move_x, move_y));
				break;
			}
		}
		if (isEnd)
			break;
	}
	if (oneContour.size() + oneContourOpp.size() > len)
	{
		if (oneContourOpp.size() > 0)
		{
			Point temp;
			for (int i = 0; i < (oneContourOpp.size() + 1) / 2; i++)
			{
				temp = oneContourOpp[i];
				oneContourOpp[i] = oneContourOpp[oneContourOpp.size() - 1 - i];
				oneContourOpp[oneContourOpp.size() - 1 - i] = temp;
			}
			oneContourOpp.insert(oneContourOpp.end(), oneContour.begin(), oneContour.end());
			edgeContours.push_back(oneContourOpp);
		}
		else
			edgeContours.push_back(oneContour);
	}
}

void findContours(Mat1b &edge,int len)
{
	VVP edgeContours;
	Mat1b edge_map = edge.clone();
	const int clockWise[8][2] = { { 0,1 },{ 1,0 },{ 0,-1 },{ -1,0 },{ -1,1 },{ 1,1 },{ 1,-1 },{ -1,-1 } };
	const int anticlockWise[8][2] = { { 0,-1 },{ 1,0 },{ 0,1 },{ -1,0 },{ -1,-1 },{ 1,-1 },{ 1,1 },{ -1,1 } };
	int rows = edge_map.rows, cols = edge_map.cols;

	for (int i = 0; i < cols; i++)
		edge_map(0, i) = edge_map(rows - 1, i) = 0;
	for (int i = 1; i < rows - 1; i++)
		edge_map(i, 0) = edge_map(i, cols - 1) = 0;
	for (int i = 1; i < rows; i++)
	{
		for (int j = 1; j < cols; j++)
		{
			if (edge_map(i,j))
			{
				edge_map(i, j) = 0;
				if (edge_map(i+1, j-1) && edge_map(i+1, j) && edge_map(i + 1, j+1))
					continue;
				else
				{
					findContour(clockWise, anticlockWise, edge_map, i, j,len, edgeContours);
				}
			}
		}
	}
}

