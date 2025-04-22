// Load MQTT.js from CDN
const mqttUrl = "https://unpkg.com/mqtt/dist/mqtt.min.js";
const script = document.createElement("script");
script.src = mqttUrl;
document.head.appendChild(script);

// This variable stores the MQTT client
let mqttClient;

// Message to display on the canvas
let messageShownOnCanvas = "No message yet.";



// When MQTT.js is loaded,
// set up the client  //called after the external MQTT.js library has finished loading.
script.onload = function () {
  setupMQTT();
};

// List of RFID tags recognized
// by the server as 'authorized'
let validRFIDs = [7895502, 805327990];

const MQTT_SUBSCRIBE_TOPIC = "IDS_ESP32/read_rfid";
const MQTT_PUBLISHING_TOPIC = "IDS_ESP32/respond_rfid";

function setupMQTT() {
  let randomId = "p5js_client_" + Math.random().toString(36).substr(2, 5);
  let broker = "wss://public.cloud.shiftr.io:443"; // Use WSS for WebSockets over SSL

  // For now, the client does not need authentication
  // which is a security risk for later
  mqttClient = mqtt.connect(broker, {
    clientId: randomId,
    username: "public",
    password: "public",
  });

  // Setting up client callback functions
  mqttClient.on("connect", onConnect);
  mqttClient.on("message", onMessageArrived);
  mqttClient.on("error", onFailure);
  mqttClient.on("close", onConnectionLost);
}

function setup() {
  createCanvas(600, 200);
  background(220);
}

function draw() {
  background(220);
  textAlign(CENTER, CENTER);
  text(messageShownOnCanvas, width / 2, height / 2);
}

function onConnect() {
  console.log("Connected to MQTT"); // Confirm connection on console
  mqttClient.subscribe(MQTT_SUBSCRIBE_TOPIC); // Subscribe to topic
}

function onMessageArrived(topic, message) {
  // Put information about the message received in the console
  let msg = message.toString();
  let time = new Date(); 
  console.log(`[INFO]: [${time.toLocaleTimeString()}] Received message ${msg}  on topic ${topic}`);

  // Parse the message into an object
  let parsed_msg = JSON.parse(message);
  
  // If the topic is 
  if (topic === MQTT_SUBSCRIBE_TOPIC) {
    if (parsed_msg.rfid) {
      // Change the message on the canvas
      messageShownOnCanvas = `[${time.toLocaleTimeString()}] RFID ${parsed_msg.rfid} read from device ${parsed_msg.device}.`;
    }

    // Check whether the RFID received is 
    // in the list of authorized IDs
    let approved = validRFIDs.includes(parsed_msg.rfid);

    // Prepare the response
    let response = {
      device: parsed_msg.device,
      approved: approved
    };

    // Publish the response under the appropriate topic
    mqttClient.publish(MQTT_PUBLISHING_TOPIC, JSON.stringify(response));
    console.log(`[INFO]: Device ${parsed_msg.device} was given a response ${approved ? "Approved" : "Denied"} for the RFID ${parsed_msg.rfid}.`)
  }
}

function onFailure(error) {
  console.error(`[ERROR]: Failed to connect: ${error}`);
}

function onConnectionLost() {
  console.warn("[WARN]: Connection Lost");
}
