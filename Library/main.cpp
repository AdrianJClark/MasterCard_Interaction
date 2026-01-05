#include "MasterCardDetector.h"
#include "highgui.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

void NRGB(float r, float g, float b, float *nr, float *ng, float *nb);
IplImage* rgbSub(IplImage *src, CvScalar col, float bThresh);
void toSpecGreyscale(IplImage *src, IplImage *dest, CvScalar col1, CvScalar col2);

MastercardDetector::MastercardDetector(CvScalar red, CvScalar yellow, int thresh, float bThresh, int minContourSize, int minDist, int maxDist, int debug) {
	debugMode = debug; cThresh = thresh;
	cRed = red; cYellow = yellow;
	this->bThresh = bThresh;
	this->minDist = minDist*minDist; this->maxDist = maxDist*maxDist;
	this->minContSize = minContourSize;
}

MastercardDetector::~MastercardDetector() {}

void MastercardDetector::setColours(CvScalar red, CvScalar yellow) {
	cRed = red; cYellow = yellow;
}

void MastercardDetector::setDebugMode(int debug) {
	debugMode = debug;
}

void MastercardDetector::setThreshold(int thresh) {
	cThresh = thresh;
}

/*Find the logo in the video frame*/
bool MastercardDetector::findLogo(IplImage *frame) {
	bool retVal = false;

	//Find the red areas in the image
	IplImage *bwImageRed = rgbSub(frame,cRed, bThresh);
	cvThreshold(bwImageRed, bwImageRed, cThresh, 255, CV_THRESH_BINARY);
	cvErode(bwImageRed, bwImageRed);

	//Make a greyscale image representing the brightness
	IplImage *bwImageGrey = cvCreateImage(cvGetSize(bwImageRed), IPL_DEPTH_8U, 1);
	toSpecGreyscale(frame, bwImageGrey, cRed, cYellow);

	//Find the yellow areas in the image
	IplImage *bwImageYellow = rgbSub(frame, cYellow, bThresh);
	cvThreshold(bwImageYellow, bwImageYellow, cThresh, 255, CV_THRESH_BINARY);
	cvErode(bwImageYellow, bwImageYellow);

	if (debugMode>1) {
		cvShowImage("bwRed", bwImageRed);
		cvShowImage("bwYellow", bwImageYellow);
	}

	//Find the areas in one image but not the other
	IplImage *bwDiffRY = cvCreateImage(cvGetSize(bwImageRed), IPL_DEPTH_8U, 1);
	IplImage *bwDiffYR = cvCreateImage(cvGetSize(bwImageRed), IPL_DEPTH_8U, 1);
	cvSub(bwImageRed, bwImageYellow, bwDiffRY);
	cvSub(bwImageYellow, bwImageRed, bwDiffYR);
	
	if (debugMode>1) {
		cvShowImage("diffRY", bwDiffRY);
		cvShowImage("diffYR", bwDiffYR);
	}

	//Combine those images together into one large image
	IplImage *bwDiff = cvCreateImage(cvGetSize(bwImageRed), IPL_DEPTH_8U, 1);
	cvOr(bwDiffRY, bwDiffYR, bwDiff);
	cvDilate(bwDiff, bwDiff, 0, 2);
	cvReleaseImage(&bwDiffRY); cvReleaseImage(&bwDiffYR); 
	if (debugMode>1) cvShowImage("Dilate diff", bwDiff);
	
	//Find contours in the difference image
	IplImage *contDiff = cvCloneImage(bwDiff);
	CvMemStorage *ms = cvCreateMemStorage(0);
	CvSeq *contours;
	int contourCount = cvFindContours(contDiff, ms, &contours);
	cvReleaseImage(&contDiff);

	if (contourCount>0) {
		//Loop through and find the largest contour
		int largestContour=-1, largestIndex = -1;
		int index=-1; CvRect r;
		for (CvSeq *c=contours; c!=NULL; c=c->h_next) {
			//We do this by finding the contour with the largest number of non-zero pixels in it's bounding box
			index++;
			CvRect rTmp=cvBoundingRect(c);
			CvMat *subMat = cvCreateMatHeader(rTmp.height, rTmp.width, CV_8U);
			cvGetSubRect(bwDiff, subMat, rTmp);

			CvMat *subMat2 = cvCreateMatHeader(rTmp.height, rTmp.width, CV_8U);
			cvGetSubRect(bwImageGrey, subMat2, rTmp);
			

			//if (cvCountNonZero(subMat)> largestContour) {largestContour = cvCountNonZero(subMat); largestIndex = index; r = cvBoundingRect(c);}
			if (cvAvg(subMat2, subMat).val[0] > largestContour && cvCountNonZero(subMat) > minContSize) {largestContour = cvAvg(subMat2, subMat).val[0]; largestIndex = index; r = cvBoundingRect(c);}
			cvReleaseMatHeader(&subMat);
			cvReleaseMatHeader(&subMat2);
		}

		if (largestContour !=-1 && largestIndex != -1 && r.width > 0 && r.height > 0) {

			//Set the bounding rectangle on the red image and find the center of mass using image contours to calculate the center of the red circle
			CvMat *subMatRed = cvCreateMatHeader(r.height, r.width, CV_8U);
			CvMoments rMoments;
			cvGetSubRect(bwImageRed, subMatRed, r);
			cvNot(bwImageRed, bwImageRed);
			cvMoments(subMatRed, &rMoments, 0);
			CvPoint rCenter = cvPoint(r.x + rMoments.m10/rMoments.m00, r.y + rMoments.m01/rMoments.m00);
			cvReleaseMatHeader(&subMatRed);

			//Set the bounding rectangle on the yellow image and find the center of mass using image contours to calculate the center of the yellow circle
			cvNot(bwImageYellow, bwImageYellow);
			CvMat *subMatYellow = cvCreateMatHeader(r.height, r.width, CV_8U);
			cvGetSubRect(bwImageYellow, subMatYellow, r);
			CvMoments yMoments;
			cvMoments(subMatYellow, &yMoments, 0);
			CvPoint yCenter = cvPoint(r.x + yMoments.m10/yMoments.m00, r.y + yMoments.m01/yMoments.m00);
			cvReleaseMatHeader(&subMatYellow);

			int dist = (yCenter.x-rCenter.x)*(yCenter.x-rCenter.x) + (yCenter.y-rCenter.y)*(yCenter.y-rCenter.y);
			if (dist >= minDist && dist <= maxDist) {

				if (debugMode>0) {
					//Draw the bounding rectangle and position of the two circles
					cvRectangle(frame, cvPoint(r.x, r.y), cvPoint(r.x+r.width, r.y+r.height), cvScalar(0,255,255), 2);
					cvCircle(frame, rCenter, 2, cvScalar(0,0,255),2);
					cvCircle(frame, yCenter, 2, cvScalar(0,255,255),2);
				}

				//Calculate the angle made by the two circles
				angle = 90+(atan2((float)(yCenter.y - rCenter.y),(float)(yCenter.x - rCenter.x))*57.295779513082320876798154814105);
				if (angle<0) angle+=360;

				retVal = true;
				if (debugMode>0) {
					//Render the angle on the frame
					char cAngle[50]; sprintf(cAngle, "%.2f", angle);
					CvFont font;
					cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1, 1, 0, 2, 8);
					cvPutText(frame, cAngle, cvPoint(5,25), &font, cvScalar(255,0,0,0));

					cvShowImage("logo", frame);
				}
			}
		}
	}

	cvReleaseImage(&bwImageYellow); cvReleaseImage(&bwImageRed); cvReleaseImage(&bwImageGrey);
	cvReleaseMemStorage(&ms);
	cvReleaseImage(&bwDiff);

	return retVal;
}

float MastercardDetector::getAngle() {
	return angle;
}

/*Calculate the normalised RGB of a colour*/
void NRGB(float r, float g, float b, float *nr, float *ng, float *nb) {
	if(r+g+b > 0){
		*nr = r/(r+g+b);
		*ng = g/(r+g+b);
		*nb = 1.0f - *nr - *ng;
	} else {
		*nr = *ng = *nb = 1.0f/3.0f;
	}
}

/*Find the difference between each pixel in an image and a colour using normalized RGB*/
IplImage* rgbSub(IplImage *src, CvScalar col, float bThresh) {
	float ncR, ncG, ncB;

	//Calculate the normalized RGB of the colour
	NRGB(col.val[0], col.val[1], col.val[2], &ncR, &ncG, &ncB);

	//Create the floating point src and dest and return images
	IplImage *srcf = cvCreateImage(cvGetSize(src), IPL_DEPTH_32F, 3); cvScale(src, srcf, 1.0f/255.0f);
	IplImage *dstf = cvCreateImage(cvGetSize(src), IPL_DEPTH_32F, 1); 
	IplImage *dst = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);

	//Loop through each pixel
	for (int i=0; i<src->height; i++) {
		for (int j=0; j<src->width; j++) {
			float nr, ng, nb;
			float r, g, b;
			b = CV_IMAGE_ELEM(srcf,float,i,j*3);
			g = CV_IMAGE_ELEM(srcf,float,i,j*3+1);
			r = CV_IMAGE_ELEM(srcf,float,i,j*3+2);

			//Calculate it's normalized RGB
			NRGB(r, g, b, &nr, &ng, &nb);

			//Find the difference
			if (r+g+b < bThresh) {
				CV_IMAGE_ELEM(dstf,float,i,j) = 10000;
			} else {
				CV_IMAGE_ELEM(dstf,float,i,j) = ((nr-ncR)*(nr-ncR)+(ng-ncG)*(ng-ncG)+(nb-ncB)*(nb-ncB)) * (r+g+b);
			}
		}
	}
	//Convert the scale back into float points	
	cvConvertScale(dstf, dst, 1000);

	//Clean up
	cvReleaseImage(&srcf); cvReleaseImage(&dstf);
	return dst;
}

void toSpecGreyscale(IplImage *src, IplImage *dest, CvScalar col1, CvScalar col2) {
	float ncR, ncG, ncB;
	float x, y, z;
	x = col1.val[0]-col2.val[0];
	y = col1.val[1]-col2.val[1];
	z = col1.val[2]-col2.val[2];

	//Create the floating point src and dest and return images
	ncR = 1.0;
	ncG = (-ncR*(x-z)-z)/(y-z);
	ncB = 1.0 - (ncR + ncG);
	//Loop through each pixel
	for (int i=0; i<src->height; i++) {
		for (int j=0; j<src->width; j++) {
			float r, g, b;
			b = CV_IMAGE_ELEM(src,unsigned char,i,j*3);
			g = CV_IMAGE_ELEM(src,unsigned char,i,j*3+1);
			r = CV_IMAGE_ELEM(src,unsigned char,i,j*3+2);

			CV_IMAGE_ELEM(dest,unsigned char,i,j) = max(0, min(255, r * ncR + g * ncG + b * ncB));
		}
	}


}