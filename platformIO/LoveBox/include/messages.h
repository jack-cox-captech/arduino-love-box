
#ifndef MESSAGES_H
#define MESSAGES_H 1

#include <Arduino.h>

#define MAX_MESSAGE_LIST_LENGTH 10

typedef struct {
  unsigned int length;
  char *bytes;
} MessageMemoryMap;

class Message  {
  public:
    bool unread;
    bool real_message;
    short message_length;
    String id;
    String message;
    String sendTime;
    int memory_location;

    Message();

    String toString();
    MessageMemoryMap *toMemoryMap();

    int initializeFromEEPROM(int eeprom, int offset);

    void markAsRead(int eeprom);
};

class MessageList {
  public:
    short max_length = MAX_MESSAGE_LIST_LENGTH;
    short current_length = 0;
    Message messages[MAX_MESSAGE_LIST_LENGTH];
    short cursor = 0;
    MessageList();

    int countOfUnreadMessage();
    
    Message *setCursorToFirstMessage();
    Message *setCursorToOldestUnreadMessage();

    Message *messageAtCursor();

    Message *moveCursorToNextMessage();

    Message *moveCursorToPriorMessage();

    Message addMessage(String id, String sendTime, String messageText);

    void initializeFromEEPROM(int eeprom);

    void saveMessageList(int eeprom);
};


#endif
