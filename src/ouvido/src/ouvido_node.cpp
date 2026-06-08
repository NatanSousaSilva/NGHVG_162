#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include <portaudio.h>

class OuvidoNode : public rclcpp::Node {
public:
    OuvidoNode() : Node("ouvido_node"), stream_(nullptr) {
        // Inicializa o driver de áudio (PortAudio)
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            RCLCPP_ERROR(this->get_logger(), "Erro ao inicializar PortAudio: %s", Pa_GetErrorText(err));
            return;
        }

        // Configuração do microfone
        PaStreamParameters input_parameters;
        input_parameters.device = Pa_GetDefaultInputDevice();
        if (input_parameters.device == paNoDevice) {
            RCLCPP_ERROR(this->get_logger(), "Nenhum dispositivo de entrada de áudio encontrado.");
            return;
        }
        input_parameters.channelCount = 1; // channels=1
        input_parameters.sampleFormat = paInt16; // dtype='int16'
        input_parameters.suggestedLatency = Pa_GetDeviceInfo(input_parameters.device)->defaultLowInputLatency;
        input_parameters.hostApiSpecificStreamInfo = nullptr;

        // Abre o fluxo passando a função de callback estática
        err = Pa_OpenStream(
            &stream_,
            &inputParameters,
            nullptr, // Sem parâmetros de saída
            16000, // samplerate=16000
            8000, // blocksize=8000
            paNoFlag,
            &Ouvido_Node::auxiliar,
            this // Passa a instância atual ('this') como userData
        );

        if (err != paNoError) {
            RCLCPP_ERROR(this->get_logger(), "Erro ao abrir stream de áudio: %s", Pa_GetErrorText(err));
            return;
        }

        err = Pa_StartStream(stream_);
        if (err != paNoError) {
            RCLCPP_ERROR(this->get_logger(), "Erro ao iniciar stream de áudio: %s", Pa_GetErrorText(err));
        }

	// publishers
	pub_llm_ = this->create_publisher<std_msgs::msg::String>("ouvido/llm", 10);


	RCLCPP_INFO(this->get_logger(), "Ouvido inicializado");
    }

    ~Ouvido_Node() {
        if (stream_) {
            Pa_StopStream(stream_);
            Pa_CloseStream(stream_);
        }
        Pa_Terminate();
    }

private:
    PaStream *stream_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_llm_;

    void captar_audio(const void *indata, unsigned long frames, PaStreamCallbackFlags status) {
        if (status) {
            RCLCPP_WARN(this->get_logger(), "Status de áudio com aviso/erro: %lu", status);
        }

        if (!indata) return;

	// 1. Converte e calcula o tamanho exato dos bytes recebidos
    const char* audio_bytes = static_cast<const char*>(indata);
    size_t bytes_count = frames * sizeof(int16_t); // Cada frame int16_t tem 2 bytes

    // 2. Inicializa o cURL
    CURL *curl = curl_easy_init();
    if (!curl) {
        RCLCPP_ERROR(this->get_logger(), "Falha ao inicializar CURL");
        return;
    }

    // String para armazenar a resposta JSON do FastAPI
    std::string response_string; 

    // Altere para o IP/Porta corretos da sua API FastAPI do Whisper
    std::string url = "http://localhost:8000/transcrever_bruto"; 

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/octet-stream");

    // 3. Configurações do cURL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    
    // ATENÇÃO: Passa o ponteiro de bytes E o tamanho exato deles
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, audio_bytes);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(bytes_count));

    // Callback para salvar o retorno do servidor
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](void *contents, size_t size, size_t nmemb, void *userp) -> size_t {
            std::string *output = static_cast<std::string*>(userp);
            output->append(static_cast<char*>(contents), size * nmemb);
            return size * nmemb;
        }
    );
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    // 4. Executa a requisição HTTP
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        RCLCPP_ERROR(this->get_logger(), "Falha na requisição cURL: %s", curl_easy_strerror(res));
    }

    // 5. Limpeza de memória obrigatória
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    // 6. Processa o resultado se o servidor respondeu com sucesso
    if (res == CURLE_OK && !response_string.empty()) {
        // O response_string conterá algo como: {"texto": "o que você falou"}
        // Você pode tratar essa string ou publicá-la direto
        RCLCPP_INFO(this->get_logger(), "Resposta da API: %s", response_string.c_str());
        
        auto msg = std_msgs::msg::String();
            msg.data = response_string; 
            pub_llm_->publish(msg);
    	}
    }

    static int auxiliar(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
                        void *userData){
        auto* node = static_cast<Ouvido_Node*>(userData);
        node->captar_audio(inputBuffer, framesPerBuffer, statusFlags);
        return paContinue;
    }
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<OuvidoNode>());
    rclcpp::shutdown();
    return 0;
}
