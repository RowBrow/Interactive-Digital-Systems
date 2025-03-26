#include <WiFi.h>
#include <WiFiAP.h>
#include <PubSubClient.h>

const char* SSID = "RUC-IOT";
const char* PASSWORD = "GiHa2638La";
const int MAX_CONNECTION_TRIES = 20;

const char* MQTT_SERVER = "public.cloud.shiftr.io";
const int MQTT_PORT = 1883;

const char* MQTT_USER = "public";
const char* MQTT_PASSWORD = "public";
const char* MQTT_CLIENT_ID = "IDS_ESP32_RFID_READER";

const char* MQTT_SUBSCRIBE_TOPIC = "IDS_ESP32/write_rfid";
const char* MQTT_PUBLISH_TOPIC = "IDS_ESP32/read_rfid";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void setup_wifi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(SSID, PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_CONNECTION_TRIES) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nConnected to WiFi with IP ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  // TODO: For now, the callback function only prints the message out
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.print("\n");
}

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      mqttClient.subscribe(MQTT_SUBSCRIBE_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  if (!mqttClient.connected()) {
    reconnect();
  }

  // Receiving messages
  mqttClient.loop();

  // Publishing messages
  mqttClient.publish(MQTT_PUBLISH_TOPIC, "{\"rfid\":\"2142a9bbb2ou34h234i\",\"device\":\"aDevice\"}");
  delay(5000);
} 
