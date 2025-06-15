#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <PubSubClient.h>
#include "secrets.h"

//mqtt
const char* mqttServer = MQTT_SERVER;
const int mqttPort = 1883;
const char* mqttTopicPublish = "esp32/home/lamp/santi";
const char* mqttTopicSubscribe = "esp32/home/lamp/dani";
const char* mqttUser = MQTT_USER;       // Usuario del broker MQTT


WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Define the LED pin
const int ledPin = 18;
bool ledState = false;
unsigned long previousMillis = 0;  // will store last time LED was updated
const long interval = 3000;  // interval at which to blink (milliseconds)
bool messsage_from_other = false;

// Touch pin 
const int touchPin = 32;
const int touchThreshold = 40;

// Variables for touch press timing
unsigned long touchStartTime = 0;
bool isTouching = false;
bool longPressDetected = false;

// Server asincrono
AsyncWebServer server(80);

// Bandera para activar WiFiManager
bool activateWiFiManager = false;

void toggleLED() {
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
    Serial.println(ledState ? "LED Encendido" : "LED Apagado");
}

void reconnectMQTT() {
    int attemps = 0;
    while (!mqttClient.connected()) {
        Serial.println("Intentando conectar al broker MQTT...");
        if (mqttClient.connect("ESP32Client", mqttUser, "")) {
            Serial.println("Conectado al broker MQTT.");
            mqttClient.subscribe(mqttTopicSubscribe); // Suscribirse al tema deseado
        } else {
            Serial.print("Error de conexión MQTT. Código: ");
            Serial.println(mqttClient.state());
            Serial.println("Reintentando en 5 segundos...");
            delay(5000);
        }
        attemps++;
        if (attemps>7){
            return ;
        }
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

void setup() {
    // Puerto serial para debug
    Serial.begin(115200);

    // Configurar pin LED como salida
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, ledState);

    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    
    // Intenta conectarte automáticamente al WiFi
    WiFiManager wifiManager;
    wifiManager.setConfigPortalTimeout(180);
    wifiManager.autoConnect("ESP32_AP");
  
    if (!WiFi.isConnected()) {
        Serial.println("WiFi no conectado. Esperando activación...");
    }
    else{ 
        
        mqttClient.setServer(mqttServer, mqttPort);
        mqttClient.setCallback(mqttCallback);
        // Intentar conectar al broker MQTT
        reconnectMQTT();
        }

    // Serve the HTML file
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", String(), false);
    });

    // Turn on LED
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
        ledState = true;
        digitalWrite(ledPin, ledState);
        request->send(200, "text/plain", "LED On");
    });

    // Turn off LED
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        ledState = false;
        digitalWrite(ledPin, ledState);
        request->send(200, "text/plain", "LED Off");
    });

    // Start server
    server.begin();
}

void loop() {
   
    // Leer el valor del sensor táctil
    int touchValue = touchRead(touchPin);

    if (touchValue < touchThreshold) {
        if (!isTouching) {
            isTouching = true;
            touchStartTime = millis();
            toggleLED();
            messsage_from_other = false;
        } else if (!longPressDetected && millis() - touchStartTime >= 1000) {
            // Pulsación larga detectada
            longPressDetected = true;

            if (mqttClient.connected()) {
                mqttClient.publish(mqttTopicPublish, "hola");
            }
            Serial.println("Se detectó una pulsación larga en el sensor táctil");
        }
    } else {
        // Sensor táctil liberado
        isTouching = false;
        longPressDetected = false;
    }

     unsigned long currentMillis = millis();

  if (mqttClient.connected() && currentMillis - previousMillis >= interval && messsage_from_other) {
    previousMillis = currentMillis;
    toggleLED();
    }

  if (WiFi.isConnected() && !mqttClient.connected()) {
        reconnectMQTT();
    }

    // Mantener el cliente MQTT
    mqttClient.loop();
}



