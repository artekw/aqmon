
#ifndef CONFIG_H
#define CONFIG_H

// ilość pomiarów jakości powiekrza PM25 i PM10
int numReadings = 5;

// SSID i hasło do sieci WiFi
const char *ssid = "xxx";
const char *password = "xxx";

// adres serwera MQTT
const char *mqttServer = "xxx";
// nazwa punktu pomiarowego
const String mqttNode = "aqmon";


const String mqttStatus = "/" + mqttNode + "/status";
const String mqttMode = "/" + mqttNode + "/mode";
const String mqttPM25cf = "/" + mqttNode + "/pm25cf";
const String mqttPM10cf = "/" + mqttNode + "/pm10cf";
const String mqttPM25 = "/" + mqttNode + "/pm25";
const String mqttPM10 = "/" + mqttNode + "/pm10";
const String mqttPM25h = "/" + mqttNode + "/pm25h";
const String mqttPM10h = "/" + mqttNode + "/pm10h";
const String mqttHumi = "/" + mqttNode + "/humidity";
const String mqttTemp = "/" + mqttNode + "/temperature";
const String mqttPress = "/" + mqttNode + "/pressure";
#endif //CONFIG_H