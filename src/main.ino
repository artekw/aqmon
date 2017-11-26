/*

aqmon - air quality monitor using PMS3003

MQTT topics:
/<mqttNode>/status
/<mqttNode>/mode
/<mqttNode>/air/pm25
/<mqttNode>/air/pm10
/<mqttNode>/timestamp

*/

#include <Arduino.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "SparkFunHTU21D.h"

#include "config.h"

#define LEN 23
#define SET_PIN D1

const unsigned long czasuspienia = 300; //czas pomiedzy pomiarami w sekundach
const unsigned long CZAS_START = 15000; // czas od wlaczenia do startu pomiaru [ms]
const unsigned long CZAS_PAUZA = 3000;  // pauza pomiedzy powtorzeniami pomiaru [ms]

unsigned char buf[LEN];

float t = 0;
float h = 0;
float divider25 = 0; 
float divider10 = 0;

//static int PM01Val[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int PM25Val[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int PM10Val[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//static int PMa01Val[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int PMa25Val[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int PMa10Val[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//static int tPM01Val = 0;
static int tPM25Val = 0;
static int tPM10Val = 0;
//static int tPMa01Val = 0;
static int tPMa25Val = 0;
static int tPMa10Val = 0;

static unsigned long licznikczasu;
static int licznik;

float a25 = 1.00123;
float b25 = 1.96141;
float c25 = 1;
float a10 = 1.15866;
float b10 = 3.16930;
float c10 = 0.7;


WiFiClient espClient;
PubSubClient mqttClient(espClient);
HTU21D myHumidity;

char sprawdzSume(unsigned char *buf, char leng) //sprawdzanie sumy kontrolnej z PMS1003
{
    char receiveflag = 0;
    int receiveSum = 0;
    for (int i = 0; i < (leng - 2); i++)
    {
        receiveSum = receiveSum + buf[i];
    }
    receiveSum = receiveSum + 0x42;
    if (receiveSum == ((buf[leng - 2] << 8) + buf[leng - 1]))
    {
        receiveSum = 0;
        receiveflag = 1;
    }
    return receiveflag;
}


void sortujint(int tablica[], int rozmiar) {
  for (int i = 0; i < (rozmiar - 1); i++) {
    for (int j = 0; j < (rozmiar - (i + 1)); j++) {
      if (tablica[j] > tablica[j + 1]) {
        int t = tablica[j];
        tablica[j] = tablica[j + 1];
        tablica[j + 1] = t;
      }
    }
  }
}


void oblicz()
{
    float divider25 = 0;
    float divider10 = 0;

    //sortujint(PM01Val, 10);
    sortujint(PM25Val, 10);
    sortujint(PM10Val, 10);
    //sortujint(PMa01Val, 10);
    sortujint(PMa25Val, 10);
    sortujint(PMa10Val, 10);

    // obliczanie sredniej z 6 pomiarów
    //int d1 = (PM01Val[2] + PM01Val[3] + PM01Val[4] + PM01Val[5] + PM01Val[6] + PM01Val[7]) / 6;
    int d2 = (PM25Val[2] + PM25Val[3] + PM25Val[4] + PM25Val[5] + PM25Val[6] + PM25Val[7]) / 6;
    int d3 = (PM10Val[2] + PM10Val[3] + PM10Val[4] + PM10Val[5] + PM10Val[6] + PM10Val[7]) / 6;
    //int d4 = (PMa01Val[2] + PMa01Val[3] + PMa01Val[4] + PMa01Val[5] + PMa01Val[6] + PMa01Val[7]) / 6;
    int d5 = (PMa25Val[2] + PMa25Val[3] + PMa25Val[4] + PMa25Val[5] + PMa25Val[6] + PMa25Val[7]) / 6;
    int d6 = (PMa10Val[2] + PMa10Val[3] + PMa10Val[4] + PMa10Val[5] + PMa10Val[6] + PMa10Val[7]) / 6;

    float d7 = h;
    float d8 = t;

    // pył zawieszony po przeliczeniach z uwzględnieniem wilgotności
    divider10 = c10 + a10 * pow(((h) / 100), b10);
    int d9 = d6 / divider10;

    divider25 = c25 + a25 * pow(((h) / 100), b25);
    int d10 = d5 / divider25;

    wyslijPomiar(d2, d3, d5, d6, d7, d8, d9, d10);
}


void transmisja(unsigned char *thebuf) //odczyt aktualnych wartosci z PMS1003
{
    //tPM01Val = ((thebuf[3] << 8) + thebuf[4]);
    tPM25Val = ((thebuf[5] << 8) + thebuf[6]);
    tPM10Val = ((thebuf[7] << 8) + thebuf[8]);
    //tPMa01Val = ((thebuf[9] << 8) + thebuf[10]);
    tPMa25Val = ((thebuf[11] << 8) + thebuf[12]);
    tPMa10Val = ((thebuf[13] << 8) + thebuf[14]);
}


void dodajpomiar(int i)
{
    //PM01Val[i] = tPM01Val;
    PM25Val[i] = tPM25Val;
    PM10Val[i] = tPM10Val;
    //PMa01Val[i] = tPMa01Val;
    PMa25Val[i] = tPMa25Val;
    PMa10Val[i] = tPMa10Val;
}


void setup_wifi() {
    WiFi.hostname(mqttNode.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        if (millis() > 120000)
            ESP.restart();
    }
}


void standby_mode() {
    digitalWrite(SET_PIN, LOW);
    mqttClient.publish(mqttMode.c_str(), "standby");
}


void operating_mode() {
    digitalWrite(SET_PIN, HIGH);
    mqttClient.publish(mqttMode.c_str(), "operating");
}


void setup()
{
    Serial.begin(9600);
    Serial.setTimeout(1500);
    pinMode(SET_PIN, OUTPUT);

    setup_wifi();

    // Create server and assign callbacks for MQTT
    mqttClient.setServer(mqttServer, 1883);
    mqttClient.setCallback(mqtt_callback);

    //operating_mode();
    myHumidity.begin();
    Wire.begin(14, 2);
    ArduinoOTA.begin();

    licznikczasu = millis();
    licznik = 0;
}


// Handle incoming commands from MQTT
void mqtt_callback(char *topic, byte *payload, unsigned int payloadLength)
{
}


void reconnect()
{
    while (!mqttClient.connected())
    {
        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);

        if (mqttClient.connect(clientId.c_str()))
        {
            mqttClient.publish(mqttStatus.c_str(), "connected");
        }
        else
        {
            delay(5000);
        }
    }
}

void wyslijPomiar(int d2, int d3, int d5, int d6, int h, int t, int d9, int d10)
{
    mqttClient.publish(mqttPM25cf.c_str(), String(d2).c_str());
    mqttClient.publish(mqttPM10cf.c_str(), String(d3).c_str());
    
    mqttClient.publish(mqttPM25.c_str(), String(d5).c_str());
    mqttClient.publish(mqttPM10.c_str(), String(d6).c_str());

    mqttClient.publish(mqttPM25h.c_str(), String(d10).c_str());
    mqttClient.publish(mqttPM10h.c_str(), String(d9).c_str());

    mqttClient.publish(mqttHumi.c_str(), String(h).c_str());
    mqttClient.publish(mqttTemp.c_str(), String(t).c_str());
}


void loop()
    {
        //tPM01Val = 0; //tymczasowe dane dla pojedynczego odczytu
        tPM25Val = 0;
        tPM10Val = 0;
        //tPMa01Val = 0;
        tPMa25Val = 0;
        tPMa10Val = 0;

        // sprawdz pol. WiFi
        if (WiFi.status() != WL_CONNECTED)
        {
            setup_wifi();
        }

        if (!mqttClient.connected())
        {
            reconnect();
        }

        // MQTT client loop
        if (mqttClient.connected())
        {
            mqttClient.loop();
        }

        if (Serial.find(0x42))
        { //wykrycie transmisji z PMS3003
            Serial.readBytes(buf, LEN);

            if (buf[0] == 0x4d)
            {

                if (sprawdzSume(buf, LEN))
                {                    //spr. czy suma kontrolna sie zgadza
                    transmisja(buf); // odczyt danych do tymczasowych zmiennych
                }
            }

        } //koniec odczytu 0x42

        if (millis() > 120000) // jesli nie ma reakcji przez 2 minuty - restart
        {
            ESP.deepSleep(czasuspienia * 1000000);
        }

        //zapis pojedynczego pomiaru
        if (millis() > CZAS_START && millis() - licznikczasu >= CZAS_PAUZA && tPMa25Val > 0)
        {
            float wilg = myHumidity.readHumidity();
            float temp = myHumidity.readTemperature();

            if (!isnan(temp))
                t = temp;
            if (!isnan(wilg))
                h = wilg;

            dodajpomiar(licznik); //zapis pojedynczego pomiaru  do tablicy
            licznikczasu = millis();
            licznik++;
        }

        if (licznik >= 10) // po 10 odczytach wyslij dane i zasnij
        {
            oblicz(); //oblicza srednie i wysyla dane
            delay(250);
            //uspienie PMS1003
            //Serial.write(sleep, sizeof(sleep));
            //delay(100);
            standby_mode();

            //uspienie esp8266
            long czasusp = (czasuspienia * 1000) - millis();
            if (czasusp < 10000)
                czasusp = 10000;

            ESP.deepSleep(czasusp * 1000);

        }

        ArduinoOTA.handle();
    }