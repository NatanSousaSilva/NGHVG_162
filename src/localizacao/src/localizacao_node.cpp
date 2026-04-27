#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>


class Localizacao_Node : public rclcpp::Node {
public:
    Localizacao_Node() : Node("localizacao_node") {
        this->iniciar_serial();

        pub_map_ = this->create_publisher<std_msgs::msg::String>("localizacao/map", 10);

        sub_cer_ = this->create_subscription<std_msgs::msg::String>(
            "cerebro/localizacao", 10,
            std::bind(&Localizacao_Node::callback_cerebro, this, std::placeholders::_1)
        );

    }

private:
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_map_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_cer_;

    int serial_fd;

    int comando = -1;

    ////////

    void callback_cerebro(const std_msgs::msg::String::SharedPtr msg) {
        RCLCPP_INFO(this->get_logger(),"%s", msg->data.c_str());
    }

    ///////

    void ler_serial(){
        char buffer[256];

        while (true){
            int n = read(serial_fd, buffer, sizeof(buffer) - 1);

            if (n > 0){
                buffer[n] = '\0';
                std::cout << buffer;
            }
        }

    }
    ////////

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



};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Localizacao_Node>());
    rclcpp::shutdown();
    return 0;
}
