/* 
 * File:   Log.cpp
 * Author: Joshua Johannson
 *
 * 
 * --------------------------------------
 * Log CLASS
 * loggable object
 * --------------------------------------
 */


#ifndef _KNX_LOG_H
#define _KNX_LOG_H
#define BAUD 9600UL
#define LOG_READ_BUFFER_SIZE  256
#define LOG_WRITE_BUFFER_SIZE 0

// include
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/setbaud.h>

template <class T>
class Queue;

/* Log class
 */
class Log
{
    public:
        static void init();
        /*
        Log();
        
        static void setLogLevel(char log_level_mask);
        void setLogName(char* nameShort); // set log name example: class name
        */
        static void log(char *format, ...);  // without tagging
        /*
        void err(char* format, ...);  // error message
        void warn(char* format, ...); // warning message
        void ok(char* format, ...);   // ok message
        void info(char* format, ...); // information message
        void out(bool error, char* format, ...); // ok/err - also print error if ok=false
         */
    
        static void sendNextFromCharsToSend(); // only send if send read, if not -> function is called later again by interrupt
    
        static char* byteToChar(int x, char* string);
        static char* bytesToChar(char* bytes, int size, char* string);
    
        static void uartPutByte(char c);
    
    private:
        char* logNameShort;
        static char logLevelMask;
        static bool uartInitReady;
    
        static volatile char  sendBuffer[LOG_WRITE_BUFFER_SIZE];
        static volatile char *sendBufferWrite;
        static volatile char *sendBufferRead;
        static volatile char *sendBufferEnd;
        
        
        
        static void sendUart(char* text);
        //void print(char* tag, va_list args, char* format);
};



/* LOG LEVEL
 */
#define LOG_LEVEL_NOTHING       0b00000000
#define LOG_LEVEL_ERROR         0b00000001
#define LOG_LEVEL_WARNING       0b00000010
#define LOG_LEVEL_OK            0b00000100
#define LOG_LEVEL_DEBUG         0b00001000
#define LOG_LEVEL_INFORMATION   0b00010000
#define LOG_LEVEL_TRACE         0b00100000
#define LOG_LEVEL_ALL           0b11111111


template <typename T>
char* str(T value, char * format)
{
    char *out;
    snprintf(format, value, out);
    return out;
}
#endif /* _KNX_LOG_H */
