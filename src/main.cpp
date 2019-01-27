/* DHTServer - ESP8266 Webserver with a DHT sensor as an input
 
   Based on ESP8266Webserver, DHTexample, and BlinkWithoutDelay (thank you)
 
   Version 1.0  5/3/2014  Version 1.0   Mike Barela for Adafruit Industries
*/

// Libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>       // MQTT library
#include <ArduinoJson.h>        // Create JSON packets for beebotte
#include <DHTesp.h>
#include "private.h"            // store wifi and mqtt credetials here. Add private.h to .gitignore

#ifdef ESP32
#pragma message(Change Code for ESP32!)
#error Select ESP8266 board.
#endif

#define SLEEPTIME     10000     // millis to sleep
#define WIFITIMER     10000     // maximum time to attempt to connect to wifi before going back to sleep
#define MQTTATTEMPTS  12        // Maximum tries to conn to MQTT
#define DHTTYPE       DHT22
#define DHTPIN        2
#define DHTPOWER      14        // Pin to power DHT22 (Allowing it to power off between readings)
#define DHTDELAY      1000      // Amount of time after power on to make sure sensor settles


#define WRITE         true
#define NOWRITE       false

uint8_t wifiIndex = 0;       // will point the the available network in our network array in private.h
unsigned long wifitimer;
unsigned long presencetimer;
unsigned long startuptimer;
unsigned long dhtStartTime;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
// WiFiClientSecure espclient;
WiFiClient espclient;
PubSubClient client(espclient);

DHTesp dht;

void onMessage(char* topic, byte* payload, unsigned int length) {
  // decode the JSON payload
  StaticJsonBuffer<128> jsonInBuffer;
  JsonObject& root = jsonInBuffer.parseObject(payload);

  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  // led resource is a boolean read it accordingly
  bool data = root["data"];

  // Set the led pin to high or low
  Serial.println(data);

  // Print the received value to serial monitor for debugging
  Serial.print("Received message of length ");
  Serial.print(length);
  Serial.println();
  Serial.print("data ");
  Serial.print(data);
  Serial.println();
}

void publish(const char* resource, char* data, bool persist){
    StaticJsonBuffer<128> jsonOutBuffer;
    JsonObject& root = jsonOutBuffer.createObject();
    root["channel"] = MQTT_CHAN;
    root["resource"] = resource;
    if (persist) {
        root["write"] = true;
    }
    root["data"] = data;

    // Now print the JSON into a char buffer
    char buffer[128];
    root.printTo(buffer, sizeof(buffer));

    // Create the topic to publish to
    char topic[64];
    sprintf(topic, "%s/%s", MQTT_CHAN, resource);

    // Now publish the char buffer to Beebotte
    client.publish(topic, buffer);
}
void publish(const char* resource, float data, bool persist){
    StaticJsonBuffer<128> jsonOutBuffer;
    JsonObject& root = jsonOutBuffer.createObject();
    root["channel"] = MQTT_CHAN;
    root["resource"] = resource;
    if (persist) {
        root["write"] = true;
    }
    root["data"] = data;

    // Now print the JSON into a char buffer
    char buffer[128];
    root.printTo(buffer, sizeof(buffer));

    // Create the topic to publish to
    char topic[64];
    sprintf(topic, "%s/%s", MQTT_CHAN, resource);

    // Now publish the char buffer to Beebotte
    client.publish(topic, buffer);
}
void publish(const char* resource, unsigned long data, bool persist){
    StaticJsonBuffer<128> jsonOutBuffer;
    JsonObject& root = jsonOutBuffer.createObject();
    root["channel"] = MQTT_CHAN;
    root["resource"] = resource;
    if (persist) {
        root["write"] = true;
    }
    root["data"] = data;

    // Now print the JSON into a char buffer
    char buffer[128];
    root.printTo(buffer, sizeof(buffer));

    // Create the topic to publish to
    char topic[64];
    sprintf(topic, "%s/%s", MQTT_CHAN, resource);

    // Now publish the char buffer to Beebotte
    client.publish(topic, buffer);
}
void publish(const char* resource, uint32_t data, bool persist){
    StaticJsonBuffer<128> jsonOutBuffer;
    JsonObject& root = jsonOutBuffer.createObject();
    root["channel"] = MQTT_CHAN;
    root["resource"] = resource;
    if (persist) {
        root["write"] = true;
    }
    root["data"] = data;

    // Now print the JSON into a char buffer
    char buffer[128];
    root.printTo(buffer, sizeof(buffer));

    // Create the topic to publish to
    char topic[64];
    sprintf(topic, "%s/%s", MQTT_CHAN, resource);

    // Now publish the char buffer to Beebotte
    client.publish(topic, buffer);
}

void gotoSleep(){
  // Sleep 
  Serial.println("ESP8266 in sleep mode...zzz");
  ESP.deepSleep(SLEEPTIME * 1000);
}

void mqtt_connect() {
  // Loop until we're reconnected or timeout
  uint8_t attempts = MQTTATTEMPTS;
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLID, MQTT_NAME, MQTT_PASS)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      publish(STATUSTOPIC, "hello world", NOWRITE);
    } 
    else {
      attempts --;
      if(attempts == 0){   // if we hit attempt number go to sleep
        Serial.print("MQTT Connection failed ... going to sleep ... Time Awake: ");
        Serial.println(millis());
        gotoSleep();
      }
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again");
      delay(1000);    // wait a little before trying again
    }
  }
}

void checkWifiPresence(){               // used to check if wifi network exists before trying to connect
  bool wifiFound = false;
  int n = WiFi.scanNetworks();
  if (n == 0){
    Serial.println("no networks found");
    gotoSleep();
  }
  else{
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i){
      for(int j = 0; j < NUMMYNETWORKS; j++){
        if(WiFi.SSID(i) == mynetworks[j][0]){                  // Looking for my network
          wifiIndex = j;
          wifiFound = true;
          Serial.println("Wifi in range");
          presencetimer = millis();
          break;
        }
      }
      if(wifiFound) break;
    }
    if(!wifiFound){
      Serial.println("My wifi isn't in range ... going to sleep ...");
      gotoSleep();
    }
  }
}

void wifiConnect(){
  unsigned long wifiStart = millis();
  if(staticIP[wifiIndex]){
    WiFi.config(ip[wifiIndex], gateway[wifiIndex], subnet[wifiIndex], gateway[wifiIndex]);  // Pull IP info from private.h
  }
  WiFi.begin(mynetworks[wifiIndex][0], mynetworks[wifiIndex][1]);               // Connect to wifi (store credentials in private.h)
  while (WiFi.status() != WL_CONNECTED) {     // Keep trying until we connect
    delay(200);                               // Delay between wifi connection checks
    Serial.print(".");
    if(millis() - wifiStart >= WIFITIMER){              // If wifi keeps failing go to sleep
      Serial.print("Wifi failed ... going to sleep ...  Time awake: ");
      Serial.println(millis());
      gotoSleep();
    }
  }
  wifitimer = millis();
  Serial.println("");
  Serial.print("WiFi connected in (ms): ");
  Serial.println(wifitimer);
  Serial.println(WiFi.localIP());             // print the ip address
}

void getTemperature(){
  while(!(millis() - dhtStartTime >= dht.getMinimumSamplingPeriod())){
    Serial.println("Waiting for sensor to settle");
    delay(200);
  }
  float humidity = dht.getHumidity();
  float temperature = dht.toFahrenheit(dht.getTemperature());
  if(!dht.getStatus()){       // Publish to server if we don't get any read errors
    publish(FAHRENHEIT, temperature, WRITE);
    publish(HUMIDITY, humidity, WRITE);
    publish(HEATINDEX, dht.computeHeatIndex(temperature, humidity, true), WRITE);
  }
  Serial.println();
  Serial.println("Status\tHumidity (%)\tTemperature (F)\tHeatIndex (F)");
  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temperature, 1);
  Serial.print("\t\t");
  Serial.println(dht.computeHeatIndex(temperature, humidity, true), 1);
}

void setup() {
  Serial.begin(115200);
  startuptimer = millis();
  Serial.println("\n\nESP8266 in normal mode");   // We booted up properly

  client.setServer(MQTT_SERV, MQTT_PORT);     // Set the server and port for connection
  checkWifiPresence();                        // Check if wifi is available before trying to connect
  // dht.setup(14, DHTesp::DHT11);                // Connect DHT sensor to GPIO 2
  // dhtStartTime = millis();                    // When sensor powered on so we can wait for settle in case wifi too fast
  // pinMode(DHTPOWER, OUTPUT);                  // Set pin for powering sensor to output (as a sink)
  // digitalWrite(DHTPOWER, LOW);
  wifiConnect();                              // attempt to connect to wifi
  mqtt_connect();                             // attempt to connect to mqtt server

  // getTemperature();                           // Now that we are connected get and publish sensor readings
  // digitalWrite(DHTPOWER, HIGH);               // Turn off DHT
  publish(WIFITIME, wifitimer, WRITE);        // Publish how long wifi connection took 
  unsigned long totaltimer = millis();        // How long were we awake?
  totaltimer += 200;                          // to account for the delay to send publish results
  publish(TOTALTIME, totaltimer, WRITE);      // Publish the total on time
  Serial.print("Total time awake: ");
  Serial.println(totaltimer);

  client.loop();              // push data out
  delay(200);                 // without delay the radio shuts down before MQTT buffer fully sent

  Serial.println();
  Serial.println("closing connection");
  gotoSleep();       // Go To sleep, will start Setup over when wake up
}

void loop() {
  // nothing will happen here because we will reboot after every sleep
}