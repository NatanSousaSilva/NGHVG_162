#include "rclcpp/rclcpp.hpp"
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <string>
#include <std_msgs/msg/string.hpp>
#include <iostream>

using json = nlohmann::json;

class Llm_Node : public rclcpp::Node {
public:
    Llm_Node() : Node("llm_ia_node") {

        pub_cer_ = this->create_publisher<std_msgs::msg::String>("llm/cerebro", 10);
	//pub_ouv_ = this->create_publisher<std_msgs::msg::String>("llm/ouvido", 10);

        sub_cer_ = this->create_subscription<std_msgs::msg::String>(
            "cerebro/llm", 10,
            std::bind(&Llm_Node::callback_cerebro, this, std::placeholders::_1)
        );
	sub_ouv_ = this->create_subscription<std_msgs::msg::String>(
	    "ouvido/llm", 10,
	    std::bind(&Llm_Node::callback_ouvido, this, std::placeholders::_1)
	);

        RCLCPP_INFO(this->get_logger(), "Nó llm inicializado com sucesso.");
    }

private:
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_cer_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_ouv_;

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_cer_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_ouv_;

    void callback_cerebro(const std_msgs::msg::String::SharedPtr msg){
    	std::string texto = msg->data;

    	std::string resposta = this->comunicar_llm(texto);

    	std_msgs::msg::String msg_cer;
    	msg_cer.data = resposta;

    	pub_cer_->publish(msg_cer);
    }

    void callback_ouvido(const std_msgs::msg::String::SharedPtr msg){
        std::string texto_recebido = msg->data;

        std::string mensagem_llm =
                "Identifique se a frase tem sentido lógico: '" +
                texto_recebido +
                "'. Escreva nesse formato: 's7-?_____' sendo '_____' a frase alterada para ter sentido caso necessário.";

        std::string resposta = this->comunicar_llm(mensagem_llm);

	RCLCPP_INFO(this->get_logger(), "Recebido ouvido: %s", texto_recebido.c_str());
	RCLCPP_INFO(this->get_logger(), "Resposta LLM: %s", resposta.c_str());

    	std_msgs::msg::String msg_saida;
        msg_saida.data = resposta;

        pub_cer_->publish(msg_saida);

    }

    std::string comunicar_llm(const std::string& prompt) {
        CURL *curl = curl_easy_init();
        if (!curl) {
            RCLCPP_ERROR(this->get_logger(), "Falha ao inicializar CURL");
            return "";
        }

        std::string url = "http://localhost:11434/api/generate";

        json body_json = {
            {"model", "robo1:latest"},
            {"prompt", prompt},
            {"stream", false}
        };

        std::string request_body = body_json.dump();
        std::string response;

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
            +[](void *contents, size_t size, size_t nmemb, void *userp) -> size_t {
                std::string *output = static_cast<std::string*>(userp);
                output->append(static_cast<char*>(contents), size * nmemb);
                return size * nmemb;
            }
        );
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            RCLCPP_ERROR(this->get_logger(), "Erro CURL: %s", curl_easy_strerror(res));
            return "";
        }

        try {
            auto json_response = json::parse(response);

            if (!json_response.contains("response")) {
                RCLCPP_WARN(this->get_logger(),
                    "Resposta JSON sem campo 'response'. Bruto: %s",
                    response.c_str());
                return "";
            }

            std::string text = json_response["response"].get<std::string>();

	    RCLCPP_INFO(this->get_logger(), "Publicado: '%s'", text.c_str());
            return text;

        } catch (const std::exception &e) {
            RCLCPP_ERROR(this->get_logger(), "Erro ao parsear JSON: %s", e.what());
            return "";
	}

    }
};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Llm_Node>());
    rclcpp::shutdown();
    return 0;
}
