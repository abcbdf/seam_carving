// ConsoleApplication2.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <vector>
#define w 2000
#define debug(x) cout << #x << "=" << x << endl;
#define small -(1e7)

using namespace std;
using namespace cv;
int cao = 0;
bool is_start = false;
const int pen_size = 5;
vector<int> sign_x, sign_y;
void drawpixel(int x, int y, Vec3f& v, Mat& m) // draw pixel
{
	if (x < 0 || x >= m.rows || y < 0 || y >= m.cols)
	{
		cout << "out" << endl;
	}
	else
	{
		m.at<Vec3b>(x, y) = (Vec3b)v;
	}
}
Vec3f getpixel(int x, int y, Mat& m) // get pixel color
{
	if (x < 0 || x >= m.rows || y < 0 || y >= m.cols)
	{
		//cout << "<" << x << "," << y << ">" << endl;
		return Vec3f(-1, -1, -1);
	}
	else
	{
		//cout << "yes" << x << "," << y << ">" << endl;
		return (Vec3f)(m.at<Vec3b>(x, y));
	}
}


vector<int> seam_carving(Mat& m, bool isLevel)
{
	Mat grey_m, grey_x, abs_grey_x;
	cvtColor(m, grey_m, CV_RGB2GRAY);
	Sobel(grey_m, grey_x, CV_16S, 1, 0, 3, 1, 0, BORDER_DEFAULT);
	convertScaleAbs(grey_x, abs_grey_x);
	int **seam;
	vector<int> seam_min(m.rows);
	seam = new int *[m.rows];
	for (int i = 0; i < m.rows; i++)
	{
		seam[i] = new int[m.cols];
	}

	for (int i = 0; i < sign_x.size(); i++)
	{
		if (isLevel)	seam[sign_x[i]][sign_y[i]] = small;
		else seam[sign_y[i]][sign_x[i]] = small;		
	}
	for (int x = 0; x < m.rows; x ++)
	{
		for (int y = 1; y < m.cols - 1; y ++)
		{
			if (seam[x][y] == small)
			{
				continue;
			}
			if (x == 0)
			{
				seam[x][y] = abs_grey_x.at<uchar>(x, y);
			}
			else
			{
				int temp_seam = seam[x - 1][y];
				if (y > 1)
				{
					temp_seam = min(seam[x - 1][y - 1], temp_seam);
				}
				if (y < m.cols - 2)
				{
					temp_seam = min(seam[x - 1][y + 1], temp_seam);
				}
				seam[x][y] = abs_grey_x.at<uchar>(x, y) + temp_seam;
			}
		}
	}
	for (int x = m.rows - 1; x >= 0; x --)
	{
		int temp_min = INT_MAX;
		if (x == m.rows - 1)
		{
			for (int y = 1; y < m.cols - 1; y ++)
			{
				if (seam[x][y] < temp_min)
				{
					seam_min[x] = y;
					temp_min = seam[x][y];
				}
			}
		}
		else
		{
			for (int y = seam_min[x + 1] - 1; y <= seam_min[x + 1] + 1; y ++)
			{
				if (y >= 1 && y < m.cols - 1 && seam[x][y] < temp_min)
				{
					seam_min[x] = y;
					temp_min = seam[x][y];
				}
			}
		}
		//debug(temp_min);
		//debug(seam_min[x]);
	}
	for (int i = 0; i < m.rows; i++)
	{
		delete[] seam[i];
	}
	delete[] seam;
	return seam_min;
}
void seam_decrease(Mat& m, vector<int> seam_min, bool isLevel)
{
	Mat new_m = m.clone();
	for (int x = 0; x < m.rows; x++)
	{
		for (int y = seam_min[x]; y < m.cols - 1; y++)
		{
			new_m.at<Vec3b>(x, y) = m.at<Vec3b>(x, y + 1);
		}
	}
	for (int i = 0; i < sign_x.size(); i++)
	{
		if (isLevel)
		{
			if (seam_min[sign_x[i]] < sign_y[i])
			{
				sign_y[i] --;
			}
			else if (seam_min[sign_x[i]] == sign_y[i])
			{
				sign_x.erase(sign_x.begin() + i);
				sign_y.erase(sign_y.begin() + i);
				i--;
			}
		}
		else
		{
			if (seam_min[sign_y[i]] < sign_x[i])
			{
				sign_x[i] --;
			}
			else if (seam_min[sign_y[i]] == sign_x[i])
			{
				sign_x.erase(sign_x.begin() + i);
				sign_y.erase(sign_y.begin() + i);
				i--;
			}
		}
	}
	m = new_m(Range(0, m.rows), Range(0, m.cols - 1));
}void seam_increase(Mat& m, vector<int> seam_min)
{
	Mat new_m(m.rows, m.cols + 1, m.type());
	for (int x = 0; x < new_m.rows; x++)
	{
		for (int y = 0; y < new_m.cols; y++)
		{
			if (y <= seam_min[x])
			{
				new_m.at<Vec3b>(x, y) = m.at<Vec3b>(x, y);
			}
			else if (y == seam_min[x] + 1)
			{
				new_m.at<Vec3b>(x, y) = m.at<Vec3b>(x, y - 1) / 2 + m.at<Vec3b>(x, y) / 2;
			}
			else
			{
				new_m.at<Vec3b>(x, y) = m.at<Vec3b>(x, y - 1);
			}
		}
	}
	m = new_m.clone();
}
void OnMouseAction(int event, int x, int y, int flags, void *ustc)
{
	
	if (event == CV_EVENT_MOUSEMOVE && is_start)
	{
		for (int temp_x = x - pen_size; temp_x <= x + pen_size; temp_x++)
		{
			for (int temp_y = y - pen_size; temp_y <= y + pen_size; temp_y++)
			{
				sign_x.push_back(temp_y);
				sign_y.push_back(temp_x);
			}
		}
		
	}
	if (event == CV_EVENT_LBUTTONDOWN)
	{
		is_start = true;
	}
	if (event == CV_EVENT_LBUTTONUP)
	{
		is_start = false;
	}
}

int main(void) {
	Mat image;
	image = imread("F:/homework/image/seam_test/11.jpg", CV_LOAD_IMAGE_COLOR);
	char atom_window[] = "Drawing 1: Atom";
	//Mat atom_image = Mat::zeros(w, w, CV_8UC3);
	//Bresenhamline(800, 800, 1200, 1200, Vec3f(0, 255, 255), atom_image);
	//MidPointCircle(1000, 1000, 10, Vec3f(255, 255, 255), atom_image);
	//BoundaryFill4(1000, 1000, Vec3f(255, 255, 255), Vec3f(255, 0, 0), atom_image);
	//myAlias(atom_image);
	//ssaaAlias(atom_image);
	/*for (int i = 0; i < 300; i++)
	{
		vector<int> seam_min = seam_carving(image, true);
		seam_decrease(image, seam_min, true);
		cout << "i:" << i << endl;
	}
	imwrite("F:/homework/image/seam_test_final_10.jpg", image);*/
	int last_key = 119;
	for (int i = 0; ; i ++)
	{
		cout << last_key << endl;
		cout << "sign_x:" << sign_x.size() << endl;
		cout << "sign_y:" << sign_y.size() << endl;
		if (last_key == 119 || last_key == 115)
		{
			vector<int> seam_min = seam_carving(image, true);
			Mat show_m = image.clone();
			for (int x = 0; x < image.rows; x++)
			{
				show_m.at<Vec3b>(x, seam_min[x]) = Vec3b(0, 255, 0);
			}
			for (int ii = 0; ii < sign_x.size(); ii++)
			{
				show_m.at<Vec3b>(sign_x[ii], sign_y[ii]) = Vec3b(255, 255, 0);
			}
			char atom_window[] = "Drawing 1: Atom";
			imshow(atom_window, show_m);
			setMouseCallback("Drawing 1: Atom", OnMouseAction);
			moveWindow(atom_window, 0, 0);
			if (last_key == 119 && sign_x.size() > 0)
			{

			}
			else
			{
				last_key = waitKey(-1);
			}
			
			seam_decrease(image, seam_min, true);
			cout << "i:" << i << endl;
		
		}
		else if (last_key == 97 || last_key == 100)
		{
			image = image.t();
			vector<int> seam_min = seam_carving(image, false);
			Mat show_m = image.clone();
			for (int x = 0; x < image.rows; x++)
			{
				show_m.at<Vec3b>(x, seam_min[x]) = Vec3b(0, 0, 255);
			}
			for (int ii = 0; ii < sign_x.size(); ii++)
			{
				show_m.at<Vec3b>(sign_y[ii], sign_x[ii]) = Vec3b(255, 255, 0);
			}
			char atom_window[] = "Drawing 1: Atom";
			imshow(atom_window, show_m.t());
			setMouseCallback("Drawing 1: Atom", OnMouseAction);
			moveWindow(atom_window, 0, 0);
			last_key = waitKey(-1);
			seam_decrease(image, seam_min, false);
			cout << "i:" << i << endl;
			image = image.t();
		}
		else if (last_key == 113)
		{
			imwrite("F:/homework/image/seam_test_final_11_obj.jpg", image);
			last_key = waitKey(-1);
		}
		else if (last_key == 106)
		{
			vector<int> seam_min = seam_carving(image, true);
			Mat show_m = image.clone();
			for (int x = 0; x < image.rows; x++)
			{
				show_m.at<Vec3b>(x, seam_min[x]) = Vec3b(0, 255, 0);
			}
			for (int ii = 0; ii < sign_x.size(); ii++)
			{
				show_m.at<Vec3b>(sign_x[ii], sign_y[ii]) = Vec3b(255, 255, 0);
			}
			char atom_window[] = "Drawing 1: Atom";
			imshow(atom_window, show_m);
			setMouseCallback("Drawing 1: Atom", OnMouseAction);
			moveWindow(atom_window, 0, 0);
			if (last_key == 119 && sign_x.size() > 0)
			{

			}
			else
			{
				last_key = waitKey(-1);
			}

			seam_increase(image, seam_min);
			cout << "i:" << i << endl;
		}
		else if (last_key == 107)
		{
			image = image.t();
			vector<int> seam_min = seam_carving(image, false);
			Mat show_m = image.clone();
			for (int x = 0; x < image.rows; x++)
			{
				show_m.at<Vec3b>(x, seam_min[x]) = Vec3b(0, 0, 255);
			}
			for (int ii = 0; ii < sign_x.size(); ii++)
			{
				show_m.at<Vec3b>(sign_y[ii], sign_x[ii]) = Vec3b(255, 255, 0);
			}
			char atom_window[] = "Drawing 1: Atom";
			imshow(atom_window, show_m.t());
			setMouseCallback("Drawing 1: Atom", OnMouseAction);
			moveWindow(atom_window, 0, 0);
			last_key = waitKey(-1);
			seam_increase(image, seam_min);
			cout << "i:" << i << endl;
			image = image.t();
		}
		else
		{
			last_key = waitKey(-1);
		}
	}	
	

	waitKey(0);
	return(0);
}
