#include "cv.h"

/*Class for the master card detector */
class MastercardDetector {
public:
	//Constructor takes the red and yellow colour values, a threshold, and optionally debug value
	MastercardDetector(CvScalar red, CvScalar yellow, int thresh, float bThresh, int minContourSize, int minDist, int maxDist, int debug=0);
	~MastercardDetector();

	//Setter methods for the colours, debug mode and threshold
	void setColours(CvScalar red, CvScalar yellow);
	void setDebugMode(int debug);
	void setThreshold(int thresh);

	//function to find the orientation of the logo
	bool findLogo(IplImage *frame);
	float getAngle();
private:
	int debugMode;
	int minDist, maxDist;
	CvScalar cRed, cYellow;
	int cThresh;
	float angle;
	float bThresh;
	int minContSize;
};
