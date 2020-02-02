
#ifndef MESSAGES
#define MESSAGES 1

#include <Arduino.h>

#define MAX_MESSAGE_LIST_LENGTH 1

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
      return msg;
    }
};


#endif
