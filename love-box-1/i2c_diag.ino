
#include <Wire.h>



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
