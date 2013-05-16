#include "opencv/cv.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

using namespace cv;
using namespace std;

void FindFaces(Mat *frame, CvMemStorage *pStorage, CvHaarClassifierCascade *cascade);
void MotionDetection(const Mat &past, const Mat &now, Mat *motion);

#define WINDOW_NAME "Webcam"
#define THRESH 20

int main( int argc, char** argv ) {
    cvNamedWindow( WINDOW_NAME, CV_WINDOW_NORMAL);
    
    int width = 640;
    int height = 480;

    CvCapture* capture = cvCreateCameraCapture(0) ;

    Mat frame, oldFrame, motion;
    CvMemStorage* pStorage = cvCreateMemStorage(0);
    CvHaarClassifierCascade* face_cascade = (CvHaarClassifierCascade *)cvLoad("face.xml",0,0,0);
    
    int args = 0;
    if(argc > 3) {
        args = atoi(argv[1]);
        width = atoi(argv[2]);
        height = atoi(argv[3]);
    } else if(argc > 1) {
        if(atoi(argv[1]) == 1)
            args = 1;
        else if(atoi(argv[1]) == 2)
            args = 2;
    }
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, width);
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, height);
   
    bool firstRun = true;
    if(args == 2)
        motion = frame.clone(); 
    while(1) {
        timeval beg, end;
        gettimeofday(&beg, NULL);
        if(!firstRun)
            oldFrame = frame.clone();
        IplImage *img = cvQueryFrame( capture );
        if( !img ) break;
        frame = img;
        
        if(args == 1) {
            FindFaces(&frame,pStorage,face_cascade);
        } else if(args == 2) {
            if(!firstRun) {
                MotionDetection(oldFrame,frame,&motion);
                cvShowImage( WINDOW_NAME, &motion);
            } else
                firstRun = false;
        } else
            cvShowImage( WINDOW_NAME, &frame );
        gettimeofday(&end, NULL);
        double elapsed = (end.tv_sec - beg.tv_sec) + 
                          ((end.tv_usec - beg.tv_usec)/1000000.0);
        printf("time: %f\n", elapsed);
        char c = cvWaitKey(33);
        if( c == 27 ) break;

    }
    cvReleaseCapture( &capture );
    cvDestroyWindow( "Webcam" );
}

void FindFaces(Mat *frame, CvMemStorage *pStorage, CvHaarClassifierCascade *cascade) {
    //find faces using haar recognition
    CvSeq * pFaceRectSeq = cvHaarDetectObjects
        (frame, cascade, pStorage,
        1.1,                       // increase search scale by 10% each pa
        3,                         // merge groups of three detections
        CV_HAAR_DO_CANNY_PRUNING,  // skip regions unlikely to contain a face
        cvSize(40,40));            // smallest size face to detect = 40x40

    //draw rectangles around the face
    for(int i=0;i<(pFaceRectSeq? pFaceRectSeq->total:0); i++ ) {
        CvRect* r = (CvRect*)cvGetSeqElem(pFaceRectSeq, i);
        CvPoint pt1 = { r->x, r->y };
        CvPoint pt2 = { r->x + r->width, r->y + r->height };
        cvRectangle(frame, pt1, pt2, CV_RGB(0,255,0), 3, 4, 0);
    }
    cvShowImage( WINDOW_NAME, frame );

}

inline int abs(int val) {
    if(val < 0)
        return val * -1;
    else
        return val;
}

void MotionDetection(const Mat &past, const Mat &now, Mat *motion) {
      int nl = past.rows; // number of lines
      // total number of element per line
      int nc = past.cols; 

      const unsigned char *pastData = reinterpret_cast<const unsigned char *>(past.data);
      const unsigned char *nowData = reinterpret_cast<const unsigned char *>(now.data);
      unsigned char *motionData = reinterpret_cast<unsigned char *>(motion->data);
      for (int i=1; i<nl; i++) {
            for (int j=0; j<nc; j+= past.channels()) {
            // process each pixel ---------------------
                  int r_diff = abs(pastData[j] - nowData[j]);
                  int g_diff = abs(pastData[j+1] - nowData[j+1]);
                  int b_diff = abs(pastData[j+2] - nowData[j+2]);
                  if(r_diff > THRESH || g_diff > THRESH || b_diff > THRESH) {
                    motionData[j] = 255;
                    motionData[j+1] = 255;
                    motionData[j+2] = 255;
                  } else {
                    motionData[j] = 0;
                    motionData[j+1] = 0;
                    motionData[j+2] = 0;
                  }
            }
            pastData+= past.channels();
            nowData+= past.channels();
            motionData+= past.channels();
      }
}
