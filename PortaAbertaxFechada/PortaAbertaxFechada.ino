#include <WiFi.h>          // Para o ESP32
#include <WiFiUdp.h>  
#include <NTPClient.h>
#include <time.h>

// Defina seu SSID e senha Wi-Fi
const char* ssid = "MARIA_2.4G";     // Substitua pelo seu SSID
const char* password = "85441200";    // Substitua pela sua senha

WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org");

unsigned long portaAbertaTempo = 0;
unsigned long portaFechadaTempo = 0;
bool portaAberta = false;
#define BOTAO_PRESSAO 18

void setup() {
  Serial.begin(115200);

  // Configura o pino do botão
  pinMode(BOTAO_PRESSAO, INPUT_PULLUP);  // Usando a resistência de pull-up interna

  // Conectar ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi!");

  // Inicializa o cliente NTP
  timeClient.begin();
  timeClient.setTimeOffset(-10800);  // -3 horas em segundos (UTC-3)
}

void loop() {
  // Atualiza a hora do NTP
  timeClient.update();

  // Converte UNIX timestamp para struct tm
  time_t rawTime = timeClient.getEpochTime();
  struct tm* timeInfo = localtime(&rawTime);

  // Formata data e hora
  char dataHora[30];
  strftime(dataHora, sizeof(dataHora), "%d/%m/%Y %H:%M:%S", timeInfo);

  // Usando millis() para medir o tempo
  unsigned long tempoAtual = millis();  // Tempo desde o início

  // Lê o estado do botão
  if (digitalRead(BOTAO_PRESSAO) == LOW) {  // Botão pressionado, pino vai para LOW
    if (!portaAberta) {
      portaFechadaTempo = tempoAtual;  // Captura o tempo de fechamento
      Serial.print("Porta Fechada: ");
      Serial.println(dataHora);
      Serial.print("Tempo Execução: ");
      Serial.println(portaFechadaTempo);  // Exibe o tempo de abertura
      portaAberta = true;
    }
  } else {  // Botão não pressionado, pino em HIGH
    if (portaAberta) {
      portaAbertaTempo = tempoAtual;
      Serial.print("Porta Aberta: ");
      Serial.println(dataHora);
      Serial.print("Tempo Execução: ");
      Serial.println(portaAbertaTempo);  // Exibe o tempo de abertura

      portaAberta = false;
    }
  }

  delay(1000);  // Espera 1 segundo
}