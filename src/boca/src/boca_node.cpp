#include "rclcpp/rclcpp.hpp"
#include <std_msgs/msg/string.hpp>
#include <curl/curl.h>
#include <fstream>
#include <string>
#include <filesystem>

class Boca_Node : public rclcpp::Node {
public:
    Boca_Node() : Node("boca_node") {
        pasta = std::string(std::getenv("HOME")) + "/audio";
        std::filesystem::create_directories(pasta);

	// subscriptions
        sub_cer_ = this->create_subscription<std_msgs::msg::String>(
            "cerebro/boca", 10,
            std::bind(&Boca_Node::callback_audio, this, std::placeholders::_1)
        );

        RCLCPP_INFO(this->get_logger(), "Nó Boca iniciado.");
    }

private:
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_cer_;

    std::string pasta;

    void callback_audio(const std_msgs::msg::String::SharedPtr msg) {
        std::string arquivo = pasta + "/audio_" +std::to_string(std::time(nullptr)) + ".wav";

        std::ofstream file(arquivo, std::ios::binary);
        if (!file.is_open()) {
            RCLCPP_ERROR(this->get_logger(), "Erro ao abrir arquivo para gravação.");
            return;
        }

        CURL *curl = curl_easy_init();
        if (!curl) {
            RCLCPP_ERROR(this->get_logger(), "Erro ao iniciar CURL.");
            return;
        }

        std::string texto = msg->data;

        char* escaped = curl_easy_escape(curl, texto.c_str(), texto.size());
        std::string url = "http://127.0.0.1:8000/boca?texto=" + std::string(escaped);
        curl_free(escaped);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void *ptr, size_t size, size_t nmemb, void *stream) -> size_t {
	    std::ofstream *out = static_cast<std::ofstream*>(stream);
	    out->write(reinterpret_cast<char*>(ptr), size * nmemb);
	    return size * nmemb;
	});

        CURLcode res = curl_easy_perform(curl);

        curl_easy_cleanup(curl);
        file.close();

        if (res != CURLE_OK) {
            RCLCPP_ERROR(this->get_logger(),
                "Erro no download do áudio: %s",
                curl_easy_strerror(res)
            );
            return;
        }

        system(("aplay " + arquivo).c_str());

        if (std::filesystem::exists(arquivo)) {
            std::filesystem::remove(arquivo);
        }
    }
};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Boca_Node>());
    rclcpp::shutdown();
    return 0;
}
