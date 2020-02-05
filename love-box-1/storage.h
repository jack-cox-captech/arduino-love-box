
#include <SPI.h>
#include <Wire.h>


void writeEEPROM(int deviceaddress, 
  unsigned int eeaddress, 
  char* data, 
  unsigned int data_len);
  
void readEEPROM(int deviceaddress, 
  unsigned int eeaddress,  
  unsigned char* data, 
  unsigned int num_chars);
