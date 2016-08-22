//
// Created by joshua on 12.08.16.
//

#ifndef KNX_KNXPACKET_H
#define KNX_KNXPACKET_H

#include "KnxIO.h"

class KnxPacket
{
    public:
        KnxPacket();
        KnxPacket(const KnxPacket&);
        ~KnxPacket();
        KnxPacket(char *bytes, int size);
    
        void  setBytes(char bytes[], int size); // for received
        char* getBytes();         // for send
    
    
        void setSourceAddr(char area, char line, char device);                  // set knx source address
        void getSourceAddr(char *area, char *line, char *device);               // get knx source address
        
        void setDestinationPhysicalAddr(char area, char line, char device);     // set knx destination address -> if it is a physical address -> look in getDestinationAddrType()
        void getDestinationPhysicalAddr(char *area, char *line, char *device);  // get knx destination address -> if it is a physical address -> look in getDestinationAddrType()
        
        void setDestinationGroupAddr(char main, char middle, char sub);         // set knx destination address -> if it is a group address -> look in getDestinationAddrType()
        void getDestinationGroupAddr(char *main, char *middle, char *sub);      // get knx destination address -> if it is a group address -> look in getDestinationAddrType()
    
        bool getDestinationAddrType();      // destination address type KNX_PACKET_DESTINATION_ADDR_TYPE_ , set automatic
    
    
        void setPriority(char  priority);   // set priority KNX_PACKET_PRIORITY_
        char getPriority();                 // get priority KNX_PACKET_PRIORITY_
        char* getPriorityString();          // get priority as string
    
        void setRepeat(bool  repeat);       // set repeat flank
        bool getRepeat();                   // get repeat flank
    
        void checkByteCreate();             // after all values are set -> set check byte
        bool checkByteCheck();              // check with check byte if packet data is correct
    
    
        // Data action
        void setDataAction(char action);    // set action (value write, read, response to read) -> KNX_PACKET_DATA_ACTION_
        char getDataAction();               // get action (value write, read, response to read) -> KNX_PACKET_DATA_ACTION_
        char* getDataActionString();        // get action as string (value write, read, response to read) -> KNX_PACKET_DATA_ACTION_
    
    
        // Data
        void setDataValueSwitch(bool on);                       // set data as switch @param on
        bool getDataValueSwitch();                              // get data as switch
    
        void setDataValueDim(bool  increase, char  steps);      // set data as dimmer @param increase @param steps
        void getDataValueDim(bool *increase, char *steps);      // get data as dimmer @param increase @param steps
    
    
        void print();                       // print all info
    
        // size of packet
        char size;
    
        // send
        void send();
    
    
    private:
        char bytes[KNX_TELEGRAM_MAX_LENGTH] = {0};
        void setDestinationAddrType(bool type);     // destination address type
        void setRoutingCounter(char value);         // set routing counter
        char getDataLength();                       // amount of data bytes
        void setDataLength(char length);            // amount of data bytes
    
};


// priority
#define KNX_PACKET_PRIORITY_SYSTEM  0b00000000
#define KNX_PACKET_PRIORITY_ALARM   0b00000100
#define KNX_PACKET_PRIORITY_NORMAL  0b00001100
#define KNX_PACKET_PRIORITY_HIGH    0b00001000

// destination addess type
#define KNX_PACKET_DESTINATION_ADDR_TYPE_PHYSICAL   0
#define KNX_PACKET_DESTINATION_ADDR_TYPE_GROUP      1

// routing counter
#define KNX_PACKET_ROUTING_COUNTER_DEFAULT 6

// data action
#define KNX_PACKET_DATA_ACTION_VALUE_READ                       0b00000000
#define KNX_PACKET_DATA_ACTION_VALUE_RESPONSE                   0b00000001
#define KNX_PACKET_DATA_ACTION_VALUE_WRITE                      0b00000010
#define KNX_PACKET_DATA_ACTION_INDIVIDUAL_ADDRESS_WRITE         0b00000011
#define KNX_PACKET_DATA_ACTION_INDIVIDUAL_ADDRESS_REQUEST       0b00000100
#define KNX_PACKET_DATA_ACTION_INDIVIDUAL_ADDRESS_RESPONSE      0b00000101

#define KNX_PACKET_DATA_OFFSET          6 // first data byte


#endif //KNX_KNXPACKET_H
