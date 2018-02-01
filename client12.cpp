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

double default_simSpeed = 5000;//(mm/s)
double simSpeed = default_simSpeed;//(mm/s)

double cycleStepSize = 800;

int action;
int red_frag;
bool death = false;

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
    if(action == 100){
      death=true;
      _RcControl.SetSteerAngle(0);
      _RcControl.SetDriveSpeed(-200);
      simSpeed = default_simSpeed;
      return ;
    }

    else if(action == 101){
      death=true;
      _RcControl.SetSteerAngle(0);
      _RcControl.SetDriveSpeed(0);
      simSpeed = default_simSpeed;
      cout << "Stop now"<< endl;
      auto stopStartTime = chrono::system_clock::now();
      while(true){
        auto nowTime5 = chrono::system_clock::now();
        diff = double(chrono::duration_cast<chrono::milliseconds>(nowTime5-stopStartTime).count());
        cout<<"Stop : "<<diff/1000<<"s"<<endl;
        if(diff/1000>30){
          break;
        }
      }
      return ;
    }
    death = false;
    switch(action){
      case 0:
        _RcControl.SetSteerAngle(-27);
        break;
      case 1:
        _RcControl.SetSteerAngle(0);
        //simSpeed += 500;
        break;
      case 2:
        _RcControl.SetSteerAngle(27);
        break;
      case 3:
        _RcControl.SetSteerAngle(0);
        simSpeed -= 500;
        break;
      case 4:
        _RcControl.SetSteerAngle(0);
        break;
    }

    if(simSpeed<default_simSpeed){
      simSpeed = default_simSpeed;
    }
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

    cv::VideoCapture capture_over(0); //0->upper 1->under
    cv::VideoCapture capture_under(1); //0->upper 1->unde

    if(!capture_over.isOpened()){  // カメラが使えない場合はプログラムを止める
        cout << "ERROR: cannot open cam device!!!!!!" << endl;
        return -1;
    }
    if(!capture_under.isOpened()){  // カメラが使えない場合はプログラムを止める
        cout << "ERROR: cannot open cam device!!!!!!" << endl;
        return -1;
    }
    cv::Mat frame_over;
    cv::Mat frame_under;

    // Open ZMQ Connection
    zmq::context_t context (1);

    zmq::socket_t socket_chen (context, ZMQ_REQ);
    socket_chen.connect ("tcp://192.168.11.14:5555");

    zmq::socket_t socket_oku (context, ZMQ_REQ);
    socket_oku.connect ("tcp://192.168.11.12:5555");
    cout<<"Connecting..."<<endl;

    auto lastSendTime_chen = chrono::system_clock::now();
    auto lastSendTime_oku = chrono::system_clock::now();
    double diff_sum_chen = 0;
    double diff_sum_oku = 0;
    int i = 1; //chenのプログラムにSendした回数
    int j = 1; //okuyamaのプログラムにSendした回数

    _RcControl.SetSteerAngle(0);
    while (1) {
        capture_over >> frame_over;
        auto nowTime1 = chrono::system_clock::now();
        diff = double(chrono::duration_cast<chrono::milliseconds>(nowTime1-lastSendTime_chen).count());

        if(diff > 2000 && death == false){
          cv::resize(frame_over, frame_over, cv::Size(960,544));

          void* data = malloc(frame_over.total() * frame_over.elemSize()); // Pixel data
          memcpy(data, frame_over.data, frame_over.total() * frame_over.elemSize());

          // Send Pixel data
          zmq::message_t msg(data, frame_over.total() * frame_over.elemSize(), my_free, NULL);
          socket_chen.send(msg);

          lastSendTime_chen = chrono::system_clock::now();

          //  Get the reply.
          zmq::message_t rcv_msg;
          socket_chen.recv(&rcv_msg, 0);
          red_frag = *(int*)rcv_msg.data();


          auto nowTime2 = chrono::system_clock::now();
          diff = double(chrono::duration_cast<chrono::milliseconds>(nowTime2-lastSendTime_chen).count());
          diff_sum_chen += diff;
          cout << "Reseived Red frag : " << red_frag << endl;
          cout << "Chen's Replay Time : "<<diff<< endl;
          cout << "Averagge Replay Time : "<<diff_sum_chen/i<< endl;
          cout << "-------------------------------------"<< endl;
          i++;
        }

        if(red_frag > 0){
          _RcControl.SetDriveSpeed(0);
          continue;
        }
        //else{
          //_RcControl.SetDriveSpeed(calcRealSpeed()/2); //ゆっくりスタート
        //}

        capture_under >> frame_under;
        auto nowTime3 = chrono::system_clock::now();
        diff = double(chrono::duration_cast<chrono::milliseconds>(nowTime3-lastSendTime_oku).count());
        if(diff > cycleStepSize){
            cv::resize(frame_under, frame_under, cv::Size(227,227));

            void* data = malloc(frame_under.total() * frame_under.elemSize()); // Pixel data
            memcpy(data, frame_under.data, frame_under.total() * frame_under.elemSize());

            // Send Pixel data
            zmq::message_t msg(data, frame_under.total() * frame_under.elemSize(), my_free, NULL);
            socket_oku.send(msg);

            lastSendTime_oku = chrono::system_clock::now();

            //  Get the reply.
            zmq::message_t rcv_msg;
            socket_oku.recv(&rcv_msg, 0);
            action = *(int*)rcv_msg.data();

            auto nowTime4 = chrono::system_clock::now();
            diff = double(chrono::duration_cast<chrono::milliseconds>(nowTime4-lastSendTime_oku).count());
            diff_sum_oku += diff;
            //cout << "Reseived Action : " << action << endl;
            //cout << "Oku's Replay Time : "<<diff<< endl;
            //cout << "Averagge Replay Time : "<<diff_sum_oku/j<< endl;
            //cout << "-------------------------------------"<< endl;
            ApplyAction(action);
            j++;
        }
    }
    _RcControl.SetDriveSpeed(0);
    cv::destroyAllWindows();
    return 0;
}
