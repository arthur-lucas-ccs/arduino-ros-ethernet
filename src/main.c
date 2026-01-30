/*
 * Projeto: Robô Híbrido ROS/Web
 * Hardware: Arduino Uno + Ethernet Shield W5100 + Driver Ponte H
 * Descrição: Nó ROS para controle de tração e leitura de sensores,
 * com interface Web Basica.
 */

#include <SPI.h>
#include <Ethernet.h>
#include <ros.h>
#include <Servo.h>
#include <std_msgs/String.h>
#include <std_msgs/Float32.h>

// --- Configuração de Rede ---
byte mac[] = {0x52, 0x7D, 0x60, 0x86, 0x2B, 0xB8};
IPAddress ip(192, 168, 1, 110);
EthernetServer servidor(80);

// --- Mapeamento de Hardware ---

// Servo (Timer1)
Servo servoDirecao;
const int pinoServo = 9;

// Motores DC (Ponte H)
const int enableEsq = 3; // PWM
const int enableDir = 5; // PWM (Pino 4 reservado para SD Card CS)

const int motorEsqFrente  = 6;
const int motorEsqReverso = 7;
const int motorDirFrente  = 8;
const int motorDirReverso = 2;

// Sensor Ultrassônico (HC-SR04)
const int trigPin = A0;
const int echoPin = A1;

// --- Instanciamento ROS ---
ros::NodeHandle nh;

std_msgs::String msgStatus;
ros::Publisher pubStatus("status_robo", &msgStatus);

std_msgs::Float32 msgDistancia;
ros::Publisher pubDistancia("distancia_ultrassonico", &msgDistancia);


// --- Funcoes ---

void publicarStatus(const char* texto) {
  msgStatus.data = texto;
  pubStatus.publish(&msgStatus);
}

float lerDistancia() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duracao = pulseIn(echoPin, HIGH, 25000); // 25ms timeout
  
  if (duracao == 0) return 0.0;
  return duracao * 0.034 / 2;
}

void pararMotores() {
  analogWrite(enableEsq, 0);
  analogWrite(enableDir, 0);
  digitalWrite(motorEsqFrente, LOW); digitalWrite(motorEsqReverso, LOW);
  digitalWrite(motorDirFrente, LOW); digitalWrite(motorDirReverso, LOW);
}

void moverFrente() {
  analogWrite(enableEsq, 120); 
  analogWrite(enableDir, 120);
  digitalWrite(motorEsqFrente, HIGH); digitalWrite(motorEsqReverso, LOW);
  digitalWrite(motorDirFrente, HIGH); digitalWrite(motorDirReverso, LOW);
}

void moverTras() {
  analogWrite(enableEsq, 120);
  analogWrite(enableDir, 120);
  digitalWrite(motorEsqFrente, LOW); digitalWrite(motorEsqReverso, HIGH);
  digitalWrite(motorDirFrente, LOW); digitalWrite(motorDirReverso, HIGH);
}


//Controle da movimentacao

void executarComando(String cmd) {
  if (cmd == "frente") {
    servoDirecao.write(90);
    moverFrente();
    publicarStatus("Frente");
  } else if (cmd == "tras") {
    servoDirecao.write(90);
    moverTras();
    publicarStatus("Tras");
  } else if (cmd == "direita") {
    servoDirecao.write(135); // Limite mecânico definido em 135 graus
    moverFrente();
    publicarStatus("Direita");
  } else if (cmd == "esquerda") {
    servoDirecao.write(45);  // Limite mecânico definido em 45 graus
    moverFrente();
    publicarStatus("Esquerda");
  } else if (cmd == "parar") {
    pararMotores();
    servoDirecao.write(90);
    publicarStatus("Parado");
  }
}

// Callback de subscrição ROS
void comandoCallback(const std_msgs::String& msg) {
  executarComando(msg.data);
}

ros::Subscriber<std_msgs::String> subCmd("comando_robo", &comandoCallback);

void setup() {
  // Inicialização de GPIOs
  servoDirecao.attach(pinoServo);
  servoDirecao.write(90);

  pinMode(motorEsqFrente, OUTPUT); pinMode(motorEsqReverso, OUTPUT);
  pinMode(motorDirFrente, OUTPUT); pinMode(motorDirReverso, OUTPUT);
  pinMode(enableEsq, OUTPUT);      pinMode(enableDir, OUTPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  pararMotores();

  // Inicialização de Stack de Rede
  Ethernet.begin(mac, ip);
  servidor.begin();

  // Inicialização ROS Serial
  nh.initNode();
  nh.advertise(pubStatus);
  nh.advertise(pubDistancia);
  nh.subscribe(subCmd);
}

void loop() {
  // Manutenção da conexão rosserial
  nh.spinOnce();

  // Telemetria 
  static unsigned long lastTime = 0;
  if (millis() - lastTime > 100) {
      float distancia = lerDistancia();
      msgDistancia.data = distancia;
      pubDistancia.publish(&msgDistancia);
      lastTime = millis();
  }

  // Servidor Web
  EthernetClient cliente = servidor.available();
  if (cliente) {
    String requisicao = "";
    int charCount = 0; 
    
    while (cliente.connected()) {
      if (cliente.available()) {
        char c = cliente.read();
        
        if (charCount < 50) { 
            requisicao += c;
            charCount++;
        }
        
        if (c == '\n') { 
            
            // Roteamento de Comandos
            if (requisicao.indexOf("GET /frente") >= 0) executarComando("frente");
            else if (requisicao.indexOf("GET /tras") >= 0) executarComando("tras");
            else if (requisicao.indexOf("GET /direita") >= 0) executarComando("direita");
            else if (requisicao.indexOf("GET /esquerda") >= 0) executarComando("esquerda");
            else if (requisicao.indexOf("GET /parar") >= 0) executarComando("parar");

            // Roteamento de API (JSON/Text) vs View (HTML)
            if (requisicao.indexOf("GET /distancia") >= 0) {
               // Endpoint API para polling via AJAX
               cliente.println(F("HTTP/1.1 200 OK"));
               cliente.println(F("Content-Type: text/plain"));
               cliente.println(F("Connection: close"));
               cliente.println();
               cliente.print(lerDistancia());
            } 
            else {
               // Renderização da Interface Web)
               cliente.println(F("HTTP/1.1 200 OK"));
               cliente.println(F("Content-Type: text/html"));
               cliente.println(F("Connection: close"));
               cliente.println();
               
               cliente.println(F("<!DOCTYPE HTML><html><head><title>Robo Dashboard</title>"));
               cliente.println(F("<meta name='viewport' content='width=device-width, initial-scale=1'>"));
               cliente.println(F("<style>button{width:80px;height:50px;margin:5px;font-size:18px}</style>"));
               
               cliente.println(F("<script>"));
               cliente.println(F("function att(){fetch('/distancia').then(r=>r.text()).then(t=>{document.getElementById('d').innerText=t+' cm'})}"));
               cliente.println(F("setInterval(att,1000);"));
               cliente.println(F("function cmd(c){fetch('/'+c);}"));
               cliente.println(F("</script></head><body style='text-align:center'>"));
               
               cliente.println(F("<h1>Controle Web</h1>"));
               cliente.println(F("<button onclick=\"cmd('frente')\">Frente</button><br>"));
               cliente.println(F("<button onclick=\"cmd('esquerda')\">Esq</button>"));
               cliente.println(F("<button onclick=\"cmd('direita')\">Dir</button><br>"));
               cliente.println(F("<button onclick=\"cmd('tras')\">Tras</button><br><br>"));
               cliente.println(F("<button onclick=\"cmd('parar')\" style='background:red;color:white'>STOP</button>"));
               cliente.println(F("<h3>Sensor: <span id='d'>--</span></h3>"));
               cliente.println(F("</body></html>"));
            }
            break; // Encerra processamento após resposta
        }
      }
    }
    delay(1); // Latência para conclusão do envio TCP
    cliente.stop();
  }
}