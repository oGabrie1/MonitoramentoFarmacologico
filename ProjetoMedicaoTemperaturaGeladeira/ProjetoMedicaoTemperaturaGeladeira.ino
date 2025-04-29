#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>          
#include <WiFiUdp.h>  
#include <NTPClient.h>
#include <time.h>

// OS ITENS COMENTADOS DEVEM SER DESCOMENTADOS NO ESP32

// Definindo os pinos e tipos dos sensores
#define DHT11PIN 13        // Pino de dados 
#define DHT22PIN 15        // Pino de dados 
#define DHT11TYPE DHT11    // Tipo do sensor, DHT11
#define DHT22TYPE DHT22    // Tipo do sensor, DHT22

// Pinos dos LEDs
const int LEDvermelho = 27;
const int LEDverde = 14;
const int LEDazul = 12;
const int AlarmeSonoro = 26;

// Defina seu SSID e senha Wi-Fi
const char* ssid = "MARIA_2.4G";     // Substitua pelo seu SSID
const char* password = "85441200";    // Substitua pela sua senha

WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org");

unsigned long portaAbertaTempo = 0;
unsigned long portaFechadaTempo = 0;
bool portaAberta = false;
#define BOTAO_PRESSAO 18

// Instanciando os sensores DHT
DHT dht11(DHT11PIN, DHT11TYPE); // Cria objeto DHT
DHT dht22(DHT22PIN, DHT22TYPE); // Cria objeto DHT

// Instanciando o display LCD (endereço I2C 0x27, 16 colunas, 2 linhas)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);   // Inicia comunicação
  Serial.println("Iniciando as configurações...");
  dht11.begin();  // Inicializa o primeiro sensor DHT (DHT11)
  dht22.begin();  // Inicializa o segundo sensor DHT (DHT22)
  lcd.init();         // Inicia o LCD
  lcd.backlight();    // Liga a luz de fundo

  // Configurando os pinos dos leds como Saída
  pinMode(LEDvermelho, OUTPUT);
  pinMode(LEDverde, OUTPUT);
  pinMode(LEDazul, OUTPUT);

  // Apagando todos os leds ao iniciar o programa
  digitalWrite(LEDvermelho, LOW);
  digitalWrite(LEDverde, LOW);
  digitalWrite(LEDazul, LOW);

  // Configurando o pino do Alarme como Saída
  pinMode(AlarmeSonoro, OUTPUT);
  digitalWrite(AlarmeSonoro, LOW); 

  // Configura o pino do botão
  pinMode(BOTAO_PRESSAO, INPUT_PULLUP);  // Usando a resistência de pull-up interna

  // Conectar ao Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi!");
  Serial.println("Iniciando o programa...");
  
  // Inicializa o cliente NTP
  timeClient.begin();
  timeClient.setTimeOffset(-10800);  // -3 horas em segundos (UTC-3)

  
}

// Pegar temperatura do DHT22
float lerTemperaturaInterna() {
  float humidade = dht22.readHumidity();          // Lê umidade
  float temperatura = dht22.readTemperature();    // Lê temperatura

  // Verifica se houve erro na leitura
  if (isnan(temperatura) || isnan(humidade)) {
    Serial.println("Erro ao ler o DHT22!");
    return -999;
  }

  // Exibe os valores lidos
  Serial.print("Temperatura Interna: ");
  Serial.print(temperatura);
  Serial.print(" °C  |  Umidade: ");
  Serial.print(humidade);
  Serial.println(" %");

  return temperatura;
}

// Pegar temperatura do DHT11
float lerTemperaturaExterna() {
  float humidade = dht11.readHumidity();          // Lê umidade
  float temperatura = dht11.readTemperature();    // Lê temperatura

  // Verifica se houve erro na leitura
  if (isnan(temperatura) || isnan(humidade)) {
    Serial.println("Erro ao ler o DHT11!");
    return -999;
  }

  // Exibe os valores lidos
  Serial.print("Temperatura Externa: ");
  Serial.print(temperatura);
  Serial.print(" °C  |  Umidade: ");
  Serial.print(humidade);
  Serial.println(" %");

  return temperatura;
}

// Tocar o Alarme quando a temperatura estiver fora de padrão
void tocarAlarme() {
    tone(AlarmeSonoro, 1000);  
    delay(100);          
    noTone(AlarmeSonoro);      
    delay(100);          
    tone(AlarmeSonoro, 1000);
    delay(100);          
    noTone(AlarmeSonoro);      
    delay(300);  
}

// Alterar cor do LED de acordo com a temperatura
void atualizarLED(float temperaturaInterna) {
  if (temperaturaInterna == -999) {
    //Caso temperatura invalida
    return;
  }

  if (temperaturaInterna > 8.0) {
    digitalWrite(LEDvermelho, HIGH);
    digitalWrite(LEDverde, LOW);
    digitalWrite(LEDazul, LOW);
    tocarAlarme();
  } else if (temperaturaInterna >= 2.0 && temperaturaInterna <= 8.0) {
    digitalWrite(LEDvermelho, LOW);
    digitalWrite(LEDverde, HIGH);
    digitalWrite(LEDazul, LOW);
  } else {
    digitalWrite(LEDvermelho, LOW);
    digitalWrite(LEDverde, LOW);
    digitalWrite(LEDazul, HIGH);
    tocarAlarme();
  }
}

// Função para exibir as temperaturas no LCD
void exibirTemperaturas(float temperaturaInterna, float temperaturaExterna) {
  lcd.clear();
  
  // Exibe a temperatura interna na primeira linha
  lcd.setCursor(0, 0);
  if (temperaturaInterna == -999) {
    lcd.print("Erro no DHT22");
  } else {
    lcd.print("Temp Int: ");
    lcd.print(temperaturaInterna);
    lcd.print(" C");
  }

  // Exibe a temperatura externa na segunda linha
  lcd.setCursor(0, 1);
  if (temperaturaExterna == -999) {
    lcd.print("Erro no DHT11");
  } else {
    lcd.print("Temp Ext: ");
    lcd.print(temperaturaExterna);
    lcd.print(" C");
  }
}

void lerSensorPorta(){
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

void loop() {
  // Lê as temperaturas
  float temperaturaExterna = lerTemperaturaExterna(); // Retornar Temperatura Externa
  float temperaturaInterna = lerTemperaturaInterna(); // Retornar Temperatura Interna

  // Exibe as temperaturas no LCD
  exibirTemperaturas(temperaturaInterna, temperaturaExterna);

  // Executar o código 5 vezes usando o loop for
  for (int i = 0; i < 5; i++) {

    // Atualiza o LED e Alarme com base na temperatura interna
    atualizarLED(temperaturaInterna);

    // Lê o sensor da porta (5 vezes)
    lerSensorPorta();

    delay(1000); // Espera 1 segundo antes de ler novamente (isso vai acontecer 5 vezes)
  }
}
