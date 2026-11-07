#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>

// =================================
// AIRCRAFT INSPECTION ROVER
// =================================

// Wi-Fi access point
const char* ssid = "Aircraft_Rover";
const char* password = "rover1234";

// Hardware objects
WebServer server(80);
Adafruit_MCP23X17 mcp;

// =================================
// ESP32-CAM AI THINKER CAMERA PINS
// =================================

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// =================================
// ULTRASONIC SENSOR ECHO PINS
// =================================

#define FRONT_ECHO 12
#define REAR_ECHO  13
#define LEFT_ECHO  14
#define RIGHT_ECHO 15

// MCP23017 output pins
#define MOTOR_IN1 0
#define MOTOR_IN2 1
#define MOTOR_IN3 2
#define MOTOR_IN4 3

#define FRONT_TRIG 4
#define REAR_TRIG  5
#define LEFT_TRIG  6
#define RIGHT_TRIG 7

#define BUZZER_PIN 8
#define LED_PIN    9

// =================================
// SYSTEM VARIABLES
// =================================

float frontDistance = 999;
float rearDistance  = 999;
float leftDistance  = 999;
float rightDistance = 999;

const float SAFE_DISTANCE = 25.0;

char currentMovement = 'S';

unsigned long lastSensorCheck = 0;

// =================================
// MOTOR CONTROL
// =================================

void stopMotors() {
  mcp.digitalWrite(MOTOR_IN1, LOW);
  mcp.digitalWrite(MOTOR_IN2, LOW);
  mcp.digitalWrite(MOTOR_IN3, LOW);
  mcp.digitalWrite(MOTOR_IN4, LOW);

  currentMovement = 'S';
}

void forward() {
  mcp.digitalWrite(MOTOR_IN1, HIGH);
  mcp.digitalWrite(MOTOR_IN2, LOW);

  mcp.digitalWrite(MOTOR_IN3, HIGH);
  mcp.digitalWrite(MOTOR_IN4, LOW);

  currentMovement = 'F';
}

void backward() {
  mcp.digitalWrite(MOTOR_IN1, LOW);
  mcp.digitalWrite(MOTOR_IN2, HIGH);

  mcp.digitalWrite(MOTOR_IN3, LOW);
  mcp.digitalWrite(MOTOR_IN4, HIGH);

  currentMovement = 'B';
}

void turnLeft() {
  mcp.digitalWrite(MOTOR_IN1, LOW);
  mcp.digitalWrite(MOTOR_IN2, HIGH);

  mcp.digitalWrite(MOTOR_IN3, HIGH);
  mcp.digitalWrite(MOTOR_IN4, LOW);

  currentMovement = 'L';
}

void turnRight() {
  mcp.digitalWrite(MOTOR_IN1, HIGH);
  mcp.digitalWrite(MOTOR_IN2, LOW);

  mcp.digitalWrite(MOTOR_IN3, LOW);
  mcp.digitalWrite(MOTOR_IN4, HIGH);

  currentMovement = 'R';
}

// =================================
// ULTRASONIC DISTANCE MEASUREMENT
// =================================

float measureDistance(int triggerPin, int echoPin) {

  mcp.digitalWrite(triggerPin, LOW);
  delayMicroseconds(3);

  mcp.digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);

  mcp.digitalWrite(triggerPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 25000);

  if (duration == 0) {
    return 999;
  }

  return duration * 0.0343 / 2.0;
}

// =================================
// READ FOUR SENSORS
// =================================

void readSensors() {

  frontDistance = measureDistance(
    FRONT_TRIG, FRONT_ECHO
  );
  delay(30);

  rearDistance = measureDistance(
    REAR_TRIG, REAR_ECHO
  );
  delay(30);

  leftDistance = measureDistance(
    LEFT_TRIG, LEFT_ECHO
  );
  delay(30);

  rightDistance = measureDistance(
    RIGHT_TRIG, RIGHT_ECHO
  );
}

// =================================
// OBSTACLE WARNING
// =================================

bool obstacleDetected() {

  return (
    frontDistance < SAFE_DISTANCE ||
    rearDistance < SAFE_DISTANCE ||
    leftDistance < SAFE_DISTANCE ||
    rightDistance < SAFE_DISTANCE
  );
}

void warningSystem() {

  if (obstacleDetected()) {
    mcp.digitalWrite(BUZZER_PIN, HIGH);
    mcp.digitalWrite(LED_PIN, HIGH);
  }
  else {
    mcp.digitalWrite(BUZZER_PIN, LOW);
    mcp.digitalWrite(LED_PIN, LOW);
  }
}

// =================================
// MOVEMENT SAFETY
// =================================

void checkMovementSafety() {

  bool blocked = false;

  if (currentMovement == 'F' &&
      frontDistance < SAFE_DISTANCE) {
    blocked = true;
  }

  if (currentMovement == 'B' &&
      rearDistance < SAFE_DISTANCE) {
    blocked = true;
  }

  if (currentMovement == 'L' &&
      leftDistance < SAFE_DISTANCE) {
    blocked = true;
  }

  if (currentMovement == 'R' &&
      rightDistance < SAFE_DISTANCE) {
    blocked = true;
  }

  if (blocked) {
    stopMotors();
  }
}

// =================================
// PHONE CONTROL WEBPAGE
// =================================

void handleRoot() {

  String webpage = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <title>Aircraft Inspection Rover</title>

  <style>
  body {
    font-family: Arial;
    text-align: center;
    background: #eeeeee;
  }

  button {
    width: 110px;
    height: 55px;
    margin: 5px;
    font-size: 17px;
  }

  img {
    width: 95%;
    max-width: 500px;
  }
  </style>
  </head>

  <body>

  <h2>AIRCRAFT INSPECTION ROVER</h2>

  <h3>Live Camera View</h3>

  <img id="camera" src="/jpg">

  <script>
  setInterval(function() {
    document.getElementById("camera").src =
      "/jpg?t=" + Date.now();
  }, 700);
  </script>

  <h3>Rover Movement</h3>

  <button onclick="move('F')">FORWARD</button>
  <br>

  <button onclick="move('L')">LEFT</button>
  <button onclick="move('S')">STOP</button>
  <button onclick="move('R')">RIGHT</button>

  <br>
  <button onclick="move('B')">BACKWARD</button>

  <h3>Sensor Readings</h3>

  <p id="readings">Loading...</p>

  <script>
  function move(direction) {
    fetch('/move?d=' + direction);
  }

  setInterval(function() {
    fetch('/sensors')
    .then(response => response.json())
    .then(data => {
      document.getElementById("readings").innerHTML =
        "Front: " + data.front + " cm<br>" +
        "Rear: " + data.rear + " cm<br>" +
        "Left: " + data.left + " cm<br>" +
        "Right: " + data.right + " cm";
    });
  }, 1000);
  </script>

  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", webpage);
}

// =================================
// CAMERA IMAGE
// =================================

void handleCamera() {

  camera_fb_t *fb = esp_camera_fb_get();

  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  WiFiClient client = server.client();

  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(fb->len);
  server.send(200, "image/jpeg", "");

  client.write(fb->buf, fb->len);

  esp_camera_fb_return(fb);
}

// =================================
// PHONE MOVEMENT COMMANDS
// =================================

void handleMovement() {

  if (!server.hasArg("d")) {
    server.send(400, "text/plain", "Missing command");
    return;
  }

  char command = server.arg("d")[0];

  // Stop command is always allowed.
  if (command == 'S') {
    stopMotors();
    server.send(200, "text/plain", "Stopped");
    return;
  }

  // Check the most recent sensor readings.
  if (command == 'F' && frontDistance < SAFE_DISTANCE) {
    stopMotors();
    server.send(200, "text/plain", "Front obstacle");
    return;
  }

  if (command == 'B' && rearDistance < SAFE_DISTANCE) {
    stopMotors();
    server.send(200, "text/plain", "Rear obstacle");
    return;
  }

  if (command == 'L' && leftDistance < SAFE_DISTANCE) {
    stopMotors();
    server.send(200, "text/plain", "Left obstacle");
    return;
  }

  if (command == 'R' && rightDistance < SAFE_DISTANCE) {
    stopMotors();
    server.send(200, "text/plain", "Right obstacle");
    return;
  }

  switch (command) {

    case 'F':
      forward();
      break;

    case 'B':
      backward();
      break;

    case 'L':
      turnLeft();
      break;

    case 'R':
      turnRight();
      break;

    default:
      server.send(400, "text/plain", "Invalid command");
      return;
  }

  server.send(200, "text/plain", "Command received");
}

// =================================
// SENSOR INFORMATION
// =================================

void handleSensors() {

  String json = "{";

  json += "\"front\":" + String(frontDistance, 1) + ",";
  json += "\"rear\":" + String(rearDistance, 1) + ",";
  json += "\"left\":" + String(leftDistance, 1) + ",";
  json += "\"right\":" + String(rightDistance, 1);

  json += "}";

  server.send(200, "application/json", json);
}

// =================================
// CAMERA INITIALIZATION
// =================================

void initializeCamera() {

  camera_config_t config = {};

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t result = esp_camera_init(&config);

  if (result != ESP_OK) {
    Serial.printf("Camera error: 0x%x\n", result);
    return;
  }

  Serial.println("Camera initialized");
}

// =================================
// SETUP
// =================================

void setup() {

  Serial.begin(115200);

  // I2C: SDA GPIO3, SCL GPIO1
  Wire.begin(3, 1);

  // Initialize MCP23017
  if (!mcp.begin_I2C(0x20)) {
    Serial.println("MCP23017 not detected!");
    while (true) {
      delay(1000);
    }
  }

  // Configure motor outputs
  for (int pin = 0; pin <= 9; pin++) {
    mcp.pinMode(pin, OUTPUT);
    mcp.digitalWrite(pin, LOW);
  }

  // Configure echo inputs
  pinMode(FRONT_ECHO, INPUT);
  pinMode(REAR_ECHO, INPUT);
  pinMode(LEFT_ECHO, INPUT);
  pinMode(RIGHT_ECHO, INPUT);

  stopMotors();

  // Initialize camera
  initializeCamera();

  // Start Wi-Fi
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println("Aircraft Rover Wi-Fi started");
  Serial.print("Connect to: ");
  Serial.println(ssid);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Website routes
  server.on("/", handleRoot);
  server.on("/jpg", handleCamera);
  server.on("/move", handleMovement);
  server.on("/sensors", handleSensors);

  server.begin();

  Serial.println("Web server started");
}

// =================================
// MAIN PROGRAM LOOP
// =================================

void loop() {

  server.handleClient();

  if (millis() - lastSensorCheck >= 250) {

    lastSensorCheck = millis();

    readSensors();

    warningSystem();

    checkMovementSafety();
  }

  delay(1);
}