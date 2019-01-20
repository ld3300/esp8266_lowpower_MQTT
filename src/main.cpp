/***************************************************
  Using Adafruit MQTT Library

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// Libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "private.h"        // store wifi and mqtt credetials here. Add private.h to .gitignore

#define SLEEPTIME 10000    // millis to sleep
#define WIFITIMER 10000     // maximum time to attempt to connect to wifi before going back to sleep

#define DEBUG 1             // comment this to disable serial

uint8_t netIndex = 0;       // will point the the available network in our network array in private.h
uint32_t timer = 0;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClientSecure client;

// Setup for MQTT connection using the credentials that are stored in private.h
// defining an explicit client ID significantly shortens connection time 
Adafruit_MQTT_Client mqtt(&client, MQTT_SERV, MQTT_PORT, MQTT_CLID, MQTT_NAME, MQTT_PASS);   
Adafruit_MQTT_Publish sensorPub = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/f/wifitime", MQTT_QOS_1);
Adafruit_MQTT_Publish totTime = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/f/tottime", MQTT_QOS_1);

void gotoSleep(){
  // Sleep
  #ifdef DEBUG 
      Serial.println("ESP8266 in sleep mode"); 
  Serial.println(millis());
  #endif
  uint32_t thisTime = millis();
  totTime.publish(thisTime);
  ESP.deepSleep(SLEEPTIME * 1000);
}

void MQTT_connect(){
  int8_t ret;                           // for logging connection error
  if (mqtt.connected()) return;         // Stop if already connected
  #ifdef DEBUG
    Serial.print("Connecting to MQTT... ");
    Serial.println();
  #endif
  uint8_t retries = 6;                  // number of tried to connect before giving up
  while ((ret = mqtt.connect()) != 0){  // connect will return 0 for connected
    #ifdef DEBUG
      Serial.print(ret);
      Serial.println(mqtt.connectErrorString(ret));   // Print connection error
    #else
      mqtt.connectErrorString(ret);
    #endif
    mqtt.disconnect();                  // Close connection before trying again
    retries--;
    if (retries == 0){                  // If we run out of retries then go to sleep
      #ifdef DEBUG
        Serial.println("MQTT Connection failed ... going to sleep ...");
      #endif
      gotoSleep();
    }
    #ifdef DEBUG
      Serial.println("Retrying MQTT connection...");
    #endif
    delay(500);                         // short delay between attempts
  }
  #ifdef DEBUG
    Serial.println("MQTT Connected!");
  #endif
}

void valuePublish(uint32_t value){       // We are going to publish our state back in case it is changed locally
  #ifdef DEBUG
    Serial.print(F("\nSending val "));
    Serial.print(value);
    Serial.print("...");
  #endif
    if (! sensorPub.publish(value)) {     // Sending value to MQTT server
    #ifdef DEBUG
      Serial.println(F("Failed"));
    } 
    else {
      Serial.println(F("OK!"));
  #endif
    }
}

void checkWifiPresence(){               // used to check if wifi network exists before trying to connect
  bool wifiFound = false;
  int n = WiFi.scanNetworks();
  if (n == 0){
    #ifdef DEBUG
      Serial.println("no networks found");
    #endif
    gotoSleep();
  }
  else{
    #ifdef DEBUG
      Serial.print(n);
      Serial.println(" networks found");
    #endif
    for (int i = 0; i < n; ++i){
      for(int j = 0; j < NUMMYNETWORKS; j++)
        if(WiFi.SSID(i) == mynetworks[j][0]){                  // Looking for my network
          netIndex = j;
          wifiFound = true;
          #ifdef DEBUG
            Serial.println("Wifi in range");
          #endif
          break;
        }
    }
    if(!wifiFound){
      #ifdef DEBUG
        Serial.println("My wifi isn't in range ... going to sleep ...");
      #endif
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
    delay(100);                               // Delay between wifi connection attempts
    #ifdef DEBUG
      Serial.print(".");
    #endif
    if(millis() >= WIFITIMER){              // If wifi keeps failing go to sleep
      #ifdef DEBUG
        Serial.println("Wifi failed ... going to sleep ...");
      #endif
      gotoSleep();
    }
  }
  timer = millis();
  #ifdef DEBUG
    Serial.println("");
    Serial.print("WiFi connected in (ms): ");
    Serial.println(timer);
    Serial.println(WiFi.localIP());             // print the ip address
  #endif
}

void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
    Serial.println("\nESP8266 in normal mode");   // We booted up properly
  #endif

  checkWifiPresence();                        // Check if wifi is available before trying to connect
  wifiConnect();

  MQTT_connect();                             // attempt to connect to mqtt server
  // uint8_t value = random(255);                // replace me with sensor read
  valuePublish(timer);                        // send our sensor reading to mqtt

  #ifdef DEBUG
    Serial.println();
    Serial.println("closing connection");
  #endif
  gotoSleep();                                // Go To sleep, will start Setup over when wake up
}

void loop() {
  // nothing will happen here because we will reboot after every sleep
}