# Arduino ROS Node via Ethernet 🤖📡

Este repositório contém o firmware e a configuração para integrar um **Arduino com Ethernet Shield** ao ecossistema **ROS (Robot Operating System)**.

O projeto foi desenvolvido como parte de um Trabalho de Conclusão de Curso (TCC), visando permitir o controle de atuadores e leitura de sensores remotamente via rede TCP/IP, eliminando a necessidade de conexão USB direta com o computador mestre.

## 📋 Funcionalidades
- **Comunicação TCP/IP:** Conexão estável via cabo de rede usando o protocolo `rosserial`.
- **Leitura de Sensores:** Publica dados de distância (Ultrassônico HC-SR04) no tópico `/ultrasound`.
- **Controle de Atuadores:** Assina o tópico `/servo_control` para mover um servo motor.
- **Integração ROS:** Totalmente compatível com ROS Noetic/Melodic.

## 🛠️ Hardware Necessário
* Arduino Uno ou Mega
* Ethernet Shield (W5100 ou W5500)
* Sensor Ultrassônico HC-SR04
* Servo Motor (ex: SG90 ou MG995)
* Cabos Jumper e Cabo de Rede (RJ45)

## 🔌 Esquema de Ligação (Pinagem)

| Componente       | Pino Arduino | Observação                  |
|------------------|--------------|-----------------------------|
| **Servo** | Pino 9       | PWM                         |
| **HC-SR04 Trig** | Pino 7       | Disparo                     |
| **HC-SR04 Echo** | Pino 6       | Retorno                     |
| **Ethernet CS** | Pino 10      | Padrão do Shield (não usar) |
| **Ethernet SD** | Pino 4       | Se usar cartão SD           |

> **Atenção:** O Ethernet Shield utiliza os pinos SPI (11, 12, 13 no Uno/Mega) para comunicação. Não conecte sensores nestes pinos.

## ⚙️ Configuração de Rede

Para que o ROS encontre o Arduino, ambos devem estar na mesma faixa de IP.
Edite o arquivo `main.c` com os IPs da sua rede:

```cpp
// Exemplo de configuração no código
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 177);    // IP do Arduino
IPAddress server(192, 168, 1, 10); // IP do PC (roscore)
