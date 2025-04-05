#include <WiFi.h>
#include <PubSubClient.h>

// Set up the credentials for the WiFi
const char* SSID = "RUC-IOT";
const char* PASSWORD = "GiHa2638La";
const int MAX_CONNECTION_TRIES = 20;

// Configure for selected MQTT broker
const char* MQTT_SERVER = "public.cloud.shiftr.io";
const int MQTT_PORT = 1883;

// Set up the credentials for the MQTT broker
const char* MQTT_USER = "public";
const char* MQTT_PASSWORD = "public";
const char* MQTT_CLIENT_ID = "IDS_ESP32_RFID_READER";

// Set up the topics to subscribe and publish to
const char* MQTT_SUBSCRIBE_TOPIC = "IDS_ESP32/write_rfid";
const char* MQTT_PUBLISH_TOPIC = "IDS_ESP32/read_rfid";

/**
 * Specifies the total number of
 * messages sent so far.
 */
int total_sent_messages = 0;

// Set up WiFi and MQTT clients
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

/**
 * Attempt to connect to the wifi
 * for a number of tries
 */
void setup_wifi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(SSID, PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_CONNECTION_TRIES) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  // Give information whether the connection was successful
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nConnected to WiFi with IP ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed!");
  }
}

/**
 * Perform actions based on the topic and the payload
 * of an MQTT message
 */
void callback(char* topic, byte* payload, unsigned int length) {
  // TODO: For now, the callback function only prints the message out.
  //  This function should include the logic for when we receive
  //  a approve/disapprove response from the server
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.print("\n");
}

/**
 * Attempt to reconnect to the MQTT server
 */
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
  // Reset the memory
  total_sent_messages = 0;
  Serial.begin(115200);
  setup_wifi(); // Set up wifi
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT); // Set up mqtt client
  mqttClient.setCallback(callback); // Set the callback function for the client
}

void loop() {
  // Check if connection to WiFi still exists
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi(); // Attempt to reconnect if necessary
  }

  // Check if connection to MQTT broker still exists
  if (!mqttClient.connected()) {
    reconnect(); // Attempt to reconnect if necessary
  }

  mqttClient.loop(); // Receive messages

  // TODO: This is the point in the code where RFID reading should happen.
  //  Based on whether there was a read or not, we will publish a message

  // Publishing messages
  mqttClient.publish(MQTT_PUBLISH_TOPIC, "{\"rfid\":\"2142a9bbb2ou34h234i\",\"device\":\"aDevice\"}"); // TODO: This will be replaced by the block above 
  delay(5000);
}
