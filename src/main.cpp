#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include "secrets.h"

// Declaraciones anticipadas
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();

// Configuración MQTT por defecto
const char* mqttServer = MQTT_SERVER;
const int mqttPort = 1883;
const char* mqttUser = MQTT_USER;

// Pines
const int ledPin = 18;
const int touchPin = 32;
const int touchThreshold = 60;

// Number of attemps
int attempts = 0;

// Estados
bool ledState = false;
bool isOfflineMode = false;
bool isTouching = false;
bool longPressDetected = false;
bool messageFromOther = false;
unsigned long touchStartTime = 0;
unsigned long previousMillis = 0;
const long interval = 3000; // Intervalo para toggle por MQTT

// Tópicos MQTT (se inicializan con valores por defecto, pero se pueden cambiar)
String mqttTopicPublish = "esp32/home/lamp/dani";
String mqttTopicSubscribe = "esp32/home/lamp/santi";

// Clientes
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WiFiManager wifiManager;
// =========================
// FUNCIONES AUXILIARES
// =========================

void toggleLED() {
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
    Serial.println(ledState ? "LED Encendido" : "LED Apagado");
}

void setupMQTT() {
    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(mqttCallback);
    reconnectMQTT();
}

void reconnectMQTT() {
   
    while (!mqttClient.connected() && attempts < 7) {
        Serial.println("Intentando conectar al broker MQTT...");
        if (mqttClient.connect("ESP32Client", mqttUser, "")) {
            Serial.println("Conectado al broker MQTT.");
            mqttClient.subscribe(mqttTopicSubscribe.c_str());
            return;
        }
        Serial.print("Error de conexión MQTT. Código: ");
        Serial.println(mqttClient.state());
        Serial.println("Reintentando en 5 segundos...");
        delay(5000);
        attempts++;
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Mensaje MQTT recibido en el tópico: ");
    Serial.println(topic);
    Serial.print("Mensaje: ");
    Serial.println(message);
    messageFromOther = true;
}

void handleTouchInput() {
    static unsigned long lastTouchChange = 0;
    const int debounceDelay = 50; // 50 ms de debounce
    int touchValue = touchRead(touchPin);

    if (touchValue < touchThreshold) {
        if (!isTouching && millis() - lastTouchChange > debounceDelay) {
            isTouching = true;
            lastTouchChange = millis();
            touchStartTime = millis();
            toggleLED();
            messageFromOther = false;
        } 
        else if (!longPressDetected && millis() - touchStartTime >= 1000 && !isOfflineMode) {
            longPressDetected = true;
            if (mqttClient.connected()) {
                mqttClient.publish(mqttTopicPublish.c_str(), "hola");
                Serial.println("Mensaje MQTT enviado por pulsación larga");
            }
        } 
        else if (!longPressDetected && millis() - touchStartTime >= 3000 && isOfflineMode) {
            longPressDetected = true;
                 // Parámetros personalizados para tópicos
            WiFiManagerParameter custom_topic_pub("pub", "MQTT Topic Publish", mqttTopicPublish.c_str(), 64);
            WiFiManagerParameter custom_topic_sub("sub", "MQTT Topic Subscribe", mqttTopicSubscribe.c_str(), 64);

            wifiManager.addParameter(&custom_topic_pub);
            wifiManager.addParameter(&custom_topic_sub);

            wifiManager.startConfigPortal("OnDemandAP");
            Serial.println("Portal WiFi on demand abierto");

            // Actualizar tópicos con los valores ingresados
            mqttTopicPublish = String(custom_topic_pub.getValue());
            mqttTopicSubscribe = String(custom_topic_sub.getValue());

            Serial.print("Nuevo topic publish: ");
            Serial.println(mqttTopicPublish);
            Serial.print("Nuevo topic subscribe: ");
            Serial.println(mqttTopicSubscribe);

            // Revisar si se conectó
            if (WiFi.isConnected()) {
                isOfflineMode = false;
                setupMQTT(); // Reconectar MQTT usando nuevos tópicos
                Serial.println("Conexión WiFi restablecida, modo online");
            }
        }
    } else {
        if (isTouching && millis() - lastTouchChange > debounceDelay) {
            isTouching = false;
            longPressDetected = false;
            lastTouchChange = millis();
        }
    }
}

void handleMQTTMessages() {
    unsigned long currentMillis = millis();
    if (mqttClient.connected() && currentMillis - previousMillis >= interval && messageFromOther) {
        previousMillis = currentMillis;
        toggleLED();
    }
}

// =========================
// SETUP
// =========================
void setup() {
    //wifiManager.resetSettings();
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, ledState);

    WiFi.mode(WIFI_STA);
    WiFi.begin();
    
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        setupMQTT();
    } else {
        isOfflineMode = true;
        Serial.println("Tiempo de conexión WiFi agotado. Modo offline activado.");
    }
}

// =========================
// LOOP
// =========================
void loop() {
    if (!isOfflineMode) {
        if (!mqttClient.connected()) {
            reconnectMQTT();
        }
        mqttClient.loop();
        handleMQTTMessages();
    }

    handleTouchInput();
}
