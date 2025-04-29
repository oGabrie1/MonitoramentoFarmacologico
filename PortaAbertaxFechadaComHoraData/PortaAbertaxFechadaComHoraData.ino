#include <DHT.h>

#define DHTPIN 15        // Pino de dados 
#define DHTTYPE DHT22    // Tipo do sensor, DHT22

// Pinos dos LEDs
const int LEDvermelho = 27;
const int LEDverde = 14;
const int LEDazul = 12;
const int AlarmeSonoro = 26;

DHT dht(DHTPIN, DHTTYPE); // Cria objeto DHT

void setup() {
  Serial.begin(115200);   // Inicia comunicação
  dht.begin();            // Inicia o sensor DHT22

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

  Serial.println("Iniciando o programa.");
}

// Pegar temperatura DHT22
float lerTemperaturaDHT22() {
  float humidadeDHT22 = dht.readHumidity();          // Lê umidade
  float temperaturaDHT22 = dht.readTemperature();    // Lê temperatura

  // Verifica se houve erro na leitura
  if (isnan(temperaturaDHT22) || isnan(humidadeDHT22)) {
    Serial.println("Erro ao ler o DHT22!");
    return -999;
  }

  // Exibe os valores lidos
  Serial.print("Temperatura: ");
  Serial.print(temperaturaDHT22);
  Serial.print(" °C  |  Umidade: ");
  Serial.print(humidadeDHT22);
  Serial.println(" %");

  return temperaturaDHT22;
}

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
void atualizarLED(float temperaturaDHT22) {
  if (temperaturaDHT22 == -999) {
    //Caso temperatura invalida
    return;
  }

  if (temperaturaDHT22 > 8.0) {
    digitalWrite(LEDvermelho, HIGH);
    digitalWrite(LEDverde, LOW);
    digitalWrite(LEDazul, LOW);
    tocarAlarme();
  } else if (temperaturaDHT22 >= 2.0 && temperaturaDHT22 <= 8.0) {
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


void loop() {
  float temperaturaDHT22 = lerTemperaturaDHT22();
  atualizarLED(temperaturaDHT22);
  delay(1000); 
}
