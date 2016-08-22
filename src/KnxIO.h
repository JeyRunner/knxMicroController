//
// Created by joshua on 10.08.16.
//

#ifndef KNX_KNXIO_H
#define KNX_KNXIO_H
#include <Log.h>

#define KNX_TELEGRAM_MAX_LENGTH          16    // byte per telegram
#define KNX_TELEGRAM_BUFFER_RECEIVE_SIZE 15    // amount of telegrams in receive buffer
//#define KNX_TELEGRAM_BUFFER_SEND_SIZE    4    // amount of telegrams in send buffer

class KnxIO
{
    public:
        static void init();
        
        volatile static enum BUS_STATUS {
            BUS_STATUS_UNDEFINED,
            BUS_STATUS_FREE,
            BUS_STATUS_START_BIT_RECEIVED,
            BUS_STATUS_WAIT_FOR_NEXT_BYTE,
            BUS_STATUS_SENDING,
        } busStatus;
    
        volatile static struct Telegram {
            volatile char  byteBuffer[KNX_TELEGRAM_MAX_LENGTH];
            volatile char *byteBufferKnx;
            volatile bool  parityBitsOk = true;
            volatile bool  readLock = true;
        } telegramBufferReceive[KNX_TELEGRAM_BUFFER_RECEIVE_SIZE],
          telegramSend;
    
        static int getTelegramBufferSize(Telegram *telegram);
    
        volatile static int   bitNullCounter;
        volatile static bool  parityBit;
        volatile static Telegram *telegramBufferReceiveWrite;
                  static Telegram *telegramBufferReceiveRead;
        volatile static Telegram *telegramBufferReceiveEnd;
    
    
        volatile static uint8_t bitNumber;
        static void telegramBufferReceiveNext();
        static void telegramBufferReceiveCheckByteParity();
        static void telegramBufferReceiveByteBufferNext();
    
        volatile static enum BUS_STATUS_SEND {
            BUS_STATUS_SEND_NOT,
            BUS_STATUS_SEND_START_BIT,
            BUS_STATUS_SEND_DATA,
            BUS_STATUS_SEND_PARITY,
            BUS_STATUS_SEND_WAIT_NEXT_TELEGRAM
        } busStatusSend;
    
        static void setBusStatus(volatile BUS_STATUS status);
    
        volatile static int  sendByteMax;
        volatile static bool telegramSending;
        volatile static int  sendBitNumber;
        volatile static int  sendBitNullCounter;
        volatile static bool sending; // 1 need to send 1, 0 need to set to 0
        static bool sendTelegram(char *bytes, int size); // return false if last telegram is not send already
        static bool telegramSendByteBufferNext();
        static bool sendCollision();
};

#define KNX_SENDING_HIGH   false
#define KNX_SENDING_LOW    true

#endif //KNX_KNXIO_H
