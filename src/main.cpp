#include <Arduino.h>
#include "secrets/wifi.h"
#include "wifi_connect.h"
#include <WiFiClientSecure.h>
#include "ca_cert.h"
#include "secrets/mqtt.h"
#include <PubSubClient.h>
#include <Ticker.h>

namespace
{
    const char *ssid = WiFiSecrets::ssid;
    const char *password = WiFiSecrets::pass;
    const char *mqtt_topic = "home/motion";
    const char *echo_topic = "esp32/echo_test";
    unsigned int publish_count = 0;
    uint16_t keepAlive = 15;    
    uint16_t socketTimeout = 5; 
}


// PIR Sensor Pin
const int pirPin = 5;  // GPIO5
int motionState = LOW;

unsigned long lastMotionTime = 0;
const unsigned long motionDelay = 5000; // 5 giây

// LED Pin
const int ledPin = 4;  // GPIO4

WiFiClientSecure tlsClient;
PubSubClient mqttClient(tlsClient);
Ticker mqttLDRTicker;

void mqttReconnect()
{
    while (!mqttClient.connected())
    {
        Serial.println("Attempting MQTT connection...");
        String client_id = "esp32-client-";
        client_id += String(WiFi.macAddress());
        if (mqttClient.connect(client_id.c_str(), MQTT::username, MQTT::password))
        {
            Serial.print(client_id);
            Serial.println("connected");

            mqttClient.subscribe(mqtt_topic);
        }
        else
        {
            Serial.print("MQTT connect failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 1 seconds");
            delay(1000);
        }
    }
}

void setup() {
  Serial.begin(115200);
  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); 
  setup_wifi(ssid, password);
  tlsClient.setCACert(ca_cert);
  mqttClient.setServer(MQTT::broker, MQTT::port);
}

void loop() {
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();

  int currentState = digitalRead(pirPin);
  if (currentState == HIGH && motionState == LOW) {
    unsigned long now = millis();
    if (now - lastMotionTime > motionDelay) {
      Serial.println("Motion detected!");
      mqttClient.publish(mqtt_topic, "Motion detected");
      lastMotionTime = now;
    }
    motionState = HIGH;
  } else if (currentState == LOW && motionState == HIGH) {
    motionState = LOW;
  }
}