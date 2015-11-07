#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 0
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "VuonKhe"
#define WLAN_PASS       "bqll1234"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "192.168.100.170"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    ""
#define AIO_KEY         ""

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Store the MQTT server, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
const char PHOTOCELL_FEED[] PROGMEM = AIO_USERNAME "/saytinh/myhome/temp";
Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt, PHOTOCELL_FEED);

// Setup a feed called 'onoff' for subscribing to changes.
const char ONOFF_FEED[] PROGMEM = AIO_USERNAME "/saytinh/myhome/light_temp";
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, ONOFF_FEED);

// Publish connection status to broker
const char LWT_FEED[] PROGMEM = AIO_USERNAME "/saytinh/myhome/light_temp/lwt";
Adafruit_MQTT_Publish lwt = Adafruit_MQTT_Publish(&mqtt, LWT_FEED);

/*************************** Sketch Code ************************************/

void setup() {
  Serial.begin(115200);
  delay(10);

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&onoffbutton);
  
  pinMode(2, OUTPUT);
  // Start up the library
  sensors.begin();
  
  //Set last will statement to broker, retain = 0
  mqtt.will("/saytinh/myhome/light_temp/lwt", "offline", 0, 0);

}

uint32_t x=0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(2000))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Got: "));
      Serial.println((char *)onoffbutton.lastread);
        String rec_data = (char *)onoffbutton.lastread;
        if (rec_data == "ON") 
          digitalWrite(2, HIGH);
        else if (rec_data == "OFF")
          digitalWrite(2, LOW);
        else
          pub_temp();         
    }
  }

  // ping the server to keep the mqtt connection alive
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }

  delay(10);  

}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
  }
  
  lwt.publish("online");
  Serial.println("MQTT Connected!");
}

void pub_temp() {
  sensors.requestTemperatures();
  int rounded_temp = (int)(sensors.getTempCByIndex(0) + 0.5);   //round temp
  Serial.print("getTempCByIndex(0) is: ");
  Serial.println(rounded_temp);
  
  // Now we can publish stuff!
  Serial.print(F("\nSending "));
  Serial.print("...");
  if (! photocell.publish(rounded_temp)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
}

