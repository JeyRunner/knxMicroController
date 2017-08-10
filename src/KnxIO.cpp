//
// Created by joshua on 10.08.16.
//

#include <avr/io.h>
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

#ifdef ATMEGA
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
#else
    // set IO
    PORTA.DIRSET &= ~(PIN0_bm); // PA0 as bus in
    PORTA.DIRSET |=  (PIN1_bm); // PA1 as bus out
    PORTA.DIRSET |=  (PIN2_bm); // PA3 as debug

    /* interrupt INT0 - knx in
     *
     * ----+    +----
     *     |    |
     *     +----+
     *     ^
     *     ^here
     */
    PORTA.INT0MASK |= PIN0_bm; // enable interrupt for PA0 -> INT0
    PORTA.PIN0CTRL |= PORT_OPC_PULLUP_gc | PORT_ISC_FALLING_gc; // PA0 - pull up, interrupt on falling edge
    PORTA.INTCTRL  |= PORT_INT0LVL_HI_gc;

    // enable low, medium, high level interrupt
    PMIC.CTRL |= PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;

    // timer
    setDebugPin(1);
    _delay_us(50);
    setDebugPin(0);
    //_delay_us(30);

    KnxIO::setTimerInterrupt(50);
    KnxIO::setTimerVal(0);

    TCC0.CTRLA = TC_CLKSEL_DIV8_gc; // prescaler 8
    TCC0.CTRLB = 0x00;              // select Modus: Normal
    TCC0.INTCTRLB= TC_CCAINTLVL_HI_gc;




#endif

    // wait till bus is free
    //KnxIO::setTimerInterrupt(375);                // interrupt after 375us
}


// -- TIMER / IO ------------------------------------------------------------
void KnxIO::setTimerVal(int us)
{
#ifdef ATMEGA
    TCNT1  = us;
#else
    TCC0.CNT = us*4;
#endif
}

void KnxIO::setTimerInterrupt(int us)
{
#ifdef ATMEGA
    OCR1A  = us;
#else
    TCC0.CCA = us*4;
#endif
}

int KnxIO::getTimerInterrupt()
{
#ifdef ATMEGA
    return OCR1A;
#else
    return TCC0.CCA/4;
#endif
}

int KnxIO::getTimerVal()
{
#ifdef ATMEGA
    return TCNT1;
#else
    return TCC0.CNT/4;
#endif
}

void KnxIO::setDebugPin(bool on)
{
#ifdef ATMEGA
      if (on)
        PORTB |= ( 1 << PB1);
      else
        PORTB &= ~( 1 << PB1);
#else
    if (on)
        PORTA.OUTSET = PIN2_bm;
    else
        PORTA.OUTCLR = PIN2_bm;
#endif
}

bool KnxIO::getInPin()
{
#ifdef ATMEGA
    return !(PORTD & ( 1 << PD3 ));
#else
    return !(PORTA.IN & (PIN0_bm)); // PA0 as bus in
#endif
}

void KnxIO::setOutPin(bool on)
{
#ifdef ATMEGA
    if (!on)
        PORTD |= (1 << PD6);    // out to high -> bus to low
    else
        PORTD &= ~(1 << PD6);   // out to low -> bus to high
#else
    if (!on)
        PORTA.OUTSET = PIN1_bm;
    else
        PORTA.OUTCLR = PIN1_bm;
#endif
}

void KnxIO::interruptTimerEnable(bool on)
{
#ifdef ATMEGA
    if (on)
        TIMSK  |= ( 1 << OCIE1A);
    else
        TIMSK  &= ~( 1 << OCIE1A);
#else
    if (on)
        TCC0.INTCTRLB= TC_CCAINTLVL_HI_gc; // enable with prescaler 8
    else
        TCC0.INTCTRLB= TC_CCAINTLVL_OFF_gc;
#endif
}
void KnxIO::interruptInPinEnable(bool on)
{
#ifdef ATMEGA
    if (on)
        GIMSK |= ( 1 << INT1 );
    else
        GIMSK &= ~( 1 << INT1 );
#else
#endif
}





// -- on flank fall ----------------------------------------------------
#ifdef ATMEGA
ISR(INT1_vect)
#else
ISR(PORTA_INT0_vect)
#endif
{
    KnxIO::setDebugPin(1);

    // disable self
    KnxIO::interruptInPinEnable(false);
    //PORTB |= ( 1 << PB1);   // debug

    // what to do
    switch (KnxIO::busStatus)
    {
        case KnxIO::BUS_STATUS_UNDEFINED:
            // signal in timeout -> bus not free
            KnxIO::setTimerInterrupt(375);      // check in 375us again
            KnxIO::setTimerVal(0);              // -> restart timer
            KnxIO::interruptTimerEnable(true);  // enable timer interrupt
            break;

        case KnxIO::BUS_STATUS_FREE:
            // free bus -> start bit received => start byte timer
            // PORTB |= ( 1 << PB1);   // debug
            KnxIO::setBusStatus(KnxIO::BUS_STATUS_START_BIT_RECEIVED);
            KnxIO::setTimerInterrupt(1040);     //  timer interrupt when byte end -> 10*104us
            KnxIO::setTimerVal(0);              // -> restart timer
            KnxIO::interruptTimerEnable(true);  // enable timer interrupt
            break;

        case KnxIO::BUS_STATUS_START_BIT_RECEIVED:
        {
            // in read byte mode -> 0 received
            // check at which bit we are -> via time gone since start of byte (-time of startBit)
            int bitPos = (KnxIO::getTimerVal() - 100 /* offset of startByte (only 100us) */ ) / 104 /* length of one bit */;
            if (bitPos < 8)
            {
                //KnxIO::setDebugPin(1);   // debug
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
            if (KnxIO::getTimerVal() < 370)
            {
                //PORTB |= ( 1 << PB1);   // debug
                // start bit received -> ok start of next byte
                KnxIO::setBusStatus(KnxIO::BUS_STATUS_START_BIT_RECEIVED);
                KnxIO::setTimerInterrupt(1040); //  timer interrupt when byte end -> 10*104us
                KnxIO::setTimerVal(0);          // -> restart timer
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

    KnxIO::setDebugPin(0);// debug

    // enable self
    KnxIO::interruptInPinEnable(true);
}



// -- on timer compare match --------------------------------------------
#ifdef ATMEGA
ISR(TIMER1_COMPA_vect)
#else
ISR(TCC0_CCA_vect)
#endif
{
    KnxIO::setDebugPin(1);

    // disable self
    KnxIO::interruptTimerEnable(false);
    // save timer
    int timer = KnxIO::getTimerInterrupt();

    switch (KnxIO::busStatus)
    {
        // receive ------------------------------------------------------------
        case KnxIO::BUS_STATUS_UNDEFINED:
            // timeout of 370(+5)us without flank fall -> bus is free
            if (!KnxIO::getInPin()) // bus still high
                KnxIO::setBusStatus(KnxIO::BUS_STATUS_FREE);
            //PORTB |= ( 1 << PB1);   // debug
            break;

        case KnxIO::BUS_STATUS_START_BIT_RECEIVED:
            // interrupt at end of byte -> now break of 3 bit (3*104)
            //PORTB &= ~( 1 << PB1);          // debug
            KnxIO::setTimerVal(0);            // -> restart timer
            KnxIO::setBusStatus(KnxIO::BUS_STATUS_WAIT_FOR_NEXT_BYTE);
            KnxIO::setTimerInterrupt(370);    // interrupt after timeout for next byte -> then bus is free
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
                        KnxIO::setTimerVal(0);

                        // start sending 0
                        KnxIO::setOutPin(0);
                        KnxIO::sending = KNX_SENDING_LOW;
                        KnxIO::setTimerInterrupt(30); // in 35us to high again
                        break;
                    }
                    // interrupt after 34us
                    if (KnxIO::sending == KNX_SENDING_LOW)
                    {
                        KnxIO::setOutPin(1); // end of startBit on
                        KnxIO::sending = KNX_SENDING_HIGH;
                        // next interrupt at next bit
                        KnxIO::busStatusSend = KnxIO::BUS_STATUS_SEND_DATA;
                        KnxIO::setTimerInterrupt(timer+ 69); // next bit after (104-35)=69us
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
                                KnxIO::setOutPin(0);
                                KnxIO::sendBitNullCounter++;
                            }
                            KnxIO::sending = KNX_SENDING_LOW;
                            KnxIO::setTimerInterrupt(timer+ 35); // in 35us to high again
                            break;
                        }
                        // if after 34us on bit
                        if (KnxIO::sending == KNX_SENDING_LOW)
                        {
                            // start sending 1
                            KnxIO::setOutPin(1);
                            KnxIO::sending = KNX_SENDING_HIGH;
                            KnxIO::sendBitNumber++; // next bit
                            KnxIO::setTimerInterrupt(timer+ 69); // next bit after (104-35)=69us
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
                                KnxIO::setOutPin(0);
                            }
                            KnxIO::sending = KNX_SENDING_LOW;
                            KnxIO::setTimerInterrupt(timer+ 35); // in 35us to high again
                            break;
                        }
                        // if after 34us on bit
                        if (KnxIO::sending == KNX_SENDING_LOW)
                        {
                            // start sending 1
                            KnxIO::setOutPin(1);
                            KnxIO::sending = KNX_SENDING_HIGH;
                            KnxIO::busStatusSend = KnxIO::BUS_STATUS_SEND_START_BIT;
                            // end of byte
                            // next after break of 312us + 69us(rest of parity bit)
                            if (KnxIO::telegramSendByteBufferNext())
                                KnxIO::setTimerInterrupt(timer+ 381);
                            else
                                KnxIO::interruptTimerEnable(false);   // disable timer interrupt
                            break;
                        }
                    }
                    break;
            }
            break;
    }

    KnxIO::setDebugPin(0);   // debug

    // enable self
    KnxIO::interruptTimerEnable(true);
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
    KnxIO::setTimerVal(0);
    KnxIO::setTimerInterrupt(5); // pin on
    KnxIO::interruptTimerEnable(true);   // enable timer interrupt


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
    KnxIO::setTimerVal(0);
    KnxIO::setTimerInterrupt(375);  // interrupt after 375us
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