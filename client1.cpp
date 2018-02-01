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

double simCycleStepSize = 50;//0.05s

double min_simSpeed = 5000;//(mm/s)
double max_simSpeed = 5000;//(mm/s)
double simSpeed = min_simSpeed;//(mm/s)

double cycleStepSize = 1000;
int action;

double diff; // timelag
RcControl  _RcControl;


void my_free(void *data, void *hint)
{
    free(data);
}

double calcRealSpeed(){
  float cyckleStepRatio = cycleStepSize/simCycleStepSize;
  return (simSpeed/2)/cyckleStepRatio;
}

void ApplyAction(int action){
    //if(10 <= action && action< 20){
      //_RcControl.SetSteerAngle(0);
      //simSpeed = 4000;
      //action -= 10;
    //}

    if(action == 100){
      _RcControl.SetSteerAngle(0);
      _RcControl.SetDriveSpeed(-200);
      simSpeed = min_simSpeed;
      return ;
    }

    else if(action == 101){
        _RcControl.SetSteerAngle(0);
        _RcControl.SetDriveSpeed(0);
        simSpeed = min_simSpeed;
        cout << "Stop now"<< endl;
        auto stopStartTime = chrono::system_clock::now();
        while(true){
          auto nowTime5 = chrono::system_clock::now();
          diff = double(chrono::duration_cast<chrono::milliseconds>(nowTime5-stopStartTime).count());
          cout<<"Stop : "<<diff/1000<<"s"<<endl;
          if(diff/1000>10){
            break;
          }
        }
        return ;
    }
    switch(action){
      case 0:
        _RcControl.SetSteerAngle(-27);
        break;
      case 1:
        _RcControl.SetSteerAngle(0);
        break;
      case 2:
        _RcControl.SetSteerAngle(27);
        break;
      case 3:
        _RcControl.SetSteerAngle(0);
        break;
      case 4:
        _RcControl.SetSteerAngle(0);
        break;
    }
    /*
    if(simSpeed<min_simSpeed){
      simSpeed = min_simSpeed;
    }
    else if(simSpeed>max_simSpeed){
      simSpeed = max_simSpeed;
    }
    */
    double speed = calcRealSpeed();
    cout<<"speed : "<<speed<<endl;
    _RcControl.SetDriveSpeed(speed);
}

int main (int argc, char **argv){
    _RcControl.init();
    _RcControl.Start();

    _RcControl.SetReportFlagReq(0x0f);
    _RcControl.SetServoEnable(1);
    _RcControl.SetMotorEnableReq(1);
    _RcControl.SetDriveSpeed(0);
    _RcControl.SetSteerAngle(0);

    cv::VideoCapture capture_under(1); //0->upper 1->under
    // カメラが使えない場合はプログラムを止める
    if(!capture_under.isOpened()){
        cout << "ERROR: cannot open cam device." << endl;
        return -1;
    }
    cv::Mat frame_under;

    // Open ZMQ Connection
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    socket.connect ("tcp://192.168.11.12:5555");
    //socket.connect ("tcp://133.12.48.118:5555");
    //socket.connect ("tcp://localhost:5555");
    cout<<"Connecting..."<<endl;

    auto lastSendTime = chrono::system_clock::now();
    double diff_sum = 0;
    int j = 1;

    _RcControl.SetSteerAngle(0);
    while (1) {
        capture_under >> frame_under;
        auto nowTime1 = chrono::system_clock::now();
        // get runtime as microseconds
        diff = double(chrono::duration_cast<chrono::milliseconds>(nowTime1-lastSendTime).count());
        //cout<<"Send wait : "<<diff<<endl;

        if(diff > cycleStepSize){
            cv::resize(frame_under, frame_under, cv::Size(227,227));

            void* data = malloc(frame_under.total() * frame_under.elemSize()); // Pixel data
            memcpy(data, frame_under.data, frame_under.total() * frame_under.elemSize());

            // Send Pixel data
            zmq::message_t msg(data, frame_under.total() * frame_under.elemSize(), my_free, NULL);
            socket.send(msg);

            lastSendTime = chrono::system_clock::now();

            //  Get the reply.
            zmq::message_t rcv_msg;
            socket.recv(&rcv_msg, 0);
            action = *(int*)rcv_msg.data();

            auto nowTime2 = chrono::system_clock::now();
            diff = double(chrono::duration_cast<chrono::milliseconds>(nowTime2-lastSendTime).count());
            diff_sum += diff;
            cout << "Reseived Action : " << action << endl;
            cout << "Replay Time : "<<diff<< endl;
            cout << "Averagge Replay Time : "<<diff_sum/j<< endl;
            ApplyAction(action);
            j++;
        }
    }
    _RcControl.SetDriveSpeed(0);
    cv::destroyAllWindows();
    return 0;
}
