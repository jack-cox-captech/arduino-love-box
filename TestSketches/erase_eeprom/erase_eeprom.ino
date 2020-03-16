void setup() {
  // put your setup code here, to run once:
  char buffer[16] = { 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF};
}

void loop() {
  // put your main code here, to run repeatedly:

}

 
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
  TRACE();
  unsigned char i=0;
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
 
  Wire.requestFrom(deviceaddress,num_chars);
 
  while(Wire.available()) data[i++] = Wire.read();

}
