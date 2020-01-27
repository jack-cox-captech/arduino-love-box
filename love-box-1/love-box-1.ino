#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ArduinoMqttClient.h>
#include <Bounce2.h>
#include <WiFi101.h>

#include <SD.h>

// connectivity variables
#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PWD;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = MQTT_BROKER;
const char inTopic[] = "love-box-1";

// display variables

#include <Fonts/FreeMono12pt7b.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

#define OLED_RESET  4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define WHITE SSD1306_WHITE
#define BLACK SSD1306_BLACK

bool blinkState = false;
int loopCount = 0;

const int prPin = 0; // photo resistor pin in A0
const int lightThreshold = 30; // below 30 turns off the display

int lightVal;
bool displayOn;

// message state
String lastMessage = "No Messages Yet";


// power management

unsigned long timeOfLastCheck = 0;
const unsigned long pollTime = 20000;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //while(!Serial); // wait for serial to init de-comment if you want prints to work during setup
  
  Serial.println("Starting setup");
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  displayOn = true;
  
  display.display(); // show splashscreen

  display.setFont(&FreeMono12pt7b);

  timeOfLastCheck = 0; // make sure it polls on the first loop

  // TODO read message from non-volatile storage

  displayMessage(lastMessage);

}

void turnOffDisplay() {
  Serial.println("turning off display");
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  displayOn = false; 
}
void turnOnDisplay() {
  Serial.println("turning on display");
  display.ssd1306_command(SSD1306_DISPLAYON);
  displayOn = true;
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentTime = millis();
  lightVal = analogRead(prPin);
  
  if ((currentTime > (timeOfLastCheck + pollTime)) || (timeOfLastCheck == 0)) { // check every X seconds
    Serial.println("polling for data");
      timeOfLastCheck = currentTime;
      if (WiFi.status() != WL_CONNECTED) {
        WiFiConnect();
      }
    
      if (!mqttClient.connected()) {
        MQTTConnect();
      }
      mqttClient.poll();
    
      lightVal = analogRead(prPin);
    
      if (lightVal > lightThreshold) {
        if (!displayOn) { // turn on the display
          turnOnDisplay();
        }
    
        display.drawPixel(display.width()-1, display.height()-1, (blinkState ? WHITE : BLACK));
        blinkState = (blinkState ? false : true);
      } else {
          if (displayOn) { // turn on the display
            turnOffDisplay();
          }
      }
      display.display();

      //WiFi.end();
  } else {
      if (lightVal > lightThreshold) {
        if (!displayOn) { // turn on the display
          turnOnDisplay();
        }
    
        display.drawPixel(display.width()-1, display.height()-1, (blinkState ? WHITE : BLACK));
        blinkState = (blinkState ? false : true);
      } else {
          if (displayOn) { // turn on the display
            turnOffDisplay();
          }
      }
  }
  
  delay(200);

}

void displayMessage(String message) {
  lastMessage = message;
  display.clearDisplay();
  display.setCursor(0,14);
  display.setTextSize(1);
  display.setTextWrap(true);
  display.setTextColor(WHITE, BLACK);
  display.print(lastMessage);
}

void WiFiConnect() {
  // attempt to connect to WiFi network:
  int waitLoop = 0;
  while ( WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    waitLoop = 0;
    while ((WiFi.status() != WL_CONNECTED) && (waitLoop < 9000)) {
      delay(100);
      waitLoop += 100;
    }
    delay(1000);
  }
  Serial.println("WiFi connect complete");
}

void MQTTConnect() {
  mqttClient.setUsernamePassword(MQTT_USERNAME, MQTT_PASSWORD);
  while (!mqttClient.connect(broker, MQTT_PORT)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    // sleep 10 seconds and try again
    delay(10000);
  }

  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);

  mqttClient.subscribe(inTopic, 0);

  Serial.println("MQTT connect complete");
}


void processMqttMessage(String topic, String message) {
    if (topic == "love-box-1") {
      displayMessage(message);
    }
}

void onMqttMessage(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.print("Received a message with topic '");
  String topic = mqttClient.messageTopic();
  Serial.print(topic);
  Serial.print("', duplicate = ");
  Serial.print(mqttClient.messageDup() ? "true" : "false");
  Serial.print(", QoS = ");
  Serial.print(mqttClient.messageQoS());
  Serial.print(", retained = ");
  Serial.print(mqttClient.messageRetain() ? "true" : "false");
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  // use the Stream interface to print the contents
  String contents = "";
  while (mqttClient.available()) {
    char charread = mqttClient.read();
    contents += charread;
    Serial.print((char)charread);
  }

  Serial.println();
  processMqttMessage(topic, contents);
  Serial.println();

}

// Storage

void openStorage() {

  
}
