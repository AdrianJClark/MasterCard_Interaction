//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#include "cv.h"
#include "CameraCapture.h"
#include "VideoCapture.h"
#include "MastercardDetector.h"
#include <string>

using namespace std;

bool running = true;

void loadConfigFile(string filename);
void sendKey(BYTE keycode);

//Detect if there's been a major difference between the last two frames
bool majorFrameDifference(IplImage *lastFrame, IplImage *newFrame);
IplImage *lastFrame;

//Key Delay
int keyDelay;
//Colour difference Threshold
int thresh;
//Brightness Threshold
float bThresh=0.1;
//The colours to look for in the logo
CvScalar colourRed, colourYellow;
//Threshold between angles
int angleThresh;
//Use camera or video
bool useCamera;
//Video filename
string vidFilename;
//Debug level
int debugLevel;
//Min and max dist between circles in logo
int minDist, maxDist;
//Screensaver timeout
int invalidcardTimeout;

//Minimum Contour Size
int minContourSize;

//Camera Index and dimensions
int camIndex, camWidth, camHeight;

//Movement Threshold
int movementThresh;

//Screensaver timeout counter
int invalidcardCounter;

void main() {
//	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
//	_CrtSetBreakAlloc(15560);

	loadConfigFile("config.txt");

	//Set up new video capture
	Capture *cap;
	if (useCamera) {
		cap = new Camera(camIndex, cvSize(camWidth,camHeight));
	} else {
		cap = new Video((char*)vidFilename.c_str());
	}

	//Set up the master card detector object
	MastercardDetector *mcDetector = new MastercardDetector(colourRed, colourYellow, thresh, bThresh, minContourSize, minDist, maxDist, debugLevel-1);
	lastFrame = cap->getFrame();

	while (running) {
		//Get a frame
		IplImage *newFrame = cap->getFrame();
		if (!newFrame) { running=false; continue; }

		if (debugLevel==1) cvShowImage("Input Image", newFrame);

		//Find the logo
		if (mcDetector->findLogo(newFrame)) {
			int angle = (int)mcDetector->getAngle();

			if (angle > 360-angleThresh || angle <angleThresh) {//Up
				sendKey(VK_UP);
			} else if (angle > 90 - angleThresh && angle < 90 + angleThresh) {//Right
				sendKey(VK_RIGHT);
			} else if (angle > 180 - angleThresh && angle < 180 + angleThresh) {//Down
				sendKey(VK_DOWN);
			} else if (angle > 270 - angleThresh && angle < 270 + angleThresh) {//Left
				sendKey(VK_LEFT);
			}
		} else {
			invalidcardCounter++; if (invalidcardCounter>invalidcardTimeout) invalidcardCounter=invalidcardTimeout;
			//If we don't find it, see if there's been a major difference between the frames, if so send 2
			if (majorFrameDifference(lastFrame, newFrame)) {
				if (invalidcardCounter >= invalidcardTimeout) sendKey(0x32);
			}
		}
		

		//Handle Input
		switch (cvWaitKey(1)) {
			case 27:
				running=false; break;
		}

		//Clean up
		 cvReleaseImage(&lastFrame); lastFrame = cvCloneImage(newFrame);
		cvReleaseImage(&newFrame);
	}

	cvReleaseImage(&lastFrame); 
	delete cap;
	delete mcDetector;
}


void loadConfigFile(string filename) {
	CvFileStorage* fs = cvOpenFileStorage(filename.c_str(), 0, CV_STORAGE_READ );
	if (fs==0) {
		std::cout << "Unable to locate config.txt, using defaults" << std::endl; 
		thresh = 55;
		colourRed = cvScalar(146, 63, 57);
		colourYellow = cvScalar(170,120,53);
		angleThresh = 15;
		useCamera = true; debugLevel = 0;
		minDist = 10; maxDist = 50;
		movementThresh = 8;
		keyDelay=10;
		camIndex=0; camWidth=320; camHeight=240;
		invalidcardTimeout=60;
		return;
	}

	//Screensaver timeout
	invalidcardTimeout=cvReadIntByName(fs, NULL, "invalidcard_timeout", 60);

	//Camera Properties
	camIndex=cvReadIntByName(fs, NULL, "camera_index", 0);
	camWidth=cvReadIntByName(fs, NULL, "camera_width", 320);
	camHeight=cvReadIntByName(fs, NULL, "camera_height", 240);

	//Key Delay
	keyDelay =  cvReadIntByName(fs, NULL, "key_delay", 10);

	//Colour difference Threshold
	thresh =  cvReadIntByName(fs, NULL, "colour_thresh", 55);

	//Threshold between angles
	angleThresh = cvReadIntByName(fs, NULL, "angle_thresh", 15);

	//Debug level
	debugLevel = cvReadIntByName(fs, NULL, "debug_level", 0);

	//Contour Size
	minContourSize = cvReadIntByName(fs, NULL, "min_contour_size", 500);

	//Use camera or video
	useCamera = (cvReadIntByName(fs, NULL, "use_camera", 1)==1);
	if (!useCamera) { //Video filename
		vidFilename = cvReadStringByName(fs, NULL, "video_filename", "Video1-2pm.mov");
	}

	//The colours to look for in the logo
	colourYellow = cvScalar(cvReadIntByName(fs, NULL, "colour_yellow_red", 170), cvReadIntByName(fs, NULL, "colour_yellow_green", 120), cvReadIntByName(fs, NULL, "colour_yellow_blue", 53));
	colourRed = cvScalar(cvReadIntByName(fs, NULL, "colour_red_red", 146), cvReadIntByName(fs, NULL, "colour_red_green", 63), cvReadIntByName(fs, NULL, "colour_red_blue", 57));

	//Min and max dist between circles in logo
	minDist = cvReadIntByName(fs, NULL, "min_dist", 10);
	maxDist = cvReadIntByName(fs, NULL, "max_dist", 50);

	//Movement Threshold
	movementThresh = cvReadIntByName(fs, NULL, "movement_thresh", 8);

	cvFree(&fs);
}

/*Check to see if there's been a major difference between the last 2 frames*/
bool majorFrameDifference(IplImage *lastFrame, IplImage *newFrame) {
	//Create some grayscale images
	IplImage *diffIm = cvCreateImage(cvGetSize(lastFrame), IPL_DEPTH_8U, 1);
	IplImage *grayLast = cvCreateImage(cvGetSize(lastFrame), IPL_DEPTH_8U, 1); cvConvertImage(lastFrame, grayLast);
	IplImage *grayNew = cvCreateImage(cvGetSize(newFrame), IPL_DEPTH_8U, 1); cvConvertImage(newFrame, grayNew);

	//Subtract one image from the other
	cvSub(grayNew , grayLast, diffIm);
	//Get the difference
	CvScalar diffScalar = cvSum(diffIm);
	//Normalize the difference
	float diff = (diffScalar.val[0] + diffScalar.val[1] + diffScalar.val[2] + diffScalar.val[3]) / (grayNew->width*grayNew->height);

	//Clean up
	cvReleaseImage(&grayNew);cvReleaseImage(&grayLast); cvReleaseImage(&diffIm);

	return diff > (float)movementThresh;
}

/*Send a key if we have exceeded our delay*/
void sendKey(BYTE keycode) {
	static int delayCounter=0; static int lastKeyPress=-1;
	if (lastKeyPress!=keycode) delayCounter = 0;
	if (delayCounter==0) keybd_event(keycode,0,0,0); keybd_event(keycode,0,KEYEVENTF_KEYUP,0); 
	delayCounter = (delayCounter+1)%keyDelay; lastKeyPress=keycode;
	invalidcardCounter=0;
}
