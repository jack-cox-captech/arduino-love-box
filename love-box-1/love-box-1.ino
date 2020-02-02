
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_LEDBackpack.h>

#include <ArduinoMqttClient.h>
#include <Bounce2.h>
#include <WiFi101.h>

#include <SD.h>

#include "constants.h"
#include "messages.h"

//#define DEBUG   1

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

#include <Fonts/FreeMono9pt7b.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

#define OLED_RESET  4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_8x8matrix frontMatrix = Adafruit_8x8matrix();

#define DS_NOMSGS 0
#define DS_UNREAD 1
#define DS_READ   2




#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define WHITE SSD1306_WHITE
#define BLACK SSD1306_BLACK

bool blinkState = false;
int loopCount = 0;

const int prPin = 0; // photo resistor pin in A0
const int lightThreshold = 20; // below this turns off the display
#define LID_OPEN    1
#define LID_CLOSED  2
int currentLidState = 0;

int lightVal;
bool displayOn;
  
// message state
String lastMessage = "No Messages Yet";
#define eeprom 0x50
MessageList messageList = MessageList();


// power management

unsigned long timeOfLastCheck = 0;
const unsigned long pollTime = 5000; // 5 seconds


void setup() {
  Wire.begin(); // create Wire object for later writing to eeprom
  // put your setup code here, to run once:
  Serial.begin(9600);

#ifdef DEBUG
  while(!Serial); // wait for serial to init de-comment if you want prints to work during setup
  
  Serial.println("Starting setup");
  find_devices();
#endif


  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)

  frontMatrix.begin(0x70);  // pass in the address
  
  // init done
  currentLidState = determineLightState(analogRead(prPin));

  display.display(); // show splashscreen

  display.setFont(&FreeMono9pt7b);

  timeOfLastCheck = 0; // make sure it polls on the first loop
  
  stop_heart_animation();
  
  // TODO read message from non-volatile storage
  
  Message msg = messageList.firstMessage();

  if (msg.message_length > 0) {
    displayMessage(msg.message);
  } else {
    displayMessage("No Messages Yet");
  }

//  
//    unsigned int address = 0;
//    for(address=0;address<5;address++) {
//      writeEEPROM(eeprom, address, '2');
//    }
//    for(address=0; address<5; address++) {
//      Serial.print(readEEPROM(eeprom, address), HEX); 
//    }
}


void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentTime = millis();
  lightVal = analogRead(prPin);
  
  continue_heart_animation();
  
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
    
      triggerStateChangeOnLightValues(lightVal);

  } else {
      triggerStateChangeOnLightValues(lightVal);
  }
  
  delay(200);

}


void lidOpened() {
  stop_heart_animation();
}

void lidClosed() {
  stop_heart_animation();
}

void triggerStateChangeOnLightValues(int lightVal) {
    int lidState = determineLightState(lightVal);
    if (lidState != currentLidState) {
      Serial.println("changing lid state");
      currentLidState = lidState;
      if (lidState == LID_OPEN) {
        lidOpened();
      } else {
        lidClosed();
      }
    }
}

int determineLightState(int lightVal) {
  if (lightVal > lightThreshold) {
    return LID_OPEN;
  }
  return LID_CLOSED;
}

void displayMessage(String message) {
  display.clearDisplay();
  display.setCursor(0,14);
  display.setTextSize(1);
  display.setTextWrap(true);
  display.setTextColor(WHITE, BLACK);
  display.print(message);
  display.display();
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
    //delay(1000);
  }
  Serial.println("WiFi connect complete");
}

void MQTTConnect() {
  mqttClient.setUsernamePassword(MQTT_USERNAME, MQTT_PASSWORD);
  while (!mqttClient.connect(broker, MQTT_PORT)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    // sleep 1 seconds and try again
    delay(1000);
  }

  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);

  mqttClient.subscribe(inTopic, 0);

  Serial.println("MQTT connect complete");
}


void processMqttMessage(String topic, String message) {
    if (topic == "love-box-1") {
      messageList.addMessage(message);
      displayMessage(message);
      if (!displayOn) {
        start_heart_animation();
      }
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


//defines the writeEEPROM function
void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data ) {
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8)); //writes the MSB
  Wire.write((int)(eeaddress & 0xFF)); //writes the LSB
  Wire.write(data);
  Wire.endTransmission();
  delay(5);
}

//defines the readEEPROM function
byte readEEPROM(int deviceaddress, unsigned int eeaddress ) {
  byte rdata = 0xFF;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8)); //writes the MSB
  Wire.write((int)(eeaddress & 0xFF)); //writes the LSB
  Wire.endTransmission();
  Wire.requestFrom(deviceaddress,1);
  if (Wire.available()) 
    rdata = Wire.read();
  return rdata;
} 
