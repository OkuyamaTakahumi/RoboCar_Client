#include <cv.h>
#include <highgui.h>
#include <ctype.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <zmq.hpp>

//#include <ctime>
#include <iostream>

#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "RcControl.h"

#include <chrono>
#include <algorithm>

using namespace zmp::zrc;
using namespace std;

double sim_cycleStepSize = 50;//0.05s
double sim_roboSpeed = 2500;//(mm/s)

double cycleStepSize = 500;//0.4s
int red_frag;

double diff; // timelag
//auto lastSpeedUpTime = chrono::system_clock::now();

double w = 1227, h = 1227;  //size of the image file
int c,count=0;
int cnt=0;
char fName[32];
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

void my_free(void *data, void *hint)
{
    free(data);
}

double calcRealSpeed(){
  float cyckleStepRatio = cycleStepSize/sim_cycleStepSize;
  return (sim_roboSpeed/2)/cyckleStepRatio;
}


int main (int argc, char **argv){
    _RcControl.init();
    _RcControl.Start();

    _RcControl.SetReportFlagReq(0x0f);
    _RcControl.SetServoEnable(1);
    _RcControl.SetMotorEnableReq(1);
    _RcControl.SetDriveSpeed(0);
    _RcControl.SetSteerAngle(0);

    cv::VideoCapture capture(0); //0->upper 1->under
    // カメラが使えない場合はプログラムを止める
    if(!capture.isOpened()){
        cout << "ERROR: cannot open cam device!!!!!!" << endl;
        return -1;
    }
    cv::Mat frame;

    // Open ZMQ Connection
    zmq::context_t context (1);
    zmq::socket_t socket_Chen (context, ZMQ_REQ);
    socket_Chen.connect ("tcp://192.168.11.14:5555");
    cout<<"Connecting..."<<endl;

    auto lastSendTime = chrono::system_clock::now();
    double diff_sum = 0;
    int i = 1;
    // (3)カメラから画像をキャプチャする
    while (1) {
        capture >> frame;
        auto nowTime = chrono::system_clock::now();
        // get runtime as microseconds
        diff = double(chrono::duration_cast<chrono::milliseconds>(nowTime-lastSendTime).count());
        //cout<<"Send wait : "<<diff<<endl;

        //if(diff > cycleStepSize){
        if(diff > 1500){
            cv::resize(frame, frame, cv::Size(960,544));
            // Pixel data
            void* data = malloc(frame.total() * frame.elemSize());
            memcpy(data, frame.data, frame.total() * frame.elemSize());

            // Send Pixel data
            zmq::message_t msg(data, frame.total() * frame.elemSize(), my_free, NULL);
            socket_Chen.send(msg);

            lastSendTime = chrono::system_clock::now();

            //  Get the reply.
            zmq::message_t rcv_msg;
            socket_Chen.recv(&rcv_msg, 0);
            red_frag = *(int*)rcv_msg.data();


            auto nowTime = chrono::system_clock::now();
            diff = double(chrono::duration_cast<chrono::milliseconds>(nowTime-lastSendTime).count());
            diff_sum += diff;
            cout << "Reseived Red frag : " << red_frag << endl;
            cout << "Replay Time : "<<diff<< endl;
            cout << "Averagge Replay Time : "<<diff_sum/i<< endl;

            if(red_frag > 0){
              _RcControl.SetDriveSpeed(0);
            }
            else{
              _RcControl.SetDriveSpeed(calcRealSpeed());
            }

            i++;
        }
    }
    _RcControl.SetDriveSpeed(0);
    cv::destroyAllWindows();
    return 0;
}
