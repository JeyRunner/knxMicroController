#define F_CPU 8000000UL // 8MHz
#define BAUD 9600UL
#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <avr/eeprom.h>
#include <Log.h>

// set knx vars
#define KNX_TIMER_INTERRUPT_VECT TIMER1_COMPA_vect
#define KNX_INPORT_INTERRUPT_VECT INT1_vect
#include "Knx.h"


// go
int main()
{
    Log::init();
    Log::log("\n\r----------------------------------------------------------\n\r");
    Log::log("start program\n\r"); //%d", start);
    
    // IO
    DDRB |=  ( 1 << PB1 ); // PB1 -> out
    DDRD &= ~( 1 << PD3 ); // PD3 -> in
    
    
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
            //free(&knxPacket);
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

      _delay_ms(30);
    }
}