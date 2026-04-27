#include "rclcpp/rclcpp.hpp"
#include <string>
#include <sstream>
#include <std_msgs/msg/string.hpp>

#include <regex>

class Cerebro_Node : public rclcpp::Node {
public:
    Cerebro_Node() : Node("cerebro_node") {

        // publishers
        pub_llm_ = this->create_publisher<std_msgs::msg::String>("cerebro/llm", 10);
        pub_des_ = this->create_publisher<std_msgs::msg::String>("cerebro/localizacao", 10);
        pub_boc_ = this->create_publisher<std_msgs::msg::String>("cerebro/boca", 10);
        pub_olh_ = this->create_publisher<std_msgs::msg::String>("cerebro/olhos", 10);
	pub_map_ = this->create_publisher<std_msgs::msg::String>("cerebro/mapeamento", 10);

        // subscriptions
        sub_llm_ = this->create_subscription<std_msgs::msg::String>(
            "llm/cerebro", 10,
            std::bind(&Cerebro_Node::callback_llm, this, std::placeholders::_1)
        );

	sub_map_ = this->create_subscription<std_msgs::msg::String>(
	    "mapeamento/cerebro", 10,
            std::bind(&Cerebro_Node::callback_mapeamento, this, std::placeholders::_1)
	);

        /*
        sub_olh_ = this->create_subscription<sensor_msgs::msg::Image>(
            "olhos/cerebro", 10,
            std::bind(&Cerebro_Node::callback_olhos, this, std::placeholders::_1)
        );*/

        RCLCPP_INFO(this->get_logger(), "Nó brain inicializado com sucesso.");
    }

private:

    // publishers
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_llm_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_des_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_boc_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_olh_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_map_;
    // subscriptions
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_llm_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_map_;

    // rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_olh_;

    //--------
    void callback_llm(const std_msgs::msg::String::SharedPtr msg){
    	std::string texto = msg->data;

    	std_msgs::msg::String msg_saida;
    	std_msgs::msg::String msg_llm;

    	// =====

    	if (std::regex_search(texto, std::regex("(ande|vá|ir)")))
    	{
            msg_llm.data =
            	"Identifique o local ou instrução na frase: '" + texto +
            	"'. Formato: 'MOV-DESTINO:_____' ou 'MOV-DESTINO:NAO'.";

            pub_llm_->publish(msg_llm);
    	}

    	// =====

    	else if (std::regex_search(texto, std::regex("(procure|encontre|ache)")))
    	{
            msg_llm.data =
            	"Identifique um nome na frase: '" + texto +
            	"'. Formato: 'BUSCA-NOME:_____' ou 'BUSCA-NOME:NAO'.";

            pub_llm_->publish(msg_llm);
    	}

        // =====

    	else if (std::regex_search(texto, std::regex("(cadastre|cadastrar|meu nome é)")))
    	{
            msg_llm.data =
            	"Identifique um nome na frase: '" + texto +
            	"'. Formato: 'CADASTRO-NOME:_____' ou 'CADASTRO-NOME:NAO'.";

            pub_llm_->publish(msg_llm);
    	}

        // =====

    	else if (texto.find("MOV-DESTINO:") != std::string::npos)
    	{
            msg_saida.data = texto.substr(13);
            pub_olh_->publish(msg_saida);
    	}
        else if (texto.find("CADASTRO-NOME:") != std::string::npos)
    	{
            msg_saida.data = texto.substr(15);
            pub_olh_->publish(msg_saida);
    	}
    	else if (texto.find("BUSCA-NOME:") != std::string::npos)
        {
            msg_saida.data = texto.substr(12);
            pub_boc_->publish(msg_saida);
    	}

        // =====

    	else
    	{
            msg_saida.data = texto;
            pub_boc_->publish(msg_saida);
        }
    }

    void callback_mapeamento(const std_msgs::msg::String::SharedPtr msg) {
        RCLCPP_INFO(this->get_logger(),"%s", msg->data.c_str());
    }



    //--------

    /*
    void callback_olhos(const sensor_msgs::msg::Image::SharedPtr msg) {
        cv::Mat frame;

        try {
            frame = cv_bridge::toCvCopy(msg, "bgr8")->image;
        } catch (const cv_bridge::Exception &e) {
            RCLCPP_ERROR(this->get_logger(), "Erro cv_bridge: %s", e.what());
            return;
        }

        auto tempo = std::chrono::system_clock::now().time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tempo).count();

        std::string arquivo = pasta_ + "/img_" + std::to_string(ms) + ".png";

        cv::imwrite(arquivo, frame);

        RCLCPP_INFO(this->get_logger(), "Imagem salva em: %s", arquivo.c_str());
    }
    */

};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Cerebro_Node>());
    rclcpp::shutdown();
    return 0;
}
