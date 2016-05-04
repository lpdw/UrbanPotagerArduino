#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "FS.h"

const char* ssid = "LPDW-IOT";
const char* password = "LPDWIOTROUTER2015";

double humidity;
double temp;
double luminosity;

ESP8266WebServer server(80);

bool readConfig(){
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }
  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }

  humidity = json["humidity"];
  temp = json["temp"];
  luminosity = json["luminosity"];

  configFile.close();
  return true;
}

void setServer(){
  server.on("/", HTTP_GET, handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void handleRoot(){
  server.send(200, "text/plain", "Bonjour");
}

bool saveConfig(){
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  
  json["humidity"] = humidity;
  json["temp"] = temp;
  json["luminosity"] = luminosity;
  
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFile);
  Serial.println("Config Done");
  return true;
}

void setWifiClient(){
  Serial.println("Wait for connexion");
  delay(100);
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  // Wait for connexion
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to: ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  setWifiClient();
}

void loop() {
  
}
