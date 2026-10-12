#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ======================================================
// CONFIGURACIÓN WIFI
// ======================================================

const char* WIFI_SSID = "iPhoneEECC";
const char* WIFI_PASSWORD = "12345678";


// ======================================================
// CONFIGURACIÓN MQTT
// ======================================================


const char* MQTT_SERVER = "172.20.10.2";
const int MQTT_PORT = 1883;

// Topics utilizados por el dashboard React.
const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";


// ======================================================
// CONFIGURACIÓN DE SENSORES
// ======================================================

// (Estudiante)
// Definir los pines y objetos necesarios para los sensores.
const int PIN_LDR = 32; // D32
const int PIN_DTH11 = 4; // D4
#define DHTTYPE DHT11

// Instanciacion del objeto de control
DHT dht(PIN_DTH11, DHTTYPE);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Enviar una lectura cada 2 segundos.
const unsigned long INTERVALO_ENVIO = 2000;

unsigned long ultimoEnvio = 0;

void conectarWiFi() {

  Serial.print("Conectando a WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");

  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {

  while (!mqttClient.connected()) {

    Serial.print("Conectando a Mosquitto...");

    // Crear un Client ID diferente para este ESP32.
    String clientId = "ESP32-";
    clientId += String(
      (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF),
      HEX
    );

    if (mqttClient.connect(clientId.c_str())) {

      Serial.println(" conectado");

    } else {

      Serial.print(" error, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 2 segundos...");

      delay(2000);
    }
  }
}


// ======================================================
// LEER Y PUBLICAR SENSORES
// ======================================================

void publicarSensores() {

  /*
    (Estudiante)
      1. Obtener la temperatura.

      2. Obtener la humedad.

      3. Obtener la lectura del sensor de iluminación.

      4. Preparar los valores para poder publicarlos
         mediante MQTT.

      5. Publicar cada dato en su topic correspondiente:

         TOPIC_TEMPERATURA
         TOPIC_HUMEDAD
         TOPIC_LUZ

      6. Mostrar las mediciones en el Monitor Serial.
  */
  // 1. Obtener la temperatura y humedad desde el DHT11
  float temperatura = dht.readTemperature();
  float humedad     = dht.readHumidity();

  // Control de errores de lectura del bus digital
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println(F("[ERROR] Fallo al leer datos del DHT11. Omisión de envío."));
    return;
  }

  // 2. Obtener la lectura analógica de iluminación (ADC1, rango 0 - 4095)
  int lecturaLuzRaw = analogRead(PIN_LDR);

  // 3. Preparar los buffers de caracteres para MQTT
  char bufferTemp[8];
  char bufferHum[8];
  char bufferLuz[8];

  // Conversión numérica a texto plano (dtostrf para floats, itoa para enteros)
  dtostrf(temperatura, 4, 1, bufferTemp); // ej: "24.5"
  dtostrf(humedad,     4, 1, bufferHum);  // ej: "55.0"
  snprintf(bufferLuz, sizeof(bufferLuz), "%d", lecturaLuzRaw); // ej: "2140"

  // 4. Publicar cada dato en su respectivo Topic
  mqttClient.publish(TOPIC_TEMPERATURA, bufferTemp);
  mqttClient.publish(TOPIC_HUMEDAD, bufferHum);
  mqttClient.publish(TOPIC_LUZ, bufferLuz);

  // 5. Imprimir en el Monitor Serial para auditoría local
  Serial.println(F("--- Publicación MQTT Exitosa ---"));
  Serial.print(F("Temperatura : ")); Serial.print(bufferTemp); Serial.println(F(" °C"));
  Serial.print(F("Humedad     : ")); Serial.print(bufferHum);  Serial.println(F(" %"));
  Serial.print(F("Luz (Raw)   : ")); Serial.print(bufferLuz);  Serial.println(F(" / 4095"));
}

void setup() {

  Serial.begin(115200);

  // (Estudiante)
  // Inicializar los sensores y configurar los pines
  // necesarios.
  Serial.println(F("--- Inicializando DHT11 en DOIT ESP32 DevKit V1 ---"));
  dht.begin();

  conectarWiFi();

  mqttClient.setServer(
    MQTT_SERVER,
    MQTT_PORT
  );
}

void loop() {

  // Si se pierde WiFi, volver a conectarse.
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // Si se pierde MQTT, volver a conectarse.
  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();


  // Publicar sensores cada 2 segundos.
  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {

    ultimoEnvio = millis();

    publicarSensores();
  }
}
