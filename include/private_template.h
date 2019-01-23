// Template file. Rename to private.h and add to gitignore to avoid commiting to github

#define MQTT_SERV     "myserver"
#define MQTT_PORT     8883                  // 1883 or for SSL use 8883
#define MQTT_NAME     "myusername"
#define MQTT_PASS     "mypasswordorkey"
#define MQTT_CLID     "myclientid"          // must be unique per device 1-23 characters

#define MQTT_CHAN     "test"
#define STATUSTOPIC   "status"              // define the MQTT topics
#define TOTALTIME     "totaltime"
#define WIFITIME      "wifitime"
#define WIFIPRESENCE  "presencetime"
#define STARTUP       "begintime"

#define NUMMYNETWORKS 2                     // number of networks in mynetworks array
const char* mynetworks [][5] = {            // {SSID,Password}
              {"SSID1","PASSWORD1"},
              {"SSID2","PASSWORD2"}};
const bool staticIP [] = {true, false};    // for each network do we have static IP?
IPAddress ip[] = { IPAddress(192,168,1,51), IPAddress(0,0,0,0)};
IPAddress gateway[] = { IPAddress(192,168,1,1), IPAddress(0,0,0,0)};
IPAddress subnet[] = { IPAddress(255,255,255,0), IPAddress(0,0,0,0)};
IPAddress dns(8,8,8,8);
