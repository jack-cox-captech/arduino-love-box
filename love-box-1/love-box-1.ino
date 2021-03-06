
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_LEDBackpack.h>

#include <ArduinoMqttClient.h>
#include <Bounce2.h>

#include <ArduinoJson.h>

#define OLED_ADDR 0x3C
#define LCD_ADDR  0x70

#ifdef ARDUINO_SAMD_MKR1000
#include <WiFi101.h> // MKR 1000
#define PR_PIN    0
#define RESET_PIN 7

#endif

#ifdef ARDUINO_SAMD_MKRWIFI1010
#include <WiFiNINA.h> // MKR 1010

#define PR_PIN    0
#define RESET_PIN 7

#endif
#ifdef ARDUINO_ESP32_DEV

#include <WiFi.h> // ESP32
#define PR_PIN    0 /* TBD */
#define RESET_PIN 7 /* TBD */


#endif

#ifdef ARDUINO_ESP8266_ESP12 // Huzzah board

#include <ESP8266WiFi.h>

#define PR_PIN    0 /* TBD */
#define RESET_PIN 7 /* TBD */



#endif


#include <SD.h>

#include "constants.h"
#include "messages.h"
#include "memory_map.h"

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
int mqtt_errors = 0;

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




//#if (SSD1306_LCDHEIGHT != 64)
//#error("Height incorrect, please fix Adafruit_SSD1306.h!");
//#endif

#define WHITE SSD1306_WHITE
#define BLACK SSD1306_BLACK

bool blinkState = false;
int loopCount = 0;

const int prPin = PR_PIN; // photo resistor pin in A0
const int lightThreshold = 40; // below this turns off the display
#define LID_OPEN    1
#define LID_CLOSED  2
int currentLidState = 0;

int lightVal;
bool displayOn;
  
// message state
String lastMessage = "No Messages Yet";
#define eeprom 0x50
MessageList messageList = MessageList();
Message currentMessage;

// power management

unsigned long timeOfLastCheck = 0;
const unsigned long pollTime = 5000; // 5 seconds

int resetPin = RESET_PIN;
void resetFunc(void) {

  Serial.println("resetting");
  delay(100);
  digitalWrite(resetPin, LOW);
}


void setup() {
  pinMode(prPin, INPUT);
  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, HIGH);
  Wire.begin(); // create Wire object for later writing to eeprom
  // put your setup code here, to run once:
  Serial.begin(9600);

  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);  // initialize with the I2C addr 0x3D (for the 128x64)

  frontMatrix.begin(LCD_ADDR);  // pass in the address
  
  // init done
  currentLidState = determineLightState(analogRead(prPin));

  display.display(); // show splashscreen

  display.setFont(&FreeMono9pt7b);

  timeOfLastCheck = 0; // make sure it polls on the first loop
  
  stop_heart_animation();
  
  // read message from non-volatile storage
  messageList.initializeFromEEPROM(eeprom);
  
  currentMessage = messageList.firstMessage();

  if (currentMessage.message_length > 0) {
    displayMessage(currentMessage.message);
    if (currentMessage.unread) {
      start_heart_animation();
    }
  } else {
    displayMessage("No Messages Yet");
  }

//  
//    unsigned int address = 0;
//    for(address=0;address<5;address++) {
//      writeEEPROM(eeprom, address, '2');
//    }

    unsigned char buf[12];
    readEEPROM(eeprom, MESSAGES_START_ADDR, buf, 16);
    for(int i=0;i<16;i++) {
      Serial.print(buf[i], HEX);
      Serial.print(':'); 
    }
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

void markMessageAsRead(Message msg) {
  if (msg.unread) { // only update if the message is unread
    msg.unread = false;
    currentMessage.markAsRead(eeprom);
  }
}

void lidOpened() {
  stop_heart_animation();
  markMessageAsRead(currentMessage);
}

void lidClosed() {
  stop_heart_animation();
  markMessageAsRead(currentMessage);
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
//#ifdef DEBUG
//  Serial.print("determineLightState: light value: ");Serial.println(lightVal);
//#endif
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
  mqtt_errors = 0;
  mqttClient.setUsernamePassword(MQTT_USERNAME, MQTT_PASSWORD);
  while (!mqttClient.connect(broker, MQTT_PORT)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    // sleep 1 seconds and try again
    mqtt_errors++;
    if (mqtt_errors > 5) {
      resetFunc();
    }
    delay(1000);
  }
  mqtt_errors = 0;
  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);

  mqttClient.subscribe(inTopic, 0);

  Serial.println("MQTT connect complete");
}


void processMqttMessage(String topic, String message) {
    if (topic == "love-box-1") {
      StaticJsonDocument<2000> doc;

      DeserializationError error = deserializeJson(doc, message);

      // Test if parsing succeeds.
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
      }
      if (doc["type"] == "reset") {
        resetFunc();
      } else if (doc["type"] == "message")  {
        
        messageList.addMessage(doc["text"]);
        messageList.saveMessageList(eeprom);
        displayMessage(doc["text"]);
        if (!displayOn) {
          start_heart_animation();
        }
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

 
void writeEEPROM(int deviceaddress, unsigned int eeaddress, char* data, unsigned int data_len) 
{
  // Uses Page Write for 24LC256
  // Allows for 64 byte page boundary
  // Splits string into max 16 byte writes
  unsigned char i=0, counter=0;
  unsigned int  address;
  unsigned int  page_space;
  unsigned int  page=0;
  unsigned int  num_writes;
  unsigned char first_write_size;
  unsigned char last_write_size;  
  unsigned char write_size;  
  
  // Calculate space available in first page
  page_space = int(((eeaddress/64) + 1)*64)-eeaddress;

  // Calculate first write size
  if (page_space>16) {
    first_write_size=page_space-((page_space/16)*16);
    if (first_write_size==0) { 
      first_write_size=16;
    }
  } else { 
     first_write_size=page_space; 
  }
  first_write_size=min((unsigned int)first_write_size,(unsigned int) data_len);

  // calculate size of last write  
  if (data_len>first_write_size) 
     last_write_size = (data_len-first_write_size)%16;   
  
  // Calculate how many writes we need
  if (data_len>first_write_size)
     num_writes = ((data_len-first_write_size)/16)+2;
  else
     num_writes = 1;  
     
  i=0;   
  address=eeaddress;
  for(page=0;page<num_writes;page++) 
  {
     if(page==0) write_size=first_write_size;
     else if(page==(num_writes-1)) write_size=last_write_size;
     else write_size=16;


  
     Wire.beginTransmission(deviceaddress);
     Wire.write((int)((address) >> 8));   // MSB
     Wire.write((int)((address) & 0xFF)); // LSB
     counter=0;
     do{ 
        Wire.write((byte) data[i]);

        i++;
        counter++;
     } while((counter<write_size));  
     Wire.endTransmission();
     address+=write_size;   // Increment address for next write
     
     delay(6);  // needs 5ms for page write

  }
}
 
void readEEPROM(int deviceaddress, unsigned int eeaddress,  
                 unsigned char* data, unsigned int num_chars) 
{
  unsigned char i=0;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(deviceaddress,num_chars);
 
  while(Wire.available()) data[i++] = Wire.read();

}
