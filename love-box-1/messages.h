
#ifndef MESSAGES
#define MESSAGES 1

#include <Arduino.h>
#include "storage.h"
#include "memory_map.h"

#define MAX_MESSAGE_LIST_LENGTH 1

#define DEBUG 1

typedef struct {
  unsigned int length;
  char *bytes;
} MessageMemoryMap;

class Message  {
  public:
    bool unread;
    bool real_message;
    short message_length;
    String message;
    int memory_location;

    Message() {
      unread = false;
      real_message = false;
      message_length = -1;
      memory_location = -1;
      message = "";
    }

    String toString() {
      return String(unread)+':'+String(message_length)+':'+message;
    }
    MessageMemoryMap *toMemoryMap() { // don't forget to free the returned buffer
      // msg memory map
      //  | 1 | - unread 
      //  | 2 | 3 | - byte count without terminating zero  
      //  | 4 | 5 | .... - chars
      //
      int memSize = sizeof(message_length) + message.length() + 1 + sizeof(unread); // unread flag
      MessageMemoryMap *map = (MessageMemoryMap *)malloc(sizeof(MessageMemoryMap));
      char *bufferPtr = (char *)malloc(memSize);
      unsigned int offset = 0;
      message_length = message.length() + 1;
      
      memcpy(bufferPtr+offset, &unread, sizeof(unread));
      offset += sizeof(unread);
      memcpy(bufferPtr+offset, &message_length,  sizeof(message_length));
      offset += sizeof(message_length);
      message.toCharArray(bufferPtr + offset, message_length);
      offset += message_length;
      map->length = offset;
      map->bytes = bufferPtr;
      return map;
    }

    int initializeFromEEPROM(int eeprom, int offset) {
      memory_location = offset;
      real_message = true;
      readEEPROM(eeprom, offset, (unsigned char *) &unread, sizeof(unread));
      offset += sizeof(unread);
      readEEPROM(eeprom, offset, (unsigned char *) &message_length, sizeof(message_length));
      offset += sizeof(message_length);
      char *strBuffer = (char *) malloc(message_length);
      readEEPROM(eeprom, offset, (unsigned char *) strBuffer, message_length);
      message = String(strBuffer);
      offset += message_length;
      
      free(strBuffer);
#ifdef DEBUG
  Serial.println(toString());
#endif
        return offset;
    }

    void markAsRead(int eeprom) {
      if (memory_location >= 0) {
        unread = false;
        writeEEPROM(eeprom, memory_location, (char *) &unread, sizeof(unread));
      }
    }
};

class MessageList {
  public:
    short max_length = MAX_MESSAGE_LIST_LENGTH;
    short current_length = 0;
    Message messages[MAX_MESSAGE_LIST_LENGTH];
    MessageList() {
      int i = 0;
      for (i=0;i<max_length;i++) {
        messages[i] = Message();
      }
    }

    Message firstMessage() {
      if (messages[0].real_message) {
        return messages[0];
      }
      return Message();
    }

    Message addMessage(String messageText) {
      Message msg = Message();
      msg.real_message = true;
      msg.unread = true;
      msg.message_length = messageText.length();
      msg.message = messageText;

      // shift array down one
      int i =0;
      for(i=max_length-1;i>0;i--) {
        messages[i] = messages[i-1];
      }
      messages[0] = msg;
      current_length = (++current_length > max_length ? max_length : current_length);
      return msg;
    }

    void initializeFromEEPROM(int eeprom) {
      int i=0;
      unsigned int offset = MESSAGES_START_ADDR;
      readEEPROM(eeprom, offset, (unsigned char *) &current_length, sizeof(current_length));
      if ((current_length > max_length) || (current_length < 0)) {
        Serial.println("invalid number of characters to read to restore messages");
        current_length = 0;
        // initialize memory
        saveMessageList(eeprom);
        return; // don't read anymore
      }
      offset += sizeof(current_length);
#ifdef DEBUG
  Serial.print("messages to read ");Serial.println(current_length);
#endif 
      for(i=0;i<current_length;i++) {
        Message m = Message();
        offset = m.initializeFromEEPROM(eeprom, offset);
        messages[i] = m;
      }
    }

    void saveMessageList(int eeprom) {
      int i=0;
      unsigned int offset = MESSAGES_START_ADDR;
      // write the count of messages
      char *countBuffer = (char *)malloc(sizeof(current_length));
      memcpy(countBuffer, &current_length, sizeof(current_length));
      writeEEPROM(eeprom, offset, countBuffer, sizeof(current_length));
      free(countBuffer);
      offset += sizeof(current_length);
      
      for(i=0;i<current_length;i++) {
          MessageMemoryMap *map = messages[i].toMemoryMap();

#ifdef DEBUG
  Serial.print("map to save to eeprom: ");Serial.println(map->length);
  for(int q=0;q<map->length;q++) {
    Serial.print(map->bytes[q], HEX);
    Serial.print(':');
  }
  Serial.println(' ');
  Serial.println(map->length);
#endif 
          writeEEPROM(eeprom, offset, map->bytes, map->length);
          offset += map->length;
          free(map->bytes);
          free(map);
      }
      
    }
};


#endif
