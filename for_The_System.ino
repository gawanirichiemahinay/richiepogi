#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// ========== Pin Definitions ==========
#define moistureSensorPin1 34
#define pumpPin1 14
#define pumpPin2 27
#define waterTankSensorPin 35
#define fertTankSensorPin 32
#define DHT_PIN 33 // Add your DHT11 signal pin here
#define DHT_TYPE DHT11

#define MOIST_PIN moistureSensorPin1
#define PUMP1_PIN pumpPin1
#define PUMP2_PIN pumpPin2
#define WATER_PIN waterTankSensorPin
#define FERT_PIN fertTankSensorPin

// ========== WiFi Credentials ==========
const char* ssid = "tustadong pogi";
const char* password = "healingpa";

WebServer server(80);

// ========== System State ==========
bool pump1Status = false;
bool pump2Status = false;
bool autoMode = true;

int moisture = 0;
int waterLevel = 0;
int fertLevel = 0;
float temperature = 0.0;
float humidity = 0.0;

DHT dht(DHT_PIN, DHT_TYPE);

// ========== File Handler ==========
void handleFile(String path) {
  if (path.endsWith("/")) path += "index.html";
  String contentType = "text/plain";

  if (path.endsWith("index.html")) contentType = "text/html";
  else if (path.endsWith("style.css")) contentType = "text/css";
  else if (path.endsWith("script.js")) contentType = "application/javascript";

  File file = SPIFFS.open(path, "r");
  if (!file) {
    server.send(404, "text/plain", "File Not Found");
    return;
  }

  server.streamFile(file, contentType);
  file.close();
}

// ========== Data Endpoint ==========
void handleData() {
  int raw = analogRead(MOIST_PIN);
  moisture = constrain(map(raw, 1200, 4095, 100, 0), 0, 100);

  if (autoMode) {
    if (moisture < 30) pump1Status = true;
    else if (moisture > 60) pump1Status = false;
    digitalWrite(PUMP1_PIN, pump1Status ? HIGH : LOW);
  }

  waterLevel = constrain(map(analogRead(WATER_PIN), 500, 3000, 0, 100), 0, 100);
  fertLevel  = constrain(map(analogRead(FERT_PIN), 500, 3000, 0, 100), 0, 100);

  // Read DHT11
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  String json = "{";
  json += "\"moisture\":" + String(moisture) + ",";
  json += "\"pump\":" + String(pump1Status ? 1 : 0) + ",";
  json += "\"pump2\":" + String(pump2Status ? 1 : 0) + ",";
  json += "\"autoMode\":" + String(autoMode ? 1 : 0) + ",";
  json += "\"waterTankLevel\":" + String(waterLevel) + ",";
  json += "\"fertTankLevel\":" + String(fertLevel) + ",";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1);
  json += "}";

  server.send(200, "application/json", json);
}

// ========== Control Endpoints ==========
void handleTogglePump() {
  if (!autoMode) {
    pump1Status = !pump1Status;
    digitalWrite(PUMP1_PIN, pump1Status ? HIGH : LOW);
  }
  server.sendHeader("Location", "/", true);
  server.send(303);
}

void handleTogglePump2() {
  pump2Status = !pump2Status;
  digitalWrite(PUMP2_PIN, pump2Status ? HIGH : LOW);
  server.sendHeader("Location", "/", true);
  server.send(303);
}

void handleToggleMode() {
  autoMode = !autoMode;
  if (autoMode) {
    pump1Status = false;
    digitalWrite(PUMP1_PIN, LOW);
  }
  server.sendHeader("Location", "/", true);
  server.send(303);
}

void handleControl() {
  if (server.hasArg("pump")) {
    int val = server.arg("pump").toInt();
    pump1Status = (val == 1);
    digitalWrite(PUMP1_PIN, pump1Status ? HIGH : LOW);
  }
  server.send(200, "text/plain", "OK");
}

// ========== Setup ==========
void setup() {
  Serial.begin(115200);

  pinMode(PUMP1_PIN, OUTPUT); digitalWrite(PUMP1_PIN, LOW);
  pinMode(PUMP2_PIN, OUTPUT); digitalWrite(PUMP2_PIN, LOW);

  dht.begin();

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed!");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  server.on("/",            []() { handleFile("/index.html"); });
  server.on("/style.css",   []() { handleFile("/style.css"); });
  server.on("/script.js",   []() { handleFile("/script.js"); });
  server.on("/data",        handleData);
  server.on("/togglePump",  HTTP_GET, handleTogglePump);
  server.on("/togglePump2", HTTP_GET, handleTogglePump2);
  server.on("/toggleMode",  HTTP_GET, handleToggleMode);
  server.on("/control",     HTTP_GET, handleControl);

  server.begin();
}

// ========== Loop ==========
void loop() {
  server.handleClient();
}
