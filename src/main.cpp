
// Libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "private.h"        // store wifi and mqtt credetials here. Add private.h to .gitignore

#define SLEEPTIME 600000     // millis to sleep between mqtt keepalive
#define POSTMINUTES 30       // Number of minutes between posts
#define WIFITIMER 7000     // maximum time to attempt to connect to wifi before going back to sleep
#define MQTTTIMER 47000  //4 //40000 //3     // Maximum time to conn to MQTT
#define Write true

uint8_t netIndex = 0;       // will point to the available network in our network array in private.h



const char * headerKeys[] = {"date", "server"};
const size_t numberOfHeaders = 2;




// Create an ESP8266 WiFiClient class to connect to the MQTT server.
// WiFiClientSecure espclient;
// WiFiClient espclient;

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
    // client.publish(topic, buffer);
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
    // client.publish(topic, buffer);
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
    // client.publish(topic, buffer);
}

void gotoSleep(){
  // Sleep 
  Serial.println("ESP8266 in sleep mode\n");
  ESP.deepSleep(SLEEPTIME * 1000);
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
  static unsigned long startTime = millis();
  // if(staticIP[netIndex]){
  //   WiFi.config(ip[netIndex], gateway[netIndex], subnet[netIndex], dns);  // Pull IP info from private.h
  // }
  WiFi.begin(mynetworks[netIndex][0], mynetworks[netIndex][1]);               // Connect to wifi (store credentials in private.h)
  while (WiFi.status() != WL_CONNECTED) {     // Keep trying until we connect
    delay(500);                               // Delay between wifi connection checks
    Serial.print(".");
    unsigned long thisTime = millis();
    // if((unsigned long)(thisTime - startTime) > WIFITIMER){              // If wifi keeps failing go to sleep
    //   Serial.println("Wifi timed out ... going to sleep ...");
    //   gotoSleep();
    // }
  }
  Serial.println("");
  Serial.print("WiFi connected in (ms): ");
  Serial.println(millis() - startTime);
  Serial.println(WiFi.localIP());             // print the ip address
}

void setup() {
  Serial.begin(115200);                                   // try to keep our connection alive
  unsigned long startTime = millis();
  Serial.println("\nESP8266 in normal mode");   // We booted up properly
  // checkWifiPresence();                        // Check if wifi is available before trying to connect
  wifiConnect();
  unsigned long totaltimer = (unsigned long)(millis() - startTime);
  publish(TOTALTIME, totaltimer, Write);
  // Serial.println();
  // Serial.print("closing connection time: ");
  // Serial.println(totaltimer);
  // gotoSleep();       // Go To sleep, will start Setup over when wake up
}

void loop() {
  StaticJsonBuffer<128> jsonOutBuffer;
  JsonObject& root = jsonOutBuffer.createObject();
  // root["channel"] = MQTT_CHAN;
  // root["resource"] = STARTUP;
  root["write"] = true;
  root["data"] = millis();

  // Now print the JSON into a char buffer
  char buffer[128];
  root.printTo(buffer, sizeof(buffer));
  // Serial.println(buffer);

  HTTPClient http;
  http.begin("https://webhook.site/cdf62ff1-9229-441a-8c75-309b83865982/", true); //Specify destination for HTTP request
  http.addHeader("Content-Type", "text/plain");               
  int httpCode = http.POST("test");
  String payload = http.getString();
  Serial.println(payload);



Serial.print("http result:");
Serial.println(httpCode);
http.end();

  delay(20000);
}