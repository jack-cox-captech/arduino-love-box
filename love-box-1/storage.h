
#include <SPI.h>
#include <Wire.h>


void writeEEPROM(int deviceaddress, unsigned int eeaddress, char* data, unsigned int data_len);
