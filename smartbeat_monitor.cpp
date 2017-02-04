#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/stitching.hpp>
#include <opencv2/stitching/warpers.hpp>


#include <vector>
#include <math.h>
#include "raspicam_cv.h"

//#pragma endregion
#include "socketserver/socketserver.h"
#include <time.h>
using namespace std;

SocketServer client;
raspicam::RaspiCam_Cv camera;
cv::VideoCapture cam(0);

bool viewportOn;

double average(double arr[], int length);
void myhandler(message_type type, std::string message) {
	printf("%s\n", message.c_str());
}
void onMessageHandler(string selector, vector<string> args) {
	if (selector == "setDisplayImageIndex")
	{
		int index = atoi(args[0].c_str());
		client.SetImageIndex(index);
	}
	else if (selector == "toggleViewport")
	{

		viewportOn = (bool)atoi(args[0].c_str());
	}
	else if (selector == "exposure")
	{
		//exposure[] means 'get' exposure and exposure[somenumber] means 'set' exposure
		if (args.size() == 0) //Get
		{
			float exposure = (float)camera.get(CV_CAP_PROP_EXPOSURE);
			
		}
		else
		{
			double exposure = std::stod(args[0]);
			camera.set(CV_CAP_PROP_EXPOSURE, exposure);
		}
	}
	else if (selector == "gain")
	{
		//gain[] means 'get' gain and gain[somenumber] means 'set' gain
		if (args.size() == 0) //Get
		{
			float gain = (float)camera.get(CV_CAP_PROP_GAIN);
			
		}
		else
		{
			double gain = std::stod(args[0]);
			camera.set(CV_CAP_PROP_GAIN, gain);
		}
	}
}

void onPortBind(int port)
{
	printf("Bound to port: %d\n", port);
}

int main(int argc, char *argv[])
{
	
	time_t timer_begin, timer_end;
	cv::Mat image;
	int nCount = 100;
	//set camera params
	camera.set(CV_CAP_PROP_FORMAT, CV_8UC3);
	camera.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	camera.set(CV_CAP_PROP_FRAME_HEIGHT, 360);
	camera.set(CV_CAP_PROP_AUTO_EXPOSURE, 0);
	cout << "camera gain: " << camera.get(CV_CAP_PROP_GAIN) << endl;
	cout << "camera exposure: " << camera.get(CV_CAP_PROP_EXPOSURE) << endl;
	cout << "camera auto control: " << (camera.get(CV_CAP_PROP_AUTO_EXPOSURE) == 0 ? false : true) << endl;
	//set camera params
	cam.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT, 360);
	
	cout << "Opening Camera..." << endl;
	if (!camera.open()) {cerr << "Error opening the camera" << endl; return -1;}
	
	if (!cam.isOpened()) {cerr << "broken, the camera" << endl; }
	//open socket server
	
	Handlers handlers;
	handlers.onSystemMessage = myhandler;
	handlers.onClientMessage = onMessageHandler;
	handlers.onPortBind = onPortBind;
	client.SetHandlers(&handlers);
	client.SecureStart("public.pem", "private.pem");
	client.SetImageIndex(0);
	
	time(&timer_begin);
	clock_t t;
	double times[10];
	int count = 0;
	int count2 = 0;
	cv::Mat resizedImg;
	cv::Mat webCamImg;
	cv::Mat webCamImg2;

	
	cv::Mat panorama;
	cv::Stitcher stitcher = cv::Stitcher::createDefault(true);
	
	/*stitcher.setWaveCorrection(false);
	stitcher.setSeamEstimationResol(0.001);
	stitcher.setPanoConfidenceThresh(0.1);
	
	stitcher.setSeamFinder(new cv::detail::NoSeamFinder());
	stitcher.setBlender(cv::detail::Blender::createDefault(cv::detail::Blender::NO, true));
	stitcher.setExposureCompensator(new cv::detail::NoExposureCompensator());*/
	
	stitcher.setRegistrationResol(-1); // 0.6
	stitcher.setSeamEstimationResol(-1);   /// 0.1
	stitcher.setCompositingResol(-1);   //1
	stitcher.setPanoConfidenceThresh(-1);   //1
	stitcher.setWaveCorrection(true);
	stitcher.setWaveCorrectKind(cv::detail::WAVE_CORRECT_HORIZ);
	cam >> webCamImg; //grab webcam image

	vector<cv::Mat> images;
	bool firstTime = true;
	while (1)
	{
		t = clock();
		//grab image from picam and flip to match orientation of webcam
		camera.grab();	
		camera.retrieve(image);
		//imwrite("picamnoFlip.png", image);
		flip(image, image, 0); //vertical vlip
		flip(image, image, 1); //horizontal flip
		//imwrite("picamFlip.png", image);

		cam >> webCamImg; //grab webcam image
		
		//push both images to vector
		images.push_back(image); 
		images.push_back(webCamImg);
		
		//client.SendClient_viewport(webCamImg, true, true);
		//client.SendClient_viewport(image, true, true);
		//imwrite("webcam.png", webCamImg);
		
		if (firstTime) // only need to estimate transform first time
		{
			stitcher.estimateTransform(images);
			firstTime = false;
		}

		stitcher.composePanorama(images, panorama);
		client.SendClient_viewport(image, true, false);
		imwrite("pan.png", panorama); //this writes image to pi, easy way to view if panorama was made
		images.clear(); //clear vector

		count++;
#pragma region DISPLAY TIMING
		t = clock() - t;
		times[count % 10] = (double)t;
		if (count > 10)
			printf("computation time: %.4f seconds\r and count2 %i", average(times, 10) / CLOCKS_PER_SEC, count2);
		//count2++;
#pragma endregion
	}


}

double average(double arr[], int length)
{
	double sum = 0.0f;
	for (int i = 0; i < length; i++)
		sum += arr[i];
	return sum / length;
}