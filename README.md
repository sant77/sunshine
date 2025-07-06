# ESP32 Touch Lamp con Modo Offline y Configuración MQTT Dinámica

Este proyecto implementa un sistema para controlar una lámpara (LED) usando un **sensor táctil** en un ESP32.  
Además, permite enviar y recibir mensajes MQTT y ofrece un **modo offline** con activación de WiFi on demand mediante una pulsación larga.

## ✨ Características principales

- ✅ Control local del LED mediante toque.
- ✅ Publicación de mensajes MQTT al tocar durante 1 segundo.
- ✅ Suscripción a mensajes MQTT para controlar el LED remotamente.
- ✅ Modo offline si no hay conexión WiFi al arrancar.
- ✅ Activación de **WiFi on demand** manteniendo el touch durante 3 segundos.
- ✅ Configuración de los tópicos MQTT (publish y subscribe) desde el portal WiFiManager.

---

## 🚀 Funcionamiento

### Inicio normal

1. Al encender, el ESP32 intenta conectarse al WiFi (timeout: 5 segundos).
2. Si se conecta, se inicializa MQTT y el dispositivo entra en **modo online**.
3. Si falla, entra en **modo offline** y no intenta reconectarse automáticamente.

### Control con el sensor táctil

- **Toque corto (< 1 segundo):** cambia (toggle) el estado del LED localmente.
- **Pulsación larga (≥ 1 segundo, estando online):** publica un mensaje "hola" al tópico configurado.
- **Pulsación muy larga (≥ 3 segundos, estando offline):** abre un portal WiFi (AP "OnDemandAP") para configurar WiFi y los tópicos MQTT.

---

## ⚙️ Configuración dinámica de MQTT

Cuando se abre el portal WiFiManager (pulsación larga en offline):

- Aparecen dos campos adicionales:
  - `MQTT Topic Publish`: tópico donde se enviarán los mensajes (por ejemplo, `esp32/home/lamp/santi`).
  - `MQTT Topic Subscribe`: tópico donde el ESP32 escuchará mensajes para controlar el LED (por ejemplo, `esp32/home/lamp/dani`).

Una vez configurado y conectado, los nuevos tópicos se usan inmediatamente.

---

## 📄 Estructura de archivos

├── ESP32_Touch_Lamp.ino # Código principal
├── secrets.h # Contiene las credenciales MQTT (MQTT_SERVER, MQTT_USER)
└── README.md # Este archivo


---

## 💡 Dependencias

- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [PubSubClient](https://github.com/knolleary/pubsubclient)

Puedes instalarlas desde el **Administrador de Librerías** de Arduino IDE.

---

## ⚡ Pines y hardware

| Elemento     | Pin ESP32 |
|---------------|-----------|
| LED           | GPIO 18   |
| Touch sensor  | GPIO 32   |

El **umbral táctil** se define en el código (`touchThreshold = 40`). Puede ajustarse dependiendo del sensor usado.

---

## 🌐 Ejemplo de uso

### 1️⃣ Primer arranque

- El ESP32 intenta conectarse a tu WiFi.
- Si no lo logra, queda en modo offline.  
- Mantén pulsado el touch ≥ 3 segundos → se abre un portal WiFi (AP "OnDemandAP").

### 2️⃣ Configuración

- Conéctate a "OnDemandAP" desde tu móvil o PC.
- Configura WiFi y los tópicos MQTT.
- Guarda y reinicia.

### 3️⃣ Control local

- Toca rápido para alternar el LED.
- Mantén ≥ 1 segundo para enviar mensaje MQTT (si está online).
- Los mensajes recibidos en el tópico de suscripción pueden alternar el LED remotamente.

---

## ⚙️ Notas

- Actualmente, los tópicos configurados se pierden si reinicias el ESP32 (no se guardan en memoria flash).
- Si quieres persistirlos (por ejemplo, usando SPIFFS o LittleFS), se puede implementar fácilmente.

---

## 🧑‍💻 Autores y créditos

- **Desarrollador principal:** [Santiago Duque Ramos✨]
- Basado en librerías de:
  - Tzapu (WiFiManager)
  - Knolleary (PubSubClient)

---

## 💬 Contacto

Si tienes dudas o sugerencias, ¡no dudes en abrir un issue o contactarme!

---

✅ **¡Disfruta controlando tu lámpara inteligente!**
