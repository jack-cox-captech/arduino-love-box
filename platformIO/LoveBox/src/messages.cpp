#include "messages.h"

#include <Arduino.h>
#include "storage.h"
#include "memory_map.h"

#define MAX_MESSAGE_LIST_LENGTH 10

#define DEBUG 1

Message::Message()
{
    unread = false;
    real_message = false;
    message_length = -1;
    memory_location = -1;
    message = "";
    id = "";
    sendTime = "";
}

String Message::toString()
{
    return String(unread) + ':' + String(message_length) + ':' + message + ':' + id + ':' + sendTime;
}
MessageMemoryMap *Message::toMemoryMap()
{ // don't forget to free the returned buffer
    // msg memory map
    //  | 1 | - unread
    //  | 2 | 3 | - tex byte count without terminating zero
    //  | 4 | 5 | .... - chars
    //  | X | - id byte count without termination zero
    //  | ... | - id chars
    //  | Y | - timestamp byte count without terminating zero
    //  | ... | timestamp chars
    //
    int memSize = sizeof(message_length) + message.length() + 1 + sizeof(unread) + 1 // unread flag
                  + id.length() + sizeof(byte) + 1                                   // id
                  + sendTime.length() + sizeof(byte) + 1;                            // sendtime flag
    MessageMemoryMap *map = (MessageMemoryMap *)malloc(sizeof(MessageMemoryMap));
    char *bufferPtr = (char *)malloc(memSize);
#ifdef DEBUG
    Serial.print("allocating for map: ");
    Serial.print(sizeof(MessageMemoryMap));
    Serial.print(" + ");
    Serial.println(memSize);
#endif
    unsigned int offset = 0;
    message_length = message.length() + 1;

    // unread flag
    memcpy(bufferPtr + offset, &unread, sizeof(unread));
    offset += sizeof(unread);
    // message text
    memcpy(bufferPtr + offset, &message_length, sizeof(message_length));
    offset += sizeof(message_length);
    message.toCharArray(bufferPtr + offset, message_length);
    offset += message_length;

    // message id
    byte id_length = (byte)id.length();
    memcpy(bufferPtr + offset, &id_length, sizeof(id_length));
    offset += sizeof(id_length);
    id.toCharArray(bufferPtr + offset, id_length);
    offset += id_length;

    // message timestamp
    byte ts_length = (byte)sendTime.length();
    memcpy(bufferPtr + offset, &ts_length, sizeof(ts_length));
    offset += sizeof(ts_length);
    sendTime.toCharArray(bufferPtr + offset, ts_length);
    offset += ts_length;

    map->length = offset;
    map->bytes = bufferPtr;
    return map;
}

int Message::initializeFromEEPROM(int eeprom, int offset)
{
    memory_location = offset;
    real_message = true;
    readEEPROM(eeprom, offset, (unsigned char *)&unread, sizeof(unread));
    offset += sizeof(unread);
    readEEPROM(eeprom, offset, (unsigned char *)&message_length, sizeof(message_length));
    offset += sizeof(message_length);
    char *strBuffer = (char *)malloc(message_length);
    readEEPROM(eeprom, offset, (unsigned char *)strBuffer, message_length);
    message = String(strBuffer);
    offset += message_length;
    free(strBuffer);

    // get id
    byte id_length;
    readEEPROM(eeprom, offset, (unsigned char *)&id_length, sizeof(id_length));
    offset += sizeof(id_length);
    strBuffer = (char *)malloc(id_length);
    readEEPROM(eeprom, offset, (unsigned char *)strBuffer, id_length);
    offset += id_length;
    id = String(strBuffer);
    free(strBuffer);

    byte ts_length;
    readEEPROM(eeprom, offset, (unsigned char *)&ts_length, sizeof(ts_length));
    offset += sizeof(ts_length);
    strBuffer = (char *)malloc(ts_length);
    readEEPROM(eeprom, offset, (unsigned char *)strBuffer, ts_length);
    offset += ts_length;
    sendTime = String(strBuffer);
    free(strBuffer);

#ifdef DEBUG
    Serial.println(toString());
#endif
    return offset;
}

void Message::markAsRead(int eeprom)
{
    if (!real_message)
    {
        return; // don't save temp messages
    }
    if (memory_location >= 0)
    {
        unread = false;
        writeEEPROM(eeprom, memory_location, (char *)&unread, sizeof(unread));
    }
}

MessageList::MessageList()
{
    int i = 0;
    for (i = 0; i < max_length; i++)
    {
        messages[i] = Message();
    }
}
int MessageList::countOfUnreadMessage() {
    int i = 0;
    int count = 0;
    for (i = 0; i < current_length; i++)
    {
        count += (messages[i].unread ? 1 : 0);
    }
    Serial.printf("There are %d unread messages\n", count);
    return count;
}
Message *MessageList::setCursorToFirstMessage() {
    if (current_length > 0) {
        cursor = 0;
        return messageAtCursor();
    } else {
        return NULL;
    }
}
Message *MessageList::setCursorToOldestUnreadMessage()
{
    for (int i = max_length - 1; i >= 0; i--)
    {
        if (messages[i].unread)
        {
            cursor = i;
            return messageAtCursor();
        }
    }
    return NULL;
}

Message *MessageList::messageAtCursor()
{
    return &messages[cursor];
}

Message *MessageList::moveCursorToNextMessage()
{
    cursor = max(0, cursor - 1);
    Serial.print("Move next Cursor is ");Serial.println(cursor);
    return messageAtCursor();
}

Message *MessageList::moveCursorToPriorMessage()
{
    cursor = min(current_length - 1, cursor + 1);
    Serial.print("Move prior Cursor is ");Serial.println(cursor);
    return messageAtCursor();
}

Message MessageList::addMessage(String id, String sendTime, String messageText)
{
    Message msg = Message();
    msg.id = id;
    msg.sendTime = sendTime;
    msg.real_message = true;
    msg.unread = true;
    msg.message_length = messageText.length();
    msg.message = messageText;

    // shift array down one
    int i = 0;
    for (i = max_length - 1; i > 0; i--)
    {
        messages[i] = messages[i - 1];
    }
    messages[0] = msg;
    current_length = (++current_length > max_length ? max_length : current_length);
    return msg;
}

void MessageList::initializeFromEEPROM(int eeprom)
{
    int i = 0;
    unsigned int offset = MESSAGES_START_ADDR;
    readEEPROM(eeprom, offset, (unsigned char *)&current_length, sizeof(current_length));
    if ((current_length > max_length) || (current_length < 0))
    {
        Serial.println("invalid number of characters to read to restore messages");
        current_length = 0;
        // initialize memory
        saveMessageList(eeprom);
        return; // don't read anymore
    }
    offset += sizeof(current_length);
#ifdef DEBUG
    Serial.print("messages to read ");
    Serial.println(current_length);
#endif
    for (i = 0; i < current_length; i++)
    {
        Message m = Message();
        offset = m.initializeFromEEPROM(eeprom, offset);
        messages[i] = m;
    }
}

void MessageList::saveMessageList(int eeprom)
{
    int i = 0;
    unsigned int offset = MESSAGES_START_ADDR;
    // write the count of messages
    char *countBuffer = (char *)malloc(sizeof(current_length));
    memcpy(countBuffer, &current_length, sizeof(current_length));
    writeEEPROM(eeprom, offset, countBuffer, sizeof(current_length));
    free(countBuffer);
    offset += sizeof(current_length);

    for (i = 0; i < current_length; i++)
    {
        MessageMemoryMap *map = messages[i].toMemoryMap();

#ifdef DEBUG
        Serial.print("map to save to eeprom: ");
        Serial.println(map->length);
        for (int q = 0; q < map->length; q++)
        {
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
