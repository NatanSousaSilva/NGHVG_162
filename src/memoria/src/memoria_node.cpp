#include "rclcpp/rclcpp.hpp"
#include <sensor_msgs/msg/image.hpp>
#include <std_msgs/msg/string.hpp>
#include <cv_bridge/cv_bridge.hpp>
#include <opencv2/opencv.hpp>

#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>


namespace fs = std::filesystem;

class Memoria_Node : public rclcpp::Node {
public:
    Memoria_Node() : Node("memoria_node") {

        fs::create_directories(fs::path(pasta) / "rostos");

        sub1_olh_ = this->create_subscription<sensor_msgs::msg::Image>(
            "olhos/o1/memoria",
            10,
            std::bind(&Memoria_Node::callback_olhos1, this, std::placeholders::_1)
        );

        sub2_olh_ = this->create_subscription<std_msgs::msg::String>(
            "olhos/o2/memoria",
            10,
            std::bind(&Memoria_Node::callback_olhos2, this, std::placeholders::_1)
        );
    }

private:
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub1_olh_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub2_olh_;

    std::string pasta = "memoria";
    fs::path arquivo_json = "memoria/nomes.json";

    std::string nome_guardar;
    std::string caminho_guardar;

    void callback_olhos1(const sensor_msgs::msg::Image::SharedPtr msg) {
        try {
            auto cv_ptr = cv_bridge::toCvCopy(msg, "bgr8");

            auto tempo = this->get_clock()->now();
            std::string nome_arquivo =
                "image_" + std::to_string(tempo.seconds()) + ".png";

            fs::path caminho = fs::path(pasta) / "rostos" / nome_arquivo;

            cv::imwrite(caminho.string(), cv_ptr->image);

            RCLCPP_INFO(this->get_logger(),
                        "Imagem salva: %s",
                        caminho.string().c_str());

            caminho_guardar = caminho.string();
            this->guardar_rostos();
        }
        catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(),
                         "Erro cv_bridge: %s",
                         e.what());
        }
    }

    void callback_olhos2(const std_msgs::msg::String::SharedPtr msg) {
        nome_guardar = msg->data;
        this->guardar_rostos();
    }

    void guardar_rostos() {
    	if (!nome_guardar.empty() && !caminho_guardar.empty()) {

            std::ifstream infile(arquivo_json);
            bool existe = infile.good();
            infile.close();

            std::ofstream file;

            if (!existe) {
            	file.open(arquivo_json);
            	file << "[\n";
            } else {
            	file.open(arquivo_json, std::ios::in | std::ios::out);
            	file.seekp(-2, std::ios_base::end); // remove "\n]"
            	file << ",\n";
            }

            file << "  {\n";
            file << "    \"nome\": \"" << nome_guardar << "\",\n";
            file << "    \"imagem\": \"" << caminho_guardar << "\"\n";
            file << "  }\n";
            file << "]";

            file.close();

            RCLCPP_INFO(this->get_logger(),
                    "Rosto associado: %s -> %s",
                    nome_guardar.c_str(),
                    caminho_guardar.c_str());

            nome_guardar.clear();
            caminho_guardar.clear();
        }
    }
};

int main(int argc, char *argv[]){
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Memoria_Node>());
    rclcpp::shutdown();
    return 0;
}
