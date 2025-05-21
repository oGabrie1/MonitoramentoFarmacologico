#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>          
#include <WiFiUdp.h>  
#include <NTPClient.h>
#include <time.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "FS.h"
#include "SPIFFS.h"

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
WebServer servidor(80); // Cria servidor esp

unsigned long portaAbertaTempo = 0;
unsigned long portaFechadaTempo = 0;
bool portaAberta = false;
#define BOTAO_PRESSAO 18

// Funções de tempo do millis
unsigned long ultimaGravacao = 0;
const unsigned long intervaloGravacao = 1800000; // 30 minutos (1800000 ms)
unsigned long ultimaAtualizacao = 0;
const unsigned long intervaloAtualizacao = 3000; // 3 segundos
unsigned long ultimoMostrarIp = 0;
const unsigned long intervaloExibeIP = 12000; // Mostra IP a cada 12s
unsigned long ultimaLeitura = 0;
const unsigned long intervaloLeitura = 1000;  // 1 segundos

// para mostrar no html
float temperaturaInternaAtual = 0.0;
float temperaturaExternaAtual = 0.0;

// Instanciando os sensores DHT
DHT dht11(DHT11PIN, DHT11TYPE); // Cria objeto DHT
DHT dht22(DHT22PIN, DHT22TYPE); // Cria objeto DHT

// Instanciando o display LCD (endereço I2C 0x27, 16 colunas, 2 linhas)
LiquidCrystal_I2C lcd(0x27, 16, 2);


void criarArquivosIniciais() {
  // Verifica se o arquivo já existe
  if (!SPIFFS.exists("/index.html")) {
    File file = SPIFFS.open("/index.html", FILE_WRITE);
    if (file) {
      file.print(R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <title>Monitor de Temperatura</title>
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" rel="stylesheet">
  <style>
    body {
      background-color: #f8f9fa;
    }
    .status p {
      font-size: 1.2rem;
      margin-bottom: 0.5rem;
    }
    .download-links a {
      margin: 0 10px;
    }
  </style>
</head>
<body>
  <div class="container py-4">
    <h1 class="text-center mb-4">Monitor de Temperatura</h1>

    <div class="row justify-content-center mb-4">
    <div class="col-md-4 mb-3">
        <div class="card text-center shadow-sm border-primary">
        <div class="card-body">
            <h5 class="card-title text-primary">Temperatura Interna</h5>
            <p class="display-6">
            <span id="tempInterna">Carregando...</span> °C
            </p>
        </div>
        </div>
    </div>

    <div class="col-md-4 mb-3">
        <div class="card text-center shadow-sm border-success">
        <div class="card-body">
            <h5 class="card-title text-success">Temperatura Externa</h5>
            <p class="display-6">
            <span id="tempExterna">Carregando...</span> °C
            </p>
        </div>
        </div>
    </div>
    </div>

    <div class="download-links text-center mb-5">
      <a href="/temperaturas" class="btn btn-primary" download>
        📥 Baixar Temperaturas CSV
      </a>
      <a href="/porta" class="btn btn-primary" download>
        📥 Baixar Eventos da Porta CSV
      </a>
    </div>

    <div class="row">
  <!-- Histórico de Temperaturas -->
  <div class="col-md-6 mb-4">
    <h2 class="text-center mb-3">Histórico de Temperaturas</h2>
    <div class="table-responsive border rounded shadow-sm p-2" style="max-height: 500px; overflow-y: auto;">
      <table id="tabelaTemperaturas" class="table table-striped table-bordered table-hover mb-0">
        <thead class="table-light">
          <tr>
            <th>Data/Hora</th>
            <th>Temperatura Interna (°C)</th>
            <th>Temperatura Externa (°C)</th>
          </tr>
        </thead>
        <tbody>
          <!-- Linhas serão inseridas via JavaScript -->
        </tbody>
      </table>
    </div>
  </div>

  <!-- Eventos da Porta -->
  <div class="col-md-6 mb-4">
    <h2 class="text-center mb-3">Eventos da Porta</h2>
    <div class="table-responsive border rounded shadow-sm p-2" style="max-height: 500px; overflow-y: auto;">
      <table id="tabelaPorta" class="table table-striped table-bordered table-hover mb-0">
        <thead class="table-light">
          <tr>
            <th>Data/Hora</th>
            <th>Evento</th>
          </tr>
        </thead>
        <tbody>
          <!-- Linhas serão inseridas via JavaScript -->
        </tbody>
      </table>
    </div>
  </div>
</div>

  <script>
    function carregarTemperaturaAtual() {
      fetch('/temperatura_atual')
        .then(res => res.json())
        .then(data => {
          document.getElementById('tempInterna').textContent = data.interna.toFixed(2);
          document.getElementById('tempExterna').textContent = data.externa.toFixed(2);
        });
    }

    function carregarTemperaturas() {
      fetch('/temperaturas')
        .then(res => res.text())
        .then(csv => {
          const linhas = csv.trim().split("\n");
          const tabela = document.getElementById("tabelaTemperaturas").querySelector("tbody");
          tabela.innerHTML = "";
          linhas.forEach(linha => {
            const cols = linha.split(",");
            if (cols.length === 3) {
              const row = tabela.insertRow();
              row.insertCell(0).textContent = cols[0];
              row.insertCell(1).textContent = cols[1];
              row.insertCell(2).textContent = cols[2];
            }
          });
        });
    }

    function carregarEventosPorta() {
      fetch('/porta')
        .then(res => res.text())
        .then(csv => {
          const linhas = csv.trim().split("\n");
          const tabela = document.getElementById("tabelaPorta").querySelector("tbody");
          tabela.innerHTML = "";
          linhas.forEach(linha => {
            const cols = linha.split(",");
            if (cols.length === 2) {
              const row = tabela.insertRow();
              row.insertCell(0).textContent = cols[0];
              row.insertCell(1).textContent = cols[1];
            }
          });
        });
    }

    function atualizarTudo() {
      carregarTemperaturaAtual();
      carregarTemperaturas();
      carregarEventosPorta();
    }

    window.onload = function() {
      atualizarTudo();
      setInterval(atualizarTudo, 10000);
    }
  </script>
</body>
</html>
      )rawliteral");
      file.close();
      Serial.println("index.html criado com sucesso.");
    } else {
      Serial.println("Erro ao criar index.html.");
    }
  }
}

// --- FUNÇÕES AUXILIARES DE SPIFFS ---
int contarLinhasArquivo(const char* caminho) {
  File arquivo = SPIFFS.open(caminho);
  if (!arquivo) return 0;
  int linhas = 0;
  while (arquivo.available()) {
    if (arquivo.read() == '\n') linhas++;
  }
  arquivo.close();
  return linhas;
}

void deletarPrimeiraLinha(const char* caminho) {
  File original = SPIFFS.open(caminho, "r");
  if (!original) return;

  String restante = "";
  bool primeira = true;
  while (original.available()) {
    String linha = original.readStringUntil('\n');
    if (primeira) { primeira = false; continue; }
    restante += linha + "\n";
  }
  original.close();

  File novo = SPIFFS.open(caminho, "w");
  if (!novo) return;
  novo.print(restante);
  novo.close();
}

void salvarTemperatura(float tempInt, float tempExt, const char* dataHora) {
  const char* caminho = "/temperaturas.csv";
  const int limite = 500;

  if (contarLinhasArquivo(caminho) >= limite) {
    deletarPrimeiraLinha(caminho);
  }

  File arquivo = SPIFFS.open(caminho, FILE_APPEND);
  if (!arquivo) return;

  arquivo.printf("%s,%.2f,%.2f\n", dataHora, tempInt, tempExt);
  arquivo.close();
}

void salvarEventoPorta(const char* evento, const char* dataHora) {
  const char* caminho = "/porta.csv";
  const int limite = 100;

  if (contarLinhasArquivo(caminho) >= limite) {
    deletarPrimeiraLinha(caminho);
  }

  File arquivo = SPIFFS.open(caminho, FILE_APPEND);
  if (!arquivo) return;

  arquivo.printf("%s,%s\n", dataHora, evento);
  arquivo.close();
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

  // Substitui temporariamente a linha da temperatura externa com o IP
  if (millis() - ultimoMostrarIp >= intervaloExibeIP) {
    ultimoMostrarIp = millis(); // atualiza o tempo da última atualização
    lcd.setCursor(0, 1);
    lcd.print("IP: ");
    lcd.print(WiFi.localIP());
  } else {
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
      salvarEventoPorta("Fechada", dataHora);
      portaAberta = true;
      Serial.print("Porta Fechada - ");
      Serial.print("Data/Hora: ");
      Serial.println(dataHora);
    }
  } else {  // Botão não pressionado, pino em HIGH
    if (portaAberta) {
      portaAbertaTempo = tempoAtual;
      salvarEventoPorta("Aberta", dataHora);
      portaAberta = false;
      Serial.print("Porta Aberta - ");
      Serial.print("Data/Hora: ");
      Serial.println(dataHora);
    }
  }
}

// Núcleo que Lida com sensores
void tarefaSensores(void *parameter) {
  while (true) {
    if (millis() - ultimaLeitura >= intervaloLeitura) {
      ultimaLeitura = millis();

      // Atualiza o tempo uma vez por iteração
      //timeClient.update();
      // Obtém o horário formatado diretamente
      //char dataHora[30];
      //String dataHora = timeClient.getFormattedTime();

      // Obtém a data/hora atual
      time_t rawTime = timeClient.getEpochTime();
      struct tm* timeInfo = localtime(&rawTime);
      char dataHora[30];
      strftime(dataHora, sizeof(dataHora), "%d/%m/%Y %H:%M:%S", timeInfo);

      // Exibe as temperaturas no LCD (a cada 3segundos e a cada 12 segundos exibe o IP)
      if (millis() - ultimaAtualizacao >= intervaloAtualizacao ) {
        Serial.print("IP do ESP32: ");
        Serial.println(WiFi.localIP());

        // Lê as temperaturas
        temperaturaExternaAtual  = lerTemperaturaExterna(); // Retornar Temperatura Externa
        temperaturaInternaAtual = lerTemperaturaInterna(); // Retornar Temperatura Interna

        ultimaAtualizacao  = millis(); // atualiza o tempo da última atualização
        exibirTemperaturas(temperaturaInternaAtual, temperaturaExternaAtual);

        // Atualiza o LED e Alarme com base na temperatura interna
        atualizarLED(temperaturaInternaAtual);
      }

      // Salva os dados de temperatura no arquivo
      // Verifica se já passou o intervalo de gravação
      if (millis() - ultimaGravacao >= intervaloGravacao) {
        Serial.print("Memória disponível: ");
        Serial.println(ESP.getFreeHeap());
        Serial.print("Persistindo Temperatura em memória: ");

        // Atualiza o tempo da última gravação
        ultimaGravacao = millis();

        // Salva os dados de temperatura
        salvarTemperatura(temperaturaInternaAtual, temperaturaExternaAtual, dataHora);
      }

      // Lê o sensor da porta (5 vezes)
      lerSensorPorta();
    }
  }
}

// Função para lidar com o servidor web (Núcleo 1)
void tarefaServidorWeb(void *parameter) {
  while (true) {
    servidor.handleClient();  // Serve as requisições HTTP (arquivo HTML, CSV, etc.)
    delay(500);  // Delay para não sobrecarregar o núcleo 1
  }
}

void loop() {
  // O servidor já está sendo gerido na tarefa do núcleo 1
}

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

  // Iniciar SPIFFS que gerencia dados
  if (!SPIFFS.begin(true)) {
    Serial.println("Erro ao montar SPIFFS");
    while (true);
  }

  // Apagar todos os arquivos armazenados no SPIFFS
  //Serial.println("Apagando todos os arquivos do SPIFFS...");
  //SPIFFS.format();  // Isso apaga todos os dados do SPIFFS
  
  // Continuar com o restante do seu setup normalmente
  //Serial.println("SPIFFS formatado!");

  criarArquivosIniciais(); // CHAMA A FUNÇÃO PRA ENFIAR A MERDA DO HTML

  // Wi-Fi com timeout
  WiFi.begin(ssid, password);
  int tentativasWiFi = 0;
  while (WiFi.status() != WL_CONNECTED && tentativasWiFi < 30) {
    delay(3000);
    Serial.println("Conectando ao WiFi...");
    tentativasWiFi++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Erro: sem Wi-Fi. Reiniciando...");
    ESP.restart();
  }

  Serial.println("Wi-Fi conectado.");
  timeClient.begin();
  timeClient.setTimeOffset(-10800);  // -3 horas em segundos (UTC-3)

  // Tentativas de sincronizar com NTP
  int tentativasNTP = 0;
  while (!timeClient.update() && tentativasNTP < 5) {
    Serial.println("Tentando sincronizar NTP...");
    delay(10000); // Espera 10 segundos entre as tentativas
    tentativasNTP++;
  }
  
  if (tentativasNTP == 5) {
    Serial.println("Erro ao sincronizar NTP. Reiniciando...");
    ESP.restart(); // Reinicia o ESP32 se não conseguir sincronizar com o NTP
  } else {
    Serial.println("Sincronizado com sucesso!");
  }


  servidor.on("/", HTTP_GET, []() {
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
      servidor.send(500, "text/plain", "Erro ao abrir index.html");
      return;
    }
    servidor.streamFile(file, "text/html");
    file.close();
  });

// Servir o arquivo de temperaturas em formato CSV
  servidor.on("/temperaturas", HTTP_GET, []() {
    File file = SPIFFS.open("/temperaturas.csv", "r");
    if (!file) {
      servidor.send(500, "text/plain", "Erro ao abrir temperaturas.csv");
      return;
    }
    servidor.streamFile(file, "text/csv");
    file.close();
  });

  // Servir o arquivo de eventos da porta em formato CSV
  servidor.on("/porta", HTTP_GET, []() {
    File file = SPIFFS.open("/porta.csv", "r");
    if (!file) {
      servidor.send(100, "text/plain", "Erro ao abrir porta.csv");
      return;
    }
    servidor.streamFile( file, "text/csv");
    file.close();
  });

  // endpoint para JSON com temperaturas atuais
  servidor.on("/temperatura_atual", HTTP_GET, []() {
    String json = "{";
    json += "\"interna\":" + String(temperaturaInternaAtual, 2) + ",";
    json += "\"externa\":" + String(temperaturaExternaAtual, 2);
    json += "}";
    servidor.send(200, "application/json", json);
  });

  // Iniciar o servidor
  servidor.begin();
  Serial.println("Servidor HTTP iniciado.");

  // Crie as tarefas no núcleo específico
  xTaskCreatePinnedToCore(
    tarefaSensores,         // Função que será executada
    "TarefaSensores",       // Nome da tarefa
    10000,                  // Tamanho da pilha
    NULL,                   // Parâmetros
    1,                      // Prioridade (1 é baixa, 5 é alta)
    NULL,                   // Handle da tarefa
    0                       // Núcleo 0 (para as leituras dos sensores)
  );

  xTaskCreatePinnedToCore(
    tarefaServidorWeb,      // Função que será executada
    "TarefaServidorWeb",    // Nome da tarefa
    10000,                  // Tamanho da pilha
    NULL,                   // Parâmetros
    1,                      // Prioridade (1 é baixa, 5 é alta)
    NULL,                   // Handle da tarefa
    1                       // Núcleo 1 (para o servidor web)
  );


  temperaturaExternaAtual  = lerTemperaturaExterna();
  temperaturaInternaAtual = lerTemperaturaInterna();

  // Formatar data/hora atual
  time_t rawTime = timeClient.getEpochTime();
  struct tm* timeInfo = localtime(&rawTime);
  char dataHora[30];
  strftime(dataHora, sizeof(dataHora), "%d/%m/%Y %H:%M:%S", timeInfo);

  // Salvar imediatamente
  salvarTemperatura(temperaturaInternaAtual, temperaturaExternaAtual, dataHora);
  Serial.println("Gravou temperatura inicial");

}