/*---------------------------------------------------------------------------
   DispSel.cpp - Disparity Selection Code
  ---------------------------------------------------------------------------
   Author: Charles Leech
   Email: cl19g10 [at] ecs.soton.ac.uk
   Copyright (c) 2016 Charlie Leech, University of Southampton.
  ---------------------------------------------------------------------------*/
#include "DispSel.h"

DispSel::DispSel(void)
{
#ifdef DEBUG_APP
		std::cout <<  "Winner-Takes-All Disparity Selection." << std::endl;
#endif // DEBUG_APP
}
DispSel::~DispSel(void) {}

void *DS_X(void *thread_arg)
{
	struct DS_X_TD *t_data;
	t_data = (struct DS_X_TD *) thread_arg;
    //Matricies
	Mat* costVol = t_data->costVol;
	Mat* dispMap = t_data->dispMap;
    //Variables
	int y = t_data->y;
	int maxDis = t_data->maxDis;

	int wid = dispMap->cols;
	uchar* dispData = (uchar*) dispMap->ptr<uchar>(y);

	for(int x = 0; x < wid; ++x)
	{
		float minCost = DBL_MAX;
		int minDis = 0;

		for(int d = 1; d < maxDis; ++d)
		{
			float* costData = (float*)costVol[d].ptr<float>(y);
			if(costData[x] < minCost)
			{
				minCost = costData[x];
				minDis = d;
			}
		}
		dispData[x] = minDis;
	}
	return (void*)0;
}

void DispSel::CVSelect_thread(Mat* costVol, const int maxDis, Mat& dispMap, int threads)
{
    int hei = dispMap.rows;

	//Set up threads for x-loop
    void* status;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_t DS_X_threads[hei];
    DS_X_TD DS_X_TD_Array[hei];

    for(int level = 0; level <= hei/threads; ++level)
	{
        //Handle remainder if threads is not power of 2.
	    int block_size = (level < hei/threads) ? threads : (hei%threads);

	    for(int iter=0; iter < block_size; ++iter)
	    {
	        int d = level*threads + iter;
            DS_X_TD_Array[d] = {costVol, &dispMap, d, maxDis};
            pthread_create(&DS_X_threads[d], &attr, DS_X, (void *)&DS_X_TD_Array[d]);
	    }
        for(int iter=0; iter < block_size; ++iter)
	    {
	        int d = level*threads + iter;
            pthread_join(DS_X_threads[d], &status);
        }
	}
	return;
}

void DispSel::CVSelect(Mat* costVol, const int maxDis, Mat& dispMap)
{
    int hei = dispMap.rows;
    int wid = dispMap.cols;

	#pragma omp parallel for
    for(int y = 0; y < hei; ++y)
    {
		for(int x = 0; x < wid; ++x)
		{
			float minCost = DBL_MAX;
			int minDis = 0;

			for(int d = 1; d < maxDis; ++d)
			{
				float* costData = (float*)costVol[d].ptr<float>(y);
				if(costData[x] < minCost)
				{
					minCost = costData[x];
					minDis = d;
				}
			}
			dispMap.at<uchar>(y,x) = minDis;
		}
    }
    return;
}
