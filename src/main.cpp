#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h> // Para configuración WiFi dinámica
#include <PubSubClient.h>
#include "secrets.h"     // Aquí defines MQTT_SERVER y MQTT_USER, etc.

// Declaraciones anticipadas
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();

// Configuración MQTT
const char* mqttServer = MQTT_SERVER;
const int mqttPort = 1883;
const char* mqttTopicPublish = "esp32/home/lamp/santi";
const char* mqttTopicSubscribe = "esp32/home/lamp/dani";
const char* mqttUser = MQTT_USER;

// Pines
const int ledPin = 18;
const int touchPin = 32;
const int touchThreshold = 40; // Umbral para detectar touch

// Estados
bool ledState = false;
bool isOfflineMode = false;
bool isTouching = false;
bool longPressDetected = false;
bool messageFromOther = false;
unsigned long touchStartTime = 0;
unsigned long previousMillis = 0;
const long interval = 3000; // Intervalo para togglear LED por MQTT

// Clientes
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// =========================
// FUNCIONES AUXILIARES
// =========================

// Alternar LED localmente
void toggleLED() {
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
    Serial.println(ledState ? "LED Encendido" : "LED Apagado");
}

// Configurar y conectar WiFi con WiFiManager (si falla, modo offline)
void setupWiFi() {
    WiFiManager wifiManager;
    wifiManager.setConfigPortalTimeout(180); // Portal dura 3 min

    if (!wifiManager.autoConnect("ESP32_AP")) {
        Serial.println("WiFi no conectado. Activando modo offline...");
        isOfflineMode = true;
    }
}

// Configurar MQTT
void setupMQTT() {
    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(mqttCallback);
    reconnectMQTT();
}

// Reconectar MQTT
void reconnectMQTT() {
    int attempts = 0;
    while (!mqttClient.connected() && attempts < 7) {
        Serial.println("Intentando conectar al broker MQTT...");
        if (mqttClient.connect("ESP32Client", mqttUser, "")) {
            Serial.println("Conectado al broker MQTT.");
            mqttClient.subscribe(mqttTopicSubscribe);
            return;
        }
        Serial.print("Error de conexión MQTT. Código: ");
        Serial.println(mqttClient.state());
        Serial.println("Reintentando en 5 segundos...");
        delay(5000);
        attempts++;
    }
}

// Callback al recibir mensajes MQTT
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

// Manejar entrada táctil
void handleTouchInput() {
    int touchValue = touchRead(touchPin);

    if (touchValue < touchThreshold) {
        if (!isTouching) {
            // Primera detección de toque
            isTouching = true;
            touchStartTime = millis();
            toggleLED();
            messageFromOther = false;
        } 
        else if (!longPressDetected && millis() - touchStartTime >= 1000 && !isOfflineMode) {
            // Pulsación larga (>= 1 s) y con WiFi activo → publicar MQTT
            longPressDetected = true;
            if (mqttClient.connected()) {
                mqttClient.publish(mqttTopicPublish, "hola");
                Serial.println("Mensaje MQTT enviado por pulsación larga");
            }
        } 
        else if (!longPressDetected && millis() - touchStartTime >= 3000 && isOfflineMode) {
            // Pulsación muy larga (>= 3 s) y en offline → abrir portal WiFi
            longPressDetected = true;
            WiFiManager wifiManager;
            wifiManager.startConfigPortal("OnDemandAP");
            Serial.println("Portal WiFi on demand abierto");

            // Re-evaluar conexión
            if (WiFi.isConnected()) {
                isOfflineMode = false;
                setupMQTT(); // Si quieres reconectar MQTT
                Serial.println("Conexión WiFi restablecida, modo online");
            }
        }
    } else {
        // Se suelta el touch
        isTouching = false;
        longPressDetected = false;
    }
}

// Controlar mensajes recibidos MQTT (toggle cada 3 s)
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

    if (WiFi.isConnected()) {
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
    if (WiFi.isConnected()) {
        if (!mqttClient.connected()) {
            reconnectMQTT();
        }
        mqttClient.loop();
        handleMQTTMessages();
    }

    handleTouchInput();
}
