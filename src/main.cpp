
// Libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <fs.h>             // recall status for wakeups
#include "private.h"        // store wifi and mqtt credetials here. Add private.h to .gitignore

#define SLEEPTIME 600000     // millis to sleep between mqtt keepalive
#define POSTMINUTES 30       // Number of minutes between posts
#define WIFITIMER 7000     // maximum time to attempt to connect to wifi before going back to sleep
#define MQTTTIMER 47000  //4 //40000 //3     // Maximum time to conn to MQTT
#define Write true

uint8_t netIndex = 0;       // will point the the available network in our network array in private.h

#define deb Serial.println("\nhere");

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClientSecure espclient;
// WiFiClient espclient;
PubSubClient client(espclient);
const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";      // for random ID
char id[17];

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
  Serial.println("ESP8266 in sleep mode\n");
  ESP.deepSleep(SLEEPTIME * 1000);
}

const char * generateID(){
  randomSeed(analogRead(0));
  int i = 0;
  for(i = 0; i < sizeof(id) - 1; i++) {
    id[i] = chars[random(sizeof(chars))];
  }
  id[sizeof(id) -1] = '\0';

  return id;
}

void mqtt_connect() {
  // Loop until we're reconnected or timeout
  while (!client.connected()) {
    static unsigned long startTime = millis();
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // if (client.connect(MQTT_CLID, MQTT_NAME, MQTT_PASS)) {
    if (client.connect(generateID(), MQTT_NAME, MQTT_PASS)) {
      Serial.print("connected in: ");
      Serial.println(millis() - startTime);
      // Once connected, publish an announcement...
      publish(STATUSTOPIC, "helloworld", false);
    } 
    else if((unsigned long)(millis() - startTime) > MQTTTIMER){   // if we hit attempt number go to sleep
      Serial.println("MQTT Connection timeout ... going to sleep ...");
      gotoSleep();
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again");
      delay(5000);    // wait a little before trying again
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
  if(staticIP[netIndex]){
    WiFi.config(ip[netIndex], gateway[netIndex], subnet[netIndex], dns);  // Pull IP info from private.h
  }
  WiFi.begin(mynetworks[netIndex][0], mynetworks[netIndex][1]);               // Connect to wifi (store credentials in private.h)
  while (WiFi.status() != WL_CONNECTED) {     // Keep trying until we connect
    delay(200);                               // Delay between wifi connection checks
    Serial.print(".");
    unsigned long thisTime = millis();
    if((unsigned long)(thisTime - startTime) > WIFITIMER){              // If wifi keeps failing go to sleep
      Serial.println("Wifi timed out ... going to sleep ...");
      gotoSleep();
    }
  }
  Serial.println("");
  Serial.print("WiFi connected in (ms): ");
  Serial.println(millis() - startTime);
  Serial.println(WiFi.localIP());             // print the ip address
}

void setup() {
  Serial.begin(115200);
  // client.loop();                                          // try to keep our connection alive
  unsigned long startTime = millis();
  // uint32_t numLoops = (POSTMINUTES*60*1000)/SLEEPTIME;       // figure out how many loops we need
  // char charCounter[15];
  // SPIFFS.begin();
  // File f = SPIFFS.open("/count.txt", "r");
  // if(f){
  //   f.readBytes(charCounter, sizeof(numLoops));
  //   f.close();
  //   f = SPIFFS.open("/count.txt", "w");
  //   uint32_t counter;
  //   if(charCounter[0]) counter = atoi(charCounter);
  //   Serial.print("\n\n\nCountdown: ");
  //   Serial.println(counter); 
  //   if(counter){                    // if still numbers in counter go back to sleep
  //     counter --;
  //     f.print(counter);
  //     f.close();
  //     Serial.print("awake millis: ");
  //     Serial.println(millis());
  //     delay(10);
  //     gotoSleep();
  //   }
  //   else{
  //     f.print(numLoops);
  //     f.close();
  //   }
  // }
  // else{
  //   SPIFFS.format();
  //   f = SPIFFS.open("/count.txt", "w");
  //   if(f) Serial.println("File created");
  //   f.print(numLoops);
  //   f.close();
  //   delay(10);
  //   gotoSleep();
  // }
  Serial.println("\nESP8266 in normal mode");   // We booted up properly
  client.setServer(MQTT_SERV, MQTT_PORT);
  checkWifiPresence();                        // Check if wifi is available before trying to connect
  wifiConnect();
  delay(5000);         // let wifi settle
  mqtt_connect();     // attempt to connect to mqtt server
  // uint8_t value = random(255);                // replace me with sensor read
  // longPublish(value, "feed/feed");                      // send our sensor reading to mqtt
  unsigned long totaltimer = (unsigned long)(millis() - startTime + 200);
  publish(TOTALTIME, totaltimer, Write);
  client.loop();              // push data out
  delay(200);                 // without delay the radio shuts down before buffer fully sent
  Serial.println();
  Serial.print("closing connection time: ");
  Serial.println(totaltimer);
  gotoSleep();       // Go To sleep, will start Setup over when wake up
}

void loop() {
  // nothing will happen here because we will reboot after every sleep
}