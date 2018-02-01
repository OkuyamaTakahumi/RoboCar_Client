#include <cv.h>
#include <highgui.h>
#include <ctype.h>
#include "opencv2/opencv.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <zmq.hpp>

#include <ctime>
#include <iostream>

#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "RcControl.h"


using namespace zmp::zrc;
using namespace std;

int
main (int argc, char **argv)
{

  double w = 320, h = 240;
  char fName[32];
  int c;
  int cnt=543;

  char buf[20];
  float angle = 0;
  int speed = 0;
  int set_v = 0;
  int set_t = 0;
  //int ang=0;
  bool sign = false;
  float set_a = 0;
  bool period = false;
  bool loop = true;
  RcControl  _RcControl;
  _RcControl.init();
  _RcControl.Start();

  _RcControl.SetReportFlagReq(0x0f);
  _RcControl.SetServoEnable(1);
  _RcControl.SetMotorEnableReq(1);
  _RcControl.SetDriveSpeed(0);
  _RcControl.SetSteerAngle(0);

    CvCapture *capture1 = cvCaptureFromCAM(0);
    //CvCapture *capture2 = cvCaptureFromCAM(1);

    if( !capture1 ) return 1;
    //if (!capture2) return 1 ;
    cvNamedWindow("Video1");
    //cvNamedWindow("Video2") ;


  cvSetCaptureProperty (capture1, CV_CAP_PROP_FRAME_WIDTH, w);
  cvSetCaptureProperty (capture1, CV_CAP_PROP_FRAME_HEIGHT, h);

  //cvSetCaptureProperty (capture2, CV_CAP_PROP_FRAME_WIDTH, w);
  //cvSetCaptureProperty (capture2, CV_CAP_PROP_FRAME_HEIGHT, h);

    while(true) {
        //grab and retrieve each frames of the video sequentially
        IplImage* frame1 = cvQueryFrame( capture1 );
        //IplImage* frame2 = cvQueryFrame( capture2 );

        //if( !frame1 || !frame2 ) break;

        cvShowImage( "Video1", frame1 );
        //cvShowImage( "Video2", frame2 );

     //sprintf(fName, "img1/%010d.png", cnt);
	   //cvSaveImage(fName, frame1);

     //sprintf(fName, "img2/%010d.png", cnt);
	   //cvSaveImage(fName, frame2);

    cnt+=1;
    c = cvWaitKey (0);
	  cout<< c << endl;


    if (c == 1113939 ) // right

    {
        if(angle < 30)
           {

            angle += 10;
        printf("send steer angle = %2.1f\n", angle);
        _RcControl.SetSteerAngle(angle);
        }
    }
        if (c == 1113937 ) // left
        {
            if(angle > -30)
              {

                angle -= 10;
            printf("send steer angle = %2.1f\n", angle);
            _RcControl.SetSteerAngle(angle);
            }
        }

    if (c == 1113938 ) // forward
    {
	speed += 50;
    if ( speed >= 300 )
    speed = 300;
        _RcControl.SetDriveSpeed(speed); }

    if (c == 1113940 ) // back
    { speed -= 50;
    if ( speed <= -300 )
    speed = -300;
        _RcControl.SetDriveSpeed(speed); }

    if (c == 1114081 ) // shift break
	speed = 0;
        _RcControl.SetDriveSpeed(speed);



    if (c == 1048603 ) // esc

       {
        _RcControl.SetDriveSpeed(0);
    _RcControl.SetSteerAngle(0);
    break;}



    }

    return 0;

}
