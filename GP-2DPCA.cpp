#include<opencv2/opencv.hpp>
#include<iostream>
#include<stdio.h>
#include"time.h"
#include<windows.h>
#include <psapi.h>
#pragma comment(lib,"psapi.lib")
#include <stdio.h>

#include <math.h>
using namespace std;
using namespace cv;

// ��������ʾ����ʹ�õ��ڴ���Ϣ
void showMemoryInfo(void)
{
	HANDLE handle = GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(handle, &pmc, sizeof(pmc));
	cout << "�ڴ�ʹ�ã�" << pmc.WorkingSetSize / 1000 << "K/" << pmc.PeakWorkingSetSize / 1000 << "K + " << pmc.PagefileUsage / 1000 << "K/" << pmc.PeakPagefileUsage / 1000 << "K" << endl;
}

// ������������� A ��Ԫ��ƽ���͵�ƽ����
double sumF(Mat A)
{
	double s = 0.0;
	for (int i = 0; i < A.rows; i++)
	{
		for (int j = 0; j < A.cols; j++)
		{
			s += A.at<float>(i, j) * A.at<float>(i, j);
		}
	}
	s = sqrt(s);
	return s;
}

void main()
{
	// ��ʼ��ʱ
	clock_t start, finish;
	double duration;
	start = clock();

	// �����ڴ�ʹ�����
	showMemoryInfo();
	cout << "�������пɻ��յ��ڴ�" << endl;
	EmptyWorkingSet(GetCurrentProcess());
	showMemoryInfo();
	cout << "��ʼ��̬�����ڴ�" << endl;

	// �������
#define eigenNum  30 // ���������������ɵ�����

	int sampleNum = 160;        // �����������ɵ�����
	//int originrows = 112, origincols = 92;//resize
	int nrows = 200, ncols = 50; // ������С���ɵ�����
	float  s = 2; // �ɵ���
	int classNum = 8; // ѵ���������
	int perclassNum = 20; // ÿ��ѵ����ͼƬ������
	int max_iteration = 40; // ����������
	Mat meanSample = Mat::zeros(nrows, ncols, CV_32FC1);    // ������ƽ��ֵ
	Mat totalSample = Mat::zeros(nrows, ncols, CV_32FC1);   // �������ܺ�
	Mat oneSample(nrows, ncols, CV_32FC1);
	// Mat sizeSample = Mat::zeros(nrows, ncols, CV_32FC1);
	int ConstrainNum = 1; // Լ���ܿ�����Ҫ��֤ nrows / ConstrainNum Ϊ����
	int computerNum = sampleNum * nrows / ConstrainNum; // ������Լ����Ŀ���
	Mat ag = Mat::zeros(ConstrainNum, ncols, CV_32FC1);

	// ��ʼ�� W
	Mat U, S, VT;
	vector<Mat> samples;        // �洢����
	vector<Mat> cosamples;        // �洢Լ������
	// ��������������������
	for (int i = 0; i < sampleNum; i++)
	{
		string imagePath = "E:\\random_noise_databases\\ETH-80_original�������ݼ�\\ETH_0.4_0.10-0.30_100x100\\block200x50\\train12\\" + to_string(static_cast<long long>(i)) + ".png"; // ͼ��·��
		Mat srcImage = imread(imagePath, 0);
		srcImage.clone().convertTo(oneSample, CV_32FC1, 1, 0);
		// resize(oneSample, sizeSample, sizeSample.size());
		totalSample += oneSample;
		samples.push_back(oneSample.clone());
	}

	//load the samples and compute the total samples    �����ֵ����ݼ�
	/*for (int q = 1; q <= classNum; q++) {
		for (int i = 0; i < perclassNum; i++)
		{
			string imagePath = "E:\\random_noise_databases\\NEC�������ݼ�\\NEC_0.4_0.10-0.30_96X116\\train1\\" + to_string(static_cast<long long>(q)) + "\\" + to_string(static_cast<long long>(i)) + ".jpg";//imagepath to_string(static_cast<long long>(i))
			Mat srcImage = imread(imagePath, 0);
			srcImage.clone().convertTo(oneSample, CV_32FC1, 1, 0);
			totalSample += oneSample;
			samples.push_back(oneSample.clone());
		}
	}
	*/
	//compute the mean sample
	meanSample = totalSample / double(sampleNum);
	//make constrain sample
	for (int i = 0; i < sampleNum; i++)
	{
		for (int j = 0; j < nrows / ConstrainNum; j++) {
			Mat J = samples[i] - meanSample;
			Mat a = J.rowRange(j*ConstrainNum, (j + 1)*ConstrainNum);
			cosamples.push_back(a.clone());
		}
	}
	for (int K = 5; K <= eigenNum; K = K + 5)
	{
		cout << K << endl;
		int iteration = 0;
		Mat I = Mat::eye(ncols, K, CV_32FC1);
		float d = 0;
		Mat G = Mat::zeros(ncols, ncols, CV_32FC(1));
		Mat H = Mat::zeros(ncols, K, CV_32FC(1));
		Mat w = Mat::eye(ncols, K, CV_32FC(1));//ע���ʼ��ֵϸ�ڣ������ó�0���������
		w = w / 20;  //��w���Խ���Ԫ�ر�ø�С����������
		Mat ws = Mat::zeros(ncols, K, CV_32FC(1));
		Mat store_redusial = Mat::zeros(max_iteration, 1, CV_32FC1);
		do {

			iteration = iteration + 1;
			//float s = 1;
			double E = 0, C = 0, F = 0, objective_function = 0;
			H = 0; double d = 0;
			double gamu = 0.000001;
			w.copyTo(ws);//����w�ĸ���
			for (int i = 0; i < computerNum; i++)
			{
				ag = cosamples[i];
				C = pow(sumF(ag), s);
				//cout << "C=" << C << endl;

				F = sumF(ag*w);
				//E = pow(E, s);
				//for (int j = 0; j < BlockNum; j++) {C = sumL2(ag.row(j));d += pow(C, p );}

				//cout << "d=" << d << endl;
				//a��b�η�
				E = sumF(ag - ag * w*w.t());
				d = C * F*E + gamu;// ����Ϊ��
				H += ag.t()*ag*w / d;
				//objective_function += E * F / C;

			}
			

			SVD::compute(H, S, U, VT, SVD::FULL_UV);
			w = U * I*VT;
			//cout << "objective_function="<< objective_function << endl;
			//store_redusial.at<float>(iteration-1,0) = float(sumF(w - ws));
			if (iteration >= max_iteration) {
				cout << "�����������£�K=" << K << "�޷����У���Ҫ������������,����Ϊw-ws��F�����в�" << endl;
				//cout << store_redusial << endl;
				break;//����whileѭ����ֱ�ӱ������һ�ε����Ľ��

			}
			cout << sumF(w - ws) << endl;
		} while (sumF(w - ws) / sumF(w) > 0.008); //��������

		CvMat vecs = w;
		char store[256]; sprintf_s(store, "E:\\����ʵ��test test�Ա�\\BPCA\\Height_diffnorms_2DPCA\\ETH200x50\\12\\hebing%d.txt", K);
		cvSave(store, &vecs);

		CvMat agSample = meanSample;
		cvSave("E:\\����ʵ��test test�Ա�\\BPCA\\Height_diffnorms_2DPCA\\ETH200x50\\12\\meanSample1.txt", &agSample);
		cout << "iteration=" << iteration << endl;
		//����ѵ����������
	/*	Mat trainDataMat = Mat::zeros(nrows, eigenNum, CV_32FC1);;
		for (int i = 0; i < sampleNum; i++)
		{

			Mat srcSample = samples[i] - meanSample;//���Ļ�����
			trainDataMat = srcSample * w;
			cout << "image_id=" << i << endl;
			for (int ii = 0; ii < eigenNum; ii++) {
				for (int j = 0; j < nrows; j++) {
					cout << trainDataMat.at<float>(j, ii) << endl;
				}cout << endl;
			}*/
			/*CvMat storeMat;
			storeMat = trainDataMat;
			char store[256]; sprintf_s(store, "E:\\random_noise_databases\\���ӻ�����\\���ɵĽ�ά�ӿռ�ͶӰ\\GHEIGHT\\s=2\\resize\\noise\\2άͶӰͼƬ%d.txt", i);
			cvSave(store, &storeMat);*/

			//}
	}

	//using memory
	showMemoryInfo();


	//finish timing
	finish = clock();
	duration = (double)(finish - start) / CLOCKS_PER_SEC;
	cout << "ѵ�����̽���,����ʱ��" << duration << "��" << endl;

	waitKey(0);

}