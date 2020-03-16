#include <Arduino.h>

#include "device_settings.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_LEDBackpack.h>

#include <ArduinoMqttClient.h>
#include <Bounce2.h>

#include <ArduinoJson.h>

#include <ArduinoTrace.h>


#define DEBUG   1

#define OLED_ADDR 0x3C
#define LCD_ADDR  0x70
#define EEPROM_ADDR    0x50



#include <SD.h>

#include "constants.h"
#include "messages.h"
#include "memory_map.h"
#include "heart_animation.h"


// connectivity variables
#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PWD;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = MQTT_BROKER;
const char inTopic[] = "love-box-2";
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

#define WHITE SSD1306_WHITE
#define BLACK SSD1306_BLACK

Bounce next_button = Bounce();
Bounce prior_button = Bounce(); 

// message state
String lastMessage = "No Messages Yet";

MessageList messageList = MessageList();
Message currentMessage;

// power management

unsigned long timeOfLastCheck = 0;
const unsigned long pollTime = 5000; // 5 seconds

void resetFunc();
void find_devices();
void displayMessage(String message);
void readEEPROM(int deviceaddress, unsigned int eeaddress,  
                 unsigned char* data, unsigned int num_chars);
void writeEEPROM(int deviceaddress, unsigned int eeaddress, char* data, unsigned int data_len);
void markMessageAsRead(Message msg);
void WiFiConnect();
void MQTTConnect();
void onMqttMessage(int messageSize);
void processMqttMessage(String topic, String message);
void handleNextButton();
void handlePriorButton();

void setup() {
#ifdef RESET_PIN
  // only do this if we are doing a hardware reset
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
#endif
  Wire.begin(); // create Wire object for later writing to eeprom
  // put your setup code here, to run once:
  Serial.begin(115200);

#ifdef DEBUG
  while(!Serial); // wait for serial to init de-comment if you want prints to work during setup
  
  Serial.println("Starting setup");
  find_devices();
#endif

// uncomment this to reset eeprom message MessageList
//  messageList.saveMessageList(EEPROM_ADDR);


  next_button.attach(NEXT_BUTTON_PIN,INPUT_PULLUP); // Attach the debouncer to a pin with INPUT mode
  next_button.interval(25); 
  prior_button.attach(PRIOR_BUTTON_PIN,INPUT_PULLUP); // Attach the debouncer to a pin with INPUT mode
  prior_button.interval(25); 
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);  // initialize with the I2C addr 0x3D (for the 128x64)

  frontMatrix.begin(LCD_ADDR);  // pass in the address

  display.display(); // show splashscreen

  display.setFont(&FreeMono9pt7b);

  timeOfLastCheck = 0; // make sure it polls on the first loop
  
  stop_heart_animation(frontMatrix);
  
  // read message from non-volatile storage
  messageList.initializeFromEEPROM(EEPROM_ADDR);
  
  currentMessage = messageList.getOldestUnreadMessage();

  if (currentMessage.message_length > 0) {
    displayMessage(currentMessage.message);
    if (currentMessage.unread) {
      start_heart_animation(frontMatrix);
    }
  } else {
    displayMessage("No Messages Yet");
  }
}

void loop() {
  unsigned long currentTime = millis();

  next_button.update(); // Update status of navigation buttons
  prior_button.update();
  
  continue_heart_animation(frontMatrix);
  
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

  }
  if (next_button.rose()) {
    // next button pressed, record current message as read and go to next one
    Serial.println("next button pressed");
  }
  if (prior_button.rose()) {
    // prior button pressed record current message as read and go to prior one
    Serial.println("prior button pressed");
  }

  // push and hold for 2 seconds both buttons at the same time to reset
  if ((next_button.read()) && (prior_button.read())) {
    if ((next_button.duration() > 2000) && (prior_button.duration() > 2000)) {
      resetFunc();
    }
  }

}

void handleNextButton() {
  markMessageAsRead(currentMessage);


}

void handlePriorButton() {

}

void markMessageAsRead(Message msg) {
  if (msg.unread) { // only update if the message is unread
    msg.unread = false;
    currentMessage.markAsRead(EEPROM_ADDR);
  }
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


void resetFunc(void) {

  Serial.println("resetting");
   delay(100);
#ifdef ARDUINO_ESP32_DEV
  ESP.restart();
#else
  digitalWrite(RESET_PIN, LOW);
#endif

}


void find_devices() {
  byte count = 0;
  for (byte i = 8; i < 120; i++)
    {
      Wire.beginTransmission (i);
      if (Wire.endTransmission () == 0)
        {
        Serial.print ("Found address: ");
        Serial.print (i, DEC);
        Serial.print (" (0x");
        Serial.print (i, HEX);
        Serial.println (")");
        count++;
        delay (1);  // maybe unneeded?
        } // end of good response
    } // end of for loop
    Serial.println ("Done.");
    Serial.print ("Found ");
    Serial.print (count, DEC);
    Serial.println (" device(s).");

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
  TRACE();
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
        Serial.println("Got message");
        messageList.addMessage(doc["message_id"], doc["message_time"], doc["text"]);
        messageList.saveMessageList(EEPROM_ADDR);
        displayMessage(doc["text"]);
        start_heart_animation(frontMatrix);
      }   
}



void onMqttMessage(int messageSize) {
  TRACE();
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
 
void writeEEPROM(int deviceaddress, unsigned int eeaddress, char* data, unsigned int data_len) 
{
  TRACE();
  // Uses Page Write for 24LC256
  // Allows for 64 byte page boundary
  // Splits string into max 16 byte writes
  unsigned char i=0, counter=0;
  unsigned int  address;
  unsigned int  page_space;
  unsigned int  page=0;
  unsigned int  num_writes;
  unsigned char first_write_size;
  unsigned char last_write_size=0;  
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
     if(page==0) 
      write_size=first_write_size;
     else if(page==(num_writes-1)) 
      write_size=last_write_size;
     else 
      write_size=16;


  
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
  TRACE();
  unsigned char i=0;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(deviceaddress,num_chars);
 
  while(Wire.available()) data[i++] = Wire.read();

}
