// --- CONFIGURAÇÕES DE COMUNICAÇÃO FIWARE ---
#include <Wire.h>                // Biblioteca para comunicação I2C (usada pelo display OLED)
#include <WiFi.h>                // Biblioteca para gerenciar a conexão Wi-Fi
#include <PubSubClient.h>        // Para comunicação MQTT com o FIWARE
#include <Adafruit_GFX.h>        // Biblioteca gráfica para o display OLED
#include <Adafruit_SSD1306.h>    // Biblioteca específica para o display OLED SSD1306

// Credenciais de Rede (Wokwi)
const char* WIFI_NAME = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// Configurações do Broker MQTT (Sua VM Azure/FIWARE)
const char* default_BROKER_MQTT = "20.150.218.100";
const int default_BROKER_PORT = 1883;

// ID e Tópico para a Entidade Cardíaca no FIWARE
const char* DEVICE_ID = "pulse001";
const char* API_KEY = "TEF";
const char* TOPICO_PUBLISH = "/TEF/pulse001/attrs";

// Definições de tamanho para o display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Declaração do objeto do display SSD1306
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Definições de pinos
#define RELAY 19
#define PULSE_PIN 35
#define alertLed 32

// Cliente Wi-Fi e MQTT
WiFiClient espClient;
PubSubClient MQTT(espClient);

// Limites da frequência cardíaca
int minHeartRate = 50;
int maxHeartRate = 120;

// --- FUNÇÕES DE CONEXÃO MQTT ---
void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.print("* Tentando conectar ao Broker MQTT FIWARE: ");
    Serial.println(default_BROKER_MQTT);
    if (MQTT.connect(DEVICE_ID)) {
      Serial.println("Conectado com sucesso ao broker MQTT FIWARE!");
    } else {
      Serial.print("Falha ao reconectar no broker, rc=");
      Serial.print(MQTT.state());
      Serial.println(". Nova tentativa em 2s");
      delay(2000);
    }
  }
}

// Callback MQTT (não utilizado neste projeto)
void mqtt_callback(char* topic, byte* payload, unsigned int length) {}

// --- SETUP ---
void setup() {
  Wire.begin();
  Wire.begin(16, 17);
  Serial.begin(115200);

  pinMode(alertLed, OUTPUT);
  pinMode(RELAY, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 30);
  display.println("Sensor Cardiaco");
  display.display();

  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Wifi not connected...\n");
  }

  Serial.println("\nWifi connected");
  Serial.println("IP Address: " + String(WiFi.localIP()));
  WiFi.mode(WIFI_STA);

  MQTT.setServer(default_BROKER_MQTT, default_BROKER_PORT);
  MQTT.setCallback(mqtt_callback);

  Serial.println("A frequencia sera monitorada:");
}

// --- LOOP PRINCIPAL ---
void loop() {
  if (!MQTT.connected()) {
    reconnectMQTT();
  }
  MQTT.loop();

  int16_t pulseValue = analogRead(PULSE_PIN);
  float voltage = pulseValue * (5.0 / 4095.0);
  int heartRate = (voltage / 3.3) * 675;

  String payload = "hr|" + String(heartRate);
  if (MQTT.publish(TOPICO_PUBLISH, payload.c_str())) {
    Serial.print("FIWARE ENVIO OK: HeartRate=");
    Serial.println(heartRate);
  } else {
    Serial.println("FIWARE ENVIO FALHOU.");
  }

  if (heartRate < minHeartRate) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Frequencia cardiaca abaixo");
    display.setCursor(0, 15);
    display.print("Limite Minimo");
    display.setCursor(20, 30);
    display.print(String(heartRate) + " bpm");
    display.display();

    digitalWrite(RELAY, LOW);
    digitalWrite(alertLed, LOW);

  } else if (heartRate > maxHeartRate) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("ALERTA MAXIMO!");
    display.setCursor(20, 30);
    display.print(String(heartRate) + " bpm");
    display.setCursor(10, 50);
    display.print("EMERGENCIA ATIVA");
    display.display();

    digitalWrite(alertLed, HIGH);
    digitalWrite(RELAY, HIGH);

  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("A frequencia esta normal");
    display.setCursor(20, 40);
    display.print(String(heartRate) + " bpm");
    display.display();

    digitalWrite(RELAY, LOW);
    digitalWrite(alertLed, LOW);
  }

  delay(10000);
}