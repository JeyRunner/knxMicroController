/* 
 * File:   Log.cpp
 * Author: Joshua Johannson
 *
 * 
 * --------------------------------------
 * Log CLASS
 * @...
 * --------------------------------------
 */

#include "Log.h"


// static
char Log::logLevelMask    = LOG_LEVEL_ALL;
bool Log::uartInitReady   = false;

volatile char  Log::sendBuffer[];
volatile char *Log::sendBufferEnd;
volatile char *Log::sendBufferWrite;
volatile char *Log::sendBufferRead;


/*
// create object
Log::Log()
{
    if (!uartInitReady)
    {
        init();
        uartInitReady = true;
    }
}*/

// set up uart
void Log::init()
{
    sendBufferRead  = sendBuffer;
    sendBufferWrite = sendBuffer;
    sendBufferEnd   = sendBuffer + LOG_WRITE_BUFFER_SIZE -1;
    
    // UART
#ifdef ATMEGA
    UBRRH = UBRRH_VALUE;                            // speed via macro in setbaud.h -> takes BAUD value
    UBRRL = UBRRL_VALUE;                            // speed
    
    UCSRB |= ( 1 <<  TXEN) | ( 1 <<  RXEN);         // enable send, receive
    //UCSRB |= ( 1 <<  UDRIE);                        // interrupt for send-ready
    UCSRC = (1<<URSEL)|(1 << UCSZ1)|(1 << UCSZ0);   // format setting: async 8N1
#elif ATXMEGA
#define BAUD 9600
#include <util/setbaud.h>
    USARTC0.BAUDCTRLB = UBRRH_VALUE; // speed via macro in setbaud.h -> takes BAUD value
    USARTC0.BAUDCTRLA = UBRRL_VALUE;

    USARTC0.CTRLB = USART_TXEN_bm | USART_RXEN_bm; // enable send, receive
    USARTC0.CTRLC = USART_CHSIZE_8BIT_gc;          // 8bit per character

    PORTC.DIRSET |= PIN3_bm;  // PC3 as tx -> send out
#endif
}

// interrupt if send ready
ISR(USART_UDRE_vect)
{
    Log::sendNextFromCharsToSend();
}
void Log::sendNextFromCharsToSend()
{
  /*
    // disable self
    UCSRB &= ~( 1 <<  UDRIE);
    
    
    // if something to send
    if (sendBufferRead != sendBufferWrite)
    {
        UDR = *sendBufferRead;
        sendBufferRead++;
    
        // reset pointer when at end of buffer
        if (sendBufferRead >= sendBufferEnd)
        {
            sendBufferRead = sendBuffer;
        }
        
        // enable self
        UCSRB |= ( 1 <<  UDRIE);
    }
    
    
    
    // something to send
    /*
    if (charsToSend.size() > 0)
    {
        // set next char to output register
        /*
        char contend = head->contend;
        Element* next = head->next;
        free(&head);
        head = next;
        
        UDR = contend;
        size--;
         *d/
        UDR = charsToSend.pop();
    }
    else
    {
        // UDR = '-';
    }*/
}


void Log::uartPutByte(char c)
{
    
    // wait until ready
#ifdef ATMEGA
    while (!(UCSRA & (1 << UDRE)));
    UDR = c;
#elif ATXMEGA
    while (!( USARTC0.STATUS & USART_DREIF_bm));
    USARTC0.DATA = c;
#endif
    
    /*
    *sendBufferWrite = c;
    sendBufferWrite++;
    
    // reset pointer when at end of buffer
    if (sendBufferWrite >= sendBufferEnd)
    {
        sendBufferWrite = sendBuffer;
    }*/
}


/*
// -- SET LOG NAME ----------------------
void Log::setLogName(char* nameShort)
{
    this->logNameShort = nameShort;
}

void Log::setLogLevel(char log_levelMask)
{
    logLevelMask = log_levelMask;
}
*/

void Log::log(char *format, ...)
{
    va_list args;
    va_start(args, format);
    
    char out[256];
    int size = vsnprintf(out, sizeof(out), format, args);
    
    // disable self
    //UCSRB &= ~( 1 <<  UDRIE);
    for (int i = 0; i < size; ++i)
    {
        //_delay_ms(10);
        uartPutByte(out[i]);
    }
    
    // enable interrupt
    //UCSRB |= ( 1 <<  UDRIE);
    
    va_end(args);
}

/*
// -- OUT -------------------------------
void Log::err(char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    if ((logLevelMask & LOG_LEVEL_ERROR) > 0)
        print("[ err  ] " , args, format);
    
    va_end(args);
}

void Log::warn(char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    if ((logLevelMask & LOG_LEVEL_WARNING) > 0)
        print("[ warn ] " , args, format);
}

void Log::ok(char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    if ((logLevelMask & LOG_LEVEL_OK) > 0)
        print("[  ok  ] " , args, format);
    
    va_end(args);
}


void Log::info(char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    if ((logLevelMask & LOG_LEVEL_INFORMATION) > 0)
        print("[ info ] " , args, format);
    
    va_end(args);
}


void Log::out(bool error, char* format, ...)
{
    va_list args;
    va_start(args, format);
    
    if (error && ((logLevelMask & LOG_LEVEL_ERROR) > 0))
         print("[ err  ] " , args, format);
     else
    {
        if ((logLevelMask & LOG_LEVEL_OK) > 0)
            print("[  ok  ] " , args, format);
    }
    
    va_end(args);
}



void Log::print(char* tag, va_list args, char* format)
{
    char out[256];
    char outFinal[sizeof(out)+ sizeof("[  ]") +sizeof(logNameShort)+ sizeof(tag)+ sizeof("\n\r")];
    vsnprintf(out, sizeof(out), format, args);
    
    strcpy(outFinal, "[ ");
    strcat(outFinal, logNameShort);
    strcat(outFinal, " ]");
    strcat(outFinal, tag);
    strcat(outFinal, out);
    strcat(outFinal, "\n\r");
    
    
    // disable interrupt
    UCSRB &= ~( 1 <<  UDRIE);
    
    for (int i = 0; i < strlen(outFinal); ++i)
    {
        uartPutByte(outFinal[i]);
    }
    
    // enable interrupt
    UCSRB |= ( 1 <<  UDRIE);
}
*/
void Log::sendUart(char *text)
{
    // lock interrupt -> no access while setting value
    //ATOMIC_BLOCK(ATOMIC_FORCEON)
    {
        for (int i = 0; i < strlen(text); ++i)
        {
            /*
            // wait until ready
            while (!(UCSRA & (1 << UDRE)))
            {}
            UDR = text[i];
             */
            
            //charsToSend.push(text[i]);
        }
    }
}

char* Log::byteToChar(int x, char* string)
{
    char *outBuffer = string;
    
    for(int n=0; n<8; n++)
    {
        if((x & 0x80) !=0)
        {
            *outBuffer = '1';
        }
        else
        {
            *outBuffer = '0';
        }
        if (n==3)
        {
            outBuffer++;
            *outBuffer = '-'; /* insert a space between nybbles */
        }
        x = x<<1;
        outBuffer++;
    }
    *outBuffer = '\0';
    return  string;
}


char* Log::bytesToChar(char *bytes, int size, char* string)
{
    // print received bytes
    char *outBuff = string;
    
    for (int i = 0; i < size; ++i)
    {
        char tmp[10];
        byteToChar(bytes[i], tmp);
        tmp[9] = ' ';
        memcpy(outBuff, tmp, 10);
        outBuff += 10;
    }
    *outBuff = '\0';
    return string;
}