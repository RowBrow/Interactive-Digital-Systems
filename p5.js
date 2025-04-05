// Load MQTT.js from CDN
const mqttUrl = "https://unpkg.com/mqtt/dist/mqtt.min.js";
const script = document.createElement("script");
script.src = mqttUrl;
document.head.appendChild(script);

// This variable stores the MQTT client
let mqttClient;

// Message to display on the canvas
let messageReceived = "No message yet.";



// When MQTT.js is loaded,
// set up the client
script.onload = function () {
  setupMQTT();
};

// This may be used for the list of RFIDs
// with 
let validRFIDs = [78955023];

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
  // Boilerplate code, will change later
  // TODO: Change canvas to be more informative
  createCanvas(600, 200);
  background(220);
  
  
}

function draw() {
  background(220);
  textAlign(CENTER, CENTER);
  text(messageReceived, width / 2, height / 2);
}

function onConnect() {
  console.log("Connected to MQTT"); // Confirm connection on console
  mqttClient.subscribe(MQTT_SUBSCRIBE_TOPIC); // Subscribe to topic
}

function onMessageArrived(topic, message) {
  let msg = message.toString();
  let time = new Date(); 
  console.log(`[INFO]: [${time.toLocaleTimeString()}] Received message ${msg}  on topic ${topic}`)
  let parsed_msg = JSON.parse(message);
  
  if (topic === MQTT_SUBSCRIBE_TOPIC) {
    if (parsed_msg.rfid) {
      messageReceived = `[${time.toLocaleTimeString()}] RFID ${parsed_msg.rfid} read from device ${parsed_msg.device}.`;
    }
  }
  
  let approved = validRFIDs.includes(parsed_msg.rfid);
  
  let response = {
    device: parsed_msg.device,
    approved: approved
  }
  
  mqttClient.publish(MQTT_PUBLISHING_TOPIC, JSON.stringify(response))
}

// Writes an id to an RFID chip over MQTT
// This means the devices on this network 
// would receive a message to write a 
// particular ID and would try a write
// on an RFID chip.
function writeRFID(device, rfid) {
  let message = {
    device: device, // Device that will use the RFID to write to a chip
    rfid: rfid, // The ID to write to the chip
  };
  mqttClient.publish(MQTT_PUBLISHING_TOPIC, JSON.stringify(message));
}

function drawRandomCircle() {
  let x = random(width);
  let y = random(height);
  let radius = random(20, 50);
  let r = random(255);
  let g = random(255);
  let b = random(255);

  fill(r, g, b);
  noStroke();
  ellipse(x, y, radius * 2);
}

function onFailure(error) {
  console.error(`[ERROR]: Failed to connect: ${error}`);
}

function onConnectionLost() {
  console.warn("[WARN]: Connection Lost");
}
