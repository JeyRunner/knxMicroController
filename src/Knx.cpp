//
// Created by joshua on 12.08.16.
//

#include "Knx.h"
#include <util/delay.h>
#include <stddef.h>


// -- init
void Knx::init()
{
    KnxIO::init();
}


// -- packet amount available
int Knx::getPacketsReceivedAmount()
{
    int diff = KnxIO::telegramBufferReceiveRead - KnxIO::telegramBufferReceiveWrite;
    return (diff < 0) ? -diff : diff;
}


// -- get next packet
KnxPacket Knx::getNextPacket()
{
    // nothing there
    if (KnxIO::telegramBufferReceiveRead == (KnxIO::telegramBufferReceiveWrite))
        return KnxPacket();
    // ignore acknowledge telegrams (only one byte long)
    /*
    while (true)
    {
        if (KnxIO::telegramBufferReceiveRead != (KnxIO::telegramBufferReceiveWrite))
        {
            // not currently writing to that telegram
            if (!KnxIO::telegramBufferReceiveRead->readLock)
            {
                //if (KnxIO::getTelegramBufferSize(KnxIO::telegramBufferReceiveRead) <= 1)
                //    nextPacket();
                //else
                    break;
            }
            else
                return KnxPacket();
        }
        else
            return KnxPacket();
        
        _delay_us(10);
    }*/

    // create packet
    // KnxPacket *packet = (KnxPacket*) malloc(sizeof(KnxPacket));
    KnxPacket packet;
    packet.setBytes((char*)KnxIO::telegramBufferReceiveRead->byteBuffer,
                     KnxIO::getTelegramBufferSize(KnxIO::telegramBufferReceiveRead));

    
    // next
    nextPacket();
    return packet;
}

KnxPacket Knx::getNextPacketBlocking()
{
    // nothing there
    // ignore acknowledge telegrams (only one byte long)
    while (true)
    {
        if (KnxIO::telegramBufferReceiveRead != (KnxIO::telegramBufferReceiveWrite))
        {
            // not currently writing to that telegram
            if (!KnxIO::telegramBufferReceiveRead->readLock)
            {
                if (KnxIO::getTelegramBufferSize(KnxIO::telegramBufferReceiveRead) <= 1)
                    nextPacket();
                else
                    break;
            }
        }
        _delay_us(10);
    }
    
    KnxPacket packet;
    //cli();
    packet.setBytes((char*)KnxIO::telegramBufferReceiveRead->byteBuffer,
                    KnxIO::getTelegramBufferSize(KnxIO::telegramBufferReceiveRead));
    //sei();
    
    // next
    nextPacket();
    return packet;
}


// -- set pointer to next packet
void Knx::nextPacket()
{
    // next telegram
    KnxIO::telegramBufferReceiveRead++;
    
    // if at end of ringbuffer reset to begin
    if (KnxIO::telegramBufferReceiveRead == KnxIO::telegramBufferReceiveEnd)
    {
        KnxIO::telegramBufferReceiveRead = (KnxIO::Telegram*)KnxIO::telegramBufferReceive;
    }
}

// -- send
void Knx::sendPacket(KnxPacket *packet)
{
    packet->checkByteCreate();
    KnxIO::sendTelegram(packet->getBytes(), packet->size);
}