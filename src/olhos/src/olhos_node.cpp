#include "rclcpp/rclcpp.hpp"
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/int32.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <cv_bridge/cv_bridge.hpp>
#include <sensor_msgs/msg/image.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include <string>
#include <vector>

using namespace cv;

class Olhos_Node : public rclcpp::Node {
public:
    Olhos_Node() : Node("olhos_node") {
        this->declare_parameter<int>("camera", 0);
        this->get_parameter("camera", camera);

        if (!face_cascade.load(
            "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml"))
        {
            RCLCPP_ERROR(this->get_logger(), "Erro ao carregar Haar Cascade!");
        }

        ///////

        if (!this->iniciar_serial("/dev/ttyUSB0")) {
    	    RCLCPP_ERROR(this->get_logger(), "Falha na serial");
	}

        ///////

        cap.open(camera);
        if (!cap.isOpened()) {
            RCLCPP_ERROR(this->get_logger(), "Erro ao abrir câmera!");
        }

        ///////

        pub_ser_ = this->create_publisher<std_msgs::msg::String>("olhos/serial", 10);
        pub_pes_ = this->create_publisher<std_msgs::msg::String>("olhos/pescoco", 10);
        pub1_mem_ = this->create_publisher<sensor_msgs::msg::Image>("olhos/o1/memoria", 10);
        pub2_mem_ = this->create_publisher<std_msgs::msg::String>("olhos/o2/memoria", 10);

        ///////

        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(33),
            std::bind(&Olhos_Node::processar_frame, this)
        );

        ////////

        sub_cer_ = this->create_subscription<std_msgs::msg::String>(
            "cerebro/olhos", 10,
            std::bind(&Olhos_Node::callback_cerebro, this, std::placeholders::_1)
        );

    }

private:
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_ser_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_pes_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub2_mem_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub1_mem_;

    rclcpp::TimerBase::SharedPtr timer_;

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_cer_;

    ///////

    CascadeClassifier face_cascade;
    VideoCapture cap;

    Mat frame, gray;

    ///////

    std_msgs::msg::String msg_cen_; // variavel offset centralização
    std_msgs::msg::String msg_cad_; // variavel nome da pessoa a ser cadastrada

    int camera;

    ///////

    int serial_fd;

    ///////

    void callback_cerebro(const std_msgs::msg::String::SharedPtr msg){
        msg_cad_.data = msg->data; 
    }

    ///////

    void processar_frame() {
        cap >> frame;
        if (frame.empty()) return;

        if (!msg_cad_.data.empty()){ // cadastra rostos
            std::vector<Rect> faces;

            int width  = frame.cols;
            int height = frame.rows;
            int centro_x = width / 2;
            int centro_y = height / 2;

            cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            equalizeHist(gray, gray);

            face_cascade.detectMultiScale(gray, faces, 1.1, 4, 0, cv::Size(30, 30));

            if (!faces.empty()) {
                Rect maior = faces[0];

                for (auto &f : faces)
                    if (f.area() > maior.area())
                        maior = f;

                int face_centro_x = maior.x + maior.width / 2;
                int face_centro_y = maior.y + maior.height / 2;

                int off_set_y = face_centro_y - centro_y;
                int off_set_x = face_centro_x - centro_x;

                std::string comando = std::to_string(off_set_x) + "," + std::to_string(off_set_y) + "\n";
                this->enviar_serial(comando);

                if ((off_set_x <= 5) && (off_set_x >= -5) &&
                    (off_set_y <= 5) && (off_set_y >= -5)){

                    cv_bridge::CvImage msg_fac;
                    msg_fac.header.stamp = this->now();
                    msg_fac.header.frame_id = "camera_link";

                    msg_fac.encoding = "bgr8";
                    msg_fac.image = frame(maior).clone();

                    pub1_mem_->publish(*msg_fac.toImageMsg());
                    pub2_mem_->publish(msg_cad_);

                    msg_cad_.data = "";
                }
            }
        }
    }

    ///////

    bool iniciar_serial(const std::string& porta = "/dev/ttyUSB0") {
    	serial_fd = open(porta.c_str(), O_RDWR | O_NOCTTY);

    	if (serial_fd < 0) {
            RCLCPP_ERROR(this->get_logger(), "Erro ao abrir serial");
            return false;
    	}

    	struct termios tty;
    	tcgetattr(serial_fd, &tty);

    	cfsetispeed(&tty, B9600);
    	cfsetospeed(&tty, B9600);

    	tty.c_cflag |= (CLOCAL | CREAD);
    	tty.c_cflag &= ~CSIZE;
    	tty.c_cflag |= CS8;
    	tty.c_cflag &= ~PARENB;
    	tty.c_cflag &= ~CSTOPB;
    	tty.c_cflag &= ~CRTSCTS;

    	tcsetattr(serial_fd, TCSANOW, &tty);

    	return true;
    }

    ///////

    void enviar_serial(const std::string& msg) {
    	if (serial_fd >= 0) {
            write(serial_fd, msg.c_str(), msg.size());
    	}
    }

};

int main(int argc, char **argv){
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Olhos_Node>());
    rclcpp::shutdown();
    return 0;
}
