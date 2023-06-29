#include "ros_interface_umi_rtx/node_camera.hpp"

void Camera::init_interfaces(){
    m_cx = 0;
    m_cy = 0;

    timer_ = this->create_wall_timer(loop_dt_,std::bind(&Camera::timer_callback,this));

    image_publisher = this->create_publisher<sensor_msgs::msg::Image>("processed_image",10);
    // coord_publisher = this->create_publisher<geometry_msgs::msg::Point>("target_position",10);
}

void Camera::init_camera(){
    cap.open(0); // webcam
    //cap.open(4) // robot's camera
    // cap.open(4);

    if (!cap.isOpened()) {
        std::cout << "ERROR! Unable to open camera" << std::endl;
    }

    // Stereo Calibration of the ZED M device
    stereo_calibration();

    m_frame_width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    m_frame_height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    //std::cout << "init done" << std::endl;
}

void Camera::timer_callback(){
    cap.read(frame);

    cv::Mat hsv_img;
    cv::cvtColor(frame,hsv_img,cv::COLOR_BGR2HSV);

    cv::Scalar lower_bound = cv::Scalar(20,100,100);
    cv::Scalar upper_bound = cv::Scalar(60,255,255);

    cv::Mat bin_hsv_img;
    cv::inRange(hsv_img, lower_bound, upper_bound, bin_hsv_img);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(bin_hsv_img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

    if(contours.empty()){
        //std::cout << "Cannot detect the target" << std::endl;

        cv::circle(frame,cv::Point(m_frame_width-40,40),20,cv::Scalar(0,0,255),-1);

        cv::line(frame,cv::Point (m_frame_width/2 - 25,m_frame_height/2),cv::Point (m_frame_width/2 + 25,m_frame_height/2),cv::Scalar(255,255,255),2);
        cv::line(frame,cv::Point (m_frame_width/2,m_frame_height/2 - 25),cv::Point (m_frame_width/2,m_frame_height/2 + 25),cv::Scalar(255,255,255),2);

        sensor_msgs::msg::Image::SharedPtr img_msg = cv_bridge::CvImage(std_msgs::msg::Header(),"bgr8",frame).toImageMsg();
        image_publisher->publish(*img_msg);
    }

    else{
        double maxArea = 0;
        int maxAreaIdx = -1;

        for (int i = 0; i < contours.size(); i++)
        {
            double area = cv::contourArea(contours[i]);

            if (area > maxArea)
            {
                maxArea = area;
                maxAreaIdx = i;
            }
        }

        //std::cout << "indice : " << maxAreaIdx << std::endl;

        geometry_msgs::msg::Point coord_msg;

        if(maxAreaIdx > -1) {
            cv::drawContours(frame, contours, maxAreaIdx, cv::Scalar(255, 255, 255), 2);

            cv::Moments moments = cv::moments(contours[maxAreaIdx]);

            if (moments.m00 != 0) {
                double cx = moments.m10 / moments.m00;
                double cy = moments.m01 / moments.m00;
                //std::cout << "Centroid : (" << cx << ", " << cy << ")" << std::endl;

                coord_msg.x = cx;
                coord_msg.y = cy;
                m_cx = cx;
                m_cy = cy;
            }

            else {
                std::cout << "Impossible centroid calculation" << std::endl;

                coord_msg.x = m_cx;
                coord_msg.y = m_cy;

                cv::circle(frame,cv::Point(m_frame_width-40,40),20,cv::Scalar(100,50,100),-1);
            }
        }

        else{
            coord_msg.x = m_cx;
            coord_msg.y = m_cy;
        }
        // coord_publisher->publish(coord_msg);

        cv::circle(frame,cv::Point(m_frame_width-40,40),20,cv::Scalar(0,255,0),-1);

        cv::line(frame,cv::Point (m_frame_width/2 - 25,m_frame_height/2),cv::Point (m_frame_width/2 + 25,m_frame_height/2),cv::Scalar(255,255,255),2);
        cv::line(frame,cv::Point (m_frame_width/2,m_frame_height/2 - 25),cv::Point (m_frame_width/2,m_frame_height/2 + 25),cv::Scalar(255,255,255),2);

        sensor_msgs::msg::Image::SharedPtr img_msg = cv_bridge::CvImage(std_msgs::msg::Header(), "bgr8", frame).toImageMsg();
        image_publisher->publish(*img_msg);
    }
}

void Camera::get_angles(){
    cv2::Mat image = frame;
    
}

void stereo_calibration(){
    std::cout << "Calibrating the stereo device...\n" << std::endl;

    std::vector<std::vector<cv::Point3f>> objectPoints;
    std::vector<std::vector<cv::Point2f>> cornersLeft;
    std::vector<std::vector<cv::Point2f>> cornersRight;

    for(int i=1;i<=5;i++){
        cv::Mat imageLeft = cv::imread("CalibrationImages/leftImage0" + std::to_string(i) + ".png", cv::IMREAD_GRAYSCALE);
        cv::Mat imageRight = cv::imread("CalibrationImages/rightImage0" + std::to_string(i) + ".png", cv::IMREAD_GRAYSCALE);

        if(imageLeft.empty()){
            std::cout << "Error reading the calibration left image " << i << std::endl;
        }
        if(imageRight.empty()){
            std::cout << "Error reading the calibration right image " << i << std::endl;
        }

        cv::Size imageLeftSize = imageLeft.size();
        cv::Size imageRightSize = imageRight.size();
        //std::cout << "Left image size: " << imageLeftSize << std::endl;
        //std::cout << "Right image size: " << imageRightSize << std::endl;

        std::vector<cv::Point2f> cornersLeftSub;
        std::vector<cv::Point2f> cornersRightSub;

        bool patternFoundLeft = cv::findChessboardCorners(imageLeft, m_patternSize, cornersLeftSub, cv::CALIB_CB_ADAPTATIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);
        bool patternFoundRight = cv::findChessboardCorners(imageRight, m_patternSize, cornersRightSub, cv::CALIB_CB_ADAPTATIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);
        //std::cout << "Found corners on left image? " << patternFoundLeft << std::endl;
        //std::cout << "Found corners on right image? " << patternFoundRight << std::endl;
        //std::cout << "Number of corners detected in image " << i << " (left) test1: " << cornersLeftSub.size() << std::endl;
        //std::cout << "Number of corners detected in image " << i << " (right) test1: " << cornersRightSub.size() << std::endl;

        if(patternFoundLeft && patternFoundRight){
            cv::cornerSubPix(imageLeft, cornersLeftSub, cv::Size(5,5), cv::Size(-1,-1), cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1));
            cv::cornerSubPix(imageRight, cornersRightSub, cv::Size(5,5), cv::Size(-1,-1), cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1));

            cornersLeft.push_back(cornersLeftSub);
            cornersRight.push_back(cornersRightSub);

            std::vector<cv::Point3f> objectPointsImage;

            for (int y = 0; y < patternSize.height; y++) {
                for (int x = 0; x < patternSize.width; x++) {
                    objectPointsImage.push_back(cv::Point3f(x * squareSize, y * squareSize, 0));
                }
            }

            objectPoints.push_back(objectPointsImage);
        }

        //cv::drawChessboardCorners(imageLeft, patternSize, cornersLeftSub, patternFoundLeft);
        //cv::drawChessboardCorners(imageRight, patternSize, cornersRightSub, patternFoundRight);
    }

    m_rms_error = cv::stereoCalibrate(objectPoints, cornersLeft, cornersRight, m_cameraMatrixLeft, m_distCoeffsLeft, m_cameraMatrixRight, m_distCoeffsRight, m_imageLeftSize, m_R, m_T, m_E, m_F);

    std::cout << "Stereo device calibrated\n" << std::endl;
}

int main(int argc, char * argv[]){
    rclcpp::init(argc,argv);
    std::shared_ptr<rclcpp::Node> node = std::make_shared<Camera>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}