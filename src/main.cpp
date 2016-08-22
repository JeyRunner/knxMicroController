#define F_CPU 8000000UL // 8MHz
#define BAUD 9600UL
#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <avr/eeprom.h>
#include <Log.h>
#include <Queue.h>
#include "Knx.h"

//uint8_t startCounter EEMEM;

char motorSteps[4] = {0b0001, 0b0010, 0b0100, 0b1000};
char motorStepAt = 0;

void motorDoSteps(int steps)
{
    bool direction = true;
    if (steps < 0)
    {
        direction = false;
        steps = -steps;
    }
    
    for (int i = 0; i < steps; ++i)
    {
        PORTA = (PORTA & ~0b00011110) | (motorSteps[motorStepAt] << 1);
    
        if (direction)
            motorStepAt++;
        else
            motorStepAt--;
        
        if (motorStepAt >= 4)
            motorStepAt = 0;
        if (motorStepAt < 0)
            motorStepAt = 3;
        
        _delay_ms(30);
    }
}


// go
int main()
{
    Log::init();
    Log::log("\n\r----------------------------------------------------------\n\r");
    Log::log("start program\n\r"); //%d", start);
    
    // IO
    DDRB |=  ( 1 << PB1 ); // PB1 -> out
    DDRD &= ~( 1 << PD3 ); // PD3 -> in
    
    // stepper motor
    DDRA |=  ( 1 << PA1 ); // PA1 -> out
    DDRA |=  ( 1 << PA2 ); // PA2 -> out
    DDRA |=  ( 1 << PA3 ); // PA3 -> out
    DDRA |=  ( 1 << PA4 ); // PA4 -> out

    
    
    // pull up for buttons
    PORTC |= ( 1 << PC2 );
    PORTC |= ( 1 << PC3 );
    
    
    // global enable interrupts
    sei();
    
    // knx io
    Knx::init();
    

    
    bool buttonUp = false, buttonDown = false;
    int counter = 1;
    while(true)
    {
        
        // wait 50ms
        // _delay_ms(100);

        if (Knx::getPacketsReceivedAmount() > 0)
        {
            // next knx packet
            KnxPacket knxPacket = Knx::getNextPacketBlocking();
    
            char main, middle, sub;
            knxPacket.getDestinationGroupAddr(&main, &middle, &sub);
            //Log::log("check pack %d/%d/%d \n\r", main, middle, sub);
            if ((main == (char)4) && (middle == (char)4) && (sub == (char)17))
            {
                if (knxPacket.getDataValueSwitch())
                    PORTB |=  ( 1 << PB1);
                else
                    PORTB &= ~( 1 << PB1);
            }
            //_delay_ms(200);
            Log::log("\n\r\n\r---- Nr.%d\n\r", counter);
            knxPacket.print();
            free(&knxPacket);
            counter++;
        }
        
        // buttons
        if ((PINC & ( 1 << PC2 )))
        {
            if (buttonUp)
            {
                Log::log("-> button up release \n\r");
                KnxPacket packetSend;
                packetSend.setSourceAddr(2,1,17);
                packetSend.setDestinationGroupAddr(3,2,17);
                packetSend.setDataValueSwitch(false);
                packetSend.send();
            }
            buttonUp = false;
        }
        else
        {
            if (!buttonUp)
            {
                Log::log("-> button up press \n\r");
                
                KnxPacket packetSend;
                packetSend.setSourceAddr(2,1,17);
                packetSend.setDestinationGroupAddr(3,1,17);
                packetSend.setDataValueSwitch(false);
                packetSend.send();
            }
            buttonUp = true;
        }
            
    
        if ((PINC & ( 1 << PC3 )))
        {
            if (buttonDown)
            {
                Log::log("-> button down release \n\r");
                KnxPacket packetSend;
                packetSend.setSourceAddr(2,1,17);
                packetSend.setDestinationGroupAddr(3,2,17);
                packetSend.setDataValueSwitch(false);
                packetSend.send();
            }
            buttonDown = false;
        }
        else
        {
            if (!buttonDown)
            {
                Log::log("-> button down press \n\r");
                
                KnxPacket packetSend;
                packetSend.setSourceAddr(2,1,17);
                packetSend.setDestinationGroupAddr(3,1,17);
                packetSend.setDataValueSwitch(true);
                packetSend.send();
            }
            buttonDown = true;
        }
        
        
        motorDoSteps(1);
    }
}