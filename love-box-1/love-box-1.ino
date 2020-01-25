#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while(!Serial); // wait for serial to init
 
  
  Serial.println("Starting setup");
  
  delay(200);
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  displayOn = true;
  
  //display.display(); // show splashscreen
  //delay(2000);
  display.clearDisplay();   // clears the screen and buffer

  // draw a single pixel
  display.drawPixel(10, 10, WHITE);
  display.display();
  delay(2000);
  display.clearDisplay();

  display.setFont(&FreeMono12pt7b);

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
  //testdrawcircle();

  lightVal = analogRead(prPin);
  display.clearDisplay();
  
  if (lightVal > lightThreshold) {
    if (!displayOn) { // turn on the display
      turnOnDisplay();
    }

    display.setCursor(0,14);
    display.setTextSize(1);
    display.setTextWrap(true);
    display.setTextColor(WHITE, BLACK);
    display.print("hello world! ");
    display.print(lightVal);
    
  
    display.drawPixel(display.width()-1, display.height()-1, (blinkState ? WHITE : BLACK));
    blinkState = (blinkState ? false : true);
    
    
    Serial.print("printed text, light value= ");
    Serial.println(lightVal);
  } else {
      if (displayOn) { // turn on the display
        turnOffDisplay();
      }
  }
  display.display();
  delay(500);

}
