#ifndef __CAMERA__
#define __CAMERA__

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "geometry_msgs/msg/point.hpp"
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <math.h>
#include <iostream>

#include <chrono>
using namespace std::chrono_literals;
using namespace std::placeholders;

class Camera : public rclcpp::Node{
public:
    Camera() : Node("control") {
        init_interfaces();
        init_camera();
    };

private:
    void timer_callback();
    void init_interfaces();
    void init_camera();

    std::chrono::milliseconds loop_dt_ = 40ms;

    rclcpp::TimerBase::SharedPtr timer_;

    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_publisher;
    rclcpp::Publisher<geometry_msgs::msg::Point>::SharedPtr coord_publisher;

    cv::VideoCapture cap;
    cv::Mat frame;

};


#endif