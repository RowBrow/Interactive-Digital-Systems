#include <WiFi.h>
#include <PubSubClient.h>
#include "ArduinoJson-v7.3.1.h"

// Set up pin numbers for LEDs
const int GREEN_LED_PIN = 16; // The ESP32 pin GPIO16 connected to LED 1
const int RED_LED_PIN = 17; // The ESP32 pin GPIO17 connected to LED 2

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
const char* MQTT_SUBSCRIBE_TOPIC = "IDS_ESP32/respond_rfid";
const char* MQTT_PUBLISH_TOPIC = "IDS_ESP32/read_rfid";

// Set up RFID reader ports
const int RX_PIN = 3;
const int TX_PIN = 1;

// Set up BUFFER sizes
const int BUFFER_SIZE = 14;       // RFID DATA FRAME FORMAT: 1byte head (value: 2), 10byte data (2byte version + 8byte tag), 2byte checksum, 1byte tail (value: 3)
const int DATA_SIZE = 10;         // 10byte data (2byte version + 8byte tag)
const int DATA_VERSION_SIZE = 2;  // 2byte version (actual meaning of these two bytes may vary)
const int DATA_TAG_SIZE = 8;      // 8byte tag
const int CHECKSUM_SIZE = 2;      // 2byte checksum

HardwareSerial ssrfid(0); // Communication between ESP32 and RDM6300

char buffer[BUFFER_SIZE];  // Used to store an incoming data frame
char send_buffer[70];
int buffer_index = 0; // Used while copying data to the buffer

unsigned long latest_transmission;
unsigned long INTERVAL_BETWEEN_TRANSMISSION = 5000; // The time between transmissions in milliseconds

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
 *
 * A response blocks all other operation
 * in the device for 2 seconds.
 */
void callback(char* topic, byte* payload, unsigned int length) {
  // Check if the topic is the correct topic
  if (strcmp(topic, MQTT_SUBSCRIBE_TOPIC) == 0) {
    JsonDocument doc; // For parsing the JSON response

    deserializeJson(doc, (char*) payload); // Parse JSON response
    
    // Print the message
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]); 
    }
    Serial.print("\n"); 

    // If the response is not directed to
    // this device, do not process further
    if (doc["device"] != MQTT_CLIENT_ID) {
      return;
    }

    // If approved, turn green led on, and vice versa.
    if(doc["approved"]) {
      Serial.println("Authorization successful, access granted.");
      digitalWrite(GREEN_LED_PIN, HIGH); 
      delay(2000);
      digitalWrite(GREEN_LED_PIN, LOW); 
    } else {
      Serial.println("Authorization failed, access not granted.");
      digitalWrite(RED_LED_PIN, HIGH); 
      delay(2000);
      digitalWrite(RED_LED_PIN, LOW);
    }
  }
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
  Serial.begin(115200);

  // Set pins for led output
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  setup_wifi(); // Set up wifi
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT); // Set up mqtt client
  mqttClient.setCallback(callback); // Set the callback function for the client

  // Start UART connection with the RFID scanner
  ssrfid.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  Serial.println("INIT DONE");
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


  // Taken from https://mschoeffler.com/2018/01/05/arduino-tutorial-how-to-use-the-rdm630-rdm6300-rfid-reader/
  // and modified to do different actions with the received RFID tag.
  if (ssrfid.available() > 0) {
    bool call_extract_tag = false;

    int ssvalue = ssrfid.read();

    if (ssvalue == -1) { // no data was read
      return;
    }

    if (ssvalue == 2) { // RDM360/RDM6300 found a tag => tag incoming
      buffer_index = 0;
      Serial.println("Tag incoming");
    } else if (ssvalue == 3) { // Tag has been fully transmitted
      Serial.println("Reading tag complete");
      call_extract_tag = true; // Extract tag at the end of the function call
    }

    if (buffer_index >= BUFFER_SIZE) { // Checking for a buffer overflow (It's very unlikely that an buffer overflow comes up!)
      Serial.println("Error: Buffer overflow detected!");
      return;
    }

    buffer[buffer_index++] = ssvalue; // Everything is alright => copy current value to buffer

    // If at least 5 seconds passed since the last transmission
    if (call_extract_tag && (millis() - latest_transmission > INTERVAL_BETWEEN_TRANSMISSION)) {
      if (buffer_index == BUFFER_SIZE) {
        long tag = extract_tag();
        JsonDocument doc; // For creating the JSON message

        // Set fields to the appropriate values
        doc["device"] = MQTT_CLIENT_ID;
        doc["rfid"] = tag;

        // Convert the JSON into its string representation and send it
        serializeJson(doc, send_buffer);
        mqttClient.publish(MQTT_PUBLISH_TOPIC, send_buffer);
        latest_transmission = millis();
      } else { // Something is wrong... start again looking for preamble (value: 2)
        buffer_index = 0;
        return;
      }
    }
  }
}


/*
 * Taken from https://mschoeffler.com/2018/01/05/arduino-tutorial-how-to-use-the-rdm630-rdm6300-rfid-reader/
 */
long extract_tag() {
  char msg_head = buffer[0];
  char *msg_data = buffer + 1;  // 10 byte => data contains 2byte version + 8byte tag
  char *msg_data_version = msg_data;
  char *msg_data_tag = msg_data + 2;
  char *msg_checksum = buffer + 11;  // 2 byte
  char msg_tail = buffer[13];

  // Print message that was sent from RDM630/RDM6300
  Serial.println("--------");

  Serial.print("Message-Head: ");
  Serial.println(msg_head);

  Serial.println("Message-Data (HEX): ");
  for (int i = 0; i < DATA_VERSION_SIZE; ++i) {
    Serial.print(char(msg_data_version[i]));
  }
  Serial.println(" (version)");
  for (int i = 0; i < DATA_TAG_SIZE; ++i) {
    Serial.print(char(msg_data_tag[i]));
  }
  Serial.println(" (tag)");

  Serial.print("Message-Checksum (HEX): ");
  for (int i = 0; i < CHECKSUM_SIZE; ++i) {
    Serial.print(char(msg_checksum[i]));
  }
  Serial.println("");

  Serial.print("Message-Tail: ");
  Serial.println(msg_tail);

  Serial.println("--");

  long tag = hexstr_to_value(msg_data_tag, DATA_TAG_SIZE); // Extract the tag
  Serial.print("Extracted Tag: ");
  Serial.println(tag);

  long checksum = 0;
  // Calculate the checksum
  for (int i = 0; i < DATA_SIZE; i += CHECKSUM_SIZE) {
    long val = hexstr_to_value(msg_data + i, CHECKSUM_SIZE);
    checksum ^= val;
  }
  Serial.print("Extracted Checksum (HEX): ");
  Serial.print(checksum, HEX);

  // Compare calculated checksum to retrieved checksum
  if (checksum == hexstr_to_value(msg_checksum, CHECKSUM_SIZE)) {
    Serial.print(" (OK)"); // Calculated checksum corresponds to transmitted checksum!
  } else {
    Serial.print(" (NOT OK)"); // Checksums do not match
  }

  Serial.println("");
  Serial.println("--------");

  return tag;
}

/**
 * Converts a hexadecimal value (encoded as ASCII string) to a numeric value 
 * Taken from https://mschoeffler.com/2018/01/05/arduino-tutorial-how-to-use-the-rdm630-rdm6300-rfid-reader/
 */
long hexstr_to_value(char *str, unsigned int length) {
  char *copy = (char *) malloc((sizeof(char) * length) + 1);
  memcpy(copy, str, sizeof(char) * length);
  copy[length] = '\0';
  // the variable "copy" is a copy of the parameter "str". "copy" has an additional '\0' element to make sure that "str" is null-terminated.
  long value = strtol(copy, NULL, 16);  // strtol converts a null-terminated string to a long value (base is 16, so hexadecimal)
  free(copy); // clean up
  return value;
}
