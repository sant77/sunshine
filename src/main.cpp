#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include "secrets.h"


// Añade estas líneas al principio, después de los includes pero antes de cualquier función
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();

// MQTT Configuration
const char* mqttServer = MQTT_SERVER;
const int mqttPort = 1883;
const char* mqttTopicPublish = "esp32/home/lamp/santi";
const char* mqttTopicSubscribe = "esp32/home/lamp/dani";
const char* mqttUser = MQTT_USER;

// Hardware Configuration
const int ledPin = 18;
const int touchPin = 32;
const int touchThreshold = 40;

// State Variables
bool ledState = false;
bool isOfflineMode = false;
bool isTouching = false;
bool longPressDetected = false;
bool messsage_from_other = false;
unsigned long touchStartTime = 0;
unsigned long previousMillis = 0;
const long interval = 3000;      // MQTT toggle interval

// Network Clients
WiFiClient espClient;
PubSubClient mqttClient(espClient);


void toggleLED() {
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
    Serial.println(ledState ? "LED Encendido" : "LED Apagado");
}

void setupWiFi() {
    WiFiManager wifiManager;
    wifiManager.setConfigPortalTimeout(180);
    if (!wifiManager.autoConnect("ESP32_AP")) {
        Serial.println("WiFi no conectado. Activando modo offline...");
    }
}

void setupMQTT() {
    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(mqttCallback);
    reconnectMQTT();
}

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

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Mensaje MQTT recibido en el tópico: ");
    Serial.println(topic);
    Serial.print("Mensaje: ");
    Serial.println(message);
    messsage_from_other = true;
}


void handleTouchInput() {
    int touchValue = touchRead(touchPin);
    
    if (touchValue < touchThreshold) {
        if (!isTouching) {

            isTouching = true;
            touchStartTime = millis();
            toggleLED();
            messsage_from_other = false;

        } else if (!longPressDetected && millis() - touchStartTime >= 1000 && !isOfflineMode) {
            longPressDetected = true;
            if (mqttClient.connected()) {

                mqttClient.publish(mqttTopicPublish, "hola");

            }
            Serial.println("Se detectó una pulsación larga en el sensor táctil");
        }else if(!longPressDetected && millis() - touchStartTime >= 3000 && isOfflineMode){

            WiFiManager wifiManager;
            wifiManager.startConfigPortal("OnDemandAP");
            Serial.println("connected...yeey :)");

        }
    } else {
        isTouching = false;
        longPressDetected = false;
    }
}

void handleMQTTMessages() {
    unsigned long currentMillis = millis();
    if (mqttClient.connected() && currentMillis - previousMillis >= interval && messsage_from_other) {
        previousMillis = currentMillis;
        toggleLED();
    }
}

void checkWiFiTimeout() {
    if (!WiFi.isConnected()) {
        isOfflineMode = true;
        Serial.println("Tiempo de conexión WiFi agotado. Modo offline activado.");
    }
}

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
    }

    checkWiFiTimeout();
}

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