
// Libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "private.h"        // store wifi and mqtt credetials here. Add private.h to .gitignore

#define SLEEPTIME 600000    // millis to sleep
#define WIFITIMER 10000     // maximum time to attempt to connect to wifi before going back to sleep
#define MQTTATTEMPTS 6      // Maximum tries to conn to MQTT
#define Write true

uint8_t netIndex = 0;       // will point the the available network in our network array in private.h
unsigned long wifitimer = 0;
unsigned long presencetimer = 0;
unsigned long startuptimer = 0;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClientSecure espclient;
// WiFiClient espclient;
PubSubClient client(espclient);
long lastMsg = 0;
char msg[50];
int value = 0;

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

void gotoSleep(){
  // Sleep 
  Serial.println("ESP8266 in sleep mode");
  ESP.deepSleep(SLEEPTIME * 1000);
}

void mqtt_connect() {
  // Loop until we're reconnected or timeout
  while (!client.connected()) {
    static uint8_t attempts = MQTTATTEMPTS;
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLID, MQTT_NAME, MQTT_PASS)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      publish(STATUSTOPIC, "hello world", false);
    } 
    else if(attempts == 0){   // if we hit attempt number go to sleep
      Serial.println("MQTT Connection failed ... going to sleep ...");
      gotoSleep();
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again");
      delay(5000);    // wait a little before trying again
    }
    attempts --;
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
          netIndex = j;
          wifiFound = true;
          Serial.println("Wifi in range");
          presencetimer = millis();
          break;
        }
      }
    }
    if(!wifiFound){
      Serial.println("My wifi isn't in range ... going to sleep ...");
      gotoSleep();
    }
  }
}

void wifiConnect(){
  if(staticIP[netIndex]){
    WiFi.config(ip[netIndex], gateway[netIndex], subnet[netIndex], dns);  // Pull IP info from private.h
  }
  WiFi.begin(mynetworks[netIndex][0], mynetworks[netIndex][1]);               // Connect to wifi (store credentials in private.h)
  while (WiFi.status() != WL_CONNECTED) {     // Keep trying until we connect
    delay(200);                               // Delay between wifi connection checks
    Serial.print(".");
    if(millis() >= WIFITIMER){              // If wifi keeps failing go to sleep
      Serial.println("Wifi failed ... going to sleep ...");
      gotoSleep();
    }
  }
  wifitimer = millis();
  Serial.println("");
  Serial.print("WiFi connected in (ms): ");
  Serial.println(wifitimer);
  Serial.println(WiFi.localIP());             // print the ip address
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP8266 in normal mode");   // We booted up properly
  client.setServer(MQTT_SERV, MQTT_PORT);
  startuptimer = millis();
  checkWifiPresence();                        // Check if wifi is available before trying to connect
  wifiConnect();
  // delay(500);         // let wifi settle
  mqtt_connect();     // attempt to connect to mqtt server
  publish(STARTUP, startuptimer, Write);
  publish(WIFIPRESENCE, presencetimer, Write);
  publish(WIFITIME, wifitimer, Write);
  // uint8_t value = random(255);                // replace me with sensor read
  // longPublish(value, "feed/feed");                      // send our sensor reading to mqtt
  unsigned long totaltimer = millis();
  totaltimer += 200;                  // to account for the later delay
  publish(TOTALTIME, totaltimer, Write);
  client.loop();              // push data out
  delay(200);                 // without delay the radio shuts down before buffer fully sent
  Serial.println();
  Serial.println("closing connection");
  gotoSleep();       // Go To sleep, will start Setup over when wake up
}

void loop() {
  // nothing will happen here because we will reboot after every sleep
}