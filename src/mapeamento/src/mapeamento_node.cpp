#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include <vector>
#include <cmath>
#include <string>

class Mapeamento_Node : public rclcpp::Node {
public:
    Mapeamento_Node() : Node("mapeamento_node") {
        mapa = std::vector<std::vector<Celula>>(3, std::vector<Celula>(3));

        for(int i = 0; i < 3; i++){
            for(int j = 0; j < 3; j++){
                mapa[i][j].estado = -1;
                mapa[i][j].lat = 0.0;
                mapa[i][j].lon = 0.0;
            }
        }

        mapa[1][1].estado = 0; // centro inicial do mapa

        pub_cer_ = this->create_publisher<std_msgs::msg::String>("mapeamento/cerebro", 10);

        sub_cer_ = this->create_subscription<std_msgs::msg::String>(
            "cerebro/mapeamento", 10,
            std::bind(&Mapeamento_Node::callback_cerebro, this, std::placeholders::_1)
        );

        sub_loc_ = this->create_subscription<std_msgs::msg::String>(
            "localizacao/mapeamento", 10,
            std::bind(&Mapeamento_Node::callback_localizacao, this, std::placeholders::_1)
        );
    }

private:
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_cer_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_loc_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_cer_;

    struct Celula {
        int estado;
        double lat;
        double lon;
    };

    std::vector<std::vector<Celula>> mapa;

    bool calcular_inicial = true;

    double tamanho_celula = 0.50; // metros
    int centro_y = 1, centro_x = 1; // recebem 1 por ser o centro inicial
    int pos_atual_x = 1, pos_atual_y = 1;

    ///////

    void callback_localizacao(const std_msgs::msg::String::SharedPtr msg){
        std::string s = msg->data;

        size_t pos1 = s.find('!');
        size_t pos2 = s.find('?');
        if(pos1 == std::string::npos || pos2 == std::string::npos) return;

        double lat = std::stod(s.substr(0, pos1));
        double lon = std::stod(s.substr(pos1 + 1, pos2 - pos1 - 1));
        int estado = std::stoi(s.substr(pos2 + 1));

        if(calcular_inicial){
            mapa[centro_y][centro_x].lat = lat;
            mapa[centro_y][centro_x].lon = lon;

            calcular_inicial = false;
            RCLCPP_INFO(this->get_logger(), "Origem definida.");
            return;
        }

        ///////

        double deg2rad = M_PI / 180.0;
        double dLat = (lat - mapa[centro_y][centro_x].lat) * deg2rad;
        double dLon = (lon - mapa[centro_y][centro_x].lon) * deg2rad;

        double r = 6378137.0;
        double y = dLat * r;
        double x = dLon * r * cos(mapa[centro_y][centro_x].lat * deg2rad);

        ///////

        int cel_x = static_cast<int>(std::round(x / tamanho_celula));
        int cel_y = static_cast<int>(std::round(y / tamanho_celula));

        int mx = centro_x + cel_x;
        int my = centro_y + cel_y;

        ////////

        while(my < 0){
            this->expandir(0);
            my++;
        }

        while(my >= (int)mapa.size()){
            this->expandir(1);
        }

        while(mx < 0){
            this->expandir(2);
            mx++;
        }

        while(mx >= (int)mapa[0].size()){
            this->expandir(3);
        }

        ///////

        if (mapa[my][mx].estado != -1){
            mapa[my][mx].estado = estado;

            pos_atual_y = my;
	    pos_atual_x = mx;
	}

        RCLCPP_INFO(this->get_logger(), "Mapa: robô em [%d, %d]", pos_atual_y, pos_atual_x);
    }

    ///////

    void callback_cerebro(const std_msgs::msg::String::SharedPtr msg){ //?0_n para andar em "tal" direção (n == norte) |||  !0000000 retornar a dsitancia da posição
        std::string s = msg->data;

        size_t pos_andar = s.find("?");
        size_t pos_dist = s.find("!");

        if(pos_andar != std::string::npos){
            size_t pos_direcao = s.find("_");

            double distancia = std::stod(
                s.substr(1, pos_direcao - 1));

            int dis_celula = std::round(distancia / tamanho_celula);

            char direcao = s[pos_direcao + 1];

        }else if(pos_dist != std::string::npos){
            RCLCPP_INFO(this->get_logger(), "Comando de distancia recebido");
        }
    }

    ///////

    void expandir(int direcao){

        int colunas = mapa[0].size();

        switch(direcao){

            case 0: { // Topo
                std::vector<Celula> nova_linha(colunas);
                for(auto &c : nova_linha){
                    c.estado = -1;
                    c.lat = 0.0;
                    c.lon = 0.0;
                }
                mapa.insert(mapa.begin(), nova_linha);
                centro_y++;
            	break;
            }

            case 1: { // Baixo
            	std::vector<Celula> nova_linha(colunas);
            	for(auto &c : nova_linha){
            	    c.estado = -1;
            	    c.lat = 0.0;
             	    c.lon = 0.0;
                }
                mapa.push_back(nova_linha);
                break;
            }

            case 2: { // Esquerda
                for(auto& linha : mapa){
                    Celula nova;
                    nova.estado = -1;
                    nova.lat = 0.0;
                    nova.lon = 0.0;
                    linha.insert(linha.begin(), nova);
                }
                centro_x++;
                break;
            }

            case 3: { // Direita
                for(auto& linha : mapa){
                    Celula nova;
                    nova.estado = -1;
                    nova.lat = 0.0;
                    nova.lon = 0.0;
                    linha.push_back(nova);
                }
                break;
            }
        }
    }
};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Mapeamento_Node>());
    rclcpp::shutdown();
    return 0;
}

