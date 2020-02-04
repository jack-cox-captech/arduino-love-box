
#ifndef MESSAGES
#define MESSAGES 1

#include <Arduino.h>
#include "storage.h"
#include "memory_map.h"

#define MAX_MESSAGE_LIST_LENGTH 1

typedef struct {
  unsigned int length;
  char *bytes;
} MessageMemoryMap;

class Message  {
  public:
    bool unread;
    bool real_message;
    int message_length;
    String message;

    Message() {
      unread = false;
      real_message = false;
      message_length = -1;
      message = "";
    }
    
    MessageMemoryMap *toMemoryMap() { // don't forget to free the returned buffer
      int memSize = sizeof(message_length) + message.length() + 1 + sizeof(unread); // unread flag
      MessageMemoryMap *map = (MessageMemoryMap *)malloc(sizeof(MessageMemoryMap));
      char *bufferPtr = (char *)malloc(memSize);
      unsigned int offset = 0;
      message_length = message.length();
      
      memcpy(bufferPtr+offset, &unread, sizeof(unread));
      offset += sizeof(unread);
      memcpy(bufferPtr+offset, &message_length,  sizeof(message_length));
      offset += sizeof(message_length);
      message.toCharArray(bufferPtr + offset, message_length+1);
      offset += message_length;
      bufferPtr[offset] = '\0';
      map->length = offset+1;
      map->bytes = bufferPtr;
      return map;
    }
};

class MessageList {
  public:
    int max_length = MAX_MESSAGE_LIST_LENGTH;
    int current_length = 0;
    Message messages[MAX_MESSAGE_LIST_LENGTH];
    MessageList() {
      int i = 0;
      for (i=0;i<max_length;i++) {
        messages[i] = Message();
      }
    }

    Message firstMessage() {
      int i;
      for(i=0;i<current_length;i++) {
        Message msg = messages[i];
        if (msg.unread) {
          return msg;
        }
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

    void saveMessageList(int eeprom) {
      int i=0;
      unsigned int offset = MESSAGES_START_ADDR;
      for(i=0;i<current_length;i++) {
          MessageMemoryMap *map = messages[i].toMemoryMap();


  Serial.print("map to save to eeprom: ");Serial.println(map->length);
  for(int q=0;q<map->length;q++) {
    Serial.print(map->bytes[q], HEX);
    Serial.print(':');
  }
  Serial.println(' ');
  
          Serial.println(map->length);
          writeEEPROM(eeprom, offset, map->bytes, map->length);
          offset += map->length;
          free(map->bytes);
          free(map);
      }
      
    }
};


#endif
