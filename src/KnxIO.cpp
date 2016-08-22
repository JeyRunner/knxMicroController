//
// Created by joshua on 10.08.16.
//

#include "KnxIO.h"

volatile   KnxIO::BUS_STATUS        KnxIO::busStatus     = KnxIO::BUS_STATUS_UNDEFINED;
volatile   KnxIO::BUS_STATUS_SEND   KnxIO::busStatusSend = KnxIO::BUS_STATUS_SEND_NOT;
volatile   int                 KnxIO::bitNullCounter = 0;
volatile   bool                KnxIO::parityBit = true;
volatile   KnxIO::Telegram     KnxIO::telegramBufferReceive[];
volatile   KnxIO::Telegram *   KnxIO::telegramBufferReceiveWrite;
            KnxIO::Telegram *   KnxIO::telegramBufferReceiveRead;
volatile   KnxIO::Telegram *   KnxIO::telegramBufferReceiveEnd;

volatile   KnxIO::Telegram     KnxIO::telegramSend;
volatile   int                 KnxIO::sendByteMax       = 0;
volatile   bool                KnxIO::telegramSending   = false;
volatile   int                 KnxIO::sendBitNumber     = 0;
volatile   int                 KnxIO::sendBitNullCounter= 0;
volatile   bool                KnxIO::sending           = KNX_SENDING_HIGH;



// -- init -------------------------------------------------------------
void KnxIO::init()
{
    Log::log("init\n\r");
    
    // telegram receive buffer
    telegramBufferReceiveWrite = &telegramBufferReceive[0];
    telegramBufferReceiveRead  = (Telegram*)&telegramBufferReceive[0];
    telegramBufferReceiveEnd   = (telegramBufferReceive)+KNX_TELEGRAM_BUFFER_RECEIVE_SIZE-1;
    
    
    // reset first telegram
    telegramBufferReceiveWrite->byteBufferKnx = &(telegramBufferReceiveWrite->byteBuffer[0]);
    memset((void*)telegramBufferReceiveWrite->byteBuffer, 0xFF, KNX_TELEGRAM_MAX_LENGTH); // if nothing happens on bus -> 1
    
    
    // debug bin as out PA0
    DDRA |= ( 1 << PA0 );
    
    // bus output PD6
    DDRD |= ( 1 << PD6 );
    
    /* interrupt INT1 - knx in
     *
     * ----+    +----
     *     |    |
     *     +----+
     *     ^
     *     ^here
     */
    GIMSK |= ( 1 << INT1 );     // enable interrupt for PD3 - INT1
    MCUCR |= ( 1 << ISC11 );    // trigger interrupt on falling flank
    PORTD |= ( 1 << PD3 );      // pull up for PD3
    
    
    
    // timer
    TIMSK  |= ( 1 << OCIE1A);   // interrupt on timer compare match
    TCCR1B |= ( 1 << CS11  );   // prescaler 8 -> one timer tick = 1us
    
    // wait till bus is free
    OCR1A   = 375;                // interrupt after 375us
}



// -- on flank fall ----------------------------------------------------
ISR(INT1_vect)
{
    // disable self
    GIMSK &= ~( 1 << INT1 );
    //PORTB |= ( 1 << PB1);   // debug
    
    // what to do
    switch (KnxIO::busStatus)
    {
        case KnxIO::BUS_STATUS_UNDEFINED:
            // signal in timeout -> bus not free
            OCR1A = 375;                // check in 375us again
            TCNT1 = 0;                  // -> restart timer
            TIMSK  |= ( 1 << OCIE1A);   // enable timer interrupt
            break;
    
        case KnxIO::BUS_STATUS_FREE:
            // free bus -> start bit received => start byte timer
            // PORTB |= ( 1 << PB1);   // debug
            KnxIO::setBusStatus(KnxIO::BUS_STATUS_START_BIT_RECEIVED);
            OCR1A  = 1040;              //  timer interrupt when byte end -> 10*104us
            TCNT1  = 0;                 // -> restart timer
            TIMSK  |= ( 1 << OCIE1A);   // enable timer interrupt
            break;
    
        case KnxIO::BUS_STATUS_START_BIT_RECEIVED:
        {
            // in read byte mode -> 0 received
            // check at which bit we are -> via time gone since start of byte (-time of startBit)
            int bitPos = (TCNT1 - 100 /* offset of startByte (only 100us) */ ) / 104 /* length of one bit */;
            if (bitPos < 8)
            {
                PORTB |= ( 1 << PB1);   // debug
                KnxIO::bitNullCounter++; // for parity check
                (*(*KnxIO::telegramBufferReceiveWrite).byteBufferKnx) &= ~(1 << (bitPos));    // write 0 to byte
            }
            else if (bitPos == 8)
            {
                // parity/control bit
                // if amount of one/zero is odd -> partyBit = 1
                KnxIO::parityBit = false;
            }
            else
            {
                // end of byte -> next byte
                // -> catched by timer interrupt
            }
            break;
        }
    
        case KnxIO::BUS_STATUS_WAIT_FOR_NEXT_BYTE:
        {
            // break for next byte max 370us
            if (TCNT1 < 370)
            {
                //PORTB |= ( 1 << PB1);   // debug
                // start bit received -> ok start of next byte
                KnxIO::setBusStatus(KnxIO::BUS_STATUS_START_BIT_RECEIVED);
                OCR1A = 1040;              //  timer interrupt when byte end -> 10*104us
                TCNT1 = 0;                 // -> restart timer
                KnxIO::telegramBufferReceiveByteBufferNext(); // write to next byte of telegram
            }
            else
            {
                // startBit of next telegram received
                // -> catched by timer interrupt
            }
            break;
        }
        
        // send
        case KnxIO::BUS_STATUS_SENDING:
            // check if bus low equals sending bit
            if (KnxIO::sending != KNX_SENDING_LOW)
            {
                // collision
                KnxIO::sendCollision();
            }
            break;
    }
    
    PORTB &= ~( 1 << PB1);   // debug
    
    // enable self
    GIMSK |= ( 1 << INT1 );
}



// -- on timer compare match --------------------------------------------
ISR(TIMER1_COMPA_vect)
{
    // disable self
    TIMSK  &= ~( 1 << OCIE1A);
    // save timer
    int timer = OCR1A;
    
    switch (KnxIO::busStatus)
    {
        // receive ------------------------------------------------------------
        case KnxIO::BUS_STATUS_UNDEFINED:
            // timeout of 370(+5)us without flank fall -> bus is free
            if (PORTD & ( 1 << PD3 ))
                KnxIO::setBusStatus(KnxIO::BUS_STATUS_FREE);
            //PORTB |= ( 1 << PB1);   // debug
            break;
    
        case KnxIO::BUS_STATUS_START_BIT_RECEIVED:
            // interrupt at end of byte -> now break of 3 bit (3*104)
            //PORTB &= ~( 1 << PB1);   // debug
            TCNT1 = 0;      // -> restart timer
            KnxIO::setBusStatus(KnxIO::BUS_STATUS_WAIT_FOR_NEXT_BYTE);
            OCR1A = 370;    // interrupt after timeout for next byte -> then bus is free
            break;
    
        case KnxIO::BUS_STATUS_WAIT_FOR_NEXT_BYTE:
            // interrupt after timeout for next byte -> telegram end -> bus is free
            KnxIO::setBusStatus(KnxIO::BUS_STATUS_FREE);
            
            // next byte
            KnxIO::telegramBufferReceiveNext();  // next telegram
            break;
            
        
        // sending --------------------------------------------------------------
        case KnxIO::BUS_STATUS_SENDING:
            //PORTB |= ( 1 << PB1);   // debug
            switch (KnxIO::busStatusSend)
            {
                case KnxIO::BUS_STATUS_SEND_START_BIT:
                    if (KnxIO::sending == KNX_SENDING_HIGH)
                    {
                        // start of byte
                        // reset timer
                        TCNT1 = 0;
    
                        // start sending 0
                        PORTD |= (1 << PD6);
                        KnxIO::sending = KNX_SENDING_LOW;
                        OCR1A  = 30; // in 35us to high again
                        break;
                    }
                    // interrupt after 34us
                    if (KnxIO::sending == KNX_SENDING_LOW)
                    {
                        PORTD &= ~(1 << PD6); // end of startBit on
                        KnxIO::sending = KNX_SENDING_HIGH;
                        // next interrupt at next bit
                        KnxIO::busStatusSend = KnxIO::BUS_STATUS_SEND_DATA;
                        OCR1A = timer+ 69; // next bit after (104-35)=69us
                        break;
                    }
                    break;
                    
    
                case KnxIO::BUS_STATUS_SEND_DATA:
                    if (KnxIO::sendBitNumber < 8)
                    {
                        // if on start of bit
                        if (KnxIO::sending == KNX_SENDING_HIGH)
                        {
                            // if bit = null
                            if (!(((*KnxIO::telegramSend.byteBufferKnx) & (1 << KnxIO::sendBitNumber) ) > 0))
                            {
                                // start sending 0
                                PORTD |= (1 << PD6);
                                KnxIO::sendBitNullCounter++;
                            }
                            KnxIO::sending = KNX_SENDING_LOW;
                            OCR1A = timer+ 35; // in 35us to high again
                            break;
                        }
                        // if after 34us on bit
                        if (KnxIO::sending == KNX_SENDING_LOW)
                        {
                            // start sending 1
                            PORTD &= ~(1 << PD6);
                            KnxIO::sending = KNX_SENDING_HIGH;
                            KnxIO::sendBitNumber++; // next bit
                            OCR1A = timer+ 69; // next bit after (104-35)=69us
                            break;
                        }
                    }
                    else
                    {
                        // parity bit
                        // if on start of bit
                        if (KnxIO::sending == KNX_SENDING_HIGH)
                        {
                            // end of data
                            // now parity bit
                            if (!(KnxIO::sendBitNullCounter & 1))
                            {
                                // start sending 0
                                PORTD |= (1 << PD6);
                            }
                            KnxIO::sending = KNX_SENDING_LOW;
                            OCR1A = timer+ 35; // in 35us to high again
                            break;
                        }
                        // if after 34us on bit
                        if (KnxIO::sending == KNX_SENDING_LOW)
                        {
                            // start sending 1
                            PORTD &= ~(1 << PD6);
                            KnxIO::sending = KNX_SENDING_HIGH;
                            KnxIO::busStatusSend = KnxIO::BUS_STATUS_SEND_START_BIT;
                            // end of byte
                            // next after break of 312us + 69us(rest of parity bit)
                            if (KnxIO::telegramSendByteBufferNext())
                                OCR1A = timer+ 381;
                            else
                                TIMSK  &= ~( 1 << OCIE1A);   // disable timer interrupt
                            break;
                        }
                    }
                    break;
            }
            break;
    }
    
    //PORTB &= ~( 1 << PB1);   // debug
    
    // enable self
    TIMSK  |= ( 1 << OCIE1A);
}


// -- next byte --------------------------------------------------------
void KnxIO::telegramBufferReceiveNext()
{
    // check last byte
    telegramBufferReceiveCheckByteParity();
    
    /*
    char out[100];
    int byte = getTelegramBufferSize((Telegram*)telegramBufferReceiveWrite);
    Log::log("get byte %d  %s", byte, Log::bytesToChar((char*)telegramBufferReceiveWrite->byteBuffer,  byte, out));
    */
     
    // unlock
    telegramBufferReceiveWrite->readLock = false;
    
    // next, if at end reset to begin of ringbuffer
    // ignore acknowledge packets
    if ((telegramBufferReceiveWrite->byteBufferKnx - telegramBufferReceiveWrite->byteBuffer +1) > 1)
    {
        telegramBufferReceiveWrite++;
        if (telegramBufferReceiveWrite == telegramBufferReceiveEnd)
            telegramBufferReceiveWrite = telegramBufferReceive;
    }
    
    // reset telegram
    telegramBufferReceiveWrite->byteBufferKnx = &(telegramBufferReceiveWrite->byteBuffer[0]);
    memset((void*)telegramBufferReceiveWrite->byteBuffer, 0xFF, KNX_TELEGRAM_MAX_LENGTH); // if nothing happens on bus -> 1
    telegramBufferReceiveWrite->parityBitsOk = true;
    
    // lock while writing to buffer
    telegramBufferReceiveWrite->readLock = true;

}
int byteCounter =0;

void KnxIO::telegramBufferReceiveByteBufferNext()
{
    // check last byte
    telegramBufferReceiveCheckByteParity();
    
    // next
    (*telegramBufferReceiveWrite).byteBufferKnx++;
    byteCounter++;
}

// -- check parity bit
void KnxIO::telegramBufferReceiveCheckByteParity()
{
    // check parity/control bit of last byte
    // if amount of one/zero is odd -> partyBit = 1
    if (((bitNullCounter & 1) != parityBit))
    {
        telegramBufferReceiveWrite->parityBitsOk = false;
    }
    
    // next byte
    // null counter = 0
    bitNullCounter = 0;
    parityBit = true;
}

// -- get telegram size
int KnxIO::getTelegramBufferSize(Telegram *telegram)
{
    return telegram->byteBufferKnx - telegram->byteBuffer +1;
}


// -- send
bool KnxIO::sendTelegram(char *bytes, int size)
{
    // if last telegram is still sending
    if (telegramSending)
        return false;
    
    // copy data into telegram
    memcpy((void*)telegramSend.byteBuffer, bytes, size);
    sendByteMax = size;
    //char raw[10*size +1];
    //Log::log("---- sending %d byte, Raw: %s\n\r", size, Log::bytesToChar(bytes, size, raw));
    
    // reset all
    sendBitNumber =0;
    sendBitNullCounter =0;
    telegramSend.byteBufferKnx = telegramSend.byteBuffer;
    
    // check if bus is free
    if (busStatus != BUS_STATUS_FREE)
        return false;
    
    
    // reset timer to start of telegram
    TCNT1 = 0;
    OCR1A = 5; // pin on
    TIMSK  |= ( 1 << OCIE1A);   // enable timer interrupt
    
    
    // start of telegram
    busStatus     = BUS_STATUS_SENDING;
    busStatusSend = BUS_STATUS_SEND_START_BIT;
    sending       = KNX_SENDING_HIGH; // now bus sending 1
    telegramSending = true;
}

bool KnxIO::telegramSendByteBufferNext()
{
    telegramSend.byteBufferKnx++;
    sendBitNumber = 0;
    sendBitNullCounter =0;
    // if at end of telegram
    if ((telegramSend.byteBufferKnx - telegramSend.byteBuffer) >= sendByteMax)
    {
        busStatus = BUS_STATUS_FREE;
        busStatusSend = BUS_STATUS_SEND_NOT;
        telegramSending = false;
        return false;
    }
    return true;
}

// on collision
bool KnxIO::sendCollision()
{
    
    // abort sending
    busStatus     = BUS_STATUS_UNDEFINED;
    busStatusSend = BUS_STATUS_SEND_NOT;

    // wait till bus is free
    TCNT1 = 0;
    OCR1A   =  375;                // interrupt after 375us
}


// set bus status
void KnxIO::setBusStatus(volatile BUS_STATUS status)
{
    // if there is something to send -> start
    if ((status == BUS_STATUS_FREE) && telegramSending)
    {
        /*
        PORTB |= ( 1 << PB1);   // debug
        // reset timer to start of telegram
        TCNT1 = 0;
        OCR1A = 5; // pin on
        TIMSK |= (1 << OCIE1A);   // enable timer interrupt
    
    
        // start of telegram
        busStatus = BUS_STATUS_SENDING;
        busStatusSend = BUS_STATUS_SEND_START_BIT;
        sending = KNX_SENDING_HIGH; // now bus sending 1
        PORTB &= ~( 1 << PB1);   // debug
        return;
        */
    }
    
    busStatus = status;
}