#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 13        // Pino de dados 
#define DHTTYPE DHT11    // Tipo do sensor, DHT11

// Inicialização dos objetos
DHT dht(DHTPIN, DHTTYPE);                          // Cria objeto DHT
LiquidCrystal_I2C lcd(0x27, 16, 2);                // Endereço I2C comum: 0x27

void setup() {
  Serial.begin(115200);   // Inicia comunicação
  dht.begin();            // Inicia o sensor DHT22
  lcd.init();         // Inicia o LCD
  lcd.backlight();    // Liga a luz de fundo
}


// Pegar temperatura DHT11
float lerTemperaturaDHT11() {
  float humidadeDHT11 = dht.readHumidity();          // Lê umidade
  float temperaturaDHT11 = dht.readTemperature();    // Lê temperatura

  // Verifica se houve erro na leitura
  if (isnan(temperaturaDHT11) || isnan(humidadeDHT11)) {
    Serial.println("Erro ao ler o DHT11!");
    return -999;
  }

  // Exibe os valores lidos
  Serial.print("Temperatura: ");
  Serial.print(temperaturaDHT11);
  Serial.print(" °C  |  Umidade: ");
  Serial.print(humidadeDHT11);
  Serial.println(" %");

  return temperaturaDHT11;
}

void loop() {
  float temperatura = lerTemperaturaDHT11(); // Simula temperatura
  
  lcd.clear();
  lcd.setCursor(0, 0);
  if (temperatura == -999) {
    lcd.print("Erro sensor DHT");
  } else {
    lcd.print("Temp: ");
    lcd.print(temperatura);
    lcd.print(" C");
  }

  delay(1000);
}

