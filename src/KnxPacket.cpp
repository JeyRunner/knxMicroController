//
// Created by joshua on 12.08.16.
//

#include "KnxPacket.h"
#include "Knx.h"


// -- create ------------------------------------
KnxPacket::KnxPacket()
{
    // Log::log("KnxPacket create \n\r");
    
    // create array with max size
    //bytes = (char*) malloc(KNX_TELEGRAM_MAX_LENGTH); // create array with correct size
    size  = KNX_TELEGRAM_MAX_LENGTH;
    
    // default values
    bytes[0] |= 0b10010000;
    setPriority(KNX_PACKET_PRIORITY_NORMAL);
    setRepeat(false);
    setRoutingCounter(KNX_PACKET_ROUTING_COUNTER_DEFAULT);
    setDataAction(KNX_PACKET_DATA_ACTION_VALUE_WRITE);
}

KnxPacket::KnxPacket(const KnxPacket &)
{
    // Log::log("KnxPacket copy \n\r");
}

KnxPacket::~KnxPacket()
{
    // delete bytes array
    //free(bytes);
    // Log::log("KnxPacket delete \n\r");
}

KnxPacket::KnxPacket(char *bytes, int size)
{
    setBytes(bytes, size);
}


// -- save bytes -------------------------------
void KnxPacket::setBytes(char bytes[], int size)
{
    //this->bytes = bytes;
    //this->bytes = (char*) malloc(size); // create array with correct size
    memcpy(this->bytes, bytes, size);   // copy data
    this->size = size;
}

char* KnxPacket::getBytes()
{
    return bytes;
}


// -- priority ----------------------------------
void KnxPacket::setPriority(char priority)
{
    bytes[0] |= priority;
}

char KnxPacket::getPriority()
{
    return bytes[0] & 0b00001100;
}

char* KnxPacket::getPriorityString()
{
    switch (getPriority())
    {
        case (char)KNX_PACKET_PRIORITY_HIGH:
            return "high";
        case (char)KNX_PACKET_PRIORITY_NORMAL:
            return "normal";
        case KNX_PACKET_PRIORITY_SYSTEM:
            return "system";
        case (char)KNX_PACKET_PRIORITY_ALARM:
            return "alarm";
    }
}


// -- repeat flak --------------------------------
void KnxPacket::setRepeat(bool repeat)
{
    bytes[0] |= ((!repeat) << 5) & 0b00100000; // only set to 1 possible (or)
}

bool KnxPacket::getRepeat()
{
    // char b[10];
    // Log::log("first byte %s\n\r", Log::byteToChar(bytes[0], b));
    return !(bytes[0] >> 5);
}


// -- source address ----------------------------
void KnxPacket::setSourceAddr(char area, char line, char device)
{
    bytes[1] = device;
    bytes[2] = (line & 0b00001111) | (area << 4);
}

void KnxPacket::getSourceAddr(char *area, char *line, char *device)
{
    *device  = bytes[1];
    *line    = bytes[2] & 0b00001111;
    *area    = bytes[2] >> 4;
}


// -- destination address -----------------------
// physical
void KnxPacket::setDestinationPhysicalAddr(char area, char line, char device)
{
    bytes[3] = device;
    bytes[4] = (line & 0b00001111) | (area << 4);
    setDestinationAddrType(KNX_PACKET_DESTINATION_ADDR_TYPE_PHYSICAL);
}

void KnxPacket::getDestinationPhysicalAddr(char *area, char *line, char *device)
{
    *device = bytes[3];
    *line   = bytes[4] & 0b00001111;
    *area   = bytes[4] >> 4;
}

// group
void KnxPacket::setDestinationGroupAddr(char main, char middle, char sub)
{
    bytes[3]  = sub;
    bytes[4]  = middle      & 0b00000111;
    bytes[4] |= (main << 3) & 0b01111000;
    setDestinationAddrType(KNX_PACKET_DESTINATION_ADDR_TYPE_GROUP);
    
}

void KnxPacket::getDestinationGroupAddr(char *main, char *middle, char *sub)
{
    *sub    = bytes[3];
    *middle = bytes[4]   & 0b00000111;
    *main   = ((bytes[4] & 0b11111000)  >> 3);
}


// -- destination address type
bool KnxPacket::getDestinationAddrType()
{
    return ((bytes[5] >> 7));
}

void KnxPacket::setDestinationAddrType(bool type)
{
    bytes[5] |= (type << 7) & 0b10000000;
}


// -- routing counter
void KnxPacket::setRoutingCounter(char value)
{
    bytes[5] |= (value << 4) & 0b01110000;
}


// -- data bytes amount
char KnxPacket::getDataLength()
{
    return (bytes[5] & 0b00001111) + 1;
}

void KnxPacket::setDataLength(char length)
{
    bytes[5] |= (length -1) & 0b00001111;
    size = 8+length-1;
}


// -- check byte
void KnxPacket::checkByteCreate()
{
    char checkSum = bytes[0];
    
    // every n bit of all bytes except check byte self
    for (int i = 1; i < size-1; ++i)
    {
        checkSum ^= bytes[i]; // xor
    }
    
    // invert -> amount of one odd -> 1
    // last byte = check byte
    bytes[size-1] = ~checkSum;
}

bool KnxPacket::checkByteCheck()
{
    char checkSum = bytes[0];
    
    // every n bit of all bytes except check byte self
    for (int i = 1; i < size-1; ++i)
    {
        checkSum ^= bytes[i]; // xor
    }
    
    // invert -> amount of one odd -> 1
    // last byte = check byte
    return bytes[size-1] == ~checkSum;
}


// -- action
void KnxPacket::setDataAction(char action)
{
    bytes[KNX_PACKET_DATA_OFFSET]   |= ((action >> 2) & 0b00000011);
    bytes[KNX_PACKET_DATA_OFFSET+1] |= ((action << 6) & 0b11000000);
}

char KnxPacket::getDataAction()
{
    return ((bytes[KNX_PACKET_DATA_OFFSET]   & 0b00000011) << 2 )  |
            ((bytes[KNX_PACKET_DATA_OFFSET+1] & 0b11000000) >> 6);
}

char* KnxPacket::getDataActionString()
{
    switch (getDataAction())
    {
        case (char)KNX_PACKET_DATA_ACTION_VALUE_READ:
            return "group value read";
        case (char)KNX_PACKET_DATA_ACTION_VALUE_RESPONSE:
            return "group value response";
        case (char)KNX_PACKET_DATA_ACTION_VALUE_WRITE:
            return "group value write";
    
        case (char)KNX_PACKET_DATA_ACTION_INDIVIDUAL_ADDRESS_REQUEST:
            return "individual address request";
        case (char)KNX_PACKET_DATA_ACTION_INDIVIDUAL_ADDRESS_RESPONSE:
            return "individual address response";
        case (char)KNX_PACKET_DATA_ACTION_INDIVIDUAL_ADDRESS_WRITE:
            return "individual address write";
        
        default:
            return "unknown";
    }
}



// -- data value --------------------------------------
// -- switch
void KnxPacket::setDataValueSwitch(bool on)
{
    if (on)
        bytes[KNX_PACKET_DATA_OFFSET+1] |=  (1);
    else
        bytes[KNX_PACKET_DATA_OFFSET+1] &= ~(1);
    setDataLength(2);
}

bool KnxPacket::getDataValueSwitch()
{
    return bytes[KNX_PACKET_DATA_OFFSET+1] & (1);
}


// -- dimmer
void KnxPacket::setDataValueDim(bool increase, char steps)
{
    // @TODO only setting possible |=
    bytes[KNX_PACKET_DATA_OFFSET+1] |= (increase << 3) |
                                       (steps & 0b00000111);
    setDataLength(2);
}

void KnxPacket::getDataValueDim(bool *increase, char *steps)
{
    *increase = (bytes[KNX_PACKET_DATA_OFFSET+1] & 0b00001000) >> 3;
    *steps    = bytes[KNX_PACKET_DATA_OFFSET+1] & 0b00000111;
}



// -- print -------------------------------------------
void KnxPacket::print()
{
    Log::log("---------- KNX Packet ------------------------\n\r");
    Log::log("-------------------------------- Control -----\n\r");
    char raw[10*size +1];
    Log::log("---- Raw:\t\t%s\n\r", Log::bytesToChar(bytes, size, raw));
    Log::log("---- Complete size:\t%d\n\r", size);
    Log::log("---- Repeat:\t\t%s\n\r", getRepeat() ? "yes" : "no");
    Log::log("---- Priority:\t\t%s\n\r", getPriorityString());
    
    
    Log::log("-------------------------------- Addresses ---\n\r");
    
    unsigned char area, line, device;
    getSourceAddr((char*)&area, (char*)&line, (char*)&device);
    Log::log("---- Source:\t\t%d/%d/%d \n\r", area, line, device);
    
    unsigned char a, b, c;
    if (getDestinationAddrType() == KNX_PACKET_DESTINATION_ADDR_TYPE_GROUP)
        getDestinationGroupAddr((char*)&a, (char*)&b, (char*)&c);
    else
        getDestinationPhysicalAddr((char*)&a, (char*)&b, (char*)&c);
    
    Log::log("---- Destination type:\t%s \n\r",
             (getDestinationAddrType() == KNX_PACKET_DESTINATION_ADDR_TYPE_GROUP) ? "group" : "physical");
    Log::log("---- Destination:\t%d/%d/%d \n\r", a, b, c);
    
    
    Log::log("-------------------------------- Data --------\n\r");
    Log::log("---- Data action type:\t%s \n\r", getDataActionString());
    Log::log("---- Data length:\t%d byte \n\r", getDataLength() - 1);
    
    // print received bytes
    char out[(getDataLength()-1)*10 +1];
    Log::log("---- Bytes raw:\t\t%s \n\r", Log::bytesToChar(&bytes[KNX_PACKET_DATA_OFFSET+1], getDataLength()-1, out));
    Log::log("---- As switch:\t\t%s \n\r", getDataValueSwitch() ? "on/down" : "off/up");
    bool increase; char step;
    getDataValueDim(&increase, &step);
    Log::log("---- As dimmer:\t\t%s \n\r----\t\t\tsteps: %d\n\r", increase ? "increase" : "decrease", step);
    
    
    Log::log("-------------------------------- Check -------\n\r");
    Log::log("---- CheckByte:\t\t%s \n\r", checkByteCheck() ? "ok" : "some data wrong");
}


// -- send
void KnxPacket::send()
{
    Knx::sendPacket(this);
}