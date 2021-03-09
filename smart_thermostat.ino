//Program to manage the servo running the thermostat dial to make it 'smart'
//Also supports an OLED with the current thermostat value, and a + and - button

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include "Servo.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET -1  //
Adafruit_SSD1306 OLED(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//Wifi and MQTT vars
const char* ssid = "***";
const char* password =  "***";
const char* mqttServer = "192.168.1.xx";
const int mqttPort = 1883;
int holding_var = 0;
String messageTemp;
int debounce1 = 0;
int debounce2 = 0;
int temp_c;

int servo_pin = D7;
int btn1_pin = D5;
int btn2_pin = D6;

Servo myservo;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
Serial.begin(9600);
//Networking Stuff Start ===========================>
WiFi.mode(WIFI_STA);
WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
   WiFi.hostname("ESP_Thermostat");
client.setServer(mqttServer, mqttPort);
client.setCallback(callback);
 while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 // Create a random client ID
    String clientId = "Thermostat_";
    clientId += String(random(0xffff), HEX);
if (client.connect(clientId.c_str())) {
Serial.println("connected");
 } else {
 Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
      }
  }
//OTA stuff --
ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
  Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //end of OTA stuff ---
 client.publish("esp/test", "Hello from Thermostat"); //handshake
 //subscribe to topic
client.subscribe("esp/thermostat");
//Netoworking Stuff End ===============================<

//OLED setup
  OLED.begin();
  OLED.display();
//oled hello
  OLED.clearDisplay(); // Clear the display buffer
  OLED.display();
  OLED.setRotation(2);
  OLED.setTextSize(2);
  OLED.setTextColor(WHITE);
  OLED.setCursor(0,0);
  OLED.println("WELCOME");
  OLED.display();

myservo.attach(servo_pin);
pinMode(btn1_pin,INPUT);
pinMode(btn2_pin,INPUT);
}

void loop() {

//Network Stuff =============>
ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
  }
//End of Network Stuff ======<
//Temp +++ Button
if(digitalRead(btn1_pin) == 1){
  //2 sec debug
  Serial.println("B1 pressed");
  if (millis() - 2500 > debounce1){
      client.publish("esp/thermo_btn", "UP");
      debounce1 = millis();
  }
}
//Temp --- Button
if(digitalRead(btn2_pin) == 1){
    Serial.println("B2 pressed");
  //2 sec debug
  if (millis() - 2500 > debounce2){
   //   client.publish("esp/thermo_btn", "DOWN");
      debounce2 = millis();
  }
}


client.loop();
delay(1);
} //end of loop

//Functions ==================================

//reconnect function for MQTT Dropout
void reconnect() {
  Serial.println("Reconnect activated");
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");


// Create a random client ID
    String clientId = "Thermostat_";
    clientId += String(random(0xffff), HEX);
if (client.connect(clientId.c_str())) {
      Serial.println("connected");
       delay(1000);
        client.subscribe("esp/thermostat");
        client.publish("esp/test", "Hello from Thermostat(recon)");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
//Callback function for recieving messages and sending to servo ------>
void callback(char* topic, byte* payload, unsigned int length) {
 //clear messageTemp
  messageTemp = "";
  //get the message, write to messageTemp char array
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  Serial.println(messageTemp);
  //send value to servo if in range
  if (messageTemp.toInt() > 0 && messageTemp.toInt() < 181){
   myservo.write(messageTemp.toInt());
  }
  //update onscreen temp
  temp_c = messageTemp.toInt();
  temp_c = (temp_c-48)/9+15;
  print2screen(String(temp_c));
}

//end of callback ----------------------------------------->

void print2screen(String x) {
  OLED.clearDisplay(); // Clear the display buffer
 // OLED.display();
  OLED.setTextSize(1);
  OLED.setCursor(0,0);
  OLED.print("Thermostat: ");
  OLED.setTextSize(2);
  OLED.setCursor(35,15);
  OLED.print(x);
  OLED.display();

}
