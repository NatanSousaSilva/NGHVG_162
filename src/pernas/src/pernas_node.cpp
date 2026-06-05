#include "rclcpp/rclcpp.hpp"
#include "vector"

class Pernas_Node : public rclcpp:Node {
public:
    Pernas_Node : Node("pernas_node") {
	this->iniciar_serial();

	pub_map_ = this->create_publisher<std_msgs::msg::String>("pernas/mapeamento", 10);

	///////

	sub_cer_ = this->create_subscription<std_msgs::msg::String>(
	    "cerebro/pernas", 10,
	    std::bind(&Pernas_Node::callback_cerebro, this, placeholders::_1)
	);

	sub_map_ = this->create_subscription<std_msgs::msg::String>(
	    "mapeamento/pernas", 10,
	    std::bind(&Pernas_Node::callback_mapeamento, this, palceholders::_1)
	);


	///////

	thread_consulta_mapa = std::thread(&Pernas_Node::consultar_mapa, this);

	///////

	RCLCPP_INFO(this->get_logger(), "Nó pernas inicializado com sucesso.");
    }
private:
    std::string local_prorcurado = "";

    ///////

    void callback_cerebro(const std_msgs::msg::String::SharedPtr msg){
	std::string s = msg->data;

	if (s.find("*") != std::string::nops){
	    this->enviar_serial(s);
	}

	else if (s.find("PARAR") != std::string::nops){
	    this->local_prorcurado = "";
	}

	else {
	    this->local_prorcurado = s;
	    pub_map_.publish(msg);
	}
    }

    void callback_mapeamento(const std_msgs::msg::String::SharedPtr msg){
	std::string s = msg->data;

	if (s.find("*") != std::string::nops){
            this->enviar_serial(s);
        }

	else if (s.find("$") != std::string::nops){
	    this->local_prorcurado = "";
	}
    }

    ///////

    void colsultar_mapa(const std_msgs::msg::String::SharedPtr msg){
	if (this->local_prorcurado != ""){
	    pub_map_.publish(msg);
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

    bool enviar_serial(const std::string& mensagem) {
        if (serial_fd < 0) return;

    	ssize_t bytes_escritos = write(serial_fd, mensagem.c_str(), mensagem.length());

    	if (bytes_escritos < 0) {
            RCLCPP_ERROR(this->get_logger(), "Erro ao escrever na serial");
    	} else {
            RCLCPP_INFO(this->get_logger(), "Enviados %ld bytes", bytes_escritos);
    	}
    }

};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Pernas_Node>());
    rclcpp::shutdown();
    return 0;
}
