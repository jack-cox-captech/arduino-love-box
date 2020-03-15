#include <SPI.h>
#include <Wire.h>

#include <PrintEx.h>

PrintEx myPrint = Serial;

#define eeprom 0x50

void setup() {
  // put your setup code here, to run once:
  Wire.begin(); // create Wire object for later writing to eeprom
  // put your setup code here, to run once:
  Serial.begin(115200);

  unsigned char buffer[16];
  while(!Serial); // wait for serial to init de-comment if you want prints to work during setup

  myPrint.printf("Starting setup\n");
  delay(1000);
  int startPage = 0x0400 / 16;
  int endPage = startPage + 10;
  for(int page=startPage;page<endPage;page++) {
      readEEPROM(eeprom, page*16, buffer, 16);
      myPrint.printf("%04x : ", page*16);

      for(int j=0;j<16;j++) {
        myPrint.printf("%02x ", buffer[j]);
      }
      myPrint.printf(" : ");
      for(int j=0;j<16;j++) {
        if ((buffer[j] >= 32) && (buffer[j] <= 126)) {
          myPrint.printf("%c", buffer[j]);
        } else {
          myPrint.printf(".");
        }
      }
      myPrint.printf("\n");
  }
}

void loop() {
  // put your main code here, to run repeatedly:

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
