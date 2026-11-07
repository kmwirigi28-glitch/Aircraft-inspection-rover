 #include <WiFi.h>
#include <WebServer.h>

// Rover Wi-Fi network
const char* ssid = "AircraftRover";
const char* password = "Rover1234";

WebServer server(80);

// L298N motor input pins
const int IN1 = 12;
const int IN2 = 13;
const int IN3 = 14;
const int IN4 = 15;

void stopRover() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void left() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void right() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void showPage() {
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>Aircraft Inspection Rover</title>
    <style>
      body {font-family:Arial;text-align:center;margin:20px}
      button {width:110px;height:55px;margin:6px;font-size:17px}
    </style>
  </head>
  <body>
    <h2>Aircraft Inspection Rover</h2>
    <p>Wireless Rover Control</p>
    <p><button onclick="location.href='/F'">Forward</button></p>
    <button onclick="location.href='/L'">Left</button>
    <button onclick="location.href='/S'">STOP</button>
    <button onclick="location.href='/R'">Right</button>
    <p><button onclick="location.href='/B'">Backward</button></p>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", page);
}

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopRover();

  WiFi.softAP(ssid, password);

  Serial.println("Aircraft Inspection Rover started");
  Serial.print("Connect your phone to: ");
  Serial.println(ssid);
  Serial.print("Open this address: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", showPage);
  server.on("/F", []() {
    forward();
    showPage();
  });
  server.on("/B", []() {
    backward();
    showPage();
  });
  server.on("/L", []() {
    left();
    showPage();
  });
  server.on("/R", []() {
    right();
    showPage();
  });
  server.on("/S", []() {
    stopRover();
    showPage();
  });

  server.onNotFound([]() {
    stopRover();
    showPage();
  });

  server.begin();
}

void loop() {
  server.handleClient();
}