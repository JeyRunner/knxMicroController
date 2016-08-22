//
// Created by joshua on 12.08.16.
//

#ifndef KNX_KNX_H
#define KNX_KNX_H

#include "KnxIO.h"
#include "KnxPacket.h"


class Knx
{
    public:
        static void init();
        
        static int      getPacketsReceivedAmount(); // amount of received packets
        static KnxPacket getNextPacket();            // returns next received packet if available -> check getPacketsReceivedAmount() before
        static KnxPacket getNextPacketBlocking();    // returns next received packet if available -> if nothing received this function blocks
        
        static void sendPacket(KnxPacket *packet);
    
    private:
        static void nextPacket();
};


#endif //KNX_KNX_H
