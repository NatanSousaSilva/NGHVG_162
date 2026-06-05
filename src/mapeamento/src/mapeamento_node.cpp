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

	sub_per_ = this->create_subscripion<std_msgs::msg::String>(
	    "pernas/mapeamento", 10,
	    std::bind(&Mapeamento_Node::callback_pernas, this, std::placeholders::_1)
	);

	RCLCPP_INFO(this->get_logger(), "Nó mapeamento inicializado com sucesso.");

    }

private:
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_cer_;

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_loc_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_cer_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_per_;

    ///////

    std::string local_prorcurado;
    std::string local_cadastrar = "";

    bool prorcurar_local = false;

    struct Celula {
	std::string local;
        int estado;
        double lat;
        double lon;
	double alt;
    };

    std::vector<std::vector<Celula>> mapa;

    bool calcular_inicial = true;

    double tamanho_celula = 0.50; // metros
    int centro_y = 1, centro_x = 1; // recebem 1 por ser o centro inicial
    int pos_atual_x = 1, pos_atual_y = 1;

    ///////

    void callback_localizacao(const std_msgs::msg::String::SharedPtr msg) { // Formato esperado: LAT ! LON ? ALT _ SAT * ANGULO
    	std::string s = msg->data;

    	try {
            //  Encontra as posições de todos os delimitadores na mensagem
            size_t p1 = s.find('!');
            size_t p2 = s.find('?');
            size_t p3 = s.find('_');
            size_t p4 = s.find('*');

            if (p1 == std::string::npos || p2 == std::string::npos ||
            	p3 == std::string::npos || p4 == std::string::npos) {
            	RCLCPP_WARN(this->get_logger(), "Pacote de dados incompleto ou malformado.");
            	return;
            }

            // Extrair e converte os dados
            double lat = std::stod(s.substr(0, p1));
            double lon = std::stod(s.substr(p1 + 1, p2 - p1 - 1));
            double alt = std::stod(s.substr(p2 + 1, p3 - p2 - 1));
            double angulo = std::stod(s.substr(p3 + 1));

            if (calcular_inicial) {
                if (lat != 0.0 && lon != 0.0) {
                    mapa[centro_y][centro_x].lat = lat;
                    mapa[centro_y][centro_x].lon = lon;
		    mapa[centro_y][centro_x].alt = alt

                    calcular_inicial = false;
                    RCLCPP_INFO(this->get_logger(), "Origem definida: Lat %f, Lon %f", lat, lon);
            	}
            	return;
            }

            	RCLCPP_DEBUG(this->get_logger(), "Alt: %.2fm, Sat: %d, Ang: %.2f", altitude, satelites, angulo);

       	    } catch (const std::exception &e) {
            	RCLCPP_ERROR(this->get_logger(), "Erro ao converter dados: %s", e.what());
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
		mapa[my][mx].lat = lat;
		mapa[my][mx].lon = lon;
		mapa[my][mx].alt = alt;

            	pos_atual_y = my;
	    	pos_atual_x = mx;
	    }

            RCLCPP_INFO(this->get_logger(), "Mapa: robô em [%d, %d]", pos_atual_y, pos_atual_x);
	}
    }

    ///////

    void callback_pernas(const std_msgs::msg::String::SharedPtr msg){
	std::string local = msg->data;

	for (int i = 0; i < (sizeof(mapa) / sizeof(mapa[0])); i++){
	    for (int j = 0; j < (sizeof(mapa[0]) / sizeof(mapa[0][0]); j++){
		if (mapa[i][j].local == local){
		    
		}
	    }
	}
    }

    void callback_cerebro(const std_msgs::msg::String::SharedPtr msg){
        std::string s = msg->data;

        if(s.find("?") != std::string::npos){
            local_prorcurado = s.substr(2);
	    prorcurar_lugar = true;

        }else if(s.find("!") != std::string::npos){
            local_cadastrar = s.substr(2);
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

